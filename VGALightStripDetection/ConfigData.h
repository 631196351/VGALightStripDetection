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
#if false
	void readConfigFile();
	void saveConfigData();
#endif
	void readConfigFile(std::string model, unsigned led_count);
	void recordConfig2WorkStates();

private:
	std::string _thermo_name;
private:
	int _intervalTime = 100;		// 灯珠亮灭的间隔时间
	int _minContoursArea = 50;
	// 合并后的轮廓像素面积若小于_ledContoursArea，则认为有些太小了
	// 原本设定的阈值是1000， 在测试Tuf系列机种时发现， 因摄像头与灯带夹角比较小时，成像面积在500左右，所以砍半设置
	// 在测试f94 3090系列时，灯带纹路成像面积在300左右，
	int _ledContoursArea =500;

	//int _cameraIndex = 0;
	//int _exposure = -5;	//相机曝光
	std::vector<int> _exposures;

	int _saturation = 65;	// 相机的饱和度
	cv::Size _frame = cv::Size(848, 480);
	cv::Rect _roi[CaptureNum];
	cv::Rect _rect2 = cv::Rect();
	int _skipFrame = 3;
	
	float _hsv[BGR][6] = { 0 };	// BG后各有2个字节冗余
	float _hsvROI[2] = { 0 };

	LEDColor _startColor = BLUE;
	LEDColor _stopColor = AllColor;

	int _randomShutDownLed = 0;// 随机灭灯
	int _shutdownTime = eNotPowerOff;   //自动关机延时
	int _thresoldBlockSize = 101;
	int _thresoldC = -9;
	int _recheckFaileLedTime = 0;	// 侦测到某个灯Faile后， 再重复测几次

	std::vector<std::string> _videoCapName;
	std::set<int> _front;
	std::set<int> _rear;
	std::set<int> _overhead;

	bool _keepDebugImg = true;

public:
	inline int intervalTime() const { return _intervalTime; }
	inline int minContoursArea() const { return _minContoursArea; }
	inline int ledContoursArea() const { return _ledContoursArea; }
	//inline int exposure() const { return _exposure; }
	inline int saturation() const { return _saturation; }
	inline const cv::Size& frame() const { return _frame; }
	inline const cv::Rect* rois() const { return _roi; }
	inline const cv::Rect& rect2() const { return _rect2; }
	inline int skipFrame() const { return _skipFrame; }
	inline LEDColor c1() const { return _startColor; }
	inline LEDColor c2() const { return _stopColor; }
	inline int randomLitOffProbability() const { return _randomShutDownLed; }
	inline int shutdownTime() const { return _shutdownTime; }
	inline int thresoldBlockSize() const { return _thresoldBlockSize; }
	inline int thresoldC() const { return _thresoldC; }
	inline int recheckFaileLedTime() const { return _recheckFaileLedTime; }
	inline const float* hsvColor(int color) const { return _hsv[color]; }
	inline const float* roiHV() const { return _hsvROI; }
	inline std::string getVideoCapName(int index) const { return _videoCapName[index]; };
	inline bool keepDebugImg() const { return _keepDebugImg; }

public:
	//void rect(cv::Rect& r);
	void rect(cv::Rect roi[][CaptureNum], int colors);
	void shutdownTime(int t);
	int ledIndexToCamera(int led_index);
	int exposure(int cap);
};

#define cfg ConfigData::instance()