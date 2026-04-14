## __ST67W6X Binaries__

Provided batch files automate the flashing process of the Network Coprocessor (NCP) available on the __X-NUCLEO-67W61M1__ Expansion Board using a NUCLEO-U575ZI-Q Board. They rely on STM32CubeProgrammer and QConn_Flash command line tools.

> [!IMPORTANT]
> 1. The delivered binaries are only usable with a locked NCP.
> 2. These scripts are intended to be used with the NUCLEO-U575ZI-Q Board and the __X-NUCLEO-67W61M1__ Expansion Board. The NUCLEO-U575ZI-Q Board must be connected to the PC via USB and the __X-NUCLEO-67W61M1__ Expansion Board must be connected to the NUCLEO-U575ZI-Q Board.
> 3. To avoid any problems, only connect the NUCLEO-U575ZI-Q Host to which the __X-NUCLEO-67W61M1__ expansion board is connected.
> 4. The default STM32CubeProgrammer installation path is `C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin`.
> 5. By default, the latest available version of the ST67W611M binary is installed. To install a specific version, open a terminal and run `NCP_update_*.bat <version>`. The available versions are [2.0.89, 2.0.97, 2.0.106].

### __Links and references__

For further information, please visit the dedicated Wiki page [ST67W611M Hardware setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup).

### __Contents__

- [NCP_update_mfg.bat](#ncp_update_mfgbat): Script to flash the Manufacturing test binary into the ST67W611M.
- [NCP_update_mission_profile_t01.bat](#ncp_update_mission_profile_t01bat): Script to flash the Network Coprocessor binary with embedded LwIP into the ST67W611M.
- [NCP_update_mission_profile_t02.bat](#ncp_update_mission_profile_t02bat): Script to flash the Network Coprocessor binary with LwIP on host into the ST67W611M.

---

### __NCP_update_mfg.bat__ {#ncp_update_mfgbat}

This script installs the Manufacturing test binary into the ST67W611M over a NUCLEO-U575ZI-Q Board as follows:

1. Loads the NUCLEO-U575ZI-Q Host with the Bootloader.bin to put the NCP in boot mode.
2. Loads the "mfg" binaries into the NCP.
3. Loads the UART bypass NUCLEO-U575ZI-Q Host application.

__To use the Manufacturing test solution__

- Download the [ST67W6X-RCT-TOOL](https://www.st.com/en/embedded-software/x-cube-st67w61.html) to manage the manufacturing operations.
- Open the ST67W6X RCT tool.
- Select the COM Port of the NUCLEO-U575ZI-Q  Host in Basic Options > Port.
- Click on "Open UART".
- Press the Reset button of the NUCLEO-U575ZI-Q Board to reset the NCP.

The detailed NCP trace will be displayed in the box at the bottom of the ST67W6X RCT tool.

---

### __NCP_update_mission_profile_t01.bat__ {#ncp_update_mission_profile_t01bat}

This script installs the Network Coprocessor binary with embedded LwIP into the ST67W611M over a NUCLEO-U575ZI-Q Board as follows:

1. Loads the NUCLEO-U575ZI-Q Host with the Bootloader.bin to put the NCP in boot mode.
2. Loads the "mission profile" binaries into the NCP.
3. Loads the ST67W6X_CLI NUCLEO-U575ZI-Q Host application.

> [!NOTE]
> The mission profile binaries include LittleFS image available in `../LittleFS/littlefs/littlefs.bin` which contains some certificates and keys preloaded in the NCP.<br/>
> These files can be modified during the host application execution using the LittleFS APIs in the ST67W6X_Network_Driver Middleware.

__To use the Network Coprocessor solution__

Open a terminal on the COM Port of the NUCLEO-U575ZI-Q with baud rate = 921600, Transmit = CRLF, Receive = CR, Local_Echo = OFF and run the following commands:
```
info           // Show the NCP + Host versions
help           // Show all commands available
wifi_scan      // Run a Wi-Fi scan to identify all nearby access points
```

---

### __NCP_update_mission_profile_t02.bat__ {#ncp_update_mission_profile_t02bat}

This script installs the Network Coprocessor binary with LwIP on host into the ST67W611M over a NUCLEO-U575ZI-Q Board as follows:

1. Loads the NUCLEO-U575ZI-Q Host with the Bootloader.bin to put the NCP in boot mode.
2. Loads the "mission profile" binaries into the NCP.
3. Loads the ST67W6X_CLI_LWIP NUCLEO-U575ZI-Q Host application.

__To use the Network Coprocessor solution__

Open a terminal on the COM Port of the NUCLEO-U575ZI-Q with baud rate = 921600, Transmit = CRLF, Receive = CR, Local_Echo = OFF and run the following commands:
```
info           // Show the NCP + Host versions
help           // Show all commands available
wifi_scan      // Run a Wi-Fi scan to identify all nearby access points
```

### License

See [LICENSE_BINARIES](LICENSE_BINARIES) for license details.
