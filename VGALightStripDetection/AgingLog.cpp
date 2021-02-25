#include "AgingLog.h"
#include "PreDefine.h"
#include "utility.h"

#include <time.h>
#include <io.h>
#include <direct.h>


#define color_num (AllColor )

AgingLog::AgingLog(int led_count)
{
	if (led_count > 0)
	{
		lpLedCount = led_count; // white 暂时不计
		lpLed = new int[led_count * color_num] { 0 };
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
			aging_file << "PPID,";
				
			char buf[10] = { 0 };
			for (int i = 0; i < color_num; i++)
			{
				for (int j = 0; j < led_count; j++)
				{
					sprintf_s(buf, 10, "'%02d%02d',", i, j);
					aging_file << buf;
				}
			}
			aging_file << "result0," << "result1" <<std::endl;
			aging_file.flush();
		}
	}

	//getVGAInfo(PPID, VGA_PPID_LENGTH);

	sprintf_s(PPID, VGA_PPID_LENGTH, "%ld", time(0));

	if (0 != _access(AgingFolder, 0))
	{
		_mkdir(AgingFolder);   // 返回 0 表示创建成功，-1 表示失败		
	}
	char path[128] = { 0 };
	sprintf_s(path, 128, "%s/%s", AgingFolder, PPID);
	_mkdir(path);
	
}


AgingLog::~AgingLog()
{
	//saveAgingLog();

	aging_file.close();

	if (lpLed != nullptr)
	{
		delete[] lpLed;
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


void AgingLog::saveAgingLog()
{
	if (aging_file.is_open()) 
	{

		char buf[10] = { 0 };
		int r = 0;
		aging_file << PPID << ",";
		for (int i = 0; i < lpLedCount * color_num; i++)
		{
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
		r += lpLed[i];
	}

	return (r > 0 ? Fail : Pass);
}

int AgingLog::allLedIsOK()
{
	int r = 0;
	for (int i = 0; i < lpLedCount * color_num; i++)
	{
		r += lpLed[i];
	}

	return (r > 0 ? Fail : Pass);
}

void AgingLog::flushData()
{
	saveAgingLog();

	memset(lpLed, 0, lpLedCount*color_num);
	sprintf_s(PPID, VGA_PPID_LENGTH, "%ld", time(0));

	if (0 != _access(AgingFolder, 0))
	{
		_mkdir(AgingFolder);   // 返回 0 表示创建成功，-1 表示失败		
	}
	char path[128] = { 0 };
	sprintf_s(path, 128, "%s/%s", AgingFolder, PPID);
	_mkdir(path);
}