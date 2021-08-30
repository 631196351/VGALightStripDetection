#include <fstream>
//#include <regex>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include "ConfigData.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
//#ifdef _DEBUG
//#include "I2CWrap.h"
//#endif
#if 0
#include <Windows.h>
#endif
#define SPRINTF(ARG) wsprintf(d, L"%d", ARG);

ConfigData::ConfigData()
{
#if false
	readConfigFile();
#endif
}


ConfigData::~ConfigData()
{
#if false
	if (_dirty)
	{
		saveConfigData();
	}
#endif
}
#if 0
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
	//SPRINTF(_minContoursSpace2); WritePrivateProfileString(L"AlgorithmThreshold", L"MinContoursSpace2", d, lpPath);
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
	_ledContoursArea = GetPrivateProfileInt(L"AlgorithmThreshold", L"LedContoursArea", _ledContoursArea, lpPath);
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
#endif

//void ConfigData::rect(cv::Rect& r)
//{
//	/// 检查ROI区域是否超出画面宽高
//	_rect = r & cv::Rect(0, 0, _frame.width, _frame.height);
//	//_rect = r;
//	//_dirty = true;
//}

void ConfigData::rect(cv::Rect roi[][CaptureNum], int colors)
{
	int w = 0;
	int h = 0;
	for (int i = 0; i < CaptureNum; ++i)
	{
		_roi[i] = ((roi[BLUE][i] | roi[GREEN][i]) & roi[RED][i]) & cv::Rect(0, 0, _frame.width, _frame.height * CaptureNum);
		SPDLOG_SINKS_DEBUG("ConfigData ROI {}th : x:{},y:{}, width:{}, height:{}", i, _roi[i].x, _roi[i].y, _roi[i].width, _roi[i].height);
		w = cv::max(w, _roi[i].width);
		h += _roi[i].height;
	}

	_rect2 = cv::Rect(0, 0, w, h);
	SPDLOG_SINKS_DEBUG("ConfigData rect2 : x:{},y:{}, width:{}, height:{}", _rect2.x, _rect2.y, _rect2.width, _rect2.height);
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

void ConfigData::readConfigFile(std::string model, unsigned led_count)
{
	model.assign(model.begin() + 4, model.end());

	if (model.find("STRIX") != std::string::npos)
	{
		if (model.find("WHITE") != std::string::npos)
		{
			//ROG-STRIX-RTX3070-O8G-GAMING-WHITE-2I3S
			_thermo_name = "STRIX-WHITE-" + std::to_string(led_count);
		}
		else if (model.find("GUNDAM") != std::string::npos) 
		{
			//ROG-STRIX-RTX3080-O10G-GUNDAM-2I3S
			_thermo_name = "STRIX-GUNDAM-" + std::to_string(led_count);
		}
		else if (model.find("GAMING") != std::string::npos)
		{
			//ROG-STRIX-RTX3080-O10G-GAMING-2I3S
			_thermo_name = "STRIX-GAMING-" + std::to_string(led_count);
		}
	}
	else if (model.find("TUF") != std::string::npos)
	{
		//TUF-RTX3090-O24G-2I3S
		//TUF-RTX3090-24G-2I3S-PD
		//TUF-RTX3060-12G-GAMING-2I3S
		_thermo_name = "TUF-" + std::to_string(led_count);
	}
	else if (model.find("KO") != std::string::npos)
	{
		_thermo_name = "KO-" + std::to_string(led_count);
	}

	std::ifstream in("3c.json");
	if (!in.is_open())
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw ErrorCodeEx(ERR_OPEN_CONFIG_FILE, "Open 3c.json file error!");
	}

	std::string json_string;
	json_string.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
	in.close();
	rapidjson::Document dom;
	if (!dom.Parse(json_string.c_str()).HasParseError())
	{
		if (dom.HasMember("AgingSetting") && dom["AgingSetting"].IsObject())
		{
			const auto& asg = dom["AgingSetting"];
			_frame.width = asg["Width"].GetInt();
			_frame.height = asg["Hight"].GetInt();
			_skipFrame = asg["SkipFrame"].GetInt();
			_startColor = (LEDColor)asg["StartColor"].GetInt();
			_stopColor = (LEDColor)asg["StopColor"].GetInt();
			_randomShutDownLed = asg["RandomShutDownLedNum"].GetInt();
			_shutdownTime = asg["ShutDownDelayTime"].GetInt();
			_recheckFaileLedTime = asg["RecheckFaileLedTime"].GetInt();
		}

		if (dom.HasMember(_thermo_name.c_str()) && dom[_thermo_name.c_str()].IsObject())
		{
			const auto& thermo = dom[_thermo_name.c_str()];
			if (thermo.HasMember("Camera") && thermo["Camera"].IsObject())
			{
				const auto& cam = thermo["Camera"];
				//_cameraIndex = cam["Index"].GetInt();
				_exposure = cam["Exposure"].GetInt();
				_saturation = cam["Saturation"].GetInt();
				
			}
			if (thermo.HasMember("AlgorithmThreshold") && thermo["AlgorithmThreshold"].IsObject())
			{
				const auto& atd = thermo["AlgorithmThreshold"];
				_intervalTime = atd["IntervalTime"].GetInt();
				_minContoursArea = atd["MinContoursArea"].GetInt();
				//_minContoursSpace = atd["MinContoursSpace"].GetInt();
				//_minROIContoursArea = atd["MinROIContoursArea"].GetInt();
				_ledContoursArea = atd["LedContoursArea"].GetInt();
				_thresoldBlockSize = atd["AdaptiveThresholdArgBlockSize"].GetInt();
				_thresoldC = atd["AdaptiveThresholdArgC"].GetInt();

				//const auto& colors = atd["ColorThres"];
				//for (unsigned i = 0; i < colors.Size(); ++i)
				//{
				//	_bgrColorThres[i] = colors[i].GetInt();
				//}
				//
				//const auto& colors_percent = atd["ColorPercentage"];
				//for (unsigned i = 0; i < colors_percent.Size(); ++i)
				//{
				//	_bgrColorPercentage[i] = colors_percent[i].GetDouble();
				//}

				/// Opencv HSV Color
				/// H ∈ [0, 180]
				/// S ∈ [0，255]
				/// V ∈ [0，255]
				///http://color.lukas-stratmann.com/color-systems/hsv.html

				const auto& hsv = atd["HSV"];
				const auto& hsv_b = hsv["b"];
				for (unsigned i = 0; i < hsv_b.Size(); ++i)
				{
					_hsvColor[BLUE][i] = hsv_b[i].GetFloat();
				}
				const auto& hsv_g = hsv["g"];
				for (unsigned i = 0; i < hsv_g.Size(); ++i)
				{
					_hsvColor[GREEN][i] = hsv_g[i].GetFloat();
				}
				const auto& hsv_r = hsv["r"];
				for (unsigned i = 0; i < hsv_r.Size(); ++i)
				{
					_hsvColor[RED][i] = hsv_r[i].GetFloat();
				}
			}
		}
		else
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			throw ErrorCodeEx(ERR_CONFIG_DATA_NOT_EXIST, "3c.json中未找到当前机种的配置数据");
		}
	}
	else
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		SPDLOG_SINKS_ERROR("Parse 3c.json file error! offset {}, {}", dom.GetErrorOffset(), rapidjson::GetParseError_En(dom.GetParseError()));
		throw ErrorCodeEx(ERR_PARSE_JSON_SYNTAX, "Parse 3c.json file error");
	}

	recordConfig2WorkStates();
}

