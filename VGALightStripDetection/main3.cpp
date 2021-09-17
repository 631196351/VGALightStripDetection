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
#include "RandomLitoff.h"
#include "CaptureDevices.h"

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
#define SAVE_ROI_FBMCR
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

const int g_pyDown = 2;	//下采样倍率

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

void getFrame( std::vector<Mat>& f)
{
	EXCEPTION_OPERATOR_TRY
	{
		Mat img;
		SPDLOG_SINKS_DEBUG("Get Frame");
		for (int i = 0; i < cfg.skipFrame(); ++i)
		{
			for (int j = 0; j < CaptureNum; ++j)
			{
				g_captures[j].read(img);
				SPDLOG_SINKS_DEBUG("{}th capture's {}th frame", j, i);
				if (img.empty())
					throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty, check camera usb");
				if (i + 1 >= cfg.skipFrame())
				{
					img.copyTo(f[j]);
				}
			}
		}
	}
	EXCEPTION_OPERATOR_CATCH_2;
}

void getSingleFrame(cv::Mat& f,int camera = 0)
{
	EXCEPTION_OPERATOR_TRY
	{
		Mat img;
		SPDLOG_SINKS_DEBUG("Get Frame");
		for (int i = 0; i < cfg.skipFrame(); ++i)
		{
			g_captures[camera].read(img);
			SPDLOG_SINKS_DEBUG("{}th capture's {}th frame", camera, i);
			if (img.empty())
				throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty, check camera usb");
			if (i + 1 >= cfg.skipFrame())
			{
				const Rect* rois = cfg.rois();
				img(rois[camera]).copyTo(f);
			}
		}
	}
	EXCEPTION_OPERATOR_CATCH_2;
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
				pyrDown(video, video, Size(video.cols / g_pyDown, video.rows / g_pyDown));
				imshow("video", video);

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
		EXCEPTION_OPERATOR_CATCH_3;
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
	EXCEPTION_OPERATOR_CATCH_2;
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

				Mat mask, mask_hsv, hsv_img_mask;
				Mat frame, back;
				g_fore[0].copyTo(frame);
				g_back[0].copyTo(back);

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

				cv::subtract(frame, back, mask);

				const float* hsv = cfg.hsvColor(currentColor);

				cv::cvtColor(mask, mask_hsv, COLOR_BGR2HSV);

				cv::inRange(mask_hsv, Scalar(hsv[0], hsv[2], hsv[3]), Scalar(hsv[1], 255, 255), hsv_img_mask);

				if (currentColor == RED)
				{
					Mat hsv_img_mask_r;

					cv::inRange(mask_hsv, Scalar(hsv[4], hsv[2], hsv[3]), Scalar(hsv[5], 255, 255), hsv_img_mask_r);
					hsv_img_mask += hsv_img_mask_r;
				}

				cv::adaptiveThreshold(hsv_img_mask, hsv_img_mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, cfg.thresoldBlockSize(), cfg.thresoldC());

				cv::medianBlur(hsv_img_mask, hsv_img_mask, 3);

				cv::GaussianBlur(hsv_img_mask, hsv_img_mask, cv::Size(3, 3), 0);

				{
#ifdef DEBUG_DETAILS
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

					//SPDLOG_SINKS_DEBUG("{}th rect.area:{}", i, r.area());
					// 轮廓面积校验
					if (r.area() < cfg.minContoursArea())
						continue;

					rect |= r;

					drawContours(result, contours, i, Scalar(0, 255, 255), 1);
				}

				saveDebugROIImg(result, currentColor, currentIndex, "contours");
				//得到灯的轮廓
				if (rect.area() > cfg.ledContoursArea())
				{
					cv::rectangle(frame, rect, Scalar(0, 255, 255), 1);
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
			EXCEPTION_OPERATOR_CATCH_3;

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
		cv::Mat back;
		cv::Mat fore;
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
				
				int camera_index = cfg.lanternIndexToCamera(index);

				I2C.setSignleColor(colorNum[index], BLACK);
				Sleep(cfg.intervalTime());
				SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);
				getSingleFrame(back, camera_index);

				litoff.IsLitOff(index) ? (void)0 : I2C.setSignleColor(index, color);
				Sleep(cfg.intervalTime());
				SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);
				getSingleFrame(fore, camera_index);

				g_set_led_mutex.lock();
				g_Index = index;
				g_Led = color;
				g_wait = true;
				//g_CameraIndex = camera_index;
				//deepCopyMat(back, g_back);
				//deepCopyMat(fore, g_fore);
				back.copyTo(g_back[0]);
				fore.copyTo(g_fore[0]);
				g_set_led_mutex.unlock();
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
	EXCEPTION_OPERATOR_CATCH_2;
}


