#pragma once

#include <opencv2/opencv.hpp>

// Σ�������趨�����봰�ڱ߿� N px �϶�ΪΣ�������ڴ�Σ�����ڵ���β�������Ա��϶�Ϊ��������
class Minefield
{
	cv::Rect _t;
	cv::Rect _r;
	cv::Rect _b;
	cv::Rect _l;
public:
	Minefield(cv::Size r);
	~Minefield();
	bool inMinefield(const cv::Rect& r);
	inline const cv::Rect& t() const { return _t; }
	inline const cv::Rect& r() const { return _r; }
	inline const cv::Rect& b() const { return _b; }
	inline const cv::Rect& l() const { return _l; }
};

