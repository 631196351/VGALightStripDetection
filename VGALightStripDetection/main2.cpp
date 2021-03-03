#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <vector>
#include <Windows.h>
#include <thread>
#include <mutex>
#include <fstream>

#include "ConfigData.h"
//#include "main.h"
#include "nvbase.h"
//#include "ene_program.h"
#include "utility.h"
#include  "AgingLog.h"

using namespace cv;
using namespace std;

#define DebugMode(oper) if(g_Config.debugMode == true){oper;};

Mat g_frame;
Mat g_current_frame;
Mat g_background_frame;

std::mutex g_get_frame_mutex;
std::mutex g_set_led_mutex;

ConfigData g_Config;

int g_Led = BLUE;
int g_Index = 0;
//int g_Index222 = 0;	// 为了测试getFrame获取到的frame是否准确
bool g_wait = false;
bool g_main_thread_exit = false;
bool g_randomShutDownLed = false;
//VideoCapture g_capture;
//AgingLog* g_aging = nullptr;	// 为了在getFrame 中保存测试frame

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

void renderTrackbarThread()
{
	if (!g_Config.showTrackBarWnd)
		return;
	int empty_w = 400, empty_h = 100;
	Mat empty = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
	namedWindow("Toolkit");
	imshow("Toolkit", empty);
	int hl = 0, sl = 0, vl = 0;
	int hh = 0, sh = 0, vh = 0;

	//Mat empty2 = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
	//namedWindow("Toolkit_RGB");
	//imshow("Toolkit_RGB", empty2);

	char buf[128] = { 0 };
	while (true)
	{
		if (g_main_thread_exit) {
			break;
		}
		if (g_Led >= AllColor)// 防止越界
			continue;

		HsvColor& hsv = g_Config.hsvColor[g_Led];
		createTrackbar("lowHue", "Toolkit", &hsv.h[5], hsv.h[4]);
		createTrackbar("higHue", "Toolkit", &hsv.h[6], hsv.h[4]);

		createTrackbar("lowSat", "Toolkit", &hsv.s[5], hsv.s[4]);
		createTrackbar("higSat", "Toolkit", &hsv.s[6], hsv.s[4]);

		createTrackbar("lowVal", "Toolkit", &hsv.v[5], hsv.v[4]);
		createTrackbar("higVal", "Toolkit", &hsv.v[6], hsv.v[4]);

		int& thresold = g_Config.bgrColorThres[g_Led];
		createTrackbar("thresold", "Toolkit", &thresold, 255);

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
		

		waitKey(30);
	}
}

void getFrame(Mat& f)
{
	for (int i = 0; i < 4; i++) 
	{
		cv::waitKey(33);

		g_get_frame_mutex.lock();
		f = g_frame.clone();
		g_get_frame_mutex.unlock();

		f = f(g_Config.rect);

		/*{
			clock_t t2 = clock();
			char name[128] = { 0 };
			sprintf_s(name, 128, "%s/%s/%02d%02d_getFrame_%d.jpg", AgingFolder, g_aging->ppid(), g_Led, g_Index222, clock());
			printf("\ngetFrame5--------------%d\n", clock() - t2);
			t2 = clock();
			imwrite(name, f);
			printf("\ngetFrame6--------------%d\n", clock() - t2);
		}*/
	}
}

void autoGetCaptureFrame(VideoCapture& capture)
{
	Mat camera;
	while (true)
	{
		if (g_main_thread_exit) {
			break;
		}
		g_get_frame_mutex.lock();
		capture.read(g_frame);
		g_get_frame_mutex.unlock();

		camera = g_frame.clone();
		rectangle(camera, g_Config.rect, Scalar(0, 255, 255), 5);
		imshow("camera", camera);

		if (cv::waitKey(33) == 0x1b)	// Esc 键
		{
			g_main_thread_exit = true;			
		}
	}
}

void getSelectROI(VideoCapture& capture)
{
	Mat roi;
	capture.read(roi);
	Rect r = selectROI(roi);
	printf("\nselectROI = (%d, %d, %d, %d)\n", r.x, r.y, r.width, r.height);
	g_Config.rect = r;
	g_Config.resetRect = false;
	cv::destroyWindow("ROI selector");
	g_Config.saveConfigData();
}

