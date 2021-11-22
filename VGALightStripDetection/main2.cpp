#if true
#include <opencv2/opencv.hpp>
//#include <stdio.h>
#include <vector>
#include <Windows.h>
#include <thread>
#include <mutex>
//#include <fstream>

#include "ConfigData.h"
//#include "nvbase.h"
#include "utility.h"
#include "AgingLog.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
#include "VideoCard.h"
#include "I2CWrap.h"
#include "Minefield.h"
#include "RandomLitoff.h"

using namespace cv;
using namespace std;

//#define DebugMode(oper) if(cfg.debugMode() == true){oper;};
//#define IfDebugMode if(cfg.debugMode() == true)
#define MainThreadIsExit if (g_main_thread_exit >= eExit) { break; }
#define OnExitFlagReturn if (g_main_thread_exit >= eExit) { return; }
#define DEBUG_DETAILS false
//#define SAVE_ROI_FBMCR	true //是否保存抓取ROI时的foreground, background, mask, contours, result 五幅图

const char* argkeys =
"{help h ?|<none>| Print help message.}"
"{version v|<none>|Print version.}"
"{@ppid p  |<none>| Video card PPID: VGALightStripDetection.exe 210381723300448 'NVIDIA GeForce RTX 3070'}"
"{@name n  |<none>| Video card Name: VGALightStripDetection.exe 210381723300448 'NVIDIA GeForce RTX 3070'}"
"{lit-off lo|| Manually lit off the lights randomly eg: --lo='1,2,3,4,5'}";

//Mat g_frame;
Mat g_current_frame;
Mat g_background_frame;
VideoCapture capture;

//std::mutex g_get_frame_mutex;
std::mutex g_set_led_mutex;

int g_Led = BLUE;
int g_Index = 0;
bool g_wait = false;
bool g_wait_capture = false;
int g_main_thread_exit = eNotExit;
//int g_randomShutDownLed = 0;
int g_recheckFaileLedTime = 0;
ErrorCode g_error = ErrorCode(ERR_All_IS_WELL, "All is well");

int showErrorCode(ErrorCode& e);	// 声明

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

void saveDebugROIImg(Mat& f, int currentColor, int currentIndex, const char* lpSuffix)
{
	try 
	{
		if (cfg.keepDebugImg())
		{
			char name[MAX_PATH] = { 0 };
			sprintf_s(name, MAX_PATH, "%s/%s/%02d_%02d%02d_%s.png", AgingFolder, VideoCardIns.targetFolder(), g_recheckFaileLedTime, currentColor, currentIndex, lpSuffix);
			bool bwrite = cv::imwrite(name, f);
			SPDLOG_SINKS_DEBUG("SaveDebugROIImg:{}, result:{}", name, bwrite);
		}
	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
}

#if false
void renderTrackbarThread()
{
	if (!cfg.showTrackBarWnd)
		return;
	int empty_w = 400, empty_h = 100;
	Mat empty = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
	namedWindow("Toolkit");
	imshow("Toolkit", empty);
	//int hl = 0, sl = 0, vl = 0;
	//int hh = 0, sh = 0, vh = 0;

	//Mat empty2 = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
	//namedWindow("Toolkit_RGB");
	//imshow("Toolkit_RGB", empty2);

	char buf[128] = { 0 };

	int thresoldC = 39;
	int thresoldBlockSize = 101;
	auto func_c = [](int pos, void* userdata) -> void {

		int t = pos - 50;

		if (cfg.thresoldC < t && t % 2 == 0)
		{
			cfg.thresoldC = t + 1;
		}
		else if (cfg.thresoldC > t && t % 2 == 0)
		{
			cfg.thresoldC = t - 1;
		}
		else
		{
			cfg.thresoldC = t;
		}
	};

	auto func_block_size = [](int pos, void* userdata) -> void {
		if (pos < 3)
			pos = 3;
		if (pos % 2 == 0)
		{
			cfg.thresoldBlockSize = pos + 1;
		}
		else
		{
			cfg.thresoldBlockSize = pos;
		}
	};

	func_c(thresoldC, NULL);
	func_block_size(thresoldBlockSize, NULL);

	while (true)
	{
		MainThreadIsExit;

		if (g_Led >= AllColor)// 防止越界
			continue;

		//HsvColor& hsv = cfg.hsvColor[g_Led];
		//createTrackbar("lowHue", "Toolkit", &hsv.h[5], hsv.h[4]);
		//createTrackbar("higHue", "Toolkit", &hsv.h[6], hsv.h[4]);
		//
		//createTrackbar("lowSat", "Toolkit", &hsv.s[5], hsv.s[4]);
		//createTrackbar("higSat", "Toolkit", &hsv.s[6], hsv.s[4]);
		//
		//createTrackbar("lowVal", "Toolkit", &hsv.v[5], hsv.v[4]);
		//createTrackbar("higVal", "Toolkit", &hsv.v[6], hsv.v[4]);
		//
		//int& thresold = cfg.bgrColorThres[g_Led];
		//cv::createTrackbar("thresold", "Toolkit", &thresold, 255);

		cv::createTrackbar("AdaptiveThresholdArgBlockSize", "Toolkit", &thresoldBlockSize, 255, func_block_size);

		cv::createTrackbar("AdaptiveThresholdArgC", "Toolkit", &thresoldC, 100, func_c);
		
		
		//hl = hsv.h[0] + hsv.h[5];
		//sl = hsv.s[0] + hsv.s[5];
		//vl = hsv.v[0] + hsv.v[5];
		//
		//hh = hsv.h[0] + hsv.h[6];
		//sh = hsv.s[0] + hsv.s[6];
		//vh = hsv.v[0] + hsv.v[6];
		//
		//sprintf_s(buf, 128, "lowHSV < higHSV !!!");
		//putText(empty, buf, Point(0, empty.rows / 4 * 1), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);
		//
		//sprintf_s(buf, 128, "Real Low HSV (%d, %d, %d)", hl, sl, vl);
		//putText(empty, buf, Point(0, empty.rows / 4 * 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);
		//
		//sprintf_s(buf, 128, "Real High HSV (%d, %d, %d)", hh, sh, vh);
		//putText(empty, buf, Point(0, empty.rows / 4 * 3), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);

		sprintf_s(buf, 128, "AdaptiveThresholdArgBlockSize = %d", cfg.thresoldBlockSize);
		putText(empty, buf, Point(0, empty.rows / 4 * 3), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);

		sprintf_s(buf, 128, "AdaptiveThresholdArgC = %d", cfg.thresoldC);
		putText(empty, buf, Point(0, empty.rows / 4 * 1), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);
		imshow("Toolkit", empty);
		empty = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
		
		waitKey(30);
	}
}
#endif

void getFrame(Mat& f, bool cutFrame = true)
{
	try 
	{
		Mat t;
		SPDLOG_SINKS_DEBUG("Get Frame");
		for (int i = 0; i < cfg.skipFrame(); i++)
		{
			capture.read(t);
			cv::waitKey(33);
			SPDLOG_SINKS_DEBUG("Get {} Frame", i);
		}
		t.copyTo(f);
		if (cutFrame)
		{
			f = f(cfg.rect());
		}
	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
}

void autoGetCaptureFrame()
{
	int key = 0;
	char txt[128] = { 0 };
	while (true)
	{
		try
		{
			MainThreadIsExit;
			if (g_wait_capture)
			{

				Mat camera;
				capture.read(camera);
				if (camera.empty())
					throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty, check camera usb");

				sprintf_s(txt, 128, "Power Off: %d", cfg.shutdownTime());
				cv::putText(camera, txt, Point(0, (camera.rows / 8)), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255), 1);
				if(!cfg.rect().empty())
					rectangle(camera, cfg.rect(), Scalar(0, 255, 255), 3);
				cv::imshow("camera", camera);

				key = cv::waitKey(33);
				if (key == 0x1b)	// Esc 键
				{
					g_main_thread_exit = eExitWithKey;
					SPDLOG_SINKS_DEBUG("AutoGetCaptureFrame eExitWithKey");
				}
				else if (key == 0x30)	// 字符 0
				{
					cfg.shutdownTime(eNotPowerOff);
					SPDLOG_SINKS_DEBUG("AutoGetCaptureFrame not poweroff");
				}
			}
			else
			{
				// 因为异常机制和多线程有冲突：主线程出现异常，要析构线程对象，会导致报错
				// 开启AP即start 工作线程，但需要等待相机初始化完成才能正式work
				Sleep(1);
			}
		}
		catch (cv::Exception& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			showErrorCode(ErrorCode(e, ERR_OPENCV_RUNTIME_EXCEPTION));
		}
		catch (ErrorCode& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			showErrorCode(e);
		}
		catch (std::exception& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			showErrorCode(ErrorCode(e, ERR_STD_EXCEPTION));
		}
	}
}

#if false
void checkContoursColor(Mat frame, Mat mask, Mat result, int currentColor, vector<vector<Point> >& contours, vector<Rect>& boundRect)
{
	try
	{
		for (int index = 0; index < contours.size(); index++)
		{
			// 生成最小包围矩形
			vector<Point> contours_poly;
			approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
			Rect rect = boundingRect(contours_poly);

			SPDLOG_SINKS_DEBUG("{}th rect.area:{}", index, rect.area());
			// 轮廓面积校验
			if (rect.area() < cfg.minContoursArea())
				continue;
			
			// 校验轮廓颜色
			Mat mask_cell = mask(rect).clone();
			Mat frame_cell = frame(rect).clone();
			//cv::imshow("frame_cell", frame_cell);
			//waitKey();

			// 计算各通道均值
			bool colorCorrect = false;
			Scalar means;
			means = mean(frame_cell, mask_cell);

			double b = means[0];
			double g = means[1];
			double r = means[2];
			double p = 0.0;
			if (currentColor == BLUE)
			{
				p = b / (b + g + r);
				// 亮bule时，b通道要占多数，其他情况一律抹掉该轮廓
				if (b > cfg.bgrThres(BLUE) && b > g && b > r && p > cfg.bgrPercentage(BLUE)) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					// b 通道独占80%，即便是亮度很低的情况下也近乎纯蓝色
					colorCorrect = true;
				}
			}
			else if (currentColor == GREEN)
			{
				p = g / (b + g + r);
				if (g > cfg.bgrThres(GREEN) && g > b && g > r && p > cfg.bgrPercentage(GREEN)) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					colorCorrect = true;
				}
			}
			else if (currentColor == RED)
			{
				p = r / (b + g + r);
				if (r > cfg.bgrThres(RED) && r > b && r > g && p > cfg.bgrPercentage(RED)) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					colorCorrect = true;
				}
			}
			//else if(currentColor == WHITE)
			//{
			//
			//	colorCorrect = true;
			//}

			if (colorCorrect)
			{
				boundRect.push_back(rect);
				// 绘制各自小轮廓
				Scalar color = Scalar(0, 255, 255);
				drawContours(result, contours, index, color, 1);

				SPDLOG_SINKS_DEBUG("CheckContoursColor {}th ({},{}) width:{} height:{} area:{}; Mean b:{}, g:{}, r:{}, p:{} --> yes", index, rect.x, rect.y, rect.width, rect.height, rect.area(), b, g, r, p);
			}
			else
			{
				SPDLOG_SINKS_DEBUG("CheckContoursColor {}th ({},{}) width:{} height:{} area:{}; Mean b:{}, g:{}, r:{}, p:{} --> no", index, rect.x, rect.y, rect.width, rect.height, rect.area(), b, g, r, p);
				rect = Rect();
			}
		}
	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
}
#endif

