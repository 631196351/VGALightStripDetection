#pragma once

enum LEDColor
{
	BLUE,
	GREEN,
	RED,
	WHITE,
	AllColor
};

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
	eExitWithKey = 2	//侦测到退出指令
};