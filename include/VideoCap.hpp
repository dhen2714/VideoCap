/*
Capture application and VideoCapture object for displaying and writing frames
from Leopard OV-580 stereo camera. Commands can be entered via the command
line after the application starts. Commands are:
    start - Starts writing frames continuously to disk.
    n     - Where 'n' is an integer; writes 'n' frames to disk.
    stop  - Stops writing. Can be used after one of the previous two commands
            are called.
    q     - Quits application.

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
// V4L2 - video4linux
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
}

#include <iostream>
#include <thread>
#include <atomic>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/call_traits.hpp>
#include <boost/bind.hpp>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

class Frame
{
public:
    cv::Mat image;
    struct timeval timestamp;

    Frame() : image(cv::Mat(480, 1280, CV_8U)) {};
    void clear();
};

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

    void open_device(); // 'open()' call on file descriptor.
    void init_device(); // Sets video capture format, fps, calls init_mmap().
    void init_mmap(); // Initiates memory mapping.
    void start_capturing(); // Starts capture, queues buffers.
    int process_frame(cv::Mat *frame); // Reads buffer into frame.
    int process_frame(Frame &frame);
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
    int read(Frame &frame);
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

class bounded_buffer
/*
Thread-safe implementation of circular buffer, ensuring mutual exclusion
between capture and write threads during buffer access. This is achieved by
wrapping the operations of the underlying boost::circular_buffer object with
lock acquisition and release.

Implementation is based on this example:
http://www.boost.org/doc/libs/master/libs/circular_buffer/example/circular_buffer_bound_example.cpp
*/
{
public:
    typedef boost::circular_buffer<Frame> container_type;
    typedef typename container_type::size_type size_type;
    typedef typename container_type::value_type value_type;
    typedef typename boost::call_traits<value_type>::param_type param_type;

    explicit bounded_buffer(size_type capacity)
    : m_unread(0), m_container(capacity) {}

    void push_front(typename boost::call_traits<value_type>::param_type item);
    void pop_back(value_type* frameCopy);
    void pop_back(value_type &frameCopy);

    void clear_buffer() { m_container.clear(); m_unread = 0;}
    void clear_consumer();
    void clear_producer();

private:
    // Disabled copy constructor.
    bounded_buffer(const bounded_buffer&);
    // Disabled assign operator.
    bounded_buffer& operator = (const bounded_buffer&);

    bool is_not_empty() const { return m_unread > 0; }
    bool is_not_full() const { return m_unread < m_container.capacity(); }

    size_type m_unread;
    container_type m_container;
    boost::mutex m_mutex;
    boost::condition m_not_empty;
    boost::condition m_not_full;
};

class CaptureApplication
{
private:
    VideoCapture vc;
    std::thread readThread; // Thread for reading frames from VideoCapture.
    std::thread writeThread; // Thread for writing frames from VideoCapture.
    bounded_buffer *CapAppBuffer; // Circular buffer for frame R/W.
    std::atomic_bool writeContinuous; // Switch for writing frames to disk.
    std::atomic_bool writeSingles; // Switch for writing given amount of frames.
    std::atomic_bool writing; // Write status.
    std::atomic_bool captureOn; // Video capture switch.
    std::atomic_ulong writeCount; // Number of frames written to disk.
    std::atomic_uint additionalFrames; // User specified number of frames.
    //cv::Mat frame; // OpenCV Mat object which camera buffer is read to.
    Frame frame;
    const unsigned int cap_app_size = 500; // Frame capacity of circular buffer.

    void run_capture(); // Loops through Videocapture.read() calls.
    void parse_command();
    void print_timestamp();
    void write_image(Frame &frame);
    void write_image(cv::Mat *image); // Write current frame to disk.
    void write_image_raw(cv::Mat *image);
    void update_write_status(); // Updates the write status.
    void get_write_status();
    bool numeric_command(const std::string *command); // Checks if input is num.
    unsigned int str2int(const std::string *command); // Converts str to int.
    void read_frames(); // Reads frames to CapAppBuffer, and displays them.
    void write_frames(); // Writes frames from CapAppBuffer.
public:
    CaptureApplication();
    ~CaptureApplication();
};

// Following code from https://lwn.net/Articles/399547/

struct foo_dev 
{
    struct v4l2_device v4l2_dev;
    struct v4l2_ctrl_handler ctrl_handler;
};

struct foo_dev *foo;



v4l2_ctrl_handler_init(&foo->ctrl_handler, 1);
v4l2_ctrl_new_std(&foo->ctrl_handler, &foo_ctrl_ops, V4L2_CID_AUTOGAIN, 0, 1, 1, 128);

if (foo->ctrl_handler.error) {
        int err = foo->ctrl_handler.error;

        v4l2_ctrl_handler_free(&foo->ctrl_handler);
        return err;
}


foo->v4l2_dev.ctrl_handler = &foo->ctrl_handler;
v4l2_ctrl_handler_free(&foo->ctrl_handler);

static const struct v4l2_ctrl_ops foo_ctrl_ops = {
        .s_ctrl = foo_s_ctrl,
};


static int foo_s_ctrl(struct v4l2_ctrl *ctrl)
{
        struct foo *state = container_of(ctrl->handler, struct foo, ctrl_handler);

        switch (ctrl->id) {
        case V4L2_CID_AUTOGAIN:
                write_reg(0x80181033[3], ctrl->val);
                write_reg(0x80181833[3], ctrl->val);
                break;
        }
        return 0;
}

//V4L2_CID_AUTOGAIN (boolean)



#endif // VIDEO_CAP_H
