#include "mandelbrotpipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>
#include <QTimer>
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
static const char BITSTREAM_MUX_NAME[] = "mux";

static const int FIXED_NODE_MUX_BEGIN = 2;
static const int FIXED_NODE_MUX_END = 4;
static const unsigned int FIXED_NODE_MUX_COUNT = FIXED_NODE_MUX_END - FIXED_NODE_MUX_BEGIN;

static const unsigned int MAX_DMA_NODES = 2;

static const double DefaultCenterX = -0.86122562296399741;
static const double DefaultCenterY = -0.23139131123653386;
static const double MinScale = 0.0000000000001;
static const double DefaultScale = 0.005;
static const double ZoomInFactor = 0.950;

/* At what position in the "line" is the worker index. */
#define WORKER_INDEX_SHIFT 12
/* At what position is the image index */
#define WORKER_IMAGE_SHIFT 11

#define LINE_MASK ((1 << WORKER_IMAGE_SHIFT) - 1)

static inline long long to_fixed_point(double v)
{
    return (long long)(v * ((long long)1 << 53));
}

class MandelbrotWorker
{
protected:
    dyplo::HardwareConfig *node;
    dyplo::HardwareFifo *to_logic;
public:
    std::vector<MandelbrotRequest> work_to_do;

    MandelbrotWorker(DyploContext *dyplo);
    ~MandelbrotWorker();
    int getNodeIndex() const;
    void commit_work();
};

MandelbrotPipeline::MandelbrotPipeline(QObject *parent) : QObject(parent),
    video_width(640),
    video_height(480),
    video_lines_per_block(16),
    z(0)
{
    setSize(video_width, video_height);
}

MandelbrotPipeline::~MandelbrotPipeline()
{
    deactivate_impl();
}

bool MandelbrotPipeline::setSize(int width, int height)
{
    if (!outgoing.empty() || !incoming.empty())
        return false; /* Cannot change size while running */
    video_width = width;
    video_height = height;
    video_lines_per_block = 16;
    for (int i = 0; i < 2; ++i)
        rendered_image[i].initialize(width, height);
    return true;
}

