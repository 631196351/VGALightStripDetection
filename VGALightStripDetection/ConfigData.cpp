#include "ConfigData.h"
#include <Windows.h>

#define SPRINTF(ARG) wsprintf(d, L"%d", ARG);

ConfigData::ConfigData()
{
	readConfigFile();
}


ConfigData::~ConfigData()
{
}

void ConfigData::saveConfigData()
{
	TCHAR lpPath[MAX_PATH] = { 0 };
	wcscpy_s(lpPath, MAX_PATH, L"./3c.ini");

	TCHAR d[128] = { 0 };
	//[Camera]
	SPRINTF(cameraIndex); WritePrivateProfileString(L"Camera", L"Index", d, lpPath);
	SPRINTF(exposure); WritePrivateProfileString(L"Camera", L"Exposure", d, lpPath);

	//[Frame]
	SPRINTF(frame.width); WritePrivateProfileString(L"Frame", L"Width", d, lpPath);
	SPRINTF(frame.height); WritePrivateProfileString(L"Frame", L"Hight", d, lpPath);

	//[RectFrame]
	SPRINTF(rect.x);	WritePrivateProfileString(L"RectFrame", L"X", d, lpPath);
	SPRINTF(rect.y);	WritePrivateProfileString(L"RectFrame", L"Y", d, lpPath);
	SPRINTF(rect.width);	WritePrivateProfileString(L"RectFrame", L"Width", d, lpPath);
	SPRINTF(rect.height);	WritePrivateProfileString(L"RectFrame", L"Hight", d, lpPath);

	//[AgingSetting]
	SPRINTF(debugMode); WritePrivateProfileString(L"AgingSetting", L"DebugMode ", d, lpPath);
	//SPRINTF(agingSettingSaveRectImages); WritePrivateProfileString(L"AgingSetting", L"SaveRectImages ", d, lpPath);
	SPRINTF(agingTime); WritePrivateProfileString(L"AgingSetting", L"AgingTime ", d, lpPath);
	SPRINTF(intervalTime); WritePrivateProfileString(L"AgingSetting", L"IntervalTime", d, lpPath);
	SPRINTF(minContoursArea); WritePrivateProfileString(L"AgingSetting", L"MinContoursArea", d, lpPath);
	SPRINTF(minContoursSpace); WritePrivateProfileString(L"AgingSetting", L"MinContoursSpace", d, lpPath);
	SPRINTF(resetRect); WritePrivateProfileString(L"AgingSetting", L"ResetRectFrame", d, lpPath);
    SPRINTF(randomShutDownLed); WritePrivateProfileString(L"AgingSetting", L"RandomShutDownLedNum", d, lpPath);
	SPRINTF(shutdownTime); WritePrivateProfileString(L"AgingSetting", L"ShutDownDelayTime", d, lpPath);
	SPRINTF(thresoldBlockSize);  WritePrivateProfileString(L"AgingSetting", L"AdaptiveThresholdArgBlockSize", d, lpPath);
	SPRINTF(thresoldC);  WritePrivateProfileString(L"AgingSetting", L"AdaptiveThresholdArgC", d, lpPath);


	//[LED]
	SPRINTF(ledCount); WritePrivateProfileString(L"LED", L"Count", d, lpPath);
	SPRINTF(startColor); WritePrivateProfileString(L"LED", L"StartColor", d, lpPath);
	SPRINTF(stopColor); WritePrivateProfileString(L"LED", L"StopColor", d, lpPath);

	//[TrackBarWindow]
	SPRINTF(showTrackBarWnd); WritePrivateProfileString(L"TrackBarWindow", L"IsShow", d, lpPath);

	SPRINTF(bgrColorThres[BLUE]); WritePrivateProfileString(L"ThresholdB", L"t", d, lpPath);
	SPRINTF(bgrColorThres[GREEN]); WritePrivateProfileString(L"ThresholdG", L"t", d, lpPath);
	SPRINTF(bgrColorThres[RED]); WritePrivateProfileString(L"ThresholdR", L"t", d, lpPath);
	SPRINTF(bgrColorThres[WHITE]); WritePrivateProfileString(L"ThresholdW", L"t", d, lpPath);

	//[RedThreshold]
	//{156, 180, 159, 180, 43, 255, 149, 255, 46, 255, 148, 255} // Red
	SPRINTF(hsvColor[RED].h[2]); WritePrivateProfileString(L"RedThreshold", L"Lh", d, lpPath);
	SPRINTF(hsvColor[RED].h[3]); WritePrivateProfileString(L"RedThreshold", L"Hh", d, lpPath);
	SPRINTF(hsvColor[RED].s[2]); WritePrivateProfileString(L"RedThreshold", L"Ls", d, lpPath);
	SPRINTF(hsvColor[RED].s[3]); WritePrivateProfileString(L"RedThreshold", L"Hs", d, lpPath);
	SPRINTF(hsvColor[RED].v[2]); WritePrivateProfileString(L"RedThreshold", L"Lv", d, lpPath);
	SPRINTF(hsvColor[RED].v[3]); WritePrivateProfileString(L"RedThreshold", L"Hv", d, lpPath);

	//[GreenThreshold]
	//{35, 77, 35, 77, 43, 255, 43, 255, 46, 255, 136, 255}	// Green
	SPRINTF(hsvColor[GREEN].h[2]); WritePrivateProfileString(L"GreenThreshold", L"Lh", d, lpPath);
	SPRINTF(hsvColor[GREEN].h[3]); WritePrivateProfileString(L"GreenThreshold", L"Hh", d, lpPath);
	SPRINTF(hsvColor[GREEN].s[2]); WritePrivateProfileString(L"GreenThreshold", L"Ls", d, lpPath);
	SPRINTF(hsvColor[GREEN].s[3]); WritePrivateProfileString(L"GreenThreshold", L"Hs", d, lpPath);
	SPRINTF(hsvColor[GREEN].v[2]); WritePrivateProfileString(L"GreenThreshold", L"Lv", d, lpPath);
	SPRINTF(hsvColor[GREEN].v[3]); WritePrivateProfileString(L"GreenThreshold", L"Hv", d, lpPath);

	//[BlueThreshold]
	//{100, 124, 100, 124, 43, 255, 43, 255, 46, 255, 176, 255} // Blue
	SPRINTF(hsvColor[BLUE].h[2]); WritePrivateProfileString(L"BlueThreshold", L"Lh", d, lpPath);
	SPRINTF(hsvColor[BLUE].h[3]); WritePrivateProfileString(L"BlueThreshold", L"Hh", d, lpPath);
	SPRINTF(hsvColor[BLUE].s[2]); WritePrivateProfileString(L"BlueThreshold", L"Ls", d, lpPath);
	SPRINTF(hsvColor[BLUE].s[3]); WritePrivateProfileString(L"BlueThreshold", L"Hs", d, lpPath);
	SPRINTF(hsvColor[BLUE].v[2]); WritePrivateProfileString(L"BlueThreshold", L"Lv", d, lpPath);
	SPRINTF(hsvColor[BLUE].v[3]); WritePrivateProfileString(L"BlueThreshold", L"Hv", d, lpPath);

	//[WhiteThreshold]
	//{0,   180, 125, 180, 0,  30,  2,   30,  221, 255, 221, 255} // White
	SPRINTF(hsvColor[WHITE].h[2]); WritePrivateProfileString(L"WhiteThreshold", L"Lh", d, lpPath);
	SPRINTF(hsvColor[WHITE].h[3]); WritePrivateProfileString(L"WhiteThreshold", L"Hh", d, lpPath);
	SPRINTF(hsvColor[WHITE].s[2]); WritePrivateProfileString(L"WhiteThreshold", L"Ls", d, lpPath);
	SPRINTF(hsvColor[WHITE].s[3]); WritePrivateProfileString(L"WhiteThreshold", L"Hs", d, lpPath);
	SPRINTF(hsvColor[WHITE].v[2]); WritePrivateProfileString(L"WhiteThreshold", L"Lv", d, lpPath);
	SPRINTF(hsvColor[WHITE].v[3]); WritePrivateProfileString(L"WhiteThreshold", L"Hv", d, lpPath);

}

