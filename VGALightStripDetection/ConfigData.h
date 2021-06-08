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

	static ConfigData& instance();
	//initConfigFile();
	void readConfigFile();
	void saveConfigData();

private:
	bool _dirty = false;	// 修改属性后，在销毁对象时进行保存
private:
	//bool resetRect = 0;
	//bool _debugMode = 0;
	//int ledCount = 22;
	//int ledHalfCount = ledCount / 2;
	int _intervalTime = 100;		// 灯珠亮灭的间隔时间
	int _minContoursArea = 50;
	int _minContoursSpace = 50;
	int _minContoursSpace2 = 300;	//查找合并轮廓阶段，为了能够将背面&&前面的亮灯区域合并到一起，设定的阈值

	int _cameraIndex = 0;
	int _exposure = -5;	//相机曝光
	int _saturation = 128;	// 相机的饱和度
	cv::Size _frame = cv::Size(848, 480);
	cv::Rect _rect = cv::Rect();// ROI 区域设定， 不建议直接修改，需要通过rect()设定
	int _skipFrame = 3;
	
	int _bgrColorThres[AllColor] = {50, 50, 50, 50};
	double _bgrColorPercentage[AllColor] = {0.45, 0.45, 0.45, 0.45};
	//HsvColor hsvColor[AllColor];

	LEDColor _startColor = BLUE;
	LEDColor _stopColor = AllColor;

	/*************** Aging Setting ***************/
	//bool showTrackBarWnd = false;
	//unsigned agingTime = 1;
	int _randomShutDownLed = 0;// 随机灭灯
	int _shutdownTime = eNotPowerOff;   //自动关机延时
	int _thresoldBlockSize = 101;
	int _thresoldC = -9;	
	int _recheckFaileLedTime = 0;	// 侦测到某个灯Faile后， 再重复测几次

public:
	//inline bool debugMode() const { return _debugMode; }
	inline int intervalTime() const { return _intervalTime; }
	inline int minContoursArea() const { return _minContoursArea; }
	inline int minContoursSpace() const { return _minContoursSpace; }
	inline int cameraIndex() const { return _cameraIndex;}
	inline int exposure() const { return _exposure; }
	inline int saturation() const { return _saturation; }
	inline const cv::Size& frame() const { return _frame; }
	inline const cv::Rect& rect() const { return _rect; }
	inline int skipFrame() const { return _skipFrame; }
	inline int bgrThres(int c) const { return _bgrColorThres[c]; }
	inline double bgrPercentage(int c) const { return _bgrColorPercentage[c]; }
	inline LEDColor c1() const { return _startColor; }
	inline LEDColor c2() const { return _stopColor; }
	inline int randomLitOffProbability() const { return _randomShutDownLed; }
	inline int shutdownTime() const { return _shutdownTime; }
	inline int thresoldBlockSize() const { return _thresoldBlockSize; }
	inline int thresoldC() const { return _thresoldC; }
	inline int recheckFaileLedTime() const { return _recheckFaileLedTime; }

public:
	void rect(cv::Rect& r);	
	void shutdownTime(int t);
};

#define cfg ConfigData::instance()