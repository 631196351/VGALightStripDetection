
#include "utility.h"

#if defined(_MSC_VER)
#include <direct.h>
#define GetCurrentDir _getcwd
#elif defined(__unix__)
#include <unistd.h>
#define GetCurrentDir getcwd
#else
#endif

std::string get_current_directory()
{
	char buff[260];
	GetCurrentDir(buff, 260);
	std::string current_working_directory(buff);
	return current_working_directory;
}