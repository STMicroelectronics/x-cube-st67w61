@echo off
echo prebuild.bat : FOTA header structure generation started
set "command=python $PROJ_DIR$/../../../../../../ST67W6X_Utilities/FOTA/fota_header_gen.py gen_header -o $PROJ_DIR$/../../App_FOTA/App"

%command%
IF %ERRORLEVEL% NEQ 0 goto :error
exit 0

:error
echo %command% : failed
exit 1
