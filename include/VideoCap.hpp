/*
Capture application and VideoCapture object for displaying and writing frames
from Leopard OV-580 stereo camera. Commands can be entered via the command
line after the application starts.

Capture Application initializes video capture device with address /dev/video0.

To initialize a VideoCapture object:
    vc = VideoCapture()
To grab a frame, call vc.read(). To release capture, vc.release().

    - David Henry 2018
*/
#ifndef VIDEO_CAP_H
#define VIDEO_CAP_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctime>
#include <cctype>

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
    const char *dev_name; // /dev/videoX
    const enum io_method io = IO_METHOD_MMAP; // Memory mapping.
    int fd = -1;
    buffer *buffers;
    unsigned int n_buffers;
    int out_buf;
    int force_format = 1; // If set != 0, img format specified in init_device()
    int fps = 100; // Defaults at 100 fps.

    void errno_exit(const char *s);
    int xioctl(int fh, int request, void *arg);

    void open_device(); // Open call on file descriptor.
    void init_device(); // Sets video capture format, fps, calls init_mmap().
    void init_mmap(); // Initiates memory mapping.
    void start_capturing(); // Starts capture.
    int process_frame(cv::Mat *frame); // Reads buffer into frame.
    void uninit_device(); // Unitiates memory map.
    void close_device(); // Closes device.
    void switch_fps(); // Fps value switch, called by capture() if needed.

public:
    VideoCapture();
    /*
    On initialization, dev_name is set to "/dev/video0",
        open_device();
        init_device();
        start_capturing();
    */
    int read(cv::Mat *frame);
    /*
    Reads buffer into input frame.
    */
    void release();
    /*
    Releases the video capture.
        cv::destroyAllWindows;
        uninit_device();
        close_device();
    */
    void capture(bool fpsSwitch = false);
    /*
    Run this after release() has been called to re-initiate capture. If
    fpsSwitch is true, then the fps is also switched (between 100 and 60).
        open_device();
        init_device();
        start_capturing();
    */
    int get_fps(); // Returns fps value.
};

class Frame
{
public:
    cv::Mat img = cv::Mat(480, 1280, CV_8U);
    struct timeval timestamp;
};

class CaptureApplication
{
private:
    VideoCapture vc;
    std::thread captureThread;
    std::atomic_bool writeContinuous; // Switch for writing frames to disk.
    std::atomic_bool writeSingles; // Switch for writing given amount of frames.
    std::atomic_bool writing; // Write status.
    std::atomic_bool captureOn; // Video capture switch.
    std::atomic_ulong writeCount; // Number of frames written to disk.
    std::atomic_uint additionalFrames; // User specified number of frames.
    cv::Mat frame = cv::Mat(480, 1280, CV_8U);

    void run_capture(); // Loops through Videocapture.read() calls.
    void parse_command();
    void print_timestamp();
    void write_image(cv::Mat *image); // Write current frame to disk.
    void update_write_status(); // Updates the write status.
    void get_write_status();
    bool numeric_command(const std::string *command); // Checks if input is num.
    unsigned int str2int(const std::string *command); // Converts str to int.


public:
    CaptureApplication();
    ~CaptureApplication();
};

#endif // VIDEO_CAP_H