int MandelbrotPipeline::activate(DyploContext *dyplo, int max_nodes)
{
    unsigned int connectedNodes = 0;

    z = 0;
    zoomFrame();
    current_scanline = 0;
    current_image = 0;
    for (int i = 0; i < 2; ++i)
        rendered_image[i].lines_remaining = video_height;

    /* Allocate the workers first */
    try
    {
        for (int num_nodes = 0; num_nodes < max_nodes; ++num_nodes)
        {
            MandelbrotWorker *next_outgoing = new MandelbrotWorker(dyplo);
            outgoing.push_back(next_outgoing);
        }
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Mandelbrot pipeline:" << ex.what();
    }

    if (outgoing.empty())
    {
        /* Nothing allocated, cannot start */
        deactivate_impl();
        return -1;
    }

    /* Ideally, create enough work do do just under one frame */
    video_lines_per_block = (video_height / (outgoing.size() + 1)) & 0xFFFFFFFE; /* Round to even number */
    if (video_lines_per_block > 18) /* Cannot queue more than 2*18 commands in hardware */
        video_lines_per_block = 18;
    else if (video_lines_per_block < 2)
        video_lines_per_block = 2;

    /* No muxes needed with up to two workers */
    if (outgoing.size() <= MAX_DMA_NODES)
    {
        try
        {
           for (MandelbrotWorkerList::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
           {
               MandelbrotIncoming *next_incoming = new MandelbrotIncoming(this, dyplo,
                       video_lines_per_block * (video_width + SCANLINE_HEADER_SIZE),
                       (*it)->getNodeIndex());
               incoming.push_back(next_incoming);
           }
           connectedNodes = outgoing.size();
        }
       catch (const std::exception& ex)
       {
           qDebug() << "Mandelbrot cannot allocate DMA:" << ex.what();
           for (MandelbrotIncomingList::iterator it = incoming.begin(); it != incoming.end(); ++it)
               delete *it;
           incoming.clear();
       }
    }

    if (!connectedNodes)
    try
    {
        unsigned int nodes_per_mux = 4;
        /* Create a mux to gather data */
        for (int mux_index = FIXED_NODE_MUX_BEGIN; mux_index < FIXED_NODE_MUX_END; ++mux_index)
        {
            if (connectedNodes == outgoing.size())
                break;
            try
            {
                dyplo::HardwareConfig *next_mux =
                        new dyplo::HardwareConfig(dyplo->GetHardwareContext(), mux_index);
                next_mux->enableNode();
                mux.push_back(next_mux);
            }
            catch (const std::exception& ex)
            {
                qDebug() << __func__ << "Failed to aquire mux" << mux_index;
                continue; /* Try the next one, maybe we're running multiple demos */
            }
            /* Create incoming DMA node */
            MandelbrotIncoming *next_incoming = new MandelbrotIncoming(this, dyplo,
                    video_lines_per_block * (video_width + SCANLINE_HEADER_SIZE),
                    mux_index);
            incoming.push_back(next_incoming);
            /* Connect output nodes  to the mux */
            for (unsigned int input = 0; input < nodes_per_mux; ++input)
            {
                int node_index = outgoing[connectedNodes]->getNodeIndex();
                dyplo->GetHardwareControl().routeAddSingle(node_index, 0, mux_index, input);
                qDebug() << __func__ << "Node" << node_index << "to mux" << mux_index << "input" << input;
                ++connectedNodes;
                if (connectedNodes == outgoing.size())
                    break;
            }
        }
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Mandelbrot pipeline:" << ex.what();
    }
    if (!connectedNodes)
    {
        /* Nothing allocated, cannot start */
        deactivate_impl();
        return -1;
    }
    /* De-allocate nodes that we could not connect to anything */
    while (connectedNodes < outgoing.size())
    {
        delete outgoing.back();
        outgoing.pop_back();
    }
    unsigned int outgoing_size = outgoing.size();
    /* Send out work for one frame or twice the number of workers, whichever is smaller */
    unsigned int lines_to_send = outgoing_size * video_lines_per_block * 2;
    if (lines_to_send > (unsigned int)video_height)
        lines_to_send = video_height;
    qDebug() << "lines_to_send" << lines_to_send << "video_lines_per_block" << video_lines_per_block;
    for (unsigned int line = 0; line < lines_to_send; ++line)
        requestNext(line  % outgoing_size);
    for (MandelbrotWorkerList::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
        (*it)->commit_work();
    emit setActive(true);
    return 0;
}

void MandelbrotPipeline::deactivate_impl()
{
    for (MandelbrotWorkerList::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
        delete *it;
    outgoing.clear();
    for (MandelbrotIncomingList::iterator it = incoming.begin(); it != incoming.end(); ++it)
        delete *it;
    incoming.clear();
    for (HardwareConfigList::iterator it = mux.begin(); it != mux.end(); ++it) {
        (*it)->deleteRoutes();
        (*it)->disableNode();
        delete *it;
    }
    mux.clear();
}

void MandelbrotPipeline::deactivate()
{
    deactivate_impl();
    emit setActive(false);
}

void MandelbrotPipeline::enumDyploResources(DyploNodeInfoList &list)
{
    for (MandelbrotWorkerList::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
        list.push_back(DyploNodeInfo((*it)->getNodeIndex(), BITSTREAM_MANDELBROT));
    for (HardwareConfigList::iterator it = mux.begin(); it != mux.end(); ++it)
        list.push_back(DyploNodeInfo((*it)->getNodeIndex(), BITSTREAM_MUX_NAME));
}

void MandelbrotPipeline::dataAvailable(const uchar *data, unsigned int bytes_used)
{
    unsigned int nlines = bytes_used / (video_width + SCANLINE_HEADER_SIZE);

    if (!nlines)
        return;

    if (bytes_used % (video_width + SCANLINE_HEADER_SIZE))
        qWarning() << "Strange size:" << bytes_used << " Expected:" << (video_width + SCANLINE_HEADER_SIZE) << "Lines:" << nlines;

    for (unsigned int i = 0; i < nlines; ++i)
    {
        unsigned int first_word = ((unsigned int *)data)[0];
        unsigned short line = (unsigned short)first_word;
        unsigned short size = (unsigned short)(first_word >> 16);
        unsigned short image_index = (line >> WORKER_IMAGE_SHIFT) & 1;
        unsigned short worker_index = (line >> WORKER_INDEX_SHIFT);

        line &= LINE_MASK;

        if ((size != video_width) || (line >= video_height) || (worker_index >= outgoing.size())) {
            qWarning() << "Invalid line:" << line << "size:" << size << "worker:" << worker_index;
            /* Abort - things are broken and there's no point in going any further */
            QTimer::singleShot(0, this, SLOT(deactivate()));
            return;
        }
        MandelbrotImage *currentImage = &rendered_image[image_index];
        memcpy(currentImage->image.scanLine(line), data + SCANLINE_HEADER_SIZE, video_width);
        if (--currentImage->lines_remaining == 0) {
            emit renderedImage(currentImage->image);
            currentImage->lines_remaining = video_height;
        }

        requestNext(worker_index);
        data += video_width + SCANLINE_HEADER_SIZE;
    }

    for (MandelbrotWorkerList::iterator it = outgoing.begin(); it != outgoing.end(); ++it)
        (*it)->commit_work();
}

void MandelbrotPipeline::zoomFrame()
{
    z *= ZoomInFactor;
    if (z < MinScale) {
        x = DefaultCenterX;
        y = DefaultCenterY;
        z = DefaultScale;
    }
    fixed_left_x = to_fixed_point(x - ((video_width/2) * z));
    fixed_z = to_fixed_point(z);
}

void MandelbrotPipeline::requestNext(unsigned short worker_index)
{
    MandelbrotRequest request;
    const int half_video_height = video_height / 2;

    request.line = current_scanline | (current_image << WORKER_IMAGE_SHIFT) | (worker_index << WORKER_INDEX_SHIFT);
    request.size = video_width;
    request.ax = fixed_left_x;
    request.ay = to_fixed_point(((current_scanline - half_video_height) * z) + y);
    request.incr = fixed_z;

    outgoing[worker_index]->work_to_do.push_back(request);
    ++current_scanline;
    if (current_scanline == video_height)
    {
        current_scanline = 0;
        current_image ^= 1;
        zoomFrame();
    }
}


MandelbrotIncoming::MandelbrotIncoming(MandelbrotPipeline *parent, DyploContext *dyplo, int blocksize, int node_index):
    pipeline(parent),
    fromLogicNotifier(NULL),
    from_logic(dyplo->createDMAFifo(O_RDONLY)),
    video_blocksize(blocksize)
{
    from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, blocksize, 8, true);
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
        to_logic = new dyplo::HardwareFifo(dyplo->GetHardwareContext().openAvailableWriteFifo()); // .openAvailableDMA(O_WRONLY));
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

void MandelbrotWorker::commit_work()
{
    unsigned int bytes_to_write = work_to_do.size() * sizeof(MandelbrotRequest);
    if (bytes_to_write)
    {
        ssize_t written = to_logic->write(&work_to_do[0], bytes_to_write);
        if (written != bytes_to_write)
            qCritical() << __func__ << "written" << written;
        work_to_do.clear(); /* works with DMA... */
    }
}


void MandelbrotImage::initialize(int width, int height)
{
    lines_remaining = height;
    image = QImage(width, height, QImage::Format_Indexed8);
    image.setColorTable(mandelbrot_color_map);
    image.fill(255);
}
