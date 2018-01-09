#include <VideoCap.hpp>

CaptureApplication::CaptureApplication()
: writeImg(false), captureOn(true)
{
    captureThread = std::thread(&CaptureApplication::run_capture, this);
    while (captureOn) {
        parse_command();
    }
    captureThread.join();
}

CaptureApplication::~CaptureApplication()
{
    vc.release();
}

void CaptureApplication::parse_command()
{
    std::string command;
    std::cin >> command;
    if (command == "q") {
        std::cout << "Quitting..." << std::endl;
        captureOn = false;
    } else if (command == "start" && writeImg == false) {
        writeImg = true;
        std::cout << "Writing frames..." << std::endl;
    } else if (command == "start" && writeImg == true) {
        std::cout << "Already writing!" << std::endl;
    } else if (command == "stop" && writeImg == false) {
        std::cout << "Enter 'start' to commence writing" << std::endl;
    } else if (command == "stop" && writeImg == true) {
        writeImg = false;
        std::cout << "Write finished!" << std::endl;
    } else {
        std::cout << "Command not recognised!" << std::endl;
    }
}

void CaptureApplication::run_capture()
{
    int ret;
    while (captureOn) {
        ret = vc.read(&frame);
        if (ret) {
            if (writeImg) {
                write_image(&frame);
            }
            cv::imshow("Frame", frame);
            cv::waitKey(1);
        }
    }
}

void CaptureApplication::write_image(cv::Mat *image)
{
    char timestamp[30];
    struct timeval tv;
    time_t curtime;
    std::string fName;

    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    strftime(timestamp, 30,"%m-%d-%Y_%T.",localtime(&curtime));
    fName = static_cast<std::string>(timestamp) +
            std::to_string(tv.tv_usec) + ".pgm";
    cv::imwrite(fName, frame);
}

void CaptureApplication::print_timestamp()
{
    char ts[30];
    struct timeval tv;

    time_t curtime;

    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    strftime(ts, 30, "%m-%d-%Y  %T.",localtime(&curtime));
    printf("%s%ld\n",ts,tv.tv_usec);
}
