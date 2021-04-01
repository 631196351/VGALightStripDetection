#if 1
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <vector>
#include <Windows.h>
#include <thread>
#include <mutex>
#include <fstream>

#include "ConfigData.h"
#include "nvbase.h"
#include "utility.h"
#include  "AgingLog.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"

using namespace cv;
using namespace std;

#define DebugMode(oper) if(g_Config.debugMode == true){oper;};
#define IfDebugMode if(g_Config.debugMode == true)
#define MainThreadIsExit if (g_main_thread_exit >= eExit) { break; }
#define OnExitFlagReturn if (g_main_thread_exit >= eExit) { return; }

//Mat g_frame;
Mat g_current_frame;
Mat g_background_frame;
VideoCapture capture;

std::mutex g_get_frame_mutex;
std::mutex g_set_led_mutex;

ConfigData g_Config;

//std::fstream aging_hander;
bool aging_hander_switch = false;

int g_Led = BLUE;
int g_Index = 0;
bool g_wait = false;
int g_main_thread_exit = eNotExit;
int g_randomShutDownLed = 0;
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

void saveDebugROIImg(Mat& f, AgingLog& aging, int currentColor, int currentIndex, const char* lpSuffix)
{
	try 
	{
		char name[MAX_PATH] = { 0 };
		sprintf_s(name, MAX_PATH, "%s/%s/%02d_%02d%02d_%s.png", AgingFolder, aging.targetFolder(), g_recheckFaileLedTime, currentColor, currentIndex, lpSuffix);
		SPDLOG_SINKS_DEBUG("SaveDebugROIImg:{}", name);
		cv::imwrite(name, f);
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

void renderTrackbarThread()
{
	if (!g_Config.showTrackBarWnd)
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

		if (g_Config.thresoldC < t && t % 2 == 0)
		{
			g_Config.thresoldC = t + 1;
		}
		else if (g_Config.thresoldC > t && t % 2 == 0)
		{
			g_Config.thresoldC = t - 1;
		}
		else
		{
			g_Config.thresoldC = t;
		}
	};

	auto func_block_size = [](int pos, void* userdata) -> void {
		if (pos < 3)
			pos = 3;
		if (pos % 2 == 0)
		{
			g_Config.thresoldBlockSize = pos + 1;
		}
		else
		{
			g_Config.thresoldBlockSize = pos;
		}
	};

	func_c(thresoldC, NULL);
	func_block_size(thresoldBlockSize, NULL);

	while (true)
	{
		MainThreadIsExit;

		if (g_Led >= AllColor)// 防止越界
			continue;

		//HsvColor& hsv = g_Config.hsvColor[g_Led];
		//createTrackbar("lowHue", "Toolkit", &hsv.h[5], hsv.h[4]);
		//createTrackbar("higHue", "Toolkit", &hsv.h[6], hsv.h[4]);
		//
		//createTrackbar("lowSat", "Toolkit", &hsv.s[5], hsv.s[4]);
		//createTrackbar("higSat", "Toolkit", &hsv.s[6], hsv.s[4]);
		//
		//createTrackbar("lowVal", "Toolkit", &hsv.v[5], hsv.v[4]);
		//createTrackbar("higVal", "Toolkit", &hsv.v[6], hsv.v[4]);
		//
		//int& thresold = g_Config.bgrColorThres[g_Led];
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

		sprintf_s(buf, 128, "AdaptiveThresholdArgBlockSize = %d", g_Config.thresoldBlockSize);
		putText(empty, buf, Point(0, empty.rows / 4 * 3), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);

		sprintf_s(buf, 128, "AdaptiveThresholdArgC = %d", g_Config.thresoldC);
		putText(empty, buf, Point(0, empty.rows / 4 * 1), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 1);
		imshow("Toolkit", empty);
		empty = Mat::zeros(Size(empty_w, empty_h), CV_8UC3);
		
		waitKey(30);
	}
}

void getFrame(Mat& f, bool cutFrame = true)
{
	try 
	{
		Mat t;
		for (int i = 0; i < g_Config.jumpFrame; i++)
		{
			capture.read(t);
			cv::waitKey(33);
		}
		t.copyTo(f);
		if (cutFrame)
		{
			f = f(g_Config.rect);
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
			Mat camera;
			capture.read(camera);

			sprintf_s(txt, 128, "Power Off: %d", g_Config.shutdownTime);
			cv::putText(camera, txt, Point(0, (camera.rows / 8)), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255), 1);
			rectangle(camera, g_Config.rect, Scalar(0, 255, 255), 5);
			cv::imshow("camera", camera);

			key = cv::waitKey(33);
			if (key == 0x1b)	// Esc 键
			{
				g_main_thread_exit = eExitWithKey;
				SPDLOG_SINKS_DEBUG("AutoGetCaptureFrame eExitWithKey");
			}
			else if (key == 0x30)	// 字符 0
			{
				g_Config.shutdownTime = eNotPowerOff;
				SPDLOG_SINKS_DEBUG("AutoGetCaptureFrame not poweroff");
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

#if 0
void getSelectROI(VideoCapture& capture)
{
	try 
	{
		SPDLOG_SINKS_DEBUG("Select ROI Start");
		capture.set(CAP_PROP_FRAME_WIDTH, g_Config.frame.width);
		capture.set(CAP_PROP_FRAME_HEIGHT, g_Config.frame.height);
		capture.set(CAP_PROP_EXPOSURE, 0);
		capture.set(CAP_PROP_SATURATION, 65);

		Mat roi;
		struct ROIData { Point p; Rect r; bool startROI = false; };

		ROIData data;
		data.r = g_Config.rect;
		SPDLOG_SINKS_DEBUG("Before ROI:({},{}), width:{}, height:{}", data.r.x, data.r.y, data.r.width, data.r.height);

		auto onMouseEvent = [](int event, int x, int y, int flags, void* userdata) -> void
		{
			ROIData* d = ((ROIData*)userdata);
			if (event == EVENT_LBUTTONDOWN)
			{
				d->p = Point(x, y);
				d->startROI = true;
				d->r = Rect();
				SPDLOG_SINKS_DEBUG("EVENT_LBUTTONDOWN:({},{})", d->p.x, d->p.y);

			}
			else if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_LBUTTON)
			{
				if (d->startROI)
				{
					d->r = Rect(d->p, Point(x, y));
					SPDLOG_SINKS_DEBUG("EVENT_MOUSEMOVE:({},{}), width:{}, height:{}", d->r.x, d->r.y, d->r.width, d->r.height);
				}
			}
			else if (event == EVENT_LBUTTONUP)
			{
				d->startROI = false;
				SPDLOG_SINKS_DEBUG("EVENT_LBUTTONUP");

			}
		};

		cv::namedWindow("frame");
		cv::setMouseCallback("frame", onMouseEvent, &data);
		int key = 0;
		while (true)
		{
			capture.read(roi);

			rectangle(roi, data.r, Scalar(0, 255, 255), 5);

			imshow("frame", roi);

			key = cv::waitKey(33);
			if (key == 0x1b)	// Esc 键
			{
				// 放弃重置RIO, 使用旧的ROI设定区域
				SPDLOG_SINKS_DEBUG("Give up reset ROI");
				cv::destroyWindow("frame");

				g_Config.resetRect = false;	// 关闭重置按钮
				g_Config.saveConfigData();
				break;
			}
			else if (key == 0x0d)	// 回车键
			{
				//std::cout << "rectangle :" << data.r << std::endl;
				//g_Config.rect = data.r;
				g_Config.setROIRect(data.r);
				SPDLOG_SINKS_DEBUG("After ROI:({},{}), width:{}, height:{}", g_Config.rect.x, g_Config.rect.y, g_Config.rect.width, g_Config.rect.height);
				g_Config.resetRect = false;
				g_Config.saveConfigData();
				cv::destroyWindow("frame");
				break;
			}

		}

		SPDLOG_SINKS_DEBUG("Select ROI End");
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

void checkContoursColor(Mat frame, Mat mask, Mat result, int currentColor, vector<vector<Point> >& contours, vector<Rect>& boundRect)
{
	try
	{
		SPDLOG_SINKS_DEBUG("MinContoursArea:{}, CurrentColor:{}", g_Config.minContoursArea, currentColor);
		for (int index = 0; index < contours.size(); index++)
		{
			// 生成最小包围矩形
			vector<Point> contours_poly;
			approxPolyDP(Mat(contours[index]), contours_poly, 3, true);
			Rect rect = boundingRect(contours_poly);

			// 轮廓面积校验
			if (rect.area() < g_Config.minContoursArea)
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
				if (b > g_Config.bgrColorThres[BLUE] && b > g && b > r && p > g_Config.bgrColorPercentage[BLUE]) {
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
				if (g > g_Config.bgrColorThres[GREEN] && g > b && g > r && p > g_Config.bgrColorPercentage[GREEN]) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					colorCorrect = true;
				}
			}
			else if (currentColor == RED)
			{
				p = r / (b + g + r);
				if (r > g_Config.bgrColorThres[RED] && r > b && r > g && p > g_Config.bgrColorPercentage[RED]) {
					colorCorrect = true;
				}
				else if ((1.0 - p) < 0.02) {
					colorCorrect = true;
				}
			}

			if (colorCorrect)
			{
				boundRect.push_back(rect);
				// 绘制各自小轮廓
				Scalar color = Scalar(0, 255, 255);
				drawContours(result, contours, index, color, 1);

				SPDLOG_SINKS_DEBUG("Contour {}th ({},{}) width:{} height:{} area:{}; Mean b:{}, g:{}, r:{}, p:{} --> yes", index, rect.x, rect.y, rect.width, rect.height, rect.area(), b, g, r, p);
			}
			else
			{
				SPDLOG_SINKS_DEBUG("Contour {}th ({},{}) width:{} height:{} area:{}; Mean b:{}, g:{}, r:{}, p:{} --> no", index, rect.x, rect.y, rect.width, rect.height, rect.area(), b, g, r, p);
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

void findFrameContours(AgingLog& aging)
{
	while (true)
	{
		MainThreadIsExit;

		g_set_led_mutex.lock();

		if (g_wait)
		{
			try
			{
				SPDLOG_SINKS_DEBUG("****************FindFrameContours****************");
				int currentColor = g_Led;
				int currentIndex = g_Index;
				SPDLOG_SINKS_DEBUG("CurrentColor = {}, CurrentIndex = {}", currentColor, currentIndex);

				if (currentColor == g_Config.startColor && currentIndex == 0)
				{
					SPDLOG_SINKS_DEBUG("Create PPID Folder {}", aging.targetFolder());
					createPPIDFolder(aging.targetFolder());
				}

				Mat original_frame, frame, mask, back;

				original_frame = g_current_frame.clone();
				frame = original_frame.clone();
				back = g_background_frame.clone();

				if (original_frame.empty())
				{
					SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
					throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty");
				}

				DebugMode(cv::imshow("original_frame", frame));
				DebugMode(cv::imshow("background", back));
				{
					saveDebugROIImg(original_frame, aging, currentColor, currentIndex, "original");

					saveDebugROIImg(back, aging, currentColor, currentIndex, "background");
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
				cv::adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, g_Config.thresoldBlockSize, g_Config.thresoldC);
				SPDLOG_SINKS_DEBUG("AdaptiveThreshold BlockSize = {} C = {}", g_Config.thresoldBlockSize, g_Config.thresoldC);
				//GaussianBlur(mask, mask, Size(5, 5), 0);

				//形态学处理
				Mat kernel = getStructuringElement(MORPH_CROSS, Size(3, 3));
				morphologyEx(mask, mask, MORPH_OPEN, kernel);
				SPDLOG_SINKS_DEBUG("Morphology open operation processing mask");

				cv::medianBlur(mask, mask, 3);
				SPDLOG_SINKS_DEBUG("Blur mask");

				DebugMode(cv::imshow("mask", mask));
				{
					saveDebugROIImg(mask, aging, currentColor, currentIndex, "mask");
				}

				//存储边缘
				vector<vector<Point> > contours;
				vector<Rect> boundRect;
				vector<Vec4i> hierarchy;
				Mat result = Mat::zeros(frame.size(), frame.type());
				findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));//查找最顶层轮廓
				SPDLOG_SINKS_DEBUG("Find {} Contours", contours.size());

				checkContoursColor(frame, mask, result, currentColor, contours, boundRect);

				DebugMode(cv::imshow("contours", result));
				{
					saveDebugROIImg(result, aging, currentColor, currentIndex, "contours");
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
					int min_gap = g_Config.minContoursSpace;	//用来记录离自己最近的距离

					for (int j = 0; j < boundRect.size(); j++)
					{
						if (i == j)     // 跳过自己
							continue;
						if (boundRect[j].area() == 0)
							continue;

						int gap = min_distance_of_rectangles(rect, boundRect[j]);

						if (gap <= g_Config.minContoursSpace)
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
					if (r.area() > g_Config.minContoursArea)
					{
						rectangle(original_frame, r, Scalar(0, 255, 255), 3);
						// 第一遍测试结果和复测结果分开
						if (g_recheckFaileLedTime == 0)
							aging.setSingleLedResult(currentIndex, currentColor, Pass);
						else
							aging.setSingleLedRetestResult(currentIndex, currentColor, Pass);

						SPDLOG_SINKS_DEBUG("Contours {}th - x:{} y:{} width:{} height:{} area:{}, RecheckFaileLedTime:{}", index, r.x, r.y, r.width, r.height, r.area(), g_recheckFaileLedTime);

					}
					else
					{
						unqualified_rect++;
					}
				}

				if (unqualified_rect == boundRect.size())
				{
					cv::putText(original_frame, "Failure", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255));
					// 第一遍测试结果和复测结果分开
					if (g_recheckFaileLedTime == 0)
						aging.setSingleLedResult(currentIndex, currentColor, Fail);
					else
						aging.setSingleLedRetestResult(currentIndex, currentColor, Fail);

					SPDLOG_SINKS_ERROR("The {}th {} light failed. RecheckFaileLedTime:{}", currentIndex, currentColor, g_recheckFaileLedTime);
				}

				// 首次侦测且开启随机灭灯情况进入
				if (g_Config.randomShutDownLed > 0 && g_recheckFaileLedTime == 0)
				{
					if (g_randomShutDownLed >= g_Config.randomShutDownLed)
						aging.setSingleLedRandomShutDownResult(currentIndex, currentColor, Pass);
					else
						aging.setSingleLedRandomShutDownResult(currentIndex, currentColor, RandomShutDownLed);
				}

				saveDebugROIImg(original_frame, aging, currentColor, currentIndex, "result");
				cv::imshow("result", original_frame);
				cv::waitKey(1);
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
			g_wait = false;
		}

		g_set_led_mutex.unlock();
		Sleep(1);// 完成工作，等待时，释放CPU时间，避免CPU在此空转
	}
}

void mainLightingControl(AgingLog& aging)
{
	OnExitFlagReturn;
	try
	{
		SPDLOG_SINKS_DEBUG("--------MainLightingControl--------");
		Mat internal_back;	// 暂存back
		RNG rng(time(NULL));
		std::vector<u8> colorNum(g_Config.ledCount);
		for (u8 i = 1; i < g_Config.ledCount; i++)
		{
			colorNum[i] = i - 1;
		}
		colorNum[0] = g_Config.ledCount - 1;
		// 关闭所有灯
		resetColor(g_Config.ledCount, 0, 0, 0);

		for (int color = g_Config.startColor; color < g_Config.stopColor; ++color)
		{
			MainThreadIsExit;
			g_Led = color;

			for (size_t index = 0; index < g_Config.ledCount; index++)
			{
				MainThreadIsExit;

				setSignleColor(colorNum[index], 0, 0, 0);
				SPDLOG_SINKS_DEBUG("Turn of the {}th Led", colorNum[index]);
				//Sleep(g_Config.intervalTime);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);
				getFrame(internal_back);
				SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);

				int r = rng.uniform(0, 255);
				SPDLOG_SINKS_DEBUG("The random number generated is {} ,RandomShutDownLedNum is {}", r, g_Config.randomShutDownLed);
				if (r >= g_Config.randomShutDownLed)
				{
					setSignleColor(index, color);
					SPDLOG_SINKS_DEBUG("Turn on the {}th {} Led", index, color);
				}

				//Sleep(g_Config.intervalTime);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);

				g_set_led_mutex.lock();
				g_Index = index;
				g_wait = true;
				getFrame(g_current_frame);
				SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);
				g_background_frame = internal_back.clone();
				g_randomShutDownLed = r;
				SPDLOG_SINKS_DEBUG("Lit the {}th {} light", index, color);
				g_set_led_mutex.unlock();
				Sleep(10); // 让出CPU时间

				// 让上一轮测试结果显示一会再关闭
				destroyWindow("final_result");
			}

		}

		resetColor(g_Config.ledCount, 0, 0, 0);
		SPDLOG_SINKS_DEBUG("Turn off {} Led", g_Config.ledCount);
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

void checkTheFailLedAgain(AgingLog& aging)
{
	OnExitFlagReturn;
	if (g_Config.recheckFaileLedTime <= 0)
		return;
	try
	{
		SPDLOG_SINKS_DEBUG(">>>>>>>>>>>>>>>>Check the Failed Led Again>>>>>>>>>>>>>>>>");

		//int g_recheckFaileLedTime = g_Config.recheckFaileLedTime;
		Mat internal_back;	// 暂存back
		//RNG rng(time(NULL));
		std::vector<u8> colorNum(g_Config.ledCount);
		for (u8 i = 1; i < g_Config.ledCount; i++)
		{
			colorNum[i] = i - 1;
		}
		colorNum[0] = g_Config.ledCount - 1;
		aging.syncSingLedResult2RetestResult();

		while (g_recheckFaileLedTime < g_Config.recheckFaileLedTime)
		{
			g_set_led_mutex.lock();
			g_recheckFaileLedTime++;
			g_set_led_mutex.unlock();
			SPDLOG_SINKS_DEBUG("Need to retest {} times, now is the {}th time", g_Config.recheckFaileLedTime, g_recheckFaileLedTime);

			for (int color = g_Config.startColor; color < g_Config.stopColor; ++color)
			{
				MainThreadIsExit;
				g_Led = color;

				for (size_t index = 0; index < g_Config.ledCount; index++)
				{
					MainThreadIsExit;

					if (aging.getSingleLedRetestResult(index, color) == Fail)
					{
						// 关闭所有灯
						resetColor(g_Config.ledCount, 0, 0, 0);
						SPDLOG_SINKS_DEBUG("Turn off {} Led", g_Config.ledCount);

						Sleep(g_Config.intervalTime);
						SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);

						getFrame(internal_back);
						SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);
						//int r = rng.uniform(0, 255);
						//if (r >= g_Config.randomShutDownLed)
						{
							setSignleColor(index, color);
							SPDLOG_SINKS_DEBUG("Turn on the {}th {} Led", index, color);
						}

						Sleep(g_Config.intervalTime);
						SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);

						g_set_led_mutex.lock();
						g_Index = index;
						g_wait = true;
						getFrame(g_current_frame);
						SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);

						g_background_frame = internal_back.clone();
						g_randomShutDownLed = 0;
						SPDLOG_SINKS_DEBUG("Lit the {}th {} light", index, color);
						g_set_led_mutex.unlock();
						Sleep(10); // 让出CPU时间

						// 让上一轮测试结果显示一会再关闭
						//destroyWindow("final_result");
					}
				}
			}
		}

		resetColor(g_Config.ledCount, 0, 0, 0);
		SPDLOG_SINKS_DEBUG("Turn off {} Led", g_Config.ledCount);
		// 等最后一颗灯复测完再++, 复测完毕后还原数据，准备下一轮测试
		g_set_led_mutex.lock();
		g_recheckFaileLedTime = 0;
		g_set_led_mutex.unlock();
		SPDLOG_SINKS_DEBUG("Reset RecheckFaileLedTime to {}", g_recheckFaileLedTime);
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

void saveSingleColorResult(AgingLog& aging)
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
			for (int color = g_Config.startColor; color < g_Config.stopColor; ++color)
			{
				MainThreadIsExit;
				// 一个轮回保存一个灯色
				resetColor(g_Config.ledCount, color);
				SPDLOG_SINKS_DEBUG("Turn on all {} led, color {}", g_Config.ledCount, color);
				Sleep(g_Config.intervalTime);
				SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);

				Mat frame;
				getFrame(frame);	// get current frame
				char name[_MAX_PATH] = { 0 };
				sprintf_s(name, _MAX_PATH, "%s/%s/all_color_%02d.png", AgingFolder, aging.targetFolder(), color);
				cv::putText(frame, aging.thisLedIsOK(color) == Pass ? "PASS" : "FAIL", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 2);
				SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);
				imwrite(name, frame);

				resetColor(g_Config.ledCount, 0, 0, 0);
				Sleep(g_Config.intervalTime);
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
}

Rect frameDiff2ROI(const Mat& back, const Mat& fore, int color)
{
	try
	{
		SPDLOG_SINKS_DEBUG("..................frameDiff2ROI.....................");
		Mat b, f, mask;
		Rect roi;
		back.copyTo(b);
		fore.copyTo(f);

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

		SPDLOG_SINKS_DEBUG("Convert back and frame to gray.");
		subtract(frame_gray, back_gray, mask);
		SPDLOG_SINKS_DEBUG("frame_gray - back_gray = mask");

		//cv::threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
		cv::adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, g_Config.thresoldBlockSize, g_Config.thresoldC);
		SPDLOG_SINKS_DEBUG("AdaptiveThreshold BlockSize = {} C = {}", g_Config.thresoldBlockSize, g_Config.thresoldC);
		//GaussianBlur(mask, mask, Size(5, 5), 0);

		//形态学处理
		Mat kernel = getStructuringElement(MORPH_CROSS, Size(3, 3));
		morphologyEx(mask, mask, MORPH_OPEN, kernel);
		SPDLOG_SINKS_DEBUG("Morphology open operation processing mask");

		cv::medianBlur(mask, mask, 3);
		SPDLOG_SINKS_DEBUG("Blur mask");

		//cv::imshow("mask", mask);

		//存储边缘
		vector<vector<Point> > contours;
		vector<Rect> boundRect;
		vector<Vec4i> hierarchy;
		Mat result = Mat::zeros(f.size(), f.type());
		findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE, Point(0, 0));//查找最顶层轮廓
		SPDLOG_SINKS_DEBUG("Find {} Contours", contours.size());

		checkContoursColor(f, mask, result, color, contours, boundRect);

		SPDLOG_SINKS_DEBUG("{} more Rect before the Rect are merged", boundRect.size());
		for (int i = 0; i < boundRect.size(); i++)
		{
			Rect& rect = boundRect[i];
			if (rect.area() == 0)
				continue;

			// 合并轮廓
			// 在已有轮廓中找距离最近的那一个,并进行标记
			int t = -1;
			int min_gap = 5;	//修补因灯带格子而导致的轮廓裂隙

			for (int j = 0; j < boundRect.size(); j++)
			{
				if (i == j)     // 跳过自己
					continue;
				if (boundRect[j].area() == 0)
					continue;

				int gap = min_distance_of_rectangles(rect, boundRect[j]);

				if (gap <= min_gap)
				{
					min_gap = gap;
					t = j;
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
		SPDLOG_SINKS_DEBUG("{} more Rect after the Rect are merged", boundRect.size());

		for (auto it = boundRect.begin(); it != boundRect.end();)
		{
			double w_p = (double)it->width / (double)g_Config.frame.width;
			double h_p = (double)it->height / (double)g_Config.frame.height;
			SPDLOG_SINKS_DEBUG("Rect - x:{} y:{} width:{} height:{} area:{} w_p:{} h_p:{}", it->x, it->y, it->width, it->height, it->area(), w_p, h_p);
			if (w_p > 0.6 || h_p > 0.6)
			{
#ifdef _DEBUG
				rectangle(f, *it, Scalar(0, 0, 255), 1);
#endif
				it++;
			}
			else
			{
				it = boundRect.erase(it);
			}
		}
#ifdef _DEBUG
		cv::imshow("frameDiff2ROI - contours - 1", f);
		waitKey(33);
#endif
		if (boundRect.size() == 1)
		{
			roi = boundRect[0];
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
}

void frameDiff2ROI(const Mat& back, const Mat& fore, int color, const Rect& roi)
{
	try
	{
		SPDLOG_SINKS_DEBUG("..................frameDiff2ROI2.....................");
		Mat b, f, mask;
		back.copyTo(b);
		fore.copyTo(f);

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

		SPDLOG_SINKS_DEBUG("Convert back and frame to gray.");
		subtract(frame_gray, back_gray, mask);
		SPDLOG_SINKS_DEBUG("frame_gray - back_gray = mask");

		//cv::threshold(mask, mask, 0, 255, THRESH_BINARY | THRESH_OTSU);
		cv::adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, g_Config.thresoldBlockSize, g_Config.thresoldC);
		SPDLOG_SINKS_DEBUG("AdaptiveThreshold BlockSize = {} C = {}", g_Config.thresoldBlockSize, g_Config.thresoldC);
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

		checkContoursColor(f, mask, result, color, contours, boundRect);

		SPDLOG_SINKS_DEBUG("{} more Rect before the Rect are merged", boundRect.size());
		for (int i = 0; i < boundRect.size(); i++)
		{
			Rect& rect = boundRect[i];
			if (rect.area() == 0)
				continue;

			// 合并轮廓
			// 在已有轮廓中找距离最近的那一个,并进行标记
			int t = -1;
			int min_gap = g_Config.minContoursSpace;	//用来记录离自己最近的距离

			for (int j = 0; j < boundRect.size(); j++)
			{
				if (i == j)     // 跳过自己
					continue;
				if (boundRect[j].area() == 0)
					continue;

				int gap = min_distance_of_rectangles(rect, boundRect[j]);

				if (gap <= g_Config.minContoursSpace)
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
		SPDLOG_SINKS_DEBUG("{} more Rect after the Rect are merged", boundRect.size());

		for (auto it = boundRect.begin(); it != boundRect.end();)
		{
			if (it->area() == 0) { it = boundRect.erase(it); }
			else {
				*it &= roi;
				SPDLOG_SINKS_DEBUG("Mixed Contours - x:{} y:{} width:{} height:{} area:{}", it->x, it->y, it->width, it->height, it->area());

				if (!it->empty() && it->area() > g_Config.minContoursArea)
				{
					//rectangle(f, *it, Scalar(0, 0, 255), 1);
					it++;
				}
				else
				{
					it = boundRect.erase(it);	//同灯带轮廓不相交or被物体遮挡
				}
			}
		}
		SPDLOG_SINKS_DEBUG("{} more Rect after the Rect are intersected", boundRect.size());

		if (boundRect.size() < 2)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw ErrorCodeEx(ERR_LED_STRIPE_BLOCKED, "The LED light strip may be blocked");
		}

		//cv::imshow("1-2", f);
		//waitKey();
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

void autoCaptureROI()
{
	// 危险区域设定，距离窗口边框 N px 认定为危险区，在此危险区内的首尾灯轮廓皆被认定为超出窗口
	int d = 2;
	int d_tb = 150;	// 上下部分的危险区域设定较大些， 让捕获到的灯带轮廓小一些
	Rect t(0, 0, g_Config.frame.width, d_tb);
	Rect r(g_Config.frame.width - d, 0, d, g_Config.frame.height);
	Rect b(0, g_Config.frame.height - d_tb, g_Config.frame.width, d_tb);
	Rect l(0, 0, d, g_Config.frame.height);
	auto InDangerZone = [=](cv::Rect rect) ->bool {
		
		// 极端情况下tl(), br() 返回的点会刚刚卡在g_Config.frame.width or g_Config.frame.height的边界线上
		// 而这种情况下 contains 是不会认为点在rect内的
		// 退而求其次， 左上角点往外走一步， 右下角点往里走一步，让他们刚好卡在rect内
		Point tl = rect.tl() + Point(1, 1);
		Point br = rect.br() - Point(1, 1);

		if (t.contains(tl) || r.contains(tl) || b.contains(tl) || l.contains(tl))
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			return true;
		}
		if (t.contains(br) || r.contains(br) || b.contains(br) || l.contains(br))
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			return true;
		}
		return false;
	};

	Mat back, fore;
	Rect roi[AllColor - 1];
	int key = 0;
	int exit = eNotExit;
	int correctLoopTime = 0;	// 被算法验证过 N 次没问题就可以认为找到恰当的轮廓了

	while (true)
	{
		if (exit >= eExit)
		{
			break;
		}
		//for (int color = g_Config.startColor; color < g_Config.stopColor; ++color)
		for (int color = g_Config.startColor; color < g_Config.startColor + 1; ++color)
		{
			if (exit >= eExit)
			{
				break;
			}
			try
			{
				SPDLOG_SINKS_DEBUG("Before reset color");
				resetColor(g_Config.ledCount, BLACK);
				SPDLOG_SINKS_DEBUG("After reset color");
				getFrame(back, false);
				SPDLOG_SINKS_DEBUG("Get the back frame");

				resetColor(g_Config.ledCount, color);

				SPDLOG_SINKS_DEBUG("After reset {} color", color);
				//Sleep(g_Config.intervalTime);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);
				getFrame(fore, false);
				SPDLOG_SINKS_DEBUG("Get the foreground frame");
				//cv::imshow("result", fore);
				//waitKey(1);

				roi[color] = frameDiff2ROI(back, fore, color);

				if (roi[color].empty() || InDangerZone(roi[color]))
					throw ErrorCodeEx(ERR_POSTRUE_CORRECTION_ERROR, "Please readjust the camera or graphics card posture");

				Mat back2, fore2;
				SPDLOG_SINKS_DEBUG("Before reset color");
				resetColor(g_Config.ledCount, BLACK);
				SPDLOG_SINKS_DEBUG("After reset color");
				getFrame(back2, false);
				SPDLOG_SINKS_DEBUG("Get the back frame");

				setSignleColor(0, color);
				setSignleColor(g_Config.ledCount - 1, color);

				SPDLOG_SINKS_DEBUG("After reset {} color", color);
				//Sleep(g_Config.intervalTime);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", g_Config.intervalTime);
				getFrame(fore2, false);
				SPDLOG_SINKS_DEBUG("Get the foreground frame");
				frameDiff2ROI(back2, fore2, color, roi[color]);

				resetColor(g_Config.ledCount, BLACK);
				SPDLOG_SINKS_DEBUG("After reset color");

				Rect& r = roi[color];
				rectangle(fore, r, Scalar(255, 0, 255), 2);
				SPDLOG_SINKS_DEBUG("Capture ROI:({},{}), width:{}, height:{}", r.x, r.y, r.width, r.height);
				correctLoopTime++;

			}
			catch (ErrorCode& e)
			{
				correctLoopTime = 0; // 重新计数，重新开始找
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				rectangle(fore, t, Scalar(0, 0, 255), -1);
				rectangle(fore, r, Scalar(0, 0, 255), -1);
				rectangle(fore, b, Scalar(0, 0, 255), -1);
				rectangle(fore, l, Scalar(0, 0, 255), -1);

				char buf[128] = { 0 };
				sprintf_s(buf, 128, "error code %d", e.error());
				SPDLOG_SINKS_ERROR(buf);
				putText(fore, buf, Point(0, (fore.rows / 8)), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255), 1);

				SPDLOG_SINKS_ERROR(e.what());
				putText(fore, e.what(), Point(0, (fore.rows / 8) * 2), FONT_HERSHEY_TRIPLEX, 0.5, Scalar(0, 255, 255), 1);				
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

			cv::imshow("result", fore);
			key = cv::waitKey(33);

			if (/*key == 0x0d ||*/ correctLoopTime >= 1)	// 回车键
			{
				Rect r = roi[BLUE] | roi[BLUE] | roi[RED];
				g_Config.setROIRect(r);
				SPDLOG_SINKS_DEBUG("After ROI:({},{}), width:{}, height:{}", g_Config.rect.x, g_Config.rect.y, g_Config.rect.width, g_Config.rect.height);
				g_Config.resetRect = false;
				g_Config.saveConfigData();
				cv::destroyWindow("result");
				exit = eExit;
			}
			//if (key == 0x1b)	// Esc 键
			//{
			//	// 放弃重置RIO, 使用旧的ROI设定区域
			//	SPDLOG_SINKS_DEBUG("Give up reset ROI");
			//	cv::destroyAllWindows();
			//
			//	g_Config.resetRect = false;	// 关闭重置按钮
			//	g_Config.saveConfigData();
			//	exit = eExit;
			//}
		}
	}
}

int showErrorCode(ErrorCode& e)
{
	SPDLOG_SINKS_ERROR("Catch Error : {}", e.what());
	g_main_thread_exit = eExitWithException;
	g_Config.shutdownTime = eNotPowerOff;
	g_error = e;
	cv::destroyAllWindows();
	SPDLOG_SINKS_DEBUG("g_main_thread_exit = {}, g_Config.shutdownTime = {}", g_main_thread_exit, g_Config.shutdownTime);
	return e.error();
}

//int showErrorCode(std::exception* e, int error)
//{
//	g_main_thread_exit = eExitWithException;
//	g_errorCode = error;
//	cv::destroyAllWindows();
//	int sw = GetSystemMetrics(SM_CXSCREEN);
//	int sh = GetSystemMetrics(SM_CYSCREEN);
//
//	Mat fr = Mat::zeros(cv::Size(sw, sh), CV_8UC3);
//	cv::putText(fr, "FAIL", Point(0, (fr.rows / 8) * 2), FONT_HERSHEY_TRIPLEX, 5, Scalar(0, 255, 255), 5);
//
//	//int len = strlen(e->what()) * sizeof(char) + 100;
//	char err[MAXBYTE] = { 0 };
//	sprintf_s(err, MAXBYTE, "error code %d", error);
//
//	cv::putText(fr, err, Point(0, (fr.rows / 8) * 4), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255));
//	SPDLOG_SINKS_ERROR(err);
//	SPDLOG_SINKS_ERROR(e->what());
//	cv::namedWindow("final_result", WINDOW_NORMAL);
//	cv::imshow("final_result", fr);
//
//	int key = cv::waitKey();
//	SPDLOG_SINKS_DEBUG("the final_result window input key : {}", key);
//	return error;
//}

int showPassorFail(AgingLog& aging)
{
	int sw = GetSystemMetrics(SM_CXSCREEN);
	int sh = GetSystemMetrics(SM_CYSCREEN);
	int wait = 1;	// Pass 是就一闪而过， Fail 时就waitkey(0) 卡住
	Mat fr = Mat::zeros(cv::Size(sw, sh), CV_8UC3);

	if (g_error.error() == ERR_All_IS_WELL)
	{
		if (aging.allLedIsOK() == Pass)
		{
			SPDLOG_SINKS_DEBUG("ALL LED IS OK");
			putText(fr, "PASS", Point(0, (fr.rows / 8) * 2), FONT_HERSHEY_TRIPLEX, 5, Scalar(0, 255, 255), 5);
		}
		else
		{
			g_error = ErrorCode(ERR_SOME_LED_FAILURE, "SOME LED FAILE");
			SPDLOG_SINKS_ERROR(g_error.what());
			cv::putText(fr, "FAIL", Point(0, (fr.rows / 8) * 2), FONT_HERSHEY_TRIPLEX, 5, Scalar(0, 255, 255), 5);

			char err[MAXBYTE] = { 0 };
			sprintf_s(err, MAXBYTE, "error code %d", g_error.error());
			SPDLOG_SINKS_ERROR(err);
			SPDLOG_SINKS_ERROR(g_error.what());
			cv::putText(fr, err, Point(0, (fr.rows / 8) * 4), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255));
			wait = 1;	// 目前先让它通过， 正式后就需要卡住
		}
	}
	else
	{
		cv::putText(fr, "FAIL", Point(0, (fr.rows / 8) * 2), FONT_HERSHEY_TRIPLEX, 5, Scalar(0, 255, 255), 5);

		char err[MAXBYTE] = { 0 };
		sprintf_s(err, MAXBYTE, "error code %d", g_error.error());
		SPDLOG_SINKS_ERROR(err);
		SPDLOG_SINKS_ERROR(g_error.what());

		cv::putText(fr, err, Point(0, (fr.rows / 8) * 4), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255));
		wait = 0;
	}
	
	cv::namedWindow("final_result");
	cv::imshow("final_result", fr);
	cv::waitKey(wait);
	return g_error.error();
}

int main(int argc, void* argv[])
{
	cv::TickMeter tm;
	tm.start();
	try
	{		
		SPDLOG_SINKS_INFO("-------------version 2.0.0.4-------------");

		initVGA();

		// 避免亮光影响相机初始化
		resetColor(g_Config.ledCount, 0, 0, 0);

		capture.open(g_Config.cameraIndex);
		if (!capture.isOpened())
		{
			throw ErrorCodeEx(ERR_CANT_OPEN_CAMERA, "Failed to open camera");
		}

		//capture.set(CAP_PROP_SETTINGS, 1);
		capture.set(CAP_PROP_FPS, 30);
		capture.set(CAP_PROP_FRAME_WIDTH, g_Config.frame.width);
		capture.set(CAP_PROP_FRAME_HEIGHT, g_Config.frame.height);
		capture.set(CAP_PROP_EXPOSURE, g_Config.exposure);
		capture.set(CAP_PROP_SATURATION, g_Config.saturation);
		
		//if (g_Config.resetRect || g_Config.rect.empty())
		autoCaptureROI();
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

	std::thread t1(autoGetCaptureFrame);

	AgingLog aging;
	try 
	{
		// 获取PPID的逻辑放在open camera 之后，让相机先去初始化，调整焦距等
		aging.initAgingLog(g_Config.ledCount, g_Config.randomShutDownLed > 0, g_Config.recheckFaileLedTime > 0);
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

	std::thread t2(findFrameContours, std::ref(aging));
	std::thread t3(renderTrackbarThread);

	try 
	{
		if (g_main_thread_exit == eNotExit)
		{
			SinkInstance.pushBasicFileSinkMT(aging.targetFolder());

			mainLightingControl(aging);

			checkTheFailLedAgain(aging);

			saveSingleColorResult(aging);

			//showPassorFail(aging);

			//aging.flushData();
			//aging.saveAgingLog();
			SinkInstance.popupLastBasicFileSinkMT();
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

	if (g_main_thread_exit == eNotExit)
	{
		g_main_thread_exit = eExit;//任务完成，正常退出
	}

	SPDLOG_SINKS_DEBUG("g_main_thread_exit = {}", g_main_thread_exit);
	SPDLOG_SINKS_DEBUG("wait for thread join before");
	t1.join();
	t2.join();
	t3.join();
	SPDLOG_SINKS_DEBUG("wait for thread join end");

	showPassorFail(aging);

	if (g_Config.shutdownTime >= ePowerOff)
	{
		char shutdown[128] = { 0 };
		sprintf_s(shutdown, 128, "shutdown -s -t %d", g_Config.shutdownTime);
		system(shutdown);
	}
	else if (g_Config.shutdownTime == eReStart)
	{
		char shutdown[128] = { 0 };
		sprintf_s(shutdown, 128, "shutdown -r -t 2");
		system(shutdown);
	}
	else if (g_Config.shutdownTime == eNotPowerOff)
	{
		;
	}
	tm.stop();
	SPDLOG_SINKS_INFO("Tick Time: {}, {}", tm.getTimeSec(), tm.getTimeTicks());
	return g_error.error();
}
#endif


///Opencv——目标跟踪Tracker
///https://blog.csdn.net/qq_43587345/article/details/102833753