#include <VideoCap.hpp>

using namespace std;

void capture(cv::Mat *frame)
{
    int ret;
    for (;;) {
        m.lock();
        ret = cap.read(frame);
        m.unlock();
    }
}

int main(int argc, char *argv[])
{
    cv::Mat frame(480, 1280, CV_8U);
    std::thread thread1;
    std::mutex m;
    VideoCapture cap;

    Gtk::Main app(0, NULL);
    Gtk::Window Win0;

    Win0.show_all();
    Gtk::Main::run(Win0);



    return 0;
}
