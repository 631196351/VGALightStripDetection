#include <Windows.h>
#include <fstream>
#include "VideoCard.h"
#include "ErrorCode.h"

VideoCard::VideoCard()
{
	time(&_time);
	system("del /F ppid.txt 2>nul");
}

VideoCard::~VideoCard()
{

}

void VideoCard::PPID(std::string ppid_file_path)
{

	std::fstream file(ppid_file_path, std::fstream::in);
	if (file.is_open())
	{
		file >> _ppid;
	}
	file.close();

	if (_ppid.empty())
	{
		char e[_MAX_PATH] = { 0 };
		sprintf_s(e, _MAX_PATH, "PPID Empty! %s", ppid_file_path.c_str());
		throw ErrorCodeEx(ERR_PPID_EMPTY, e);
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