void findFrameContours()
{
#if false
	while (true)
	{
		MainThreadIsExit;

		g_set_led_mutex.lock();

		if (g_wait)
		{
			try
			{
				cv::TickMeter t;
				t.start();
				SPDLOG_SINKS_DEBUG("****************FindFrameContours****************");
				int currentColor = g_Led;
				int currentIndex = g_Index;
				SPDLOG_SINKS_DEBUG("CurrentColor = {}, CurrentIndex = {}", currentColor, currentIndex);

				Mat original_frame, frame, mask, back;

				original_frame = g_current_frame.clone();
				frame = original_frame.clone();
				back = g_background_frame.clone();

				if (original_frame.empty())
				{
					SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
					throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty");
				}

#if DEBUG_DETAILS
				cv::imshow("original_frame", frame);
				cv::imshow("background", back);
#endif
				{
					saveDebugROIImg(original_frame, currentColor, currentIndex, "original");

					saveDebugROIImg(back, currentColor, currentIndex, "background");
				}
				SPDLOG_SINKS_DEBUG("Frame difference algorithm starts.");
				std::vector<Mat> frame_bgrs, back_bgrs;
				Mat frame_gray, back_gray, temp;
				if (currentColor == WHITE) {
					cv::cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
					cv::cvtColor(back, back_gray, COLOR_BGR2GRAY);
				}
				else
				{
					split(frame, frame_bgrs);
					split(back, back_bgrs);
					frame_gray = frame_bgrs[currentColor];
					back_gray = back_bgrs[currentColor];
				}

				SPDLOG_SINKS_DEBUG("Convert back and frame to gray.");
				subtract(frame_gray, back_gray, mask);
				SPDLOG_SINKS_DEBUG("frame_gray - back_gray = mask");

				//cv::threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
				cv::adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, cfg.thresoldBlockSize(), cfg.thresoldC());
				//GaussianBlur(mask, mask, Size(5, 5), 0);

				//形态学处理
				Mat kernel = getStructuringElement(MORPH_CROSS/*MORPH_RECT*/, Size(3, 3));
				morphologyEx(mask, mask, MORPH_OPEN, kernel);
				SPDLOG_SINKS_DEBUG("Morphology open operation processing mask");

				cv::medianBlur(mask, mask, 3);
				SPDLOG_SINKS_DEBUG("Blur mask");

#if DEBUG_DETAILS
				cv::imshow("mask", mask);
#endif
				{
					saveDebugROIImg(mask, currentColor, currentIndex, "mask");
				}

				//存储边缘
				vector<vector<Point> > contours;
				vector<Rect> boundRect;
				vector<Vec4i> hierarchy;
				Mat result = Mat::zeros(frame.size(), frame.type());
				findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));//查找最顶层轮廓
				SPDLOG_SINKS_DEBUG("Find {} Contours", contours.size());

				checkContoursColor(frame, mask, result, currentColor, contours, boundRect);
#if DEBUG_DETAILS
				cv::imshow("contours", result);
#endif
				{
					saveDebugROIImg(result, currentColor, currentIndex, "contours");
				}

				SPDLOG_SINKS_DEBUG("{} more Rect before the Rect are merged", boundRect.size());
				for (int i = 0; i < boundRect.size(); i++)
				{
					Rect& rect = boundRect[i];
					if (rect.area() == 0)
						continue;

					//printf("\ncontours1----------------[x:%d, y:%d, w:%d, h:%d]\n", rect.x, rect.y, rect.width, rect.height);
					// 合并轮廓
					// 在已有轮廓中找距离最近的那一个,并进行标记
					int t = -1;
					int min_gap = cfg.minContoursSpace();	//用来记录离自己最近的距离

					for (int j = 0; j < boundRect.size(); j++)
					{
						if (i == j)     // 跳过自己
							continue;
						if (boundRect[j].area() == 0)
							continue;

						int gap = min_distance_of_rectangles(rect, boundRect[j]);

						if (gap <= cfg.minContoursSpace())
						{
							if (gap <= min_gap)
							{
								min_gap = gap;
								t = j;
							}
						}
					}

					// 同距离自己最近的轮廓进行合并， 都离的远就自成一家
					if (t >= 0)
					{
						Rect r = boundRect[t];
						boundRect[t] |= rect;
						rect = Rect();
					}
				}


				for (auto it = boundRect.begin(); it != boundRect.end();)
				{
					if (it->area() == 0) { it = boundRect.erase(it); }
					else { it++; }
				}

				SPDLOG_SINKS_DEBUG("{} more Rect after the Rect are merged", boundRect.size());
				//得到灯的轮廓
				size_t unqualified_rect = 0;
				for (int index = 0; index < boundRect.size(); index++)
				{
					const Rect& r = boundRect[index];
					if (r.area() == 0)
						continue;

					// 合并轮廓时会将被合并轮廓抹掉
					if (r.area() > cfg.ledContoursArea())
					{
						rectangle(original_frame, r, Scalar(0, 255, 255), 3);
						// 第一遍测试结果和复测结果分开
						if (g_recheckFaileLedTime == 0)
							AgingInstance.setSingleLedResult(currentIndex, currentColor, Pass);
						else
							AgingInstance.setSingleLedRetestResult(currentIndex, currentColor, Pass);

						SPDLOG_SINKS_DEBUG("Contours {}th - x:{} y:{} width:{} height:{} area:{}, RecheckFaileLedTime:{}", index, r.x, r.y, r.width, r.height, r.area(), g_recheckFaileLedTime);

					}
					else
					{
						unqualified_rect++;
					}
				}

				if (unqualified_rect == boundRect.size())
				{
					cv::putText(original_frame, "Fail", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255));
					// 第一遍测试结果和复测结果分开
					if (g_recheckFaileLedTime == 0)
						AgingInstance.setSingleLedResult(currentIndex, currentColor, Fail);
					else
						AgingInstance.setSingleLedRetestResult(currentIndex, currentColor, Fail);

					SPDLOG_SINKS_ERROR("The {}th {} light failed. RecheckFaileLedTime:{}", currentIndex, currentColor, g_recheckFaileLedTime);
				}

				// 首次侦测且开启随机灭灯情况进入
				if (/*cfg.randomLitOffProbability() > 0*/litoff.getRandomLitOffState() && g_recheckFaileLedTime == 0)
				{
					//if (g_randomShutDownLed >= cfg.randomLitOffProbability())
					if (!litoff.IsLitOff(currentIndex))
						AgingInstance.setSingleLedRandomShutDownResult(currentIndex, currentColor, Pass);
					else
						AgingInstance.setSingleLedRandomShutDownResult(currentIndex, currentColor, RandomShutDownLed);
				}

				saveDebugROIImg(original_frame, currentColor, currentIndex, "result");
				cv::imshow("result", original_frame);
				cv::waitKey(1);

				t.stop();
				SPDLOG_SINKS_INFO("Tick Time: {}, {}", t.getTimeSec(), t.getTimeTicks());
			}
			catch (cv::Exception& e)
			{
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				showErrorCode(ErrorCode(e, ERR_OPENCV_RUNTIME_EXCEPTION));
			}
			catch (ErrorCode& e)
			{
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				showErrorCode(e);
			}
			catch (std::exception& e)
			{
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				showErrorCode(ErrorCode(e, ERR_STD_EXCEPTION));
			}
			g_wait = false;	// !important

		}

		g_set_led_mutex.unlock();
		Sleep(1);// 完成工作，等待时，释放CPU时间，避免CPU在此空转
	}
