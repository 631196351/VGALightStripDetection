#if 0
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <vector>
#include <Windows.h>
#include <thread>
#include <mutex>
#include <fstream>

#include "main.h"
#include "nvbase.h"
#include "ene_program.h"
#include "utility.h"

using namespace cv;
using namespace std;

#define VGA_PPID_LENGTH 20
#define LED_COUNT 22
#define LED_HALF_COUNT (LED_COUNT/2)
#define CONFIG_FILE "3c.ini"

bool g_wait = false;
//unsigned char g_product = 0;
bool g_main_thread_exit = false;
bool g_complete_a_cycle = false;	// 完成一个轮回， 进行一次抓拍
std::mutex g_mutex_wait;
std::condition_variable g_led_set_color_ok; // 条件变量, 指示LED 灯已经设置成功
std::condition_variable g_image_process_ok; // 条件变量, 指示完成抓拍，已经处理好图片
std::fstream g_aging_file;
int g_Led_Color = BLUE;
time_t g_start_time;

int g_CameraIndex = 0;
Size g_FrameSize = Size(1280, 780);
Rect g_RectFrame = Rect(200, 240, 900, 200);
int g_LedCount = 22;
int g_LedHalfCount = g_LedCount / 2;
bool g_ShowTrackBarWnd = true;
int g_IntervalTime = 100;		// 灯珠亮灭的间隔时间
char g_PPID[VGA_PPID_LENGTH] = { 0 };
AgingLog2* g_lpAgingLog2 = NULL;
//----		Red		Green	Blue	White
//hmin		156		35		100		0
//hmax		180		77		124		180
//smin		43		43		43		0
//smax		255		255		255		30
//vmin		46		46		46		221
//vmax		255		255		255		255
HsvColor g_HsvColor[AllColor];

bool g_AgingSettingSaveRectImages = true;

#if false
#define SPLIT_BGR true

void rgb_less_250_set_to_0(Mat& src)
{
	for (int row = 0; row < src.rows; row++)
	{
		for (int col = 0; col < src.cols; col++)
		{
			int b = src.at<Vec3b>(row, col)[0];
			int g = src.at<Vec3b>(row, col)[1];
			int r = src.at<Vec3b>(row, col)[2];

			if (r < 250)
			{
				src.at<Vec3b>(row, col)[0] = 0;
				src.at<Vec3b>(row, col)[1] = 0;
				src.at<Vec3b>(row, col)[2] = 0;
			}
		}
	}
}

int main001()
{
	// 读取一个正常的灯带
	Mat red_0 = imread("./img/red_bands00.png");
	if (!red_0.data)
	{
		printf("ERROR : could not load red_bands.png .\n");
		return -1;
	}

	//rgb_less_250_set_to_0(red_0);

	imshow("red_0", red_0);
	moveWindow("red_0", 300, 0);

	// 将正常灯带转成灰度图, 作为对比目标
	//Mat red_0_gray;
	//cvtColor(red_0, red_0_gray, COLOR_BGR2GRAY);
	//imshow("red_0_gray", red_0_gray);
	//moveWindow("red_0_gray", 1200, 0);
#if SPLIT_BGR
	vector<Mat> red_0_bgr;
	split(red_0, red_0_bgr);
	imshow("red_0_bgr", red_0_bgr[2]);
	moveWindow("red_0_bgr", 1200, 0);
#else
	Mat red_0_hsv;
	cvtColor(red_0, red_0_hsv, COLOR_BGR2HSV);
	imshow("red_0_hsv", red_0_hsv);
	moveWindow("red_0_hsv", 1200, 0);
#endif

	// 读取灭一个灯的灯带, 并转成灰度图
	Mat red_1 = imread("./img/red_bands1.png");
	if (!red_1.data)
	{
		printf("ERROR : could not load red_bands1.png .\n");
		return -1;
	}

	//rgb_less_250_set_to_0(red_1);

	imshow("red_1", red_1);
	moveWindow("red_1", 300, 200);

	//Mat red_1_gray;
	//cvtColor(red_1, red_1_gray, COLOR_BGR2GRAY);
	//imshow("red_1_gray", red_1_gray);
	//moveWindow("red_1_gray", 1200, 200);
#if SPLIT_BGR
	vector<Mat> red_1_bgr;
	split(red_1, red_1_bgr);
	imshow("red_1_bgr", red_1_bgr[2]);
	moveWindow("red_1_bgr", 1200, 200);
#else
	Mat red_1_hsv;
	cvtColor(red_1, red_1_hsv, COLOR_BGR2HSV);
	imshow("red_1_hsv", red_1_hsv);
	moveWindow("red_1_hsv", 1200, 200);
#endif

	// 读取灭2个灯的灯带, 并转成灰度图
	Mat red_2 = imread("./img/red_bands2.png");
	if (!red_2.data)
	{
		printf("ERROR : could not load red_bands2.png .\n");
		return -1;
	}

	//rgb_less_250_set_to_0(red_2);
	imshow("red_2", red_2);
	moveWindow("red_2", 300, 400);

	//Mat red_2_gray;
	//cvtColor(red_2, red_2_gray, COLOR_BGR2GRAY);
	//imshow("red_2_gray", red_2_gray);
	//moveWindow("red_2_gray", 1200, 400);
#if SPLIT_BGR
	vector<Mat> red_2_bgr;
	split(red_2, red_2_bgr);
	imshow("red_2_bgr", red_2_bgr[2]);
	moveWindow("red_2_bgr", 1200, 400);
#else
	Mat red_2_hsv;
	cvtColor(red_2, red_2_hsv, COLOR_BGR2HSV);
	imshow("red_2_hsv", red_2_hsv);
	moveWindow("red_2_hsv", 1200, 400);
#endif

	// 读取灭3个灯的灯带, 并转成灰度图
	Mat red_3 = imread("./img/red_bands3.png");
	if (!red_3.data)
	{
		printf("ERROR : could not load red_bands3.png .\n");
		return -1;
	}

	//rgb_less_250_set_to_0(red_3);
	imshow("red_3", red_3);
	moveWindow("red_3", 300, 600);

	//Mat red_3_gray;
	//cvtColor(red_3, red_3_gray, COLOR_BGR2GRAY);
	//imshow("red_3_gray", red_3_gray);
	//moveWindow("red_3_gray", 1200, 600);
#if SPLIT_BGR
	vector<Mat> red_3_bgr;
	split(red_3, red_3_bgr);
	imshow("red_3_bgr", red_3_bgr[2]);
	moveWindow("red_3_bgr", 1200, 600);
#else
	Mat red_3_hsv;
	cvtColor(red_3, red_3_hsv, COLOR_BGR2HSV);
	imshow("red_3_hsv", red_3_hsv);
	moveWindow("red_3_hsv", 1200, 600);
#endif

	Mat red_0_histogram, red_1_histogram, red_2_histogram, red_3_histogram;

#if SPLIT_BGR

	int channels = 0;
	int histSize = 256;
	float hranges[] = { 0, 255 };
	const float* ranges[] = { hranges };

	calcHist(&red_0_bgr[2], 1, &channels, Mat(), red_0_histogram, 1, &histSize, ranges, true, false);
	calcHist(&red_1_bgr[2], 1, &channels, Mat(), red_1_histogram, 1, &histSize, ranges, true, false);
	calcHist(&red_2_bgr[2], 1, &channels, Mat(), red_2_histogram, 1, &histSize, ranges, true, false);
	calcHist(&red_3_bgr[2], 1, &channels, Mat(), red_3_histogram, 1, &histSize, ranges, true, false);

	int hist_h = 400;
	int hist_w = 512;
	int bin_w = hist_w / histSize;

	Mat histImage(hist_w, hist_h, CV_8UC3, Scalar(0, 0, 0));

	normalize(red_0_histogram, red_0_histogram, 0, hist_h, NORM_MINMAX, -1, Mat());
	normalize(red_1_histogram, red_1_histogram, 0, hist_h, NORM_MINMAX, -1, Mat());
	normalize(red_2_histogram, red_2_histogram, 0, hist_h, NORM_MINMAX, -1, Mat());
	normalize(red_3_histogram, red_3_histogram, 0, hist_h, NORM_MINMAX, -1, Mat());

	double v01 = compareHist(red_0_histogram, red_1_histogram, HISTCMP_CORREL);
	double v02 = compareHist(red_0_histogram, red_2_histogram, HISTCMP_CORREL);
	double v03 = compareHist(red_0_histogram, red_3_histogram, HISTCMP_CORREL);

	std::cout << "0v1 = " << v01 << endl;
	std::cout << "0v2 = " << v02 << endl;
	std::cout << "0v3 = " << v03 << endl;

	for (int i = 1; i < histSize; i++)
	{
		line(histImage, Point((i - 1)*bin_w, hist_h - cvRound(red_0_histogram.at<float>(i - 1))),
			Point((i)*bin_w, hist_h - cvRound(red_0_histogram.at<float>(i))), Scalar(0, 0, 255), 2, LINE_AA);

		line(histImage, Point((i - 1)*bin_w, hist_h - cvRound(red_1_histogram.at<float>(i - 1))),
			Point((i)*bin_w, hist_h - cvRound(red_1_histogram.at<float>(i))), Scalar(0, 255, 0), 2, LINE_AA);

		line(histImage, Point((i - 1)*bin_w, hist_h - cvRound(red_2_histogram.at<float>(i - 1))),
			Point((i)*bin_w, hist_h - cvRound(red_2_histogram.at<float>(i))), Scalar(255, 0, 0), 2, LINE_AA);

		line(histImage, Point((i - 1)*bin_w, hist_h - cvRound(red_3_histogram.at<float>(i - 1))),
			Point((i)*bin_w, hist_h - cvRound(red_3_histogram.at<float>(i))), Scalar(255, 255, 255), 2, LINE_AA);
	}
	imshow("histogram", histImage);

#else
	// 4. 初始化计算直方图需要的实参(bins, 范围，通道 H 和 S ).
	int h_bins = 50; int s_bins = 60;
	int histSize[] = { h_bins, s_bins };
	// hue varies from 0 to 179, saturation from 0 to 255     
	float h_ranges[] = { 0, 180 };
	float s_ranges[] = { 0, 256 };
	const float* ranges[] = { h_ranges, s_ranges };
	// Use the o-th and 1-st channels     
	int channels[] = { 0, 1 };

	calcHist(&red_0_hsv, 1, channels, Mat(), red_0_histogram, 2, histSize, ranges, true, false);
	calcHist(&red_1_hsv, 1, channels, Mat(), red_1_histogram, 2, histSize, ranges, true, false);
	calcHist(&red_2_hsv, 1, channels, Mat(), red_2_histogram, 2, histSize, ranges, true, false);
	calcHist(&red_3_hsv, 1, channels, Mat(), red_3_histogram, 2, histSize, ranges, true, false);

	normalize(red_0_histogram, red_0_histogram, 0, 1, NORM_MINMAX, -1, Mat());
	normalize(red_0_histogram, red_1_histogram, 0, 1, NORM_MINMAX, -1, Mat());
	normalize(red_0_histogram, red_2_histogram, 0, 1, NORM_MINMAX, -1, Mat());
	normalize(red_0_histogram, red_3_histogram, 0, 1, NORM_MINMAX, -1, Mat());

	double compareHist0v0 = compareHist(red_0_histogram, red_0_histogram, HISTCMP_CORREL);
	double compareHist0v1 = compareHist(red_0_histogram, red_1_histogram, HISTCMP_CORREL);
	double compareHist0v2 = compareHist(red_0_histogram, red_2_histogram, HISTCMP_CORREL);
	double compareHist0v3 = compareHist(red_0_histogram, red_3_histogram, HISTCMP_CORREL);

	std::cout << "0v0 = " << compareHist0v0 << endl;
	std::cout << "0v1 = " << compareHist0v1 << endl;
	std::cout << "0v2 = " << compareHist0v2 << endl;
	std::cout << "0v3 = " << compareHist0v3 << endl;
#endif

	waitKey(0);
	return 0;
}

#endif

typedef unsigned long Ul32;

typedef int(*lpLoadVenderDLL)();
typedef bool(*lpVGA_Read_IC_I2C)(UCHAR ucAddress, UCHAR reg_address, BYTE &rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags);
typedef bool(*lpVGA_Write_IC_I2C)(UCHAR ucAddress, UCHAR reg_address, UCHAR *rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags);

lpLoadVenderDLL  LOAD_VENDOR_DLL;
lpVGA_Read_IC_I2C    VGA_READ_IC_I2C;
lpVGA_Write_IC_I2C   VGA_WRITE_IC_I2C;

// LED 灯的地址
BYTE REG[22] = { 0x60, 0x63, 0x66, 0x69, 0x6c, 0x6f, 0x72, 0x75, 0x78, 0x7b, 0x7e
				, 0x81, 0x84, 0x87, 0x8a, 0x8d, 0x90, 0x93, 0x96, 0x99, 0x9c, 0x9f };

BYTE uOffset[12] = { 0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00 };

#if 0
void setColor(unsigned char red, unsigned char blue, unsigned char green)
{
	unsigned char buf[90] = { 0 };
	for (size_t i = 0; i < 90; i += 3)
	{
		buf[i] = red;
		buf[i + 1] = blue;
		buf[i + 2] = green;
	}
	for (size_t j = 0; j < 90; j += 30)
	{
		eneWriteRegs(0x8160 + j, &buf[j], 30);
	}
	eneWriteReg(0x8021, 1);
	eneWriteReg(0x802f, 1);
}

