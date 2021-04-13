#include "Minefield.h"

Minefield::Minefield(cv::Size r)
{
#if false
	// Σ�������趨�����봰�ڱ߿� N px �϶�ΪΣ�������ڴ�Σ�����ڵ���β�������Ա��϶�Ϊ��������
	Rect box(0, (int)(cfg.frame.height * 0.25), cfg.frame.width, (int)(cfg.frame.height * 0.5));
	auto InBox = [=](const cv::Rect& rect) ->bool {
		if (rect != (box & rect))
		{
			SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
			return false;
		}
		return true;
	};
#else
	// Σ�������趨�����봰�ڱ߿� N px �϶�ΪΣ�������ڴ�Σ�����ڵ���β�������Ա��϶�Ϊ��������
	int rl = 2;
	int tb = (int)(r.height * 0.25);	// ���²��ֵ�Σ�������趨�ϴ�Щ�� �ò��񵽵ĵƴ�����СһЩ
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

	// ���������tl(), br() ���صĵ��ոտ���cfg.frame.width or cfg.frame.height�ı߽�����
	// ����������� contains �ǲ�����Ϊ����rect�ڵ�
	// �˶�����Σ� ���Ͻǵ�������һ���� ���½ǵ�������һ���������Ǹպÿ���rect��
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