#else
	while (true)
	{
		MainThreadIsExit;

		g_set_led_mutex.lock();

		if (g_wait)
		{
			try
			{
				cv::TickMeter t;
				t.start();
				SPDLOG_SINKS_DEBUG("****************FindFrameContours****************");
				int currentColor = g_Led;
				int currentIndex = g_Index;
				SPDLOG_SINKS_DEBUG("CurrentColor = {}, CurrentIndex = {}", currentColor, currentIndex);

				Mat frame, back, mask, mask_hsv;
				Mat hsv_img_mask;
				frame = g_current_frame.clone();
				back = g_background_frame.clone();

				if (frame.empty() || back.empty())
				{
					SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
					throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty");
				}

				{
#if DEBUG_DETAILS
					cv::imshow("original_frame", frame);
					cv::imshow("background", back);
#endif
					saveDebugROIImg(frame, currentColor, currentIndex, "fore");
					saveDebugROIImg(back, currentColor, currentIndex, "back");
				}

				cv::subtract(frame, back, mask);
				
				const float* hsv = cfg.hsvColor(currentColor);

				cvtColor(mask, mask_hsv, COLOR_BGR2HSV);

				inRange(mask_hsv, Scalar(hsv[eHmin], hsv[eSmin], hsv[eVmin]), Scalar(hsv[eHmax], 255, 255), hsv_img_mask);

				if (currentColor == RED)
				{
					Mat hsv_img_mask_r;

					inRange(mask_hsv, Scalar(hsv[eHmin2], hsv[eSmin], hsv[eVmin]), Scalar(hsv[eHmax2], 255, 255), hsv_img_mask_r);
					hsv_img_mask += hsv_img_mask_r;
				}

				cv::adaptiveThreshold(hsv_img_mask, hsv_img_mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 101, -9);

				cv::medianBlur(hsv_img_mask, hsv_img_mask, 3);

				GaussianBlur(hsv_img_mask, hsv_img_mask, cv::Size(3, 3), 0);

				{
#if DEBUG_DETAILS
					cv::imshow("mask", mask);
					imshow("mask_hsv", mask_hsv);
#endif
					saveDebugROIImg(hsv_img_mask, currentColor, currentIndex, "mask");
				}

				//存储边缘
				vector<vector<Point> > contours;
				vector<Rect> boundRect;
				vector<Vec4i> hierarchy;
				Mat result = Mat::zeros(frame.size(), frame.type());
				cv::findContours(hsv_img_mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));//查找最顶层轮廓
				SPDLOG_SINKS_DEBUG("Find {} Contours", contours.size());

				Rect rect;
				for (int i = 0; i < contours.size(); ++i)
				{
					// 生成最小包围矩形
					vector<Point> contours_poly;
					approxPolyDP(Mat(contours[i]), contours_poly, 3, true);
					Rect r = boundingRect(contours_poly);

					SPDLOG_SINKS_DEBUG("{}th rect.area:{}", i, r.area());
					// 轮廓面积校验
					if (r.area() < cfg.minContoursArea())
						continue;

					rect |= r;

					drawContours(result, contours, i, Scalar(0, 255, 255), 1);
				}
				
				{
					saveDebugROIImg(result, currentColor, currentIndex, "contours");
				}
				//得到灯的轮廓
				if (rect.area() > cfg.ledContoursArea())
				{
					rectangle(frame, rect, Scalar(0, 255, 255), 1);
					// 第一遍测试结果和复测结果分开
					if (g_recheckFaileLedTime == 0)
						AgingInstance.setSingleLedResult(currentIndex, currentColor, Pass);
					else
						AgingInstance.setSingleLedRetestResult(currentIndex, currentColor, Pass);

					SPDLOG_SINKS_DEBUG("Contours x:{} y:{} width:{} height:{} area:{}, RecheckFaileLedTime:{} --> Pass", rect.x, rect.y, rect.width, rect.height, rect.area(), g_recheckFaileLedTime);
				}
				else
				{
					cv::putText(frame, "Fail", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255));
					// 第一遍测试结果和复测结果分开
					if (g_recheckFaileLedTime == 0)
						AgingInstance.setSingleLedResult(currentIndex, currentColor, Fail);
					else
						AgingInstance.setSingleLedRetestResult(currentIndex, currentColor, Fail);

					SPDLOG_SINKS_DEBUG("Contours x:{} y:{} width:{} height:{} area:{}, RecheckFaileLedTime:{} --> Fail", rect.x, rect.y, rect.width, rect.height, rect.area(), g_recheckFaileLedTime);
				}

				// 首次侦测且开启随机灭灯情况进入
				if (litoff.getRandomLitOffState() && g_recheckFaileLedTime == 0)
				{
					if (!litoff.IsLitOff(currentIndex))
						AgingInstance.setSingleLedRandomShutDownResult(currentIndex, currentColor, Pass);
					else
						AgingInstance.setSingleLedRandomShutDownResult(currentIndex, currentColor, RandomShutDownLed);
				}

				saveDebugROIImg(frame, currentColor, currentIndex, "result");
				cv::imshow("result", frame);
				cv::waitKey(1);

				t.stop();
				SPDLOG_SINKS_INFO("Tick Time: {}, {}", t.getTimeSec(), t.getTimeTicks());
			}
			catch (cv::Exception& e)
			{
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				showErrorCode(ErrorCode(e, ERR_OPENCV_RUNTIME_EXCEPTION));
			}
			catch (ErrorCode& e)
			{
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				showErrorCode(e);
			}
			catch (std::exception& e)
			{
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				showErrorCode(ErrorCode(e, ERR_STD_EXCEPTION));
			}
			g_wait = false;	// !important

		}

		g_set_led_mutex.unlock();
		Sleep(1);// 完成工作，等待时，释放CPU时间，避免CPU在此空转
	}
