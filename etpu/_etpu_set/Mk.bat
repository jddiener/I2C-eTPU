echo off
setlocal

set ETEC_PATH="C:\Program Files (x86)\ASH WARE\eTPU Compiler V2_62D"

set CC=%ETEC_PATH%\ETEC_cc.exe
set ASM=%ETEC_PATH%\ETEC_asm.exe
set LINK=%ETEC_PATH%\ETEC_link.exe

:DoneCheckPathing

echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
echo RUNNING:  %CD%\Mk.bat  AT  %TIME%
echo ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

mkdir obj

echo Building eTPU code ...

%CC% etec_i2c_master.c -globalscratchpad -out=obj\etec_i2c_master.eao
if  %ERRORLEVEL% NEQ 0 ( goto errors )
%CC% etec_i2c_slave.c -globalscratchpad -out=obj\etec_i2c_slave.eao
if  %ERRORLEVEL% NEQ 0 ( goto errors )

%LINK% obj\etec_i2c_master.eao obj\etec_i2c_slave.eao -out=etpu_set.elf -etba=0x0 -CodeSize=0x1800 -map -lst
if  %ERRORLEVEL% NEQ 0 ( goto errors )

%LINK% obj\etec_i2c_master.eao obj\etec_i2c_slave.eao -out=etpu_c_set.elf -etba=0x0 -CodeSize=0x1800 -map -GM=C_
if  %ERRORLEVEL% NEQ 0 ( goto errors )

echo .
echo BUILD PASSES

goto end
:errors
echo *************************************************
echo        YIKES, WE GOT ERRORS!!
echo *************************************************
exit /b -1
:end

