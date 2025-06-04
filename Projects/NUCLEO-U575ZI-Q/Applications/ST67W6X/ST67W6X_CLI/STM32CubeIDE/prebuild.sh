#!/bin/bash -
#prebuild script
echo prebuild.sh : FOTA header structure generation started
command="python $1/../../../../../ST67W6X_Utilities/FOTA/fota_header_gen.py gen_header -o $1/../App_CLI/App"
echo $command
$command
ret=$?

if [ $ret != 0 ]; then
  #an error
  echo $command : failed
  exit 1
fi

exit 0