#endif
}

void mainLightingControl()
{
	OnExitFlagReturn;
	try
	{
		SPDLOG_SINKS_DEBUG("---------------- MainLightingControl ----------------");
		cv::TickMeter t;
		t.start();
		Mat internal_back;	// 暂存back
		RNG rng(time(NULL));
		std::vector<int> colorNum(I2C.getLedCount());
		for (int i = 1; i < I2C.getLedCount(); i++)
		{
			colorNum[i] = i - 1;
		}
		colorNum[0] = I2C.getLedCount() - 1;
		// 关闭所有灯
		I2C.resetColor(0, 0, 0);

		for (int color = cfg.c1(); color < cfg.c2(); ++color)
		{
			MainThreadIsExit;

			for (int index = 0; index < I2C.getLedCount(); index++)
			{
				MainThreadIsExit;

				I2C.setSignleColor(colorNum[index], 0, 0, 0);
				SPDLOG_SINKS_DEBUG("Turn off the {}th Led", colorNum[index]);
				Sleep(cfg.intervalTime());
				SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);
				getFrame(internal_back);

				//int r = rng.uniform(0, 101);	//[0, 101)
				//SPDLOG_SINKS_DEBUG("The random number generated is {} ,RandomShutDownLedNum is {}", r, cfg.randomLitOffProbability());
				//if (r >= cfg.randomLitOffProbability())
				//if(AgingInstance.getThisLedLitOffState(index))
				{
					litoff.IsLitOff(index) ? (void)0 : I2C.setSignleColor(index, color);
					SPDLOG_SINKS_DEBUG("Turn on the {}th {} Led", index, color);
					Sleep(cfg.intervalTime());
				}

				//Sleep(cfg.intervalTime);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", cfg.intervalTime);

				g_set_led_mutex.lock();
				g_Index = index;
				g_Led = color;
				g_wait = true;
				getFrame(g_current_frame);
				SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);
				g_background_frame = internal_back.clone();
				//g_randomShutDownLed = r;
				SPDLOG_SINKS_DEBUG("Lit the {}th {} light", index, color);
				g_set_led_mutex.unlock();
				Sleep(10); // 让出CPU时间

				// 让上一轮测试结果显示一会再关闭
				//destroyWindow("final_result");
			}

		}

		I2C.resetColor(0, 0, 0);
		SPDLOG_SINKS_DEBUG("Turn off {} Led", I2C.getLedCount());

		// 等待工作线程完成后继续
		g_set_led_mutex.lock();
		t.stop();
		g_set_led_mutex.unlock();
		SPDLOG_SINKS_INFO("----------------MainLightingControl---------------- Tick Time: {}, {}", t.getTimeSec(), t.getTimeTicks());

	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
}

