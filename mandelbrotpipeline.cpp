#include "mandelbrotpipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>
#include <dyplo/hardware.hpp>
#include "dyplocontext.h"
#include "colormap.h"

#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 320
#define VIDEO_SIZE (VIDEO_WIDTH*VIDEO_HEIGHT)

static const char BITSTREAM_MANDELBROT[] = "mandelbrot";

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
    delete fromLogicNotifier;
    fromLogicNotifier = NULL;
    delete from_logic;
    from_logic = NULL;
    delete node;
    node = NULL;
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
    dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
    if (!block)
        return;

    QImage image((const uchar*)block->data, VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_Indexed8);
    image.setColorTable(mandelbrot_color_map);
    emit renderedImage(image);

    block->bytes_used = VIDEO_SIZE;
    from_logic->enqueue(block);
}
