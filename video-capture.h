

class VideoCapture
{
public:
	VideoCapture();
	~VideoCapture();

    int open(const char* filename); /* returns -1 on error */
    int setup(int width, int height);
    int start();
    int grab(const void **data, unsigned int *bytesused);
    int wait();
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
	
    int init_mmap();
};
