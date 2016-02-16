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
      current_buf(new struct v4l2_buffer)
{
}

VideoCapture::~VideoCapture()
{
    close();
}

int VideoCapture::open(const char *filename)
{
    fd = ::open(filename, O_RDWR | O_NONBLOCK, 0);
    return fd;
}

int VideoCapture::setup(int width, int height)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
       /* Not a V4L2 device */
       return -errno;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        return -EINVAL;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        return -EINVAL;
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

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

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        return -errno;

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
            fmt.fmt.pix.sizeimage = min;

    return init_mmap();
}

int VideoCapture::start()
{
    enum v4l2_buf_type type;

    for (unsigned int i = 0; i < n_buffers; ++i)
    {
            struct v4l2_buffer buf;
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                    return -errno;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        return -errno;

    return 0;
}

int VideoCapture::begin_grab(const void **data, unsigned int *bytesused)
{
    CLEAR(*current_buf);
    current_buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    current_buf->memory = V4L2_MEMORY_MMAP;

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
    *bytesused = current_buf->bytesused;

    return 1;
}

int VideoCapture::end_grab()
{
    if (-1 == xioctl(fd, VIDIOC_QBUF, current_buf))
            return -errno;
}


int VideoCapture::stop()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
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
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
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

        CLEAR(buf);
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                return -errno;

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
                mmap(NULL /* start anywhere */,
                      buf.length,
                      PROT_READ | PROT_WRITE /* required */,
                      MAP_SHARED /* recommended */,
                      fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
                return -EFAULT;
    }

    return 0;
}
