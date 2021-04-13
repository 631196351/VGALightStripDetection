#pragma once

#include <Windows.h>

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

};

#define I2C I2CWrap::i2c()
