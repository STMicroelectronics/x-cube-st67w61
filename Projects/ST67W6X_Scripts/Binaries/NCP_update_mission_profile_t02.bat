@echo off

set RED=[0;31m
set NC=[0m

set "SCRIPT_DIR=%~dp0"
set "NCP_BIN_DIR=%SCRIPT_DIR%NCP_Binaries"
set "SDK_VERSION=%~1"
set "SELECTED_VERSION="
set "SELECTED_BINARY="
set "DEFAULT_CONFIG_FILE=mission_t02_flash_prog_cfg.ini"
set "CONFIG_FILE=mission_t02_flash_prog_cfg_auto.ini"
set "BINARY_PREFIX=st67w611m_mission_t02"

REM === Optional argument to select specific version of SDK ===
if defined SDK_VERSION goto :resolve_requested_version
goto :resolve_latest_version

:resolve_requested_version
if exist "%NCP_BIN_DIR%\%BINARY_PREFIX%_v%SDK_VERSION%.bin" (
    set "SELECTED_VERSION=%SDK_VERSION%"
) else (
    echo Requested SDK version %SDK_VERSION% is not available in %NCP_BIN_DIR%
    goto :error
)
goto :version_resolved

:resolve_latest_version
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "$ErrorActionPreference='Stop'; Get-ChildItem -LiteralPath '%NCP_BIN_DIR%' -Filter '%BINARY_PREFIX%_v*.bin' | ForEach-Object { if ($_.BaseName -match '%BINARY_PREFIX%_v(?<ver>\d+\.\d+\.\d+)$') { [version]$Matches.ver } } | Sort-Object -Descending | Select-Object -First 1 | ForEach-Object { $_.ToString() }"`) do set "SELECTED_VERSION=%%i"
if "%SELECTED_VERSION%"=="" (
    echo No compliant mission t02 binaries found in %NCP_BIN_DIR%
    goto :error
)

:version_resolved

set "SELECTED_BINARY=%BINARY_PREFIX%_v%SELECTED_VERSION%.bin"
echo Selected mission t02 binary: %SELECTED_BINARY%

powershell -NoProfile -Command "$ErrorActionPreference='Stop'; $template='%NCP_BIN_DIR%\%DEFAULT_CONFIG_FILE%'; $output='%NCP_BIN_DIR%\%CONFIG_FILE%'; $binary='%SELECTED_BINARY%'; $updated=Get-Content -LiteralPath $template | ForEach-Object { if ($_ -match '^filedir\s*=\s*\./%BINARY_PREFIX%_v.*\.bin\s*$') { 'filedir = ./' + $binary } else { $_ } }; Set-Content -LiteralPath $output -Value $updated -Encoding Ascii"
if %ERRORLEVEL% neq 0 (
    echo Failed to generate %CONFIG_FILE%
    goto :error
)

REM === Warning message ===
echo %RED%"##########################################################"%NC%
echo %RED%"# You are about to load a signed binary to the NCP."%NC%
echo %RED%"# This will lock the ST67W61M if not yet locked."%NC%
echo %RED%"##########################################################"%NC%

echo Are you sure to proceed? (Y/N)
choice /c YN /n /m "Press Y to continue or N to abort: "
if errorlevel 2 goto :abort
if errorlevel 1 goto :continue

:abort
echo Operation aborted.
call :cleanup_generated_config
exit /B

:continue
REM === Set CubeProgrammer path ===
set "CUBEPROGRAMMER=C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"
set "PATH=%CUBEPROGRAMMER%;%PATH%"
set "current_dir=%SCRIPT_DIR%"

REM === Find the STLink Virtual COM Port using PowerShell ===
set "COMADDR="
for /f "usebackq delims=" %%i in (`powershell -Command "Get-WmiObject Win32_SerialPort | Where-Object { $_.Description -like '*STMicroelectronics STLink Virtual COM Port*' } | Select-Object -ExpandProperty DeviceID"`) do set "COMADDR=%%i"

if "%COMADDR%"=="" (
    echo COM PORT not detected
    goto :error
)

REM === Set baud rate and check for errors ===
mode %COMADDR% baud=2000000 parity=N data=8 stop=1 >nul
if %ERRORLEVEL% neq 0 (
    echo COM PORT already used
    goto :error
)

:: Run STM32_Programmer_CLI and capture output
for /f "tokens=3" %%i in ('"STM32_Programmer_CLI.exe -c port=swd mode=ur | findstr/C:"Board""') do set BOARD_ID=%%i

if "%BOARD_ID%" == "" echo Board ID not detected && goto :error

:: Replace NUCLEO-H7R3L8 with NUCLEO-H7S3L8
if "%BOARD_ID%" == "NUCLEO-H7R3L8" (
    set BOARD_ID=NUCLEO-H7S3L8
)

:: If the board ID is STLINK-V3SET or STLINK-V3MINI, request user input to select the board manually
if "%BOARD_ID%" == "STLINK-V3SET" goto :manual_select
if "%BOARD_ID%" == "STLINK-V3MINI" goto :manual_select

goto :next
:manual_select
echo .
echo STLINK-V3 detected. Please select the board manually:
echo [1] NUCLEO-U575ZI-Q
echo [2] NUCLEO-H7S3L8
echo [3] NUCLEO-H563ZI
echo [4] NUCLEO-N657X0-Q
set /p choice="Enter your choice (1-4): "
if "%choice%" == "1" set BOARD_ID=NUCLEO-U575ZI-Q
if "%choice%" == "2" set BOARD_ID=NUCLEO-H7S3L8
if "%choice%" == "3" set BOARD_ID=NUCLEO-H563ZI
if "%choice%" == "4" set BOARD_ID=NUCLEO-N657X0-Q

:next
:: Check if the board ID is recognized (NUCLEO-U575ZI-Q, NUCLEO-H7S3L8, NUCLEO-H563ZI or NUCLEO-N657X0-Q)
if not "%BOARD_ID%" == "NUCLEO-H7S3L8" if not "%BOARD_ID%" == "NUCLEO-U575ZI-Q" if not "%BOARD_ID%" == "NUCLEO-H563ZI" if not "%BOARD_ID%" == "NUCLEO-N657X0-Q" (
    echo Board ID %BOARD_ID% not recognized
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

echo NCP Flashing in progress ...
set /a FLASH_RETRY=0

:run_qconn_flash
%current_dir%QConn_Flash\QConn_Flash_Cmd.exe --port %COMADDR% --config %current_dir%NCP_Binaries\%CONFIG_FILE% --efuse=%current_dir%\NCP_Binaries\efusedata.bin
if %ERRORLEVEL% equ 0 goto :qconn_flash_done
if %FLASH_RETRY% geq 1 goto :error
echo QConn_Flash_Cmd failed. Retrying once...
set /a FLASH_RETRY+=1
timeout /t 1 /nobreak >nul
goto :run_qconn_flash

:qconn_flash_done

if "%BOARD_ID%" == "NUCLEO-N657X0-Q" (
    echo "Set the boot mode in development mode (BOOT1 switch position is 2-3, BOOT0 switch position doesn't matter)"
    echo "Press any key to continue..."
    pause >nul
    STM32_Programmer_CLI.exe -c port=swd mode=ur -w %current_dir%%BOARD_ID%_Binaries\ST67W6X_CLI_LWIP_FSBL.bin 0x70000000 --extload %CUBEPROGRAMMER%\ExternalLoader\MX25UM51245G_STM32N6570-NUCLEO.stldr
    if %ERRORLEVEL% neq 0 goto :error
    echo "Set the boot mode in boot from external Flash (BOOT0 switch position is 1-2 and BOOT1 switch position is 1-2)"
    echo "Press the reset button then press any key to continue..."
    pause >nul
) else if "%BOARD_ID%" == "NUCLEO-H7S3L8" (
    STM32_Programmer_CLI.exe -c port=swd mode=ur -w %current_dir%%BOARD_ID%_Binaries\ST67W6X_CLI_LWIP_Boot.bin 0x08000000
    if %ERRORLEVEL% neq 0 goto :error
    STM32_Programmer_CLI.exe -c port=swd mode=ur -w %current_dir%%BOARD_ID%_Binaries\ST67W6X_CLI_LWIP_Appli.bin 0x70000000 --extload %CUBEPROGRAMMER%\ExternalLoader\MX25UW25645G_NUCLEO-H7S3L8.stldr
    if %ERRORLEVEL% neq 0 goto :error
) else (
    STM32_Programmer_CLI.exe -c port=swd mode=ur -w %current_dir%%BOARD_ID%_Binaries\ST67W6X_CLI_LWIP.bin 0x08000000 --go
    if %ERRORLEVEL% neq 0 goto :error
)

pause
call :cleanup_generated_config
exit /B

:error
echo %RED%!!!! Error detected !!!!%NC%
call :cleanup_generated_config
pause
exit /B

:cleanup_generated_config
if exist "%NCP_BIN_DIR%\%CONFIG_FILE%" del /q "%NCP_BIN_DIR%\%CONFIG_FILE%"
exit /B
