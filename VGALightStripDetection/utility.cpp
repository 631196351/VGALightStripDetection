
#include <Windows.h>
#include <tchar.h>
#include "utility.h"
#include <fstream>
#include <io.h>
#include <direct.h>
#include "PreDefine.h"

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
	return exitCode;
}

typedef int(*lpLoadVenderDLL)();
typedef bool(*lpVGA_Read_IC_I2C)(UCHAR ucAddress, UCHAR reg_address, BYTE &rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags);
typedef bool(*lpVGA_Write_IC_I2C)(UCHAR ucAddress, UCHAR reg_address, UCHAR *rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags);

lpLoadVenderDLL  LOAD_VENDOR_DLL;
lpVGA_Read_IC_I2C    VGA_READ_IC_I2C;
lpVGA_Write_IC_I2C   VGA_WRITE_IC_I2C;

// LED 灯的地址
BYTE REG[22] = { 0x60, 0x63, 0x66, 0x69, 0x6c, 0x6f, 0x72, 0x75, 0x78, 0x7b, 0x7e
				, 0x81, 0x84, 0x87, 0x8a, 0x8d, 0x90, 0x93, 0x96, 0x99, 0x9c, 0x9f };

BYTE uOffset[12] = { 0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00 };

void initVGA()
{
	HINSTANCE hDLL;		// Handle to DLL
	hDLL = LoadLibrary(L"VGA_Extra_x64.dll");
	LOAD_VENDOR_DLL = (lpLoadVenderDLL)GetProcAddress(hDLL, "LoadVenderDLL");
	VGA_READ_IC_I2C = (lpVGA_Read_IC_I2C)GetProcAddress(hDLL, "VGA_Read_IC_I2C");
	VGA_WRITE_IC_I2C = (lpVGA_Write_IC_I2C)GetProcAddress(hDLL, "VGA_Write_IC_I2C");
	// 载入dll
	LOAD_VENDOR_DLL();
}

void setSignleColor(int led, BYTE r, BYTE g, BYTE b)
{
	//Set Start Address
	uOffset[0] = 0x81;
	uOffset[1] = REG[led];
	VGA_WRITE_IC_I2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address

	uOffset[0] = 3;	//rgb size
	uOffset[1] = r;
	uOffset[2] = b;
	uOffset[3] = g;
	//(UCHAR ucAddress, UCHAR reg_address, UCHAR *rData, UINT iCardNumber, Ul32 ulDDCPort, UCHAR regSize, UCHAR DataSize, Ul32 flags)
	VGA_WRITE_IC_I2C((BYTE)0xCE, (BYTE)0x03, (BYTE*)uOffset, 0, 1, 1, 4, 1);

	uOffset[0] = 0x80;
	uOffset[1] = 0x21;
	VGA_WRITE_IC_I2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address

	uOffset[0] = 0x01;
	VGA_WRITE_IC_I2C(0xCE, 0x1, (BYTE*)uOffset, 0, 1, 1, 1, 1);	//write data

	uOffset[0] = 0x80;
	uOffset[1] = 0x2F;
	VGA_WRITE_IC_I2C(0xCE, 0x0, (BYTE*)uOffset, 0, 1, 1, 2, 1);	//set address

	uOffset[0] = 0x01;
	VGA_WRITE_IC_I2C(0xCE, 0x01, (BYTE*)uOffset, 0, 1, 1, 1, 1);	//write data
}

void resetColor(int count, BYTE r, BYTE g, BYTE b)
{
	for (int i = 0; i < count; i++)
	{
		setSignleColor(i, r, g, b);
	}
}

void createPPIDFolder(const char* ppid)
{
	if (0 != _access(AgingFolder, 0))
	{
		_mkdir(AgingFolder);   // 返回 0 表示创建成功，-1 表示失败		
	}
	char path[128] = { 0 };
	sprintf_s(path, 128, "%s/%s", AgingFolder, ppid);
	_mkdir(path);
}