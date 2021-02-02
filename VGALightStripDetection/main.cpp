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
bool g_complete_a_cycle = false;	// ���һ���ֻأ� ����һ��ץ��
std::mutex g_mutex_wait;
std::condition_variable g_led_set_color_ok; // ��������, ָʾLED ���Ѿ����óɹ�
std::condition_variable g_image_process_ok; // ��������, ָʾ���ץ�ģ��Ѿ������ͼƬ
std::fstream g_aging_file;
int g_Led_Color = WHITE;
time_t g_start_time;

int g_CameraIndex = 0;
Size g_FrameSize = Size(1280, 780);
Rect g_RectFrame = Rect(200, 240, 900, 200);
int g_LedCount = 22;
int g_LedHalfCount = g_LedCount / 2;
bool g_ShowTrackBarWnd = true;
int g_IntervalTime = 100;		// ��������ļ��ʱ��
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
	// ��ȡһ�������ĵƴ�
	Mat red_0 = imread("./img/red_bands00.png");
	if (!red_0.data)
	{
		printf("ERROR : could not load red_bands.png .\n");
		return -1;
	}

	//rgb_less_250_set_to_0(red_0);

	imshow("red_0", red_0);
	moveWindow("red_0", 300, 0);

	// �������ƴ�ת�ɻҶ�ͼ, ��Ϊ�Ա�Ŀ��
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

	// ��ȡ��һ���Ƶĵƴ�, ��ת�ɻҶ�ͼ
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

	// ��ȡ��2���Ƶĵƴ�, ��ת�ɻҶ�ͼ
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

	// ��ȡ��3���Ƶĵƴ�, ��ת�ɻҶ�ͼ
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
	// 4. ��ʼ������ֱ��ͼ��Ҫ��ʵ��(bins, ��Χ��ͨ�� H �� S ).
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

// LED �Ƶĵ�ַ
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
	u8 bufLight[3] = { r,g,b };	// Ҫ�򿪵ĵ���ɫ
	u8 bufDelight[3] = { 0,0,0 };// Ҫ�ص��ĵ�

	eneWriteRegs(0x8160 + ledNumlight * 3, bufLight, 3);

	eneWriteRegs(0x8160 + ledNumDelight * 3, bufDelight, 3);

	eneWriteReg(0x8021, 1);	// ����Ϊ��̬
	eneWriteReg(0x802f, 1);	// ��������
}
#else

// ����Ӳ��ƽ̨, ����led�ƹ�
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

