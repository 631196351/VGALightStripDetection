#pragma once
#include <string>
#include <time.h>

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
	std::string PPID() const;
	std::string getPPIDFolder() const;
	inline const char* targetFolder() const { return _ppid_time.c_str(); }
	inline tm* getTimestamp() const { return localtime(&_time);}

	static VideoCard& instance();
};

#define VideoCardIns VideoCard::instance()