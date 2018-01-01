#include <VideoCap.h>

using namespace std;

int main()
{
    cv::Mat image(480, 640, CV_8U);
    VideoCapture vc = VideoCapture();

    for (int i = 0; i < 10; i++)
    vc.mainloop();

    vc.release();

    //cv::imwrite("test.jpg", image);

    cout << "Hello world!" << endl;
    return 0;
}