// ����Ϊ�ض���ɫ
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
	/* �����һ���Ƶ�ӳ���ϵ����һ���ƣ���Ҫ��ǰ��򿪵ĵƹر�
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
					// �����̵߳�ÿ��loopǰ�ж��Ƿ���Ҫ�˳�
					// ���߳��˳��������, ���߳̽��յ��˳�key��, ҵ��ѭ���߼������˳�,����join���߳�״̬
					// ���̼߳�����ǡ����������ƺ����, ���߳���Ϊ��������ҵ��, ���̻߳����¸�ѭ����ʼǰ�ڴ˴��˳�
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
					Sleep(50);	//�Ƶ���ɫ�������ý��Կ�
					g_wait = true;
					g_mutex_wait.unlock();
					Sleep(50);// Give Main Thread CPU Time
				}

				// ��Main �ѻ�����
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
	/* �����һ���Ƶ�ӳ���ϵ����һ���ƣ���Ҫ��ǰ��򿪵ĵƹر�
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
				// �����̵߳�ÿ��loopǰ�ж��Ƿ���Ҫ�˳�
				// ���߳��˳��������, ���߳̽��յ��˳�key��, ҵ��ѭ���߼������˳�,����join���߳�״̬
				// ���̼߳�����ǡ����������ƺ����, ���߳���Ϊ��������ҵ��, ���̻߳����¸�ѭ����ʼǰ�ڴ˴��˳�
				if (g_main_thread_exit)
				{
					return;
				}

				// һ����������-��������ģ��
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
				Sleep(50);	//�Ƶ���ɫ�������ý��Կ�
				g_wait = true;
				//g_mutex_wait.unlock();

				// �������߳�Ŀǰ����Ҫ��ע�����ߵ�״̬
				g_led_set_color_ok.notify_all();
				lock.unlock(); // ����.
				Sleep(2000);// Give Main Thread CPU Time
			}
		}
	}

}

void setColorThread3()
{
	/* �����һ���Ƶ�ӳ���ϵ����һ���ƣ���Ҫ��ǰ��򿪵ĵƹر�
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
				// �����̵߳�ÿ��loopǰ�ж��Ƿ���Ҫ�˳�
				// ���߳��˳��������, ���߳̽��յ��˳�key��, ҵ��ѭ���߼������˳�,����join���߳�״̬
				// ���̼߳�����ǡ����������ƺ����, ���߳���Ϊ��������ҵ��, ���̻߳����¸�ѭ����ʼǰ�ڴ˴��˳�
				if (g_main_thread_exit)
				{
					if (g_lpAgingLog2 != NULL)
						delete g_lpAgingLog2;
					return;
				}

				// һ����������-��������ģ��
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
				Sleep(50);	//�Ƶ���ɫ�������ý��Կ�
				g_wait = true;
				//g_mutex_wait.unlock();

				// �������߳�Ŀǰ����Ҫ��ע�����ߵ�״̬
				g_led_set_color_ok.notify_all();
				lock.unlock(); // ����.
				Sleep(g_IntervalTime);// Give Main Thread CPU Time
			}
			
			// һ���ֻؽ�����Ҫ�����еƶ���һ�Σ� �������ձ���
			if(1)
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
				Sleep(50);	//�Ƶ���ɫ�������ý��Կ�
				g_complete_a_cycle = true;
				g_wait = true;

				g_led_set_color_ok.notify_all();
				lock.unlock(); // ����.
				Sleep(g_IntervalTime);// Give Main Thread CPU Time
			}

			// ���Ҵ�����ͼƬ���ٻ���
			std::unique_lock<std::mutex> lock(g_mutex_wait);
			if (g_wait)
			{
				g_image_process_ok.wait(lock);
			}
			resetColor(0, 0, 0);	//���еƶ�ˢһ����ɫ�� ��Ҫ����һ�����ã� ����Ӱ����һ��
			lock.unlock(); // ����.
		}

		if (g_lpAgingLog2 != NULL)
		{
			tm *local = localtime(&g_start_time);
			char format_time[MAXCHAR] = { 0 };
			strftime(format_time, MAXCHAR, "%Y%m%d%H%M%S", local);

			g_aging_file << g_PPID << ","<< format_time << ",";

			int result_count1 = 0;	// һ���ֻصĽ��
			int result_count2 = 0;	// �ĸ��ֻصĽ��
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

				if ((i + 1) % g_LedCount == 0)	// һ�ֵ�������ͳ����
				{
					g_aging_file << result_count1;
					result_count1 = 0;	// һ������ͳ�������㣬��������
				}
			}
			g_aging_file << ","<< result_count2 <<"\n";

			
			delete g_lpAgingLog2;
		}
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
		//	// ��ӱ�ͷ
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
			startTime = clock();//��ʱ��ʼ

			AgingLog aging;

			Rect rect(40, 280, 1150, 110);	// ��һ����ȡ�����	
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

			// ��img ���ֳ������8��, ͳ��ÿ����r >=240 �ĵ�����
			count = 0;
			for (size_t i = 0; i < img.cols; i++)
			{
				//uchar c; //ȡ��ָ��ͨ������ɫֵ
				//int t;   // ����ɫֵ��Ӧ�Ĺ�����ֵ
				int block_index = i / (img.cols / 8);	// �����ص���������

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


			bool b8[8] = { 0 };	// 8��������, ���������Ƿ���color
			int t[2] = { -1, -1 };	// ��¼�����±�
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

			// ����
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
				startTime = clock();//��ʱ��ʼ
				(*capture).read(frame); // !important, ȷ����ȡ�����ĵ�����ȫ������
				AgingLog aging;

				Rect rect(40, 280, 1150, 110);	// ��һ����ȡ�����	
				Mat img = frame(rect);

				Mat frame_clone = frame.clone();
				rectangle(frame_clone, rect, Scalar(255, 255, 255), 5);
				imshow("graphics_card", frame_clone);

				// �����3�ŵ�ͨ��image
				Mat channels_m[3];
				split(img, channels_m);
				//cvtColor(img, img, COLOR_BGR2GRAY);
				//imshow("gray_img", img);
				// ֱ���D�Q�ɶ�ֵ�D
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

				//��ֵ�˲�
				medianBlur(img, img, 5);

				imshow("medianBlur_img", img);

				//��̬ѧ����
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
		if (key == 0x1b)	// Esc ��
		{
			g_main_thread_exit = true;
			break;
		}
		else if (key == 0x20) // �ո��
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
			// һ���ֻؽ���һ��ץ�Ĳ�����			
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
				g_wait = false;	// ��ʾɨ���߳��Ѿ��깤��
				g_complete_a_cycle = false;
				g_image_process_ok.notify_all();
				lock.unlock();
				continue;	// �����������ɨ���߼�
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

				startTime = clock();//��ʱ��ʼ
				(*capture).read(frame); // !important, ȷ����ȡ�����ĵ�����ȫ������
				//AgingLog aging;

				//Rect rect(40, 280, 1150, 110);	// ��һ����ȡ�����	
				Rect rect(g_RectFrame.x, g_RectFrame.y, g_RectFrame.width, g_RectFrame.height);
				Mat img = frame(rect);
				//Mat img = frame.clone();
				if(g_AgingSettingSaveRectImages)
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
								
				//��ֵ�˲�
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

				//��̬ѧ�����������
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

				//�洢��Ե
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;

				Mat tempBinaryFrame = hsv_img_mask.clone();
				findContours(tempBinaryFrame, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));//�����������

				//�洢
				vector<Rect> boundRect;
				boundRect.clear();
				for (int index = 0; index < contours.size(); index++)
				{
					// ��һ�����������ȷ������
					// �ڶ���������������е����һ���������бȽϣ����˴˵ľ����Ƿ񳬹���ֵ
					// ���û�г�����ֵ��˵��ͬ��һ�������������Ժϲ��������������·������
					// ���������ֵ�� ˵������������������
					vector<Point> contours_poly;
					approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
					Rect rect = boundingRect(Mat(contours_poly));
					if (rect.width < 20 || rect.height < 10)// �ȹ��˵�С��
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
				//�õ��Ƶ�����
				for (int index = 0; index < boundRect.size(); index++)
				{
					rectangle(result, boundRect[index], Scalar(0, 255, 255), 1);
					const Mat contour = hsv_img(boundRect[index]);
					
					if(1)
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
		if (key == 0x1b)	// Esc ��
		{
			g_main_thread_exit = true;
			break;
		}
		else if (key == 0x20) // �ո��
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

	char buf[128] = { 0 };
	while (true)
	{
		if (g_main_thread_exit) {
			break;
		}
		if (g_Led_Color >= AllColor)// ��ֹԽ��
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

	//[LED]
	g_LedCount = GetPrivateProfileInt(L"LED", L"Count", 22, lpPath);
	g_IntervalTime = GetPrivateProfileInt(L"LED", L"IntervalTime", 100, lpPath);

	//[TrackBarWindow]
	g_ShowTrackBarWnd = GetPrivateProfileInt(L"TrackBarWindow", L"IsShow", 1, lpPath);

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

	g_HsvColor[RED] = { 156, 180, r_Lh, r_Hh, 43, 255, r_Ls, r_Hs, 46, 255, r_Lv, r_Hv };
	g_HsvColor[GREEN] = { 35, 77, g_Lh, g_Hh, 43, 255, g_Ls, g_Hs, 46, 255, g_Lv, g_Hv };
	g_HsvColor[BLUE] = { 100, 124, b_Lh, b_Hh, 43, 255, b_Ls, b_Hs, 46, 255, b_Lv, b_Hv };
	g_HsvColor[WHITE] = { 0, 180, w_Lh, w_Hh, 0,  30, w_Ls, w_Hs, 221, 255, w_Lv, w_Hv };
}

int main()
{
	fstream f(CONFIG_FILE, std::fstream::in);
	if (!f.good())
		initConfigFile();
	else
		readConfigFile();
	f.close();
#if true	// ȫ�ֱ�����ʼ��
	getVGAInfo(g_PPID, VGA_PPID_LENGTH);
	g_start_time = time(NULL); //��ȡ����ʱ��
#endif
	Mat frame;
	VideoCapture capture(g_CameraIndex);
	//capture.set(CAP_PROP_SETTINGS, 1);
	capture.set(CAP_PROP_FRAME_WIDTH, g_FrameSize.width);
	capture.set(CAP_PROP_FRAME_HEIGHT, g_FrameSize.height);

	HINSTANCE hDLL;		// Handle to DLL
	hDLL = LoadLibrary(L"VGA_Extra_x64.dll");
	LOAD_VENDOR_DLL = (lpLoadVenderDLL)GetProcAddress(hDLL, "LoadVenderDLL");
	VGA_READ_IC_I2C = (lpVGA_Read_IC_I2C)GetProcAddress(hDLL, "VGA_Read_IC_I2C");
	VGA_WRITE_IC_I2C = (lpVGA_Write_IC_I2C)GetProcAddress(hDLL, "VGA_Write_IC_I2C");

	// ����dll
	LOAD_VENDOR_DLL();

	// �ر����е�
	resetColor(0, 0, 0);

	// ��ʼ��log�ļ����
	if (!openAgingLog())
	{
		printf("open aging.csv error!\n");
		return -1;
	}

	std::thread t1(setColorThread3);
	std::thread t2(setFrameImgThread3, &capture);
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
			startTime = clock();//��ʱ��ʼ

			Rect rect(230, 180, 850, 300);	// ��һ����ȡ�����	
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

			// ��img ���ֳ������8��, ͳ��ÿ����r >=240 �ĵ�����

			count = 0;
			for (size_t i = 0; i < img.cols; i++)
			{
				//uchar c; //ȡ��ָ��ͨ������ɫֵ
				//int t;		// ����ɫֵ��Ӧ�Ĺ�����ֵ
				int block_index = i / (img.cols / 8);	// �����ص���������

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


			bool b8[8] = { 0 };	// 8��������, ���������Ƿ��а�ɫ
			int t[2] = { -1, -1 };	// ��¼�����±�
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