void setSingleColor(u8 ledNumlight, u8 ledNumDelight, u8 r, u8 g, u8 b)
{
	u8 bufLight[3] = { r,g,b };	// 要打开的灯颜色
	u8 bufDelight[3] = { 0,0,0 };// 要关掉的灯

	eneWriteRegs(0x8160 + ledNumlight * 3, bufLight, 3);

	eneWriteRegs(0x8160 + ledNumDelight * 3, bufDelight, 3);

	eneWriteReg(0x8021, 1);	// 设置为静态
	eneWriteReg(0x802f, 1);	// 设置启用
}
#else
void initVGA()
{
	HINSTANCE hDLL;		// Handle to DLL
	hDLL = LoadLibrary(L"VGA_Extra_x64.dll");
	LOAD_VENDOR_DLL = (lpLoadVenderDLL)GetProcAddress(hDLL, "LoadVenderDLL");
	VGA_READ_IC_I2C = (lpVGA_Read_IC_I2C)GetProcAddress(hDLL, "VGA_Read_IC_I2C");
	VGA_WRITE_IC_I2C = (lpVGA_Write_IC_I2C)GetProcAddress(hDLL, "VGA_Write_IC_I2C");
	// 载入dll
	LOAD_VENDOR_DLL();
}
// 隔离硬件平台, 设置led灯光
void setSignleColor(int led, BYTE r, BYTE g, BYTE b)
{
	//Set Start Address
	uOffset[0] = 0x81;
	uOffset[1] = REG[led];
	VGA_WRITE_IC_I2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address

	uOffset[0] = 3;	//rgb size
	uOffset[1] = r;
	uOffset[2] = b;
	uOffset[3] = g;
	//(UCHAR ucAddress, UCHAR reg_address, UCHAR *rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags)
	VGA_WRITE_IC_I2C((BYTE)0xCE, (BYTE)0x03, (BYTE*)uOffset, 0, 1, 1, 4, 1);

	uOffset[0] = 0x80;
	uOffset[1] = 0x21;
	VGA_WRITE_IC_I2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address

	uOffset[0] = 0x01;
	VGA_WRITE_IC_I2C(0xCE, 0x1, (BYTE*)uOffset, 0, 1, 1, 1, 1);	//write data

	uOffset[0] = 0x80;
	uOffset[1] = 0x2F;
	VGA_WRITE_IC_I2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address

	uOffset[0] = 0x01;
	VGA_WRITE_IC_I2C(0xCE, 0x01, (BYTE*)uOffset, 0, 1, 1, 1, 1);	//write data
}

// 重置为特定颜色
void resetColor(BYTE r, BYTE g, BYTE b)
{
	for (int i = 0; i < LED_COUNT; i++)
	{
		setSignleColor(i, r, g, b);
	}
}
#endif

void setColorThread()
{
	/* 这里存一个灯的映射关系，打开一个灯，需要把前面打开的灯关闭
	 0 - 10
	 1 - 0
	 10 - 9
	 -------------
	 11 - 21
	 12 - 11
	 21 - 20
	 */
	u8 colorNum[22];
	for (u8 i = 1; i < 22; i++)
	{
		colorNum[i] = i - 1;
	}
	colorNum[0] = LED_HALF_COUNT - 1;
	colorNum[LED_HALF_COUNT] = LED_COUNT - 1;

	while (true)
	{
		if (!g_wait)
		{
			for (g_Led_Color = WHITE; g_Led_Color <= BLUE; g_Led_Color++)
			{
				for (size_t i = 0, t = LED_HALF_COUNT; i < LED_HALF_COUNT && t < LED_COUNT; i++, t++)
				{
					// 在子线程的每次loop前判定是否需要退出
					// 主线程退出是随机的, 主线程接收到退出key后, 业务循环逻辑立即退出,进入join子线程状态
					// 子线程即便是恰巧在设置完灯后解锁, 主线程因为不再受理业务, 子线程会在下个循环开始前在此处退出
					if (g_main_thread_exit)
					{
						return;
					}
					g_mutex_wait.lock();
					setSignleColor(colorNum[i], 0, 0, 0);
					setSignleColor(colorNum[t], 0, 0, 0);

					if (g_Led_Color == WHITE)
					{
						setSignleColor(i, 255, 255, 255);
						setSignleColor(t, 255, 255, 255);
					}
					else if (g_Led_Color == RED)
					{
						setSignleColor(i, 255, 0, 0);
						setSignleColor(t, 255, 0, 0);
					}
					else if (g_Led_Color == GREEN)
					{
						setSignleColor(i, 0, 255, 0);
						setSignleColor(t, 0, 255, 0);
					}
					else if (g_Led_Color == BLUE)
					{
						setSignleColor(i, 0, 0, 255);
						setSignleColor(t, 0, 0, 255);
					}
					printf("**************\n");
					Sleep(50);	//灯的颜色真正设置进显卡
					g_wait = true;
					g_mutex_wait.unlock();
					Sleep(50);// Give Main Thread CPU Time
				}

				// 等Main 把活做完
				g_mutex_wait.lock();
				printf("++++++++++++++\n");
				g_mutex_wait.unlock();
				//while (g_wait) {}

				//if (g_Led_Color >= BLUE)
				//	g_Led_Color = WHITE;
				//else
				//	g_Led_Color++;
			}
		}
	}
}

void setColorThread2()
{
	/* 这里存一个灯的映射关系，打开一个灯，需要把前面打开的灯关闭
	 0 - 10
	 1 - 0
	 10 - 9
	 -------------
	 11 - 21
	 12 - 11
	 21 - 20
	 */
	u8 colorNum[22];
	for (u8 i = 1; i < 22; i++)
	{
		colorNum[i] = i - 1;
	}
	colorNum[0] = LED_HALF_COUNT - 1;
	colorNum[LED_HALF_COUNT] = LED_COUNT - 1;

	while (true)
	{
		for (g_Led_Color = WHITE; g_Led_Color <= BLUE; g_Led_Color++)
		{
			for (size_t i = 0, t = LED_HALF_COUNT; i < LED_HALF_COUNT && t < LED_COUNT; i++, t++)
			{
				// 在子线程的每次loop前判定是否需要退出
				// 主线程退出是随机的, 主线程接收到退出key后, 业务循环逻辑立即退出,进入join子线程状态
				// 子线程即便是恰巧在设置完灯后解锁, 主线程因为不再受理业务, 子线程会在下个循环开始前在此处退出
				if (g_main_thread_exit)
				{
					return;
				}

				// 一个单生产者-单消费者模型
				std::unique_lock<std::mutex> lock(g_mutex_wait);
				if (g_wait)
				{
					g_image_process_ok.wait(lock);
				}
				//g_mutex_wait.lock();
				setSignleColor(colorNum[i], 0, 0, 0);
				setSignleColor(colorNum[t], 0, 0, 0);

				if (g_Led_Color == WHITE)
				{
					setSignleColor(i, 255, 255, 255);
					setSignleColor(t, 255, 255, 255);
				}
				else if (g_Led_Color == RED)
				{
					setSignleColor(i, 255, 0, 0);
					setSignleColor(t, 255, 0, 0);
				}
				else if (g_Led_Color == GREEN)
				{
					setSignleColor(i, 0, 255, 0);
					setSignleColor(t, 0, 255, 0);
				}
				else if (g_Led_Color == BLUE)
				{
					setSignleColor(i, 0, 0, 255);
					setSignleColor(t, 0, 0, 255);
				}
				printf("**************\n");
				Sleep(50);	//灯的颜色真正设置进显卡
				g_wait = true;
				//g_mutex_wait.unlock();

				// 消费者线程目前不需要关注生产者的状态
				g_led_set_color_ok.notify_all();
				lock.unlock(); // 解锁.
				Sleep(2000);// Give Main Thread CPU Time
			}
		}
	}

}

void setColorThread3()
{
	/* 这里存一个灯的映射关系，打开一个灯，需要把前面打开的灯关闭
	 0 - 10
	 1 - 0
	 10 - 9
	 -------------
	 11 - 21
	 12 - 11
	 21 - 20
	 */
	u8 *colorNum = new u8[g_LedCount]{ 0 };
	for (u8 i = 1; i < g_LedCount; i++)
	{
		colorNum[i] = i - 1;
	}
	colorNum[0] = g_LedHalfCount - 1;
	colorNum[g_LedHalfCount] = g_LedCount - 1;

	//while (true)
	{
		g_lpAgingLog2 = new AgingLog2(AllColor * g_LedCount);
		for (g_Led_Color = WHITE; g_Led_Color < AllColor; g_Led_Color++)
		{
			g_complete_a_cycle = false;
			for (size_t i = 0, t = g_LedHalfCount; i < g_LedHalfCount && t < g_LedCount; i++, t++)
			{
				// 在子线程的每次loop前判定是否需要退出
				// 主线程退出是随机的, 主线程接收到退出key后, 业务循环逻辑立即退出,进入join子线程状态
				// 子线程即便是恰巧在设置完灯后解锁, 主线程因为不再受理业务, 子线程会在下个循环开始前在此处退出
				if (g_main_thread_exit)
				{
					if (g_lpAgingLog2 != NULL)
						delete g_lpAgingLog2;
					return;
				}

				// 一个单生产者-单消费者模型
				std::unique_lock<std::mutex> lock(g_mutex_wait);
				if (g_wait)
				{
					g_image_process_ok.wait(lock);
				}
				//g_mutex_wait.lock();
				setSignleColor(colorNum[i], 0, 0, 0);
				setSignleColor(colorNum[t], 0, 0, 0);

				if (g_Led_Color == WHITE)
				{
					setSignleColor(i, 255, 255, 255);
					setSignleColor(t, 255, 255, 255);
				}
				else if (g_Led_Color == RED)
				{
					setSignleColor(i, 255, 0, 0);
					setSignleColor(t, 255, 0, 0);
				}
				else if (g_Led_Color == GREEN)
				{
					setSignleColor(i, 0, 255, 0);
					setSignleColor(t, 0, 255, 0);
				}
				else if (g_Led_Color == BLUE)
				{
					setSignleColor(i, 0, 0, 255);
					setSignleColor(t, 0, 0, 255);
				}
				g_lpAgingLog2->setCurrentLedIndex(i, t);

				printf("**************\n");
				Sleep(50);	//灯的颜色真正设置进显卡
				g_wait = true;
				//g_mutex_wait.unlock();

				// 消费者线程目前不需要关注生产者的状态
				g_led_set_color_ok.notify_all();
				lock.unlock(); // 解锁.
				Sleep(g_IntervalTime);// Give Main Thread CPU Time
			}

			// 一个轮回结束后，要将所有灯都打开一次， 进行拍照保存
			if (1)
			{
				std::unique_lock<std::mutex> lock(g_mutex_wait);
				if (g_wait)
				{
					g_image_process_ok.wait(lock);
				}
				switch (g_Led_Color)
				{
				case WHITE:
					resetColor(255, 255, 255);
					break;
				case RED:
					resetColor(255, 0, 0);
					break;
				case GREEN:
					resetColor(0, 255, 0);
					break;
				case BLUE:
					resetColor(0, 0, 255);
					break;
				}
				printf("**************\n");
				Sleep(50);	//灯的颜色真正设置进显卡
				g_complete_a_cycle = true;
				g_wait = true;

				g_led_set_color_ok.notify_all();
				lock.unlock(); // 解锁.
				Sleep(g_IntervalTime);// Give Main Thread CPU Time
			}

			// 等我处理完图片你再换灯
			std::unique_lock<std::mutex> lock(g_mutex_wait);
			if (g_wait)
			{
				g_image_process_ok.wait(lock);
			}
			resetColor(0, 0, 0);	//所有灯都刷一个颜色后， 需要进行一次重置， 不再影响下一轮
			lock.unlock(); // 解锁.
		}

		if (g_lpAgingLog2 != NULL)
		{
			tm *local = localtime(&g_start_time);
			char format_time[MAXCHAR] = { 0 };
			strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

			g_aging_file << g_PPID << "," << format_time << ",";

			int result_count1 = 0;	// 一个轮回的结果
			int result_count2 = 0;	// 四个轮回的结果
			for (int i = 0; i < g_lpAgingLog2->getSize(); i++)
			{
				SingleLEDHSV* lpdata = g_lpAgingLog2->ptr(i);
				g_aging_file << lpdata->h << ","
					<< lpdata->s << ","
					<< lpdata->v << ","
					<< lpdata->result << ",";
				//printf("%d - [%d, %d, %d] - %d\n", i, lpdata->h, lpdata->s, lpdata->v, lpdata->result);
				result_count1 += lpdata->result;
				result_count2 += lpdata->result;

				if ((i + 1) % g_LedCount == 0)	// 一轮的数据已统计完
				{
					g_aging_file << result_count1;
					result_count1 = 0;	// 一轮数据统计完后归零，重新来过
				}
			}
			g_aging_file << "," << result_count2 << "\n";


			delete g_lpAgingLog2;
		}
	}

	if (colorNum != NULL)
		delete[] colorNum;

	g_main_thread_exit = true;
}

