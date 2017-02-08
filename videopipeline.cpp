#include "videopipeline.h"

#include <QDebug>
#include <QImage>
#include <QSocketNotifier>

#include <dyplo/hardware.hpp>
#include "dyplocontext.h"

#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 480
#define VIDEO_YUV_SIZE (VIDEO_WIDTH*VIDEO_HEIGHT*2)
#define VIDEO_RGB_SIZE (VIDEO_WIDTH*VIDEO_HEIGHT*3)

static const char BITSTREAM_YUVTORGB[] = "yuvtorgb";
static const char BITSTREAM_FILTER_YUV_GRAY[] = "grayscale";
static const char BITSTREAM_FILTER_YUV_CONTRAST[] = "contrast";
static const char BITSTREAM_FILTER_YUV_TRESHOLD[] = "treshold";

#define SOFTWARE_FLAG_CONTRAST 1
#define SOFTWARE_FLAG_GRAY 2
#define SOFTWARE_FLAG_THD 4

VideoPipeline::VideoPipeline():
    captureNotifier(NULL),
    fromLogicNotifier(NULL),
    rgb_buffer(NULL),
    to_logic(NULL),
    from_logic(NULL),
    yuv2rgb(NULL),
    filter1(NULL),
    filterTreshold(NULL),
    yuvfilter1(NULL),
    software_flags(0),
    yuv_buffer(NULL)
{

}

VideoPipeline::~VideoPipeline()
{
    deactivate();
    free(yuv_buffer);
}

int VideoPipeline::activate(DyploContext *dyplo, bool hardwareYUV, bool filterContrast, bool filterGray, bool filterThd)
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
            /* Create filters in the order they'll be used. This improves backplane usage */
            to_logic = dyplo->createDMAFifo(O_RDWR);
            int tailnode = to_logic->getNodeAndFifoIndex();

            if (filterContrast) {
                filter1 = dyplo->createConfig(BITSTREAM_FILTER_YUV_CONTRAST);
                int id = filter1->getNodeIndex();
                dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, id, 0);
                tailnode = id;
            }
            if (filterGray) {
                yuvfilter1 = dyplo->createConfig(BITSTREAM_FILTER_YUV_GRAY);
                int id = yuvfilter1->getNodeIndex();
                dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, id, 0);
                tailnode = id;
            }
            if (filterThd) {
                filterTreshold = dyplo->createConfig(BITSTREAM_FILTER_YUV_TRESHOLD);
                int id = filterTreshold->getNodeIndex();
                dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, id, 0);
                tailnode = id;
            }
            yuv2rgb = dyplo->createConfig(BITSTREAM_YUVTORGB);
            int headnode = yuv2rgb->getNodeIndex();
            dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, headnode, 0);
            tailnode = headnode;
            from_logic = dyplo->createDMAFifo(O_RDONLY);
            from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, VIDEO_RGB_SIZE, 2, true);
            from_logic->addRouteFrom(tailnode);
            to_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, VIDEO_YUV_SIZE, 2, false);
            if (filterTreshold)
                filterTreshold->enableNode();
            if (yuvfilter1)
                yuvfilter1->enableNode();
            if (filter1)
                filter1->enableNode();
            yuv2rgb->enableNode();
            /* Prime reader */
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
        software_flags = 0;
        if (filterContrast)
            software_flags |= SOFTWARE_FLAG_CONTRAST;
        if (filterGray)
            software_flags |= SOFTWARE_FLAG_GRAY;
        if (filterThd)
            software_flags |= SOFTWARE_FLAG_THD;
        if (software_flags)
            allocYUVbuffer();
        captureNotifier = new QSocketNotifier(capture.device_handle(), QSocketNotifier::Read, this);
        connect(captureNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableSoft(int)));
        captureNotifier->setEnabled(true);
    }

    emit setActive(true);
    return r;
}

/* Disable, disconnect and delete a node */
static void dispose_node(dyplo::HardwareConfig **node_p)
{
    dyplo::HardwareConfig *node = *node_p;
    if (node)
    {
        *node_p = NULL;
        node->deleteRoutes();
        node->disableNode();
        delete node;
    }
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
    dispose_node(&yuv2rgb);
    dispose_node(&filter1);
    dispose_node(&filterTreshold);
    dispose_node(&yuvfilter1);
    capture.close();
    emit renderedImage(QImage()); /* Render an empty image to clear the video screen */
    emit setActive(false);
}

