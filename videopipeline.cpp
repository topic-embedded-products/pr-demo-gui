#include "videopipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>

VideoPipeline::VideoPipeline():
    socketNotifier(NULL)
{

}

VideoPipeline::~VideoPipeline()
{
    deactivate();
}

int VideoPipeline::activate()
{
    int r;

    r = capture.open("/dev/video1");
    if (r < 0)
        r = capture.open("/dev/video0");
    if (r < 0) {
        qWarning() << "Failed to open video capture device";
        return r;
    }

    r = capture.setup(640, 320);
    if (r < 0) {
        qWarning() << "Failed to configure video capture device";
        return r;
    }

    r = capture.start();
    if (r < 0) {
        qWarning() << "Failed to start video capture device";
        return r;
    }

    socketNotifier = new QSocketNotifier(capture.device_handle(), QSocketNotifier::Read, this);
    connect(socketNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailable(int)));
    socketNotifier->setEnabled(true);

    return r;
}

void VideoPipeline::deactivate()
{
    delete socketNotifier;
    socketNotifier = NULL;
    capture.close();
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

static unsigned char* rgb_buffer = NULL;

void VideoPipeline::frameAvailable(int)
{
    /* TODO: Grab a single frame and send to Dyplo */
    const void* data;
    unsigned int size;
    int r;
    r = capture.grab(&data, &size);

    if ( r <= 0) {
        if (r < 0)
            deactivate();
        return;
    }

    if (!rgb_buffer)
        rgb_buffer = new unsigned char[640*320*3];

    if (size > 640*320*2)
        size = 640*320*2;
    torgb((const uchar*)data, size, rgb_buffer);

    QImage image(rgb_buffer, 640, 320, QImage::Format_RGB888);
    emit renderedImage(image);
}
