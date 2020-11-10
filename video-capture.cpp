#include "video-capture.h"

#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <qdebug.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

VideoCapture::VideoCapture():
      fd(-1),
      buffers(NULL),
      n_buffers(0),
      current_buf(new struct v4l2_buffer),
      current_planes(new struct v4l2_plane)
{
}

VideoCapture::~VideoCapture()
{
    close();
    delete current_planes;
    delete current_buf;
}

int VideoCapture::open(const char *filename)
{
    close();
    fd = ::open(filename, O_RDWR | O_NONBLOCK, 0);
    if (fd == -1)
        return -1;

    /* Verify that the device can actually capture video. Kernel 4.19
     * reports 2 video devices per camera, but only one can capture */
    struct v4l2_capability cap;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
       /* Not a V4L2 device */
       // qDebug() << filename << "Not a V4L2 device (VIDIOC_QUERYCAP)";
       close();
       return -errno;
    }

    if (!(cap.device_caps & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE))) {
        // qDebug() << filename << "No capture capability (V4L2_CAP_VIDEO_CAPTURE[_MPLANE])";
        close();
        return -EINVAL;
    }

    /* Xilinx video requires MPLANE interface */
    multiplanar = (!(cap.device_caps & (V4L2_CAP_VIDEO_CAPTURE)));
    if (multiplanar)
        qDebug() << filename << "requires MPLANE";

    if (!(cap.device_caps & V4L2_CAP_STREAMING)) {
        qDebug() << filename << "No streaming capability (V4L2_CAP_STREAMING)";
        close();
        return -EINVAL;
    }

    return fd;
}

static int set_framerate(int fd, enum v4l2_buf_type type, int fps)
{
    struct v4l2_streamparm parm;

    CLEAR(parm);
    parm.type = type;
    parm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = fps;
    return xioctl(fd, VIDIOC_S_PARM, &parm);
}

static bool supports_frame_size(int fd)
{
    struct v4l2_frmsizeenum fse;
    CLEAR(fse);
    fse.index = 0;
    fse.pixel_format = V4L2_PIX_FMT_YUYV;

    int ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fse);
    return ret >= 0;
}

int VideoCapture::setup(int width, int height, int fps, VideoCaptureSettings *settings)
{
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    enum v4l2_buf_type type = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    unsigned int min;

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = type;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
            crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            crop.c = cropcap.defrect; /* reset to default */

            if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                    switch (errno) {
                    case EINVAL:
                            /* Cropping not supported. */
                            break;
                    default:
                            /* Errors ignored. */
                            break;
                    }
            }
    } else {
            /* Errors ignored. */
    }

    CLEAR(fmt);
    fmt.type = type;
    /*
     * Workaround issue with Xilinx' capture device that does not
     * really support changing the resolution. It fails to enumerate
     * so we use that as a way to determine whether we can set the
     * output.
     */
    if (supports_frame_size(fd)) {
        if (multiplanar) {
            fmt.fmt.pix_mp.width       = width;
            fmt.fmt.pix_mp.height      = height;
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUYV;
            fmt.fmt.pix_mp.field       = V4L2_FIELD_INTERLACED;
            fmt.fmt.pix_mp.num_planes  = 1;
        } else {
            fmt.fmt.pix.width       = width;
            fmt.fmt.pix.height      = height;
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
            fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        }
    } else {
        /* Retrieve format */
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
            qDebug() << "VIDIOC_G_FMT:" << errno;
            return -errno;
        }
        if (multiplanar) {
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUYV;
            fmt.fmt.pix_mp.field       = V4L2_FIELD_INTERLACED;
            fmt.fmt.pix_mp.num_planes  = 1;
            /* Workaround for weird behavior of Xilinx capture device, which reports 1920/1 and similar nonsense */
            if (fmt.fmt.pix_mp.width == 1920)
                fmt.fmt.pix_mp.height = 1080;
        } else {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
            fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        }
    }
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        return -errno;

    if (multiplanar) {
        if (fmt.fmt.pix_mp.num_planes != 1) {
            qDebug() << "Unsupported num_planes:" << fmt.fmt.pix_mp.num_planes;
        }
        settings->width  = fmt.fmt.pix_mp.width;
        settings->height = fmt.fmt.pix_mp.height;
        settings->stride = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
        settings->size   = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
        settings->format = fmt.fmt.pix_mp.pixelformat;
    } else {
        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        settings->width  = fmt.fmt.pix.width;
        settings->height = fmt.fmt.pix.height;
        settings->stride = fmt.fmt.pix.bytesperline;
        settings->size   = fmt.fmt.pix.sizeimage;
        settings->format = fmt.fmt.pix.pixelformat;
    }

    if (set_framerate(fd, type, fps) < 0) {
        qWarning() << "Failed to set" << fps << "FPS mode. Camera supports these:";
        /* Obtain possible settings for framerate... */
        struct v4l2_frmivalenum frmival;
        memset(&frmival, 0, sizeof(frmival));
        frmival.type = type;
        frmival.pixel_format = fmt.fmt.pix.pixelformat;
        frmival.width = fmt.fmt.pix.width;
        frmival.height = fmt.fmt.pix.height;
        while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0)
        {
            if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
                qDebug() << "Discrete:"
                         << frmival.discrete.denominator << "/" << frmival.discrete.numerator;
            else
                qDebug() << "Stepwise:"
                            << frmival.stepwise.min.denominator << "/" << frmival.stepwise.min.numerator << ".."
                            << frmival.stepwise.max.denominator << "/" << frmival.stepwise.max.numerator;
            frmival.index++;
        }
    }

    return init_mmap();
}