void ConfigData::readConfigFile()
{
	TCHAR lpPath[MAX_PATH] = { 0 };
	wcscpy_s(lpPath, MAX_PATH, L"./3c.ini");

	//[Camera]
	cameraIndex = GetPrivateProfileInt(L"Camera", L"Index", cameraIndex, lpPath);
	exposure = GetPrivateProfileInt(L"Camera", L"Exposure", exposure, lpPath);

	//[Frame]
	frame.width = GetPrivateProfileInt(L"Frame", L"Width", 1280, lpPath);
	frame.height = GetPrivateProfileInt(L"Frame", L"Hight", 780, lpPath);

	//[RectFrame]
	rect.x = GetPrivateProfileInt(L"RectFrame", L"X", 200, lpPath);
	rect.y = GetPrivateProfileInt(L"RectFrame", L"Y", 240, lpPath);
	rect.width = GetPrivateProfileInt(L"RectFrame", L"Width", 900, lpPath);
	rect.height = GetPrivateProfileInt(L"RectFrame", L"Hight", 200, lpPath);

	//[AgingSetting]
	debugMode = GetPrivateProfileInt(L"AgingSetting", L"DebugMode ", 0, lpPath);
	//agingSettingSaveRectImages = GetPrivateProfileInt(L"AgingSetting", L"SaveRectImages ", 1, lpPath);
	agingTime = GetPrivateProfileInt(L"AgingSetting", L"AgingTime ", 1, lpPath);
	intervalTime = GetPrivateProfileInt(L"AgingSetting", L"IntervalTime", 100, lpPath);
	minContoursArea = GetPrivateProfileInt(L"AgingSetting", L"MinContoursArea", 200, lpPath);
	minContoursSpace = GetPrivateProfileInt(L"AgingSetting", L"MinContoursSpace", 60, lpPath);
	resetRect = GetPrivateProfileInt(L"AgingSetting", L"ResetRectFrame", 0, lpPath);
    randomShutDownLed = GetPrivateProfileInt(L"AgingSetting", L"RandomShutDownLedNum", 200, lpPath);
	shutdownTime = GetPrivateProfileInt(L"AgingSetting", L"ShutDownDelayTime", 0, lpPath);
	thresoldBlockSize = GetPrivateProfileInt(L"AgingSetting", L"AdaptiveThresholdArgBlockSize", 101, lpPath);
	thresoldC = GetPrivateProfileInt(L"AgingSetting", L"AdaptiveThresholdArgC", -9, lpPath);

	//[LED]
	ledCount = GetPrivateProfileInt(L"LED", L"Count", 22, lpPath);
	startColor = (LEDColor)GetPrivateProfileInt(L"LED", L"StartColor", 0, lpPath);
	stopColor = (LEDColor)GetPrivateProfileInt(L"LED", L"StopColor", 4, lpPath);

	//[TrackBarWindow]
	showTrackBarWnd = GetPrivateProfileInt(L"TrackBarWindow", L"IsShow", 1, lpPath);

	bgrColorThres[BLUE] = GetPrivateProfileInt(L"ThresholdB", L"t", 150, lpPath);
	bgrColorThres[GREEN] = GetPrivateProfileInt(L"ThresholdG", L"t", 150, lpPath);
	bgrColorThres[RED] = GetPrivateProfileInt(L"ThresholdR", L"t", 150, lpPath);
	bgrColorThres[WHITE] = GetPrivateProfileInt(L"ThresholdW", L"t", 50, lpPath);

	//[RedThreshold]
	//{156, 180, 159, 180, 43, 255, 149, 255, 46, 255, 148, 255} // Red
	int r_Lh = GetPrivateProfileInt(L"RedThreshold", L"Lh", 159, lpPath);
	int r_Hh = GetPrivateProfileInt(L"RedThreshold", L"Hh", 180, lpPath);
	int r_Ls = GetPrivateProfileInt(L"RedThreshold", L"Ls", 149, lpPath);
	int r_Hs = GetPrivateProfileInt(L"RedThreshold", L"Hs", 255, lpPath);
	int r_Lv = GetPrivateProfileInt(L"RedThreshold", L"Lv", 148, lpPath);
	int r_Hv = GetPrivateProfileInt(L"RedThreshold", L"Hv", 255, lpPath);

	//[GreenThreshold]
	//{35, 77, 35, 77, 43, 255, 43, 255, 46, 255, 136, 255}	// Green
	int g_Lh = GetPrivateProfileInt(L"GreenThreshold", L"Lh", 35, lpPath);
	int g_Hh = GetPrivateProfileInt(L"GreenThreshold", L"Hh", 77, lpPath);
	int g_Ls = GetPrivateProfileInt(L"GreenThreshold", L"Ls", 43, lpPath);
	int g_Hs = GetPrivateProfileInt(L"GreenThreshold", L"Hs", 255, lpPath);
	int g_Lv = GetPrivateProfileInt(L"GreenThreshold", L"Lv", 136, lpPath);
	int g_Hv = GetPrivateProfileInt(L"GreenThreshold", L"Hv", 255, lpPath);

	//[BlueThreshold]
	//{100, 124, 100, 124, 43, 255, 43, 255, 46, 255, 176, 255} // Blue
	int b_Lh = GetPrivateProfileInt(L"BlueThreshold", L"Lh", 100, lpPath);
	int b_Hh = GetPrivateProfileInt(L"BlueThreshold", L"Hh", 124, lpPath);
	int b_Ls = GetPrivateProfileInt(L"BlueThreshold", L"Ls", 43, lpPath);
	int b_Hs = GetPrivateProfileInt(L"BlueThreshold", L"Hs", 255, lpPath);
	int b_Lv = GetPrivateProfileInt(L"BlueThreshold", L"Lv", 176, lpPath);
	int b_Hv = GetPrivateProfileInt(L"BlueThreshold", L"Hv", 255, lpPath);

	//[WhiteThreshold]
	//{0,   180, 125, 180, 0,  30,  2,   30,  221, 255, 221, 255} // White
	int w_Lh = GetPrivateProfileInt(L"WhiteThreshold", L"Lh", 125, lpPath);
	int w_Hh = GetPrivateProfileInt(L"WhiteThreshold", L"Hh", 180, lpPath);
	int w_Ls = GetPrivateProfileInt(L"WhiteThreshold", L"Ls", 2, lpPath);
	int w_Hs = GetPrivateProfileInt(L"WhiteThreshold", L"Hs", 30, lpPath);
	int w_Lv = GetPrivateProfileInt(L"WhiteThreshold", L"Lv", 221, lpPath);
	int w_Hv = GetPrivateProfileInt(L"WhiteThreshold", L"Hv", 255, lpPath);

	hsvColor[RED] = { 0, 180, r_Lh, r_Hh, 0, 255, r_Ls, r_Hs, 0, 255, r_Lv, r_Hv };
	hsvColor[GREEN] = { 0, 180, g_Lh, g_Hh, 0, 255, g_Ls, g_Hs, 0, 255, g_Lv, g_Hv };
	hsvColor[BLUE] = { 0, 180, b_Lh, b_Hh, 0, 255, b_Ls, b_Hs, 0, 255, b_Lv, b_Hv };
	hsvColor[WHITE] = { 0, 180, w_Lh, w_Hh, 0,  255, w_Ls, w_Hs, 0, 255, w_Lv, w_Hv };
}