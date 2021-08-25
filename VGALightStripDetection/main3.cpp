#define LIGHTSTRIPV3
#ifdef LIGHTSTRIPV3
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>

#include "ConfigData.h"
#include "AgingLog.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
#include "VideoCard.h"
#include "I2CWrap.h"
//#include "Minefield.h"
#include "RandomLitoff.h"

using namespace cv;
using namespace std;

const char* argkeys =
"{help h ?|<none>| Print help message.}"
"{version v|<none>|Print version.}"
"{@ppid p  |<none>| Video card PPID: VGALightStripDetection.exe 210381723300448 'NVIDIA GeForce RTX 3070'}"
"{@name n  |<none>| Video card Name: VGALightStripDetection.exe 210381723300448 'NVIDIA GeForce RTX 3070'}"
"{lit-off lo|| Manually lit off the lights randomly eg: --lo='1,2,3,4,5'}";

#define MainThreadIsExit if (g_main_thread_exit >= eExit) { break; }
#define OnExitFlagReturn if (g_main_thread_exit >= eExit) { return; }

//#define DEBUG_DETAILS

std::mutex g_set_led_mutex;
int g_Led = BLUE;
int g_Index = 0;
bool g_wait = false;
bool g_wait_capture = false;
int g_main_thread_exit = eNotExit;
int g_recheckFaileLedTime = 0;
int showErrorCode(ErrorCode& e);	// 声明
ErrorCode g_error = ErrorCode(ERR_All_IS_WELL, "All is well");
std::vector<VideoCapture> g_captures(CaptureNum);
std::vector<Mat> g_fore(CaptureNum);
std::vector<Mat> g_back(CaptureNum);

void spliceMultipleFrames(std::vector<Mat>& frames, Mat& result)
{
	int w = g_captures[0].get(CAP_PROP_FRAME_WIDTH);
	int h = g_captures[0].get(CAP_PROP_FRAME_HEIGHT);
	result = Mat(Size(w, h* CaptureNum), CV_8UC3, Scalar::all(0));
	for (int i = 0; i < frames.size(); ++i)
	{
		Mat roi = result(Rect(0, i*h, w, h));
		frames[i].copyTo(roi);
	}
}

