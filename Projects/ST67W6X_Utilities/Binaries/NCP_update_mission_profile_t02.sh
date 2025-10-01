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

echo -e "${RED}##########################################################${NC}"
echo -e "${RED}# You are about to load a signed binary to the NCP.${NC}"
echo -e "${RED}# This will lock the ST67W61M if not yet locked.${NC}"
echo -e "${RED}##########################################################${NC}"

echo "Are you sure to proceed? (Y/N)"
read -r response
if [ "$response" != "Y" ] && [ "$response" != "y" ]; then
  echo "Exiting..."
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

# Replace NUCLEO-H7R3L8 with NUCLEO-H7S3L8
if [ "$BOARD_ID" = "NUCLEO-H7R3L8" ]; then
    BOARD_ID="NUCLEO-H7S3L8"
fi

# If the board ID is STLINK-V3SET or STLINK-V3MINI, request user input to select the board manually
if [ "$BOARD_ID" = "STLINK-V3SET" ] || [ "$BOARD_ID" = "STLINK-V3MINI" ]; then
    echo "STLINK-V3 detected. Please select the board manually:"
    echo "[1] NUCLEO-U575ZI-Q"
    echo "[2] NUCLEO-H7S3L8"
    echo "[3] NUCLEO-H563ZI"
    echo "[4] NUCLEO-N657X0-Q"
    echo "Enter your choice (1-4):"
    read -r choice
    if [ "$choice" = "1" ]; then
        BOARD_ID="NUCLEO-U575ZI-Q"
    elif [ "$choice" = "2" ]; then
        BOARD_ID="NUCLEO-H7S3L8"
    elif [ "$choice" = "3" ]; then
        BOARD_ID="NUCLEO-H563ZI"
    elif [ "$choice" = "4" ]; then
        BOARD_ID="NUCLEO-N657X0-Q"
    fi
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
QConn_Flash/QConn_Flash_Cmd-ubuntu --port $COMADDR --config "$current_dir/NCP_Binaries/mission_t02_flash_prog_cfg.ini" --efuse="$current_dir/NCP_Binaries/efusedata.bin"
if [ $? -ne 0 ]; then
  echo -e "${RED}!!!! Error detected !!!!${NC}"
  exit 1
fi

if [ "$BOARD_ID" = "NUCLEO-N657X0-Q" ]; then
    echo "ST67W6X_CLI_LWIP not available for NUCLEO-N657X0-Q"
    echo "Press any key to continue..."
    read -r junk
elif [ "$BOARD_ID" = "NUCLEO-H7S3L8" ]; then
    STM32_Programmer_CLI -c port=swd mode=ur -w "$current_dir/"$BOARD_ID"_Binaries/ST67W6X_CLI_LWIP_Boot.bin" 0x08000000
    if [ $? -ne 0 ]; then
      echo -e "${RED}!!!! Error detected !!!!${NC}"
      exit 1
    fi
    STM32_Programmer_CLI -c port=swd mode=ur -w "$current_dir/"$BOARD_ID"_Binaries/ST67W6X_CLI_LWIP_Appli.bin" 0x70000000 --extload "$CUBEPROGRAMMER/ExternalLoader/MX25UW25645G_NUCLEO-H7S3L8.stldr"
    if [ $? -ne 0 ]; then
      echo -e "${RED}!!!! Error detected !!!!${NC}"
      exit 1
    fi
else
    STM32_Programmer_CLI -c port=swd mode=ur -w "$current_dir/"$BOARD_ID"_Binaries/ST67W6X_CLI_LWIP.bin" 0x08000000 --go
    if [ $? -ne 0 ]; then
      echo -e "${RED}!!!! Error detected !!!!${NC}"
      exit 1
    fi
fi

echo "Press any key to continue..."
read -r junk
