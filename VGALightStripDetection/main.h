#pragma once

#include <ctime>

enum ControlLEDColor
{
	WHITE,
	RED,
	GREEN,
	BLUE,
	AllColor
};

// �����������ص����ֵ
enum 
{
	W_Threshold = 250,
	R_Threshold = 250, 
	G_Threshold = 250,
	B_Threshold = 250
};

struct AgingLog
{
	unsigned int PPID;	// �Կ�PPID
	
	int color[3] = {255, 255, 255};	// ���õ���ɫ
	
	unsigned int point_block[8] = { 0 };	// �зֳ�����8������, ���Լ�¼ÿ�������еķ������������ص����
	
	clock_t time_consuming;		// ���Ժ�ʱ

	time_t img_name;	// ��¼ʧ��ʱ��img�ļ���
	
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

//----		Red		Green	Blue	White
//hmin		156		35		100		0
//hmax		180		77		124		180
//smin		43		43		43		0
//smax		255		255		255		30
//vmin		46		46		46		221
//vmax		255		255		255		255
// createTrackbar(const String& trackbarname, const String& winname,int* value, int count,TrackbarCallback onChange = 0,void* userdata = 0);
// @param count ��ʾ�����ؼ��Ŀ̶ȷ�Χ��ֻ�ܱ�ʾ[0, count], �޷���ʾ Red ��[hmin, hmax]
// SO����Ҫͨ������ṹ�彫Red ��[hmin, hmax]�ع鵽[0, count]��ʾ
// ���� [0, hmax - hmin]
// ��ȡ��Trackbar ��value�� value + hmin ��Ϊ������Ҫ h_value
struct HsvColor
{
	int h[7] = { 0 };
	int s[7] = { 0 };
	int v[7] = { 0 };
	HsvColor( int hmin, int hmax, int hlow, int hhight
			, int smin, int smax, int slow, int shight
			, int vmin, int vmax, int vlow, int vhight)		
	{
		h[0] = hmin;		// �Ŷ�ӦH��Сֵ
		h[1] = hmax;		// �Ŷ�ӦH���ֵ
		h[2] = hlow;		// ��ʵ����߽�, h[2]��[ h[0], h[1] ]
		h[3] = hhight;		// ��ʵ���ұ߽�, h[3]��[ h[0], h[1] ]
		h[4] = h[1] - h[0];	// ��TrackBar���ֵ
		h[5] = h[2] - h[0]; // ��TrackBar ��߽�Ĭ��ֵ, h[5]��[ 0, h[4] ]
		h[6] = h[3] - h[0];	// ��TrackBar �ұ߽�Ĭ��ֵ, h[6]��[ 0, h[4] ]

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