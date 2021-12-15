#include <fstream>
#include <regex>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include "ConfigData.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
#ifdef _DEBUG
#include "I2CWrap.h"
#endif
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

void ConfigData::readConfigFile(std::string model, unsigned led_count)
{
	//PPIDROG-STRIX-RTX3090-O24G-GUNDAM-2I3S
	//PPIDTUF-RTX3070TI-8G-GAMING-2I3S
	//model = "PPIDROG-KO-RTX3090-O24G-GUNDAM-2I3S";
	//model.assign(model.begin() + 4, model.end());
	SPDLOG_SINKS_INFO("ModelName:{}, led:{}", model, led_count);

	if (model.find("STRIX") != std::string::npos)
	{
		if (model.find("WHITE") != std::string::npos)
		{
			//ROG-STRIX-RTX3070-O8G-GAMING-WHITE-2I3S
			_thermal_name = "STRIX-WHITE-" + std::to_string(led_count);
		}
		else if (model.find("GUNDAM") != std::string::npos)
		{
			//ROG-STRIX-RTX3080-O10G-GUNDAM-2I3S
			_thermal_name = "STRIX-GUNDAM-" + std::to_string(led_count);
		}
		else
		{
			//ROG-STRIX-RX6800-O16G-I3S
			_thermal_name = "STRIX";
		}
	}
	else if (model.find("TUF") != std::string::npos)
	{
		//TUF-RTX3090-O24G-2I3S
		//TUF-RTX3090-24G-2I3S-PD
		//TUF-RTX3060-12G-GAMING-2I3S
		//TUF-RX6800XT-O16G-I3S
		_thermal_name = "TUF";
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
			_cameraFps = asg["FPS"].GetInt();
			_startColor = (LEDColor)asg["StartColor"].GetInt();
			_stopColor = (LEDColor)asg["StopColor"].GetInt();
			_skipFrame = asg["SkipFrame"].GetInt();
			_intervalTime = asg["IntervalTime"].GetInt();
			_randomShutDownLed = asg["RandomShutDownLedNum"].GetInt();
			_shutdownTime = asg["ShutDownDelayTime"].GetInt();
			_recheckFaileLedTime = asg["RecheckFaileLedTime"].GetInt();
			_keepDebugImg = asg["KeepDebugImg"].GetBool();
		}

		if (dom.HasMember(_thermal_name.c_str()) && dom[_thermal_name.c_str()].IsObject())
		{
			const auto& thermo = dom[_thermal_name.c_str()];
			if (thermo.HasMember("Camera") && thermo["Camera"].IsObject())
			{
				const auto& cam = thermo["Camera"];
				_cameraIndex = cam["Index"].GetInt();
				_exposure = cam["Exposure"].GetInt();
				_saturation = cam["Saturation"].GetInt();
			}
			if (thermo.HasMember("AlgorithmThreshold") && thermo["AlgorithmThreshold"].IsObject())
			{
				const auto& atd = thermo["AlgorithmThreshold"];
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

				//const auto& colors_percent = atd["ColorPercentage"];
				//for (unsigned i = 0; i < colors_percent.Size(); ++i)
				//{
				//	_bgrColorPercentage[i] = colors_percent[i].GetDouble();
				//}


				/// http://color.lukas-stratmann.com/color-systems/hsv.html
				/// https://blog.csdn.net/timidsmile/article/details/17297811
				/// 在配置中 H∈[0, 360], S∈[0, 255], V∈[0, 255]
				/// 在OpenCV中 H∈[0, 180], S∈[0, 255], V∈[0, 255]
				const auto& hsv = atd["HSV"];
				const auto& hsv_b = hsv["b"];
				for (unsigned i = 0; i < hsv_b.Size(); ++i)
				{
					_hsv[BLUE][i] = hsv_b[i].GetFloat();
				}
				const auto& hsv_g = hsv["g"];
				for (unsigned i = 0; i < hsv_g.Size(); ++i)
				{
					_hsv[GREEN][i] = hsv_g[i].GetFloat();
				}
				const auto& hsv_r = hsv["r"];
				for (unsigned i = 0; i < hsv_r.Size(); ++i)
				{
					_hsv[RED][i] = hsv_r[i].GetFloat();
				}

				_hsv[BLUE][eHmin] *= 0.5f;
				_hsv[BLUE][eHmax] *= 0.5f;
				_hsv[GREEN][eHmin] *= 0.5f;
				_hsv[GREEN][eHmax] *= 0.5f;
				_hsv[RED][eHmin] *= 0.5f;
				_hsv[RED][eHmax] *= 0.5f;
				_hsv[RED][eHmin2] *= 0.5f;
				_hsv[RED][eHmax2] *= 0.5f;

				/// 这两个用于在获取灯带ROI期间使用S 和 V
				const auto& hsv_roi = hsv["roi"];
				_hsvROI[0] = hsv_roi[0].GetFloat();
				_hsvROI[1] = hsv_roi[1].GetFloat();
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
	SPDLOG_SINKS_INFO("\t KeepDebugImg : {}", _keepDebugImg);

	SPDLOG_SINKS_INFO("\t Thermal : {}", _thermal_name);
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
	SPDLOG_SINKS_INFO("\t HSV-Blue: {}≤ h ≤{},{}≤s≤255,{}≤v≤255", _hsv[BLUE][eHmin], _hsv[BLUE][eHmax], _hsv[BLUE][eSmin], _hsv[BLUE][eVmin]);
	SPDLOG_SINKS_INFO("\t HSV-Greem: {}≤ h ≤{},{}≤s≤255,{}≤v≤255", _hsv[GREEN][eHmin], _hsv[GREEN][eHmax], _hsv[GREEN][eSmin], _hsv[GREEN][eVmin]);
	SPDLOG_SINKS_INFO("\t HSV-Red: {}≤ h1 ≤{}∪{}≤ h2 ≤{},{}≤s≤255,{}≤v≤255", _hsv[RED][eHmin], _hsv[RED][eHmax], _hsv[RED][eHmin2], _hsv[RED][eHmax2], _hsv[RED][eSmin], _hsv[RED][eVmin]);
	SPDLOG_SINKS_INFO("\t HSV-ROI: {}≤s≤255,{}≤v≤255", _hsvROI[0], _hsvROI[1]);
}