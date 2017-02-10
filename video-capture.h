

class VideoCapture
{
public:
	VideoCapture();
	~VideoCapture();

    int open(const char* filename); /* returns -1 on error */
    int setup(int width, int height, int fps);
    int start();
    int begin_grab(const void **data, unsigned int *bytesused);
    int end_grab();
    int stop();
    void teardown();
    void close();

	/* Return handle for use in select() for UI loop integration */
	int device_handle() const { return fd; }

protected:
    struct buffer {
        void *start;
        unsigned int length;
    };
    int fd;
    buffer* buffers;
    unsigned int n_buffers;
    struct v4l2_buffer *current_buf;

    int init_mmap();
};
