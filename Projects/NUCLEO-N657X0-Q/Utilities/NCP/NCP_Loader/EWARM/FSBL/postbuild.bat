@ECHO OFF

set "projectdir=%1"
set "configname=%2"
set stm32tool_path=C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin
set stm32signingtoolcli=%stm32tool_path%\STM32_SigningTool_CLI.exe

:: # Check SignTool version. If >= 2.21.0, add --align parameter during signing
for /f "tokens=2 delims=v" %%a in ('"%stm32signingtoolcli%" --version') do set version=%%a
for /f "tokens=1,2,3 delims=." %%a in ("%version%") do (
    set major=%%a
    set minor=%%b
    set patch=%%c
)
set use_align=0
if %major% GEQ 2 (
    if %minor% GEQ 21 (
        set use_align=1
    )
) else if %major% GEQ 3 (
    set use_align=1
)
if %use_align% EQU 1 (
    set command="%stm32signingtoolcli%" -bin %projectdir%\%configname%\Exe\%configname%.bin -s -nk -of 0x80000000 -t fsbl -o %projectdir%\%configname%\Exe\%configname%-trusted.bin -hv 2.3 --align -dump %projectdir%\%configname%\Exe\%configname%-trusted.bin
) else (
    set command="%stm32signingtoolcli%" -bin %projectdir%\%configname%\Exe\%configname%.bin -s -nk -of 0x80000000 -t fsbl -o %projectdir%\%configname%\Exe\%configname%-trusted.bin -hv 2.3 -dump %projectdir%\%configname%\Exe\%configname%-trusted.bin
)
%command%
if %errorlevel% neq 0 goto :error

:: remove the old bin file
del /F %projectdir%\%configname%\Exe\%configname%.bin

exit 0

:error
echo "%command% failed"
cmd /k
exit 1
