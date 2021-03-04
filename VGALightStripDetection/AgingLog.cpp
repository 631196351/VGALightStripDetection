#include "AgingLog.h"
#include "PreDefine.h"
#include "utility.h"

#include <time.h>
#include <io.h>
#include <cmath>

#define color_num (AllColor )

static bool reset_ppid = false;
time_t aging_time;

AgingLog::AgingLog(int led_count)
{
	if (led_count > 0)
	{
		lpLedCount = led_count; // white 暂时不计
		lpLed = new int[led_count * color_num] { 0 };
		lpRandomShutDownLedCache = new int[led_count * color_num]{ 0 };
	}
	
	aging_file.open("./aging.csv", std::fstream::out | std::fstream::app);
	if (aging_file.is_open())
	{
		// get length of file:
		aging_file.seekg(0, aging_file.end);
		std::streampos length = aging_file.tellg();
		if (length == 0)
		{
			// 添加表头
			aging_file << "PPID," << "Time,";
				
			char buf[10] = { 0 };
			for (int i = 0; i < color_num; i++)
			{
				for (int j = 0; j < led_count; j++)
				{
					sprintf_s(buf, 10, "%02d%02d\t,", i, j);
					aging_file << buf;
				}
			}
			aging_file << "result0," << "result1" <<std::endl;
			aging_file.flush();
		}
	}

	getVGAInfo(PPID, VGA_PPID_LENGTH);
    time(&aging_time);
	if (PPID[0] == 0)	// 获取不到PPID时， 用time() 来替代
	{
		sprintf_s(PPID, VGA_PPID_LENGTH, "%ld", aging_time);
		reset_ppid = true;
	}
}


AgingLog::~AgingLog()
{
	aging_file.close();

	if (lpLed != nullptr)
	{
		delete[] lpLed;
	}

	if (lpRandomShutDownLedCache != nullptr)
	{
		delete[] lpRandomShutDownLedCache;
	}
}

void AgingLog::setPPID(char* ppid, int len)
{
	if(ppid != nullptr)
		memcpy_s(PPID, VGA_PPID_LENGTH, ppid, len);
}

void AgingLog::setSingleLedResult(int index, int color, int result)
{
	int i = index + lpLedCount * (color);
	if (i < lpLedCount * color_num)
	{
		lpLed[i] = result;
	}
}

void AgingLog::setSingleLedRandomShutDownResult(int index, int color, int result)
{
	int i = index + lpLedCount * (color);
	if (i < lpLedCount * color_num)
	{
		lpRandomShutDownLedCache[i] = result;
	}
}

void AgingLog::saveAgingLog()
{
	if (aging_file.is_open()) 
	{
		int r = 0;
		struct tm *p = localtime(&aging_time);
		char t[128] = { 0 };
		sprintf_s(t, 128, "%d%02d%02d%02d%02d%02d\t", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

		////////////////////////////////////////////////////////////////////////////
		aging_file << PPID << "," << t <<",";
		for (int i = 0; i < lpLedCount * color_num; i++)
		{
			// 随机灭掉的灯用-1表示
			r += lpRandomShutDownLedCache[i];
			aging_file << lpRandomShutDownLedCache[i] << ",";
		}

		aging_file << r << ",";
		if (r < 0)
		{
			aging_file << "Failure" << std::endl;
		}
		else
		{
			aging_file << "Pass" << std::endl;
		}

		////////////////////////////////////////////////////////////////////////////
		r = 0;
		aging_file << PPID << "," << t << ",";
		for (int i = 0; i < lpLedCount * color_num; i++)
		{
			// 随机灭灯时， 失败用 -1表示；正常失败时用 1表示；故最终结果取绝对值
			r += lpLed[i];
			aging_file << lpLed[i] << ",";
		}

		aging_file << r << ",";
		if (r > 0)
		{
			aging_file << "Failure" << std::endl;
		}
		else
		{
			aging_file << "Pass" << std::endl;
		}
	}
}


int AgingLog::thisLedIsOK(int color)
{
	int r = 0;
	int i = lpLedCount * (color);
	int j = lpLedCount * (color + 1);
	for (; i < j; i++)
	{
		r += abs(lpLed[i]);

		if (lpLed[i] != abs(lpRandomShutDownLedCache[i]))
		{
			return Fail;
		}
	}

	return (r > 0 ? Fail : Pass);
}

int AgingLog::allLedIsOK()
{
	int r = 0;
	for (int i = 0; i < lpLedCount * color_num; i++)
	{
		r += abs(lpLed[i]);
		if (lpLed[i] != abs(lpRandomShutDownLedCache[i]))
		{
			return Fail;
		}
	}

	return (r > 0 ? Fail : Pass);
}

void AgingLog::flushData()
{
	saveAgingLog();

	memset(lpLed, 0, lpLedCount*color_num);
	memset(lpRandomShutDownLedCache, 0, lpLedCount*color_num);

    time(&aging_time);
	if (reset_ppid)
	{
		sprintf_s(PPID, VGA_PPID_LENGTH, "%ld", aging_time);
	}	
}