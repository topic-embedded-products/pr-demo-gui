#include "videopipeline.h"

#include <QDebug>
#include <QSocketNotifier>

#include <dyplo/hardware.hpp>
#include "dyplocontext.h"

#define VIDEO_FRAMERATE 25

static const char BITSTREAM_CAMERA_XRGB[] = "camera_xrgb";
static const char BITSTREAM_YUVTORGB[] = "yuvtorgb";
static const char BITSTREAM_FILTER_YUV_GRAY[] = "grayscale";
static const char BITSTREAM_FILTER_YUV_CONTRAST[] = "contrast";
static const char BITSTREAM_FILTER_YUV_TRESHOLD[] = "treshold";
static const char BITSTREAM_FILTER_RGB32_GRAY[] = "rgb_grayscale";
static const char BITSTREAM_FILTER_RGB32_CONTRAST[] = "rgb_contrast";
static const char BITSTREAM_FILTER_RGB32_TRESHOLD[] = "rgb_treshold";

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
    filterContrast(NULL),
    filterTreshold(NULL),
    filterGrayscale(NULL),
    ioCamera(NULL),
    software_flags(0),
    yuv_buffer(NULL),
    outputformat(QImage::Format_RGB888)
{
}

VideoPipeline::~VideoPipeline()
{
    deactivate_impl();
}

int VideoPipeline::openIOCamera(DyploContext *dyplo, bool filterContr, bool filterGray, bool filterThd)
{
    try
    {
        ioCamera = dyplo->createConfig(BITSTREAM_CAMERA_XRGB);

        /* Hardcoded settings */
        settings.format = 0;
        settings.width = 1920;
        settings.height = 1080;
        settings.stride = settings.width * 4;
        settings.size = settings.stride * settings.height;
        rgb_size = settings.size;
        yuv_size = settings.size;
        crop_width = settings.width;
        crop_height = settings.height;
        outputformat = QImage::Format_RGB32; /* IO camera outputs 32 bit color */

        int tailnode = ioCamera->getNodeIndex();
        ioCamera->disableNode();
        ioCamera->resetWriteFifos(0xf);

        if (filterContr) {
            filterContrast = dyplo->createConfig(BITSTREAM_FILTER_RGB32_CONTRAST);
            int id = filterContrast->getNodeIndex();
            dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, id, 0);
            tailnode = id;
        }
        if (filterGray) {
            filterGrayscale = dyplo->createConfig(BITSTREAM_FILTER_RGB32_GRAY);
            int id = filterGrayscale->getNodeIndex();
            dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, id, 0);
            tailnode = id;
        }
        if (filterThd) {
            filterTreshold = dyplo->createConfig(BITSTREAM_FILTER_RGB32_TRESHOLD);
            int id = filterTreshold->getNodeIndex();
            dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, id, 0);
            tailnode = id;
        }
        from_logic = dyplo->createDMAFifo(O_RDONLY);
        from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, rgb_size, 2, true);
        from_logic->addRouteFrom(tailnode);
        /* Prime reader */
        for (unsigned int i = 0; i < from_logic->count(); ++i)
        {
            dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
            block->bytes_used = rgb_size;
            from_logic->enqueue(block);
        }
        from_logic->fcntl_set_flag(O_NONBLOCK);
        fromLogicNotifier = new QSocketNotifier(from_logic->handle, QSocketNotifier::Read, this);
        connect(fromLogicNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableDyplo(int)));
        fromLogicNotifier->setEnabled(true);

        if (filterTreshold)
            filterTreshold->enableNode();
        if (filterGrayscale)
            filterGrayscale->enableNode();
        if (filterContr)
            filterContrast->enableNode();
        ioCamera->enableNode();
    }
    catch (const std::exception& ex)
    {
        qCritical() << ex.what();
        deactivate_impl(); /* Cleanup */
        return -1; /* Hmm... */
    }

    return 0;
}


