#include "ConfigData.h"
#include <Windows.h>

#define SPRINTF(ARG) wsprintf(d, L"%d", ARG);

ConfigData::ConfigData()
{
	readConfigFile();
}


ConfigData::~ConfigData()
{
	if (_dirty)
	{
		saveConfigData();
	}
}

void ConfigData::saveConfigData()
{
	TCHAR lpPath[MAX_PATH] = { 0 };
	wcscpy_s(lpPath, MAX_PATH, L"./3c.ini");

	TCHAR d[128] = { 0 };
	//[Camera]
	SPRINTF(_cameraIndex); WritePrivateProfileString(L"Camera", L"Index", d, lpPath);
	SPRINTF(_exposure); WritePrivateProfileString(L"Camera", L"Exposure", d, lpPath);
	SPRINTF(_saturation); WritePrivateProfileString(L"Camera", L"Saturation", d, lpPath);

	//[Frame]
	SPRINTF(_frame.width); WritePrivateProfileString(L"Frame", L"Width", d, lpPath);
	SPRINTF(_frame.height); WritePrivateProfileString(L"Frame", L"Hight", d, lpPath);
	SPRINTF(_skipFrame); WritePrivateProfileString(L"Frame", L"SkipFrame", d, lpPath);
	
	//[AgingSetting]
	SPRINTF(_startColor); WritePrivateProfileString(L"AgingSetting", L"StartColor", d, lpPath);
	SPRINTF(_stopColor); WritePrivateProfileString(L"AgingSetting", L"StopColor", d, lpPath);	
    SPRINTF(_randomShutDownLed); WritePrivateProfileString(L"AgingSetting", L"RandomShutDownLedNum", d, lpPath);
	SPRINTF(_shutdownTime); WritePrivateProfileString(L"AgingSetting", L"ShutDownDelayTime", d, lpPath);
	SPRINTF(_recheckFaileLedTime); WritePrivateProfileString(L"AgingSetting", L"RecheckFaileLedTime", d, lpPath);

	//[AlgorithmThreshold]
	SPRINTF(_intervalTime); WritePrivateProfileString(L"AlgorithmThreshold", L"IntervalTime", d, lpPath);
	SPRINTF(_minContoursArea); WritePrivateProfileString(L"AlgorithmThreshold", L"MinContoursArea", d, lpPath);
	SPRINTF(_minContoursSpace); WritePrivateProfileString(L"AlgorithmThreshold", L"MinContoursSpace", d, lpPath);
	SPRINTF(_minContoursSpace2); WritePrivateProfileString(L"AlgorithmThreshold", L"MinContoursSpace2", d, lpPath);
	SPRINTF(_thresoldBlockSize);  WritePrivateProfileString(L"AlgorithmThreshold", L"AdaptiveThresholdArgBlockSize", d, lpPath);
	SPRINTF(_thresoldC);  WritePrivateProfileString(L"AlgorithmThreshold", L"AdaptiveThresholdArgC", d, lpPath);
	SPRINTF(_bgrColorThres[BLUE]); WritePrivateProfileString(L"AlgorithmThreshold", L"BBrightness", d, lpPath);
	SPRINTF((int)(_bgrColorPercentage[BLUE] * 100)); WritePrivateProfileString(L"AlgorithmThreshold", L"BPercentage", d, lpPath);

	SPRINTF(_bgrColorThres[GREEN]); WritePrivateProfileString(L"AlgorithmThreshold", L"GBrightness", d, lpPath);
	SPRINTF((int)(_bgrColorPercentage[GREEN] * 100)); WritePrivateProfileString(L"AlgorithmThreshold", L"GPercentage", d, lpPath);

	SPRINTF(_bgrColorThres[RED]); WritePrivateProfileString(L"AlgorithmThreshold", L"RBrightness", d, lpPath);
	SPRINTF((int)(_bgrColorPercentage[RED] * 100)); WritePrivateProfileString(L"AlgorithmThreshold", L"RPercentage", d, lpPath);

	SPRINTF(_bgrColorThres[WHITE]); WritePrivateProfileString(L"AlgorithmThreshold", L"WBrightness", d, lpPath);
	SPRINTF((int)(_bgrColorPercentage[WHITE] * 100)); WritePrivateProfileString(L"AlgorithmThreshold", L"WPercentage", d, lpPath);
}

