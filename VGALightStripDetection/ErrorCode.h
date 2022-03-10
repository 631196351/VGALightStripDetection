#pragma once
#include <exception>
#include <opencv2/opencv.hpp>

enum eError
{
	ERR_All_IS_WELL,				// IT'S OK
	ERR_SOME_LED_FAILURE,			// 部分LED灯无法找到轮廓
	ERR_CRITICAL_LEVEL_0 = 1000,	// Terminate AP


	ERR_STD_EXCEPTION,				// C++ 标准异常
	ERR_SPDLOG_EXCEPTION,			// Spdlog 异常
	ERR_OPENCV_RUNTIME_EXCEPTION,	// Opencv Runtime异常

	ERR_CANT_OPEN_CAMERA,			// 打开相机异常
	ERR_LOAD_I2C_FAILURE,			// I2C载入异常
	ERR_RUN_I2C_FAILURE,			// I2C执行异常
	ERR_INCOMPLETE_ARGS,				// 参数不全
	ERR_COMMAND_LINE_ARGS,			// 命令行参数异常
	ERR_OPEN_CONFIG_FILE,			// 配置文件无法打开
	ERR_PARSE_JSON_SYNTAX,			// 配置文件解析错误
	ERR_CONFIG_DATA_NOT_EXIST,		// 配置文件中没有该机种的配置参数
	ERR_GPU_LOAD_FAILURE,			// Vender load GPU 失败
	ERR_CAMERA_NOT_MATCH_JSON_FILE,	// 相机设备名称与3c.json不匹配

	ERR_CRITICAL_LEVEL_1 = 2000,	// NOT Terminate AP 
	ERR_ORIGIN_FRAME_EMPTY_EXCEPTION,	//当前帧空帧异常	
	ERR_LED_STRIPE_BLOCKED,			// 自动获取ROI时，发现灯带被遮挡
	ERR_POSTRUE_CORRECTION_ERROR,	// 相机或显卡位置偏离，需要重新调整姿态
	ERR_MISSING_FRAME,				// 抓到的帧数小于相机个数
};

class ErrorCode : public std::exception
{
	int _code;///< error code
	std::string _msg;///< the formatted error message

	std::string _error; ///< error description
	std::string _func; ///< function name. Available only when the compiler supports getting it
	std::string _file; ///< source file name where the error has occurred
	int _line; ///< line number in the source file where the error has occurred

public:
	ErrorCode(int code, const std::string& err);
	//ErrorCode(int error, std::string msg);
	ErrorCode(const ErrorCode& other);
	ErrorCode(const std::exception& e, int code);
	ErrorCode(const cv::Exception& e, int code);
	ErrorCode(int code, const std::string& err, const std::string& file, const std::string& func, int line);
	~ErrorCode();

	virtual const char* what() const;
	inline int error() const { return _code; }

private:
	void formatErrorMsg();
};

#define ErrorCodeEx(code, error) ErrorCode(code, error, __FILE__, __FUNCTION__, __LINE__)

