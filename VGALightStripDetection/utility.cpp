
#include <Windows.h>
#include <tchar.h>
#include "utility.h"
#include <fstream>
#include <io.h>
#include <direct.h>
#include "PreDefine.h"
#include "SpdMultipleSinks.h"
#include "ErrorCode.h"
#include "I2CWrap.h"

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

	// 等8秒
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
	SPDLOG_SINKS_DEBUG("================PPID:{}================", ppid);
	return exitCode;
}

void createPPIDFolder(const char* ppid)
{
	int ret = 0;
	if (0 != _access(AgingFolder, 0))
	{
		ret = _mkdir(AgingFolder);   // 返回 0 表示创建成功，-1 表示失败	
		if (ret != 0)
		{
			printf("\ncreate %s dir fail-%ld\n", AgingFolder, GetLastError());
		}
	}
	char path[MAX_PATH] = { 0 };
	sprintf_s(path, MAX_PATH, "%s/%s", AgingFolder, ppid);
	ret = _mkdir(path);
	if (ret != 0)
	{
		printf("\ncreate %s dir fail-%ld\n", path, GetLastError());

	}
}

