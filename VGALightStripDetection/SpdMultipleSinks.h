#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>

class SpdMultipleSinks
{
public:
	SpdMultipleSinks();
	~SpdMultipleSinks();
	
	template<typename... Args>
	void log(spdlog::source_loc loc, spdlog::level::level_enum lvl, spdlog::string_view_t fmt, Args&&...args)
	{
		_logger.log(loc, lvl, fmt, std::forward<Args>(args)...);
	}

	void pushBasicFileSinkMT(const char* path);
	void popupLastBasicFileSinkMT();

	static SpdMultipleSinks& sinks();
private:
	void initSpdlog();
	

private:
	spdlog::logger _logger;
};
#define SinkInstance SpdMultipleSinks::sinks()
#define SPDLOG_SINKS_TRACE(fmt, ...) SinkInstance.log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::trace, fmt, __VA_ARGS__)
#define SPDLOG_SINKS_DEBUG(fmt, ...) SinkInstance.log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, fmt, __VA_ARGS__)
#define SPDLOG_SINKS_INFO(fmt, ...) SinkInstance.log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, fmt, __VA_ARGS__)
#define SPDLOG_SINKS_WARN(fmt, ...) SinkInstance.log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, fmt, __VA_ARGS__)
#define SPDLOG_SINKS_ERROR(fmt, ...) SinkInstance.log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, fmt, __VA_ARGS__)
#define SPDLOG_SINKS_CRITICAL(fmt, ...) SinkInstance.log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical, fmt, __VA_ARGS__)

