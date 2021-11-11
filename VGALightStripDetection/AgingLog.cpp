
//#include <regex>
//#include <time.h>
//#include <string.h>
#include <cstdio>
#include <cmath>
#include "AgingLog.h"
#include "PreDefine.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
#include "VideoCard.h"
#include "ConfigData.h"
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

void AgingLog::initAgingLog(int led_count, bool randomLightDown, bool retest)
{
	try 
	{
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

		aging_file.open("./aging.csv", std::fstream::out | std::fstream::app);
		if (aging_file.is_open())
		{
			// get length of file:
			aging_file.seekg(0, aging_file.end);
			std::streampos length = aging_file.tellg();
			if (length == 0)
			{
				// 添加表头
				aging_file <<"VideoCard,"<< "Time," << "PPID," << "Type,";

				char buf[10] = { 0 };
				for (int i = 0; i < color_num; i++)
				{
					for (int j = 0; j < led_count; j++)
					{
						//sprintf_s(buf, 10, "%02d%02d\t,", i, j);
						std::snprintf(buf, 10, "%02d%02d\t,", i, j);
						aging_file << buf;
					}
				}
				aging_file << "result0," << "result1" << std::endl;
				aging_file.flush();
			}
		}
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

		#ifdef WINDOWS
		std::memcpy(lpRetest, len, lpLed, len);
		#else
		std::memcpy(lpRetest, lpLed, len);
		#endif
	}
}