void checkTheFailLedAgain()
{
	OnExitFlagReturn;
	if (cfg.recheckFaileLedTime() <= 0)
		return;
	try
	{
		SPDLOG_SINKS_DEBUG("---------------- Check the Failed Led Again 1 ----------------");

		//int g_recheckFaileLedTime = cfg.recheckFaileLedTime;
		Mat internal_back;	// 暂存back
		//RNG rng(time(NULL));
		//std::vector<int> colorNum(I2C.getLedCount());
		//for (int i = 1; i < I2C.getLedCount(); i++)
		//{
		//	colorNum[i] = i - 1;
		//}
		//colorNum[0] = I2C.getLedCount() - 1;
		AgingInstance.syncSingLedResult2RetestResult();

		while (g_recheckFaileLedTime < cfg.recheckFaileLedTime())
		{
			g_set_led_mutex.lock();
			g_recheckFaileLedTime++;
			g_set_led_mutex.unlock();
			SPDLOG_SINKS_DEBUG("Need to retest {} times, now is the {}th time", cfg.recheckFaileLedTime(), g_recheckFaileLedTime);

			for (int color = cfg.c1(); color < cfg.c2(); ++color)
			{
				MainThreadIsExit;

				for (int index = 0; index < I2C.getLedCount(); index++)
				{
					MainThreadIsExit;

					if (AgingInstance.getSingleLedRetestResult(index, color) == Fail)
					{
						// 关闭所有灯
						I2C.resetColor(0, 0, 0);
						SPDLOG_SINKS_DEBUG("Turn off {} Led", I2C.getLedCount());

						Sleep(cfg.intervalTime());
						SPDLOG_SINKS_DEBUG("Sleep {} millisecond", cfg.intervalTime());

						getFrame(internal_back);
						SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);
						
						I2C.setSignleColor(index, color);
						SPDLOG_SINKS_DEBUG("Turn on the {}th {} Led", index, color);

						Sleep(cfg.intervalTime());
						SPDLOG_SINKS_DEBUG("Sleep {} millisecond", cfg.intervalTime());

						g_set_led_mutex.lock();
						g_Index = index;
						g_Led = color;
						g_wait = true;
						getFrame(g_current_frame);
						SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);

						g_background_frame = internal_back.clone();
						//g_randomShutDownLed = 0;
						SPDLOG_SINKS_DEBUG("Lit the {}th {} light", index, color);
						g_set_led_mutex.unlock();
						Sleep(10); // 让出CPU时间

						// 让上一轮测试结果显示一会再关闭
						//destroyWindow("final_result");
					}
				}
			}
		}

		I2C.resetColor(0, 0, 0);
		SPDLOG_SINKS_DEBUG("Turn off {} Led", I2C.getLedCount());
		// 等最后一颗灯复测完再++, 复测完毕后还原数据，准备下一轮测试
		g_set_led_mutex.lock();
		g_recheckFaileLedTime = 0;
		g_set_led_mutex.unlock();
		SPDLOG_SINKS_DEBUG("Reset RecheckFaileLedTime to {}", g_recheckFaileLedTime);
		SPDLOG_SINKS_DEBUG("---------------- Check the Failed Led Again 2 ----------------");
	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
}

void saveSingleColorResult()
{
	OnExitFlagReturn;

#ifdef _DEBUG	
	if (false)
#else
	if (true)
#endif
	{
		try 
		{
			SPDLOG_SINKS_DEBUG("---------------- save single led color 1 -----------------");

			for (int color = cfg.c1(); color < cfg.c2(); ++color)
			{
				MainThreadIsExit;
				// 一个轮回保存一个灯色
				I2C.resetColor(color);
				SPDLOG_SINKS_DEBUG("Turn on all {} led, color {}", I2C.getLedCount(), color);
				Sleep(cfg.intervalTime());
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", cfg.intervalTime());

				Mat frame;
				getFrame(frame);	// get current frame
				char name[_MAX_PATH] = { 0 };
				sprintf_s(name, _MAX_PATH, "%s/%s/all_color_%02d.png", AgingFolder, VideoCardIns.targetFolder(), color);
				cv::putText(frame, AgingInstance.thisLedIsOK(color) == Pass ? "PASS" : "FAIL", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 2);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", cfg.intervalTime());
				cv::imwrite(name, frame);

				//I2C.resetColor(0, 0, 0);
				//Sleep(cfg.intervalTime());
			}
			I2C.resetColor(0, 0, 0);	//确保结束后灯是灭的
			SPDLOG_SINKS_DEBUG("---------------- save single led color 2 -----------------");

		}
		catch (cv::Exception& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw e;
		}
		catch (ErrorCode& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw e;
		}
		catch (std::exception& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw e;
		}
	}
}

#if false
void checkROIContoursColor(Mat frame, Mat mask, Mat result, int currentColor, vector<vector<Point> >& contours, vector<Rect>& boundRect)
{
	try
	{
		for (int index = 0; index < contours.size(); index++)
		{
			// 生成最小包围矩形
			vector<Point> contours_poly;
			approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
			Rect rect = boundingRect(contours_poly);

			SPDLOG_SINKS_DEBUG("{}th rect.area:{}", index, rect.area());
			// 轮廓面积校验
			if (rect.area() < cfg.minROIContoursArea())
				continue;

			// 校验轮廓颜色
			Mat mask_cell = mask(rect).clone();
			Mat frame_cell = frame(rect).clone();
			//cv::imshow("frame_cell", frame_cell);
			//waitKey();

			// 计算各通道均值
			bool colorCorrect = false;
			Scalar means;
			means = mean(frame_cell, mask_cell);

			double b = means[0];
			double g = means[1];
			double r = means[2];
			double p = 0.0;
			if (currentColor == BLUE)
			{
				p = b / (b + g + r);
				// 亮bule时，b通道要占多数，其他情况一律抹掉该轮廓
				if (b > cfg.bgrThres(BLUE) && b > g && b > r && p > cfg.bgrPercentage(BLUE)) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					// b 通道独占80%，即便是亮度很低的情况下也近乎纯蓝色
					colorCorrect = true;
				}
			}
			else if (currentColor == GREEN)
			{
				p = g / (b + g + r);
				if (g > cfg.bgrThres(GREEN) && g > b && g > r && p > cfg.bgrPercentage(GREEN)) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					colorCorrect = true;
				}
			}
			else if (currentColor == RED)
			{
				p = r / (b + g + r);
				if (r > cfg.bgrThres(RED) && r > b && r > g && p > cfg.bgrPercentage(RED)) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					colorCorrect = true;
				}
			}
			//else if(currentColor == WHITE)
			//{
			//
			//	colorCorrect = true;
			//}

			if (colorCorrect)
			{
				boundRect.push_back(rect);
				// 绘制各自小轮廓
				Scalar color = Scalar(0, 255, 255);
				drawContours(result, contours, index, color, 1);

				SPDLOG_SINKS_DEBUG("CheckContoursColor {}th ({},{}) width:{} height:{} area:{}; Mean b:{}, g:{}, r:{}, p:{} --> yes", index, rect.x, rect.y, rect.width, rect.height, rect.area(), b, g, r, p);
			}
			else
			{
				SPDLOG_SINKS_DEBUG("CheckContoursColor {}th ({},{}) width:{} height:{} area:{}; Mean b:{}, g:{}, r:{}, p:{} --> no", index, rect.x, rect.y, rect.width, rect.height, rect.area(), b, g, r, p);
				rect = Rect();
			}
		}
	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
}
#endif

