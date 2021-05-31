#pragma once
#include <set>

class RandomLitoff
{
private:
	//int					 _ledCount = 0;
	std::set<int>		 _rand_set;
	bool				 _bRlitOffState = false;

	RandomLitoff() {};
	~RandomLitoff() {};

	RandomLitoff(const RandomLitoff&) = delete;
	RandomLitoff& operator= (const RandomLitoff&) = delete;
public:

	//�趨������״̬, �趨�ֶ��ص��б�
	void setRandomLitOffState(int probability, std::string manualset);

	//�Ƿ�����������
	inline bool getRandomLitOffState() const { return _bRlitOffState; }

	//��ǰ���Ƿ���Ҫ�ص�
	bool IsLitOff(int currentIndex);

	static RandomLitoff& getInstance()
	{
		static RandomLitoff instance;
		return instance;
	}
};

#define litoff RandomLitoff::getInstance()
