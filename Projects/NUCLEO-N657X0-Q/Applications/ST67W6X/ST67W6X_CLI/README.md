## __ST67W6X_CLI Application Description__

This application aims to evaluate and to test the X-NUCLEO-67W61M1 Wi-Fi and Bluetooth LE solutions via command line interface (CLI).

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

The application allows to perform some basic Wi-Fi operations like scanning available local access points (AP), connecting to an AP, but also to test network functionalities like Ping, DHCP, Socket, MQTT.

> [!IMPORTANT]
> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t01_v2.0.106.bin.<br/>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Scripts/Binaries/README.md) instructions using the __NCP_update_mission_profile_t01.bat__ script.

### __Keywords__

Connectivity, WiFi, BLE, ST67W6X_Network_Driver, FreeRTOS, CLI, Station mode, Soft Access Point mode, DHCP, Ping, Echo, FOTA, MQTT, Scan, TCP, UDP, WPA2, WPA3

### __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_CLI](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_ST67W6X_CLI_Application).

### __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI/FSBL/Appli/App/|                                                     |Main application code directory|
|ST67W6X_CLI/FSBL/Appli/Target/|                                                  |Logging, Shell, Low-Power and BSP interfaces|
|ST67W6X_CLI/FSBL/Core/Src|                                                       |STM32CubeMX generated sources code|
|ST67W6X_CLI/FSBL/Core/Inc|                                                       |STM32CubeMX generated header files|
|ST67W6X_CLI/FSBL/ST67W6X/App|                                                    |Entry point to start the application associated to the ST67W6X_Network_Driver Middleware|
|ST67W6X_CLI/FSBL/ST67W6X/Target|                                                 |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_CLI/FSBL/littlefs/lfs|                                                   |Certificates used to execute secure operations|
|ST67W6X_CLI/FSBL/littlefs/Target|                                                |Configuration and port files to manage the littlefs in flash|
|ST67W6X_CLI/EWARM|                                                               |Project for the IAR Embedded workbench for Arm|
|ST67W6X_CLI/MDK-ARM|                                                             |Project for the RealView Microcontroller Development Kit|
|ST67W6X_CLI/STM32CubeIDE|                                                        |Project for the STM32CubeIDE toolchain|

### __Directory contents__