Rect frameDiff2ROI(const Mat& back, const Mat& fore, int color)
{
#if false
	try
	{
		SPDLOG_SINKS_DEBUG("..................frameDiff2ROI-1.....................");
		Mat b, f, mask;
		Rect roi;
		b = back;// back.copyTo(b);
		f = fore;// fore.copyTo(f);

		std::vector<Mat> frame_bgrs, back_bgrs;
		Mat frame_gray, back_gray;
		if (color == WHITE) {
			cv::cvtColor(f, frame_gray, COLOR_BGR2GRAY);
			cv::cvtColor(b, back_gray, COLOR_BGR2GRAY);
		}
		else
		{
			split(f, frame_bgrs);
			split(b, back_bgrs);
			frame_gray = frame_bgrs[color];
			back_gray = back_bgrs[color];
		}

		char name[MAX_PATH] = { 0 };
#ifdef SAVE_ROI_FBMCR
		{
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_fore.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, f);

			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_back.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, b);
		}
#endif
		SPDLOG_SINKS_DEBUG("Convert back and frame to gray.");
		subtract(frame_gray, back_gray, mask);
		threshold(mask, mask, 100, 255, THRESH_TOZERO);
		SPDLOG_SINKS_DEBUG("frame_gray - back_gray = mask");

#ifdef SAVE_ROI_FBMCR
		{
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_mask.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, mask);
		}
#endif
		//cv::threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
		cv::adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, cfg.thresoldBlockSize(), cfg.thresoldC());
		//SPDLOG_SINKS_DEBUG("AdaptiveThreshold BlockSize = {} C = {}", cfg.thresoldBlockSize(), cfg.thresoldC());
		//GaussianBlur(mask, mask, Size(5, 5), 0);

		//形态学处理
		Mat kernel = getStructuringElement(MORPH_CROSS, Size(3, 3));
		morphologyEx(mask, mask, MORPH_OPEN, kernel);
		SPDLOG_SINKS_DEBUG("Morphology open operation processing mask");

		cv::medianBlur(mask, mask, 3);
		SPDLOG_SINKS_DEBUG("Blur mask");

		//存储边缘
		vector<vector<Point> > contours;
		vector<Rect> boundRect;
		vector<Vec4i> hierarchy;
		Mat result = Mat::zeros(f.size(), f.type());
		findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));//查找最顶层轮廓
		SPDLOG_SINKS_DEBUG("Find {} Contours", contours.size());

		checkROIContoursColor(f, mask, result, color, contours, boundRect);

#ifdef SAVE_ROI_FBMCR
		{
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_contours.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, result);
		}
#endif

		std::sort(boundRect.begin(), boundRect.end(), [](cv::Rect& l, cv::Rect& r) { return l.area() > r.area(); });
		for (int i = 0; i < boundRect.size(); ++i)
		{
			roi |= boundRect[i];
		}
		//SPDLOG_SINKS_DEBUG("{} more Rect before the Rect are merged", boundRect.size());
		//for (int i = 0; i < boundRect.size(); i++)
		//{
		//	Rect& rect = boundRect[i];
		//	if (rect.area() == 0)
		//		continue;
		//
		//	// 合并轮廓
		//	// 在已有轮廓中找距离最近的那一个,并进行标记
		//	int t = -1;
		//	//int min_gap = 5;	//修补因灯带格子而导致的轮廓裂隙
		//	int min_gap = cfg.minContoursSpace();
		//
		//	for (int j = 0; j < boundRect.size(); j++)
		//	{
		//		if (i == j)     // 跳过自己
		//			continue;
		//		if (boundRect[j].area() == 0)
		//			continue;
		//
		//		int gap = min_distance_of_rectangles(rect, boundRect[j]);
		//
		//		if (gap <= min_gap)
		//		{
		//			min_gap = gap;
		//			t = j;
		//		}
		//	}
		//
		//	// 同距离自己最近的轮廓进行合并， 都离的远就自成一家
		//	if (t >= 0)
		//	{
		//		Rect r = boundRect[t];
		//		boundRect[t] |= rect;
		//		rect = Rect();
		//	}
		//}
		//SPDLOG_SINKS_DEBUG("{} more Rect after the Rect are merged", boundRect.size());



#ifdef SAVE_ROI_FBMCR
		//for (auto r : boundRect)
		//{
		//	if (!r.empty())
		//	{
		//		rectangle(f, r, Scalar(255, 0, 255), 1);
		//	}
		//}

		rectangle(f, roi, Scalar(255, 0, 255), 1);
		sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_result.png", AgingFolder, VideoCardIns.targetFolder(), color);
		cv::imwrite(name, f);

#endif
		//if (boundRect.size() > 0)
		//{
		//	roi = boundRect[0];
		//}
		//
		return roi;
	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
#else
	try
	{
		SPDLOG_SINKS_DEBUG("..................frameDiff2ROI-1.....................");
		Mat b, f, mask, mask_hsv, hsv_img_mask;
		Rect roi;
		b = back;// back.copyTo(b);
		f = fore;// fore.copyTo(f);

		char name[MAX_PATH] = { 0 };
		if (cfg.keepDebugImg())
		{
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_fore.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, f);

			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_back.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, b);
		}
		SPDLOG_SINKS_DEBUG("Frame difference algorithm starts.");
		cv::subtract(f, b, mask);

		const float* hsv = cfg.hsvColor(color);
		const float* roi_hv = cfg.roiHV();

		cvtColor(mask, mask_hsv, COLOR_BGR2HSV);

		inRange(mask_hsv, Scalar(hsv[eHmin], roi_hv[0], roi_hv[1]), Scalar(hsv[eHmax], 255, 255), hsv_img_mask);

		if (color == RED)
		{
			Mat hsv_img_mask_r;

			inRange(mask_hsv, Scalar(hsv[eHmin2], roi_hv[0], roi_hv[1]), Scalar(hsv[eHmax2], 255, 255), hsv_img_mask_r);
			hsv_img_mask += hsv_img_mask_r;
		}

		cv::adaptiveThreshold(hsv_img_mask, hsv_img_mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 101, -9);

		cv::medianBlur(hsv_img_mask, hsv_img_mask, 3);

		GaussianBlur(hsv_img_mask, hsv_img_mask, cv::Size(3, 3), 0);

		if (cfg.keepDebugImg())
		{
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_mask.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, hsv_img_mask);
		}
		//存储边缘
		vector<vector<Point> > contours;
		vector<Rect> boundRect;
		vector<Vec4i> hierarchy;
		Mat result = Mat::zeros(f.size(), f.type());
		cv::findContours(hsv_img_mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));//查找最顶层轮廓
		SPDLOG_SINKS_DEBUG("Find {} Contours", contours.size());

		for (int i = 0; i < contours.size(); ++i)
		{
			// 生成最小包围矩形
			vector<Point> contours_poly;
			approxPolyDP(Mat(contours[i]), contours_poly, 3, true);
			Rect r = boundingRect(contours_poly);

			// 轮廓面积校验
			if (r.area() < cfg.minContoursArea())
				continue;
			SPDLOG_SINKS_DEBUG("{}th rect.area:{}", i, r.area());

			roi |= r;

			drawContours(result, contours, i, Scalar(0, 255, 255), 1);
		}

		if (cfg.keepDebugImg())
		{
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_contours.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, result);

			rectangle(f, roi, Scalar(255, 0, 255), 1);
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_result.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, f);
		}

		return roi;
	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