void setColorThread4()
{
	/* 这里存一个灯的映射关系，打开一个灯，需要把前面打开的灯关闭
	 0 - 10
	 1 - 0
	 10 - 9
	 -------------
	 11 - 21
	 12 - 11
	 21 - 20
	 */
	u8 *colorNum = new u8[g_LedCount]{ 0 };
	for (u8 i = 1; i < g_LedCount; i++)
	{
		colorNum[i] = i - 1;
	}
	colorNum[0] = g_LedHalfCount - 1;
	colorNum[g_LedHalfCount] = g_LedCount - 1;

	while (g_AgingTime > 0)
	{
		g_lpAgingLog2 = new AgingLog2((g_StopColor - g_StartColor) * g_LedCount);
		for (g_Led_Color = g_StartColor; g_Led_Color < g_StopColor; g_Led_Color++)
		{
			g_complete_a_cycle = false;
			for (size_t i = 0, t = g_LedHalfCount; i < g_LedHalfCount && t < g_LedCount; i++, t++)
			{
				// 在子线程的每次loop前判定是否需要退出
				// 主线程退出是随机的, 主线程接收到退出key后, 业务循环逻辑立即退出,进入join子线程状态
				// 子线程即便是恰巧在设置完灯后解锁, 主线程因为不再受理业务, 子线程会在下个循环开始前在此处退出
				if (g_main_thread_exit)
				{
					if (g_lpAgingLog2 != NULL)
						delete g_lpAgingLog2;
					return;
				}

				// 一个单生产者-单消费者模型
				std::unique_lock<std::mutex> lock(g_mutex_wait);
				if (g_wait)
				{
					g_image_process_ok.wait(lock);
				}
				//g_mutex_wait.lock();
				setSignleColor(colorNum[i], 0, 0, 0);
				setSignleColor(colorNum[t], 0, 0, 0);

				if (g_Led_Color == WHITE)
				{
					setSignleColor(i, 255, 255, 255);
					setSignleColor(t, 255, 255, 255);
				}
				else if (g_Led_Color == RED)
				{
					setSignleColor(i, 255, 0, 0);
					setSignleColor(t, 255, 0, 0);
				}
				else if (g_Led_Color == GREEN)
				{
					setSignleColor(i, 0, 255, 0);
					setSignleColor(t, 0, 255, 0);
				}
				else if (g_Led_Color == BLUE)
				{
					setSignleColor(i, 0, 0, 255);
					setSignleColor(t, 0, 0, 255);
				}
				g_lpAgingLog2->setCurrentLedIndex(i, t);

				printf("**************\n");
				Sleep(50);	//灯的颜色真正设置进显卡
				g_wait = true;
				//g_mutex_wait.unlock();

				// 消费者线程目前不需要关注生产者的状态
				g_led_set_color_ok.notify_all();
				lock.unlock(); // 解锁.
				Sleep(g_IntervalTime);// Give Main Thread CPU Time
			}

			// 一个轮回结束后，要将所有灯都打开一次， 进行拍照保存
			if (0)
			{
				std::unique_lock<std::mutex> lock(g_mutex_wait);
				if (g_wait)
				{
					g_image_process_ok.wait(lock);
				}
				switch (g_Led_Color)
				{
				case WHITE:
					resetColor(255, 255, 255);
					break;
				case RED:
					resetColor(255, 0, 0);
					break;
				case GREEN:
					resetColor(0, 255, 0);
					break;
				case BLUE:
					resetColor(0, 0, 255);
					break;
				}
				printf("**************\n");
				Sleep(50);	//灯的颜色真正设置进显卡
				g_complete_a_cycle = true;
				g_wait = true;

				g_led_set_color_ok.notify_all();
				lock.unlock(); // 解锁.
				Sleep(g_IntervalTime);// Give Main Thread CPU Time
			}

			// 等我处理完图片你再换灯
			std::unique_lock<std::mutex> lock(g_mutex_wait);
			if (g_wait)
			{
				g_image_process_ok.wait(lock);
			}
			resetColor(0, 0, 0);	//所有灯都刷一个颜色后， 需要进行一次重置， 不再影响下一轮
			lock.unlock(); // 解锁.
		}

		if (g_lpAgingLog2 != NULL)
		{
			tm *local = localtime(&g_start_time);
			char format_time[MAXCHAR] = { 0 };
			strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

			g_aging_file << g_PPID << "," << format_time << ",";

			int result_count1 = 0;	// 一个轮回的结果
			int result_count2 = 0;	// 四个轮回的结果
			for (int i = 0; i < g_lpAgingLog2->getSize(); i++)
			{
				SingleLEDHSV* lpdata = g_lpAgingLog2->ptr(i);
				g_aging_file << lpdata->h << ","
					<< lpdata->s << ","
					<< lpdata->v << ","
					<< lpdata->result << ",";
				//printf("%d - [%d, %d, %d] - %d\n", i, lpdata->h, lpdata->s, lpdata->v, lpdata->result);
				result_count1 += lpdata->result;
				result_count2 += lpdata->result;

				if ((i + 1) % g_LedCount == 0)	// 一轮的数据已统计完
				{
					g_aging_file << result_count1;
					result_count1 = 0;	// 一轮数据统计完后归零，重新来过
				}
			}
			g_aging_file << "," << result_count2 << "\n";


			delete g_lpAgingLog2;
		}

		g_AgingTime--;
	}

	if (colorNum != NULL)
		delete[] colorNum;

	g_main_thread_exit = true;
}

bool openAgingLog()
{
	g_aging_file.open("./aging.csv", std::fstream::out | std::fstream::app);
	if (g_aging_file.is_open())
	{
		// get length of file:
		//g_aging_file.seekg(0, g_aging_file.end);
		//std::streampos length = g_aging_file.tellg();
		//if (length == 0)
		//{
		//	// 添加表头
		//	// PPID|img_name|convert img_name to time string|r,g,b|a8[0]|a8[1]|a8[2]|a8[3]|a8[4]|a8[5]|a8[6]|a8[7]|test result
		//	g_aging_file << "PPID,"
		//		<< "img_name,"
		//		<< "rgb,"
		//		<< "a8[0],"
		//		<< "a8[1],"
		//		<< "a8[2],"
		//		<< "a8[3],"
		//		<< "a8[4],"
		//		<< "a8[5],"
		//		<< "a8[6],"
		//		<< "a8[7],"
		//		<< "test result" << endl;
		//	g_aging_file.flush();
		//}
		return true;
	}
	return false;
}


int min_distance_of_rectangles(const Rect& rect1, const Rect& rect2)
{
	int min_dist;

	//首先计算两个矩形中心点
	Point C1, C2;
	C1.x = rect1.x + (rect1.width / 2);
	C1.y = rect1.y + (rect1.height / 2);
	C2.x = rect2.x + (rect2.width / 2);
	C2.y = rect2.y + (rect2.height / 2);

	// 分别计算两矩形中心点在X轴和Y轴方向的距离
	int Dx, Dy;
	Dx = abs(C2.x - C1.x);
	Dy = abs(C2.y - C1.y);

	//两矩形不相交，在X轴方向有部分重合的两个矩形，最小距离是上矩形的下边线与下矩形的上边线之间的距离
	if ((Dx < ((rect1.width + rect2.width) / 2)) && (Dy >= ((rect1.height + rect2.height) / 2)))
	{
		min_dist = Dy - ((rect1.height + rect2.height) / 2);
	}

	//两矩形不相交，在Y轴方向有部分重合的两个矩形，最小距离是左矩形的右边线与右矩形的左边线之间的距离
	else if ((Dx >= ((rect1.width + rect2.width) / 2)) && (Dy < ((rect1.height + rect2.height) / 2)))
	{
		min_dist = Dx - ((rect1.width + rect2.width) / 2);
	}

	//两矩形不相交，在X轴和Y轴方向无重合的两个矩形，最小距离是距离最近的两个顶点之间的距离，
	// 利用勾股定理，很容易算出这一距离
	else if ((Dx >= ((rect1.width + rect2.width) / 2)) && (Dy >= ((rect1.height + rect2.height) / 2)))
	{
		int delta_x = Dx - ((rect1.width + rect2.width) / 2);
		int delta_y = Dy - ((rect1.height + rect2.height) / 2);
		min_dist = sqrt(delta_x * delta_x + delta_y * delta_y);
	}

	//两矩形相交，最小距离为负值，返回-1
	else
	{
		min_dist = -1;
	}

	return min_dist;
}

