#pragma once

#include <ctime>

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

struct AgingLog
{
	unsigned int PPID;	// 显卡PPID
	
	int color[3] = {255, 255, 255};	// 设置的颜色
	
	unsigned int point_block[8] = { 0 };	// 切分出来的8个区块, 用以记录每个区块中的符合条件的像素点个数
	
	clock_t time_consuming;		// 测试耗时

	time_t img_name;	// 记录失败时的img文件名
	
	bool result;

	AgingLog():PPID(0), time_consuming(clock()), result(false){}

	void setColor(int c)
	{
		switch (c)
		{
		case WHITE:
			color[0] = 255;
			color[1] = 255;
			color[2] = 255;
			break;
		case RED:
			color[0] = 255;
			color[1] = 0;
			color[2] = 0;
			break;
		case GREEN:
			color[0] = 0;
			color[1] = 255;
			color[2] = 0;
			break;
		case BLUE:
			color[0] = 0;
			color[1] = 0;
			color[2] = 255;
			break;
		}
	}

};

struct SingleLEDHSV
{
	int hsv_avg[3] = { 0 };
	int result = 1;	//非零为成功

	int& h = hsv_avg[0];
	int& s = hsv_avg[1];
	int& v = hsv_avg[2];
};

struct AgingLog2
{
	SingleLEDHSV* lpSingleLEDHSV = NULL;
	int nSingleLEDHSVSize = 0;
private:
	int currentLed[2] = { -1 };	// 指示当前亮灯的下标
	int& f = currentLed[0];	// 指示当前第一个亮着的灯
	int& s = currentLed[1];	// 指示当前第二个亮着的灯

public:
	AgingLog2(int size) :nSingleLEDHSVSize(size)
	{
		lpSingleLEDHSV = new SingleLEDHSV[nSingleLEDHSVSize];
	}

	~AgingLog2()
	{
		if (lpSingleLEDHSV != NULL)
		{
			delete[] lpSingleLEDHSV;
		}
		nSingleLEDHSVSize = 0;
	}

	SingleLEDHSV* ptr(int i)
	{
		if (i < nSingleLEDHSVSize)
			return &lpSingleLEDHSV[i];
		else
			return NULL;
	}

	int getSize() const { return nSingleLEDHSVSize; }

	// 两两亮灯时， 同时设置两个亮着的灯的下标
	// 单个亮灯时， 同时设置一个亮着的灯的下标
	void setCurrentLedIndex(int nf, int ns = -1)
	{
		this->f = nf;
		this->s = ns;
	}

	int getCurrentLedIndex_F() { return this->f; }
	int getCurrentLedIndex_S() { return this->s; }
};

//----		Red		Green	Blue	White
//hmin		156		35		100		0
//hmax		180		77		124		180
//smin		43		43		43		0
//smax		255		255		255		30
//vmin		46		46		46		221
//vmax		255		255		255		255
// createTrackbar(const String& trackbarname, const String& winname,int* value, int count,TrackbarCallback onChange = 0,void* userdata = 0);
// @param count 表示滑动控件的刻度范围；只能表示[0, count], 无法表示 Red 的[hmin, hmax]
// SO，需要通过这个结构体将Red 的[hmin, hmax]回归到[0, count]表示
// 即： [0, hmax - hmin]
// 获取到Trackbar 的value后， value + hmin 即为我们想要 h_value
struct HsvColor
{
	int h[7] = { 0 };
	int s[7] = { 0 };
	int v[7] = { 0 };
	HsvColor() {}
	HsvColor( int hmin, int hmax, int hlow, int hhight
			, int smin, int smax, int slow, int shight
			, int vmin, int vmax, int vlow, int vhight)		
	{
		h[0] = hmin;		// 放实际H最小值
		h[1] = hmax;		// 放实际H最大值
		h[2] = hlow;		// 放实际左边界, h[2]∈[ h[0], h[1] ]
		h[3] = hhight;		// 放实际右边界, h[3]∈[ h[0], h[1] ]
		h[4] = h[1] - h[0];	// 放TrackBar最大值
		h[5] = h[2] - h[0]; // 放TrackBar 左边界默认值, h[5]∈[ 0, h[4] ]
		h[6] = h[3] - h[0];	// 放TrackBar 右边界默认值, h[6]∈[ 0, h[4] ]

		s[0] = smin;
		s[1] = smax;
		s[2] = slow;
		s[3] = shight;
		s[4] = s[1] - s[0];
		s[5] = s[2] - s[0];
		s[6] = s[3] - s[0];

		v[0] = vmin;
		v[1] = vmax;
		v[2] = vlow;
		v[3] = vhight;
		v[4] = v[1] - v[0];
		v[5] = v[2] - v[0];
		v[6] = v[3] - v[0];
	}
};

struct BGRColor
{
	int color[3] = { 0 };
	int& b = color[0];
	int& g = color[1];
	int& r = color[2];
};

BGRColor g_BGRColors[AllColor];

LEDColor g_StartColor = BLUE;
LEDColor g_StopColor = AllColor;
unsigned g_AgingTime = 1;
int g_MinContoursArea = 200;
int g_MinContoursSpace = 60;
int g_tick = 50;