|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI/FSBL/Appli/App/app_config.h|                                         |Configuration for main application|
|ST67W6X_CLI/FSBL/Appli/App/echo.h|                                               |Echo test definition|
|ST67W6X_CLI/FSBL/Appli/App/fota.h|                                               |FOTA test definition|
|ST67W6X_CLI/FSBL/Appli/App/main_app.h|                                           |Header for main_app.c|
|ST67W6X_CLI/FSBL/Appli/Target/logshell_ctrl.h|                                   |Header for logshell_ctrl module|
|ST67W6X_CLI/FSBL/Core/Inc/app_freertos.h|                                        |FreeRTOS applicative header file|
|ST67W6X_CLI/FSBL/Core/Inc/FreeRTOSConfig.h|                                      |Header for FreeRTOS application specific definitions|
|ST67W6X_CLI/FSBL/Core/Inc/main.h|                                                |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_CLI/FSBL/Core/Inc/stm32n6xx_hal_conf.h|                                  |HAL configuration file.|
|ST67W6X_CLI/FSBL/Core/Inc/stm32n6xx_it.h|                                        |This file contains the headers of the interrupt handlers.|
|ST67W6X_CLI/FSBL/littlefs/Target/easyflash.h|                                    |Header file that adapts LittleFS to EasyFlash4|
|ST67W6X_CLI/FSBL/littlefs/Target/lfs_port.h|                                     |lfs flash port definition|
|ST67W6X_CLI/FSBL/littlefs/Target/lfs_util_config.h|                              |lfs utility user configuration|
|ST67W6X_CLI/FSBL/ST67W6X/App/app_st67w6x.h|                                      |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_CLI/FSBL/ST67W6X/Target/bsp_conf.h|                                      |This file contains definitions for the BSP interface|
|ST67W6X_CLI/FSBL/ST67W6X/Target/logging_config.h|                                |Header file for the W6X Logging configuration module|
|ST67W6X_CLI/FSBL/ST67W6X/Target/shell_config.h|                                  |Header file for the W6X Shell configuration module|
|ST67W6X_CLI/FSBL/ST67W6X/Target/w61_driver_config.h|                             |Header file for the W61 configuration module|
|ST67W6X_CLI/FSBL/ST67W6X/Target/w6x_config.h|                                    |Header file for the W6X configuration module|
|      |                                                                          |           |
|ST67W6X_CLI/FSBL/Appli/App/echo.c|                                               |Test an echo with a server|
|ST67W6X_CLI/FSBL/Appli/App/fota.c|                                               |Test a FOTA with a server|
|ST67W6X_CLI/FSBL/Appli/App/main_app.c|                                           |main_app program body|
|ST67W6X_CLI/FSBL/Appli/Target/logshell_ctrl.c|                                   |logshell_ctrl (uart interface)|
|ST67W6X_CLI/FSBL/Core/Src/app_freertos.c|                                        |Code for freertos applications|
|ST67W6X_CLI/FSBL/Core/Src/main.c|                                                |Main program body|
|ST67W6X_CLI/FSBL/Core/Src/stm32n6xx_hal_msp.c|                                   |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_CLI/FSBL/Core/Src/stm32n6xx_hal_timebase_tim.c|                          |HAL time base based on the hardware TIM.|
|ST67W6X_CLI/FSBL/Core/Src/stm32n6xx_it.c|                                        |Interrupt Service Routines.|
|ST67W6X_CLI/FSBL/Core/Src/system_stm32n6xx_fsbl.c|                               |CMSIS Cortex-M55 Device Peripheral Access Layer System Source File<br>to be used after the boot ROM execution in an applicative code called<br>"FSBL" for First Stage Boot Loader.|
|ST67W6X_CLI/FSBL/littlefs/Target/lfs_easyflash.c|                                |Adapts LittleFS to EasyFlash4|
|ST67W6X_CLI/FSBL/littlefs/Target/lfs_flash.c|                                    |Host flash interface|
|ST67W6X_CLI/FSBL/ST67W6X/App/app_st67w6x.c|                                      |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_CLI/FSBL/ST67W6X/Target/spi_port.c|                                      |SPI bus interface porting layer implementation|
|ST67W6X_CLI/FSBL/ST67W6X/Target/util_task_port.c|                                |Task Performance porting layer implementation|
|ST67W6X_CLI/MDK-ARM/startup_stm32n657xx_fsbl.c|                                  |CMSIS-Core Device Startup File for STM32N6xx Device <br>after bootROM execution|
|ST67W6X_CLI/STM32CubeIDE/FSBL/Application/User/Core/syscalls.c|                  |STM32CubeIDE Minimal System calls file|
|ST67W6X_CLI/STM32CubeIDE/FSBL/Application/User/Core/sysmem.c|                    |STM32CubeIDE System Memory calls file|

### __Hardware and Software environment__

  - This example runs on the NUCLEO-N657X0-Q board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-N657X0-Q board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9

For further information, please visit the dedicated Wiki page [ST67W611M Hardware setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup).

### __How to use it?__

