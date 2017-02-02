#include "mandelbrotpipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>
#include <dyplo/hardware.hpp>
#include "dyplocontext.h"
#include "colormap.h"

#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 480
/* Scanline + 32-bit header*/
#define VIDEO_SIZE (VIDEO_HEIGHT * (VIDEO_WIDTH + 4))


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
static const double MinScale = 0.000000000000053607859314274247;
static const double DefaultScale = 0.005f;
static const double ZoomInFactor = 0.95f;
static const double ZoomOutFactor = 1 / ZoomInFactor;


static inline long long to_fixed_point(double v)
{
    return (long long)(v * ((long long)2 << 45));
}

static void writeConfig(dyplo::HardwareFifo *fifo, double x, double y, double z)
{
    MandelbrotRequest req[VIDEO_HEIGHT];
    long long ax = to_fixed_point(x - ((VIDEO_WIDTH/2) * z));
    long long step = to_fixed_point(z);
    for (int line = 0; line < VIDEO_HEIGHT; ++line)
    {
        req[line].size = VIDEO_WIDTH;
        req[line].line = line;
        req[line].ax = ax;
        req[line].ay = to_fixed_point(((line - (VIDEO_HEIGHT/2)) * z) + y);
        req[line].incr = step;
    }
    fifo->write(req, sizeof(req));
}


MandelbrotPipeline::MandelbrotPipeline(QObject *parent) : QObject(parent),
    fromLogicNotifier(NULL),
    from_logic(NULL),
    to_logic(NULL),
    node(NULL)
{
}

MandelbrotPipeline::~MandelbrotPipeline()
{
    deactivate();
}

int MandelbrotPipeline::activate(DyploContext *dyplo)
{
    try
    {
        node = dyplo->createConfig(BITSTREAM_MANDELBROT);
        from_logic = dyplo->createDMAFifo(O_RDONLY);
        from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, VIDEO_SIZE, 2, true);
        from_logic->addRouteFrom(node->getNodeIndex());
        /* Prime reader */
        for (unsigned int i = 0; i < from_logic->count(); ++i)
        {
            dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
            block->bytes_used = VIDEO_SIZE;
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

        writeConfig(to_logic, x, y, z);
        z *= ZoomInFactor;
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
    emit renderedImage(QImage()); /* Render an empty image to clear the video screen */
    emit setActive(false);
}

void MandelbrotPipeline::enumDyploResources(DyploNodeInfoList &list)
{
    if (node)
        list.push_back(DyploNodeInfo(node->getNodeIndex(), BITSTREAM_MANDELBROT));
}

void MandelbrotPipeline::frameAvailableDyplo(int)
{
    if (z < MinScale) {
        x = DefaultCenterX;
        y = DefaultCenterY;
        z = DefaultScale;
    }
    writeConfig(to_logic, x, y, z);
    z *= ZoomInFactor;

    dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
    if (!block)
        return;

    unsigned int nlines = block->size / (VIDEO_WIDTH + 4);

    if (block->size != VIDEO_SIZE)
        qWarning() << "Strange size:" << block->size << " Expected:" << VIDEO_SIZE << "Lines:" << nlines;


    QImage image(VIDEO_WIDTH, nlines, QImage::Format_Indexed8);
    image.setColorTable(mandelbrot_color_map);
    image.fill(255);

    const unsigned char* data = (const unsigned char *)block->data;
    while (--nlines)
    {
        unsigned short line = *((unsigned short *)data);

        if (line >= VIDEO_HEIGHT) {
            qWarning() << "Invalid line:" << line;
            break;
        }
        memcpy(image.scanLine(line), data + 4, VIDEO_WIDTH);

        data += VIDEO_WIDTH + 4;
    }

    emit renderedImage(image);

    block->bytes_used = VIDEO_SIZE;
    from_logic->enqueue(block);
}
