# Overview

Provided batch files automate the flashing process of the Network CoProcessor (NCP) available on the __X-NUCLEO-67W61M1__ Expansion Board using a NUCLEO-U575ZI-Q Board. They rely on STM32CubeProgrammer amd QConn_Flash command line tools.

> __IMPORTANT__
>
> 1. The delivered binaries are only usable with a locked NCP.
> 2. These scripts are intended to be used with the NUCLEO-U575ZI-Q Board and the __X-NUCLEO-67W61M1__ Expansion Board. The NUCLEO-U575ZI-Q Board must be connected to the PC via USB and the __X-NUCLEO-67W61M1__ Expansion Board must be connected to the NUCLEO-U575ZI-Q Board.
> 3. To avoid any problems, only connect the NUCLEO-U575ZI-Q Host to which the __X-NUCLEO-67W61M1__ expansion board is connected.
> 4. STM32CubeProgrammer default installation path is C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin

## __Links and references__

For further information, please visit the dedicated Wiki page [ST67W611M Hardware setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup).

## NCP_update_mfg.bat

This script installs the Manufacturing test binary into the ST67W611M over NUCLEO-U575ZI-Q Board as follow:

1. Loads the NUCLEO-U575ZI-Q  Host with the Bootloader.bin to switch the NCP in boot mode.
2. Loads the "mfg" binaries into the NCP.
3. Loads the UART bypass NUCLEO-U575ZI-Q  Host application.

__To use the Manufacturing test solution__

Download the [QConn RCT Tool](https://git.codelinaro.org/clo/qcc7xx/QCCSDK-QCC74x/-/blob/2.0.61.1/tools/qcc74x_tools/QConn_RCT/QConn_RCT.exe) to manage the manufacturing operations.

Open the QConn_RCT tool:

- select the COM Port of the NUCLEO-U575ZI-Q  Host in Basic Options > Port
- Click on "Open Uart" then push the Reset button of the NUCLEO-U575ZI-Q Board to reset the NCP.

The detailed NCP trace will be displayed in the box at the bottom of the QConn_RCT tool.


## NCP_update_mission_profile.bat

This script installs the Network Coprocessor binary into the ST67W611M over NUCLEO-U575ZI-Q Board as follow:

1. Loads the NUCLEO-U575ZI-Q  Host with the Bootloader.bin to switch the NCP in boot mode.
2. Loads the "mission profile" binaries into the NCP.
3. Loads the ST67W6X_CLI NUCLEO-U575ZI-Q  Host application.

__To use the Network Coprocessor solution__

Open a Terminal on the COM Port of the NUCLEO-U575ZI-Q with baudrate=921600, Transmit=CRLF, Receive=CR, Local_Echo=OFF and run the following commands
```
info           // Show the NPC + Host versions
help           // Show all commands available
wifi_scan      // Run a WiFi scan to identify all nearby Access Points
```
