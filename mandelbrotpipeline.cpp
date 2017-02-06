#include "mandelbrotpipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>
#include <dyplo/hardware.hpp>
#include "dyplocontext.h"
#include "colormap.h"

/* Scanline + 32-bit header*/
#define SCANLINE_HEADER_SIZE 4

struct MandelbrotRequest
{
    unsigned short line;
    unsigned short size;
    long long ax;
    long long ay;
    long long incr;
} __attribute__((packed));

static const char BITSTREAM_MANDELBROT[] = "mandelbrot";

static const double DefaultCenterX = -0.86122562296399741;
static const double DefaultCenterY = -0.23139131123653386;
static const double MinScale = 0.0000000000001;
static const double DefaultScale = 0.005;
static const double ZoomInFactor = 0.950;

/* At what position in the "line" is the worker index. */
#define WORKER_INDEX_SHIFT 11
/* At what position is the image index */
#define WORKER_IMAGE_SHIFT 10

#define LINE_MASK ((1 << WORKER_IMAGE_SHIFT) - 1)

static inline long long to_fixed_point(double v)
{
    return (long long)(v * ((long long)1 << 53));
}


MandelbrotPipeline::MandelbrotPipeline(QObject *parent) : QObject(parent),
    video_width(640),
    video_height(480)
{
    setSize(video_width, video_height);
}

MandelbrotPipeline::~MandelbrotPipeline()
{
    deactivate();
}

bool MandelbrotPipeline::setSize(int width, int height)
{
    if (!outgoing.empty() || !incoming.empty())
        return false; /* Cannot change size while running */
    video_width = width;
    video_height = height;
    video_lines_per_block = (video_height / 8);
    for (int i = 0; i < 2; ++i)
        rendered_image[i].initialize(width, height);
    return true;
}

int MandelbrotPipeline::activate(DyploContext *dyplo)
{
    x = DefaultCenterX;
    y = DefaultCenterY;
    z = DefaultScale;
    current_scanline = 0;
    current_image = 0;
    for (int i = 0; i < 2; ++i)
        rendered_image[i].lines_remaining = video_height;

    try
    {
        for(;;) /* run until failure */
        {
            MandelbrotWorker *next_outgoing = new MandelbrotWorker(dyplo);
            try
            {
                MandelbrotIncoming *next_incoming = new MandelbrotIncoming(this, dyplo,
                        video_lines_per_block * (video_width + SCANLINE_HEADER_SIZE),
                        next_outgoing->getNodeIndex());
                incoming.push_back(next_incoming);
            }
            catch  (const std::exception&)
            {
                // Failed to allocate, clean up.
                delete next_outgoing;
                throw;
            }
            outgoing.push_back(next_outgoing);
        }
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Could not setup Mandelbrot pipeline:" << ex.what();
    }
    if (incoming.empty())
        return -1; /* Nothing allocated, cannot start */
    unsigned int outgoing_size = outgoing.size();
    unsigned int outgoing_index = 0;
    /* Send out work for one frame */
    for (int line = 0; line < video_height; line += video_lines_per_block)
    {
        requestNext(outgoing_index  % outgoing_size, video_lines_per_block);
        ++outgoing_index;
        if (outgoing_index >= 2 * outgoing_size) /* No point in working more than 2 blocks ahead */
            break;
    }
    emit setActive(true);
    return 0;
}

