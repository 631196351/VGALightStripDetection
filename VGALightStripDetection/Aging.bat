@echo off
::ping -n 60 127.0.0.1>null
setlocal enabledelayedexpansion
cd /d %~dp0

rem 设定连续测试次数
set LoopTime=1
set log=PPID.log

echo ----------------AgingBat---------------
cd ./GetVGAINFO
rem 清理旧的PPID.log文件
if exist !log! (del /q !log!)
rem 获取PPID
start /wait GetVGAINFO.exe ppid
rem 读取PPID
for /f %%i in (!log!) do (set ppid=%%i)
cd ..

if [!ppid!]==[] (        
    for /f %%i in ('powershell -command "$([guid]::NewGuid().ToString())"') do (set ppid=%%i)        
    set ppid=!ppid:~0,8!!ppid:~9,4!!ppid:~14,3!        
)

rem 获取第一个显卡料号
for /f "skip=1 delims=" %%i in ('wmic path Win32_VideoController get Name') do (
    set name=%%i
    echo !name!
    goto eof
)
:eof

for /L %%i in (1, 1, %LoopTime%) do (
    echo ----------------loop-%%i---------------
    echo !ppid!
    echo !name!
    rem 传入PPID
    start /b /wait VGALightStripDetection.exe %ppid% "%name%"
)

pause