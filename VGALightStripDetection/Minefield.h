#pragma once

#include <opencv2/opencv.hpp>

// 危险区域设定，距离窗口边框 N px 认定为危险区，在此危险区内的首尾灯轮廓皆被认定为超出窗口
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

