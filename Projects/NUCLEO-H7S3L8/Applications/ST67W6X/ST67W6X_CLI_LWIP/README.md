## __ST67W6X_CLI_LWIP Application Description__

This application aims to evaluate and to test the X-NUCLEO-67W61M1 Wi-Fi, Bluetooth LE and Network interface with LwIP solutions via command line interface (CLI).

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

The application allows to perform some basic Wi-Fi operations like scanning available local access points (AP), connecting to an AP, but also to test network functionalities like Ping, DHCP, Socket.

> [!IMPORTANT]
> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t02_v2.0.106.bin.<br/>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Scripts/Binaries/README.md) instructions using the __NCP_update_mission_profile_t02.bat__ script.

### __Keywords__

Connectivity, WiFi, BLE, ST67W6X_Network_Driver, FreeRTOS, LwIP, CLI, Station mode, Soft Access Point mode, DHCP, Ping, Echo, Scan, TCP, UDP, WPA2, WPA3

### __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_CLI](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_ST67W6X_CLI_Application).

### __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI_LWIP/Appli/Appli/App/|                                               |Main application code directory|
|ST67W6X_CLI_LWIP/Appli/Appli/Target/|                                            |Logging, Shell, Low-Power and BSP interfaces|
|ST67W6X_CLI_LWIP/Appli/Core/Src|                                                 |STM32CubeMX generated sources code|
|ST67W6X_CLI_LWIP/Appli/Core/Inc|                                                 |STM32CubeMX generated header files|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/App|                                              |Entry point to start the application associated to the ST67W6X_Network_Driver Middleware|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target|                                           |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_CLI_LWIP/Appli/LWIP/App|                                                 |LwIP application, LwIP shell commands, netif interface and TLS socket adaptation|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target|                                              |Configuration files to manage the LwIP Middleware|
|ST67W6X_CLI_LWIP/Appli/littlefs/lfs|                                             |Certificates used to execute secure operations|
|ST67W6X_CLI_LWIP/Appli/littlefs/Target|                                          |Configuration and port files to manage the littlefs in flash|
|ST67W6X_CLI_LWIP/EWARM|                                                          |Project for the IAR Embedded workbench for Arm|
|ST67W6X_CLI_LWIP/MDK-ARM|                                                        |Project for the RealView Microcontroller Development Kit|
|ST67W6X_CLI_LWIP/STM32CubeIDE|                                                   |Project for the STM32CubeIDE toolchain|

### __Directory contents__

