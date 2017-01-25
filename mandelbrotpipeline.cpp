#include "mandelbrotpipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>
#include <dyplo/hardware.hpp>
#include "dyplocontext.h"
#include "colormap.h"

#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 480
#define VIDEO_SIZE (VIDEO_WIDTH*VIDEO_HEIGHT)

static const char BITSTREAM_MANDELBROT[] = "mandelbrot";

static const double DefaultCenterX = -0.86122562296399741;
static const double DefaultCenterY = -0.23139131123653386;
static const double MinScale = 0.000000000000053607859314274247;
static const double DefaultScale = 0.005f;
static const double ZoomInFactor = 0.95f;
static const double ZoomOutFactor = 1 / ZoomInFactor;


static void writefp(dyplo::HardwareConfig &cfg, int offset, double v)
{
    long long iv = (long long)(v * ((long long)2 << 45));
    cfg.seek(offset);
    cfg.write(&iv, sizeof(iv));
}

static void writeConfig(dyplo::HardwareConfig &cfg, double x, double y, double z)
{
    writefp(cfg, 0x10, x);
    writefp(cfg, 0x20, y);
    writefp(cfg, 0x30, z);
}

static void enableRendering(dyplo::HardwareConfig &cfg, unsigned int enable)
{
    cfg.seek(0);
    cfg.write(&enable, sizeof(enable)); /* Start node */
}



MandelbrotPipeline::MandelbrotPipeline(QObject *parent) : QObject(parent),
    fromLogicNotifier(NULL),
    from_logic(NULL),
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
        x = DefaultCenterX;
        y = DefaultCenterY;
        z = DefaultScale;
        writeConfig(*node, x, y, z);
        enableRendering(*node, 1);
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
    if (node) {
        enableRendering(*node, 0);
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
    writeConfig(*node, x, y, z);
    z *= ZoomInFactor;

    dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
    if (!block)
        return;

    QImage image((const uchar*)block->data, VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_Indexed8);
    image.setColorTable(mandelbrot_color_map);
    emit renderedImage(image);

    block->bytes_used = VIDEO_SIZE;
    from_logic->enqueue(block);
}