int VideoPipeline::openCaptureDevice(int width, int height)
{
    int r;
    char dev[16];

    /* Walk downwards so we prefer the last addition to the system */
    for (int index = 9; index >= 0; --index)
    {
        snprintf(dev, sizeof(dev), "/dev/video%d", index);
        r = capture.open(dev);
        if (r < 0)
            continue;

        r = capture.setup(width, height, VIDEO_FRAMERATE, &settings);
        if (r < 0) {
            qDebug() << "Failed to configure video capture device" << dev;
            continue;
        }
        if ((int)settings.height > height)
        {
            crop_top = (settings.height - height) / 2;
            crop_height = height;
        }
        else
        {
            crop_top = 0;
            crop_height = settings.height;
        }
        if ((int)settings.width > width + 32) /* Only when worth the effort */
        {
            crop_left = (settings.width - width) / 2;
            /* Align on multiple of 4 pixels */
            crop_left &= ~3;
            crop_width = ((unsigned int)(width + 3) >> 2) << 2;
        }
        else
        {
            crop_left = 0;
            crop_width = settings.width;
        }
        update_buffer_sizes();

        r = capture.start();
        if (r < 0) {
            qDebug() << "Failed to start video capture device" << dev;
            continue;
        }

        /* Found one that works */
        return 0;
    }

    /* Failure */
    return -1;
}


int VideoPipeline::activate(DyploContext *dyplo, int width, int height, bool hardwareYUV, bool filterContr, bool filterGray, bool filterThd)
{
    int r;

    /* Make sure width is a multiple of 4 */
    width &= ~3;

    r = openIOCamera(dyplo, filterContr, filterGray, filterThd);
    if (r == 0)
    {
        /* We're done */
        emit setActive(true);
        return 0;
    }

    r = openCaptureDevice(width, height);
    if (r) {
        qWarning() << "No capture device available";
        return r;
    }

    /* Software camera outputs 24 bit color */
    outputformat = QImage::Format_RGB888;

    if (hardwareYUV)
    {
        try
        {
            /* Create filters in the order they'll be used. This improves backplane usage */
            to_logic = dyplo->createDMAFifo(O_RDWR);
            int tailnode = to_logic->getNodeAndFifoIndex();

            if (filterContr) {
                filterContrast = dyplo->createConfig(BITSTREAM_FILTER_YUV_CONTRAST);
                int id = filterContrast->getNodeIndex();
                dyplo->GetHardwareControl().routeAddSingle(tailnode & 0xFF, tailnode >> 8, id, 0);
                tailnode = id;
            }
            if (filterGray) {
                filterGrayscale = dyplo->createConfig(BITSTREAM_FILTER_YUV_GRAY);
                int id = filterGrayscale->getNodeIndex();
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
            from_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, rgb_size, 2, true);
            from_logic->addRouteFrom(tailnode);
            to_logic->reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, yuv_size, 2, false);
            to_logic->fcntl_set_flag(O_NONBLOCK);
            if (filterTreshold)
                filterTreshold->enableNode();
            if (filterGrayscale)
                filterGrayscale->enableNode();
            if (filterContr)
                filterContrast->enableNode();
            yuv2rgb->enableNode();
            /* Prime reader */
            for (unsigned int i = 0; i < from_logic->count(); ++i)
            {
                dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
                block->bytes_used = rgb_size;
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
            deactivate_impl(); /* Cleanup */
            return -1; /* Hmm... */
        }
    }
    else
    {
        software_flags = 0;
        if (filterContr)
            software_flags |= SOFTWARE_FLAG_CONTRAST;
        if (filterGray)
            software_flags |= SOFTWARE_FLAG_GRAY;
        if (filterThd)
            software_flags |= SOFTWARE_FLAG_THD;
        if (software_flags || crop_width != settings.width)
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
    deactivate_impl();
    emit renderedImage(QImage()); /* Render an empty image to clear the video screen */
    emit setActive(false);
}

void VideoPipeline::deactivate_impl()
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
    dispose_node(&filterContrast);
    dispose_node(&filterTreshold);
    dispose_node(&filterGrayscale);
    dispose_node(&ioCamera);
    capture.close();
    free(yuv_buffer);
    yuv_buffer = NULL;
    free(rgb_buffer);
    rgb_buffer = NULL;
}

