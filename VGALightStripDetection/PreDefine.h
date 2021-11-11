#pragma once

enum LEDColor
{
	BLUE,
	GREEN,
	RED,
	WHITE,
	BGR = WHITE,
	AllColor,
	BGRW = AllColor,
	BLACK,
};

static const char* color_str[] = { "Blue", "Green", "Red", "White" };

// 用来过滤像素点的阈值
enum
{
	W_Threshold = 250,
	R_Threshold = 250,
	G_Threshold = 250,
	B_Threshold = 250
};

#define AgingFolder "aging_rect_image"

enum
{
	eNotExit = 0,	//未侦测到退出指令
	eExit = 1,		//执行完毕，正常退出指令
	eExitWithKey = 2,	//侦测到退出指令
	eExitWithException = 3	// 异常退出
};

enum
{
	ePowerOff = 0,
	eNotPowerOff = -1,
	eReStart = -2
};

enum
{
	VersionMajor = 2,
	VersionSec = 0,
	VersionThi = 1,
	VersionMin = 5
};

enum HSV
{
	eHmin = 0,
	eHmax = 1,
	eSmin = 2,
	eVmin = 3,
	eHmin2 = 4,
	eHmax2 = 5
};