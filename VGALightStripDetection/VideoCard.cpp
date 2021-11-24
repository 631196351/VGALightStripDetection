#include <Windows.h>
#include <fstream>
#include "VideoCard.h"

VideoCard::VideoCard()
{
	time(&_time);
	system("del /F ppid.txt 2>nul");
}

VideoCard::~VideoCard()
{

}

void VideoCard::PPID(std::string ppid)
{
	_ppid = ppid;
	if (_ppid.empty())
	{
		char p[64] = { 0 };
		GUID guid;
		CoCreateGuid(&guid);

		snprintf(p, 64, "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1],
			guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
		_ppid = p;
	}

	struct tm *p = localtime(&_time);
	char f[_MAX_PATH] = { 0 };
	sprintf_s(f, _MAX_PATH, "%d%02d%02d%02d%02d%02d_%s", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, _ppid.c_str());
	_ppid_time = f;
}

void VideoCard::Name(std::string name)
{
	if (name.empty())
		_name = "NA";
	else
		_name = name;
}

//std::string VideoCard::PPID() const
//{
//	return _ppid;
//}

std::string VideoCard::getPPIDFolder() const
{
	return _ppid_time;
}

VideoCard& VideoCard::instance()
{
	static VideoCard ri;
	return ri;
}

/// 应PE 要求, 将PPID 写入到程式同级目录的ppid.txt中
void VideoCard::savePPID()
{
	std::fstream file("./ppid.txt", std::fstream::out);
	if (file.is_open())
	{
		file << _ppid;
		file.flush();
	}
	file.close();
}