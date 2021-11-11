﻿#include "I2CWrap.h"
#include "ErrorCode.h"
#include "SpdMultipleSinks.h"
#include "PreDefine.h"
#include "RandomLitoff.h"

#if defined(LINUX)
#include <fstream>
#include "VideoCard.h"
#include "utility.h"
#endif

// LED 灯的地址
BYTE REG[22] = {0x60, 0x63, 0x66, 0x69, 0x6c, 0x6f, 0x72, 0x75, 0x78, 0x7b, 0x7e, 0x81, 0x84, 0x87, 0x8a, 0x8d, 0x90, 0x93, 0x96, 0x99, 0x9c, 0x9f};

BYTE uOffset[12] = {0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00};


I2CWrap::I2CWrap()
{
#if defined(WINDOWS)
	_hDLL = LoadLibrary(L"VGA_Extra_x64.dll");
	_hDLL == NULL ? throw ErrorCodeEx(ERR_LOAD_I2C_FAILURE, "Load VGA_Extra_x64.dll Failure") : (void)0;

	_lpLoadVenderDLL = (lpLoadVenderDLL)GetProcAddress(_hDLL, "LoadVenderDLL");
	_lpVGAReadICI2C = (lpVGAReadICI2C)GetProcAddress(_hDLL, "VGA_Read_IC_I2C");
	_lpVGAWriteICI2C = (lpVGAWriteICI2C)GetProcAddress(_hDLL, "VGA_Write_IC_I2C");
	SPDLOG_SINKS_DEBUG("LOAD_VENDOR_DLL:{}", _lpLoadVenderDLL == NULL ? "NULL" : "NOT NULL");
	SPDLOG_SINKS_DEBUG("VGA_READ_IC_I2C:{}", _lpVGAReadICI2C == NULL ? "NULL" : "NOT NULL");
	SPDLOG_SINKS_DEBUG("VGA_WRITE_IC_I2C:{}", _lpVGAWriteICI2C == NULL ? "NULL" : "NOT NULL");
	// 载入dll
	if (_lpLoadVenderDLL == NULL)
	{
		throw ErrorCodeEx(ERR_LOAD_I2C_FAILURE, "Load VGA_Extra_x64.dll Failure");
	}
	int gpu_count = 0;
	for (int i = 0; i < 120; ++i)
	{
		(i > 0) ? Sleep(1000) : (void)0;
		gpu_count = _lpLoadVenderDLL();
		SPDLOG_SINKS_DEBUG("{}th LoadVenderDLL return GPU count {}", i + 1, gpu_count);
		if (gpu_count > 0)
			break;
	}
	gpu_count == 0 ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Init VenderDll fail") : (void)0;

//#else
//	nvi2cinit();
#endif

	// 从寄存器中获取LED 灯数量
	BYTE offset[2] = {0x1D, 0x0C};
#if defined(WINDOWS)
	bool result = false;
	// 在2分钟内持续调用i2c, 直到调用成功，否则报 ERR_RUN_I2C_FAILURE 异常
	for (int i = 0; i < 120; ++i)
	{
		(i > 0) ? Sleep(1000) : (void)0;
		result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE *)offset, 0, 1, 1, 2, 1); //set address
		SPDLOG_SINKS_DEBUG("{}th time to call write i2c, return {}", i + 1, result);
		if (result)
			break;
	}
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to get led count") : (void)0;

	//#else
	// NvAPI_Status result;
	// for (int i = 0; i < 5; ++i) {
	// 	result = nvi2cWriteBlock(0xCE, 0x0, offset, 2);
	// 	SPDLOG_SINKS_DEBUG("{}th time to call write i2c, return {}", i+1, result);
	// 	if (result == NVAPI_OK)
	// 		break;
	// }
	// result != NVAPI_OK ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to get led count") : (void)0;
#endif

#if defined(WINDOWS)
	_lpVGAReadICI2C(0xCE, 0x08, (BYTE &)_ledCount, 0, 1, 1, 1, 1);
	//#else
	// nvi2cReadBlock(0xCE, 0x08, (NvU8*)&_ledCount, 1);