void VideoPipeline::enumDyploResources(DyploNodeInfoList &list)
{
    if (yuv2rgb)
        list.push_back(DyploNodeInfo(yuv2rgb->getNodeIndex(), BITSTREAM_YUVTORGB));
    if (filter1)
        list.push_back(DyploNodeInfo(filter1->getNodeIndex(), BITSTREAM_FILTER_YUV_CONTRAST));
    if (yuvfilter1)
        list.push_back(DyploNodeInfo(yuvfilter1->getNodeIndex(), BITSTREAM_FILTER_YUV_GRAY));
    if (filterTreshold)
        list.push_back(DyploNodeInfo(filterTreshold->getNodeIndex(), BITSTREAM_FILTER_YUV_TRESHOLD));
}

static inline unsigned char thd_process(unsigned char y)
{
    if (y < (64 - 32))
        return 0;
    if (y < (128 - 32))
        return 63;
    if (y < (192 - 32))
        return 127;
    if (y < (256 - 32))
        return 191;
    return 255;
}

static inline unsigned char thd_processc(unsigned char uv)
{
    if (uv < 0x60)
        return 0x00;
    if (uv > 0xA0)
        return 0xFF;
    return 0x80;
}

static void thd(const unsigned int *input, unsigned int size, unsigned int *output)
{
    size >>= 2;
    while(size)
    {
        const unsigned int yuyv = *input;
        const unsigned char y0 = (unsigned char) (yuyv & 0x000000FF);
        const unsigned char u = (unsigned char) ((yuyv & 0x0000FF00) >> 8);
        const unsigned char y1 = (unsigned char) ((yuyv & 0x00FF0000) >> 16);
        const unsigned char v = (unsigned char) ((yuyv & 0xFF000000) >> 24);

        *output = thd_process(y0) |
            (((unsigned int)thd_processc(u)) << 8) |
            (((unsigned int)thd_process(y1)) << 16) |
            (((unsigned int)thd_processc(v)) << 24);

        ++output;
        ++input;
        --size;
    }
}

static inline unsigned char stretch(unsigned char y)
{
    if (y <= 64)
        return 0;
    if (y >= 192)
        return 255;
    return (y - 64) << 1;
}

static void contrast(const unsigned int *input, unsigned int size, unsigned int *output)
{
    size >>= 2;
    while(size)
    {
        const unsigned int yuyv = *input;
        const unsigned char y0 = (unsigned char) (yuyv & 0x000000FF);
        const unsigned char y1 = (unsigned char) ((yuyv & 0x00FF0000) >> 16);

        unsigned int y = stretch(y0) | (((unsigned int)stretch(y1)) << 16);
        *output = (*input & 0xFF00FF00) | y;

        ++output;
        ++input;
        --size;
    }
}

static inline unsigned char clamp(short v)
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
static void torgb_gray(const unsigned char *p, unsigned int size, unsigned char *rgb_buffer)
{
        unsigned int index = 0;
        unsigned int s;

        for (s = 0; s < size; s += 4) {
                unsigned char y0 = p[s];
                unsigned char y1 = p[s+2];

                rgb_buffer[index+0] = y0;
                rgb_buffer[index+1] = y0;
                rgb_buffer[index+2] = y0;
                rgb_buffer[index+3] = y1;
                rgb_buffer[index+4] = y1;
                rgb_buffer[index+5] = y1;

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
    if (software_flags & SOFTWARE_FLAG_CONTRAST) {
        contrast((const unsigned int*)data, size, yuv_buffer);
        data = yuv_buffer;
    }
    if (software_flags & SOFTWARE_FLAG_THD) {
        thd((const unsigned int*)data, size, yuv_buffer);
        data = yuv_buffer;
    }
    if (software_flags & SOFTWARE_FLAG_GRAY)
        torgb_gray((const uchar*)data, size, rgb_buffer);
    else
        torgb((const uchar*)data, size, rgb_buffer);

    emit renderedImage(QImage(rgb_buffer, VIDEO_WIDTH, VIDEO_HEIGHT, QImage::Format_RGB888));

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

void VideoPipeline::allocYUVbuffer()
{
    if (!yuv_buffer)
    {
        yuv_buffer = (unsigned int*)malloc(VIDEO_YUV_SIZE);
    }
}
