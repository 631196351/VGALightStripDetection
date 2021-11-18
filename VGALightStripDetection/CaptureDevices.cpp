#include "CaptureDevices.h"
#include "SpdMultipleSinks.h"

#include <windows.h>
#include <dshow.h>
#pragma comment(lib, "strmiids")

#include <string>
#include <codecvt>
#include <locale>

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
	// IMF 获取到的设备id-name 会概率性的与 opencv open id 不匹配
	// 这里换成 COM 形式获取
	//_deviceList.EnumerateDevices();
	//
	//unsigned count = _deviceList.Count();
	//wchar_t *szFriendlyName = NULL;
	//SPDLOG_SINKS_DEBUG("capture count :{}", count);
	//for (unsigned i = 0; i < count; ++i)
	//{
	//	_deviceList.GetDeviceName(i, &szFriendlyName);
	//
	//	std::string t = to_string(szFriendlyName);
	//	_videoCaptures.insert({ t, i });
	//	SPDLOG_SINKS_DEBUG("capture name :{}", t);
	//
	//	CoTaskMemFree(szFriendlyName);
	//	szFriendlyName = NULL;
	//}
	EnumerateDevicesInWindows();
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