#elif defined(LINUX)
	if (VideoCardIns.getVendor() == VGA_NVIDIA)
		RunCmd("./tool/nv-led.sh 4");
	else if (VideoCardIns.getVendor() == VGA_AMD)
		RunCmd("./tool/amd-led.sh 4");

	std::ifstream f("./tool/led_count.txt");
	if (f.is_open())
	{
		// get length of file:
		f.seekg(0, f.end);
		std::streampos length = f.tellg();
		f.seekg(0, f.beg);

		if (length == 0)
		{
			throw ErrorCodeEx(ERR_INVALID_LED_COUNT, "led count == 0");
		}
		else
		{
			char *buffer = new char[length]{0};
			f.read(buffer, length);
			_ledCount = std::stoi(buffer, nullptr, 16);
			delete[] buffer;
		}
	}
	f.close();

	RunCmd("rm -f ./tool/led_count.txt 1>/dev/null");
#endif
	SPDLOG_SINKS_DEBUG("Number of LEDs : {}", _ledCount);
}

I2CWrap::~I2CWrap()
{
#if defined(WINDOWS)
	FreeLibrary(_hDLL);
#endif
}

I2CWrap &I2CWrap::i2c()
{
	static I2CWrap instance;
	return instance;
}

void I2CWrap::setSignleColor(int led, BYTE r, BYTE g, BYTE b)
{
#if defined(WINDOWS)
	//Set Start Address
	uOffset[0] = 0x81;
	uOffset[1] = REG[led];
	bool result = false;
	result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE *)uOffset, 0, 1, 1, 2, 1); //set address
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
	//#else
	// NvAPI_Status result = NVAPI_OK;
	// result = nvi2cWriteBlock(0xCE, 0x0, uOffset, 2);
	// result != NVAPI_OK ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 3; //rgb size
	uOffset[1] = r;
	uOffset[2] = b;
	uOffset[3] = g;
	//(UCHAR ucAddress, UCHAR reg_address, UCHAR *rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags)
	result = _lpVGAWriteICI2C((BYTE)0xCE, (BYTE)0x03, (BYTE *)uOffset, 0, 1, 1, 4, 1);
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
	//#else
	// result = nvi2cWriteBlock(0xCE, 0x03, uOffset, 4);
	// result != NVAPI_OK ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x80;
	uOffset[1] = 0x21;
	result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE *)uOffset, 0, 1, 1, 2, 1); //set address
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
	//#else
	// result = nvi2cWriteBlock(0xCE, 0x0, uOffset, 2);
	// result != NVAPI_OK ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x01;
	result = _lpVGAWriteICI2C(0xCE, 0x1, (BYTE *)uOffset, 0, 1, 1, 1, 1); //write data
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
	//#else
	// result = nvi2cWriteBlock(0xCE, 0x1, uOffset, 1);
	// result != NVAPI_OK ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x80;
	uOffset[1] = 0x2F;
	result = _lpVGAWriteICI2C(0xCE, 0x0, (BYTE *)uOffset, 0, 1, 1, 2, 1); //set address
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
	//#else
	// result = nvi2cWriteBlock(0xCE, 0x0, uOffset, 2);
	// result != NVAPI_OK ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;

	uOffset[0] = 0x01;
	result = _lpVGAWriteICI2C(0xCE, 0x01, (BYTE *)uOffset, 0, 1, 1, 1, 1); //write data
	result == false ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
	//#else
	// result = nvi2cWriteBlock(0xCE, 0x01, uOffset, 1);
	// result != NVAPI_OK ? throw ErrorCodeEx(ERR_RUN_I2C_FAILURE, "Write I2C Failure, failed to switch lights") : (void)0;
#elif defined(LINUX)
	char cmd[128] = {0};
	if (VideoCardIns.getVendor() == VGA_NVIDIA)
		std::snprintf(cmd, 128, "./tool/nv-led.sh 6 0x%X 0x%X 0x%X %d", r, g, b, led);
	else if (VideoCardIns.getVendor() == VGA_AMD)
		std::snprintf(cmd, 128, "./tool/amd-led.sh 6 0x%X 0x%X 0x%X %d", r, g, b, led);
	RunCmd(cmd);

