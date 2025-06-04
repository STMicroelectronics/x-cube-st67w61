#!/bin/sh

RED='\033[0;31m'
NC='\033[0m'

CUBEPROGRAMMER="/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin"
export PATH="$CUBEPROGRAMMER:$PATH"
current_dir="$(dirname "$0")"

# Check if the script is being run on Windows
if [ "$(uname -o)" = "Msys" ] || [ "$(uname -o)" = "Cygwin" ]; then
    echo -e "${RED}!!!! Error: This script cannot be run on Windows !!!!${NC}"
    exit 1
fi

# Check if the OS is Ubuntu
if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$NAME" != "Ubuntu" ]; then
        echo -e "${RED}!!!! Error: This script is only supported on Ubuntu !!!!${NC}"
        exit 1
    fi
else
    echo -e "${RED}!!!! Error: Cannot determine the operating system !!!!${NC}"
    exit 1
fi

COMADDR=$(ls /dev/serial/by-id/ | grep -i "STLINK-V3" | head -n 1)

if [ -z "$COMADDR" ]; then
  echo "COM PORT not detected"
  exit 1
fi

COMADDR=$(readlink -f "/dev/serial/by-id/$COMADDR")

stty -F $COMADDR 2000000 cs8 -cstopb -parenb
if [ $? -ne 0 ]; then
  echo "COM PORT already used"
  exit 1
fi

# Detect the board ID using STM32_Programmer_CLI and grep
BOARD_ID=$(STM32_Programmer_CLI -c port=swd mode=ur | grep 'Board\s*:' | awk -F': ' '{print $2}')

if [ -z "$BOARD_ID" ]; then
    echo "Board ID not detected"
    exit 1
fi

# Check if the board ID is recognized (NUCLEO-U575ZI-Q, NUCLEO-H7S3L8, NUCLEO-H563ZI or NUCLEO-N657X0-Q)
if [ "$BOARD_ID" != "NUCLEO-U575ZI-Q" ] && [ "$BOARD_ID" != "NUCLEO-H7S3L8" ] && [ "$BOARD_ID" != "NUCLEO-H563ZI" ] && [ "$BOARD_ID" != "NUCLEO-N657X0-Q" ]; then
    echo "Board ID not recognized"
    exit 1
fi

echo "Board ID detected: $BOARD_ID"

if [ "$BOARD_ID" = "NUCLEO-N657X0-Q" ]; then
    echo "Set the boot mode in development mode (BOOT1 switch position is 2-3, BOOT0 switch position doesn't matter)"
    echo "Press any key to continue..."
    read -r junk
    STM32_Programmer_CLI -c port=swd mode=ur -w "$current_dir/NUCLEO-N657X0-Q_Binaries/Bootloader.bin" 0x70000000 --extload "$CUBEPROGRAMMER/ExternalLoader/MX25UM51245G_STM32N6570-NUCLEO.stldr"
    if [ $? -ne 0 ]; then
      echo -e "${RED}!!!! Error detected !!!!${NC}"
      exit 1
    fi
    echo "Set the boot mode in boot from external Flash (BOOT0 switch position is 1-2 and BOOT1 switch position is 1-2)"
    echo "Press the reset button then press any key to continue..."
    read -r junk
else
    STM32_Programmer_CLI -c port=swd mode=ur -w "$current_dir/"$BOARD_ID"_Binaries/Bootloader.bin" 0x08000000 --go
    if [ $? -ne 0 ]; then
      echo -e "${RED}!!!! Error detected !!!!${NC}"
      exit 1
    fi
fi

# Configure the COM port (adjust settings as needed)
stty -F $COMADDR 2000000 cs8 -cstopb -parenb

# Add a small delay to simulate a reset
sleep 1

# Reopen the COM port to reset buffers
stty -F $COMADDR 2000000 cs8 -cstopb -parenb
echo "Buffers for $COMADDR have been flushed"

echo "NCP Flashing in progress ..."
NCP_info/QConn_Eflash-ubuntu -r --efuse --chipname=qcc743 -p $COMADDR --start=0x0 --end=0x1ff --file=../flash.bin
if [ $? -ne 0 ]; then
  echo -e "${RED}!!!! Error detected !!!!${NC}"
  exit 1
fi

python NCP_info/read_chip_info.py flash.bin
if [ $? -ne 0 ]; then
  echo -e "${RED}!!!! Error detected !!!!${NC}"
  exit 1
fi

rm -f flash.bin

echo "Press any key to continue..."
read -r junk
