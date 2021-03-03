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
	int minContoursArea = 200;
	int minContoursSpace = 60;
	int g_tick = 50;


	cv::Size frame = cv::Size(1280, 780);
	cv::Rect rect = cv::Rect(200, 240, 900, 200);
	
	//char PPID[VGA_PPID_LENGTH] = { 0 };
	int bgrColorThres[AllColor];
	HsvColor hsvColor[AllColor];

	LEDColor startColor = BLUE;
	LEDColor stopColor = AllColor;

	/*************** Aging Setting ***************/
	bool showTrackBarWnd = true;
	//bool agingSettingSaveRectImages = true;
	unsigned agingTime = 1;
	int randomShutDownLed = 200;// 随机灭灯
	int shutdownTime = 0;   //自动关机延时

};