void MandelbrotPipeline::deactivate()
{
    for (MandelbrotWorkerList::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
        delete *it;
    outgoing.clear();
    for (MandelbrotIncomingList::iterator it = incoming.begin(); it != incoming.end(); ++it)
        delete *it;
    incoming.clear();
    emit setActive(false);
}

void MandelbrotPipeline::enumDyploResources(DyploNodeInfoList &list)
{
    for (MandelbrotWorkerList::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
        list.push_back(DyploNodeInfo((*it)->getNodeIndex(), BITSTREAM_MANDELBROT));
}

void MandelbrotPipeline::dataAvailable(const uchar *data, unsigned int bytes_used)
{

    unsigned int nlines = bytes_used / (video_width + SCANLINE_HEADER_SIZE);

    if (!nlines)
        return;

    if (bytes_used % (video_width + SCANLINE_HEADER_SIZE))
        qWarning() << "Strange size:" << bytes_used << " Expected:" << (video_width + SCANLINE_HEADER_SIZE) << "Lines:" << nlines;

    unsigned short worker_index = ((unsigned short *)data)[0] >> WORKER_INDEX_SHIFT;

    qDebug() << __func__ << worker_index << "img" << ((((unsigned short *)data)[0] >> WORKER_IMAGE_SHIFT) & 1)
            << "line" << (((unsigned short *)data)[0] & LINE_MASK) << "+" << nlines;

    for (unsigned int i = 0; i < nlines; ++i)
    {
        unsigned short line = ((unsigned short *)data)[0];
        unsigned short size = ((unsigned short *)data)[1];
        unsigned short image_index = (line >> WORKER_IMAGE_SHIFT) & 1;

        line &= LINE_MASK;

        if ((size != video_width) || (line >= video_height)) {
            unsigned char dummy[256];
            memcpy(dummy, data, sizeof(dummy));
            qWarning() << "Invalid line:" << line << "size:" << size;
            break;
        }
        MandelbrotImage *currentImage = &rendered_image[image_index];
        memcpy(currentImage->image.scanLine(line), data + SCANLINE_HEADER_SIZE, video_width);
        if (--currentImage->lines_remaining == 0) {
            qDebug() << "img" << image_index << "ready";
            emit renderedImage(currentImage->image);
            currentImage->lines_remaining = video_height;
        }

        data += video_width + SCANLINE_HEADER_SIZE;
    }

    requestNext(worker_index, nlines);
}

void MandelbrotPipeline::writeConfig(int outgoing_index, int start_scanline, int number_of_lines, unsigned short extra_line_bits)
{
    MandelbrotRequest req[number_of_lines];
    long long ax = to_fixed_point(x - ((video_width/2) * z));
    long long step = to_fixed_point(z);
    int half_video_height = video_height / 2;

    qDebug() << "Worker" << outgoing_index << "img" << ((extra_line_bits >> WORKER_IMAGE_SHIFT) & 1) << "Req" << start_scanline << "to" << (start_scanline + number_of_lines) << "bits" << extra_line_bits;

    for (int line = 0; line < number_of_lines; ++line)
    {
        int scanline = line + start_scanline;
        req[line].size = video_width;
        req[line].line = scanline | extra_line_bits;
        req[line].ax = ax;
        req[line].ay = to_fixed_point(((scanline - half_video_height) * z) + y);
        req[line].incr = step;
    }
    outgoing[outgoing_index]->request(req, number_of_lines);
}

void MandelbrotPipeline::zoomFrame()
{
    z *= ZoomInFactor;
    if (z < MinScale) {
        x = DefaultCenterX;
        y = DefaultCenterY;
        z = DefaultScale;
    }
}

void MandelbrotPipeline::requestNext(int outgoing_index, int lines)
{
    int next_scanline = current_scanline + lines;
    if (next_scanline > video_height) {
        int l = next_scanline - video_height;
        writeConfig(outgoing_index, current_scanline, l, (outgoing_index << WORKER_INDEX_SHIFT) | (current_image << WORKER_IMAGE_SHIFT));
        zoomFrame();
        current_scanline = 0;
        current_image ^= 1;
        lines -= l;
        next_scanline = lines;
    }
    writeConfig(outgoing_index, current_scanline, lines, (outgoing_index << WORKER_INDEX_SHIFT) | (current_image << WORKER_IMAGE_SHIFT));
    if (next_scanline >= video_height) {
        next_scanline = 0;
        current_image ^= 1;
        zoomFrame();
    }
    current_scanline = next_scanline;
}


MandelbrotIncoming::MandelbrotIncoming(MandelbrotPipeline *parent, DyploContext *dyplo, int blocksize, int node_index):
    pipeline(parent),
    fromLogicNotifier(NULL),
    from_logic(dyplo->createDMAFifo(O_RDONLY)),
    video_blocksize(blocksize)
{
    from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, blocksize, 4, true);
    from_logic->addRouteFrom(node_index);
    /* Prime reader */
    for (unsigned int i = 0; i < from_logic->count(); ++i)
    {
        dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
        block->bytes_used = blocksize;
        from_logic->enqueue(block);
    }
    from_logic->fcntl_set_flag(O_NONBLOCK);

    fromLogicNotifier = new QSocketNotifier(from_logic->handle, QSocketNotifier::Read, this);
    connect(fromLogicNotifier, SIGNAL(activated(int)), this, SLOT(dataAvailable(int)));
    fromLogicNotifier->setEnabled(true);
}

MandelbrotIncoming::~MandelbrotIncoming()
{
    delete fromLogicNotifier;
    fromLogicNotifier = NULL;
    delete from_logic;
    from_logic = NULL;
}

void MandelbrotIncoming::dataAvailable(int)
{
    dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
    if (!block)
        return;

    pipeline->dataAvailable((const uchar *)block->data, block->bytes_used);

    block->bytes_used = video_blocksize;
    from_logic->enqueue(block);
}

MandelbrotWorker::MandelbrotWorker(DyploContext *dyplo):
    node(dyplo->createConfig(BITSTREAM_MANDELBROT))
{
    try
    {
        to_logic = new dyplo::HardwareFifo(dyplo->GetHardwareContext().openAvailableDMA(O_WRONLY));
    }
    catch (const std::exception&)
    {
        /* destructor will not be called on exception, so we have to clean up here */
        delete node;
        throw;
    }

    node->enableNode();
    to_logic->addRouteTo(node->getNodeIndex());
}

MandelbrotWorker::~MandelbrotWorker()
{
    if (to_logic) {
        delete to_logic;
        to_logic = NULL;
    }
    if (node) {
        node->deleteRoutes();
        delete node;
        node = NULL;
    }
}

int MandelbrotWorker::getNodeIndex() const
{
    return node->getNodeIndex();
}

void MandelbrotWorker::request(const void *data, int count)
{
    to_logic->write(data, sizeof(MandelbrotRequest) * count);
}


void MandelbrotImage::initialize(int width, int height)
{
    lines_remaining = height;
    image = QImage(width, height, QImage::Format_Indexed8);
    image.setColorTable(mandelbrot_color_map);
    image.fill(255);
}
