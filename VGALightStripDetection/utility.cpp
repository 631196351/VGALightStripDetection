
#include <Windows.h>
#include <tchar.h>
#include "utility.h"
#include <fstream>

int getVGAInfo(char* ppid, size_t size)
{
	DWORD exitCode = 0;
	TCHAR cmd[MAX_PATH] = { 0 };
	TCHAR args[MAX_PATH] = { 0 };
	TCHAR directory[MAX_PATH] = { 0 };
	SHELLEXECUTEINFO ShExecInfo = { 0 };

	_tremove(L".\\GetVGAINFO\\PPID.log");

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.lpDirectory = NULL;
#if _DEBUG
	ShExecInfo.nShow = SW_SHOW;
#else 
	ShExecInfo.nShow = SW_HIDE;
#endif
	ShExecInfo.hInstApp = NULL;

	_stprintf_s(args, MAX_PATH, L"ppid");
	ShExecInfo.lpParameters = args;

	_stprintf_s(cmd, MAX_PATH, L".\\GetVGAINFO\\GetVGAINFO.exe");
	ShExecInfo.lpFile = cmd;

	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
	GetExitCodeProcess(ShExecInfo.hProcess, &exitCode);

	// µÈ8Ãë
	//_tsystem(L"choice /t 8 /d y /n > nul");

	//unsigned int ppid = 0;
	std::fstream file(".\\GetVGAINFO\\PPID.log", std::fstream::in);
	if (file.good())
	{
		//char buf[MAXCHAR] = { 0 };
		file.getline(ppid, size);
		//sscanf_s(buf, "%u", &ppid);		
	}
	file.close();
	return exitCode;
}