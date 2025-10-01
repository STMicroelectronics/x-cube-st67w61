@echo off

:: Copyright (c) 2025 STMicroelectronics.
:: All rights reserved.
::
:: This software is licensed under terms that can be found in the LICENSE file
:: in the root directory of this software component.
:: If no LICENSE file comes with this software, it is provided AS-IS.

echo build.bat : Generate littlefs binary for certificates ...

REM Get the littlefs directory of the script
set "SCRIPT_DIR=%~dp0"

REM Run the mklfs.exe tool to generate the littlefs binary
%SCRIPT_DIR%\mklfs\mklfs.exe -c Certificates -b 4096 -p 256 -r 256 -s 0x6d000 -i ./littlefs/littlefs.bin
