@echo off

set RED=[0;31m
set NC=[0m

set CUBEPROGRAMMER="C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"
set PATH=%CUBEPROGRAMMER%;%PATH%
set "current_dir=%~dp0"

set "COMADDR="
set _ping_cmd=wmic path Win32_SerialPort
for /f "tokens=8" %%i in ('%_ping_cmd% ^| findstr /I /C:"STMicroelectronics STLink Virtual COM Port"') do set COMADDR=%%i

if "%COMADDR%" == "" echo COM PORT not detected && goto :error
set COMADDR=%COMADDR:~1,-1%

mode %COMADDR% baud=2000000 parity=N data=8 stop=1 >nul
if %ERRORLEVEL% neq 0 echo COM PORT already used && goto :error

:: Run STM32_Programmer_CLI and capture output
for /f "tokens=3" %%i in ('"STM32_Programmer_CLI.exe -c port=swd mode=ur | findstr/C:"Board""') do set BOARD_ID=%%i

if "%BOARD_ID%" == "" echo Board ID not detected && goto :error

:: Check if the board ID is recognized (NUCLEO-U575ZI-Q, NUCLEO-H7S3L8, NUCLEO-H563ZI or NUCLEO-N657X0-Q)
if not "%BOARD_ID%" == "NUCLEO-H7S3L8" if not "%BOARD_ID%" == "NUCLEO-U575ZI-Q" if not "%BOARD_ID%" == "NUCLEO-H563ZI" if not "%BOARD_ID%" == "NUCLEO-N657X0-Q" (
    echo Board ID not recognized
    goto :error
)

echo Detected %BOARD_ID% board

if "%BOARD_ID%" == "NUCLEO-N657X0-Q" (
    echo "Set the boot mode in development mode (BOOT1 switch position is 2-3, BOOT0 switch position doesn't matter)"
    echo "Press any key to continue..."
    pause >nul
    STM32_Programmer_CLI.exe -c port=swd mode=ur -w %current_dir%%BOARD_ID%_Binaries\Bootloader.bin 0x70000000 --extload %CUBEPROGRAMMER%\ExternalLoader\MX25UM51245G_STM32N6570-NUCLEO.stldr
    if %ERRORLEVEL% neq 0 goto :error
    echo "Set the boot mode in boot from external Flash (BOOT0 switch position is 1-2 and BOOT1 switch position is 1-2)"
    echo "Press the reset button then press any key to continue..."
    pause >nul
) else (
    STM32_Programmer_CLI.exe -c port=swd mode=ur -w %current_dir%%BOARD_ID%_Binaries\Bootloader.bin 0x08000000 --go
    if %ERRORLEVEL% neq 0 goto :error
)

:: Configure the COM port (adjust settings as needed)
mode %COMADDR% baud=2000000 parity=N data=8 stop=1 >nul

:: Add a small delay to simulate a reset
timeout /t 1 /nobreak >nul

:: Reopen the COM port to reset buffers
mode %COMADDR% baud=2000000 parity=N data=8 stop=1 >nul
echo Buffers for %COMADDR% have been flushed

NCP_info\QConn_Eflash.exe -r --efuse --chipname=qcc743 -p %COMADDR% --start=0x0 --end=0x1ff --file=..\flash.bin
if %ERRORLEVEL% neq 0 goto :error

NCP_info\read_chip_info.exe flash.bin
if %ERRORLEVEL% neq 0 goto :error

del flash.bin >nul

pause
exit /B

:error
echo %RED%!!!! Error detected !!!!%NC%
pause
