#include "SpdMultipleSinks.h"	// 需要放置在第一行
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "PreDefine.h"
#include "ErrorCode.h"

using namespace spdlog;

const char* lpatten = "%^[%Y-%m-%d %H:%M:%S %e] [thread %t] [%l - %#] [%o] %v%$";

SpdMultipleSinks::SpdMultipleSinks():_logger("LOG")
{
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	_logger.sinks().push_back(console_sink);

	// Create a file rotating logger with 5mb size max and 3 rotated files
	//auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/rotating.txt", 1024 * 1024 * 5, 3);
	//_logger.sinks().push_back(rotating_sink);

#ifdef  _DEBUG
	auto mscv_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
	_logger.sinks().push_back(mscv_sink);
#endif

	_logger.set_pattern(lpatten);
	_logger.set_level(spdlog::level::trace);

	_logger.set_error_handler([](const std::string& msg) {		
		throw ErrorCodeEx(ERR_SPDLOG_EXCEPTION, msg);
	});

	// 及时flush 数据到log中
	// 避免程序一开始跑就被abort后，出现 log 文件为空的情况
	_logger.flush_on(spdlog::level::trace);
}

SpdMultipleSinks::~SpdMultipleSinks()
{
	;
}

void SpdMultipleSinks::pushBasicFileSinkMT(const char* path)
{
	char b[MAX_PATH] = { 0 };
	sprintf_s(b, MAX_PATH, "%s/%s/work_stats.txt", AgingFolder, path);
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(b, true);
	file_sink->set_pattern(lpatten);
	file_sink->set_level(spdlog::level::trace);
	_logger.sinks().push_back(file_sink);
}

void SpdMultipleSinks::popupLastBasicFileSinkMT()
{
	_logger.sinks().pop_back();
}

void SpdMultipleSinks::addPPID2FileSinkMT(const char* ppid)
{
	pushBasicFileSinkMT(ppid);
}

SpdMultipleSinks& SpdMultipleSinks::sinks()
{
	static SpdMultipleSinks g_sinks;
	return g_sinks;
}