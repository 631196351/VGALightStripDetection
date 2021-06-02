#pragma once
#include <fstream>
//#include <set>

#define VGA_PPID_LENGTH 64

enum { Pass = 0, Fail = 1, RandomShutDownLed = -1 };

class AgingLog
{

	//char Video[VGA_PPID_LENGTH] = { 0 };
	//char lpTargetFolder[_MAX_PATH] = { 0 };
	int* lpLed = nullptr;
	int* lpRandomShutDownLedCache = nullptr;
	int* lpRetest = nullptr;
	int lpLedCount = 0;
	int startColor = 0;
	int stopColor = 0;

	bool retest = false;
	bool randomLightDown = false;
	std::fstream aging_file;

	//std::set<int> rand_set;

public:
	AgingLog();
	void initAgingLog(int led_count, bool randomLightDown, bool retest = false);
	~AgingLog();

	//void setPPID(char* ppit, int len);

	//inline const char* ppid() { return Video; }

	void setSingleLedResult(int index, int color, int result);

	void setSingleLedRandomShutDownResult(int index, int color, int result);

	void setSingleLedRetestResult(int index, int color, int result);

	void syncSingLedResult2RetestResult();

	void saveAgingLog();

	int thisLedIsOK(int color);

	int getSingleLedResult(int index, int color);

	int getSingleLedRetestResult(int index, int color);

	int allLedIsOK();

	/// 保存Log, 抹除上一次的测试结果数据，时间戳刷新； 
	/// 有的情况下会同一张卡测N次，所以这里是为下一遍测试做准备
	//void flushData();

	/// 获取一个CurrentTime_PPID 的字符串，来作为图片的保存文件夹名
	//inline const char* targetFolder() { return lpTargetFolder; }

	
	//设定随机灭灯状态, 设定手动关灯列表
	//void setRandomLitOffState(int probability, std::string manualset);

	//是否开启了随机灭灯
	//inline bool getRandomLitOffState() const { return randomLightDown; }

	//当前灯是否需要关掉
	//bool getThisLedLitOffState(int currentIndex);


	static AgingLog& aging();

	//friend std::ostream& operator<<(std::ostream& ost, const AgingLog& al);
	void serialize();
};

#define AgingInstance AgingLog::aging()