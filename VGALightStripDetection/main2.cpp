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

#define MainThreadIsExit if (g_main_thread_exit >= eExit) { break; }
#define OnExitFlagReturn if (g_main_thread_exit >= eExit) { return; }
#define DEBUG_DETAILS false

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

int g_wait_time = 33;	//waitKey(g_wait_time)
int g_Led = BLUE;
int g_Index = 0;
bool g_wait = false;
bool g_wait_capture = false;
int g_main_thread_exit = eNotExit;
//int g_randomShutDownLed = 0;
int g_recheckFaileLedTime = 0;
ErrorCode g_error = ErrorCode(ERR_All_IS_WELL, "All is well");

int showErrorCode(ErrorCode& e);	// 声明

void saveDebugROIImg(Mat& f, int currentColor, int currentIndex, const char* lpSuffix)
{
	try 
	{
		//if (kConfig.keepDebugImg())
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

void getFrame(Mat& f, bool cutFrame = true)
{
	try 
	{
		Mat t;
		SPDLOG_SINKS_DEBUG("Get Frame");
		for (int i = 0; i < kConfig.skipFrame(); i++)
		{
			capture.read(t);
			cv::waitKey(g_wait_time);
			SPDLOG_SINKS_DEBUG("Get {} Frame", i);
		}
		t.copyTo(f);
		if (cutFrame)
		{
			f = f(kConfig.rect());
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

				sprintf_s(txt, 128, "Power Off: %d", kConfig.shutdownTime());
				cv::putText(camera, txt, Point(0, (camera.rows / 8)), FONT_HERSHEY_TRIPLEX, 1, Scalar(0, 255, 255), 1);
				if(!kConfig.rect().empty())
					rectangle(camera, kConfig.rect(), Scalar(0, 255, 255), 3);
				cv::imshow("camera", camera);

				key = cv::waitKey(g_wait_time);
				if (key == 0x1b)	// Esc 键
				{
					g_main_thread_exit = eExitWithKey;
					SPDLOG_SINKS_DEBUG("AutoGetCaptureFrame eExitWithKey");
				}
				else if (key == 0x30)	// 字符 0
				{
					kConfig.shutdownTime(eNotPowerOff);
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
				
				const float* hsv = kConfig.hsvColor(currentColor);

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
					if (r.area() < kConfig.minContoursArea())
						continue;

					rect |= r;

					drawContours(result, contours, i, Scalar(0, 255, 255), 1);
				}
				
				{
					saveDebugROIImg(result, currentColor, currentIndex, "contours");
				}
				//得到灯的轮廓
				if (rect.area() > kConfig.ledContoursArea())
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

		for (int color = kConfig.c1(); color < kConfig.c2(); ++color)
		{
			MainThreadIsExit;

			for (int index = 0; index < I2C.getLedCount(); index++)
			{
				MainThreadIsExit;

				I2C.setSignleColor(colorNum[index], 0, 0, 0);
				SPDLOG_SINKS_DEBUG("Turn off the {}th Led", colorNum[index]);
				Sleep(kConfig.intervalTime());
				SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);
				getFrame(internal_back);

				//int r = rng.uniform(0, 101);	//[0, 101)
				//SPDLOG_SINKS_DEBUG("The random number generated is {} ,RandomShutDownLedNum is {}", r, kConfig.randomLitOffProbability());
				//if (r >= kConfig.randomLitOffProbability())
				//if(AgingInstance.getThisLedLitOffState(index))
				{
					litoff.IsLitOff(index) ? (void)0 : I2C.setSignleColor(index, color);
					SPDLOG_SINKS_DEBUG("Turn on the {}th {} Led", index, color);
					Sleep(kConfig.intervalTime());
				}

				//Sleep(kConfig.intervalTime);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", kConfig.intervalTime);

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
	if (kConfig.recheckFaileLedTime() <= 0)
		return;
	try
	{
		SPDLOG_SINKS_DEBUG("---------------- Check the Failed Led Again 1 ----------------");

		//int g_recheckFaileLedTime = kConfig.recheckFaileLedTime;
		Mat internal_back;	// 暂存back
		//RNG rng(time(NULL));
		//std::vector<int> colorNum(I2C.getLedCount());
		//for (int i = 1; i < I2C.getLedCount(); i++)
		//{
		//	colorNum[i] = i - 1;
		//}
		//colorNum[0] = I2C.getLedCount() - 1;
		AgingInstance.syncSingLedResult2RetestResult();

		while (g_recheckFaileLedTime < kConfig.recheckFaileLedTime())
		{
			g_set_led_mutex.lock();
			g_recheckFaileLedTime++;
			g_set_led_mutex.unlock();
			SPDLOG_SINKS_DEBUG("Need to retest {} times, now is the {}th time", kConfig.recheckFaileLedTime(), g_recheckFaileLedTime);

			for (int color = kConfig.c1(); color < kConfig.c2(); ++color)
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

						Sleep(kConfig.intervalTime());
						SPDLOG_SINKS_DEBUG("Sleep {} millisecond", kConfig.intervalTime());

						getFrame(internal_back);
						SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);
						
						I2C.setSignleColor(index, color);
						SPDLOG_SINKS_DEBUG("Turn on the {}th {} Led", index, color);

						Sleep(kConfig.intervalTime());
						SPDLOG_SINKS_DEBUG("Sleep {} millisecond", kConfig.intervalTime());

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

			for (int color = kConfig.c1(); color < kConfig.c2(); ++color)
			{
				MainThreadIsExit;
				// 一个轮回保存一个灯色
				I2C.resetColor(color);
				SPDLOG_SINKS_DEBUG("Turn on all {} led, color {}", I2C.getLedCount(), color);
				Sleep(kConfig.intervalTime());
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", kConfig.intervalTime());

				Mat frame;
				getFrame(frame);	// get current frame
				char name[_MAX_PATH] = { 0 };
				sprintf_s(name, _MAX_PATH, "%s/%s/all_color_%02d.png", AgingFolder, VideoCardIns.targetFolder(), color);
				cv::putText(frame, AgingInstance.thisLedIsOK(color) == Pass ? "PASS" : "FAIL", Point(0, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 2);
				//SPDLOG_SINKS_DEBUG("Sleep {} millisecond", kConfig.intervalTime());
				cv::imwrite(name, frame);

				//I2C.resetColor(0, 0, 0);
				//Sleep(kConfig.intervalTime());
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

Rect frameDiff2ROI(const Mat& back, const Mat& fore, int color)
{
	try
	{
		SPDLOG_SINKS_DEBUG("..................frameDiff2ROI-1.....................");
		Mat b, f, mask, mask_hsv, hsv_img_mask;
		Rect roi;
		b = back;// back.copyTo(b);
		f = fore;// fore.copyTo(f);

		char name[MAX_PATH] = { 0 };
		//if (kConfig.keepDebugImg())
		{
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_fore.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, f);

			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d_back.png", AgingFolder, VideoCardIns.targetFolder(), color);
			cv::imwrite(name, b);
		}
		SPDLOG_SINKS_DEBUG("Frame difference algorithm starts.");
		cv::subtract(f, b, mask);

		const float* hsv = kConfig.hsvColor(color);
		const float* roi_hv = kConfig.roiHV();

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

		//if (kConfig.keepDebugImg())
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
			if (r.area() < kConfig.minContoursArea())
				continue;
			SPDLOG_SINKS_DEBUG("{}th rect.area:{}", i, r.area());

			roi |= r;

			drawContours(result, contours, i, Scalar(0, 255, 255), 1);
		}

		//if (kConfig.keepDebugImg())
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
				Sleep(kConfig.intervalTime());
				getFrame(back, false);

				SPDLOG_SINKS_DEBUG("lit-on {}th color", color);
				I2C.resetColor(color);
				Sleep(kConfig.intervalTime());
				getFrame(fore, false);

				roi[color] = frameDiff2ROI(back, fore, color);

				if (roi[color].empty()) 
				{
					SPDLOG_SINKS_ERROR("{}th color roi empty", color);
					throw ErrorCodeEx(ERR_POSTRUE_CORRECTION_ERROR, "Please readjust the camera or graphics card posture");
				}
			}

			cv::imshow("result", fore);
			cv::waitKey(g_wait_time);

			Rect r = (roi[BLUE] | roi[GREEN]) & roi[RED];
			kConfig.rect(r);
			SPDLOG_SINKS_DEBUG("After ROI:({},{}), width:{}, height:{}, area:{}", kConfig.rect().x, kConfig.rect().y, kConfig.rect().width, kConfig.rect().height, kConfig.rect().area());
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
	kConfig.shutdownTime(eNotPowerOff);
	g_error = e;
	cv::destroyAllWindows();
	SPDLOG_SINKS_DEBUG("g_main_thread_exit = {}, kConfig.shutdownTime = {}", g_main_thread_exit, kConfig.shutdownTime());
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
			if (!kConfig.keepDebugImg())
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

		kConfig.readConfigFile(VideoCardIns.Name(), I2C.getLedCount());

		litoff.setRandomLitOffState(kConfig.randomLitOffProbability(), parser.get<std::string>("lo"));

		capture.open(kConfig.cameraIndex());
		if (!capture.isOpened())
		{
			throw ErrorCodeEx(ERR_CANT_OPEN_CAMERA, "Failed to open camera");
		}
		else
		{
			//capture.set(CAP_PROP_SETTINGS, 1);
			capture.set(CAP_PROP_FPS, kConfig.cameraFps());
			capture.set(CAP_PROP_FRAME_WIDTH, kConfig.frame().width);
			capture.set(CAP_PROP_FRAME_HEIGHT, kConfig.frame().height);
			capture.set(CAP_PROP_EXPOSURE, kConfig.exposure());
			//capture.set(CAP_PROP_SATURATION, kConfig.saturation());

			capture.set(CAP_PROP_FOURCC, MAKEFOURCC('M', 'J', 'P', 'G'));

			g_wait_time = 1000 / kConfig.cameraFps();
			SPDLOG_SINKS_INFO("Wait Time:{}", g_wait_time);
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
	VideoCardIns.savePPID();

	showPassorFail();

	tm.stop();
	SPDLOG_SINKS_INFO("Tick Time: {}, {}", tm.getTimeSec(), tm.getTimeTicks());

	if (kConfig.shutdownTime() >= ePowerOff)
	{
		char shutdown[128] = { 0 };
		sprintf_s(shutdown, 128, "shutdown -s -t %d", kConfig.shutdownTime());
		system(shutdown);
	}
	else if (kConfig.shutdownTime() == eReStart)
	{
		char shutdown[128] = { 0 };
		sprintf_s(shutdown, 128, "shutdown -r -t 2");
		system(shutdown);
	}
	else if (kConfig.shutdownTime() == eNotPowerOff)
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