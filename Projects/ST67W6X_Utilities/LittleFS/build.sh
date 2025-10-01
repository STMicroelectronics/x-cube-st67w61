#!/bin/bash -

# Copyright (c) 2025 STMicroelectronics.
# All rights reserved.
#
# This software is licensed under terms that can be found in the LICENSE file
# in the root directory of this software component.
# If no LICENSE file comes with this software, it is provided AS-IS.

echo build.sh : Generate littlefs binary for certificates ...

# Evaluate the OS to determine which mklfs tool to use
if [ "$(uname -o)" = "MS/Windows" ] || [ "$(uname -o)" = "Msys" ] || [ "$(uname -o)" = "Cygwin" ]; then
  mklfs_setup="mklfs.exe"
elif [ "$(uname -o)" = "GNU/Linux" ]; then
  mklfs_setup="mklfs-ubuntu"
else
  echo "Unsupported OS"
  exit 1
fi

# Get the directory of the script
script_dir="$(dirname "$0")"

# Run the mklfs.exe tool to generate the littlefs binary
$script_dir/mklfs/$mklfs_setup -c Certificates -b 4096 -p 256 -r 256 -s 0x6d000 -i ./littlefs/littlefs.bin
