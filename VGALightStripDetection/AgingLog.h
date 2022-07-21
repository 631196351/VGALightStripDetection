#pragma once
#include <fstream>
//#include <set>

#define VGA_PPID_LENGTH 64

// 本模块维护 输出结果CSV 文件

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

	// 根据灯珠数量动态分配数组容器空间
	void initAgingLog();
	~AgingLog();

	//void setPPID(char* ppit, int len);

	//inline const char* ppid() { return Video; }
	// 打开CSV 文件
	void openAgingCsv();

	// 将亮color 时， 第index颗灯结果保存到lpLed
	void setSingleLedResult(int index, int color, int result);

	// 将亮color 时， 第index颗灯结果保存到lpRandomShutDownLedCache
	void setSingleLedRandomShutDownResult(int index, int color, int result);

	// 将亮color 时， 第index颗灯结果保存到lpRetest
	void setSingleLedRetestResult(int index, int color, int result);

	//填补表格中不足22颗灯中，结果保存空（led num = 4， 表格中5-22的结果用-1填充）
	void setSingleLedResultEmpty(int index, int color, int result);

	// 为了不污染lpLed， 将lpLed copy 到lpRetest
	void syncSingLedResult2RetestResult();

	void saveAgingLog(int error);

	int thisLedIsOK(int color);

	int getSingleLedResult(int index, int color);

	int getSingleLedRetestResult(int index, int color);

	// 返回所有灯珠的结果
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