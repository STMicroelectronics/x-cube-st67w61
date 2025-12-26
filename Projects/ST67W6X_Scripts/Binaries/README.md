## __ST67W6X Binaries__

Provided batch files automate the flashing process of the Network CoProcessor (NCP) available on the __X-NUCLEO-67W61M1__ Expansion Board using a NUCLEO-U575ZI-Q Board. They rely on STM32CubeProgrammer amd QConn_Flash command line tools.

> __IMPORTANT__
>
> 1. The delivered binaries are only usable with a locked NCP.
> 2. These scripts are intended to be used with the NUCLEO-U575ZI-Q Board and the __X-NUCLEO-67W61M1__ Expansion Board. The NUCLEO-U575ZI-Q Board must be connected to the PC via USB and the __X-NUCLEO-67W61M1__ Expansion Board must be connected to the NUCLEO-U575ZI-Q Board.
> 3. To avoid any problems, only connect the NUCLEO-U575ZI-Q Host to which the __X-NUCLEO-67W61M1__ expansion board is connected.
> 4. STM32CubeProgrammer default installation path is C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin
> 5. By default, the latest available version of the ST67W611M binary is installed. To install a specific version, open a terminal and run ___NCP_update\_**.bat \<version>___. The available versions are [2.0.89, 2.0.97].

### __Links and references__

For further information, please visit the dedicated Wiki page [ST67W611M Hardware setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup).

### __Contents__

- [NCP_update_mfg.bat](#ncp_update_mfgbat): Script to flash the Manufacturing test binary into the ST67W611M.
- [NCP_update_mission_profile_t01.bat](#ncp_update_mission_profile_t01bat): Script to flash the Network Coprocessor binary with embedded LwIP into the ST67W611M.
- [NCP_update_mission_profile_t02.bat](#ncp_update_mission_profile_t02bat): Script to flash the Network Coprocessor binary with LwIP on host into the ST67W611M.

---

### __NCP_update_mfg.bat__ {#ncp_update_mfgbat}

This script installs the Manufacturing test binary into the ST67W611M over NUCLEO-U575ZI-Q Board as follow:

1. Loads the NUCLEO-U575ZI-Q  Host with the Bootloader.bin to switch the NCP in boot mode.
2. Loads the "mfg" binaries into the NCP.
3. Loads the UART bypass NUCLEO-U575ZI-Q Host application.

__To use the Manufacturing test solution__

- Download the [ST67W6X-RCT-TOOL](https://www.st.com/en/embedded-software/x-cube-st67w61.html) to manage the manufacturing operations.
- Open the ST67W6X RCT tool.
- Select the COM Port of the NUCLEO-U575ZI-Q  Host in Basic Options > Port.
- Click on "Open UART".
- Push the Reset button of the NUCLEO-U575ZI-Q Board to reset the NCP.

The detailed NCP trace will be displayed in the box at the bottom of the ST67W6X RCT tool.

---

### __NCP_update_mission_profile_t01.bat__ {#ncp_update_mission_profile_t01bat}

This script installs the Network Coprocessor binary with embedded LwIP into the ST67W611M over NUCLEO-U575ZI-Q Board as follow:

1. Loads the NUCLEO-U575ZI-Q  Host with the Bootloader.bin to switch the NCP in boot mode.
2. Loads the "mission profile" binaries into the NCP.
3. Loads the ST67W6X_CLI NUCLEO-U575ZI-Q Host application.

> __NOTE__
>
> The mission profile binaries include LittleFS image available in `../LittleFS/littlefs/littlefs.bin` which contains some certificates and keys preloaded in the Coprocessor. These files can be modified during the host application execution using the LittleFS APIs in the ST67W6X_Network_Driver Middleware.

__To use the Network Coprocessor solution__

Open a Terminal on the COM Port of the NUCLEO-U575ZI-Q with baudrate=921600, Transmit=CRLF, Receive=CR, Local_Echo=OFF and run the following commands
```
info           // Show the NPC + Host versions
help           // Show all commands available
wifi_scan      // Run a WiFi scan to identify all nearby Access Points
```

---

### __NCP_update_mission_profile_t02.bat__ {#ncp_update_mission_profile_t02bat}

This script installs the Network Coprocessor binary with LwIP on host into the ST67W611M over NUCLEO-U575ZI-Q Board as follow:

1. Loads the NUCLEO-U575ZI-Q  Host with the Bootloader.bin to switch the NCP in boot mode.
2. Loads the "mission profile" binaries into the NCP.
3. Loads the ST67W6X_CLI_LWIP NUCLEO-U575ZI-Q Host application.

> __NOTE__
>
> The mission profile binaries include LittleFS image available in `../LittleFS/littlefs/littlefs.bin` which contains some certificates and keys preloaded in the Coprocessor. These files can be modified during the host application execution using the LittleFS APIs in the ST67W6X_Network_Driver Middleware.

__To use the Network Coprocessor solution__

Open a Terminal on the COM Port of the NUCLEO-U575ZI-Q with baudrate=921600, Transmit=CRLF, Receive=CR, Local_Echo=OFF and run the following commands
```
info           // Show the NPC + Host versions
help           // Show all commands available
wifi_scan      // Run a WiFi scan to identify all nearby Access Points
```

### License

See [LICENSE_BINARIES](LICENSE_BINARIES) for license details.
