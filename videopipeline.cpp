#include "videopipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>

#include <dyplo/hardware.hpp>
#include "dyplocontext.h"

#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 320
#define VIDEO_YUV_SIZE (VIDEO_WIDTH*VIDEO_HEIGHT*2)
#define VIDEO_RGB_SIZE (VIDEO_WIDTH*VIDEO_HEIGHT*3)

static const char BITSTREAM_YUVTORGB[] = "yuvtorgb";

VideoPipeline::VideoPipeline():
    captureNotifier(NULL),
    fromLogicNotifier(NULL),
    rgb_buffer(NULL),
    to_logic(NULL),
    from_logic(NULL),
    yuv2rgb(NULL)
{

}

VideoPipeline::~VideoPipeline()
{
    deactivate();
}

int VideoPipeline::activate(DyploContext *dyplo, bool hardwareYUV)
{
    int r;

    r = capture.open("/dev/video1");
    if (r < 0)
        r = capture.open("/dev/video0");
    if (r < 0) {
        qWarning() << "Failed to open video capture device";
        return r;
    }

    r = capture.setup(VIDEO_WIDTH, VIDEO_HEIGHT);
    if (r < 0) {
        qWarning() << "Failed to configure video capture device";
        return r;
    }

    r = capture.start();
    if (r < 0) {
        qWarning() << "Failed to start video capture device";
        return r;
    }

    if (hardwareYUV)
    {
        try
        {
            yuv2rgb = dyplo->createConfig(BITSTREAM_YUVTORGB);
            from_logic = dyplo->createDMAFifo(O_RDONLY);
            to_logic = dyplo->createDMAFifo(O_RDWR);
            int node = yuv2rgb->getNodeIndex();
            from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, VIDEO_RGB_SIZE, 2, true);
            from_logic->addRouteFrom(node);
            to_logic->addRouteTo(node);
            to_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, VIDEO_YUV_SIZE, 2, false);
            /* Prime reader */
            yuv2rgb->enableNode();
            for (unsigned int i = 0; i < from_logic->count(); ++i)
            {
                dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
                block->bytes_used = VIDEO_RGB_SIZE;
                from_logic->enqueue(block);
            }
            from_logic->fcntl_set_flag(O_NONBLOCK);
            captureNotifier = new QSocketNotifier(capture.device_handle(), QSocketNotifier::Read, this);
            connect(captureNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableHard(int)));
            captureNotifier->setEnabled(true);
            fromLogicNotifier = new QSocketNotifier(from_logic->handle, QSocketNotifier::Read, this);
            connect(fromLogicNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableDyplo(int)));
            fromLogicNotifier->setEnabled(true);
        }
        catch (const std::exception& ex)
        {
            qCritical() << ex.what();
            deactivate(); /* Cleanup */
            return -1; /* Hmm... */
        }
    }
    else
    {
        captureNotifier = new QSocketNotifier(capture.device_handle(), QSocketNotifier::Read, this);
        connect(captureNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableSoft(int)));
        captureNotifier->setEnabled(true);
    }

    emit setActive(true);
    return r;
}

void VideoPipeline::deactivate()
{
    delete captureNotifier;
    captureNotifier = NULL;
    delete fromLogicNotifier;
    fromLogicNotifier = NULL;
    delete to_logic;
    to_logic = NULL;
    delete from_logic;
    from_logic = NULL;
    delete yuv2rgb;
    yuv2rgb = NULL;
    capture.close();
    emit renderedImage(QImage()); /* Render an empty image to clear the video screen */
    emit setActive(false);
}

void VideoPipeline::enumDyploResources(DyploNodeInfoList &list)
{
    if (yuv2rgb)
        list.push_back(DyploNodeInfo(yuv2rgb->getNodeIndex(), BITSTREAM_YUVTORGB));
}

static unsigned char clamp(short v)
{
        if (v > 255)
                return 255;
        if (v < 0)
                return 0;
        return v;
}

static void torgb(const unsigned char *p, unsigned int size, unsigned char *rgb_buffer)
{
        unsigned int index = 0;
        unsigned int s;

        for (s = 0; s < size; s += 4) {
                short y0 = p[s];
                short u  = p[s+1] - 0x80;
                short y1 = p[s+2];
                short v  = p[s+3] - 0x80;

                short rr = (45 * v) >> 5;
                short gg = - (((45 * v) + (22 * u)) >> 6);
                short bb = (111 * u) >> 6;

                rgb_buffer[index+0] = clamp(y0 + rr);
                rgb_buffer[index+1] = clamp(y0 + gg);
                rgb_buffer[index+2] = clamp(y0 + bb);
                rgb_buffer[index+3] = clamp(y1 + rr);
                rgb_buffer[index+4] = clamp(y1 + gg);
                rgb_buffer[index+5] = clamp(y1 + bb);

                index += 6;
        }
}

void VideoPipeline::frameAvailableSoft(int)
{
    /* Grab a single frame, convert and display */
    const void* data;
    unsigned int size;
    int r;

    r = capture.begin_grab(&data, &size);
    if ( r <= 0) {
        if (r < 0)
            deactivate();
        return;
    }

    if (!rgb_buffer)
        rgb_buffer = new unsigned char[VIDEO_RGB_SIZE];

    if (size > VIDEO_YUV_SIZE)
        size = VIDEO_YUV_SIZE;
    torgb((const uchar*)data, size, rgb_buffer);

    QImage image(rgb_buffer, VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_RGB888);
    emit renderedImage(image);

    r = capture.end_grab();
    if (r < 0)
        deactivate();
}

void VideoPipeline::frameAvailableHard(int)
{
    /* Grab a single frame and send to Dyplo */
    const void* data;
    unsigned int size;
    int r;

    r = capture.begin_grab(&data, &size);
    if ( r <= 0) {
        if (r < 0)
            deactivate();
        return;
    }
    if (size > VIDEO_YUV_SIZE)
        size = VIDEO_YUV_SIZE;

    dyplo::HardwareDMAFifo::Block *block = to_logic->dequeue();
    block->bytes_used = size;
    memcpy(block->data, data, size);
    to_logic->enqueue(block);

    r = capture.end_grab();
    if (r < 0)
        deactivate();
}

void VideoPipeline::frameAvailableDyplo(int)
{
    dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
    if (!block)
        return;

    emit renderedImage(QImage((const uchar*)block->data, VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_RGB888));

    block->bytes_used = VIDEO_RGB_SIZE;
    from_logic->enqueue(block);
}
