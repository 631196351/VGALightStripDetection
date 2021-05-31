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

	//设定随机灭灯状态, 设定手动关灯列表
	void setRandomLitOffState(int probability, std::string manualset);

	//是否开启了随机灭灯
	inline bool getRandomLitOffState() const { return _bRlitOffState; }

	//当前灯是否需要关掉
	bool IsLitOff(int currentIndex);

	static RandomLitoff& getInstance()
	{
		static RandomLitoff instance;
		return instance;
	}
};

#define litoff RandomLitoff::getInstance()
