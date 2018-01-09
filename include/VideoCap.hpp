/*
VidoeCapture class to grab frames from a USB camera.
To initialize a VideoCapture object:
    vc = VideoCapture()
To grab a frame, call vc.read(). To release capture, vc.release().

David Henry 2018
*/
#ifndef VIDEO_CAP_H
#define VIDEO_CAP_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>

extern "C" {
#include <getopt.h>
// Low level i/o
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
}

#include <iostream>
#include <thread>
#include <atomic>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

class buffer {
public:
    void *start;
    size_t length;
};

enum io_method {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
};

class VideoCapture{
private:
    const char *dev_name;
    const enum io_method io = IO_METHOD_MMAP;
    int fd = -1;
    buffer *buffers;
    unsigned int n_buffers;
    int out_buf;
    int force_format = 1; // If set != 0, img format specified in init_device()
    int frame_count = 200;
    unsigned int frame_number = 0;

    void errno_exit(const char *s);
    int xioctl(int fh, int request, void *arg);

    void open_device();
    void init_device();
    void init_mmap();
    void start_capturing();
    int process_frame(cv::Mat *frame);
    void stop_capturing();
    void uninit_device();
    void close_device();

public:
    VideoCapture();

    int read(cv::Mat *frame);
    void release();
};

class CaptureApplication
{
private:
    VideoCapture vc;
    std::thread captureThread;
    std::atomic_bool writeImg; // Switch for writing frames to disk.
    std::atomic_bool captureOn; // Video capture switch.
    cv::Mat frame = cv::Mat(480, 1280, CV_8U);

    void run_capture();
    void parse_command();
    void print_timestamp();
    void write_image(cv::Mat *image);

public:
    CaptureApplication();
    ~CaptureApplication();
};

#endif // VIDEO_CAP_H
