setlocal
echo off

rem During installation of the ASH WARE ETEC Compiler
rem environment variable, 'DEV_TOOL_ETPU_BIN', is set to point
rem to the latest installed version
set DEVTOOL=%DEV_TOOL_ETPU_BIN%\ETpuDevTool.exe
if exist %DEVTOOL% goto FOUND_DEVTOOL

echo .
echo *************************************************
echo It appears the  'DEV_TOOL_ETPU_BIN'  environment variable 
echo is not set, or not set correctly.
echo DEV_TOOL_ETPU_BIN = %DEV_TOOL_ETPU_BIN%
echo Correct this problem in order to build this demo.
echo *************************************************
pause
echo .
goto errors

:FOUND_DEVTOOL

set DEVTOOL_OPTIONS=-AutoRun -NoEnvFile -Minimize -q -lf5=Sim.log

echo .
echo DevTool:         %DEVTOOL%
echo DEVTOOL_OPTIONS: %DEVTOOL_OPTIONS%
echo CmdLineArgs:     %1  %2  %3  %4
echo .

echo Build code ...
%DEVTOOL% -p=Proj.ETpuIdeProj %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo -------------------------------------------
echo Running I2C Single-Target Tests ...

echo Deleting coverage data, etc ...
del  *.CoverageData *.report *.log  /Q

echo Running "WriteTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=WriteTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "ReadTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=ReadTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "ReadTestCoverage" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=ReadTestCoverage.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "CombinedTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=CombinedTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "TimingTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=TimingTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "MultiSlaveTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=MultiSlaveTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "AddressTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=AddressTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "ClockStretchTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=ClockStretchTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "MasterErrorTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=MasterErrorTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Running "SlaveErrorTest" ...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=SlaveErrorTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo Test for 100 percent code coverage...
%DEVTOOL% -p=Proj.ETpuIdeProj -s=CoverageTest.ETpuCommand -NoBuild %DEVTOOL_OPTIONS% %1 %2 %3 %4
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo .
echo All I2C Single-Target Tests Pass

goto end
:errors
echo *************************************************
echo        YIKES, WE GOT ERRORS!!
echo *************************************************
exit /b -1
:end
exit /b 0
