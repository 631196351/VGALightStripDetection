#include <regex>
#include "I2CWrap.h"
#include "RandomLitoff.h"
#include "ErrorCode.h"
#include "SpdMultipleSinks.h"

//设定随机灭灯状态, 设定手动关灯列表
void RandomLitoff::setRandomLitOffState(int probability, std::string manualset)
{
	SPDLOG_SINKS_DEBUG("RandomLitOffState probability:{}, manualset:{}", probability, manualset);

	if (probability > 0 && !manualset.empty())
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw ErrorCodeEx(ERR_COMMAND_LINE_ARGS, "Random lit-off parameter configuration is repeated");
	}

	if (probability > 0 || !manualset.empty())
	{
		_bRlitOffState = true;
	}

	SPDLOG_SINKS_DEBUG("RandomLitOffState randomLightDown:{}", _bRlitOffState);

	if (!manualset.empty())
	{
		std::regex reg(",");		// 匹配split
		std::sregex_token_iterator pos(manualset.begin(), manualset.end(), reg, -1);
		decltype(pos) end;              // 自动推导类型
		for (; pos != end; ++pos)
		{
			auto it = _rand_set.insert(atoi(pos->str().c_str()));
			SPDLOG_SINKS_DEBUG("Lit-Off {}th Led", *it.first);
		}
	}

	// 直接在这里初始化好每颗灯的命运
	// eg: 22颗灯里面有【3,4,7,10,15】这几颗灯会随机灭掉，BGR都会灭掉
	if (probability > 0)
	{
		cv::RNG rng(time(NULL));
		for (int i = 0; i < I2C.getLedCount(); i++)
		{
			int r = rng.uniform(0, 101);	//[0, 101)
			if (probability >= r)
			{
				auto it = _rand_set.insert(i);
				SPDLOG_SINKS_DEBUG("Lit-Off {}th Led, RNG {}", *it.first, r);
			}
		}
	}
}

//当前灯是否需要关掉
bool RandomLitoff::IsLitOff(int currentIndex)
{
	//手动随机灭灯情况下
	if (_rand_set.find(currentIndex) != _rand_set.end())
	{
		return true;	//此灯要随机灭灯
	}
	//SPDLOG_SINKS_DEBUG("The {}th needn't Lit-Off", currentIndex);
	return false;	//此灯不进行随机灭灯
}