In order to make the program work, you must do the following :

  - Build the chosen Host project
    - Open your preferred toolchain
    - Set the boot mode in development mode (BOOT1 switch position is 2-3, BOOT0 switch position doesn't matter)
    - Rebuild all files
    - Resort to STM32CubeProgrammer to add a header to the generated binary "ST67W6X_CLI_FSBL.bin" with the following command
      ```
      STM32_SigningTool_CLI.exe -bin ST67W6X_CLI_FSBL.bin -nk -of 0x80000000 -t fsbl -o ST67W6X_CLI_FSBL_trusted.bin -hv 2.3 -dump ST67W6X_CLI_FSBL_trusted.bin
      ```
      __Warning__ If using STM32CubeProgrammer __v2.21__ version or more recent, add ___--align___ option in the command line.

      The resulting binary is ST67W6X_CLI_FSBL_trusted.bin
    - Open STM32CubeProgrammer
      - Select the External loader MX25UM51245G_STM32N6570-NUCLEO
      - Load the binary and its header (ST67W6X_CLI_FSBL_trusted.bin) in the board external Flash at address 0x7000'0000
    - Set the boot mode in boot from external Flash (BOOT0 switch position is 1-2 and BOOT1 switch position is 1-2)
    - Press the reset button. The code then executes in boot from external Flash mode

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
    DHCP client start, this may take few seconds
    Connected to following Access Point :
    [<BSSID>] Channel: 1 | RSSI: -22 | SSID: AP_SSID
    Connection success
    Station got a new IP address : 192.168.1.24

ping "google.com" -s 64 -c 4 -i 1000

    Ping: 9ms
    Ping: 10ms
    Ping: 7ms
    Ping: 9ms
    4 packets transmitted, 4 received, 0% packet loss, time 8ms
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

The default BLE configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** String defining BLE hostname */
#define W6X_BLE_HOSTNAME                        "ST67W61_BLE"
```

The default Net configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_NET_DHCP                            3U

/** String defining Soft-AP subnet to use.
  *  Last digit of IP address automatically set to 1 */
#define W6X_NET_SAP_IP_SUBNET                   {10, 19, 96}

/** String defining Wi-Fi hostname */
#define W6X_NET_HOSTNAME                        "ST67W61_WiFi"

/** Timeout in ticks when calling W6X_Net_Recv() */
#define W6X_NET_RECV_TIMEOUT                    10000U

/** Timeout in ticks when calling W6X_Net_Send() */
#define W6X_NET_SEND_TIMEOUT                    10000U

/** Default Net socket receive buffer size
  * @note In the NCP, the LWIP recv function is used with a static buffer with
  * a fixed length of 4608 (3 * 1536). The data is read in chunks of 4608 bytes
  * So in order to get optimal performances, the buffer on NCP side should be twice as big */
#define W6X_NET_RECV_BUFFER_SIZE                9216U
```

The default HTTP configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** HTTP Client thread stack size */
#define W6X_HTTP_CLIENT_THREAD_STACK_SIZE       1536U

/** HTTP Client thread priority */
#define W6X_HTTP_CLIENT_THREAD_PRIO             30

/** Timeout value in millisecond for receiving data via TCP socket used by the HTTP client.
  * This value is set to compensate for when the NCP get stuck for a long time (1 second or more)
  * when retrieving data from an HTTP server for example */
#define W6X_HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT   1000

/** Size of the TCP socket used by the HTTP client, recommended to be at least 0x2000 when fetching lots of data.
  * 0x2000 is the value used in the SPI host project for OTA update, which retrieves around 1 mega bytes of data. */
#define W6X_HTTP_CLIENT_TCP_SOCKET_SIZE         12288
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

The echo client configuration can be modified in the _Appli/App/echo.h_ file:
```
/** URL of Echo TCP remote server */
#define ECHO_SERVER_URL             "tcpbin.com"

/** Port of Echo TCP remote server */
#define ECHO_SERVER_PORT            4242
#endif /* NET_USE_IPV6 */

/** Minimal size of a packet */
#define ECHO_TRANSFER_SIZE_START    1000

/** Maximal size of a packet */
#define ECHO_TRANSFER_SIZE_MAX      2000

/** To increment the size of a group of packets */
#define ECHO_TRANSFER_SIZE_ITER     250

/** Number of packets to be sent */
#define ECHO_ITERATION_COUNT        10
```

The fota configuration can be modified in the _Appli/App/fota.h_ file:
```
/** Timeout value to set the FOTA timer to when the FOTA application encountered an error for the first time.
  * This allows to tune the timeout value before doing a retry attempt. (not applicable if FOTA timer is not used)*/
#define FOTA_TIMEOUT                20000

/** Stack size of the FOTA application, this value needs to take into account the HTTP client
  * and NCP FWU static data allocation */
#define FOTA_TASK_STACK_SIZE        1800

/** The max size of the URI supported, this because the buffer
  * that will receive this info is allocated at compile time (static) */
#define FOTA_URI_MAX_SIZE           256

/** Default URI for the ST67 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI               "/download/st67w611m_mission_t01_v2.0.106.bin.ota"

/** Default HTTP server address */
#define FOTA_HTTP_SERVER_ADDR       "192.168.8.105"

/** Default HTTP port */
#define FOTA_HTTP_SERVER_PORT       8000

/** As specified in RFC 1035 Domain Implementation and Specification
  * from November 1987, domain names are 255 octets or less */
#define FOTA_MAX_DOMAIN_NAME_SIZE   255U

/** FOTA HTTP request timeout value in milliseconds.
  * When timeout is reached the HTTP request will be considered as a failure.
  */
#define FOTA_HTTP_TIMEOUT_IN_MS     60000U

/** Set the priority of the FOTA task using FreeRTOS priority evaluation system */
#define FOTA_TASK_PRIORITY          24

/** Size of the buffer used to transfer the FWU header to the ST67 in one shot (required by ST67) */
#define FWU_HEADER_SIZE             512

/** Multiple of data length that should be written in ST67, recommendation to ensure correct write into ST67 memory */
#define FWU_SECTOR_ALIGNMENT        256
```

### __Known limitations__

  - Wi-Fi station does not answer to ARP requests by Access Point when static IP is used
  - The Host STOP Power mode is not supported in this application
  - The TCP echo server (tcpbin.com) used in example has two limitations: Messages shall end by \n (0x0A) and not contain \r (0x0D)