|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI_LWIP/Appli/Appli/App/app_config.h|                                   |Configuration for main application|
|ST67W6X_CLI_LWIP/Appli/Appli/App/main_app.h|                                     |Header for main_app.c|
|ST67W6X_CLI_LWIP/Appli/Appli/Target/logshell_ctrl.h|                             |Header for logshell_ctrl module|
|ST67W6X_CLI_LWIP/Appli/Core/Inc/FreeRTOSConfig.h|                                |Header for FreeRTOS application specific definitions|
|ST67W6X_CLI_LWIP/Appli/Core/Inc/main.h|                                          |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_CLI_LWIP/Appli/Core/Inc/stm32h7rsxx_hal_conf.h|                          |HAL configuration template file.<br>This file should be copied to the application folder and renamed<br>to stm32h7rsxx_hal_conf.h.|
|ST67W6X_CLI_LWIP/Appli/Core/Inc/stm32h7rsxx_it.h|                                |This file contains the headers of the interrupt handlers.|
|ST67W6X_CLI_LWIP/Appli/littlefs/Target/easyflash.h|                              |Header file that adapts LittleFS to EasyFlash4|
|ST67W6X_CLI_LWIP/Appli/littlefs/Target/lfs_port.h|                               |lfs flash port definition|
|ST67W6X_CLI_LWIP/Appli/littlefs/Target/lfs_util_config.h|                        |lfs utility user configuration|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/altls_mbedtls.h|                                 |Application layered TCP/TLS connection API|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/dhcp_server.h|                                   |DHCP Server definition|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/echo.h|                                          |Echo test definition|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/http_client.h|                                   |Simple HTTP client API|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/lwip.h|                                          |This file provides code for the configuration of the LWIP.|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/lwip_netif.h|                                    |This file provides code for the configuration of the ST67W6X Network interface over LwIP|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/ping.h|                                          |Ping module definition|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/sntp.h|                                          |This is simple "SNTP" client definition|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/lwipopts.h|                                   |This file overrides LwIP stack default configuration|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/bpstruct.h|                              |Packed structs support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/cc.h|                                    |specific compiler functions support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/cpu.h|                                   |specific architecture definitions<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/epstruct.h|                              |Packed structs support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/init.h|                                  |modules initializations<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/lib.h|                                   |specific libraries support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/perf.h|                                  |perf measurement support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/LWIP/Target/arch/sys_arch.h|                              |specific os functions support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/App/app_st67w6x.h|                                |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target/bsp_conf.h|                                |This file contains definitions for the BSP interface|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target/logging_config.h|                          |Header file for the W6X Logging configuration module|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target/shell_config.h|                            |Header file for the W6X Shell configuration module|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target/w61_driver_config.h|                       |Header file for the W61 configuration module|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target/w6x_config.h|                              |Header file for the W6X configuration module|
|ST67W6X_CLI_LWIP/Boot/Core/Inc/extmem_manager.h|                                 |Header for secure_manager_api.c file.|
|ST67W6X_CLI_LWIP/Boot/Core/Inc/main.h|                                           |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_CLI_LWIP/Boot/Core/Inc/stm32h7rsxx_hal_conf.h|                           |HAL configuration template file.<br>This file should be copied to the application folder and renamed<br>to stm32h7rsxx_hal_conf.h.|
|ST67W6X_CLI_LWIP/Boot/Core/Inc/stm32h7rsxx_it.h|                                 |This file contains the headers of the interrupt handlers.|
|ST67W6X_CLI_LWIP/Boot/Core/Inc/stm32_extmem_conf.h|                              |Header for extmem.c file.|
|      |                                                                          |           |
|ST67W6X_CLI_LWIP/Appli/Appli/App/main_app.c|                                     |main_app program body|
|ST67W6X_CLI_LWIP/Appli/Appli/Target/logshell_ctrl.c|                             |logshell_ctrl (uart interface)|
|ST67W6X_CLI_LWIP/Appli/Core/Src/freertos.c|                                      |Code for freertos applications|
|ST67W6X_CLI_LWIP/Appli/Core/Src/main.c|                                          |Main program body|
|ST67W6X_CLI_LWIP/Appli/Core/Src/stm32h7rsxx_hal_msp.c|                           |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_CLI_LWIP/Appli/Core/Src/stm32h7rsxx_hal_timebase_tim.c|                  |Template for HAL time base based on the peripheral hardware TIM1.|
|ST67W6X_CLI_LWIP/Appli/Core/Src/stm32h7rsxx_it.c|                                |Interrupt Service Routines.|
|ST67W6X_CLI_LWIP/Appli/Core/Src/system_stm32h7rsxx.c|                            |CMSIS Cortex-M7 Device Peripheral Access Layer System Source File|
|ST67W6X_CLI_LWIP/Appli/littlefs/Target/lfs_easyflash.c|                          |Adapts LittleFS to EasyFlash4|
|ST67W6X_CLI_LWIP/Appli/littlefs/Target/lfs_flash.c|                              |Host flash interface|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/altls_mbedtls.c|                                 |Application layered TLS connection API|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/dhcp_server_raw.c|                               |A simple DHCP server implementation|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/echo.c|                                          |Test an echo with a server|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/http_client.c|                                   |This file provides code fora simple HTTP client API|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/lwip.c|                                          |This file provides initialization code for LwIP middleware.|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/lwip_netif.c|                                    |This file provides initialization code for ST67W6X Network interface over LwIP|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/lwip_shell.c|                                    |This file provides code for LwIP Shell Commands|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/ping.c|                                          |Ping application|
|ST67W6X_CLI_LWIP/Appli/LWIP/App/sntp.c|                                          |This is simple "SNTP" client for the lwIP raw API.<br>It is a minimal implementation of SNTPv4 as specified in RFC 4330.|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/App/app_st67w6x.c|                                |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target/spi_port.c|                                |SPI bus interface porting layer implementation|
|ST67W6X_CLI_LWIP/Appli/ST67W6X/Target/util_task_port.c|                          |Task Performance porting layer implementation|
|ST67W6X_CLI_LWIP/Boot/Core/Src/extmem_manager.c|                                 |This file implements the extmem configuration|
|ST67W6X_CLI_LWIP/Boot/Core/Src/main.c|                                           |Main program body|
|ST67W6X_CLI_LWIP/Boot/Core/Src/stm32h7rsxx_hal_msp.c|                            |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_CLI_LWIP/Boot/Core/Src/stm32h7rsxx_it.c|                                 |Interrupt Service Routines.|
|ST67W6X_CLI_LWIP/Boot/Core/Src/system_stm32h7rsxx.c|                             |CMSIS Cortex-M7 Device Peripheral Access Layer System Source File|
|ST67W6X_CLI_LWIP/STM32CubeIDE/Appli/Application/User/Core/syscalls.c|            |STM32CubeIDE Minimal System calls file|
|ST67W6X_CLI_LWIP/STM32CubeIDE/Appli/Application/User/Core/sysmem.c|              |STM32CubeIDE System Memory calls file|
|ST67W6X_CLI_LWIP/STM32CubeIDE/Boot/Application/User/Core/syscalls.c|             |STM32CubeIDE Minimal System calls file|
|ST67W6X_CLI_LWIP/STM32CubeIDE/Boot/Application/User/Core/sysmem.c|               |STM32CubeIDE System Memory calls file|

### __Hardware and Software environment__

  - This example runs on the NUCLEO-H7S3L8 board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-H7S3L8 board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9

  - User Option Bytes requirement (with STM32CubeProgrammer tool)
    XSPI2_HSLV=1 I/O XSPIM_P2 High speed option enabled

For further information, please visit the dedicated Wiki page [ST67W611M Hardware setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup).

