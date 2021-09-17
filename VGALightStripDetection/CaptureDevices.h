#pragma once

#include <map>

class CaptureDevices
{
	std::map<std::string, int> _videoCaptures;
	CaptureDevices();

public:
	~CaptureDevices();
	CaptureDevices(const CaptureDevices&) = delete;
	CaptureDevices& operator=(const CaptureDevices&) = delete;
	int cameraIndex(const std::string& camera);
	static CaptureDevices& instance();

private:
	void EnumerateDevicesInWindows();
};

#define CameraDevices CaptureDevices::instance()


