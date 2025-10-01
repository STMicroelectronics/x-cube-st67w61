@echo off
REM build script
echo build.bat : Generate littlefs binary for certificates ...

REM Get the littlefs directory of the script
set "SCRIPT_DIR=%~dp0"

REM Run the mklfs.exe tool to generate the littlefs binary
%SCRIPT_DIR%..\..\..\..\..\ST67W6X_Utilities\LittleFS\mklfs\mklfs.exe -c lfs -b 2048 -p 256 -r 256 -s 0x4000 -i littlefs.bin
