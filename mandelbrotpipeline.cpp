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
static const double DefaultScale = 0.010;
static const double ZoomInFactor = 0.950;
static const double ZoomOutFactor = 1 / ZoomInFactor;


static inline long long to_fixed_point(double v)
{
    return (long long)(v * ((long long)1 << 46));
}


MandelbrotPipeline::MandelbrotPipeline(QObject *parent) : QObject(parent),
    video_width(640),
    video_height(480),
    fromLogicNotifier(NULL),
    from_logic(NULL),
    to_logic(NULL),
    node(NULL)
{
    setSize(video_width, video_height);
}

MandelbrotPipeline::~MandelbrotPipeline()
{
    deactivate();
}

bool MandelbrotPipeline::setSize(int width, int height)
{
    if (node)
        return false; /* Cannot change size while running */
    video_width = width;
    video_height = height;
    /* Process half a frame */
    video_lines_per_block = (video_height / 4);
    video_blocksize = video_lines_per_block * (video_width + SCANLINE_HEADER_SIZE);
    currentImage.lines_remaining = height;
    currentImage.image = QImage(width, height, QImage::Format_Indexed8);
    currentImage.image.setColorTable(mandelbrot_color_map);
    currentImage.image.fill(255);
    return true;
}

int MandelbrotPipeline::activate(DyploContext *dyplo)
{
    try
    {
        node = dyplo->createConfig(BITSTREAM_MANDELBROT);
        from_logic = dyplo->createDMAFifo(O_RDONLY);
        from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, video_blocksize, 4, true);
        from_logic->addRouteFrom(node->getNodeIndex());
        /* Prime reader */
        for (unsigned int i = 0; i < from_logic->count(); ++i)
        {
            dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
            block->bytes_used = video_blocksize;
            from_logic->enqueue(block);
        }
        from_logic->fcntl_set_flag(O_NONBLOCK);

        fromLogicNotifier = new QSocketNotifier(from_logic->handle, QSocketNotifier::Read, this);
        connect(fromLogicNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableDyplo(int)));
        fromLogicNotifier->setEnabled(true);
        node->enableNode();

        to_logic = new dyplo::HardwareFifo(dyplo->GetHardwareContext().openAvailableDMA(O_WRONLY));
        to_logic->addRouteTo(node->getNodeIndex());

        x = DefaultCenterX;
        y = DefaultCenterY;
        z = DefaultScale;
        current_scanline = 0;
        requestNext(video_height);

        currentImage.lines_remaining = video_height;
    }
    catch (const std::exception& ex)
    {
        qCritical() << "Could not setup Mandelbrot pipeline:" << ex.what();
        deactivate(); /* Cleanup */
        return -1; /* Hmm... */
    }
    emit setActive(true);
    return 0;
}

void MandelbrotPipeline::deactivate()
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
    delete fromLogicNotifier;
    fromLogicNotifier = NULL;
    delete from_logic;
    from_logic = NULL;
    //emit renderedImage(QImage()); /* Render an empty image to clear the video screen */
    emit setActive(false);
}

void MandelbrotPipeline::enumDyploResources(DyploNodeInfoList &list)
{
    if (node)
        list.push_back(DyploNodeInfo(node->getNodeIndex(), BITSTREAM_MANDELBROT));
}

void MandelbrotPipeline::frameAvailableDyplo(int)
{
    dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
    if (!block)
        return;

    unsigned int nlines = block->bytes_used / (video_width + SCANLINE_HEADER_SIZE);

    if (block->bytes_used % (video_width + SCANLINE_HEADER_SIZE))
        qWarning() << "Strange size:" << block->bytes_used << " Expected:" << video_blocksize << "Lines:" << nlines;

    const unsigned char* data = (const unsigned char *)block->data;
    for (unsigned int i = 0; i < nlines; ++i)
    {
        unsigned short line = ((unsigned short *)data)[0];
        unsigned short size = ((unsigned short *)data)[1];

        if ((size != video_width) || (line >= video_height)) {
            unsigned char dummy[256];
            memcpy(dummy, data, sizeof(dummy));
            qWarning() << "Invalid line:" << line << "size:" << size;
            break;
        }
        memcpy(currentImage.image.scanLine(line), data + SCANLINE_HEADER_SIZE, video_width);
        if (--currentImage.lines_remaining == 0) {
            emit renderedImage(currentImage.image);
            currentImage.lines_remaining = video_height;
        }

        data += video_width + SCANLINE_HEADER_SIZE;
    }

    requestNext(nlines);
    block->bytes_used = video_blocksize;
    from_logic->enqueue(block);
}

void MandelbrotPipeline::writeConfig(int start_scanline, int number_of_lines)
{
    MandelbrotRequest req[number_of_lines];
    long long ax = to_fixed_point(x - ((video_width/2) * z));
    long long step = to_fixed_point(z);
    int half_video_height = video_height / 2;
    for (int line = 0; line < number_of_lines; ++line)
    {
        int scanline = line + start_scanline;
        req[line].size = video_width;
        req[line].line = scanline;
        req[line].ax = ax;
        req[line].ay = to_fixed_point(((scanline - half_video_height) * z) + y);
        req[line].incr = step;
    }
    to_logic->write(req, sizeof(req));
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

void MandelbrotPipeline::requestNext(int lines)
{
    int next_scanline = current_scanline + lines;
    if (next_scanline > video_height) {
        int l = video_height - next_scanline;
        writeConfig(current_scanline, l);
        zoomFrame();
        current_scanline = 0;
        lines -= l;
        next_scanline = lines;
    }
    writeConfig(current_scanline, lines);
    if (next_scanline >= video_height) {
        next_scanline = 0;
        zoomFrame();
    }
    current_scanline = next_scanline;
}
