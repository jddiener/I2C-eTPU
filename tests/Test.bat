setlocal
echo off

echo .
echo ********************************
echo Run I2C standalone tests ...
echo ********************************
echo .

pushd ETpu
call  Test.bat  %1  %2
popd
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo .
echo ********************************
echo Run I2C system tests ...
echo ********************************
echo .

pushd System
call  Test.bat  %1  %2
popd
if  %ERRORLEVEL% NEQ 0 ( goto errors )


echo ALL TESTS PASS

goto end
:errors
echo *************************************************
echo        YIKES, WE GOT ERRORS!!
echo *************************************************
exit /b -1
:end
