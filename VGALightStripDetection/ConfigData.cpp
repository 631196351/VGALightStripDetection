#include <fstream>
//#include <regex>
//#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include "ConfigData.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
#include "CaptureDevices.h"

ConfigData::ConfigData()
{
}

ConfigData::~ConfigData()
{
}

void ConfigData::rect(ColorROI& roi)
{
	int w = 0;
	int h = 0;
	//int size = kCameraDevices.size();
	//for (int i = 0; i < size; ++i)
	FacadeROI& facade = roi[BLUE];
	for(int i = 0; i < facade.size(); ++i)
	{
		// 将多颜色下， 抓到的同一立面的ROI进行合并
		// 若kCameraDevices.size() == 1， 表示只有1个立面，基本上是Top 立面
		// 将BGR三色下， 抓到的Top立面的ROI进行合并
		// 若kCameraDevices.size() == 2， 表示只有2个立面，基本上是Top 立面和Rear 立面
		// 将BGR三色下， 抓到的Top立面和Rear立面的ROI分别进行合并
		// 若kCameraDevices.size() == 3， 表示只有3个立面，基本上是Top 立面，Rear 立面， Front 立面
		// 将BGR三色下， 抓到的Top立面，Rear立面，Front 立面的ROI分别进行合并

		_roi[i] = ((roi[BLUE][i] | roi[GREEN][i]) & roi[RED][i]) & cv::Rect(0, 0, _frame.width, _frame.height * facade.size());
		SPDLOG_SINKS_DEBUG("ConfigData ROI {}th : x:{},y:{}, width:{}, height:{}", i, _roi[i].x, _roi[i].y, _roi[i].width, _roi[i].height);
		w = cv::max(w, _roi[i].width);
		h += _roi[i].height;
	}

	//_rect2 = cv::Rect(0, 0, w, h);
	//SPDLOG_SINKS_DEBUG("ConfigData rect2 : x:{},y:{}, width:{}, height:{}", _rect2.x, _rect2.y, _rect2.width, _rect2.height);
}

void ConfigData::shutdownTime(int t)
{
	_shutdownTime = t;
}

ConfigData& ConfigData::instance()
{
	static ConfigData instance;
	return instance;
}

