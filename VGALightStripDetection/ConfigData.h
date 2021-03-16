#pragma once

#include <opencv2/opencv.hpp>
#include "CommonColor.h"
#include "PreDefine.h"

//#define VGA_PPID_LENGTH 20

class ConfigData
{
public:
	ConfigData();
	~ConfigData();

	//initConfigFile();
	void readConfigFile();
	void saveConfigData();

	bool resetRect = 0;
	bool debugMode = 0;
	int cameraIndex = 0;
	int ledCount = 22;
	int ledHalfCount = ledCount / 2;
	int intervalTime = 100;		// 灯珠亮灭的间隔时间
	int minContoursArea = 50;
	int minContoursSpace = 50;

	cv::Size frame = cv::Size(640, 480);
	int exposure = -5;	//相机曝光
	cv::Rect rect = cv::Rect(200, 240, 900, 200);
	
	int bgrColorThres[AllColor] = {50, 50, 50, 50};
	double bgrColorPercentage[AllColor] = {0.45, 0.45, 0.45, 0.45};
	HsvColor hsvColor[AllColor];

	LEDColor startColor = BLUE;
	LEDColor stopColor = AllColor;

	/*************** Aging Setting ***************/
	bool showTrackBarWnd = false;
	unsigned agingTime = 1;
	int randomShutDownLed = 0;// 随机灭灯
	int shutdownTime = eNotPowerOff;   //自动关机延时

	int thresoldBlockSize = 101;
	int thresoldC = -9;
	
	int recheckFaileLedTime = 0;	// 侦测到某个灯Faile后， 再重复测几次
};
