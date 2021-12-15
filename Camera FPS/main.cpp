#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace std::chrono;

typedef unsigned long       DWORD;
typedef unsigned char       BYTE;

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

int main()
{
	cv::VideoCapture camera(0);

	int num = 60;
	int width = 1280;
	int height = 720;
	int wait = 1000 / num;
	camera.set(cv::CAP_PROP_FPS, num);
	camera.set(cv::CAP_PROP_FRAME_WIDTH, width);
	camera.set(cv::CAP_PROP_FRAME_HEIGHT, height);
	camera.set(cv::CAP_PROP_EXPOSURE, -6);
	camera.set(cv::CAP_PROP_SATURATION, 65);

	camera.set(cv::CAP_PROP_FOURCC, MAKEFOURCC('M', 'J', 'P', 'G'));

	//capture.set(CAP_PROP_SETTINGS, 1);
	std::cout << "CAP_PROP_FPS: " << camera.get(cv::CAP_PROP_FPS) << std::endl;
	std::cout << "CAP_PROP_FRAME_WIDTH: " << camera.get(cv::CAP_PROP_FRAME_WIDTH) << std::endl;
	std::cout << "CAP_PROP_FRAME_HEIGHT: " << camera.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
	std::cout << "CAP_PROP_EXPOSURE: " << camera.get(cv::CAP_PROP_EXPOSURE) << std::endl;
	std::cout << "CAP_PROP_SATURATION: " << camera.get(cv::CAP_PROP_SATURATION) << std::endl;

	cv::namedWindow("Camera");
	cv::resizeWindow("Camera", cv::Size(width, height));
	cv::Mat image;


	high_resolution_clock::time_point t1 = high_resolution_clock::now();

	unsigned long long frame = 0;
	char txt[128] = { 0 };
	int fps = 0;
	while (camera.read(image))
	{
		++frame;
		high_resolution_clock::time_point f2 = high_resolution_clock::now();
		seconds ms = duration_cast<seconds>(f2 - t1);
		if (ms.count() > 0)
		{
			fps = frame / ms.count();
		}
		sprintf_s(txt, 128, "FPS: %d", fps);
		cv::putText(image, txt, cv::Point(0, 100), cv::FONT_HERSHEY_TRIPLEX, 1, cv::Scalar(0, 255, 255), 1);

		cv::imshow("Camera", image);
		if (cv::waitKey(1) == 27)
			break;
	}

	cv::destroyAllWindows();
	return 0;
}
