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

#define AgingFolder "./aging_rect_image"