void VideoPipeline::enumDyploResources(DyploNodeResourceList &list)
{
    if (yuv2rgb)
        list.push_back(DyploNodeResource(yuv2rgb->getNodeIndex(), BITSTREAM_YUVTORGB));
    if (filterContrast)
        list.push_back(DyploNodeResource(filterContrast->getNodeIndex(), BITSTREAM_FILTER_YUV_CONTRAST));
    if (filterGrayscale)
        list.push_back(DyploNodeResource(filterGrayscale->getNodeIndex(), BITSTREAM_FILTER_YUV_GRAY));
    if (filterTreshold)
        list.push_back(DyploNodeResource(filterTreshold->getNodeIndex(), BITSTREAM_FILTER_YUV_TRESHOLD));
    if (ioCamera)
        list.push_back(DyploNodeResource(ioCamera->getNodeIndex(), BITSTREAM_CAMERA_XRGB));
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

    /* Vertical cropping is easy, just move the pointer */
    data = (const char*)data + crop_offset;
    if (size > yuv_size)
        size = yuv_size;

    if (crop_width != settings.width)
    {
        unsigned int src_stride = settings.width * 2;
        unsigned int dst_stride = crop_width * 2;
        char *dest = (char*)yuv_buffer;
        for (unsigned int y = 0; y < crop_height; ++y)
        {
            memcpy(dest, data, dst_stride);
            dest += dst_stride;
            data = (const char*)data + src_stride;
        }
        data = yuv_buffer;
    }

    if (!rgb_buffer)
        rgb_buffer = new unsigned char[rgb_size];

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

    emit renderedImage(QImage(rgb_buffer, crop_width, crop_height, QImage::Format_RGB888));

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
    /* crop image vertically */
    data = (const char*)data + crop_offset;
    if (size > yuv_size)
        size = yuv_size;

    dyplo::HardwareDMAFifo::Block *block = to_logic->dequeue();
    if (block)
    {
        block->bytes_used = size;

        if (crop_width != settings.width)
        {
            unsigned int src_stride = settings.width * 2;
            unsigned int dst_stride = crop_width * 2;
            char *dest = (char*)block->data;
            for (unsigned int y = 0; y < crop_height; ++y)
            {
                memcpy(dest, data, dst_stride);
                dest += dst_stride;
                data = (const char*)data + src_stride;
            }
        }
        else
        {
            memcpy(block->data, data, size);
        }
        to_logic->enqueue(block);
    }

    r = capture.end_grab();
    if (r < 0)
        deactivate();
}

void VideoPipeline::frameAvailableDyplo(int)
{
    dyplo::HardwareDMAFifo::Block *block = from_logic->dequeue();
    if (!block)
        return;

    unsigned int bytes = block->bytes_used;
    if (bytes)
    {
        unsigned int lines = bytes / crop_width;
        if (lines > crop_height)
            lines = crop_height;
        emit renderedImage(QImage((const uchar*)block->data, crop_width, lines, outputformat));
    }

    block->bytes_used = rgb_size;
    from_logic->enqueue(block);
}

void VideoPipeline::allocYUVbuffer()
{
    if (!yuv_buffer)
    {
        yuv_buffer = (unsigned int*)malloc(yuv_size);
    }
}

void VideoPipeline::update_buffer_sizes()
{
    crop_offset = settings.width * crop_top * 2;
    crop_offset += crop_left * 2;
    yuv_size = crop_width * crop_height * 2;
    rgb_size = crop_width * crop_height * 3;

    qDebug() << settings.width << "x" << settings.height <<
                "crop=" << crop_offset << crop_left << crop_top << crop_width << crop_height <<
                "yuvsize=" << yuv_size <<
                "rgbsize=" << rgb_size;
}
