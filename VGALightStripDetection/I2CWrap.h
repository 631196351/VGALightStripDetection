#pragma once

#include <Windows.h>
#include <set>
typedef unsigned long Ul32;
typedef unsigned char BYTE;

typedef int(*lpLoadVenderDLL)();
typedef bool(*lpVGAReadICI2C)(UCHAR ucAddress, UCHAR reg_address, BYTE &rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags);
typedef bool(*lpVGAWriteICI2C)(UCHAR ucAddress, UCHAR reg_address, UCHAR *rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags);

class I2CWrap
{
	HINSTANCE			 _hDLL = NULL;
	lpLoadVenderDLL		 _lpLoadVenderDLL = NULL;
	lpVGAReadICI2C		 _lpVGAReadICI2C = NULL;
	lpVGAWriteICI2C		 _lpVGAWriteICI2C = NULL;
	int					 _ledCount = 0;

	//std::set<int>		 _rand_set;
	//bool				 _bRlitOffState = false;

public:
	I2CWrap();
	~I2CWrap();

	static I2CWrap& i2c();

	inline int getLedCount() const { return _ledCount; };

	void setSignleColor(int led, BYTE r, BYTE g, BYTE b);

	void setSignleColor(int led, int color);

	void resetColor(BYTE r, BYTE g, BYTE b);

	void resetColor(int color);

	void resetColorIter(int begin, int end, int color);

	////设定随机灭灯状态, 设定手动关灯列表
	//void setRandomLitOffState(int probability, std::string manualset);
	//
	////是否开启了随机灭灯
	//inline bool getRandomLitOffState() const { return _bRlitOffState; }
	//
	////当前灯是否需要关掉
	//bool IsLitOff(int currentIndex);

};

#define I2C I2CWrap::i2c()
