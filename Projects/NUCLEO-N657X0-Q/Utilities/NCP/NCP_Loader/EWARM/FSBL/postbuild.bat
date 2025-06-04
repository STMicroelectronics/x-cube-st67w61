@ECHO OFF

set "projectdir=%1"
set "configname=%2"
set stm32tool_path=C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin
set stm32signingtoolcli=%stm32tool_path%\STM32_SigningTool_CLI.exe

set command="%stm32signingtoolcli%" -bin %projectdir%\%configname%\Exe\%configname%.bin -s -nk -of 0x80000000 -t fsbl -o %projectdir%\%configname%\Exe\%configname%-trusted.bin -hv 2.3 -dump %projectdir%\%configname%\Exe\%configname%-trusted.bin
%command%
if %errorlevel% neq 0 goto :error

:: remove the old bin file
del /F %projectdir%\%configname%\Exe\%configname%.bin

exit 0

:error
echo "%command% failed"
cmd /k
exit 1