void ConfigData::readConfigFile(std::string model, unsigned led_count)
{
	//model.assign(model.begin() + 4, model.end());
	SPDLOG_SINKS_INFO("model : {}, led_count:{}", model, led_count);

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
			_thermo_name = "STRIX-" + std::to_string(led_count);
		}
		else if (model.find("LC") != std::string::npos)
		{
			//ROG-STRIX-LC-RTX6800-O16G-UI2S
			_thermo_name = "STRIX-LC-" + std::to_string(led_count);
		}
		else if (model.find("EVA") != std::string::npos)
		{
			//ROG-STRIX-RX6800-O16G-I3S
			_thermo_name = "STRIX-" + std::to_string(led_count);
		}
		else if (model.find("RX") != std::string::npos)
		{
			//ROG-STRIX-RX6800-O16G-I3S  ROG-STRIX-RX6750-O12G-I3S  ROG-STRIX-RX6600-O12G-I3S  ROG-STRIX-RX6650-O12G-I3S
			_thermo_name = "STRIX-" + std::to_string(led_count);
		}		
	}
	else if (model.find("TUF") != std::string::npos)
	{
		//TUF-RTX3090-O24G-2I3S
		//TUF-RTX3090-24G-2I3S-PD
		//TUF-RTX3060-12G-GAMING-2I3S
		_thermo_name = "TUF";
	}
	else if (model.find("KO") != std::string::npos)
	{
		_thermo_name = "KO";
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
			_skipFrame = asg["SkipFrame"].GetInt();
			_startColor = (LEDColor)asg["StartColor"].GetInt();
			_stopColor = (LEDColor)asg["StopColor"].GetInt();
			_intervalTime = asg["IntervalTime"].GetInt();
			_randomShutDownLed = asg["RandomShutDownLedNum"].GetInt();
			_shutdownTime = asg["ShutDownDelayTime"].GetInt();
			_recheckFaileLedTime = asg["RecheckFaileLedTime"].GetInt();

			const auto& name = asg["VideoCapName"];
			for (unsigned i = 0; i < name.Size(); ++i)
			{
				_videoCapName.push_back(name[i].GetString());
			}

			_keepDebugImg = asg["KeepDebugImg"].GetBool();
		}

		if (dom.HasMember(_thermo_name.c_str()) && dom[_thermo_name.c_str()].IsObject())
		{
			const auto& thermo = dom[_thermo_name.c_str()];
			if (thermo.HasMember("Camera") && thermo["Camera"].IsObject())
			{
				const auto& cam = thermo["Camera"];
				//_cameraIndex = cam["Index"].GetInt();
				//_exposure = cam["Exposure"].GetInt();
				const auto& exposures = cam["Exposure"];
				for (unsigned i = 0; i < exposures.Size(); ++i)
				{
					_exposures.push_back(exposures[i].GetInt());
				}

				_saturation = cam["Saturation"].GetInt();
				
				// 虽然笨拙，但胜在兼容各种机种
				const auto& front = cam["Front"];
				for (unsigned i = 0; i < front.Size(); ++i)
				{
					_front.insert(front[i].GetInt() - 1);
				}

				const auto& rear = cam["Rear"];
				for (unsigned i = 0; i < rear.Size(); ++i)
				{
					_rear.insert(rear[i].GetInt() - 1);
				}

				const auto& overhead = cam["Overhead"];
				for (unsigned i = 0; i < overhead.Size(); ++i)
				{
					_overhead.insert(overhead[i].GetInt() - 1);
				}
			}
			if (thermo.HasMember("AlgorithmThreshold") && thermo["AlgorithmThreshold"].IsObject())
			{
				const auto& atd = thermo["AlgorithmThreshold"];
				
				_minContoursArea = atd["MinContoursArea"].GetInt();
				_ledContoursArea = atd["LedContoursArea"].GetInt();
				_thresoldBlockSize = atd["AdaptiveThresholdArgBlockSize"].GetInt();
				_thresoldC = atd["AdaptiveThresholdArgC"].GetInt();

				/// Opencv HSV Color
				/// H ∈ [0, 180]
				/// S ∈ [0，255]
				/// V ∈ [0，255]
				///http://color.lukas-stratmann.com/color-systems/hsv.html

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
	std::string msg;
	SPDLOG_SINKS_INFO("AgingSetting");
	SPDLOG_SINKS_INFO("\t startColor : {}", _startColor);
	SPDLOG_SINKS_INFO("\t stopColor : {}", _stopColor);
	SPDLOG_SINKS_INFO("\t randomShutDownLed : {}", _randomShutDownLed);
	SPDLOG_SINKS_INFO("\t shutdownTime : {}", _shutdownTime);
	SPDLOG_SINKS_INFO("\t recheckFaileLedTime : {}", _recheckFaileLedTime);

	SPDLOG_SINKS_INFO("\t Thermo : {}", _thermo_name);
	//SPDLOG_SINKS_INFO("\t Exposure : {}", _exposure);
	for (auto it = _exposures.begin(); it != _exposures.end(); ++it)
	{
		msg += std::to_string(*it);
		msg += ",";
	}
	SPDLOG_SINKS_INFO("\t Exposures : {}", msg);
	msg.clear();

	SPDLOG_SINKS_INFO("\t Saturation : {}", _saturation);
	SPDLOG_SINKS_INFO("\t SkipFrame : {}", _skipFrame);
	SPDLOG_SINKS_INFO("\t Width : {}", _frame.width);
	SPDLOG_SINKS_INFO("\t Hight : {}", _frame.height);
	SPDLOG_SINKS_INFO("\t FPS : {}", _cameraFps);
	
	for (auto it = _front.begin(); it != _front.end(); ++it)
	{
		msg += std::to_string(*it);
		msg += ",";
	}
	SPDLOG_SINKS_INFO("\t Front : {}", msg);
	msg.clear();

	for (auto it = _rear.begin(); it != _rear.end(); ++it)
	{
		msg += std::to_string(*it);
		msg += ",";
	}
	SPDLOG_SINKS_INFO("\t Rear : {}", msg);
	msg.clear();

	for (auto it = _overhead.begin(); it != _overhead.end(); ++it)
	{
		msg += std::to_string(*it);
		msg += ",";
	}
	SPDLOG_SINKS_INFO("\t Overhead : {}", msg);

	SPDLOG_SINKS_INFO("\t IntervalTime : {}", _intervalTime);
	SPDLOG_SINKS_INFO("\t MinContoursArea : {}", _minContoursArea);
	SPDLOG_SINKS_INFO("\t LedContoursArea : {}", _ledContoursArea);

	SPDLOG_SINKS_INFO("\t AdaptiveThresholdArgBlockSize : {}", _thresoldBlockSize);
	SPDLOG_SINKS_INFO("\t AdaptiveThresholdArgC : {}", _thresoldC);
	SPDLOG_SINKS_INFO("\t HSV-Blue: {}<= h <={}， {}<=s<=255, {}<=v<=255", _hsv[BLUE][0], _hsv[BLUE][1], _hsv[BLUE][2], _hsv[BLUE][3]);
	SPDLOG_SINKS_INFO("\t HSV-Greem: {}<= h <={}， {}<=s<=255, {}<=v<=255", _hsv[GREEN][0], _hsv[GREEN][1], _hsv[GREEN][2], _hsv[GREEN][3]);
	SPDLOG_SINKS_INFO("\t HSV-Red: {}<= h1 <={} ∪ {}<= h2 <={}， {}<= s <=255, {}<= v <=255", _hsv[RED][0], _hsv[RED][1], _hsv[RED][4], _hsv[RED][5], _hsv[RED][2], _hsv[RED][3]);
	SPDLOG_SINKS_INFO("\t HSV-ROI: {}<=s<=255,{}<=v<=255", _hsvROI[0], _hsvROI[1]);
}

/// 在此规定 3c.json 中 AgingSetting::VideoCapName 下所列值依次为 [上， 后， 前]
int ConfigData::ledIndexToCamera(int led_index)
{
	if (_overhead.find(led_index) != _overhead.end())
	{
		SPDLOG_SINKS_DEBUG("led_index : {}, camera : {}", led_index, _videoCapName[0]);
		return kCameraDevices.cameraIndex(_videoCapName[0]);
	}
	else if (_rear.find(led_index) != _rear.end())
	{
		SPDLOG_SINKS_DEBUG("led_index : {}, camera : {}", led_index, _videoCapName[1]);
		return kCameraDevices.cameraIndex(_videoCapName[1]);
	}
	else if (_front.find(led_index) != _front.end())
	{
		SPDLOG_SINKS_DEBUG("led_index : {}, camera : {}", led_index, _videoCapName[2]);
		return kCameraDevices.cameraIndex(_videoCapName[2]);
	}
	return -1;
}

int ConfigData::exposure(const std::string& cap)
{
	if (cap.compare(_videoCapName[0]) == 0)
	{
		return _exposures[0];
	}
	else if (cap.compare(_videoCapName[1]) == 0)
	{
		return _exposures[1];
	}
	else if (cap.compare(_videoCapName[2]) == 0)
	{
		return _exposures[2];
	}
	else
		throw std::exception("Can't not find correct exposure");
}
/// 根据相机设备名称, 从配置档中获取对应的灯珠数量
/// 若相机对应的灯珠数量为0, 则返回false, 表示不需要打开该相机
/// 若相机对应的灯珠数量>0, 则返回true, 表示需要打开该相机
bool ConfigData::captureOpenState(const std::string& capture_name)
{
	if (capture_name.compare(_videoCapName[0]) == 0)
	{
		//true if the container is empty, false otherwise
		return !_overhead.empty();
	}
	else if (capture_name.compare(_videoCapName[1]) == 0)
	{
		return !_rear.empty();
	}
	else if (capture_name.compare(_videoCapName[2]) == 0)
	{
		return !_front.empty();
	}
	return true;
}