void ConfigData::readConfigFile()
{
	TCHAR lpPath[MAX_PATH] = { 0 };
	wcscpy_s(lpPath, MAX_PATH, L"./3c.ini");

	//[Camera]
	_cameraIndex = GetPrivateProfileInt(L"Camera", L"Index", _cameraIndex, lpPath);
	_exposure = GetPrivateProfileInt(L"Camera", L"Exposure", _exposure, lpPath);
	_saturation = GetPrivateProfileInt(L"Camera", L"Saturation", _saturation, lpPath);

	//[Frame]
	_frame.width = GetPrivateProfileInt(L"Frame", L"Width", _frame.width, lpPath);
	_frame.height = GetPrivateProfileInt(L"Frame", L"Hight", _frame.height, lpPath);
	_skipFrame = GetPrivateProfileInt(L"Frame", L"SkipFrame", _skipFrame, lpPath);

	//[AgingSetting]
	_startColor = (LEDColor)GetPrivateProfileInt(L"AgingSetting", L"StartColor", _startColor, lpPath);
	_stopColor = (LEDColor)GetPrivateProfileInt(L"AgingSetting", L"StopColor", _stopColor, lpPath);
    _randomShutDownLed = GetPrivateProfileInt(L"AgingSetting", L"RandomShutDownLedNum", _randomShutDownLed, lpPath);
	_shutdownTime = GetPrivateProfileInt(L"AgingSetting", L"ShutDownDelayTime", _shutdownTime, lpPath);	
	_recheckFaileLedTime = GetPrivateProfileInt(L"AgingSetting", L"RecheckFaileLedTime", _recheckFaileLedTime, lpPath);

	//[AlgorithmThreshold]
	_intervalTime = GetPrivateProfileInt(L"AlgorithmThreshold", L"IntervalTime", _intervalTime, lpPath);
	_minContoursArea = GetPrivateProfileInt(L"AlgorithmThreshold", L"MinContoursArea", _minContoursArea, lpPath);
	_minContoursSpace = GetPrivateProfileInt(L"AlgorithmThreshold", L"MinContoursSpace", _minContoursSpace, lpPath);
	_minContoursSpace2 = GetPrivateProfileInt(L"AlgorithmThreshold", L"MinContoursSpace2", _minContoursSpace2, lpPath);
	_thresoldBlockSize = GetPrivateProfileInt(L"AlgorithmThreshold", L"AdaptiveThresholdArgBlockSize", _thresoldBlockSize, lpPath);
	_thresoldC = GetPrivateProfileInt(L"AlgorithmThreshold", L"AdaptiveThresholdArgC", _thresoldC, lpPath);
	
	_bgrColorThres[BLUE] = GetPrivateProfileInt(L"AlgorithmThreshold", L"BBrightness", _bgrColorThres[BLUE], lpPath);
	_bgrColorThres[GREEN] = GetPrivateProfileInt(L"AlgorithmThreshold", L"GBrightness", _bgrColorThres[GREEN], lpPath);
	_bgrColorThres[RED] = GetPrivateProfileInt(L"AlgorithmThreshold", L"RBrightness", _bgrColorThres[RED], lpPath);
	_bgrColorThres[WHITE] = GetPrivateProfileInt(L"AlgorithmThreshold", L"WBrightness", _bgrColorThres[WHITE], lpPath);

	_bgrColorPercentage[BLUE] = GetPrivateProfileInt(L"AlgorithmThreshold", L"BPercentage", 45, lpPath) / 100.0;
	_bgrColorPercentage[GREEN] = GetPrivateProfileInt(L"AlgorithmThreshold", L"GPercentage", 45, lpPath) / 100.0;
	_bgrColorPercentage[RED] = GetPrivateProfileInt(L"AlgorithmThreshold", L"RPercentage", 45, lpPath) / 100.0;
	_bgrColorPercentage[WHITE] = GetPrivateProfileInt(L"AlgorithmThreshold", L"WPercentage", 45, lpPath) / 100.0;
}

void ConfigData::rect(cv::Rect& r)
{
	/// 检查ROI区域是否超出画面宽高
	_rect = r & cv::Rect(0, 0, _frame.width, _frame.height);
	//_dirty = true;
}

void ConfigData::shutdownTime(int t)
{
	_shutdownTime = t;
	//_dirty = true;
}

ConfigData& ConfigData::instance()
{
	static ConfigData instance;
	return instance;
}