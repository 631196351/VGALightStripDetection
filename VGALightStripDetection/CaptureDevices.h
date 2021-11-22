#pragma once

#include <map>

class CaptureDevices
{
	std::map<std::string, int> _videoCaptures;
	std::map<int, std::string> _index2Cap;
	CaptureDevices();

public:
	~CaptureDevices();
	CaptureDevices(const CaptureDevices&) = delete;
	CaptureDevices& operator=(const CaptureDevices&) = delete;
	int cameraIndex(const std::string& camera);
	std::string cameraName(const int index);
	static CaptureDevices& instance();

private:
	void EnumerateDevicesInWindows();
};

#define CameraDevices CaptureDevices::instance()