//void getPPIDThread(AgingLog& aging)
//{
//	char ppid[VGA_PPID_LENGTH] = { 0 };
//	getVGAInfo(ppid, VGA_PPID_LENGTH);
//
//	aging.setPPID(ppid, VGA_PPID_LENGTH);
//}

void findFrameContours(AgingLog& aging)
{	
	clock_t startTime0 = clock(), startTime = clock();
	while (true)
	{
		if (g_main_thread_exit) {
			break;
		}

		g_set_led_mutex.lock();
		if(g_wait)
		{
			startTime0 = clock(), startTime = clock();

			int currentColor = g_Led;
			int currentIndex = g_Index;
			printf("\ncurrentColor = %d, currentIndex = %d\n", currentColor, currentIndex);
			
			{				
				Mat original_frame, frame, mask, back;

				original_frame = g_current_frame.clone();
				frame = original_frame.clone();
				back = g_background_frame.clone();
				
				printf("\n2--------------%d\n", clock() - startTime);
				startTime = clock();

				if (original_frame.empty())
				{
					printf("current frame empty !\n");
					return;
				}
								
				int lowhsv[3] = { 0 };
				int highsv[3] = { 0 };				

				DebugMode(imshow("original_frame", frame));
				DebugMode(imshow("background", back));
												
				if (currentColor == WHITE)
				{
					Mat frame_gray, back_gray, temp;
					cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
					cvtColor(back, back_gray, COLOR_BGR2GRAY);

					bitwise_xor(frame_gray, back_gray, temp);	// 取出两幅图所有的不同点，记为temp集合
					DebugMode(imshow("bitwise_xor", temp));

					//在 temp 集合中找ROI部分的点
					bitwise_and(frame_gray, temp, mask);
					DebugMode(imshow("bitwise_and", mask));

					medianBlur(mask, mask, 3);

					threshold(mask, mask, g_Config.bgrColorThres[currentColor], 255, THRESH_TOZERO);
				}
				else 
				{
					for (int i = 0; i < frame.rows; i++)
					{
						for (int j = 0; j < frame.cols; j++)
						{
							const uchar* lpback = back.ptr<uchar>(i, j);
							uchar* lpframe = frame.ptr<uchar>(i, j);

							switch (currentColor)
							{
							case RED:
								if ((lpframe[2] - lpback[2] < g_Config.bgrColorThres[currentColor]))
								{
									lpframe[0] = lpframe[1] = lpframe[2] = 0;
								}
								break;
							case GREEN:
								if ((lpframe[1] - lpback[1] < g_Config.bgrColorThres[currentColor]))
								{
									lpframe[0] = lpframe[1] = lpframe[2] = 0;
								}
								break;
							case BLUE:
								if ((lpframe[0] - lpback[0] < g_Config.bgrColorThres[currentColor]))
								{
									lpframe[0] = lpframe[1] = lpframe[2] = 0;
								}
								break;
							}
						}
					}

					printf("3--------------%d\n", clock() - startTime);
					startTime = clock();

					const HsvColor& hsv = g_Config.hsvColor[currentColor];
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
					
					printf("5--------------%d\n", clock() - startTime);
					startTime = clock();
				}
				
				DebugMode(imshow("mask", mask));

				{
					char name[128] = { 0 };
					sprintf_s(name, 128, "%s/%s/%02d%02d_mask.png", AgingFolder, aging.ppid(), currentColor, currentIndex);
					imwrite(name, mask);
				}

				//存储边缘
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;
				findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));//查找最顶层轮廓

				Mat result = Mat::zeros(original_frame.size(), original_frame.type());
				// 生成最小包围矩形
				vector<Rect> boundRect;
				for (int index = 0; index < contours.size(); index++)
				{
					// 绘制各自小轮廓
					Scalar color = Scalar(rand() % 255, rand() % 255, rand() % 255);
					drawContours(result, contours, index, color, 1);

					vector<Point> contours_poly;
					approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
					Rect rect = boundingRect(contours_poly);
					boundRect.push_back(rect);
				}

				DebugMode(imshow("contours", result));
				{
					char name[128] = { 0 };
					sprintf_s(name, 128, "%s/%s/%02d%02d_contours.png", AgingFolder, aging.ppid(), currentColor, currentIndex);
					imwrite(name, result);
				}
				
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
						if (gap < g_Config.minContoursSpace)
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

				//得到灯的轮廓
				size_t empty_rect = 0;
				for (int index = 0; index < boundRect.size(); index++)
				{
					if (boundRect[index].area() > 0)
					{
						rectangle(original_frame, boundRect[index], Scalar(0, 255, 255), 3);
						aging.setSingleLedResult(currentIndex, currentColor, Pass);
					}
					else {
						empty_rect++;
					}
				}
				if (empty_rect == boundRect.size())
				{
					putText(original_frame, "Failure", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255));
					if(g_randomShutDownLed)
						aging.setSingleLedResult(currentIndex, currentColor, Fail_RandomShutDownLed);
					else
						aging.setSingleLedResult(currentIndex, currentColor, Fail);

				}
				{
					char name[128] = { 0 };
					sprintf_s(name, 128, "%s/%s/%02d%02d_original.png", AgingFolder, aging.ppid(), currentColor, currentIndex);
					imwrite(name, original_frame);
				}

				imshow("result", original_frame);
				waitKey(1);

				printf("6--------------%d\n", clock() - startTime);
				printf("7--------------%d\n", clock() - startTime0);
			}
			g_wait = false;
		}
		g_set_led_mutex.unlock();
	}
}

