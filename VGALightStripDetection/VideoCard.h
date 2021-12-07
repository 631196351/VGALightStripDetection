#pragma once
#include <string>
#include <time.h>

/// 在plan-c 分支中, VideoCard 其实是面向 ROG-WINGWALL-HOLDER 来做的
/// 这里的ppid其实就是每个 ROG-WINGWALL-HOLDER 自身的ppid
class VideoCard
{
	time_t _time;
	std::string _ppid;
	std::string _name;
	std::string _ppid_time;
public:
	VideoCard();
	~VideoCard();

	void PPID(std::string ppid);
	void Name(std::string name);
	inline std::string Name() const { return _name; }
	inline std::string PPID() const { return _ppid; }
	std::string getPPIDFolder() const;
	inline const char* targetFolder() const { return _ppid_time.c_str(); }
	inline tm* getTimestamp() const { return localtime(&_time);}

	void savePPID();
	static VideoCard& instance();
};

#define VideoCardIns VideoCard::instance()