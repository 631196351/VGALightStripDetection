//#include <vector>
#include <string>
#include "ErrorCode.h"

ErrorCode::ErrorCode(int code, const std::string& err):_code(code),_error(err)
{
	formatErrorMsg();
}

//ErrorCode::ErrorCode(int error, std::string msg) : _code(error), _msg(msg)
//{
//}

ErrorCode::ErrorCode(const ErrorCode& other)
{
	this->_code = other._code;
	this->_error = other._error;
	this->_func = other._func;
	this->_file = other._file;
	this->_line = other._line;
	formatErrorMsg();
}

ErrorCode::ErrorCode(const std::exception& e, int code) :_code(code)
{
	this->_error = e.what();
	formatErrorMsg();
}

ErrorCode::ErrorCode(const cv::Exception& e, int code) :_code(code)
{
	this->_func = e.func;
	this->_error = e.err;
	this->_file = e.file;
	this->_line = e.line;
	formatErrorMsg();
}

ErrorCode::ErrorCode(int code, const std::string& err, const std::string& file, const std::string& func, int line)
	:_code(code), _error(err), _func(func), _file(file), _line(line)
{
	formatErrorMsg();
}

ErrorCode::~ErrorCode()
{

}

void ErrorCode::formatErrorMsg()
{
	_msg = "Error code : ";
	_msg += std::to_string(_code);
	_msg += ". ";
	_msg += _error;
	if (!_func.empty()) 
	{
		_msg += ", Func : ";
		_msg += _func;
		_msg += ": ";
		_msg += std::to_string(_line);
	}
}
const char* ErrorCode::what() const noexcept
{
	return _msg.c_str();
}