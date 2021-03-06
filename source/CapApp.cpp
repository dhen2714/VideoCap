#include <VideoCap.hpp>

CaptureApplication::CaptureApplication()
: writeContinuous(false), writeSingles(false), captureOn(true), writeCount(0)
{
    // Print out current fps.
    std::cout << "FPS: " << vc.get_fps() << std::endl;
    writing = (writeContinuous || writeCount); // Initial write status = 0.
    get_write_status();
    /* Because the bounded buffer could not be initialized directly as a class
    variable, we initialize the CapAppBuffer pointer instead, and set it to
    the address of the bounded buffer buf. */
    bounded_buffer buf(cap_app_size);
    CapAppBuffer = &buf;

    // Start separate threads for reading and writing frames to/from the
    // application buffer.
    readThread = std::thread(&CaptureApplication::read_frames, this);
    writeThread = std::thread(&CaptureApplication::write_frames, this);

    while (captureOn) {
        parse_command();
    }
    // When captureOn is set to false via the 'q' command, end application.
    readThread.join();
    std::cout << "..." << std::endl;
    writeThread.join();
    std::cout << "..." << std::endl;
    CapAppBuffer->clear_buffer();
}

CaptureApplication::~CaptureApplication()
{
    vc.release();
    std::cout << "Application exited." << std::endl;
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
        // Quit command.
        std::cout << "Quitting..." << std::endl;
        captureOn = false;
        std::cout << "..." << std::endl;
    } else if (command == "start" && !writing) {
        // Starts writing frames to disk continuously.
        writeContinuous = true; update_write_status();
        std::cout << "Writing frames..." << std::endl;
    } else if (command == "start" && writing) {
        std::cout << "Already writing!" << std::endl;
    } else if (command == "stop" && !writing) {
        std::cout << "Enter 'start' to commence writing" << std::endl;
    } else if (command == "stop" && writing) {
        // Stops writing frames to disk.
        writeContinuous = false; writeSingles = false;
        std::cout << "Write stopped!" << std::endl;
        update_write_status();
    } else if (numeric_command(&command) && !writing) {
        additionalFrames = str2int(&command);
        std::cout << "Writing " << additionalFrames
                  << " frames" << std::endl;
        writeSingles = true; update_write_status();
    } else if (numeric_command(&command) && writing) {
        std::cout << "Already writing!" << std::endl;
    } else if (command == "fps") {
        captureOn = false;
        readThread.join();
        writeThread.join();
        CapAppBuffer->clear_buffer();
        vc.release();
        vc.capture(true);
        captureOn = true;
        readThread = std::thread(&CaptureApplication::read_frames, this);
        writeThread = std::thread(&CaptureApplication::write_frames, this);
        get_write_status();
    } else {
        std::cout << "Command not valid!" << std::endl;
    }
}

void CaptureApplication::read_frames()
{
    int ret;
    while (captureOn)
    {
        ret = vc.read(frame);
        if (ret) {
            CapAppBuffer->push_front(frame); // writes to front of buffer
            cv::imshow("Frame", frame.image);
            cv::waitKey(1);
        }
    }
    cv::destroyAllWindows();
    CapAppBuffer->clear_consumer();
}

void CaptureApplication::write_frames()
{
    // Mat objects held in buffer are written to frameCopy.
    //cv::Mat frameCopy(480, 1280, CV_8U);
    Frame frameCopy;
    while (captureOn)
    {
        CapAppBuffer->pop_back(frameCopy);
        if (writeContinuous) {
            write_image(frameCopy);
            writeCount += 1;
        } else if (writeSingles) {
            if (additionalFrames > 0) {
                write_image(frameCopy);
                writeCount += 1;
                --additionalFrames;
            } else {
                writeSingles = false;
                update_write_status();
            }
        }
    }
    CapAppBuffer->clear_producer();
}
/*
void CaptureApplication::run_capture()
{
    // This is the old method in which read and write calls were on the
    // same thread. This lead instances where writing frames took longer than
    // time interval between vc.read() calls, causing stutters in the video
    // feed.
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
                    write_image_raw(&frame);
                    writeCount += 1;
                    --additionalFrames;
                } else {
                    writeSingles = false;
                    update_write_status();
                }
            }
            cv::imshow("Frame", frame);
            cv::waitKey(1);
        } else {
            std::cout << "Dropped frame!" << std::endl;
        }
    }
}
*/
void CaptureApplication::write_image(Frame &frame)
{
    struct timeval tv;
    std::string fName;
    long ts;

    tv = frame.timestamp;
    ts = tv.tv_sec*1e6 + tv.tv_usec;
    fName = std::to_string(ts) + "_" + std::to_string(this->writeCount) + ".pgm";
    cv::imwrite(fName, frame.image);
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
    cv::imwrite(fName, *image);
    //std::cout << fName << std::endl;
}

void CaptureApplication::write_image_raw(cv::Mat *image)
{
    // Writing raw images rather than pgm does not prevent dropouts.
    char timestamp[30];
    struct timeval tv;
    time_t curtime;
    char fileName[36];
    //std::string fName;

    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    strftime(timestamp, 30,"%m-%d-%Y_%T.",localtime(&curtime));
    sprintf(fileName, "%s%ld",timestamp,tv.tv_usec);
    /*fName = static_cast<std::string>(timestamp) +
            std::to_string(tv.tv_usec) + ".pgm"; */

    FILE *fp = fopen(fileName, "wb");
    int frameSize = image->total() * image->elemSize();
    fwrite(image->data, frameSize, 1, fp);

    fflush(fp);
    fclose(fp);
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

void bounded_buffer::push_front(
    typename boost::call_traits<value_type>::param_type item)
{
    // `param_type` represents the "best" way to pass a parameter of type
    // `value_type` to a method.
    boost::mutex::scoped_lock lock(m_mutex);
    m_not_full.wait(lock, boost::bind(&bounded_buffer::is_not_full, this));
    m_container.push_front(item);
    ++m_unread;
    lock.unlock();
    m_not_empty.notify_one();
}

void bounded_buffer::pop_back(value_type &frameCopy)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_not_empty.wait(lock, boost::bind(&bounded_buffer::is_not_empty, this));
    frameCopy = m_container[--m_unread];
    lock.unlock();
    m_not_full.notify_one();
}

void bounded_buffer::pop_back(value_type* frameCopy)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_not_empty.wait(lock, boost::bind(&bounded_buffer::is_not_empty, this));
    *frameCopy = m_container[--m_unread];
    lock.unlock();
    m_not_full.notify_one();
}

void bounded_buffer::clear_consumer()
{
    ++m_unread;
    m_not_empty.notify_one();
}

void bounded_buffer::clear_producer()
{
    m_unread = m_container.capacity() - 1;
    m_not_full.notify_one();
}

void Frame::clear()
{
    image.data = nullptr;
    timestamp.tv_sec = 0L;
    timestamp.tv_usec = 0L;
}
