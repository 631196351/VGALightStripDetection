#include "CaptureDevices.h"
#include "SpdMultipleSinks.h"

#include <windows.h>
#include <dshow.h>
#pragma comment(lib, "strmiids")

#include <string>
#include <codecvt>
#include <locale>

#include "ConfigData.h"
#include "ErrorCode.h"

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

std::string to_string(std::wstring wstr)
{
	return strconverter.to_bytes(wstr);
}

std::wstring to_wstring(std::string str)
{
	return strconverter.from_bytes(str);
}

HRESULT EnumerateDevicesFunction(REFGUID category, IEnumMoniker **ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the category.
		hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
		}
		pDevEnum->Release();
	}
	return hr;
}

void CaptureDevices::EnumerateDevicesInWindows()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		IEnumMoniker *pEnum;

		int deviceId = -1;
		hr = EnumerateDevicesFunction(CLSID_VideoInputDeviceCategory, &pEnum);
		if (SUCCEEDED(hr))
		{
			IMoniker *pMoniker = NULL;

			while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
			{
				std::string deviceName;
				std::string devicePath;

				IPropertyBag *pPropBag;
				HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
				if (FAILED(hr))
				{
					pMoniker->Release();
					continue;
				}

				VARIANT var;
				VariantInit(&var);

				// Get description or friendly name.
				hr = pPropBag->Read(L"Description", &var, 0);
				if (FAILED(hr))
				{
					hr = pPropBag->Read(L"FriendlyName", &var, 0);
				}
				if (SUCCEEDED(hr))
				{
					deviceName = to_string(var.bstrVal);
					VariantClear(&var);
				}

				//hr = pPropBag->Write(L"FriendlyName", &var);
				//
				// WaveInID applies only to audio capture devices.
				//hr = pPropBag->Read(L"WaveInID", &var, 0);
				//if (SUCCEEDED(hr))
				//{
				//	printf("WaveIn ID: %d\n", var.lVal);
				//	VariantClear(&var);
				//}

				hr = pPropBag->Read(L"DevicePath", &var, 0);
				if (SUCCEEDED(hr))
				{
					// The device path is not intended for display.
					devicePath = to_string(var.bstrVal);
					VariantClear(&var);
				}
				deviceId++;
				_videoCaptures.insert({ deviceName, deviceId });
				_index2Cap.insert({ deviceId, deviceName });
				SPDLOG_SINKS_DEBUG("{}th capture {}, DevicePath : {}", deviceId, deviceName, devicePath);
				pPropBag->Release();
				pMoniker->Release();
			}
			pEnum->Release();
		}
		CoUninitialize();
	}
}

CaptureDevices::CaptureDevices()
{
	EnumerateDevicesInWindows();
	
	for (auto& c : _videoCaptures)
	{
		SPDLOG_SINKS_INFO("Init {} camera start", c.first);
		// 目标立面不开启相机时, 放一个空相机上去
		auto& _pair = _captures.insert(std::make_pair(c.second, cv::VideoCapture()));
		if (kConfig.captureOpenState(c.first))
		{
			auto& camera = _pair.first->second;
			camera.open(c.second);
			camera.set(cv::CAP_PROP_FPS, kConfig.cameraFps());
			camera.set(cv::CAP_PROP_FRAME_WIDTH, kConfig.frame().width);
			camera.set(cv::CAP_PROP_FRAME_HEIGHT, kConfig.frame().height);
			camera.set(cv::CAP_PROP_EXPOSURE, kConfig.exposure(c.first));
			camera.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
		}
		SPDLOG_SINKS_INFO("Init {} camera stop", c.first);
	}

	_waitTime = 1000 / kConfig.cameraFps();
	SPDLOG_SINKS_INFO("Capture wait time {}s", _waitTime);
}

CaptureDevices::~CaptureDevices()
{
}

CaptureDevices& CaptureDevices::instance()
{
	static CaptureDevices g_capture;
	return g_capture;
}

int CaptureDevices::cameraIndex(const std::string& camera)
{
	auto iter = _videoCaptures.find(camera);
	if (iter == _videoCaptures.end())
		return -1;
	else
		return iter->second;
}

std::string CaptureDevices::cameraName(const int index)
{
	auto i = _index2Cap.find(index);
	if (i == _index2Cap.end())
		return std::string();
	else
		return i->second;
}

/// @brief 从指定相机中抓取一张图像出来
/// @param index 表示相机序列
/// @param image 为输出图像
/// @param filling 表示是否要对输出图像根据对应的ROI进行裁剪
/// @return 图像抓取成功返回true, 否则返回false
bool CaptureDevices::read(int index, cv::Mat& image, bool filling)
{
	bool result = false;
	auto& c = _captures.find(index);
	if (c != _captures.end() && c->second.isOpened())
	{
		result = c->second.read(image);
		cv::waitKey(_waitTime);
		if (image.empty())
			throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty, check camera usb");

		if (filling)
		{
			const cv::Rect* rois = kConfig.rois();
			image(rois[index]).copyTo(image);
		}
	}
	return result;
}

/// @brief 从所有相机中抓取一张图
/// @param f 表示输出图像集合
/// @param filling 表示是本次调用是否需要将图像输出到f中
bool CaptureDevices::read_iterator(std::vector<cv::Mat>& f, bool filling)
{
	int index = 0;
	for (auto& c : _captures)
	{
		if (c.second.isOpened())
		{
			cv::Mat img;
			c.second.read(img);
			cv::waitKey(_waitTime);
			if (img.empty())
				throw ErrorCodeEx(ERR_ORIGIN_FRAME_EMPTY_EXCEPTION, "Original frame empty, check camera usb");
			if (filling)
			{
				img.copyTo(f[index]);
			}
		}
		else
		{
			// 该立面的相机不处于打开状态
			// 为了autoCaptureROI2() B-G-R三色来圈取灯带的ROI 能够和相机的index对齐
			// 在这里放一张空照片,来假装我已经抓到该立面的图了
			f[index] = cv::Mat();
		}
		++index;
	}
	return true;
}