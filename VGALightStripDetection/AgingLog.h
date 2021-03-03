#pragma once
#include <fstream>

#define VGA_PPID_LENGTH 20
enum {	Pass = 0, Fail = 1, Fail_RandomShutDownLed = -1};

class AgingLog
{

	char PPID[VGA_PPID_LENGTH] = { 1 };
	int* lpLed = nullptr;
	int lpLedCount = 0;
	int startColor = 0;
	int stopColor = 0;

	std::fstream aging_file;

public:	
	AgingLog(int led_count);
	~AgingLog();

	void setPPID(char* ppit, int len);

	inline char* ppid() { return PPID; }

	void setSingleLedResult(int index, int color, int result);

	void saveAgingLog();

	int thisLedIsOK(int color);

	int allLedIsOK();


	void flushData();

};