#endif
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
#if defined(WINDOWS)
	for (int i = 0; i < _ledCount; i++)
	{
		setSignleColor(i, r, g, b);
	}
#elif defined(LINUX)
	char cmd[128] = {0};

	if (VideoCardIns.getVendor() == VGA_NVIDIA)
		std::snprintf(cmd, 128, "./tool/nv-led.sh 5 0x%X 0x%X 0x%X", r, g, b);
	else if (VideoCardIns.getVendor() == VGA_AMD)
		std::snprintf(cmd, 128, "./tool/amd-led.sh 5 0x%X 0x%X 0x%X", r, g, b);
	RunCmd(cmd);
#endif
}

void I2CWrap::resetColor(int color)
{
#if defined(WINDOWS)
	for (int i = 0; i < _ledCount; i++)
	{
		setSignleColor(i, color);
	}
#elif defined(LINUX)
	char cmd[128] = {0};
	char tool[20] = {0};

	if (VideoCardIns.getVendor() == VGA_NVIDIA)
		std::snprintf(tool, 20, "nv-led.sh");
	else if (VideoCardIns.getVendor() == VGA_AMD)
		std::snprintf(tool, 20, "amd-led.sh");

	switch (color)
	{
	case BLUE:
		std::snprintf(cmd, 128, "./tool/%s 5 0x%X 0x%X 0x%X", tool, 0, 0, 255);
		break;
	case GREEN:
		std::snprintf(cmd, 128, "./tool/%s 5 0x%X 0x%X 0x%X", tool, 0, 255, 0);
		break;
	case RED:
		std::snprintf(cmd, 128, "./tool/%s 5 0x%X 0x%X 0x%X", tool, 255, 0, 0);
		break;
	case WHITE:
		std::snprintf(cmd, 128, "./tool/%s 5 0x%X 0x%X 0x%X", tool, 255, 255, 255);
		break;
	case BLACK:
		std::snprintf(cmd, 128, "./tool/%s 5 0x%X 0x%X 0x%X", tool, 0, 0, 0);
		break;
	}
	RunCmd(cmd);
#endif
}

void I2CWrap::resetColorIter(int begin, int end, int color)
{
	for (int i = begin; i < end; i++)
	{
		setSignleColor(i, color);
	}
}

//设定随机灭灯状态, 设定手动关灯列表
//void I2CWrap::setRandomLitOffState(int probability, std::string manualset)
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
//		_bRlitOffState = true;
//	}
//
//	SPDLOG_SINKS_DEBUG("RandomLitOffState randomLightDown:{}", _bRlitOffState);
//
//	if (!manualset.empty())
//	{
//		std::regex reg(",");		// 匹配split
//		std::sregex_token_iterator pos(manualset.begin(), manualset.end(), reg, -1);
//		decltype(pos) end;              // 自动推导类型
//		for (; pos != end; ++pos)
//		{
//			auto it = _rand_set.insert(atoi(pos->str().c_str()));
//			SPDLOG_SINKS_DEBUG("Lit-Off {}th Led", *it.first);
//		}
//	}
//
//	// 直接在这里初始化好每颗灯的命运
// eg: 22颗灯里面有【3,4,7,10,15】这几颗灯会随机灭掉，BGR都会灭掉
//	if (probability > 0)
//	{
//		cv::RNG rng(time(NULL));
//		for (int i = 0; i < _ledCount; i++)
//		{
//			int r = rng.uniform(0, 101);	//[0, 101)
//			if (probability >= r)
//			{
//				auto it = _rand_set.insert(i);
//				SPDLOG_SINKS_DEBUG("Lit-Off {}th Led, RNG {}", *it.first, r);
//			}
//		}
//	}
//}
//
//当前灯是否需要关掉
//bool I2CWrap::IsLitOff(int currentIndex)
//{
//	//手动随机灭灯情况下
//	if (_rand_set.find(currentIndex) != _rand_set.end())
//	{
//		return true;	//此灯要随机灭灯
//	}
//	//SPDLOG_SINKS_DEBUG("The {}th needn't Lit-Off", currentIndex);
//	return false;	//此灯不进行随机灭灯
//}