### __How to use it?__

In order to make the program work, you must do the following :

  - Build the chosen Host project
    - Open your preferred toolchain
    - Rebuild all files of Boot project
    - Load your image into Host target memory
    - Rebuild all files of Appli project
    - For EWARM of MDK-ARM
      - Load Appli part in External memory available on NUCLEO-H7S3L8 board
    - For STM32CubeIDE
      - Open the menu [Run]->[Debug configuration] and double click on [STM32 C/C++ Application] (it creates a default debug configuration for the current project selected)
      - In [Debugger] tab, section "External loaders" add the external loader corresponding to your Board/Memory as described below:
        - In "External loaders" section, click on [Add]
          - Select the loader MX25UW25645G_NUCLEO-H7S3L8.stldr
          - Option "Enabled" checked and Option "Initialize" unchecked
        - In "Misc" section, uncheck the option "Verify flash download"
        - In [Startup] tab, section "Load Image and Symbols":
          - Click on [Add]
          - click on "Project" and then select the Boot project
          - click on Build configuration and select "Use active"
          - then select the following options:
            - "Perform build" checked
            - "Download" checked
            - "Load symbols" unchecked
  - Use the application through the serial link
    - Open a Terminal client connected to the Host ST-LINK COM port
    - UART Config
      - Baudrate: 921600
      - Data: 8b
      - Stopbit: 1b
      - Parity: none
      - Flow control: none
      - Rx: LF
      - Tx: CR+LF
      - Local Echo: Off
  - Press Reset button of the Host board

### __User setup__

Type "help" to list all the available commands of the CLI.

To add optional shell commands, the default shell commands list level can be modified in the _ST67W6X/Target/shell_config.h_ file:
```
/** Default shell commands list level (0: Minimal, 1: Full) */
#define SHELL_CMD_LEVEL                         1
```
By default the full shell commands list is disabled to reduce the application size in memory.

Some commands need to be connected before being executed, for example the `ping` command.

```
wifi_sta_connect AP_SSID Password

    NCP is treating the connection request
    Netif : Link is up
    Connection success
    Connected to following Access Point :
    [<BSSID>] Channel: 1 | RSSI: -22 | SSID: AP_SSID
    STA IP :
    IP :              192.168.8.242
    Gateway :         192.168.8.1
    Netmask :         255.255.255.0
    IPv6 address 0 : FE80::4082:7B00:11B4

ping 8.8.8.8 -s 64 -c 4 -i 1000

    Ping: 193 ms
    Ping: 225 ms
    Ping: 258 ms
    Ping: 84 ms
    4 packets transmitted, 4 received, 0% packet loss, time 190ms
```

Otherwise a message on the console will indicate that application is not in the correct state for executing that command.

#### __ST67W6X configuration__

The default System configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** NCP power save mode : 0: NCP stays always active / 1: NCP goes in low power mode when idle */
#define W6X_POWER_SAVE_AUTO                     1

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1U
```

> [!IMPORTANT]
> In order to setup and use external clock, SW and HW settings have to be modified.<br/>
> Refer to [Wiki ST67W611M1 32.768 kHz and low power operation](https://wiki.st.com/stm32mcu/wiki/Connectivity:ST67W611M1_32KHz_management) page in order to be informed about required changes.

The default Wi-Fi configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** Boolean to enable/disable autoconnect functionality */
#define W6X_WIFI_AUTOCONNECT                    1

/** Define the max number of stations that can connect to the Soft-AP */
#define W6X_WIFI_SAP_MAX_CONNECTED_STATIONS     4

/** Define the region code, supported values : [CN, JP, US, EU, 00] */
#define W6X_WIFI_COUNTRY_CODE                   "00"

/** Define if the country code will match AP's one.
  * 0: match AP's country code,
  * 1: static country code */
#define W6X_WIFI_ADAPTIVE_COUNTRY_CODE          0
```

Additionally, some others options can be modified in the _ST67W6X/Target_ directory with different configuration files as below:

- _logging_config.h_ : This file provides configuration for the logging component to set the log level.
- _shell_config.h_ : This file provides configuration for Shell component.
- _w61_driver_config.h_ : This file provides configuration for the W61 configuration module.

#### __Application configuration__

The logging output mode can be modified in the _Appli/App/app_config.h_ file:
```
/** Select output log mode [0: printf / 1: UART / 2: ITM] */
#define LOG_OUTPUT_MODE             LOG_OUTPUT_UART
```

The default DTIM Wi-Fi power mode can be modified in the _Appli/App/app_config.h_ file:
```
/** Define the default factor to apply to AP DTIM interval when connected and power save mode is enabled */
#define WIFI_DTIM                   1
```

The host low power mode can be modified in the _Appli/App/app_config.h_ file:
```
/** Low power configuration [0: disable / 1: sleep / 2: stop / 3: standby] */
#define LOW_POWER_MODE              LOW_POWER_DISABLE
```

### __Known limitations__

  - The Host STOP Power mode is not supported in this application
  - The TCP echo server (tcpbin.com) used in example has two limitations: Messages shall end by \n (0x0A) and not contain \r (0x0D)