void getFrame( std::vector<Mat>& f, bool cutFrame = true)
{
	try
	{
		SPDLOG_SINKS_DEBUG("Get Frame");
		for (int i = 0; i < cfg.skipFrame(); ++i)
		{
			for (int j = 0; j < CaptureNum; ++j)
			{
				Mat img;
				g_captures[j].read(img);
				SPDLOG_SINKS_DEBUG("Get Frame {}, {}", i, j);
				if (img.empty())
					throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty, check camera usb");
				if (i + 1 >= cfg.skipFrame())
				{
					if (cutFrame)
					{
						const Rect* rois = cfg.rois();
						img(rois[j]).copyTo(f[j]);
					}
					else
					{
						img.copyTo(f[j]);
					}
				}
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
				int w = g_captures[0].get(CAP_PROP_FRAME_WIDTH);
				int h = g_captures[0].get(CAP_PROP_FRAME_HEIGHT);

				Mat video = Mat(Size(w, h* CaptureNum), CV_8UC3, Scalar::all(0));
				for (int i = 0; i < CaptureNum; ++i)
				{
					Mat img = Mat(Size(w, h), CV_8UC3, Scalar::all(0));
					g_captures[i].read(img);
					if (img.empty())
						throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty, check camera usb");
					Mat roi = video(Rect(0, i*h, w, h));
					img.copyTo(roi);
				}

				sprintf_s(txt, 128, "Power Off: %d", cfg.shutdownTime());
				putText(video, txt, Point(0, (video.rows / 8)), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255), 1);
				//if (!cfg.rect().empty())
				//	rectangle(video, cfg.rect(), Scalar(0, 255, 255), 5);

				imshow("video", video);

				key = waitKey(33);
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
		catch (Exception& e)
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

void saveDebugROIImg(Mat& f, int currentColor, int currentIndex, const char* lpSuffix)
{
	try
	{
		char name[MAX_PATH] = { 0 };
		sprintf_s(name, MAX_PATH, "%s/%s/%02d_%02d%02d_%s.png", AgingFolder, VideoCardIns.targetFolder(), g_recheckFaileLedTime, currentColor, currentIndex, lpSuffix);
		bool bwrite = cv::imwrite(name, f);
		SPDLOG_SINKS_DEBUG("SaveDebugROIImg:{}, result:{}", name, bwrite);
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

void findFrameContours()
{
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
				bool isPass = false;
				int currentColor = g_Led;
				int currentIndex = g_Index;
				SPDLOG_SINKS_DEBUG("Color = {}, Index = {}", currentColor, currentIndex);

				Mat mask;
				// 多视角画面拼接
				Mat frame = Mat::zeros(cfg.rect2().size(), CV_8UC3);
				Mat back = Mat::zeros(cfg.rect2().size(), CV_8UC3);
				for (int x = 0; x < CaptureNum; ++x)
				{
					if (x == 0)
					{
						g_fore[x].copyTo(frame(Rect(0, 0, g_fore[x].cols, g_fore[x].rows)));
						g_back[x].copyTo(back(Rect(0, 0, g_back[x].cols, g_back[x].rows)));
					}
					else
					{
						g_fore[x].copyTo(frame(Rect(0, g_fore[x - 1].rows, g_fore[x].cols, g_fore[x].rows)));
						g_back[x].copyTo(back(Rect(0, g_back[x - 1].rows, g_back[x].cols, g_back[x].rows)));
					}
				}


				if (frame.empty() || back.empty())
				{
					SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
					throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty");
				}

#ifdef DEBUG_DETAILS
				cv::imshow("frame", frame);
				cv::imshow("back", back);
#endif
				{
					saveDebugROIImg(frame, currentColor, currentIndex, "fore");

					saveDebugROIImg(back, currentColor, currentIndex, "back");
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

#ifdef DEBUG_DETAILS
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
#ifdef DEBUG_DETAILS
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
					SPDLOG_SINKS_DEBUG("Rect {}th - x:{} y:{} width:{} height:{} area:{}, RecheckFaileLedTime:{}", index, r.x, r.y, r.width, r.height, r.area(), g_recheckFaileLedTime);
					if (r.area() > cfg.ledContoursArea())
					{
						rectangle(frame, r, Scalar(0, 255, 255), 3);
						// 第一遍测试结果和复测结果分开
						if (g_recheckFaileLedTime == 0)
							AgingInstance.setSingleLedResult(currentIndex, currentColor, Pass);
						else
							AgingInstance.setSingleLedRetestResult(currentIndex, currentColor, Pass);
					}
					else
					{
						unqualified_rect++;
					}
				}

				if (unqualified_rect == boundRect.size())
				{
					cv::putText(frame, "Fail", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255));
					// 第一遍测试结果和复测结果分开
					if (g_recheckFaileLedTime == 0)
						AgingInstance.setSingleLedResult(currentIndex, currentColor, Fail);
					else
						AgingInstance.setSingleLedRetestResult(currentIndex, currentColor, Fail);

					SPDLOG_SINKS_ERROR("The {}th {} light FAIL. RecheckFaileLedTime:{}", currentIndex, currentColor, g_recheckFaileLedTime);
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
}

void mainLightingControl()
{
	OnExitFlagReturn;
	try
	{
		SPDLOG_SINKS_DEBUG("---------------- MainLightingControl ----------------");
		cv::TickMeter t;
		t.start();
		std::vector<Mat> back(CaptureNum);
		std::vector<Mat> fore(CaptureNum);
		std::vector<int> colorNum(I2C.getLedCount());
		for (int i = 1; i < I2C.getLedCount(); i++)
		{
			colorNum[i] = i - 1;
		}
		colorNum[0] = I2C.getLedCount() - 1;
		// 关闭所有灯
		I2C.resetColor(0, 0, 0);

		auto deepCopyMat = [](std::vector<Mat>& l, std::vector<Mat>& r)
		{
			if (l.size() != CaptureNum)
			{
				SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
				throw ErrorCodeEx(ERR_MISSING_FRAME, "抓到的图像数小于相机个数");
			}
			//r.clear();
			r.swap(l);
		};
		for (int color = cfg.c1(); color < cfg.c2(); ++color)
		{
			MainThreadIsExit;

			for (int index = 0; index < I2C.getLedCount(); index++)
			{
				MainThreadIsExit;

				I2C.setSignleColor(colorNum[index], BLACK);
				SPDLOG_SINKS_DEBUG("Turn off the {}th Led", colorNum[index]);
				Sleep(cfg.intervalTime());
				SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);
				getFrame(back);

				litoff.IsLitOff(index) ? (void)0 : I2C.setSignleColor(index, color);
				SPDLOG_SINKS_DEBUG("Turn on the {}th {} Led", index, color);
				Sleep(cfg.intervalTime());
				SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);
				getFrame(fore);

				g_set_led_mutex.lock();
				g_Index = index;
				g_Led = color;
				g_wait = true;
				deepCopyMat(back, g_back);
				deepCopyMat(fore, g_fore);
				g_set_led_mutex.unlock();
				SPDLOG_SINKS_DEBUG("Lit the {}th {} light", index, color);
				Sleep(10); // 让出CPU时间

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

void frameDiff2ROI(const std::vector<Mat>& back, const std::vector<Mat>& fore, int color, Rect outRect[][CaptureNum])
{
	try
	{
		for (int x = 0; x < CaptureNum; ++x)
		{
			Mat b, f, mask;
			Rect roi;
			b = back[x];// back.copyTo(b);
			f = fore[x];// fore.copyTo(f);

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
				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_fore.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
				cv::imwrite(name, f);

				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_back.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
				cv::imwrite(name, b);
			}
#endif
			SPDLOG_SINKS_DEBUG("Convert back and frame to gray.");
			subtract(frame_gray, back_gray, mask);
			threshold(mask, mask, 100, 255, THRESH_TOZERO);
			SPDLOG_SINKS_DEBUG("frame_gray - back_gray = mask");

#ifdef SAVE_ROI_FBMCR
			{
				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_mask.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
				cv::imwrite(name, mask);
			}
#endif
			cv::adaptiveThreshold(mask, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, cfg.thresoldBlockSize(), cfg.thresoldC());

			//形态学处理
			Mat kernel = getStructuringElement(MORPH_CROSS, Size(3, 3));
			morphologyEx(mask, mask, MORPH_OPEN, kernel);
			cv::medianBlur(mask, mask, 3);

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
				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_contours.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
				cv::imwrite(name, result);
			}
#endif

			std::sort(boundRect.begin(), boundRect.end(), [](cv::Rect& l, cv::Rect& r) { return l.area() > r.area(); });
			for (int i = 0; i < boundRect.size(); ++i)
			{
				roi |= boundRect[i];
			}

			rectangle(f, roi, Scalar(255, 0, 255), 1);
#ifdef SAVE_ROI_FBMCR
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_result.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
			cv::imwrite(name, f);
#endif
			SPDLOG_SINKS_DEBUG("{} color {}th ROI x:{},y:{}, width:{}, height:{}", color, x, roi.x, roi.y, roi.width, roi.height);
			outRect[color][x] = roi;
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

void autoCaptureROI2()
{
	// B-G-R三色来圈取灯带ROI
	// 可能存在的问题是，若其中一个颜色(如 Green) 只抓到了一半的ROI， 进行合并后
	// Rect 相交 取最小区域，最后结果就只有半个ROI
	std::vector<Mat> back(CaptureNum), fore(CaptureNum);

	Rect roi[BGR][CaptureNum];

	auto checkROI = [&](Rect* roi) -> bool
	{
		int notEmpty = 0;
		for (int i = 0; i < CaptureNum; ++i)
			if (!roi[i].empty())
				++notEmpty;
		return notEmpty > 0;
	};

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

				frameDiff2ROI(back, fore, color, roi);

				if (!checkROI(roi[color]))
				{
					SPDLOG_SINKS_ERROR("{}th color roi empty", color);
					throw ErrorCodeEx(ERR_POSTRUE_CORRECTION_ERROR, "Please readjust the camera or graphics card posture");
				}

				Mat frame;
				char name[MAX_PATH] = { 0 };
				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d.png", AgingFolder, VideoCardIns.targetFolder(), color);
				spliceMultipleFrames(fore, frame);
				cv::imwrite(name, frame);
				cv::imshow("result", frame);
				cv::waitKey(33);
			}

			cfg.rect(roi, BGR);
			cv::destroyWindow("result");
			break;
		}
		catch (ErrorCode& e)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
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


void checkTheFailLedAgain()
{
	OnExitFlagReturn;
	if (cfg.recheckFaileLedTime() <= 0)
		return;
	auto deepCopyMat = [](std::vector<Mat>& l, std::vector<Mat>& r)
	{
		if (l.size() != CaptureNum)
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw ErrorCodeEx(ERR_MISSING_FRAME, "抓到的图像数小于相机个数");
		}
		//r.clear();
		r.swap(l);
	};
	try
	{
		SPDLOG_SINKS_DEBUG("---------------- Check the Failed Led Again 1 ----------------");
		std::vector<Mat> back(CaptureNum);
		std::vector<Mat> fore(CaptureNum);

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
						Sleep(cfg.intervalTime());
						getFrame(back);
						SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);

						I2C.setSignleColor(index, color);
						Sleep(cfg.intervalTime());
						getFrame(fore);
						SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);

						g_set_led_mutex.lock();
						g_Index = index;
						g_Led = color;
						g_wait = true;
						deepCopyMat(back, g_back);
						deepCopyMat(fore, g_fore);

						SPDLOG_SINKS_DEBUG("Lit the {}th {} light", index, color);
						g_set_led_mutex.unlock();
						Sleep(10); // 让出CPU时间

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

int showPassorFail()
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
		// 未发生异常
		if (AgingInstance.allLedIsOK() == Pass)
		{
			pass_msg();
		}
		else
		{
			g_error = ErrorCodeEx(ERR_SOME_LED_FAILURE, "some led fail");
			fail_msg();
		}
	}
	else
	{
		// 发生了异常
		fail_msg();
	}

	return g_error.error();
}

int main(int argc, char* argv[])
{

	cv::CommandLineParser parser(argc, argv, argkeys);
	if (parser.has("help"))
	{
		parser.printMessage();
		return ERR_All_IS_WELL;
	}
	else if (parser.has("version"))
	{
		std::cout << "Version " << VersionMajor << "." << VersionSec << "." << VersionThi << "." << VersionMin << std::endl;
		return ERR_All_IS_WELL;
	}

	cv::TickMeter tm;
	tm.start();
	litoff;

	std::thread t1(autoGetCaptureFrame);
	std::thread t2(findFrameContours);
	
	try
	{
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

		for (int i = 0; i < CaptureNum; i++)
		{
			if (!g_captures[i].open(i))
			{
				SPDLOG_SINKS_ERROR("Failed to open {}th camera", i + 1);
				throw ErrorCodeEx(ERR_CANT_OPEN_CAMERA, "Failed to open camera");
			}
			else
			{
				g_captures[i].set(CAP_PROP_FPS, 30);
				g_captures[i].set(CAP_PROP_FRAME_WIDTH, cfg.frame().width);
				g_captures[i].set(CAP_PROP_FRAME_HEIGHT, cfg.frame().height);
				g_captures[i].set(CAP_PROP_EXPOSURE, cfg.exposure());
				//g_captures[i].set(CAP_PROP_SATURATION, cfg.saturation());
				g_captures[i].set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));

				int w = g_captures[0].get(CAP_PROP_FRAME_WIDTH);
				int h = g_captures[0].get(CAP_PROP_FRAME_HEIGHT);
				SPDLOG_SINKS_INFO("g_captures[{}] w:{}, h:{}", i, w, h);
			}
		}
		g_wait_capture = true;	//自动拍摄线程开始工作

		autoCaptureROI2();

		// 获取Video的逻辑放在open camera 之后，让相机先去初始化，调整焦距等
		AgingInstance.initAgingLog(I2C.getLedCount(), litoff.getRandomLitOffState(), cfg.recheckFaileLedTime() > 0);

		{
			mainLightingControl();
			checkTheFailLedAgain();
			//saveSingleColorResult();
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
	SPDLOG_SINKS_DEBUG("wait for thread join end");

	// 优先保证测试日志可以写入
	AgingInstance.saveAgingLog();

	showPassorFail();

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
	tm.stop();
	SPDLOG_SINKS_INFO("Tick Time: {}, {}", tm.getTimeSec(), tm.getTimeTicks());
	return g_error.error();
}
#endif // LIGHTSTRIPV3
