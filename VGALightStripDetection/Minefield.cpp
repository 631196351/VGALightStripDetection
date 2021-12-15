#include "Minefield.h"

Minefield::Minefield(cv::Size r)
{
#if false
	// 危险区域设定，距离窗口边框 N px 认定为危险区，在此危险区内的首尾灯轮廓皆被认定为超出窗口
	Rect box(0, (int)(kConfig.frame.height * 0.25), kConfig.frame.width, (int)(kConfig.frame.height * 0.5));
	auto InBox = [=](const cv::Rect& rect) ->bool {
		if (rect != (box & rect))
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			return false;
		}
		return true;
	};
#else
	// 危险区域设定，距离窗口边框 N px 认定为危险区，在此危险区内的首尾灯轮廓皆被认定为超出窗口
	int rl = 2;
	int tb = 2;
	//int tb = (int)(r.height * 0.25);	// 上下部分的危险区域设定较大些， 让捕获到的灯带轮廓小一些
	_t = cv::Rect(0, 0, r.width, tb);
	_r = cv::Rect(r.width - rl, 0, rl, r.height);
	_b = cv::Rect(0, r.height - tb, r.width, tb);
	_l = cv::Rect(0, 0, rl, r.height);
#endif
}

Minefield::~Minefield()
{

}

bool Minefield::inMinefield(const cv::Rect& r)
{

	// 极端情况下tl(), br() 返回的点会刚刚卡在kConfig.frame.width or kConfig.frame.height的边界线上
	// 而这种情况下 contains 是不会认为点在rect内的
	// 退而求其次， 左上角点往外走一步， 右下角点往里走一步，让他们刚好卡在rect内
	cv::Point tl = r.tl() + cv::Point(1, 1);
	cv::Point br = r.br() - cv::Point(1, 1);

	if (_t.contains(tl) || _r.contains(tl) || _b.contains(tl) || _l.contains(tl))
	{
		return true;
	}
	if (_t.contains(br) || _r.contains(br) || _b.contains(br) || _l.contains(br))
	{
		return true;
	}
	return false;
}
