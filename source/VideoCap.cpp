#include <VideoCap.h>


VideoCapture::VideoCapture()
{
    dev_name = "/dev/video0";
    std::cout << "DEFAULT" << std::endl;
    open_device();
    init_device();
    start_capturing();
}

void VideoCapture::release()
{
    uninit_device();
    close_device();
}

void VideoCapture::errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

int VideoCapture::xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while(-1 == r && EINTR == errno);

    return r;
}

void VideoCapture::open_device()
{
    struct stat st;

    if (-1 == stat(dev_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    fd = open(dev_name , O_RDWR | O_NONBLOCK, 0);

    if (fd == -1) {
        fprintf(stderr, "Cannot open '%s' : %d, %s\n",
                dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void VideoCapture::close_device()
{
    if (-1 == close(fd))
        errno_exit("close");
    fd = -1;
}

void VideoCapture::init_device()
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n",
                    dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device \n",
                dev_name);
        exit(EXIT_FAILURE);
    }

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(fd, VIDIOC_CROPCAP, &cropcap) == 0) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; // reset to default

        if (xioctl(fd, VIDIOC_S_CROP, &crop) == -1) {
            switch (errno) {
            case EINVAL:
                break;
            default:
                break;
            }
        }
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    std::cout << "FORCE FORMAT: " << force_format << std::endl;
    if (force_format) {
        fprintf(stderr, "SET OV580\r\n");
        fmt.fmt.pix.width = 640;
        fmt.fmt.pix.height = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;

        if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
            errno_exit("VIDIOC_S_FMT");
    } else {
        if (xioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
            errno_exit("VIDIOC_G_FMT");
    }
    std::cout << fmt.fmt.pix.width << std::endl;
    std::cout << fmt.fmt.pix.height << std::endl;
    std::cout << fmt.fmt.pix.pixelformat << std::endl;

    init_mmap();
}

void VideoCapture::init_mmap()
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = static_cast<buffer*>(calloc(req.count, sizeof(*buffers)));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        fd, buf.m.offset);
        if (buffers[n_buffers].start == MAP_FAILED)
            errno_exit("mmap");
    }
}

void VideoCapture::uninit_device()
{
    unsigned int i;

    for (i = 0; i < n_buffers; ++i)
        if (munmap(buffers[i].start, buffers[i].length) == -1)
            errno_exit("munmap");

    free(buffers);
}

void VideoCapture::start_capturing()
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1)
            errno_exit("VIDIOC_QBUF");
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type))
        errno_exit("VIDIOC_STREAMON");
}

void VideoCapture::mainloop()
{
    for (;;) {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (r == -1) {
            if (errno == EINTR)
                continue;
            errno_exit("select");
        }
        if (r == 0) {
            fprintf(stderr, "select timeout \n");
            exit(EXIT_FAILURE);
        }

        if (read_frame())
            break;
    }
}

int VideoCapture::read_frame()
{
    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        switch (errno) {
        case EAGAIN:
            return 0;
        default:
            errno_exit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < n_buffers);

    process_image(buffers[buf.index].start, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");

    return 1;
}

void VideoCapture::process_image(const void *p, int size)
{
    char filename[15];
    int frame_number = 0;

    sprintf(filename, "frame-%d.raw", frame_number);
    FILE *fp = fopen(filename, "wb");

    fwrite(p, size, 1, fp);

    fflush(fp);
    fclose(fp);
    /*
    std::cout << "WRITING FILE" << std::endl;

    int jpgfile;
    if((jpgfile = open("myimage.jpeg", O_WRONLY | O_CREAT, 0660)) < 0){
        perror("open");
        exit(1);
    }

    write(jpgfile, p, size);
    close(jpgfile);
    */
}