void ConfigData::recordConfig2WorkStates()
{
	SPDLOG_SINKS_INFO("AgingSetting");
	SPDLOG_SINKS_INFO("\t startColor : {}", _startColor);
	SPDLOG_SINKS_INFO("\t stopColor : {}", _stopColor);
	SPDLOG_SINKS_INFO("\t randomShutDownLed : {}", _randomShutDownLed);
	SPDLOG_SINKS_INFO("\t shutdownTime : {}", _shutdownTime);
	SPDLOG_SINKS_INFO("\t recheckFaileLedTime : {}", _recheckFaileLedTime);

	SPDLOG_SINKS_INFO("\t Thermo : {}", _thermo_name);
	SPDLOG_SINKS_INFO("\t Exposure : {}", _exposure);
	SPDLOG_SINKS_INFO("\t Saturation : {}", _saturation);
	SPDLOG_SINKS_INFO("\t SkipFrame : {}", _skipFrame);
	SPDLOG_SINKS_INFO("\t Width : {}", _frame.width);
	SPDLOG_SINKS_INFO("\t Hight : {}", _frame.height);

	SPDLOG_SINKS_INFO("\t IntervalTime : {}", _intervalTime);
	SPDLOG_SINKS_INFO("\t MinContoursArea : {}", _minContoursArea);
	SPDLOG_SINKS_INFO("\t LedContoursArea : {}", _ledContoursArea);

	SPDLOG_SINKS_INFO("\t AdaptiveThresholdArgBlockSize : {}", _thresoldBlockSize);
	SPDLOG_SINKS_INFO("\t AdaptiveThresholdArgC : {}", _thresoldC);
	SPDLOG_SINKS_INFO("\t HSV-Blue: {}≤ h ≤{}， {}≤s≤255, {}≤v≤255", _hsvColor[BLUE][0], _hsvColor[BLUE][1], _hsvColor[BLUE][2], _hsvColor[BLUE][3]);
	SPDLOG_SINKS_INFO("\t HSV-Greem: {}≤ h ≤{}， {}≤s≤255, {}≤v≤255", _hsvColor[GREEN][0], _hsvColor[GREEN][1], _hsvColor[GREEN][2], _hsvColor[GREEN][3]);
	SPDLOG_SINKS_INFO("\t HSV-Red: {}≤ h1 ≤{} ∪ {}≤ h2 ≤{}， {}≤s≤255, {}≤v≤255", _hsvColor[RED][0], _hsvColor[RED][1], _hsvColor[RED][4], _hsvColor[RED][5], _hsvColor[RED][2], _hsvColor[RED][3]);
}