void frameDiff2ROI(const std::vector<Mat>& back, const std::vector<Mat>& fore, int color, Rect outRect[][CaptureNum])
{
	try
	{
		SPDLOG_SINKS_DEBUG("****************frameDiff2ROI****************");
		for (int x = 0; x < CaptureNum; ++x)
		{
			Mat b, f, mask, mask_hsv, hsv_img_mask;
			Rect roi;
			b = back[x];// back.copyTo(b);
			f = fore[x];// fore.copyTo(f);

#ifdef SAVE_ROI_FBMCR
			char name[MAX_PATH] = { 0 };
			{
				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_fore.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
				cv::imwrite(name, f);

				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_back.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
				cv::imwrite(name, b);
			}
#endif
			cv::subtract(f, b, mask);

			const float* hsv = cfg.hsvColor(color);
			const float* roi_hv = cfg.roiHV();

			cv::cvtColor(mask, mask_hsv, COLOR_BGR2HSV);

			cv::inRange(mask_hsv, Scalar(hsv[eHmin], roi_hv[0], roi_hv[1]), Scalar(hsv[eHmax], 255, 255), hsv_img_mask);

			if (color == RED)
			{
				Mat hsv_img_mask_r;

				cv::inRange(mask_hsv, Scalar(hsv[eHmin2], roi_hv[0], roi_hv[1]), Scalar(hsv[eHmax2], 255, 255), hsv_img_mask_r);
				hsv_img_mask += hsv_img_mask_r;
			}

			cv::adaptiveThreshold(hsv_img_mask, hsv_img_mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, cfg.thresoldBlockSize(), cfg.thresoldC());

			cv::medianBlur(hsv_img_mask, hsv_img_mask, 3);

			cv::GaussianBlur(hsv_img_mask, hsv_img_mask, cv::Size(3, 3), 0);
#ifdef SAVE_ROI_FBMCR
			{
				sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_mask.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
				cv::imwrite(name, hsv_img_mask);
			}
#endif
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

				//SPDLOG_SINKS_DEBUG("{}th rect.area:{}", i, r.area());
				// 轮廓面积校验
				if (r.area() < cfg.minContoursArea())
					continue;

				roi |= r;

				drawContours(result, contours, i, Scalar(0, 255, 255), 1);
			}

			cv::rectangle(f, roi, Scalar(255, 0, 255), 1);
#ifdef SAVE_ROI_FBMCR
			sprintf_s(name, MAX_PATH, "%s/%s/roi_%02d%02d_result.png", AgingFolder, VideoCardIns.targetFolder(), color, x);
			cv::imwrite(name, f);
#endif
			SPDLOG_SINKS_DEBUG("{} color {}th ROI x:{},y:{}, width:{}, height:{}", color, x, roi.x, roi.y, roi.width, roi.height);
			outRect[color][x] = roi;
		}
	}
	EXCEPTION_OPERATOR_CATCH_2;
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
				getFrame(back);

				SPDLOG_SINKS_DEBUG("lit-on {}th color", color);
				I2C.resetColor(color);
				Sleep(cfg.intervalTime());
				getFrame(fore);

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
				pyrDown(frame, frame, Size(frame.cols / g_pyDown, frame.rows / g_pyDown));
				cv::imshow("result", frame);
				cv::waitKey(33);
			}

			cfg.rect(roi, BGR);
			cv::destroyWindow("result");
			break;
		}
		EXCEPTION_OPERATOR_CATCH_1;
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

	try
	{
		SPDLOG_SINKS_DEBUG("---------------- Check the Failed Led Again 1 ----------------");
		cv::Mat back;
		cv::Mat fore;

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
						int camera_index = cfg.lanternIndexToCamera(index);

						// 关闭所有灯
						I2C.resetColor(0, 0, 0);
						Sleep(cfg.intervalTime());
						getSingleFrame(back, camera_index);
						SPDLOG_SINKS_DEBUG("Get the background of the {}th Led ", index);

						I2C.setSignleColor(index, color);
						Sleep(cfg.intervalTime());
						getSingleFrame(fore, camera_index);
						SPDLOG_SINKS_DEBUG("Get the foreground of the {}th Led ", index);

						g_set_led_mutex.lock();
						g_Index = index;
						g_Led = color;
						g_wait = true;
						back.copyTo(g_back[0]);
						fore.copyTo(g_fore[0]);

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
	EXCEPTION_OPERATOR_CATCH_2;
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
		SPDLOG_SINKS_INFO("CaptureNum:{}", CaptureNum);
		CameraDevices;

		// 避免亮光影响相机初始化
		I2C.resetColor(0, 0, 0);

		cfg.readConfigFile(VideoCardIns.Name(), I2C.getLedCount());

		litoff.setRandomLitOffState(cfg.randomLitOffProbability(), parser.get<std::string>("lo"));

		for (int i = 0; i < CaptureNum; i++)
		{
			if (!g_captures[i].open(i, CAP_DSHOW))
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
		}
	}
	EXCEPTION_OPERATOR_CATCH_3;

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