void setFrameImgThread(void* lpcapture)
{
	Mat frame;
	VideoCapture* capture = (VideoCapture*)lpcapture;

	char key = '0';
	clock_t startTime;
	int file_index = 1;
	while (true)
	{
		(*capture).read(frame);
		if (g_wait)
		{
			g_mutex_wait.lock();
			startTime = clock();//计时开始

			AgingLog aging;

			Rect rect(40, 280, 1150, 110);	// 画一个截取框出来	
			Mat img = frame(rect);
			//imshow("imga", img);

			Mat frame_clone = frame.clone();
			rectangle(frame_clone, rect, Scalar(255, 255, 255), 5);
			imshow("graphics_card", frame_clone);

			char graphics_card_name[256] = { 0 };
			sprintf_s(graphics_card_name, 256, "./aging_img/%d.png", file_index);
			imwrite(graphics_card_name, frame);

			int count = 0;
			//startTime = clock();
			for (int i = 0; i < img.rows; i++)
			{
				for (int j = 0; j < img.cols; j++)
				{
					uchar* lpdata = img.ptr<uchar>(i, j);
					switch (g_Led_Color)
					{
					case WHITE:
						if (lpdata[2] < R_Threshold
							&& lpdata[1] < G_Threshold
							&& lpdata[0] < B_Threshold)
						{
							lpdata[0] = 0;
							lpdata[1] = 0;
							lpdata[2] = 0;
						}
						else {
							count++;
						}
						break;
					case RED:
						if (lpdata[2] < R_Threshold)
						{
							lpdata[0] = 0;
							lpdata[1] = 0;
							lpdata[2] = 0;
						}
						else {
							count++;
						}
						break;
					case GREEN:
						if (lpdata[1] < G_Threshold)
						{
							lpdata[0] = 0;
							lpdata[1] = 0;
							lpdata[2] = 0;
						}
						else {
							count++;
						}
						break;
					case BLUE:
						if (lpdata[0] < B_Threshold)
						{
							lpdata[0] = 0;
							lpdata[1] = 0;
							lpdata[2] = 0;
						}
						else {
							count++;
						}
						break;
					}

				}
			}
			//printf("time consuming1 = %d\n", clock() - startTime);
			//startTime = clock();

			// 将img 划分成竖向的8列, 统计每列中r >=240 的点数量
			count = 0;
			for (size_t i = 0; i < img.cols; i++)
			{
				//uchar c; //取出指定通道的颜色值
				//int t;   // 该颜色值对应的过滤阈值
				int block_index = i / (img.cols / 8);	// 该像素点所在区块

				for (size_t j = 0; j < img.rows; j++)
				{
					uchar* lpdata = img.ptr<uchar>(j, i);
					switch (g_Led_Color)
					{
					case WHITE:
						if (lpdata[2] > 0
							&& lpdata[1] > 0
							&& lpdata[0] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					case RED:
						if (lpdata[2] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					case GREEN:
						if (lpdata[1] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					case BLUE:
						if (lpdata[0] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					}
				}
			}
			//printf("time consuming2 = %d\n", clock() - startTime);


			bool b8[8] = { 0 };	// 8个区块中, 各个区块是否有color
			int t[2] = { -1, -1 };	// 记录区块下标
			for (int s = 0; s < 8; s++)
			{
				if (s == 0 && aging.point_block[s] > 100)
				{
					b8[s] = true;
					t[0] = s;
				}
				else if (aging.point_block[s] > 100 && !b8[s - 1])
				{
					b8[s] = true;
					if (t[0] >= 0)
						t[1] = s;
					else
						t[0] = s;
				}
				else if (s == 3 && aging.point_block[s] > 70 && !b8[s - 1])
				{
					b8[s] = true;
					if (t[0] >= 0)
						t[1] = s;
					else
						t[0] = s;
				}
			}

			// 划线
			for (int i = 1; i < 8; i++)
				line(img, Point(img.cols / 8 * i, 0), Point(img.cols / 8 * i, img.rows), Scalar(255, 255, 0));

			aging.PPID = 1111111;
			aging.setColor(g_Led_Color);
			time(&aging.img_name);

			if (t[0] < 0 || t[1] < 0)
			{
				const unsigned int* a8 = aging.point_block;

				char aging_img[256] = { 0 };
				//sprintf_s(aging_img, 256, "./aging_img/%I64u.png", aging.img_name);
				sprintf_s(aging_img, 256, "./aging_img_ok/%d.png", file_index);

				char msg[256] = { 0 };
				sprintf_s(msg, 256, "Fail - %s", aging_img);
				putText(img, msg, Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "t[0] = %d t[1] = %d", t[0], t[1]);
				putText(img, msg, Point(0, 80), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "a[0] = %d,a[1] = %d,a[2] = %d,a[3] = %d,a[4] = %d,a[5] = %d,a[6] = %d,a[7] = %d"
					, a8[0], a8[1], a8[2], a8[3], a8[4], a8[5], a8[6], a8[7]);
				putText(img, msg, Point(0, 110), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				imwrite(aging_img, img);
				//imshow("img_fail", img);
				//waitKey(1);
				aging.result = false;
			}
			else
			{
				const unsigned int* a8 = aging.point_block;
				char aging_img[256] = { 0 };
				//sprintf_s(aging_img, 256, "./aging_img_ok/%I64u.png", aging.img_name);
				sprintf_s(aging_img, 256, "./aging_img_ok/%d.png", file_index);

				char msg[256] = { 0 };
				sprintf_s(msg, 256, "Success - %s", aging_img);
				putText(img, msg, Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "t[0] = %d t[1] = %d", t[0], t[1]);
				putText(img, msg, Point(0, 80), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "a[0] = %d,a[1] = %d,a[2] = %d,a[3] = %d,a[4] = %d,a[5] = %d,a[6] = %d,a[7] = %d"
					, a8[0], a8[1], a8[2], a8[3], a8[4], a8[5], a8[6], a8[7]);
				putText(img, msg, Point(0, 110), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				imwrite(aging_img, img);
				aging.result = true;
			}
			file_index++;
			imshow("img", img);

			aging.time_consuming = clock() - startTime;

			//printf("time consuming - %d\n", aging.time_consuming);

			if (g_aging_file.is_open())
			{
				g_aging_file << aging.PPID << ","
					<< aging.img_name << ",\""
					<< aging.color[0] << "," << aging.color[1] << "," << aging.color[2] << "\","
					<< aging.point_block[0] << ","
					<< aging.point_block[1] << ","
					<< aging.point_block[2] << ","
					<< aging.point_block[3] << ","
					<< aging.point_block[4] << ","
					<< aging.point_block[5] << ","
					<< aging.point_block[6] << ","
					<< aging.point_block[7] << ","
					<< aging.result << endl;
				g_aging_file.flush();
			}

			g_wait = false;
			printf("--------------%d\n", aging.time_consuming);
			g_mutex_wait.unlock();
		}
		key = waitKey(30);
		if (key == 0x1b)
		{
			g_main_thread_exit = true;
			break;
		}
		else if (key == 0x20)
		{
			g_mutex_wait.lock();
			waitKey();
			g_mutex_wait.unlock();
		}
	}
}

void setFrameImgThread2(void* lpcapture)
{
	Mat frame;
	VideoCapture* capture = (VideoCapture*)lpcapture;

	char key = '0';
	clock_t startTime;

	while (true)
	{
		(*capture).read(frame);
		if (g_wait)
		{
			//g_mutex_wait.lock();
			std::unique_lock<std::mutex> lock(g_mutex_wait);
			{
				startTime = clock();//计时开始
				(*capture).read(frame); // !important, 确保读取出来的灯是完全点亮的
				AgingLog aging;

				Rect rect(40, 280, 1150, 110);	// 画一个截取框出来	
				Mat img = frame(rect);

				Mat frame_clone = frame.clone();
				rectangle(frame_clone, rect, Scalar(255, 255, 255), 5);
				imshow("graphics_card", frame_clone);

				// 分离成3张单通道image
				Mat channels_m[3];
				split(img, channels_m);
				//cvtColor(img, img, COLOR_BGR2GRAY);
				//imshow("gray_img", img);
				// 直接轉換成二值圖
				switch (g_Led_Color)
				{
				case WHITE:
					cvtColor(img, img, COLOR_BGR2GRAY);
					threshold(img, img, W_Threshold, 255, THRESH_BINARY);
					break;
				case RED:
					threshold(channels_m[2], img, R_Threshold, 255, THRESH_BINARY);
					break;
				case GREEN:
					threshold(channels_m[1], img, G_Threshold, 255, THRESH_BINARY);
					break;
				case BLUE:
					threshold(channels_m[0], img, B_Threshold, 255, THRESH_BINARY);
					break;
				}

				imshow("threshold_img", img);

				//均值滤波
				medianBlur(img, img, 5);

				imshow("medianBlur_img", img);

				//形态学处理
				Mat kernel = getStructuringElement(MORPH_CROSS, Size(4, 4));
				morphologyEx(img, img, MORPH_CLOSE, kernel, Point(-1, -1), 7, BORDER_REPLICATE);

				imshow("morphologyEx_img", img);
				waitKey(1);
				g_wait = false;
				printf("--------------%d\n", clock() - startTime);
				//g_mutex_wait.unlock();
			}
			g_image_process_ok.notify_all();
			lock.unlock();
		}

		key = waitKey(1);
		if (key == 0x1b)	// Esc 键
		{
			g_main_thread_exit = true;
			break;
		}
		else if (key == 0x20) // 空格键
		{
			g_mutex_wait.lock();
			waitKey();
			g_mutex_wait.unlock();
		}
	}
}

void setFrameImgThread3(void* lpcapture)
{
	Mat frame;
	VideoCapture* capture = (VideoCapture*)lpcapture;

	char key = '0';
	clock_t startTime;

	int lowhsv[3] = { 0 };
	int highsv[3] = { 0 };

	while (true)
	{
		(*capture).read(frame);
		if (g_wait)
		{
			//g_mutex_wait.lock();
			// 一个轮回进行一次抓拍并保存			
			if (g_complete_a_cycle)
			{
				std::unique_lock<std::mutex> lock(g_mutex_wait);
				(*capture).read(frame);
				tm *local = localtime(&g_start_time);
				char format_time[MAXCHAR] = { 0 };
				strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

				char frame_file[MAXCHAR] = { 0 };
				sprintf_s(frame_file, MAXCHAR, ".\\aging_original_image\\%s-%s-%d.png", g_PPID, format_time, g_Led_Color);

				imwrite(frame_file, frame);
				g_wait = false;	// 表示扫描线程已经完工了
				g_complete_a_cycle = false;
				g_image_process_ok.notify_all();
				lock.unlock();
				continue;	// 不再走下面的扫描逻辑
			}


			std::unique_lock<std::mutex> lock(g_mutex_wait);
			if (g_Led_Color < AllColor)
			{
				const HsvColor& hsv = g_HsvColor[g_Led_Color];
				lowhsv[0] = hsv.h[0] + hsv.h[5];
				lowhsv[1] = hsv.s[0] + hsv.s[5];
				lowhsv[2] = hsv.v[0] + hsv.v[5];

				highsv[0] = hsv.h[0] + hsv.h[6];
				highsv[1] = hsv.s[0] + hsv.s[6];
				highsv[2] = hsv.v[0] + hsv.v[6];

				startTime = clock();//计时开始
				(*capture).read(frame); // !important, 确保读取出来的灯是完全点亮的
				//AgingLog aging;

				//Rect rect(40, 280, 1150, 110);	// 画一个截取框出来	
				Rect rect(g_RectFrame.x, g_RectFrame.y, g_RectFrame.width, g_RectFrame.height);
				Mat img = frame(rect);
				//Mat img = frame.clone();
				if (g_AgingSettingSaveRectImages)
				{
					tm *local = localtime(&g_start_time);
					char format_time[MAXCHAR] = { 0 };
					strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

					int f = g_lpAgingLog2->getCurrentLedIndex_F();
					int s = g_lpAgingLog2->getCurrentLedIndex_S();

					char frame_file[MAXCHAR] = { 0 };
					sprintf_s(frame_file, MAXCHAR, ".\\aging_rect_image\\%s-%s-%d-%d-%d-original.png", g_PPID, format_time, g_Led_Color, f, s);

					imwrite(frame_file, img);
				}

				Mat frame_clone = frame.clone();
				rectangle(frame_clone, rect, Scalar(255, 255, 0), 5);
				imshow("graphics_card", frame_clone);

				//均值滤波
				//medianBlur(img, img, 7);
				GaussianBlur(img, img, Size(7, 7), 1.0);
				//imshow("medianBlur_img", img);
				//moveWindow("medianBlur_img", 0, 0);

				Mat hsv_img;
				cvtColor(img, hsv_img, COLOR_BGR2HSV);
				//imshow("hsv_img", hsv_img);
				//moveWindow("hsv_img", 0, 150);

				Mat hsv_img_mask;
				inRange(hsv_img, Scalar(lowhsv[0], lowhsv[1], lowhsv[2]), Scalar(highsv[0], highsv[1], highsv[2]), hsv_img_mask);
				//imshow("inRange_mask", hsv_img_mask);
				//moveWindow("inRange_mask", 0, 500);
				if (g_AgingSettingSaveRectImages)
				{
					tm *local = localtime(&g_start_time);
					char format_time[MAXCHAR] = { 0 };
					strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

					int f = g_lpAgingLog2->getCurrentLedIndex_F();
					int s = g_lpAgingLog2->getCurrentLedIndex_S();

					char frame_file[MAXCHAR] = { 0 };
					sprintf_s(frame_file, MAXCHAR, ".\\aging_rect_image\\%s-%s-%d-%d-%d-mask.png", g_PPID, format_time, g_Led_Color, f, s);

					imwrite(frame_file, hsv_img_mask);
				}

				//形态学处理消除噪点
				Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(7, 7));
				morphologyEx(hsv_img_mask, hsv_img_mask, MORPH_CLOSE, kernel);
				//imshow("MORPH_CLOSE", hsv_img_mask);
				//moveWindow("MORPH_CLOSE", 0, 650);

				morphologyEx(hsv_img_mask, hsv_img_mask, MORPH_OPEN, kernel);
				//imshow("MORPH_OPEN", hsv_img_mask);
				//moveWindow("MORPH_OPEN", 0, 800);

				Mat result = Mat::zeros(img.size(), img.type());
				bitwise_and(img, img, result, hsv_img_mask);
				//imshow("bitwise_and", result);
				//moveWindow("bitwise_and", 900, 0);
				if (g_AgingSettingSaveRectImages)
				{
					tm *local = localtime(&g_start_time);
					char format_time[MAXCHAR] = { 0 };
					strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

					int f = g_lpAgingLog2->getCurrentLedIndex_F();
					int s = g_lpAgingLog2->getCurrentLedIndex_S();

					char frame_file[MAXCHAR] = { 0 };
					sprintf_s(frame_file, MAXCHAR, ".\\aging_rect_image\\%s-%s-%d-%d-%d-result.png", g_PPID, format_time, g_Led_Color, f, s);

					imwrite(frame_file, result);
				}

				//存储边缘
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;

				Mat tempBinaryFrame = hsv_img_mask.clone();
				findContours(tempBinaryFrame, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));//查找最顶层轮廓

				//存储
				vector<Rect> boundRect;
				boundRect.clear();
				for (int index = 0; index < contours.size(); index++)
				{
					// 第一个轮廓会首先放入队列
					// 第二个轮廓会跟队列中的最后一个轮廓进行比较，看彼此的距离是否超过阈值
					// 如果没有超过阈值，说明同属一个大轮廓，所以合并两个轮廓后，重新放入队列
					// 如果超出阈值， 说明是两个独立的轮廓
					vector<Point> contours_poly;
					approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
					Rect rect = boundingRect(Mat(contours_poly));
					if (rect.width < 20 || rect.height < 10)// 先过滤掉小点
						continue;
					if (boundRect.empty())
					{
						boundRect.push_back(rect);
						continue;
					}
					else
					{
						auto tail = boundRect.rbegin();
						int gap = 0;
						if (tail->x > rect.x)
						{
							gap = abs(tail->x - (rect.x + rect.width));
						}
						else {
							gap = abs(rect.x - (tail->x + tail->width));
						}

						if (gap < 60)
						{
							Rect big_rect;
							big_rect.x = min(tail->x, rect.x);
							big_rect.y = min(tail->y, rect.y);
							big_rect.width = max(tail->x + tail->width, rect.x + rect.width) - big_rect.x;
							big_rect.height = max(tail->y + tail->height, rect.y + rect.height) - big_rect.y;
							*tail = big_rect;
						}
						else
						{
							boundRect.push_back(rect);
						}
					}
				}
				if (boundRect.size() < 2)
				{
					putText(result, "Failure", Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));
				}
				//得到灯的轮廓
				for (int index = 0; index < boundRect.size(); index++)
				{
					rectangle(result, boundRect[index], Scalar(0, 255, 255), 1);
					const Mat contour = hsv_img(boundRect[index]);

					if (1)
					{
						int hall = 0, sall = 0, vall = 0;
						int point_count = 0;
						for (size_t i = 0; i < contour.rows; i++)
						{
							for (size_t j = 0; j < contour.cols; j++)
							{
								const uchar* lphsv = contour.ptr<uchar>(i, j);
								if (lphsv[0] >= lowhsv[0])
								{
									hall += lphsv[0];
									sall += lphsv[1];
									vall += lphsv[2];
									point_count++;
								}
								//printf("[%d,%d,%d]",lphsv[0], lphsv[1], lphsv[2]);
							}
							//printf("\n");
						}
						if (index == 0 && point_count > 0)
						{
							int f = (g_Led_Color * g_LedCount) + g_lpAgingLog2->getCurrentLedIndex_F();
							SingleLEDHSV* lpSingleLEDHSV = g_lpAgingLog2->ptr(f);
							lpSingleLEDHSV->h = hall / point_count;
							lpSingleLEDHSV->s = sall / point_count;
							lpSingleLEDHSV->v = vall / point_count;
							lpSingleLEDHSV->result = 0;
						}
						else if (index == 1 && point_count > 0)
						{
							int s = (g_Led_Color * g_LedCount) + g_lpAgingLog2->getCurrentLedIndex_S();
							SingleLEDHSV* lpSingleLEDHSV = g_lpAgingLog2->ptr(s);
							lpSingleLEDHSV->h = hall / point_count;
							lpSingleLEDHSV->s = sall / point_count;
							lpSingleLEDHSV->v = vall / point_count;
							lpSingleLEDHSV->result = 0;
						}
					}
				}
				imshow("contours", result);

				waitKey(1);
				g_wait = false;
				printf("--------------%d\n", clock() - startTime);
				//g_mutex_wait.unlock();
			}
			g_image_process_ok.notify_all();
			lock.unlock();
		}

		key = waitKey(1);
		if (key == 0x1b)	// Esc 键
		{
			g_main_thread_exit = true;
			break;
		}
		else if (key == 0x20) // 空格键
		{
			g_mutex_wait.lock();
			waitKey();
			g_mutex_wait.unlock();
		}

		if (g_main_thread_exit)
		{
			break;
		}
	}
}

void setFrameImgThread4(void* lpcapture)
{
	Mat frame;
	VideoCapture* capture = (VideoCapture*)lpcapture;

	char key = '0';
	clock_t startTime;

	int lowhsv[3] = { 0 };
	int highsv[3] = { 0 };

	while (true)
	{
		(*capture).read(frame);
		if (frame.empty())
		{
			(*capture).open("video/GA1.avi");
			continue;
		}
		if (g_wait)
		{
			//g_mutex_wait.lock();
			// 一个轮回进行一次抓拍并保存			
			if (g_complete_a_cycle)
			{
				std::unique_lock<std::mutex> lock(g_mutex_wait);
				(*capture).read(frame);
				tm *local = localtime(&g_start_time);
				char format_time[MAXCHAR] = { 0 };
				strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

				char frame_file[MAXCHAR] = { 0 };
				sprintf_s(frame_file, MAXCHAR, ".\\aging_original_image\\%s-%s-%d.png", g_PPID, format_time, g_Led_Color);

				imwrite(frame_file, frame);
				g_wait = false;	// 表示扫描线程已经完工了
				g_complete_a_cycle = false;
				g_image_process_ok.notify_all();
				lock.unlock();
				continue;	// 不再走下面的扫描逻辑
			}

			std::unique_lock<std::mutex> lock(g_mutex_wait);
			if (g_Led_Color < AllColor)
			{
				const HsvColor& hsv = g_HsvColor[g_Led_Color];
				lowhsv[0] = hsv.h[0] + hsv.h[5];
				lowhsv[1] = hsv.s[0] + hsv.s[5];
				lowhsv[2] = hsv.v[0] + hsv.v[5];

				highsv[0] = hsv.h[0] + hsv.h[6];
				highsv[1] = hsv.s[0] + hsv.s[6];
				highsv[2] = hsv.v[0] + hsv.v[6];

				startTime = clock();//计时开始
				//(*capture).read(frame); // !important, 确保读取出来的灯是完全点亮的
				//AgingLog aging;

				// ROI
				Rect rect(g_RectFrame.x, g_RectFrame.y, g_RectFrame.width, g_RectFrame.height);
				Mat original_img = frame(rect);


				Mat img = original_img.clone();
				if (g_AgingSettingSaveRectImages)
				{
					tm *local = localtime(&g_start_time);
					char format_time[MAXCHAR] = { 0 };
					strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

					int f = g_lpAgingLog2->getCurrentLedIndex_F();
					int s = g_lpAgingLog2->getCurrentLedIndex_S();

					char frame_file[MAXCHAR] = { 0 };
					sprintf_s(frame_file, MAXCHAR, ".\\aging_rect_image\\%s-%s-%d-%d-%d-original.png", g_PPID, format_time, g_Led_Color, f, s);

					imwrite(frame_file, img);
				}

				Mat frame_clone = frame.clone();
				rectangle(frame_clone, rect, Scalar(255, 255, 0), 5);
				imshow("graphics_card", frame_clone);


				for (size_t i = 0; i < img.rows; i++)
				{
					for (size_t j = 0; j < img.cols; j++)
					{
						uchar* lpdata = img.ptr<uchar>(i, j);
						if ((lpdata[0] < g_BGRColors[g_Led_Color].b || lpdata[1] < g_BGRColors[g_Led_Color].g || lpdata[2] < g_BGRColors[g_Led_Color].r)
							|| (lpdata[0] >= g_BGRColors[WHITE].b && lpdata[1] >= g_BGRColors[WHITE].g && lpdata[2] >= g_BGRColors[WHITE].r))
						{
							lpdata[0] = 0;
							lpdata[1] = 0;
							lpdata[2] = 0;
						}
						//switch (g_Led_Color)
						//{
						//	if (/*lpdata[0] > g_BGRColors[g_Led_Color].b && lpdata[1] > g_BGRColors[g_Led_Color].g && lpdata[2] > (g_BGRColors[g_Led_Color].r + g_tick)
						//		||*/ (lpdata[0] < g_BGRColors[g_Led_Color].b || lpdata[1] < g_BGRColors[g_Led_Color].g || lpdata[2] < g_BGRColors[g_Led_Color].r))
						//	{
						//		lpdata[0] = 0;
						//		lpdata[1] = 0;
						//		lpdata[2] = 0;
						//	}
						//
						//default:
						//	break;
						//}
					}
				}				
				imshow("img", img);

				//均值滤波
				medianBlur(img, img, 3);
				//GaussianBlur(img, img, Size(7, 7), 1.0);
				imshow("img blure", img);


				Mat hsv_img;
				cvtColor(img, hsv_img, COLOR_BGR2HSV);
				imshow("hsv_img", hsv_img);

				Mat mask;
				inRange(hsv_img, Scalar(lowhsv[0], lowhsv[1], lowhsv[2]), Scalar(highsv[0], highsv[1], highsv[2]), mask);
				imshow("mask", mask);

				//形态学处理消除噪点
				Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(11, 11));
				dilate(mask, mask, kernel);
				imshow("dilate", mask);

				morphologyEx(mask, mask, MORPH_CLOSE, kernel);
				morphologyEx(mask, mask, MORPH_OPEN, kernel);
				imshow("morphologyEx", mask);

				Mat result = Mat::zeros(img.size(), img.type());
				bitwise_and(original_img, original_img, result, mask);
				imshow("bitwise_and", result);

				//存储边缘
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;
				findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));//查找最顶层轮廓

				// 生成最小包围矩形
				vector<Rect> boundRect;
				for (int index = 0; index < contours.size(); index++)
				{
					// 绘制各自小轮廓
					Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
					drawContours(result, contours, index, color, 2);

					vector<Point> contours_poly;
					approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
					Rect rect = boundingRect(Mat(contours_poly));
					boundRect.push_back(rect);
				}

				//sort(boundRect.begin(), boundRect.end(), [](const Rect& l, const Rect& r) -> bool {return l.x < r.x;});

				// 轮廓合并
				vector<Rect> boundRect2;
				for (int i = 0; i < boundRect.size(); i++)
				{
					Rect& rect = boundRect[i];
					if (rect.area() <= g_MinContoursArea)
					{
						rect = Rect();
						continue;
					}
					for (int j = 0; j < boundRect.size(); j++)
					{
						if (i == j)	// 跳过自己
							continue;
						Rect& rect2 = boundRect[j];
						if (rect2.area() <= g_MinContoursArea)
							continue;
						int gap = min_distance_of_rectangles(rect, rect2);
						//printf("(%d, %d) space (%d, %d) = %d\n", rect.x, rect.y, rect.x, rect.y, gap);
						if (gap < g_MinContoursSpace)
						{
							Rect big_rect;
							big_rect.x = min(rect.x, rect2.x);
							big_rect.y = min(rect.y, rect2.y);
							big_rect.width = max(rect.x + rect.width, rect2.x + rect2.width) - big_rect.x;
							big_rect.height = max(rect.y + rect.height, rect2.y + rect2.height) - big_rect.y;
							rect = big_rect;
							rect2 = Rect();
						}
					}
				}

				//if (boundRect2.size() < 2)
				//{
				//	putText(result, "Failure", Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));
				//}
				//得到灯的轮廓
				for (int index = 0; index < boundRect.size(); index++)
				{
					if (boundRect[index].area() > 0)
					{
						rectangle(result, boundRect[index], Scalar(0, 255, 255), 1);
					}
					const Mat contour = hsv_img(boundRect[index]);

					if (0)
					{
						int hall = 0, sall = 0, vall = 0;
						int point_count = 0;
						for (size_t i = 0; i < contour.rows; i++)
						{
							for (size_t j = 0; j < contour.cols; j++)
							{
								const uchar* lphsv = contour.ptr<uchar>(i, j);
								if (lphsv[0] >= lowhsv[0])
								{
									hall += lphsv[0];
									sall += lphsv[1];
									vall += lphsv[2];
									point_count++;
								}
								//printf("[%d,%d,%d]",lphsv[0], lphsv[1], lphsv[2]);
							}
							//printf("\n");
						}
						if (index == 0 && point_count > 0)
						{
							int f = (g_Led_Color * g_LedCount) + g_lpAgingLog2->getCurrentLedIndex_F();
							SingleLEDHSV* lpSingleLEDHSV = g_lpAgingLog2->ptr(f);
							lpSingleLEDHSV->h = hall / point_count;
							lpSingleLEDHSV->s = sall / point_count;
							lpSingleLEDHSV->v = vall / point_count;
							lpSingleLEDHSV->result = 0;
						}
						else if (index == 1 && point_count > 0)
						{
							int s = (g_Led_Color * g_LedCount) + g_lpAgingLog2->getCurrentLedIndex_S();
							SingleLEDHSV* lpSingleLEDHSV = g_lpAgingLog2->ptr(s);
							lpSingleLEDHSV->h = hall / point_count;
							lpSingleLEDHSV->s = sall / point_count;
							lpSingleLEDHSV->v = vall / point_count;
							lpSingleLEDHSV->result = 0;
						}
					}
				}
				imshow("contours", result);

				waitKey(1);
				g_wait = false;
				printf("--------------%d\n", clock() - startTime);
				//g_mutex_wait.unlock();
			}
			g_image_process_ok.notify_all();
			lock.unlock();
		}

		key = waitKey(30);
		if (key == 0x1b)	// Esc 键
		{
			g_main_thread_exit = true;
			break;
		}
		else if (key == 0x20) // 空格键
		{
			g_mutex_wait.lock();
			waitKey();
			g_mutex_wait.unlock();
		}

		if (g_main_thread_exit)
		{
			break;
		}
	}
}