#endif
}


void autoCaptureROI2()
{
	// B-G-R三色来圈取灯带ROI
	// 可能存在的问题是，若其中一个颜色(如 Green) 只抓到了一半的ROI， 进行合并后
	// Rect 相交 取最小区域，最后结果就只有半个ROI
	Mat back, fore;
	Rect roi[BGR];
	while (true)
	{
		MainThreadIsExit;
		try
		{
			for (int color = BLUE; color < WHITE; ++color)
			{
				I2C.resetColor(BLACK);
				Sleep(cfg.intervalTime());
				getFrame(back, false);

				SPDLOG_SINKS_DEBUG("lit-on {}th color", color);
				I2C.resetColor(color);
				Sleep(cfg.intervalTime());
				getFrame(fore, false);

				roi[color] = frameDiff2ROI(back, fore, color);

				if (roi[color].empty()) 
				{
					SPDLOG_SINKS_ERROR("{}th color roi empty", color);
					throw ErrorCodeEx(ERR_POSTRUE_CORRECTION_ERROR, "Please readjust the camera or graphics card posture");
				}
			}

			cv::imshow("result", fore);
			cv::waitKey(33);

			Rect r = (roi[BLUE] | roi[GREEN]) & roi[RED];
			cfg.rect(r);
			SPDLOG_SINKS_DEBUG("After ROI:({},{}), width:{}, height:{}, area:{}", cfg.rect().x, cfg.rect().y, cfg.rect().width, cfg.rect().height, cfg.rect().area());
			cv::destroyWindow("result");
			break;

		}
		catch (ErrorCode& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			
			char buf[128] = { 0 };
			sprintf_s(buf, 128, "error code %d", e.error());
			SPDLOG_SINKS_WARN(buf);
			putText(fore, buf, Point(0, (fore.rows / 8)), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255), 1);
			
			SPDLOG_SINKS_WARN(e.what());
			putText(fore, e.what(), Point(0, (fore.rows / 8) * 2), FONT_HERSHEY_TRIPLEX, 0.5, Scalar(0, 255, 255), 1);
			if(e.error() == ERR_RUN_I2C_FAILURE)
				throw e;
		}
		catch (cv::Exception& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw e;
		}
		catch (std::exception& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw e;
		}
			
	}
}

int showErrorCode(ErrorCode& e)
{
	SPDLOG_SINKS_ERROR("Catch Error : {}", e.what());
	g_main_thread_exit = eExitWithException;
	cfg.shutdownTime(eNotPowerOff);
	g_error = e;
	cv::destroyAllWindows();
	SPDLOG_SINKS_DEBUG("g_main_thread_exit = {}, cfg.shutdownTime = {}", g_main_thread_exit, cfg.shutdownTime());
	return e.error();
}

void getFinalResult()
{
	// 走到这里没有突发异常
	if (g_error.error() == ERR_All_IS_WELL)
	{
		// 未发生异常
		if (AgingInstance.allLedIsOK() == Pass)
		{
			// Release 模式下测试时，Pass 不保留图片
			if (!cfg.keepDebugImg())
			{
				char delete_cmd[MAX_PATH * 2] = { 0 };
				sprintf_s(delete_cmd, MAX_PATH * 2, "del /S /Q %s\\%s\\%s\\*.png>nul", get_current_directory().c_str(), AgingFolder, VideoCardIns.targetFolder());
				system(delete_cmd);
				SPDLOG_SINKS_INFO("cmd : {}", delete_cmd);
			}
		}
		else
		{
			// 但是发现有灯不良
			g_error = ErrorCodeEx(ERR_SOME_LED_FAILURE, "some led fail");
		}
	}
}

void showPassorFail()
{
	auto pass_msg = []()
	{
		SPDLOG_SINKS_INFO("");
		SPDLOG_SINKS_INFO("");
		SPDLOG_SINKS_INFO("PPPPPPPP          AAA            SSSSS            SSSSS");
		SPDLOG_SINKS_INFO("PPPPPPPPPP       AAAAA         SSSSSSSSSS       SSSSSSSSSS");
		SPDLOG_SINKS_INFO("PPP     PPP     AAA AAA       SSS     SSSS     SSS     SSSS");
		SPDLOG_SINKS_INFO("PPP     PPP    AAA   AAA      SSS              SSS");
		SPDLOG_SINKS_INFO("PPPPPPPPPP    AAA     AAA      SSS              SSS");
		SPDLOG_SINKS_INFO("PPPPPPPP      AAA     AAA        SSSS             SSSS");
		SPDLOG_SINKS_INFO("PPP           AAA     AAA          SSSS             SSSS");
		SPDLOG_SINKS_INFO("PPP           AAAAAAAAAAA            SSSS             SSSS");
		SPDLOG_SINKS_INFO("PPP           AAAAAAAAAAA              SSS              SSS");
		SPDLOG_SINKS_INFO("PPP           AAA     AAA     SSSS     SSS     SSSS     SSS");
		SPDLOG_SINKS_INFO("PPP           AAA     AAA      SSSSSSSSSS       SSSSSSSSSS");
		SPDLOG_SINKS_INFO("PPP           AAA     AAA         SSSSS            SSSSS");
		SPDLOG_SINKS_INFO("");
		SPDLOG_SINKS_INFO("");
	};

	auto fail_msg = []()
	{
		SPDLOG_SINKS_ERROR("");
		SPDLOG_SINKS_ERROR("");
		SPDLOG_SINKS_ERROR("FFFFFFFFF         AAA         IIIIIIIII     LLL");
		SPDLOG_SINKS_ERROR("FFFFFFFFF        AAAAA        IIIIIIIII     LLL");
		SPDLOG_SINKS_ERROR("FFF             AAA AAA          III        LLL");
		SPDLOG_SINKS_ERROR("FFF            AAA   AAA         III        LLL");
		SPDLOG_SINKS_ERROR("FFF           AAA     AAA        III        LLL");
		SPDLOG_SINKS_ERROR("FFFFFFFF      AAA     AAA        III        LLL");
		SPDLOG_SINKS_ERROR("FFFFFFFF      AAA     AAA        III        LLL");
		SPDLOG_SINKS_ERROR("FFF           AAAAAAAAAAA        III        LLL");
		SPDLOG_SINKS_ERROR("FFF           AAAAAAAAAAA        III        LLL");
		SPDLOG_SINKS_ERROR("FFF           AAA     AAA        III        LLL");
		SPDLOG_SINKS_ERROR("FFF           AAA     AAA     IIIIIIIII     LLLLLLLLLLL");
		SPDLOG_SINKS_ERROR("FFF           AAA     AAA     IIIIIIIII     LLLLLLLLLLL");
		SPDLOG_SINKS_ERROR("");
		SPDLOG_SINKS_ERROR("");
		SPDLOG_SINKS_ERROR("{} - {}", g_error.error(), g_error.what());
		AgingInstance.serialize();

		if (litoff.getRandomLitOffState())
		{
			// 要是随即灭灯开了，就说明处于测试阶段，直接过
		}
		else
		{
			// 没有开启随机灭灯，但出现了fail，直接卡住
			system("pause");
		}
	};

	if (g_error.error() == ERR_All_IS_WELL)
	{
		pass_msg();
	}
	else
	{
		fail_msg();
	}
}

