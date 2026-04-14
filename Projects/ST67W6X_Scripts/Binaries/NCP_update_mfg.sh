#!/bin/sh

RED='\033[0;31m'
NC='\033[0m'

CUBEPROGRAMMER="/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin"
export PATH="$CUBEPROGRAMMER:$PATH"
current_dir="$(dirname "$0")"
NCP_BIN_DIR="$current_dir/NCP_Binaries"
DEFAULT_CONFIG_FILE="mfg_flash_prog_cfg.ini"
CONFIG_FILE="mfg_flash_prog_cfg_auto.ini"
BINARY_PREFIX="st67w611m_mfg"

cleanup_generated_config() {
    if [ -n "$generated_config_path" ] && [ -f "$generated_config_path" ]; then
        rm -f "$generated_config_path"
    fi
}

generated_config_path=""
trap cleanup_generated_config EXIT

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

# === Optional argument to select specific version of SDK ===
SDK_VERSION="$1"
if [ -n "$SDK_VERSION" ]; then
    if [ -f "$NCP_BIN_DIR/${BINARY_PREFIX}_v${SDK_VERSION}.bin" ]; then
        selected_version="$SDK_VERSION"
    else
        echo "Requested SDK version $SDK_VERSION is not available in $NCP_BIN_DIR"
        exit 1
    fi
else
    selected_version=$(ls "$NCP_BIN_DIR"/${BINARY_PREFIX}_v*.bin 2>/dev/null | sed -n "s#^.*/${BINARY_PREFIX}_v\([0-9][0-9.]*\)\.bin\$#\1#p" | sort -V | tail -n 1)
    if [ -z "$selected_version" ]; then
        echo "No compliant mfg binaries found in $NCP_BIN_DIR"
        exit 1
    fi
fi

selected_binary="${BINARY_PREFIX}_v${selected_version}.bin"
echo "Selected mfg binary: $selected_binary"

generated_config_path="$NCP_BIN_DIR/$CONFIG_FILE"
sed "s#^filedir[[:space:]]*=[[:space:]]*\./${BINARY_PREFIX}_v.*\.bin[[:space:]]*\$#filedir = ./${selected_binary}#" \
    "$NCP_BIN_DIR/$DEFAULT_CONFIG_FILE" > "$generated_config_path"
if [ $? -ne 0 ]; then
    echo "Failed to generate $CONFIG_FILE"
    exit 1
fi

config_file="$CONFIG_FILE"

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
FLASH_RETRY=0
while true; do
    QConn_Flash/QConn_Flash_Cmd-ubuntu --port $COMADDR --config "$current_dir/NCP_Binaries/$config_file" --efuse="$current_dir/NCP_Binaries/efusedata.bin"
    if [ $? -eq 0 ]; then
        break
    fi
    if [ "$FLASH_RETRY" -ge 1 ]; then
        echo -e "${RED}!!!! Error detected !!!!${NC}"
        exit 1
    fi
    echo "QConn_Flash_Cmd failed. Retrying once..."
    FLASH_RETRY=$((FLASH_RETRY + 1))
    sleep 1
done

if [ "$BOARD_ID" = "NUCLEO-N657X0-Q" ]; then
    echo "Set the boot mode in development mode (BOOT1 switch position is 2-3, BOOT0 switch position doesn't matter)"
    echo "Press any key to continue..."
    read -r junk
    STM32_Programmer_CLI -c port=swd mode=ur -w "$current_dir/NUCLEO-N657X0-Q_Binaries/UART_bypass.bin" 0x70000000 --extload "$CUBEPROGRAMMER/ExternalLoader/MX25UM51245G_STM32N6570-NUCLEO.stldr"
    if [ $? -ne 0 ]; then
      echo -e "${RED}!!!! Error detected !!!!${NC}"
      exit 1
    fi
    echo "Set the boot mode in boot from external Flash (BOOT0 switch position is 1-2 and BOOT1 switch position is 1-2)"
    echo "Press the reset button then press any key to continue..."
    read -r junk
else
    STM32_Programmer_CLI -c port=swd mode=ur -w "$current_dir/"$BOARD_ID"_Binaries/UART_bypass.bin" 0x08000000 --go
    if [ $? -ne 0 ]; then
      echo -e "${RED}!!!! Error detected !!!!${NC}"
      exit 1
    fi
fi

echo "Press any key to continue..."
read -r junk
