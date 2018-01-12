#include <VideoCap.hpp>

CaptureApplication::CaptureApplication()
: writeContinuous(false), writeSingles(false), captureOn(true), writeCount(0)
{
    std::cout << "FPS: " << vc.get_fps() << std::endl;
    writing = (writeContinuous || writeCount);
    get_write_status();

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

bool CaptureApplication::numeric_command(const std::string *command)
{
    for (char c : *command) {
        if (!isdigit(c))
            return false;
    }
    return true;
}

void CaptureApplication::get_write_status()
{
    std::cout << "Write status: " << writing << std::endl;
}

void CaptureApplication::update_write_status()
{
    writing = (writeContinuous || writeSingles);
    get_write_status();
}

unsigned int CaptureApplication::str2int(const std::string *command)
{
    std::stringstream oldCommand(*command);
    unsigned int newCommand;
    oldCommand >> newCommand;
    return newCommand;
}

void CaptureApplication::parse_command()
{
    std::string command;
    std::cin >> command;
    if (command == "q") {
        std::cout << "Quitting..." << std::endl;
        captureOn = false;
    } else if (command == "start" && !writing) {
        writeContinuous = true; update_write_status();
        std::cout << "Writing frames..." << std::endl;
    } else if (command == "start" && writing) {
        std::cout << "Already writing!" << std::endl;
    } else if (command == "stop" && !writing) {
        std::cout << "Enter 'start' to commence writing" << std::endl;
    } else if (command == "stop" && writing) {
        writeContinuous = false; writeSingles = false;
        std::cout << "Write stopped!" << std::endl;
        update_write_status();
    } else if (numeric_command(&command) && !writing) {
        additionalFrames = str2int(&command);
        std::cout << "Writing " << additionalFrames
                  << " frames" << std::endl;
        writeSingles = true; update_write_status();
    } else if (command == "fps") {
        captureOn = false;
        captureThread.join();
        vc.release();
        vc.capture(true);
        captureOn = true;
        captureThread = std::thread(&CaptureApplication::run_capture, this);
        get_write_status();
    } else {
        std::cout << "Command not valid!" << std::endl;
    }
}

void CaptureApplication::run_capture()
{
    int ret;
    while (captureOn) {
        frame.data == NULL;
        ret = vc.read(&frame);
        if (ret) {
            if (writeContinuous) {
                write_image(&frame);
                writeCount += 1;
            } else if (writeSingles) {
                if (additionalFrames > 0) {
                    write_image(&frame);
                    writeCount += 1;
                    --additionalFrames;
                } else {
                    writeSingles = false; update_write_status();
                }
            }
            cv::imshow("Frame", frame);
            cv::waitKey(1);
        } else {
            std::cout << "Dropped frame!" << std::endl;
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
    //std::cout << fName << std::endl;
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
