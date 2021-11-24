
//#include <regex>
#include <time.h>
#include <io.h>
#include <cmath>
#include "AgingLog.h"
#include "PreDefine.h"
//#include "utility.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
#include "VideoCard.h"
#include "ConfigData.h"
#include "I2CWrap.h"
#include "RandomLitoff.h"
#define color_num (BGR)

//static bool reset_ppid = false;
//time_t aging_time;
AgingLog::AgingLog()
{

}

//AgingLog::AgingLog(int led_count, bool randomLightDown, bool retest)
//{
//	initAgingLog(led_count, randomLightDown, retest);
//}

void AgingLog::openAgingCsv()
{
	aging_file.open("./aging.csv", std::fstream::out | std::fstream::app);
	if (aging_file.is_open())
	{
		// get length of file:
		aging_file.seekg(0, aging_file.end);
		std::streampos length = aging_file.tellg();
		if (length == 0)
		{
			// 添加表头
			aging_file << "VideoCard, Time, PPID, Type, FinalResult, AllFailCount, ErrorCode,";

			char buf[10] = { 0 };
			for (int i = 0; i < color_num; i++)
			{
				// 因为1006 错误的存在，所以在初始化aging.csv文件时，无法get 到led count
				for (int j = 0; j < 22; j++)
				{
					sprintf_s(buf, 10, "%02d%02d\t,", i, j);
					aging_file << buf;
				}
			}
			aging_file << std::endl;
			aging_file.flush();
		}
	}
	else
	{
		throw std::exception("Can not open aging.csv");
	}
}

void AgingLog::initAgingLog()
{
	EXCEPTION_OPERATOR_TRY
	{
		int led_count = I2C.getLedCount();
		bool randomLightDown = litoff.getRandomLitOffState();
		bool retest = cfg.recheckFaileLedTime() > 0;

		if (led_count > 0)
		{
			lpLedCount = led_count; // white 暂时不计
			lpLed = new int[led_count * color_num];
			std::fill_n(lpLed, led_count * color_num, Fail);
		}

		if (randomLightDown)
		{
			this->randomLightDown = randomLightDown;
			lpRandomShutDownLedCache = new int[led_count * color_num];
			std::fill_n(lpRandomShutDownLedCache, led_count * color_num, Fail);
		}

		if (retest)
		{
			this->retest = retest;
			lpRetest = new int[led_count * color_num];
			std::fill_n(lpRetest, led_count * color_num, Fail);
		}

		SPDLOG_SINKS_DEBUG("AgingLog Init led_count:{}, retest:{}", led_count, retest);
	}
	catch (std::exception& e)
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw e;
	}
}

AgingLog& AgingLog::aging()
{
	static AgingLog aginglog;
	return aginglog;
}

AgingLog::~AgingLog()
{
	//saveAgingLog();

	aging_file.close();

	if (lpLed != nullptr)
	{
		delete[] lpLed;
	}

	if (lpRandomShutDownLedCache != nullptr)
	{
		delete[] lpRandomShutDownLedCache;
	}

	if (lpRetest != nullptr)
	{
		delete[] lpRetest;
	}
}

//void AgingLog::setPPID(char* ppid, int len)
//{
//	if(ppid != nullptr)
//		memcpy_s(PPID, VGA_PPID_LENGTH, ppid, len);
//}

void AgingLog::setSingleLedResult(int index, int color, int result)
{
	int i = index + lpLedCount * (color);
	if (i < lpLedCount * color_num)
	{
		lpLed[i] = result;
		SPDLOG_SINKS_DEBUG("AgingLog SetSingleLedResult i:{}, index:{}, color:{}, result:{}", i, index, color, result);
	}
}

void AgingLog::setSingleLedRetestResult(int index, int color, int result)
{
	if (retest) {
		int i = index + lpLedCount * (color);
		if (i < lpLedCount * color_num)
		{
			lpRetest[i] = result;
			SPDLOG_SINKS_DEBUG("AgingLog SetSingleLedRetestResult i:{}, index:{}, color:{}, result:{}", i, index, color, result);
		}
	}
}

