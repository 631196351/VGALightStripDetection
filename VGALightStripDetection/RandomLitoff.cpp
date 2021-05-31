#include <regex>
#include "I2CWrap.h"
#include "RandomLitoff.h"
#include "ErrorCode.h"
#include "SpdMultipleSinks.h"

//�趨������״̬, �趨�ֶ��ص��б�
void RandomLitoff::setRandomLitOffState(int probability, std::string manualset)
{
	SPDLOG_SINKS_DEBUG("RandomLitOffState probability:{}, manualset:{}", probability, manualset);

	if (probability > 0 && !manualset.empty())
	{
		SPDLOG_NOTES_THIS_FUNC_EXCEPTION;
		throw ErrorCodeEx(ERR_COMMAND_LINE_ARGS, "Random lit-off parameter configuration is repeated");
	}

	if (probability > 0 || !manualset.empty())
	{
		_bRlitOffState = true;
	}

	SPDLOG_SINKS_DEBUG("RandomLitOffState randomLightDown:{}", _bRlitOffState);

	if (!manualset.empty())
	{
		std::regex reg(",");		// ƥ��split
		std::sregex_token_iterator pos(manualset.begin(), manualset.end(), reg, -1);
		decltype(pos) end;              // �Զ��Ƶ�����
		for (; pos != end; ++pos)
		{
			auto it = _rand_set.insert(atoi(pos->str().c_str()));
			SPDLOG_SINKS_DEBUG("Lit-Off {}th Led", *it.first);
		}
	}

	// ֱ���������ʼ����ÿ�ŵƵ�����
	// eg: 22�ŵ������С�3,4,7,10,15���⼸�ŵƻ���������BGR�������
	if (probability > 0)
	{
		cv::RNG rng(time(NULL));
		for (int i = 0; i < I2C.getLedCount(); i++)
		{
			int r = rng.uniform(0, 101);	//[0, 101)
			if (probability >= r)
			{
				auto it = _rand_set.insert(i);
				SPDLOG_SINKS_DEBUG("Lit-Off {}th Led, RNG {}", *it.first, r);
			}
		}
	}
}

//��ǰ���Ƿ���Ҫ�ص�
bool RandomLitoff::IsLitOff(int currentIndex)
{
	//�ֶ������������
	if (_rand_set.find(currentIndex) != _rand_set.end())
	{
		return true;	//�˵�Ҫ������
	}
	//SPDLOG_SINKS_DEBUG("The {}th needn't Lit-Off", currentIndex);
	return false;	//�˵Ʋ�����������
}