void renderTrackbarThread()
{
	if (!g_ShowTrackBarWnd)
		return;
	int empty_w = 400, empty_h = 100;
	Mat empty = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
	namedWindow("Toolkit");
	imshow("Toolkit", empty);
	int hl = 0, sl = 0, vl = 0;
	int hh = 0, sh = 0, vh = 0;

	Mat empty2 = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
	namedWindow("Toolkit_RGB");
	imshow("Toolkit_RGB", empty2);

	char buf[128] = { 0 };
	while (true)
	{
		if (g_main_thread_exit) {
			break;
		}
		if (g_Led_Color >= AllColor)// 防止越界
			continue;
		
		HsvColor& hsv = g_HsvColor[g_Led_Color];
		createTrackbar("lowHue", "Toolkit", &hsv.h[5], hsv.h[4]);
		createTrackbar("higHue", "Toolkit", &hsv.h[6], hsv.h[4]);

		createTrackbar("lowSat", "Toolkit", &hsv.s[5], hsv.s[4]);
		createTrackbar("higSat", "Toolkit", &hsv.s[6], hsv.s[4]);

		createTrackbar("lowVal", "Toolkit", &hsv.v[5], hsv.v[4]);
		createTrackbar("higVal", "Toolkit", &hsv.v[6], hsv.v[4]);

		hl = hsv.h[0] + hsv.h[5];
		sl = hsv.s[0] + hsv.s[5];
		vl = hsv.v[0] + hsv.v[5];

		hh = hsv.h[0] + hsv.h[6];
		sh = hsv.s[0] + hsv.s[6];
		vh = hsv.v[0] + hsv.v[6];

		sprintf_s(buf, 128, "lowHSV < higHSV !!!");
		putText(empty, buf, Point(0, empty.rows / 4 * 1), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);

		sprintf_s(buf, 128, "Real Low HSV (%d, %d, %d)", hl, sl, vl);
		putText(empty, buf, Point(0, empty.rows / 4 * 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);

		sprintf_s(buf, 128, "Real High HSV (%d, %d, %d)", hh, sh, vh);
		putText(empty, buf, Point(0, empty.rows / 4 * 3), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);
		imshow("Toolkit", empty);
		empty = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);

		g_mutex_wait.lock();
		createTrackbar("b", "Toolkit_RGB", &g_BGRColors[g_Led_Color].color[0], 255);
		createTrackbar("g", "Toolkit_RGB", &g_BGRColors[g_Led_Color].color[1], 255);
		createTrackbar("r", "Toolkit_RGB", &g_BGRColors[g_Led_Color].color[2], 255);

		createTrackbar("tick", "Toolkit_RGB", &g_tick, 200);
		g_mutex_wait.unlock();

		imshow("Toolkit_RGB", empty2);

		waitKey(1);
	}
}