void AgingLog::setSingleLedRandomShutDownResult(int index, int color, int result)
{
	if (randomLightDown) {
		int i = index + lpLedCount * (color);
		if (i < lpLedCount * color_num)
		{
			lpRandomShutDownLedCache[i] = result;
			SPDLOG_SINKS_DEBUG("AgingLog SetSingleLedRandomShutDownResult i:{}, index:{}, color:{}, result:{}", i, index, color, result);
		}
	}
}

/// 复测逻辑
/// 假如复测2次，则在复测前将首次的测试结果同步过来
/// 复测时若将灯由 Faile  判 Pass, 则第二遍将不测此灯
void AgingLog::syncSingLedResult2RetestResult()
{
	std::string t, t2;
	if (retest) {
		int len = (lpLedCount * color_num) * sizeof(int);
		memcpy_s(lpRetest, len, lpLed, len);
	}
}

void AgingLog::saveAgingLog(int error)
{
	if (aging_file.is_open())
	{
		int r = 0;
		struct tm *p = VideoCardIns.getTimestamp();
		char t[128] = { 0 };
		std::stringstream ss;
		sprintf_s(t, 128, "%d%02d%02d%02d%02d%02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

		////////////////////////////////////////////////////////////////////////////
		if (randomLightDown)
		{
			aging_file << VideoCardIns.Name() << "," << t << "\t," << VideoCardIns.PPID() << "\t," << "Random,";
			for (int i = 0; i < lpLedCount * color_num; i++)
			{
				// 随机灭掉的灯用-1表示
				r += lpRandomShutDownLedCache[i];
				ss << "," << lpRandomShutDownLedCache[i];
			}
			
			if (r > 0 || error > ERR_All_IS_WELL)
			{
				aging_file << "Fail," << r << ",";
			}
			else
			{
				aging_file << "Pass," << r << ",";
			}
			
			if (lpLedCount > 0)
			{
				aging_file << error << ss.rdbuf() << std::endl;
			}
			else
			{
				aging_file << error << std::endl;
			}
			ss.clear();

			SPDLOG_SINKS_DEBUG("AgingLog Save random PPID:{},t:{},r:{}", VideoCardIns.PPID(), t, r);
		}
		////////////////////////////////////////////////////////////////////////////
		// 若发生1004错误时, lpLed是还没有完成内存分配的
		r = 0;
		aging_file << VideoCardIns.Name() << "," << t << "\t," << VideoCardIns.PPID() << "\t," << "Normal,";
		for (int i = 0; (i < lpLedCount * color_num) && (lpLed != nullptr); i++)
		{
			r += lpLed[i];
			ss << "," << lpLed[i] ;
		}

		if (r > 0 || error > ERR_All_IS_WELL)
		{
			aging_file << "Fail," << r <<",";
		}
		else
		{
			aging_file << "Pass," << r <<",";
		}
		
		if (lpLedCount > 0)
		{
			aging_file << error << ss.rdbuf() << std::endl;
		}
		else
		{
			aging_file << error << std::endl;
		}
		ss.clear();

		SPDLOG_SINKS_DEBUG("AgingLog Save normal PPID:{},t:{},r:{}", VideoCardIns.PPID(), t, r);

		////////////////////////////////////////////////////////////////////////////
		if (retest)
		{
			r = 0;
			aging_file << VideoCardIns.Name() << "," << t << "\t," << VideoCardIns.PPID() << "\t," << "Retest,";
			for (int i = 0; i < lpLedCount * color_num; i++)
			{
				r += lpRetest[i];
				ss << "," << lpRetest[i];
			}

			if (r > 0 || error > ERR_All_IS_WELL)
			{
				aging_file << "Fail," << r << ",";
			}
			else
			{
				aging_file << "Pass," << r << ",";
			}

			if (lpLedCount > 0)
			{
				aging_file << error << ss.rdbuf() << std::endl;
			}
			else
			{
				aging_file << error << std::endl;
			}
			ss.clear();
			SPDLOG_SINKS_DEBUG("AgingLog Save reset PPID:{},t:{},r:{}", VideoCardIns.PPID(), t, r);
		}

		aging_file.flush();
	}

	std::fstream file("./result.txt", std::fstream::out);
	if (file.is_open())
	{
		if (error == ERR_All_IS_WELL)
			file << "PASS";
		else
			file << "FAIL";
		file.flush();
	}
	file.close();
}

int AgingLog::thisLedIsOK(int color)
{
	int r1 = 0, r2 = 0, r3 = 0;
	int i = lpLedCount * (color);
	int j = lpLedCount * (color + 1);
	for (; i < j; i++)
	{
		// 随机灭灯的结果同实际测出来的结果不匹配， 发生了误判
		//if (lpLed[i] != abs(lpRandomShutDownLedCache[i]))
		//{
		//	return Fail;
		//}

		// 随机灭灯结果要实际测试结果相抵为0，才算pass
		// 若r != 0, 即产生误判
		// r < 0, 本应该判定灭灯却误判成亮灯
		// r > 0, 未灭灯却判定灭灯，即轮廓不足， 导致误判成灭灯
		//r += (lpLed[i] + lpRandomShutDownLedCache[i]);

		r1 += lpLed[i];
		if (randomLightDown)
		{
			r2 += lpRandomShutDownLedCache[i];
		}

		if (retest)
		{
			r3 += lpRetest[i];
		}
	}

	if (retest)
	{
		SPDLOG_SINKS_DEBUG("Aginglog ThisLedIsOK retest:{}, r3:{}", retest, r3);
		if (r3 > 0)
			return Fail;
		else
			return Pass;
}
	else
	{
		SPDLOG_SINKS_DEBUG("Aginglog ThisLedIsOK retest:{}, r1:{}", retest, r1);
		if (r1 > 0)
			return Fail;
		else
			return Pass;
	}

}

int AgingLog::allLedIsOK()
{
	int r1 = 0, /*r2 = 0,*/ r3 = 0;
	int c = lpLedCount * color_num;
	for (int i = 0; i < c; i++)
	{
		r1 += lpLed[i];
		//if (randomLightDown)
		//{
		//	r2 += lpRandomShutDownLedCache[i];
		//}

		if (retest)
		{
			r3 += lpRetest[i];
		}
		// 随机灭灯的结果同实际测出来的结果不匹配， 发生了误判
		//if (lpLed[i] != abs(lpRandomShutDownLedCache[i]))
		//{
		//	return Fail;
		//}

		// 随机灭灯结果要实际测试结果相抵为0，才算pass
		// 若r != 0, 即产生误判
		// r < 0, 本应该判定灭灯却误判成亮灯
		// r > 0, 未灭灯却判定灭灯，即轮廓不足， 导致误判成灭灯
		//r += (lpLed[i] + lpRandomShutDownLedCache[i]);
	}

	// r1 > 0 说明有Faile 灯
	// r3 > 0 说明复测有Faile 灯	
	if (retest)
	{
		SPDLOG_SINKS_DEBUG("Aginglog AllLedIsOK retest:{}, r3:{}", retest, r3);
		if (r3 > 0)
			return Fail;
		else
			return Pass;
	}
	else
	{
		SPDLOG_SINKS_DEBUG("Aginglog AllLedIsOK retest:{}, r1:{}", retest, r1);
		if (r1 > 0)
			return Fail;
		else
			return Pass;
	}
	
}

int AgingLog::getSingleLedResult(int index, int color)
{
	int i = index + lpLedCount * (color);
	SPDLOG_SINKS_DEBUG("AgingLog GetSingleLedResult i:{}, index:{}, color:{}, result:{}", i, index, color, lpLed[i]);
	if (lpLed[i] > 0)
		return Fail;
	else
		return Pass;
}

int AgingLog::getSingleLedRetestResult(int index, int color)
{
	int i = index + lpLedCount * (color);
	SPDLOG_SINKS_DEBUG("AgingLog GetSingleLedRetestResult i:{}, index:{}, color:{}, result:{}", i, index, color, lpRetest[i]);
	if (lpRetest[i] > 0)
		return Fail;
	else
		return Pass;
}

void AgingLog::serialize()
{
	std::string s;
	for (int c = 0; c < color_num; c++)
	{
		s.clear();
		for (int j = 0; j < lpLedCount; ++j)
		{
			int x = j + lpLedCount * (c);
			if (lpLed[x] > 0)
			{
				s += std::to_string(j + 1);	// 下标从1 开始显示
				s += ", ";
			}
		}
		if (s.length() > 0)
		{
			SPDLOG_SINKS_ERROR("{}:  {}", color_str[c], s);
		}
	}	
}