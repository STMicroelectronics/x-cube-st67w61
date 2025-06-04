#!/bin/bash - 
#prebuild script 
echo prebuild.sh : HTML generation started
command="python $1/../Html/html_to_h.py --config $1/../Html/conf.txt --output $1/../App_WiFi_Commissioning/App"
echo $command
$command
ret=$?

if [ $ret != 0 ]; then
  #an error
  echo $command : failed
  exit 1
fi

exit 0