void AgingLog::saveAgingLog()
{
	if (aging_file.is_open()) 
	{
		int r = 0;
		struct tm *p = VideoCardIns.getTimestamp();
		char t[128] = { 0 };
		std::snprintf(t, 128, "%d%02d%02d%02d%02d%02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

		////////////////////////////////////////////////////////////////////////////
		if (randomLightDown)
		{
			aging_file <<VideoCardIns.Name()<<","<< t << "\t," << VideoCardIns.PPID() << "\t,"<<"Random,";
			for (int i = 0; i < lpLedCount * color_num; i++)
			{
				// 随机灭掉的灯用-1表示
				r += lpRandomShutDownLedCache[i];
				aging_file << lpRandomShutDownLedCache[i] << ",";
			}

			aging_file << r << ",";
			if (r < 0)
			{
				aging_file << "Fail" << std::endl;
			}
			else
			{
				aging_file << "Pass" << std::endl;
			}

			SPDLOG_SINKS_DEBUG("AgingLog Save random PPID:{},t:{},r:{}", VideoCardIns.PPID(), t, r);
		}
		////////////////////////////////////////////////////////////////////////////
		r = 0;
		aging_file << VideoCardIns.Name() << "," << t << "\t," << VideoCardIns.PPID() << "\t," << "Normal,";
		for (int i = 0; i < lpLedCount * color_num; i++)
		{
			r += lpLed[i];
			aging_file << lpLed[i] << ",";
		}

		aging_file << r << ",";
		if (r > 0)
		{
			aging_file << "Fail" << std::endl;
		}
		else
		{
			aging_file << "Pass" << std::endl;
		}

		SPDLOG_SINKS_DEBUG("AgingLog Save normal PPID:{},t:{},r:{}", VideoCardIns.PPID(), t, r);

		////////////////////////////////////////////////////////////////////////////
		if (retest)
		{
			r = 0;
			aging_file << VideoCardIns.Name() << "," << t << "\t," << VideoCardIns.PPID() << "\t," << "Retest,";
			for (int i = 0; i < lpLedCount * color_num; i++)
			{
				r += lpRetest[i];
				aging_file << lpRetest[i] << ",";
			}

			aging_file << r << ",";
			if (r > 0)
			{
				aging_file << "Fail" << std::endl;
			}
			else
			{
				aging_file << "Pass" << std::endl;
			}
			SPDLOG_SINKS_DEBUG("AgingLog Save reset PPID:{},t:{},r:{}", VideoCardIns.PPID(), t, r);
		}

		aging_file.flush();
	}
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

//void AgingLog::flushData()
//{
//	SPDLOG_SINKS_DEBUG("......AgingLog Flush Data......");
//	saveAgingLog();
//
//	memset(lpLed, 0, lpLedCount*color_num);
//	if (randomLightDown)
//	{
//		memset(lpRandomShutDownLedCache, 0, lpLedCount*color_num);
//	}
//	if (retest)
//	{
//		memset(lpRetest, 0, lpLedCount*color_num);
//	}
//
//    //time(&aging_time);
//	//struct tm *p = localtime(&aging_time);
//
//	//sprintf_s(lpTargetFolder, _MAX_PATH, "%d%02d%02d%02d%02d%02d_%s", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, PPID);	
//	//SPDLOG_SINKS_DEBUG("AgingLog Create Target Folder:{}", lpTargetFolder);
//}

//设定随机灭灯状态, 设定手动关灯列表
//void AgingLog::setRandomLitOffState(int probability, std::string manualset)
//{
//	SPDLOG_SINKS_DEBUG("RandomLitOffState probability:{}, manualset:{}", probability, manualset);
//
//	if (probability > 0 && !manualset.empty())
//	{
//		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
//		throw ErrorCodeEx(ERR_COMMAND_LINE_ARGS, "Random lit-off parameter configuration is repeated");
//	}
//
//	if (probability > 0 || !manualset.empty())
//	{
//		randomLightDown = true;
//	}
//	SPDLOG_SINKS_DEBUG("RandomLitOffState randomLightDown:{}", randomLightDown);
//
//	if (randomLightDown)
//	{
//		this->randomLightDown = randomLightDown;
//		lpRandomShutDownLedCache = new int[lpLedCount * color_num];
//		std::fill_n(lpRandomShutDownLedCache, lpLedCount * color_num, Fail);
//	}
//
//	if (!manualset.empty())
//	{
//		std::regex reg(",");		// 匹配split
//		std::sregex_token_iterator pos(manualset.begin(), manualset.end(), reg, -1);
//		decltype(pos) end;              // 自动推导类型 
//		for (; pos != end; ++pos)
//		{
//			auto it = rand_set.insert(atoi(pos->str().c_str()));
//			SPDLOG_SINKS_DEBUG("RandomLitOffState {}th Led", *it.first);
//		}
//	}
//
//	// 直接在这里初始化好每颗灯的命运
//	// eg: 22颗灯里面有【3,4,7,10,15】这几颗灯会随机灭掉，BGR都会灭掉
//	if (probability > 0)
//	{
//		cv::RNG rng(time(NULL));
//		for (int i = 0; i < lpLedCount; i++)
//		{
//			int r = rng.uniform(0, 101);	//[0, 101)
//			if (probability >= r)
//			{
//				auto it = rand_set.insert(i);
//				SPDLOG_SINKS_DEBUG("RandomLitOffState {}th Led, RNG {}", *it.first, r);
//			}
//		}
//	}
//}
//
////当前灯是否需要关掉
//bool AgingLog::getThisLedLitOffState(int currentIndex)
//{
//	//手动随机灭灯情况下
//	if (rand_set.find(currentIndex) != rand_set.end())
//	{
//		SPDLOG_SINKS_DEBUG("The {}th need Lit-Off", currentIndex);
//		return false;	//此灯要随机灭灯
//	}
//	SPDLOG_SINKS_DEBUG("The {}th needn't Lit-Off", currentIndex);
//	return true;	//此灯不进行随机灭灯
//}


//std::ostream& operator<<(std::ostream& ost, const AgingLog& al)
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
				// 日志显示时下标从1开始记
				s += std::to_string(j + 1);
				s += ", ";
			}
		}
		if (s.length() > 0)
		{
			SPDLOG_SINKS_ERROR("{}:  {}", color_str[c], s);
		}
	}	
}