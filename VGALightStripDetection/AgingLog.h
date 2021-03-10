#pragma once
#include <fstream>

#define VGA_PPID_LENGTH 20

enum { Pass = 0, Fail = 1, RandomShutDownLed = -1 };

class AgingLog
{

	char PPID[VGA_PPID_LENGTH] = { 0 };
	char lpTargetFolder[_MAX_PATH] = { 0 };
	int* lpLed = nullptr;
	int* lpRandomShutDownLedCache = nullptr;
	int lpLedCount = 0;
	int startColor = 0;
	int stopColor = 0;

	std::fstream aging_file;

public:
	AgingLog(int led_count);
	~AgingLog();

	void setPPID(char* ppit, int len);

	inline const char* ppid() { return PPID; }

	void setSingleLedResult(int index, int color, int result);

	void setSingleLedRandomShutDownResult(int index, int color, int result);

	void saveAgingLog();

	int thisLedIsOK(int color);

	int allLedIsOK();

	/// 保存Log, 抹除上一次的测试结果数据，时间戳刷新； 
	/// 有的情况下会同一张卡测N次，所以这里是为下一遍测试做准备
	void flushData();

	/// 获取一个CurrentTime_PPID 的字符串，来作为图片的保存文件夹名
	inline const char* targetFolder() { return lpTargetFolder; }
};