int VideoCapture::start()
{
    enum v4l2_buf_type type = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for (unsigned int i = 0; i < n_buffers; ++i)
    {
            struct v4l2_buffer buf;
            CLEAR(buf);
            buf.type = type;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if (multiplanar) {
                buf.m.planes = current_planes;
                buf.length = 1;
            }
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
                qDebug() << "VIDIOC_QBUF:" << errno;
                return -errno;
            }
    }

    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
        qDebug() << "VIDIOC_STREAMON:" << errno;
        return -errno;
    }

    return 0;
}

int VideoCapture::begin_grab(const void **data, unsigned int *bytesused)
{
    CLEAR(*current_buf);
    current_buf->type = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    current_buf->memory = V4L2_MEMORY_MMAP;
    if (multiplanar) {
        current_buf->m.planes = current_planes;
        current_buf->length = 1;
    }

    if (-1 == xioctl(fd, VIDIOC_DQBUF, current_buf)) {
            switch (errno) {
            case EAGAIN:
                    return 0;
            case EIO:
                    /* Could ignore EIO, see spec. */
                    /* fall through */
            default:
                    return -errno;
            }
    }

    if (current_buf->index >= n_buffers)
        return -EFAULT;

    *data = buffers[current_buf->index].start;
    *bytesused = multiplanar ? current_buf->m.planes[0].bytesused : current_buf->bytesused;

    return 1;
}

int VideoCapture::end_grab()
{
    if (-1 == xioctl(fd, VIDIOC_QBUF, current_buf))
            return -errno;
    return 0;
}


int VideoCapture::stop()
{
    enum v4l2_buf_type type = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            return -errno;
    return 0;
}

void VideoCapture::teardown()
{
    if (buffers)
    {
        stop(); /* In case the user forgot to cal it */
        for (unsigned int i = 0; i < n_buffers; ++i)
                munmap(buffers[i].start, buffers[i].length);
        delete [] buffers;
        n_buffers = 0;
        buffers = NULL;
    }
}

void VideoCapture::close()
{
    if (fd != -1)
    {
        teardown();
        ::close(fd);
        fd = -1;
    }
}

int VideoCapture::init_mmap()
{
    struct v4l2_requestbuffers req;

    CLEAR(req);
    req.count = 4;
    req.type = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        return -errno;
    }

    if (req.count < 2)
        return -ENOMEM;

    buffers = new buffer[req.count];
    if (!buffers)
        return -ENOMEM;

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[1];

        CLEAR(buf);
        buf.type        = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;
        if (multiplanar) {
            buf.m.planes = planes;
            buf.length = 1;
        }

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
                qDebug() << "VIDIOC_QUERYBUF failed:" << errno;
                return -errno;
        }

        if (multiplanar) {
            buffers[n_buffers].length = planes[0].length;
            buffers[n_buffers].start =
                    mmap(NULL /* start anywhere */,
                          planes[0].length,
                          PROT_READ | PROT_WRITE /* required */,
                          MAP_SHARED /* recommended */,
                          fd, planes[0].m.mem_offset);
        } else {
            buffers[n_buffers].length = buf.length;
            buffers[n_buffers].start =
                    mmap(NULL /* start anywhere */,
                          buf.length,
                          PROT_READ | PROT_WRITE /* required */,
                          MAP_SHARED /* recommended */,
                          fd, buf.m.offset);
        }
        if (MAP_FAILED == buffers[n_buffers].start) {
            qDebug() << "mmap failed";
            return -EFAULT;
        }
    }

    return 0;
}
