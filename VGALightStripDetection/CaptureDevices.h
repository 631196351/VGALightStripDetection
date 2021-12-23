#pragma once

#include <map>
#include <vector>
#include "PreDefine.h"
#include <opencv2/opencv.hpp>

using VideoCaptures = std::map<int, cv::VideoCapture>;
using VideoCapturesName2Index = std::map<std::string, int>;
using VideoCapturesIndex2Name = std::map<int, std::string>;
class CaptureDevices
{
	VideoCapturesName2Index _videoCaptures;
	VideoCapturesIndex2Name _index2Cap;
	VideoCaptures _captures;
	unsigned _waitTime = 33;
	unsigned _openedCount = 0;

	CaptureDevices();

public:
	~CaptureDevices();
	CaptureDevices(const CaptureDevices&) = delete;
	CaptureDevices& operator=(const CaptureDevices&) = delete;
	int cameraIndex(const std::string& camera);
	std::string cameraName(const int index);
	static CaptureDevices& instance();

public:
	bool read(int index, cv::Mat& image, bool filling = false);
	bool read_iterator(std::vector<cv::Mat>& f, bool filling = false);
	inline unsigned waitTime() const { return _waitTime; }
	inline unsigned openCount() const { return _openedCount; }
private:
	void EnumerateDevicesInWindows();

};

#define kCameraDevices CaptureDevices::instance()


/**
以KO系列为例

CaptureDevices 初始化时, 获取已安装的相机设备信息<index, device name> 映射表;
再根据 <index, device name> 来逐一打开相机, 并设置该相机所独有的属性值. 

using VideoCaptures = std::map<int, cv::VideoCapture>; 来保存相机id 和 相机对象的映射表

在抓取灯带ROI期间, 点亮所有灯, 通过 read_iterator 从三个相机中抓取到三个面的图像
blue = <top_img_b, rear_img_b, front_img_b> (这个顺序是依据 VideoCaptures 来的)
green = <top_img_g, rear_img_g, front_img_g> 
red  = <top_img_r, rear_img_r, front_img_r> 

roi[CaptureNum];

roi[index_0] = (top_img_b || top_img_g) && top_img_r
roi[index_1] = (rear_img_b || rear_img_g) && rear_img_r
roi[index_2] = (front_img_b || front_img_g) ** front_img_r

ConfigData::rect(ColorROI& roi, int colors)中,
roi[BLUE]的 0, 1, 2, 顺序是跟相机顺序一致的, 但不能确定是哪个相机的, 因为相机每次的开启顺序有不确定性
但这种不确定性, 在跑起来的时候, 就确定了(薛定谔的猫????)

在mainLightingControl 中, led index 会去找对应的camera index;
这个camera index 就会和 roi[BLUE] 中的0, 1, 2 对应上

*/