@echo off
setlocal enabledelayedexpansion
cd /d %~dp0

rem 设定连续测试次数
set LoopTime=1
echo ----------------Aging---------------

set log=PPID.log

cd "%~dp0/NVFlash 5.670/Windows64"

if exist !log! (del /q !log!)

start /wait nvflash.exe --rdoem !log!

for /f "tokens=1,2* delims=#" %%i in (!log!) do (
    set model_name=%%i
    set ppid=%%j
)

if "%ppid%" == "" if "%model_name%" == "" (
    cd /d "%~dp0/GetVGAINFO"

    if exist *.log (del /q *.log)

    start /wait GetVGAINFO.exe ppid

    for /f %%i in (!log!) do (set ppid=%%i)

    start /wait GetVGAINFO.exe ModelName

    for /f %%i in (BIOSModelName.log) do (set model_name=%%i)
)

echo !model_name!
echo !ppid!

cd /d %~dp0
ping -n 5 127.0.0.1

for /L %%i in (1, 1, %LoopTime%) do (
    start /b /wait VGALightStripDetection.exe !ppid! !model_name!
    rem start /b /wait VGALightStripDetection.exe "210381723300448" "ROG-STRIX-RTX3070-O8G-GAMING-WHITE-2I3S"
)

rem pause