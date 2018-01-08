#include <VideoCap.hpp>

CaptureApplication::CaptureApplication()
: writeImg(false)
{
    captureThread = std::thread(&CaptureApplication::run_capture, this);
    //guiThread = std::thread(&CaptureApplication::run_interface, this);
    //run_interface(0,NULL);
    /*
    Gtk::Main app(0, NULL);
    Gtk::Window Win0;

    Win0.show_all();
    Gtk::Main::run(Win0);
    //run_capture();
    /*
    int derp = 0;
    char** derp1 = 0;
    auto app = Gtk::Application::create(derp, derp1, "org.gtkmm.example");

    Interface gui;
    int ret;
    ret = app->run(gui);
    */
    std::string lul;
    std::cin >> lul;
    //captureThread.join();
    //guiThread.join();

}

void CaptureApplication::run_interface()
{
    /*
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

    Interface gui;
    return app->run(gui);
    */
    Gtk::Main app(0, NULL);
    Gtk::Window Win0;

    Win0.show_all();
    Gtk::Main::run(Win0);
}

CaptureApplication::~CaptureApplication()
{
    
    vc.release();
}

void CaptureApplication::run_capture()
{
    int ret;
    for (;;) {
        ret = vc.read(&frame);
        if (ret) {
            cv::imshow("Frame", frame);
            cv::waitKey(1);
        }
    }
}

Interface::Interface()
: start("Start")
{
    set_border_width(10);
    start.signal_clicked().connect(sigc::mem_fun(*this,
        &Interface::on_button_clicked));
    add(start);
    start.show();
}

Interface::~Interface()
{}

void Interface::on_button_clicked()
{
    std::cout << "Hello World!" << std::endl;
}