void initConfigFile()
{
	TCHAR lpPath[MAX_PATH] = { 0 };
	wcscpy_s(lpPath, MAX_PATH, L"./3c.ini");

	//[Camera]
	WritePrivateProfileString(L"Camera", L"Index", L"0", lpPath);

	//[Frame]
	WritePrivateProfileString(L"Frame", L"Width", L"1280", lpPath);
	WritePrivateProfileString(L"Frame", L"Hight", L"780", lpPath);

	//[RectFrame]
	WritePrivateProfileString(L"RectFrame", L"X", L"200", lpPath);
	WritePrivateProfileString(L"RectFrame", L"Y", L"240", lpPath);
	WritePrivateProfileString(L"RectFrame", L"Width", L"900", lpPath);
	WritePrivateProfileString(L"RectFrame", L"Hight", L"200", lpPath);

	//[AgingSetting]
	WritePrivateProfileString(L"AgingSetting", L"SaveRectImages ", L"1", lpPath);

	//[LED]
	WritePrivateProfileString(L"LED", L"Count", L"22", lpPath);
	WritePrivateProfileString(L"LED", L"IntervalTime", L"100", lpPath);

	//[TrackBarWindow]
	WritePrivateProfileString(L"TrackBarWindow", L"IsShow", L"1", lpPath);

	//[RedThreshold]
	//{156, 180, 159, 180, 43, 255, 149, 255, 46, 255, 148, 255} // Red
	WritePrivateProfileString(L"RedThreshold", L"Lh", L"159", lpPath);
	WritePrivateProfileString(L"RedThreshold", L"Hh", L"180", lpPath);
	WritePrivateProfileString(L"RedThreshold", L"Ls", L"149", lpPath);
	WritePrivateProfileString(L"RedThreshold", L"Hs", L"255", lpPath);
	WritePrivateProfileString(L"RedThreshold", L"Lv", L"148", lpPath);
	WritePrivateProfileString(L"RedThreshold", L"Hv", L"255", lpPath);

	//[GreenThreshold]
	//{35, 77, 35, 77, 43, 255, 43, 255, 46, 255, 136, 255}	// Green
	WritePrivateProfileString(L"GreenThreshold", L"Lh", L"35", lpPath);
	WritePrivateProfileString(L"GreenThreshold", L"Hh", L"77", lpPath);
	WritePrivateProfileString(L"GreenThreshold", L"Ls", L"43", lpPath);
	WritePrivateProfileString(L"GreenThreshold", L"Hs", L"255", lpPath);
	WritePrivateProfileString(L"GreenThreshold", L"Lv", L"136", lpPath);
	WritePrivateProfileString(L"GreenThreshold", L"Hv", L"255", lpPath);

	//[BlueThreshold]
	//{100, 124, 100, 124, 43, 255, 43, 255, 46, 255, 176, 255} // Blue
	WritePrivateProfileString(L"BlueThreshold", L"Lh", L"100", lpPath);
	WritePrivateProfileString(L"BlueThreshold", L"Hh", L"124", lpPath);
	WritePrivateProfileString(L"BlueThreshold", L"Ls", L"43", lpPath);
	WritePrivateProfileString(L"BlueThreshold", L"Hs", L"255", lpPath);
	WritePrivateProfileString(L"BlueThreshold", L"Lv", L"176", lpPath);
	WritePrivateProfileString(L"BlueThreshold", L"Hv", L"255", lpPath);

	//[WhiteThreshold]
	//{0,   180, 125, 180, 0,  30,  2,   30,  221, 255, 221, 255} // White
	WritePrivateProfileString(L"WhiteThreshold", L"Lh", L"125", lpPath);
	WritePrivateProfileString(L"WhiteThreshold", L"Hh", L"180", lpPath);
	WritePrivateProfileString(L"WhiteThreshold", L"Ls", L"2", lpPath);
	WritePrivateProfileString(L"WhiteThreshold", L"Hs", L"30", lpPath);
	WritePrivateProfileString(L"WhiteThreshold", L"Lv", L"221", lpPath);
	WritePrivateProfileString(L"WhiteThreshold", L"Hv", L"255", lpPath);

}

void readConfigFile()
{
	TCHAR lpPath[MAX_PATH] = { 0 };
	wcscpy_s(lpPath, MAX_PATH, L"./3c.ini");

	//[Camera]
	g_CameraIndex = GetPrivateProfileInt(L"Camera", L"Index", 0, lpPath);

	//[Frame]
	g_FrameSize.width = GetPrivateProfileInt(L"Frame", L"Width", 1280, lpPath);
	g_FrameSize.height = GetPrivateProfileInt(L"Frame", L"Hight", 780, lpPath);

	//[RectFrame]
	g_RectFrame.x = GetPrivateProfileInt(L"RectFrame", L"X", 200, lpPath);
	g_RectFrame.y = GetPrivateProfileInt(L"RectFrame", L"Y", 240, lpPath);
	g_RectFrame.width = GetPrivateProfileInt(L"RectFrame", L"Width", 900, lpPath);
	g_RectFrame.height = GetPrivateProfileInt(L"RectFrame", L"Hight", 200, lpPath);

	//[AgingSetting]
	g_AgingSettingSaveRectImages = GetPrivateProfileInt(L"AgingSetting", L"SaveRectImages ", 1, lpPath);
	g_AgingTime = GetPrivateProfileInt(L"AgingSetting", L"AgingTime ", 1, lpPath);
	g_IntervalTime = GetPrivateProfileInt(L"AgingSetting", L"IntervalTime", 100, lpPath);
	g_MinContoursArea = GetPrivateProfileInt(L"AgingSetting", L"MinContoursArea", 200, lpPath);
	g_MinContoursSpace = GetPrivateProfileInt(L"AgingSetting", L"MinContoursSpace", 60, lpPath);

	//[LED]
	g_LedCount = GetPrivateProfileInt(L"LED", L"Count", 22, lpPath);
	g_StartColor = (LEDColor)GetPrivateProfileInt(L"LED", L"StartColor", 0, lpPath);
	g_StopColor = (LEDColor)GetPrivateProfileInt(L"LED", L"StopColor", 4, lpPath);

	//[TrackBarWindow]
	g_ShowTrackBarWnd = GetPrivateProfileInt(L"TrackBarWindow", L"IsShow", 1, lpPath);

	g_BGRColors[BLUE].b = GetPrivateProfileInt(L"ThresholdB", L"b", 250, lpPath);
	g_BGRColors[BLUE].g = GetPrivateProfileInt(L"ThresholdB", L"g", 250, lpPath);
	g_BGRColors[BLUE].r = GetPrivateProfileInt(L"ThresholdB", L"r", 250, lpPath);

	g_BGRColors[GREEN].b = GetPrivateProfileInt(L"ThresholdG", L"b", 250, lpPath);
	g_BGRColors[GREEN].g = GetPrivateProfileInt(L"ThresholdG", L"g", 250, lpPath);
	g_BGRColors[GREEN].r = GetPrivateProfileInt(L"ThresholdG", L"r", 250, lpPath);

	g_BGRColors[RED].b = GetPrivateProfileInt(L"ThresholdR", L"b", 250, lpPath);
	g_BGRColors[RED].g = GetPrivateProfileInt(L"ThresholdR", L"g", 250, lpPath);
	g_BGRColors[RED].r = GetPrivateProfileInt(L"ThresholdR", L"r", 250, lpPath);

	g_BGRColors[WHITE].b = GetPrivateProfileInt(L"ThresholdW", L"b", 250, lpPath);
	g_BGRColors[WHITE].g = GetPrivateProfileInt(L"ThresholdW", L"g", 250, lpPath);
	g_BGRColors[WHITE].r = GetPrivateProfileInt(L"ThresholdW", L"r", 250, lpPath);

	//[RedThreshold]
	//{156, 180, 159, 180, 43, 255, 149, 255, 46, 255, 148, 255} // Red
	int r_Lh = GetPrivateProfileInt(L"RedThreshold", L"Lh", 159, lpPath);
	int r_Hh = GetPrivateProfileInt(L"RedThreshold", L"Hh", 180, lpPath);
	int r_Ls = GetPrivateProfileInt(L"RedThreshold", L"Ls", 149, lpPath);
	int r_Hs = GetPrivateProfileInt(L"RedThreshold", L"Hs", 255, lpPath);
	int r_Lv = GetPrivateProfileInt(L"RedThreshold", L"Lv", 148, lpPath);
	int r_Hv = GetPrivateProfileInt(L"RedThreshold", L"Hv", 255, lpPath);

	//[GreenThreshold]
	//{35, 77, 35, 77, 43, 255, 43, 255, 46, 255, 136, 255}	// Green
	int g_Lh = GetPrivateProfileInt(L"GreenThreshold", L"Lh", 35, lpPath);
	int g_Hh = GetPrivateProfileInt(L"GreenThreshold", L"Hh", 77, lpPath);
	int g_Ls = GetPrivateProfileInt(L"GreenThreshold", L"Ls", 43, lpPath);
	int g_Hs = GetPrivateProfileInt(L"GreenThreshold", L"Hs", 255, lpPath);
	int g_Lv = GetPrivateProfileInt(L"GreenThreshold", L"Lv", 136, lpPath);
	int g_Hv = GetPrivateProfileInt(L"GreenThreshold", L"Hv", 255, lpPath);

	//[BlueThreshold]
	//{100, 124, 100, 124, 43, 255, 43, 255, 46, 255, 176, 255} // Blue
	int b_Lh = GetPrivateProfileInt(L"BlueThreshold", L"Lh", 100, lpPath);
	int b_Hh = GetPrivateProfileInt(L"BlueThreshold", L"Hh", 124, lpPath);
	int b_Ls = GetPrivateProfileInt(L"BlueThreshold", L"Ls", 43, lpPath);
	int b_Hs = GetPrivateProfileInt(L"BlueThreshold", L"Hs", 255, lpPath);
	int b_Lv = GetPrivateProfileInt(L"BlueThreshold", L"Lv", 176, lpPath);
	int b_Hv = GetPrivateProfileInt(L"BlueThreshold", L"Hv", 255, lpPath);

	//[WhiteThreshold]
	//{0,   180, 125, 180, 0,  30,  2,   30,  221, 255, 221, 255} // White
	int w_Lh = GetPrivateProfileInt(L"WhiteThreshold", L"Lh", 125, lpPath);
	int w_Hh = GetPrivateProfileInt(L"WhiteThreshold", L"Hh", 180, lpPath);
	int w_Ls = GetPrivateProfileInt(L"WhiteThreshold", L"Ls", 2, lpPath);
	int w_Hs = GetPrivateProfileInt(L"WhiteThreshold", L"Hs", 30, lpPath);
	int w_Lv = GetPrivateProfileInt(L"WhiteThreshold", L"Lv", 221, lpPath);
	int w_Hv = GetPrivateProfileInt(L"WhiteThreshold", L"Hv", 255, lpPath);

	g_HsvColor[RED] = { 0, 180, r_Lh, r_Hh, 0, 255, r_Ls, r_Hs, 0, 255, r_Lv, r_Hv };
	g_HsvColor[GREEN] = { 0, 180, g_Lh, g_Hh, 0, 255, g_Ls, g_Hs, 0, 255, g_Lv, g_Hv };
	g_HsvColor[BLUE] = { 0, 180, b_Lh, b_Hh, 0, 255, b_Ls, b_Hs, 0, 255, b_Lv, b_Hv };
	g_HsvColor[WHITE] = { 0, 180, w_Lh, w_Hh, 0,  255, w_Ls, w_Hs, 0, 255, w_Lv, w_Hv };
}

