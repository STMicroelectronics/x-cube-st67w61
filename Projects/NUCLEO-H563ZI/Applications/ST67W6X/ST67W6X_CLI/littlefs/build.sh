#!/bin/bash -
# build script
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
$script_dir/../../../../../ST67W6X_Utilities/LittleFS/mklfs/$mklfs_setup -c lfs -b 2048 -p 256 -r 256 -s 0x4000 -i littlefs.bin

# Touch the lfs_flash.c file to update its timestamp
if command -v touch > /dev/null 2>&1; then
  touch Target/lfs_flash.c >/dev/null 2>&1
fi