int main()
{
	initVGA();

	VideoCapture capture;
	do
	{
		capture.open(g_Config.cameraIndex);
		printf("\n-------------opening camera-------------\n");

	} while (!capture.isOpened());

	//capture.set(CAP_PROP_SETTINGS, 1);
	capture.set(CAP_PROP_FPS, 30);
	capture.set(CAP_PROP_FRAME_WIDTH, g_Config.frame.width);
	capture.set(CAP_PROP_FRAME_HEIGHT, g_Config.frame.height);

	//g_capture = capture;
	g_wait = false;
	g_main_thread_exit = false;
	std::thread t1(autoGetCaptureFrame, std::ref(capture));

	// 获取PPID的逻辑放在open camera 之后，让相机先去初始化，调整焦距等
	AgingLog aging(g_Config.ledCount);
	//g_aging = &aging;
	
	if (g_Config.resetRect)
		getSelectROI(capture);

	std::thread t2(findFrameContours, std::ref(aging));
	std::thread t3(renderTrackbarThread);

	u8 *colorNum = new u8[g_Config.ledCount]{ 0 };
	for (u8 i = 1; i < g_Config.ledCount; i++)
	{
		colorNum[i] = i - 1;
	}
	colorNum[0] = g_Config.ledCount - 1;
	srand((unsigned)time(NULL));

	clock_t startTime = clock(), startTime1;

	Mat internal_back;	// 暂存back
	while (g_Config.agingTime > 0)
	{
		g_Config.agingTime--;
		// 关闭所有灯
		resetColor(g_Config.ledCount, 0, 0, 0);

		createPPIDFolder(aging.ppid());

		if (g_main_thread_exit) { break; }
		for (int color = g_Config.startColor; color < g_Config.stopColor; ++color)
		{
			if (g_main_thread_exit) { break; }
			g_Led = color;

			for (size_t index = 0; index < g_Config.ledCount; index++)
			{
				if (g_main_thread_exit) { break; }

				startTime1 = clock();
				setSignleColor(colorNum[index], 0, 0, 0);

				Sleep(g_Config.intervalTime);
				printf("\nget_background_frame--------------\n");
				//g_Index222 = index;
				getFrame(internal_back);

				DebugMode(imshow("background", internal_back));
				DebugMode(waitKey(1));
				{
					char name[128] = { 0 };
					sprintf_s(name, 128, "%s/%s/%02d%02d_back.png", AgingFolder, aging.ppid(), color, index);
					imwrite(name, internal_back);
				}

				int r = rand() % 255;
				printf("\nrandomShutDownLed--------------%d\n", r);
				if (r >= g_Config.randomShutDownLed)
				{
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
				}
				printf("\nsetSignleColor--------------\n");

				Sleep(g_Config.intervalTime);
				g_set_led_mutex.lock();
				g_Index = index;
				g_wait = true;
				getFrame(g_current_frame);
				g_background_frame = internal_back.clone();
				g_randomShutDownLed = (r >= g_Config.randomShutDownLed) ? false : true;
				printf("\nindex = %d, g_Led = %d, time =%d", index, g_Led, clock() - startTime1);
				g_set_led_mutex.unlock();
				Sleep(10); // 让出CPU时间
			}
			// 一个轮回保存一个灯色
			if (1)
			{
				//Sleep(200); // 等工作线程把事情做完
				switch (color)
				{
				case BLUE:
					resetColor(g_Config.ledCount, 0, 0, 255);
					break;
				case GREEN:
					resetColor(g_Config.ledCount, 0, 255, 0);
					break;
				case RED:
					resetColor(g_Config.ledCount, 255, 0, 0);
					break;
				case WHITE:
					resetColor(g_Config.ledCount, 255, 255, 255);
					break;
				}
				printf("resetColor--------------%d\n", __LINE__);

				Sleep(g_Config.intervalTime);

				Mat frame;
				getFrame(frame);	// get current frame
				char name[128] = { 0 };
				sprintf_s(name, 128, "%s/%s/all_color_%02d.png", AgingFolder, aging.ppid(), g_Led);
				putText(frame, aging.thisLedIsOK(color) == Pass ? "PASS" : "FAIL", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 5);
				imwrite(name, frame);

				//sprintf_s(name, 128, "%s_%02d.png", aging.ppid(), g_Led);
				//imshow(name, frame);
				//waitKey(1);
				resetColor(g_Config.ledCount, 0, 0, 0);
				Sleep(g_Config.intervalTime);
			}

		}

		{
			Mat fr = Mat::zeros(g_Config.frame, CV_8UC3);
			putText(fr, aging.allLedIsOK() == Pass ? "PASS" : "FAIL", Point(0, fr.rows), FONT_HERSHEY_SIMPLEX, fr.cols / 100, Scalar(0, 255, 255), 5);
			int sw = GetSystemMetrics(SM_CXSCREEN);
			int sh = GetSystemMetrics(SM_CYSCREEN);
			namedWindow("final_result");
			moveWindow("final_result", sw / 4, sh / 4);
			imshow("final_result", fr);
			waitKey(1);
		}

		aging.flushData();
	}

	g_main_thread_exit = true;
	t1.join();
	t2.join();
	t3.join();
	//t4.join();
	//t5.join();
	//aging.saveAgingLog();

	printf("\nall time ==== %d\n", clock() - startTime);
	delete[] colorNum;
	//waitKey();
	if (g_Config.shutdownTime > 0) 
	{
		char shutdown[128] = { 0 };
		sprintf_s(shutdown, 128, "shutdown -s -t %d", g_Config.shutdownTime);
		system(shutdown);
	}
	
	return 0;
}