int main000()
{
	fstream f(CONFIG_FILE, std::fstream::in);
	if (!f.good())
		initConfigFile();
	else
		readConfigFile();
	f.close();
#if true	// 全局变量初始化
	getVGAInfo(g_PPID, VGA_PPID_LENGTH);
	g_start_time = time(NULL); //获取日历时间
#endif
	Mat frame;
	VideoCapture capture;
	//VideoCapture capture(g_CameraIndex);
	//capture.set(CAP_PROP_SETTINGS, 1);
	capture.set(CAP_PROP_FRAME_WIDTH, g_FrameSize.width);
	capture.set(CAP_PROP_FRAME_HEIGHT, g_FrameSize.height);

	HINSTANCE hDLL;		// Handle to DLL
	hDLL = LoadLibrary(L"VGA_Extra_x64.dll");
	LOAD_VENDOR_DLL = (lpLoadVenderDLL)GetProcAddress(hDLL, "LoadVenderDLL");
	VGA_READ_IC_I2C = (lpVGA_Read_IC_I2C)GetProcAddress(hDLL, "VGA_Read_IC_I2C");
	VGA_WRITE_IC_I2C = (lpVGA_Write_IC_I2C)GetProcAddress(hDLL, "VGA_Write_IC_I2C");

	// 载入dll
	LOAD_VENDOR_DLL();

	// 关闭所有灯
	resetColor(0, 0, 0);

	// 初始化log文件句柄
	if (!openAgingLog())
	{
		printf("open aging.csv error!\n");
		return -1;
	}

	std::thread t1(setColorThread4);
	std::thread t2(setFrameImgThread4, &capture);
	std::thread t3(renderTrackbarThread);

#if 0
	//char key = '0';
	//unsigned long index = 0;
	//clock_t startTime;
	while (false)
	{
		capture.read(frame);
		if (g_wait)
		{
			AgingLog aging;
			//printf("_clock1 = %u\n", clock());
			g_mutex_wait.lock();
			startTime = clock();//计时开始

			Rect rect(230, 180, 850, 300);	// 画一个截取框出来	
			Mat img = frame(rect);
			imshow("img", img);

			Mat frame_clone = frame.clone();
			rectangle(frame_clone, rect, Scalar(255, 255, 255), 5);
			imshow("graphics_card", frame_clone);

			int count = 0;
			for (int row = 0; row < img.rows; row++)
			{
				for (int c = 0; c < img.cols; c++)
				{
					switch (g_Led_Color)
					{
					case WHITE:
						if (img.at<Vec3b>(row, c)[2] < R_Threshold
							&& img.at<Vec3b>(row, c)[1] < G_Threshold
							&& img.at<Vec3b>(row, c)[0] < B_Threshold)
						{
							img.at<Vec3b>(row, c)[0] = 0;
							img.at<Vec3b>(row, c)[1] = 0;
							img.at<Vec3b>(row, c)[2] = 0;
						}
						else {
							count++;
						}
						break;
					case RED:
						if (img.at<Vec3b>(row, c)[2] < R_Threshold)
						{
							img.at<Vec3b>(row, c)[0] = 0;
							img.at<Vec3b>(row, c)[1] = 0;
							img.at<Vec3b>(row, c)[2] = 0;
						}
						else {
							count++;
						}
						break;
					case GREEN:
						if (img.at<Vec3b>(row, c)[1] < G_Threshold)
						{
							img.at<Vec3b>(row, c)[0] = 0;
							img.at<Vec3b>(row, c)[1] = 0;
							img.at<Vec3b>(row, c)[2] = 0;
						}
						else {
							count++;
						}
						break;
					case BLUE:
						if (img.at<Vec3b>(row, c)[0] < B_Threshold)
						{
							img.at<Vec3b>(row, c)[0] = 0;
							img.at<Vec3b>(row, c)[1] = 0;
							img.at<Vec3b>(row, c)[2] = 0;
						}
						else {
							count++;
						}
						break;
					}

				}
			}
			//printf("count1 = %d\n", count);

			// 将img 划分成竖向的8列, 统计每列中r >=240 的点数量

			count = 0;
			for (size_t i = 0; i < img.cols; i++)
			{
				//uchar c; //取出指定通道的颜色值
				//int t;		// 该颜色值对应的过滤阈值
				int block_index = i / (img.cols / 8);	// 该像素点所在区块

				for (size_t j = 0; j < img.rows; j++)
				{
					//int b = img.at<Vec3b>(j, i)[0];
					//int g = img.at<Vec3b>(j, i)[1];
					//uchar r = img.at<Vec3b>(j, i)[2];
					//if (i < img.cols * 1 / 8 && r >= 240)
					//{
					//	aging.point_block[0]++;
					//}
					//else if (i < img.cols * 2 / 8 && r >= 240)
					//{
					//	aging.point_block[1]++;
					//}
					//else if (i < img.cols * 3 / 8 && r >= 240)
					//{
					//	aging.point_block[2]++;
					//}
					//else if (i < img.cols * 4 / 8 && r >= 240)
					//{
					//	aging.point_block[3]++;
					//}
					//else if (i < img.cols * 5 / 8 && r >= 240)
					//{
					//	aging.point_block[4]++;
					//}
					//else if (i < img.cols * 6 / 8 && r >= 240)
					//{
					//	aging.point_block[5]++;
					//}
					//else if (i < img.cols * 7 / 8 && r >= 240)
					//{
					//	aging.point_block[6]++;
					//}
					//else if (i < img.cols && r >= 240)
					//{
					//	aging.point_block[7]++;
					//}

					switch (g_Led_Color)
					{
					case WHITE:
						if (img.at<Vec3b>(j, i)[2] > 0
							&& img.at<Vec3b>(j, i)[1] > 0
							&& img.at<Vec3b>(j, i)[0] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					case RED:
						if (img.at<Vec3b>(j, i)[2] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					case GREEN:
						if (img.at<Vec3b>(j, i)[1] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					case BLUE:
						if (img.at<Vec3b>(j, i)[0] > 0)
						{
							aging.point_block[block_index]++;
							count++;
						}
						break;
					}
				}
			}
			//printf("count2 = %d\n", count);


			bool b8[8] = { 0 };	// 8个区块中, 各个区块是否有白色
			int t[2] = { -1, -1 };	// 记录区块下标
			for (int s = 0; s < 8; s++)
			{
				if (s == 0 && aging.point_block[s] > 100)
				{
					b8[s] = true;
					t[0] = s;
				}
				else if (aging.point_block[s] > 100 && !b8[s - 1])
				{
					b8[s] = true;
					if (t[0] >= 0)
						t[1] = s;
					else
						t[0] = s;
				}
				else if (s == 3 && aging.point_block[s] > 70 && !b8[s - 1])
				{
					b8[s] = true;
					if (t[0] >= 0)
						t[1] = s;
					else
						t[0] = s;
				}
			}

			aging.PPID = 1111111;
			aging.setColor(g_Led_Color);
			time(&aging.img_name);

			if (t[0] < 0 || t[1] < 0)
			{
				const unsigned int* a8 = aging.point_block;
				char fail_img_name[256] = { 0 };
				sprintf_s(fail_img_name, 256, "./fail_img/%I64u.png", aging.img_name);

				char msg[256] = { 0 };
				sprintf_s(msg, 256, "Fail - %d", index++);
				putText(img, msg, Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "%s t[0] = %d t[1] = %d", fail_img_name, t[0], t[1]);
				putText(img, msg, Point(0, 80), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "a[0] = %d,a[1] = %d,a[2] = %d,a[3] = %d,a[4] = %d,a[5] = %d,a[6] = %d,a[7] = %d"
					, a8[0], a8[1], a8[2], a8[3], a8[4], a8[5], a8[6], a8[7]);
				putText(img, msg, Point(0, 110), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				imwrite(fail_img_name, img);
				imshow("img_fail", img);
				waitKey(1);
				aging.result = false;
			}
			else
			{
				const unsigned int* a8 = aging.point_block;
				char fail_img_name[256] = { 0 };
				sprintf_s(fail_img_name, 256, "./fail_img/%I64u.png", aging.img_name);

				char msg[256] = { 0 };
				sprintf_s(msg, 256, "Success - %d", index++);
				putText(img, msg, Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "%s t[0] = %d t[1] = %d", fail_img_name, t[0], t[1]);
				putText(img, msg, Point(0, 80), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				sprintf_s(msg, 256, "a[0] = %d,a[1] = %d,a[2] = %d,a[3] = %d,a[4] = %d,a[5] = %d,a[6] = %d,a[7] = %d"
					, a8[0], a8[1], a8[2], a8[3], a8[4], a8[5], a8[6], a8[7]);
				putText(img, msg, Point(0, 110), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));

				//imwrite(fail_img_name, img);
				aging.result = true;
			}

			for (int i = 1; i < 8; i++)
				line(img, Point(img.cols / 8 * i, 0), Point(img.cols / 8 * i, img.rows), Scalar(255, 255, 0));

			imshow("imgb", img);

			aging.time_consuming = clock() - startTime;

			//printf("time consuming - %d\n", aging.time_consuming);

			if (g_aging_file.is_open())
			{
				//char asctime_img_name[128] = { 0 };
				//tm *lptime = gmtime(&aging.img_name);
				//sprintf_s(asctime_img_name, 128, "%d%02d%02d%02d%02d%02d"
				//	, lptime->tm_year + 1900, lptime->tm_mon, lptime->tm_mday
				//	, lptime->tm_hour, lptime->tm_hour, lptime->tm_sec);

				//char buf[256] = { 0 };
				// PPID|img_name|convert img_name to time string|r,g,b|a8[0]|a8[1]|a8[2]|a8[3]|a8[4]|a8[5]|a8[6]|a8[7]|test result
				g_aging_file << aging.PPID << ","
					<< aging.img_name << ",\""
					<< aging.color[0] << "," << aging.color[1] << "," << aging.color[2] << "\","
					<< aging.point_block[0] << ","
					<< aging.point_block[1] << ","
					<< aging.point_block[2] << ","
					<< aging.point_block[3] << ","
					<< aging.point_block[4] << ","
					<< aging.point_block[5] << ","
					<< aging.point_block[6] << ","
					<< aging.point_block[7] << ","
					<< aging.result << endl;
				g_aging_file.flush();
			}

			g_wait = false;
			printf("--------------\n");
			g_mutex_wait.unlock();
		}

		if (g_main_thread_exit)
		{
			break;
		}
	}
#endif
	t1.join();
	t2.join();
	t3.join();
	FreeLibrary(hDLL);
	g_aging_file.close();
	destroyAllWindows();
	return 0;
}


int main456()
{
	HINSTANCE hDLL;		// Handle to DLL
	hDLL = LoadLibrary(L"VGA_Extra_x64.dll");
	LOAD_VENDOR_DLL = (lpLoadVenderDLL)GetProcAddress(hDLL, "LoadVenderDLL");
	VGA_READ_IC_I2C = (lpVGA_Read_IC_I2C)GetProcAddress(hDLL, "VGA_Read_IC_I2C");
	VGA_WRITE_IC_I2C = (lpVGA_Write_IC_I2C)GetProcAddress(hDLL, "VGA_Write_IC_I2C");

	// 载入dll
	LOAD_VENDOR_DLL();

	readConfigFile();

	// 关闭所有灯
	resetColor(0, 0, 0);
	Sleep(30);

	Mat frame;
	Mat frame1;
	//g_FrameSize.width = (640);
	//g_FrameSize.height = (480);
	VideoCapture capture(0);
	capture.set(CAP_PROP_SETTINGS, 1);
	capture.set(CAP_PROP_FPS, 30);
	capture.set(CAP_PROP_FRAME_WIDTH, g_FrameSize.width);
	capture.set(CAP_PROP_FRAME_HEIGHT, g_FrameSize.height);

	u8 *colorNum = new u8[g_LedCount]{ 0 };
	for (u8 i = 1; i < g_LedCount; i++)
	{
		colorNum[i] = i - 1;
	}
	//colorNum[0] = g_LedHalfCount - 1;
	//colorNum[g_LedHalfCount] = g_LedCount - 1;
	colorNum[0] = g_LedCount - 1;

	Sleep(1000);
	capture >> frame;
	Rect rect(g_RectFrame.x, g_RectFrame.y, g_RectFrame.width, g_RectFrame.height);
	frame = frame(rect);
	imshow("1111", frame);
	//waitKey(30);


	clock_t startTime0, startTime,startTime2;
	int lowhsv[3] = { 0 };
	int highsv[3] = { 0 };
	std::thread t3(renderTrackbarThread);


	//while (true)
	{
		startTime0 = clock();
		for (g_Led_Color = BLUE; g_Led_Color < WHITE; g_Led_Color++)
		{			
			size_t t = 0;
			for (size_t index = 0; index < LED_COUNT; index++)
			{
				startTime = clock();
				startTime2 = clock();
				setSignleColor(colorNum[index], 0, 0, 0);
				//setSignleColor(colorNum[t], 0, 0, 0);

				/*if (g_Led_Color == WHITE)
				{
					setSignleColor(index, 255, 255, 255);
					setSignleColor(t, 255, 255, 255);
				}
				else*/ if (g_Led_Color == RED)
				{
					setSignleColor(index, 255, 0, 0);
					//setSignleColor(t, 255, 0, 0);
				}
				else if (g_Led_Color == GREEN)
				{
					setSignleColor(index, 0, 255, 0);
					//setSignleColor(t, 0, 255, 0);
				}
				else if (g_Led_Color == BLUE)
				{
					setSignleColor(index, 0, 0, 255);
					//setSignleColor(t, 0, 0, 255);
				}
				printf("1--------------%d\n", clock() - startTime);
				startTime = clock();

				for (int i = 0; i < 3; i++)
				{
					startTime = clock();
					capture.read(frame1);	
					waitKey(33);
					printf("1.1--------------%d\n", clock() - startTime);

					
				}
				startTime = clock();

				Rect rect(g_RectFrame.x, g_RectFrame.y, g_RectFrame.width, g_RectFrame.height);
				frame1 = frame1(rect);
				
				imshow("2222", frame1);
				printf("2--------------%d\n", clock() - startTime);
				startTime = clock();
				//Mat frame1_result = frame1.clone();

				for (int i = 0; i < frame.rows; i++)
				{
					for (int j = 0; j < frame.cols; j++)
					{
						uchar* pdata = frame.ptr<uchar>(i, j);
						uchar* pdata1 = frame1.ptr<uchar>(i, j);

						
						switch (g_Led_Color)
						{
						case RED:
							if ((pdata1[2] - pdata[2] < 150))
							{
								pdata1[0] = pdata1[1] = pdata1[2] = 0;
							}
							break;
						case GREEN:
							if ((pdata1[1] - pdata[1] < 150))
							{
								pdata1[0] = pdata1[1] = pdata1[2] = 0;
							}
							break;
						case BLUE:
							if ((pdata1[0] - pdata[0] < 150))
							{
								pdata1[0] = pdata1[1] = pdata1[2] = 0;
							}
							break;
						/*case WHITE:
							if ((pdata1[2] - pdata[2] < 150))
							{
								pdata1[0] = pdata1[1] = pdata1[2] = 0;
							}
							break;*/
						}
					}
				}
				
				printf("3--------------%d\n", clock() - startTime);
				startTime = clock();

				const HsvColor& hsv = g_HsvColor[g_Led_Color];
				lowhsv[0] = hsv.h[0] + hsv.h[5];
				lowhsv[1] = hsv.s[0] + hsv.s[5];
				lowhsv[2] = hsv.v[0] + hsv.v[5];

				highsv[0] = hsv.h[0] + hsv.h[6];
				highsv[1] = hsv.s[0] + hsv.s[6];
				highsv[2] = hsv.v[0] + hsv.v[6];
				
				//均值滤波
				medianBlur(frame1, frame1, 3);
				//GaussianBlur(img, img, Size(7, 7), 1.0);
				//imshow("frame1 blure", frame1);
				printf("4--------------%d\n", clock() - startTime);
				startTime = clock();

				Mat mask;
				Mat frame1_hsv = frame1.clone();
				cvtColor(frame1, frame1_hsv, COLOR_BGR2HSV);
				inRange(frame1_hsv, Scalar(lowhsv[0], lowhsv[1], lowhsv[2]), Scalar(highsv[0], highsv[1], highsv[2]), mask);

				//inRange(frame1, Scalar(35, 43, 46), Scalar(124, 255, 255), hsv_img_mask);
				//形态学处理消除噪点
				//Mat result1, result2;
				//Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
				//Mat kernel2 = getStructuringElement(MORPH_ELLIPSE, Size(7, 7));

				//dilate(hsv_img_mask, hsv_img_mask, kernel);
				//imshow("hsv_img_mask", hsv_img_mask);
				//
				//// 7-腐蚀
				//erode(hsv_img_mask, result1, kernel2);
				//imshow("result1", result1);
				//
				//// 5-腐蚀
				//erode(hsv_img_mask, result2, kernel);
				//imshow("result2", result2);
				//morphologyEx(hsv_img_mask, hsv_img_mask, MORPH_OPEN, kernel);

				imshow("mask", mask);
				printf("5--------------%d\n", clock() - startTime);
				startTime = clock();

				//Mat result = Mat::zeros(frame1.size(), frame1.type());
				//存储边缘
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;
				findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));//查找最顶层轮廓

				// 生成最小包围矩形
				vector<Rect> boundRect;
				for (int index = 0; index < contours.size(); index++)
				{
					// 绘制各自小轮廓
					//Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
					//drawContours(result, contours, index, color, 2);

					vector<Point> contours_poly;
					approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
					Rect rect = boundingRect(Mat(contours_poly));
					boundRect.push_back(rect);
				}

				//sort(boundRect.begin(), boundRect.end(), [](const Rect& l, const Rect& r) -> bool {return l.x < r.x;});

				// 轮廓合并
				//vector<Rect> boundRect2;
				for (int i = 0; i < boundRect.size(); i++)
				{
					Rect& rect = boundRect[i];
					//if (rect.area() <= g_MinContoursArea)
					//{
					//	rect = Rect();
					//	continue;
					//}
					for (int j = 0; j < boundRect.size(); j++)
					{
						if (i == j)	// 跳过自己
							continue;
						Rect& rect2 = boundRect[j];
						//if (rect2.area() <= g_MinContoursArea)
						//	continue;
						int gap = min_distance_of_rectangles(rect, rect2);
						//printf("(%d, %d) space (%d, %d) = %d\n", rect.x, rect.y, rect.x, rect.y, gap);
						if (gap < g_MinContoursSpace)
						{
							Rect big_rect;
							big_rect.x = min(rect.x, rect2.x);
							big_rect.y = min(rect.y, rect2.y);
							big_rect.width = max(rect.x + rect.width, rect2.x + rect2.width) - big_rect.x;
							big_rect.height = max(rect.y + rect.height, rect2.y + rect2.height) - big_rect.y;
							rect = big_rect;
							rect2 = Rect();
						}
					}
				}
				
				if (boundRect.size() < 1)
				{
					putText(frame1, "Failure", Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));
				}
				//得到灯的轮廓
				for (int index = 0; index < boundRect.size(); index++)
				{
					if (boundRect[index].area() > 0)
					{
						rectangle(frame1, boundRect[index], Scalar(0, 255, 255), 3);
					}
				}
				imshow("result", frame1);
				printf("6--------------%d\n", clock() - startTime);
				printf("--------------%d\n", clock() - startTime2);
				
				//if (index > 8 && index < 13)
				//{
				//	char name[128] = { 0 };
				//	sprintf_s(name, 128, "aging_rect_image/frame1_result-%d-%d.png", rand(), index);
				//	imwrite(name, frame1_result);
				//}
			}
			/*if (char(waitKey(1)) == 'q')
			{
				waitKey();
			}*/
		}
		printf("--------------%d\n", clock() - startTime0);

		waitKey(1);
	}
	t3.join();
	waitKey();
	return 0;
}

#if 0
Mat g_frame;
Mat g_current_frame;
Mat g_background_frame;

void getFrame(Mat& f)
{
	g_mutex_wait.lock();
	f = g_frame.clone();	
	g_mutex_wait.unlock();
}

void autoGetCaptureFrame(void* arg)
{
	VideoCapture* lpcapture = (VideoCapture*)arg;
	while (true)
	{
		g_mutex_wait.lock();
		lpcapture->read(g_frame);		
		g_mutex_wait.unlock();
		imshow("g_frame", g_frame);

		if (waitKey(33) == 0x1b)	// Esc 键
		{
			break;
		}
	}
}


void findFrameContours()
{
	int currentColor = 0;
	while (true)
	{
		while (g_wait)
		{
			Sleep(1);
		}
		{
			currentColor = g_Led_Color;
			//printf("this color = %d\n", currentColor);

			//while (false)
			{
				Mat frame, mask;
				getFrame(frame);	// get current frame
				printf("\n2--------------\n");
				if (frame.empty())
				{
					printf("current frame empty !\n");
					return;
				}

				clock_t startTime0 = clock(), startTime = clock();
				int lowhsv[3] = { 0 };
				int highsv[3] = { 0 };

				Rect rect(g_RectFrame.x, g_RectFrame.y, g_RectFrame.width, g_RectFrame.height);
				frame = frame(rect);
				Mat original_frame = frame.clone();
				//printf("--------------cols = %d, rols = %d", frame.cols, frame.rows);
				//namedWindow("original_frame");
				imshow("original_frame", frame);
				imshow("background", g_background_frame(rect));

				for (int i = 0; i < frame.rows; i++)
				{
					for (int j = 0; j < frame.cols; j++)
					{
						uchar* lpback = g_background_frame.ptr<uchar>(i, j);
						uchar* lpframe = frame.ptr<uchar>(i, j);

						switch (currentColor)
						{
						case RED:
							if ((lpframe[2] - lpback[2] < 150))
							{
								lpframe[0] = lpframe[1] = lpframe[2] = 0;
							}
							break;
						case GREEN:
							if ((lpframe[1] - lpback[1] < 150))
							{
								lpframe[0] = lpframe[1] = lpframe[2] = 0;
							}
							break;
						case BLUE:
							if ((lpframe[0] - lpback[0] < 150))
							{
								lpframe[0] = lpframe[1] = lpframe[2] = 0;
							}
							break;
						//case WHITE:
						//	if ((lpframe[0] - lpback[0] < 180) || (lpframe[1] - lpback[1] < 180) || (lpframe[2] - lpback[2] < 180))
						//	{
						//		lpframe[0] = lpframe[1] = lpframe[2] = 0;
						//	}
						//	break;
						}
					}
				}

				if (currentColor == WHITE)
				{
					Mat background = g_background_frame(rect);
					Mat temp_frame = frame.clone();
					cvtColor(background, background, COLOR_BGR2GRAY);
					cvtColor(temp_frame, temp_frame, COLOR_BGR2GRAY);					
					absdiff(background, temp_frame, temp_frame);
					threshold(temp_frame, temp_frame, 50, 255, THRESH_BINARY);
					medianBlur(temp_frame, temp_frame, 3);
					imshow("background", background);
					imshow("temp_frame", temp_frame);
					waitKey(1);

				}
				printf("3--------------%d\n", clock() - startTime);
				startTime = clock();

				const HsvColor& hsv = g_HsvColor[currentColor];
				lowhsv[0] = hsv.h[0] + hsv.h[5];
				lowhsv[1] = hsv.s[0] + hsv.s[5];
				lowhsv[2] = hsv.v[0] + hsv.v[5];

				highsv[0] = hsv.h[0] + hsv.h[6];
				highsv[1] = hsv.s[0] + hsv.s[6];
				highsv[2] = hsv.v[0] + hsv.v[6];

				//均值滤波
				medianBlur(frame, frame, 3);
				printf("4--------------%d\n", clock() - startTime);
				startTime = clock();


				cvtColor(frame, frame, COLOR_BGR2HSV);
				inRange(frame, Scalar(lowhsv[0], lowhsv[1], lowhsv[2]), Scalar(highsv[0], highsv[1], highsv[2]), mask);
				cv::imshow("mask", mask);
				printf("5--------------%d\n", clock() - startTime);
				startTime = clock();

				//存储边缘
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;
				findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));//查找最顶层轮廓

				// 生成最小包围矩形
				vector<Rect> boundRect;
				for (int index = 0; index < contours.size(); index++)
				{
					vector<Point> contours_poly;
					approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
					Rect rect = boundingRect(Mat(contours_poly));
					boundRect.push_back(rect);
				}

				//sort(boundRect.begin(), boundRect.end(), [](const Rect& l, const Rect& r) -> bool {return l.x < r.x;});

				// 轮廓合并
				for (int i = 0; i < boundRect.size(); i++)
				{
					Rect& rect = boundRect[i];
					for (int j = 0; j < boundRect.size(); j++)
					{
						if (i == j)	// 跳过自己
							continue;
						Rect& rect2 = boundRect[j];
						int gap = min_distance_of_rectangles(rect, rect2);
						//printf("(%d, %d) space (%d, %d) = %d\n", rect.x, rect.y, rect.x, rect.y, gap);
						if (gap < g_MinContoursSpace)
						{
							Rect big_rect;
							big_rect.x = min(rect.x, rect2.x);
							big_rect.y = min(rect.y, rect2.y);
							big_rect.width = max(rect.x + rect.width, rect2.x + rect2.width) - big_rect.x;
							big_rect.height = max(rect.y + rect.height, rect2.y + rect2.height) - big_rect.y;
							rect = big_rect;
							rect2 = Rect();
						}
					}
				}

				if (boundRect.size() < 1)
				{
					putText(original_frame, "Failure", Point(0, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255));
				}
				//得到灯的轮廓
				for (int index = 0; index < boundRect.size(); index++)
				{
					if (boundRect[index].area() > 0)
					{
						rectangle(original_frame, boundRect[index], Scalar(0, 255, 255), 3);
					}
				}
				imshow("result", original_frame);

				waitKey(1);
				printf("6--------------%d\n", clock() - startTime);
				printf("7--------------%d\n", clock() - startTime0);
			}
			g_wait = true;
		}
	}	
}

