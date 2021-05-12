@echo off
::ping -n 60 127.0.0.1>null
setlocal enabledelayedexpansion
cd /d %~dp0

rem 设定连续测试次数
set LoopTime=1
set log=ppid.txt

cd "./NVFlash 5.670/Windows64"
rem 清理旧的PPID.log文件
if exist !log! (del /q !log!)
rem 获取PPID
start /wait nvflash.exe --rdoem !log!
rem 读取PPID
for /f "tokens=1,2* delims=#" %%i in (!log!) do (
    set model_name=%%i
    set ppid=%%j
)
cd /d %~dp0

ping -n 3 127.0.0.1
for /L %%i in (1, 1, %LoopTime%) do (
    echo !model_name!
    echo !ppid!
    start /b /wait VGALightStripDetection.exe !ppid! !model_name!
)
pause