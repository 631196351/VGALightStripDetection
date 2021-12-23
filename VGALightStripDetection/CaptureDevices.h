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
��KOϵ��Ϊ��

CaptureDevices ��ʼ��ʱ, ��ȡ�Ѱ�װ������豸��Ϣ<index, device name> ӳ���;
�ٸ��� <index, device name> ����һ�����, �����ø���������е�����ֵ. 

using VideoCaptures = std::map<int, cv::VideoCapture>; ���������id �� ��������ӳ���

��ץȡ�ƴ�ROI�ڼ�, �������е�, ͨ�� read_iterator �����������ץȡ���������ͼ��
blue = <top_img_b, rear_img_b, front_img_b> (���˳�������� VideoCaptures ����)
green = <top_img_g, rear_img_g, front_img_g> 
red  = <top_img_r, rear_img_r, front_img_r> 

roi[CaptureNum];

roi[index_0] = (top_img_b || top_img_g) && top_img_r
roi[index_1] = (rear_img_b || rear_img_g) && rear_img_r
roi[index_2] = (front_img_b || front_img_g) ** front_img_r

ConfigData::rect(ColorROI& roi, int colors)��,
roi[BLUE]�� 0, 1, 2, ˳���Ǹ����˳��һ�µ�, ������ȷ�����ĸ������, ��Ϊ���ÿ�εĿ���˳���в�ȷ����
�����ֲ�ȷ����, ����������ʱ��, ��ȷ����(Ѧ���̵�è????)

��mainLightingControl ��, led index ��ȥ�Ҷ�Ӧ��camera index;
���camera index �ͻ�� roi[BLUE] �е�0, 1, 2 ��Ӧ��

*/