int main0223()
{
	initVGA();

	readConfigFile();

	VideoCapture capture(0);
	if (!capture.isOpened())
	{
		printf("error capture not open\n!");
		return -1;
	}
	//capture.set(CAP_PROP_SETTINGS, 1);
	capture.set(CAP_PROP_FPS, 30);
	capture.set(CAP_PROP_FRAME_WIDTH, g_FrameSize.width);
	capture.set(CAP_PROP_FRAME_HEIGHT, g_FrameSize.height);

	g_wait = true;
	std::thread t1(autoGetCaptureFrame, (void*)&capture);	
	std::thread t2(findFrameContours);

	// 关闭所有灯
	resetColor(0, 0, 0);
	Sleep(100);
	getFrame(g_background_frame);	
	//imshow("g_background_frame", g_background_frame);
	//waitKey(1);

	u8 *colorNum = new u8[g_LedCount]{ 0 };
	for (u8 i = 1; i < g_LedCount; i++)
	{
		colorNum[i] = i - 1;
	}
	colorNum[0] = g_LedCount - 1;

	clock_t startTime0, startTime;

	while (true)
	{
		g_wait = true;
		startTime0 = clock();
		for (int color = g_Led_Color = WHITE; color < AllColor; g_Led_Color = ++color)
		{
			if (g_Led_Color >= AllColor)
				g_Led_Color = WHITE;
			for (size_t index = 0; index < LED_COUNT; index++)
			{
				setSignleColor(colorNum[index], 0, 0, 0);				
				if (color == RED)
				{
					setSignleColor(index, 255, 0, 0);
				}
				else if (color == GREEN)
				{
					setSignleColor(index, 0, 255, 0);
				}
				else if (color == BLUE)
				{
					setSignleColor(index, 0, 0, 255);
				}
				else if (color == WHITE)
				{
					setSignleColor(index, 255, 255, 255);
				}
				Sleep(100);
				//printf("led = %d, current = %d, last = %d\n", index, i, g_Led_Color);
				g_wait = false;
				Sleep(20);
			}
		}
		printf("main ==== %d\n", clock() - startTime0);
	}
	

	t1.join();
	t2.join();
	return 0;
}

#endif

#endif