#pragma once
#include <exception>
#include <opencv2/opencv.hpp>

enum eError
{
	ERR_All_IS_WELL,				// IT'S OK
	ERR_SOME_LED_FAILURE,			// ����LED���޷��ҵ�����
	ERR_CRITICAL_LEVEL_0 = 1000,	// Terminate AP


	ERR_STD_EXCEPTION,				// C++ ��׼�쳣
	ERR_SPDLOG_EXCEPTION,			// Spdlog �쳣
	ERR_OPENCV_RUNTIME_EXCEPTION,	// Opencv Runtime�쳣

	ERR_CANT_OPEN_CAMERA,			// ������쳣
	ERR_LOAD_I2C_FAILURE,			// I2C�����쳣
	ERR_RUN_I2C_FAILURE,			// I2Cִ���쳣


	ERR_CRITICAL_LEVEL_1 = 2000,	// NOT Terminate AP 
	ERR_ORIGIN_FRAME_EMPTY_EXCEPTION,	//��ǰ֡��֡�쳣	
	ERR_LED_STRIPE_BLOCKED,			// �Զ���ȡROIʱ�����ֵƴ����ڵ�
	ERR_POSTRUE_CORRECTION_ERROR,	// ������Կ�λ��ƫ�룬��Ҫ���µ�����̬
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

