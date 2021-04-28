#include "I2CWrap.h"
#include "ErrorCode.h"
#include "SpdMultipleSinks.h"
#include "PreDefine.h"
// LED 灯的地址
BYTE REG[22] = { 0x60, 0x63, 0x66, 0x69, 0x6c, 0x6f, 0x72, 0x75, 0x78, 0x7b, 0x7e
				, 0x81, 0x84, 0x87, 0x8a, 0x8d, 0x90, 0x93, 0x96, 0x99, 0x9c, 0x9f };

BYTE uOffset[12] = { 0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00 };

I2CWrap::I2CWrap()
{
	int res = 0;
	//HINSTANCE hDLL;		// Handle to DLL
	_hDLL = LoadLibrary(L"VGA_Extra_x64.dll");
	_hDLL == NULL ? throw ErrorCodeEx(ERR_LOAD_I2C_FAILURE, "Load VGA_Extra_x64.dll Failure") : (void)0;

	_lpLoadVenderDLL = (lpLoadVenderDLL)GetProcAddress(_hDLL, "LoadVenderDLL");
	_lpVGAReadICI2C = (lpVGAReadICI2C)GetProcAddress(_hDLL, "VGA_Read_IC_I2C");
	_lpVGAWriteICI2C = (lpVGAWriteICI2C)GetProcAddress(_hDLL, "VGA_Write_IC_I2C");
	SPDLOG_SINKS_DEBUG("LOAD_VENDOR_DLL:{}", _lpLoadVenderDLL == NULL ? "NULL" : "NOT NULL");
	SPDLOG_SINKS_DEBUG("VGA_READ_IC_I2C:{}", _lpVGAReadICI2C == NULL ? "NULL" : "NOT NULL");
	SPDLOG_SINKS_DEBUG("VGA_WRITE_IC_I2C:{}", _lpVGAWriteICI2C == NULL ? "NULL" : "NOT NULL");
	// 载入dll
	if (_lpLoadVenderDLL != NULL)
	{
		res = _lpLoadVenderDLL();
		SPDLOG_SINKS_DEBUG("LoadVenderDLL return {}", res);
	}
	else
	{
		throw ErrorCodeEx(ERR_LOAD_I2C_FAILURE, "Load VGA_Extra_x64.dll Failure");
	}

	// 从寄存器中获取LED 灯数量
	bool result = false;
	//BYTE count = 0x00;
	BYTE offset[2] = { 0x1D,0x0C };
	result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE*)offset, 0, 1, 1, 2, 1);	//set address
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to get led count") : (void)0;

	_lpVGAReadICI2C(0xCE, 0x08, (BYTE&)_ledCount, 0, 1, 1, 1, 1);
	SPDLOG_SINKS_DEBUG("Number of LEDs : {}", _ledCount);
}


I2CWrap::~I2CWrap()
{
	FreeLibrary(_hDLL);
}

I2CWrap& I2CWrap::i2c()
{
	static I2CWrap instance;
	return instance;
}

void I2CWrap::setSignleColor(int led, BYTE r, BYTE g, BYTE b)
{
	bool result = false;
	//Set Start Address
	uOffset[0] = 0x81;
	uOffset[1] = REG[led];
	result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address	
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 3;	//rgb size
	uOffset[1] = r;
	uOffset[2] = b;
	uOffset[3] = g;
	//(UCHAR ucAddress, UCHAR reg_address, UCHAR *rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags)
	result = _lpVGAWriteICI2C((BYTE)0xCE, (BYTE)0x03, (BYTE*)uOffset, 0, 1, 1, 4, 1);
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x80;
	uOffset[1] = 0x21;
	result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x01;
	result = _lpVGAWriteICI2C(0xCE, 0x1, (BYTE*)uOffset, 0, 1, 1, 1, 1);	//write data
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x80;
	uOffset[1] = 0x2F;
	result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x01;
	result = _lpVGAWriteICI2C(0xCE, 0x01, (BYTE*)uOffset, 0, 1, 1, 1, 1);	//write data
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
}


void I2CWrap::setSignleColor(int led, int color)
{
	switch (color)
	{
	case BLUE:
		setSignleColor(led, 0, 0, 255);
		break;
	case GREEN:
		setSignleColor(led, 0, 255, 0);
		break;
	case RED:
		setSignleColor(led, 255, 0, 0);
		break;
	case WHITE:
		setSignleColor(led, 255, 255, 255);
		break;
	case BLACK:
		setSignleColor(led, 0, 0, 0);
		break;
	}
}

void I2CWrap::resetColor(BYTE r, BYTE g, BYTE b)
{
	for (int i = 0; i < _ledCount; i++)
	{
		setSignleColor(i, r, g, b);
	}
}

void I2CWrap::resetColor(int color)
{
	for (int i = 0; i < _ledCount; i++)
	{
		setSignleColor(i, color);
	}
}

void I2CWrap::resetColorIter(int begin, int end, int color)
{
	for (int i = begin; i < end; i++)
	{
		setSignleColor(i, color);
	}
}