int main899874()
{
	int w = 640;
	int h = 480;
	VideoCapture capture(1);
	//capture.set(CAP_PROP_FPS, 30);
	capture.set(CAP_PROP_FRAME_WIDTH, w);
	capture.set(CAP_PROP_FRAME_HEIGHT, h);

	Mat frame ;

	int i = 0;
	int j = 0;
	int k = 0;

	clock_t t = clock();
	while (true)
	{
		capture.read(frame);
		imshow("frame", frame);
		if (waitKey(33) == 0x1b) { break; }// Esc 键
		if ((clock() - t) > 10000)	// 跑10s
			break;
	}

	t = clock();
	while (true)
	{
		clock_t t3 = clock();
		capture.read(frame);
		printf("\ntest--------------%d\n", clock() - t3);

		imshow("frame", frame);

		if (waitKey(33) == 0x1b) { break; }// Esc 键
		
		if (true) {
			clock_t t2 = clock();
			char name[128] = { 0 };
			sprintf_s(name, 128, "aging_rect_image/test/%d_%d.jpg", i, clock());
			printf("\ngetFrame5--------------%d\n", clock() - t2);
			t2 = clock();
			imwrite(name, frame);
			printf("\ngetFrame6--------------%d\n", clock() - t2);
		}

		if ((clock() - t) > 20000)
			break;
	}

	printf("k - %d, i - %d, j - %d\n", k, i, j);
	return 0; 
}