int main(int argc, char* argv[])
{
	//屏蔽控制台关闭按钮
	HWND hwnd = GetConsoleWindow();
	HMENU hmenu = GetSystemMenu(hwnd, false);
	RemoveMenu(hmenu, SC_CLOSE, MF_BYCOMMAND);
	SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	ShowWindow(hwnd, SW_SHOWNORMAL);
	DestroyMenu(hmenu);
	ReleaseDC(hwnd, NULL);

	cv::CommandLineParser parser(argc, argv, argkeys);
	if (parser.has("help"))
	{
		parser.printMessage();
		return ERR_All_IS_WELL;
	}
	else if(parser.has("version"))
	{
		std::cout << "Version " << VersionMajor << "." << VersionSec << "." << VersionThi << "." << VersionMin << std::endl;
		return ERR_All_IS_WELL;
	}

	cv::TickMeter tm;
	tm.start();
	litoff;

	std::thread t1(autoGetCaptureFrame);
	std::thread t2(findFrameContours);
	//std::thread t3(renderTrackbarThread);
	try
	{
		// 程式开启时打开csv, 准备随时接受异常报错
		AgingInstance.openAgingCsv();

		if (parser.has("@ppid") && parser.has("@name"))
		{
			VideoCardIns.PPID(parser.get<std::string>("@ppid"));
			VideoCardIns.Name(parser.get<std::string>("@name"));

			SinkInstance.addPPID2FileSinkMT(VideoCardIns.targetFolder());

		}
		else
		{
			throw ErrorCodeEx(ERR_INCOMPLETE_ARGS, "Incomplete required parameters(ppid or model name)");
		}

		SPDLOG_SINKS_INFO("-------------version {}.{}.{}.{}-------------", VersionMajor, VersionSec, VersionThi, VersionMin);
		SPDLOG_SINKS_INFO("Model Name:{}", VideoCardIns.Name());
		SPDLOG_SINKS_INFO("PPID:{}", VideoCardIns.PPID());

		// 避免亮光影响相机初始化
		I2C.resetColor(0, 0, 0);

		cfg.readConfigFile(VideoCardIns.Name(), I2C.getLedCount());

		litoff.setRandomLitOffState(cfg.randomLitOffProbability(), parser.get<std::string>("lo"));

		capture.open(cfg.cameraIndex());
		if (!capture.isOpened())
		{
			throw ErrorCodeEx(ERR_CANT_OPEN_CAMERA, "Failed to open camera");
		}
		else
		{
			//capture.set(CAP_PROP_SETTINGS, 1);
			capture.set(CAP_PROP_FPS, 30);
			capture.set(CAP_PROP_FRAME_WIDTH, cfg.frame().width);
			capture.set(CAP_PROP_FRAME_HEIGHT, cfg.frame().height);
			capture.set(CAP_PROP_EXPOSURE, cfg.exposure());
			capture.set(CAP_PROP_SATURATION, cfg.saturation());

			capture.set(CAP_PROP_FOURCC, MAKEFOURCC('M', 'J', 'P', 'G'));

			g_wait_capture = true;	//自动拍摄线程开始工作
		}

		// 开始工作前分配内存
		AgingInstance.initAgingLog();

		autoCaptureROI2();

		mainLightingControl();

		checkTheFailLedAgain();

		//saveSingleColorResult();

	}
	catch (cv::Exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		showErrorCode(ErrorCode(e, ERR_OPENCV_RUNTIME_EXCEPTION));
	}
	catch (ErrorCode& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		showErrorCode(e);
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		showErrorCode(ErrorCode(e, ERR_STD_EXCEPTION));
	}

	if (g_main_thread_exit == eNotExit)
	{
		g_main_thread_exit = eExit;//任务完成，正常退出
	}

	SPDLOG_SINKS_DEBUG("g_main_thread_exit = {}", g_main_thread_exit);
	SPDLOG_SINKS_DEBUG("wait for thread join before");
	t1.join();
	t2.join();
	//t3.join();
	SPDLOG_SINKS_DEBUG("wait for thread join end");

	// 先整理出来一个最终的结果, 好保证可以将errorcode正确写入aging.csv
	getFinalResult();

	// 优先保证测试日志可以写入
	AgingInstance.saveAgingLog(g_error.error());

	showPassorFail();

	tm.stop();
	SPDLOG_SINKS_INFO("Tick Time: {}, {}", tm.getTimeSec(), tm.getTimeTicks());

	if (cfg.shutdownTime() >= ePowerOff)
	{
		char shutdown[128] = { 0 };
		sprintf_s(shutdown, 128, "shutdown -s -t %d", cfg.shutdownTime());
		system(shutdown);
	}
	else if (cfg.shutdownTime() == eReStart)
	{
		char shutdown[128] = { 0 };
		sprintf_s(shutdown, 128, "shutdown -r -t 2");
		system(shutdown);
	}
	else if (cfg.shutdownTime() == eNotPowerOff)
	{
		;
	}
	
	return g_error.error();
}
#endif


///Opencv——目标跟踪Tracker
///https://blog.csdn.net/qq_43587345/article/details/102833753
///
///Color models and color spaces
///https://programmingdesignsystems.com/color/color-models-and-color-spaces/
///
///HSV(Hue, Saturation and Value)
///https://www.tech-faq.com/hsv.html
///
///http://colorizer.org/
///http://color.lukas-stratmann.com/color-systems/hsv.html
///
///Why do we use the HSV colour space so often in vision and image processing ?
///https://dsp.stackexchange.com/questions/2687/why-do-we-use-the-hsv-colour-space-so-often-in-vision-and-image-processing
///
///The HSV color space
///https://www.blackice.com/Help/Tools/Image%20OCX%20webhelp/WebHelp/The_HSV_color_space.htm
///
///https://math.hws.edu/graphicsbook/demos/c2/rgb-hsv.html
///
///Color models and color spaces
///https://programmingdesignsystems.com/color/color-models-and-color-spaces/