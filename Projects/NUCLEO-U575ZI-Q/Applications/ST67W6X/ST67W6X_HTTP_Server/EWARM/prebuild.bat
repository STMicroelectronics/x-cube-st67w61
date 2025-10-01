@echo off
echo prebuild.bat : HTML generation started
set "command=python $PROJ_DIR$/../../Html/html_to_h.py --config $PROJ_DIR$/../../Html/conf.txt --output $PROJ_DIR$/../../Appli/App"

%command%
IF %ERRORLEVEL% NEQ 0 goto :error
exit 0

:error
echo %command% : failed
exit 1
