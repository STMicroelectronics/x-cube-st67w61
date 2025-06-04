# __ST67W6X_CLI Application Description__

This application aims to evaluate and to test the X-NUCLEO-67W61M1 Wi-Fi and Bluetooth LE solutions via command line interface (CLI).

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

The application allows to perform some basic Wi-Fi operations like scanning available local access points (AP), connecting to an AP, but also to test network functionalities like Ping, DHCP, Socket, MQTT.

> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t01_v2.0.75.bin.
>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Utilities/Binaries/README.md) instructions using the __NCP_update_mission_profile.bat__ script.

## __Keywords__

Connectivity, WiFi, BLE, ST67W6X_Network_Driver, FreeRTOS, CLI, Station mode, Soft Access Point mode, DHCP, Ping, Echo, FOTA, MQTT, Scan, TCP, UDP, WPA2, WPA3

## __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_CLI](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wifi_ST67W6X_CLI_Application).

## __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI/Appli/App_CLI/App/|                                                  |Main application code directory|
|ST67W6X_CLI/Appli/App_CLI/Target/|                                               |Logging, Shell, Low-Power and SPI BSP interfaces|
|ST67W6X_CLI/Appli/App_CLI/Core/Src|                                              |STM32CubeMX generated sources code|
|ST67W6X_CLI/Appli/App_CLI/Core/Inc|                                              |STM32CubeMX generated header files|
|ST67W6X_CLI/Appli/App_CLI/ST67W6X/Target|                                        |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_CLI/Appli/App_CLI/littlefs/lfs|                                          |Certificates used to execute secure operations|
|ST67W6X_CLI/Appli/App_CLI/littlefs/Target|                                       |Configuration and port files to manage the littlefs in flash|
|ST67W6X_CLI/EWARM|                                                               |Project for the IAR Embedded workbench for Arm|
|ST67W6X_CLI/MDK-ARM|                                                             |Project for the RealView Microcontroller Development Kit|
|ST67W6X_CLI/STM32CubeIDE|                                                        |Project for the STM32CubeIDE toolchain|

## __Directory contents__


|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI/Appli/App_CLI/App/app_config.h|                                      |Configuration for main application|
|ST67W6X_CLI/Appli/App_CLI/App/echo.h|                                            |Echo test definition|
|ST67W6X_CLI/Appli/App_CLI/App/fota.h|                                            |FOTA test definition|
|ST67W6X_CLI/Appli/App_CLI/App/main_app.h|                                        |Header for main_app.c|
|ST67W6X_CLI/Appli/App_CLI/Target/logshell_ctrl.h|                                |Header for logshell_ctrl.h|
|ST67W6X_CLI/Appli/App_CLI/Target/spi_port_conf.h|                                |Interfaces/maps the SPI instance to be used for NCP communication|
|ST67W6X_CLI/Appli/Core/Inc/FreeRTOSConfig.h|                                     |Header for FreeRTOS application specific definitions|
|ST67W6X_CLI/Appli/Core/Inc/main.h|                                               |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_CLI/Appli/Core/Inc/stm32h7rsxx_hal_conf.h|                               |HAL configuration template file.<br>This file should be copied to the application folder and renamed<br>to stm32h7rsxx_hal_conf.h.|
|ST67W6X_CLI/Appli/Core/Inc/stm32h7rsxx_it.h|                                     |This file contains the headers of the interrupt handlers.|
|ST67W6X_CLI/Appli/littlefs/Target/easyflash.h|                                   |Header file that adapts LittleFS to EasyFlash4|
|ST67W6X_CLI/Appli/littlefs/Target/lfs_port.h|                                    |lfs flash port definition|
|ST67W6X_CLI/Appli/littlefs/Target/lfs_util_config.h|                             |lfs utility user configuration|
|ST67W6X_CLI/Appli/ST67W6X/Target/logging_config.h|                               |Header file for the W6X Logging configuration module|
|ST67W6X_CLI/Appli/ST67W6X/Target/shell_config.h|                                 |Header file for the W6X Shell configuration module|
|ST67W6X_CLI/Appli/ST67W6X/Target/w61_driver_config.h|                            |Header file for the W61 configuration module|
|ST67W6X_CLI/Appli/ST67W6X/Target/w6x_config.h|                                   |Header file for the W6X configuration module|
|ST67W6X_CLI/Boot/Core/Inc/extmem_manager.h|                                      |: Header for secure_manager_api.c file.|
|ST67W6X_CLI/Boot/Core/Inc/main.h|                                                |: Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_CLI/Boot/Core/Inc/stm32h7rsxx_hal_conf.h|                                |HAL configuration template file.<br>This file should be copied to the application folder and renamed<br>to stm32h7rsxx_hal_conf.h.|
|ST67W6X_CLI/Boot/Core/Inc/stm32h7rsxx_it.h|                                      |This file contains the headers of the interrupt handlers.|
|ST67W6X_CLI/Boot/Core/Inc/stm32_extmem_conf.h|                                   |: Header for extmem.c file.|
|      |                                                                          |           |
|ST67W6X_CLI/Appli/App_CLI/App/echo.c|                                            |Test an echo with a server|
|ST67W6X_CLI/Appli/App_CLI/App/fota.c|                                            |Test a FOTA with a server|
|ST67W6X_CLI/Appli/App_CLI/App/main_app.c|                                        |main_app program body|
|ST67W6X_CLI/Appli/App_CLI/Target/logshell_ctrl.c|                                |logshell_ctrl (uart interface)|
|ST67W6X_CLI/Appli/Core/Src/freertos.c|                                           |Code for freertos applications|
|ST67W6X_CLI/Appli/Core/Src/main.c|                                               |Main program body|
|ST67W6X_CLI/Appli/Core/Src/stm32h7rsxx_hal_msp.c|                                |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_CLI/Appli/Core/Src/stm32h7rsxx_hal_timebase_tim.c|                       |Template for HAL time base based on the peripheral hardware TIM1.|
|ST67W6X_CLI/Appli/Core/Src/stm32h7rsxx_it.c|                                     |Interrupt Service Routines.|
|ST67W6X_CLI/Appli/Core/Src/system_stm32h7rsxx.c|                                 |CMSIS Cortex-M7 Device Peripheral Access Layer System Source File|
|ST67W6X_CLI/Appli/littlefs/Target/lfs_easyflash.c|                               |Adapts LittleFS to EasyFlash4|
|ST67W6X_CLI/Appli/littlefs/Target/lfs_flash.c|                                   |Host flash interface|
|ST67W6X_CLI/Appli/ST67W6X/Target/spi_port.c|                                     |SPI bus interface porting layer implementation|
|ST67W6X_CLI/Boot/Core/Src/extmem_manager.c|                                      |: This file implements the extmem configuration|
|ST67W6X_CLI/Boot/Core/Src/main.c|                                                |: Main program body|
|ST67W6X_CLI/Boot/Core/Src/stm32h7rsxx_hal_msp.c|                                 |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_CLI/Boot/Core/Src/stm32h7rsxx_it.c|                                      |Interrupt Service Routines.|
|ST67W6X_CLI/Boot/Core/Src/system_stm32h7rsxx.c|                                  |CMSIS Cortex-M7 Device Peripheral Access Layer System Source File|
|ST67W6X_CLI/STM32CubeIDE/Appli/Application/User/Core/syscalls.c|                 |STM32CubeIDE Minimal System calls file|
|ST67W6X_CLI/STM32CubeIDE/Appli/Application/User/Core/sysmem.c|                   |STM32CubeIDE System Memory calls file|
|ST67W6X_CLI/STM32CubeIDE/Boot/Application/User/Core/syscalls.c|                  |STM32CubeIDE Minimal System calls file|
|ST67W6X_CLI/STM32CubeIDE/Boot/Application/User/Core/sysmem.c|                    |STM32CubeIDE System Memory calls file|


## __Hardware and Software environment__

  - This example runs on the NUCLEO-H7S3L8 board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-H7S3L8 board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9

  - User Option Bytes requirement (with STM32CubeProgrammer tool)
    XSPI2_HSLV=1 I/O XSPIM_P2 High speed option enabled

For further information, please visit the dedicated Wiki page [ST67W611M Hardware set-up](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wifi_MCU_Hardware_Setup).

## __How to use it?__

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

##  __User setup__

Type "help" to list all the available commands of the CLI:
```
help

shell commands list:
echo                 - echo [ iteration ]
fota_http            - fota_http < server IP > < server port > < ST67 resource URI > [ STM32 resource URI ] [ FOTA header resource URI ]. Run firmware update over HTTP
quit                 - quit. Stop application execution
info_app             - info_app. Display application info
ble_disconnect       - ble_disconnect [ conn handle: 0 or 1 ]
ble_connect          - ble_connect [ conn handle: 0 or 1 ] [ BD addr ]
ble_stop_scan        - ble_stop_scan
ble_start_scan       - ble_start_scan
ble_adv_stop         - ble_adv_stop
ble_adv_start        - ble_adv_start
ble_init             - ble_init [ 1: client mode; 2:server mode ]
mqtt_publish         - mqtt_publish < Topic > < Message >
mqtt_unsubscribe     - mqtt_unsubscribe < Topic >
mqtt_subscribe       - mqtt_subscribe < Topic >
mqtt_disconnect      - mqtt_disconnect
mqtt_connect         - mqtt_connect < -h Host > < -p Port >
mqtt_configure       - mqtt_configure < -s Scheme > < -i ClientId > [ -u Username ] [ -pw Password ] [ -c Certificate ] [ -k PrivateKey ] [ -ca CACertificate ] [ -sni ]
dnslookup            - dnslookup <hostname>
time                 - time < timezone : UTC format : range [-12; 14] or HHmm format : with HH in range [-12; +14] and mm in range [00; 59] >
ping                 - ping <hostname> [ -c count [1; max(uint16_t) - 1] ] [ -s size [1; 10000] ] [ -i interval [100; 3500] ]
atcmd                - atcmd < "AT+CMD?" >. Execute AT command
powersave            - powersave [ 0: disable; 1: enable ]
fs_list              - fs_list. List all files in the file system
fs_delete            - fs_delete < filename >. Delete file from the NCP file system
fs_read              - fs_read < filename >. Read file content
fs_write             - fs_write < filename >. Write file content from the Host to the NCP
reset                - reset < 0: HAL_Reset; 1: NCP_Restore >
info                 - info. Display ST67W6X module info
wifi_twt_teardown    - wifi_twt_teardown < 0: announced; 1: unannounced >; 2: all >
wifi_twt_set         - wifi_twt_set
wifi_twt_setup       - wifi_twt_setup < setup_type(0: request; 1: suggest; 2: demand) > < flow_type(0: announced; 1: unannounced) > < wake_int_exp > < min_wake_duration > < wake_int_mantissa >
dtim                 - dtim < value [0; 10] >
wifi_ap_mac          - wifi_ap_mac
wifi_dhcp            - wifi_dhcp [ 0:DHCP disabled; 1:DHCP enabled ] [ 1:STA only; 2:AP only; 3:STA + AP ] [ lease_time [1; 2880] ]
wifi_ap_ip           - wifi_ap_ip
wifi_ap_disconnect_sta - wifi_ap_disconnect_sta < MAC >
wifi_ap_list_sta     - wifi_ap_list_sta
wifi_ap_mode         - wifi_ap_mode [ mode ]
wifi_ap_stop         - wifi_ap_stop
wifi_ap_start        - wifi_ap_start [ -s SSID ] [ -p Password ] [ -c channel [1; 13] ] [ -e security [0:Open; 2:WPA; 3:WPA2; 4:WPA3] ] [ -h hidden [0; 1] ]
wifi_country_code    - wifi_country_code [ 0:AP aligned country code; 1:User country code ] [ Country code [CN; JP; US; EU; 00] ]
wifi_sta_state       - wifi_sta_state
wifi_sta_mac         - wifi_sta_mac
wifi_sta_dns         - wifi_sta_dns [ 0:default IPs; 1: manual IPs ] [ DNS1 addr ] [ DNS2 addr ] [ DNS3 addr ]
wifi_sta_ip          - wifi_sta_ip [ IP addr ] [ Gateway addr ] [ Netmask addr ]
wifi_hostname        - wifi_hostname [ hostname ]
wifi_auto_connect    - wifi_auto_connect
wifi_sta_mode        - wifi_sta_mode [ mode ]
wifi_sta_disconnect  - wifi_sta_disconnect [ -r ]
wifi_sta_connect     - wifi_sta_connect < SSID > [ Password ] [ -b BSSID ] [ -i interval [0; 7200] ] [ -n nb_attempts [0; 1000] ] [ -wps ] [ -wep ]
wifi_scan            - wifi_scan [ -p ] [ -s SSID ] [ -b BSSID ] [ -c channel [1; 13] ] [ -n max_count [1; 50] ]
help                 - help [ command ]. Display all available commands and the relative help message
iperf                - iperf [ options ]. Iperf command line tool for network performance measurement. [ -h ] for help
task_report          - task_report. Display task performance report
task_perf            - task_perf [ -s ]. Start or stop [ -s ] task performance measurement
echostop             - echostop. WFA - Stops the UDP echo server.
echostart            - echostart < port >. WFA - Starts the UDP echo server on the specified port.
```

Some commands need to be connected before being executed, for example the `ping` command.

```
wifi_sta_connect AP_SSID Password

    NCP is treating the connection request
    DHCP client start, this may take few seconds
    Connected to following Access Point :
    [<BSSID>] Channel: 1 | RSSI: -22 | SSID: AP_SSID
    Station got an IP from Access Point : 192.168.1.24
    Connection success

ping "google.com" -s 64 -c 4 -i 1000

    Ping: 9ms
    Ping: 10ms
    Ping: 7ms
    Ping: 9ms
    4 packets transmitted, 4 received, 0% packet loss, time 8ms
```

Otherwise a message on the console will indicate that application is not in the correct state for executing that command.

###  __ST67W6X configuration__

The default System configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file.
```
/** NCP will go by default in low power mode when NCP is in idle mode */
#define W6X_POWER_SAVE_AUTO                     1

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1
```

The default Wi-Fi configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file.
```
/** Boolean to enable/disable autoconnect functionality */
#define W6X_WIFI_AUTOCONNECT                    1

/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_WIFI_DHCP                           3

/** Define the max number of stations that can connect to the Soft-AP */
#define W6X_WIFI_SAP_MAX_CONNECTED_STATIONS     4

/** String defining Soft-AP subnet to use.
  *  Last digit of IP address automatically set to 1 */
#define W6X_WIFI_SAP_IP_SUBNET                  {192, 168, 8}

/** String defining Soft-AP subnet to use in case of conflict with the AP the STA is connected to.
  *  Last digit of IP address automatically set to 1 */
#define W6X_WIFI_SAP_IP_SUBNET_BACKUP           {192, 168, 9}

/** Define if the DNS addresses are set manually or automatically */
#define W6X_WIFI_DNS_MANUAL                     0

/** String defining DNS IP 1 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_1                       {208, 67, 222, 222}

/** String defining DNS IP 2 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_2                       {8, 8, 8, 8}

/** String defining DNS IP 3 address to use
  * @note: This address will be used only if W6X_WIFI_DNS_MANUAL equals 1 */
#define W6X_WIFI_DNS_IP_3                       {0, 0, 0, 0}

/** Define the region code, supported values : [CN, JP, US, EU, 00] */
#define W6X_WIFI_COUNTRY_CODE                   "00"

/** Define if the country code will match AP's one.
  * 0: match AP's country code,
  * 1: static country code */
#define W6X_WIFI_ADAPTIVE_COUNTRY_CODE          0

/** String defining Wi-Fi hostname */
#define W6X_WIFI_HOSTNAME                       "ST67W61_WiFi"
```

Additionally, some others options can be modified in the _ST67W6X/Target_ directory with different configuration files as below:

- _logging_config.h_ : This files provides configuration for the logging component to set the log level.
- _shell_config.h_ : This file provides configuration for Shell component.
- _w61_driver_config.h_ : This file provides configuration for the W61 configuration module.

###  __Application configuration__

The logging output mode can be modified in the _App_CLI/App/app_config.h_ file :
```
/** Select output log mode [0: printf / 1: UART / 2: ITM] */
#define LOG_OUTPUT_MODE             LOG_OUTPUT_UART
```

The host low power mode can be modified in the _App_CLI/App/app_config.h_ file :
```
/** Low power configuration [0: disable / 1: sleep / 2: stop / 3: standby] */
#define LOW_POWER_MODE              LOW_POWER_DISABLE
```

The echo client configuration can be modified in the _App_CLI/App/app_config.h_ file :
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

The fota configuration can be modified in the _App_CLI/App/app_config.h_ file :
```
/** Timeout value to set the FOTA timer to when the FOTA application encountered an error for the first time.
  * This allows to tune the timeout value before doing a retry attempt. (not applicable if FOTA timer is not used)*/
#define FOTA_TIMEOUT                20000

/** Delay to wait before rebooting the host device, waiting for NCP device to finish update */
#define FOTA_DELAY_BEFORE_REBOOT    16000

/** Stack size of the FOTA application, this value needs to take into account the HTTP client
  * and NCP OTA static data allocation */
#define FOTA_TASK_STACK_SIZE        2048

/** The max size of the URI supported, this because the buffer
  * that will receive this info is allocated at compile time (static) */
#define FOTA_URI_MAX_SIZE           128

/** Default URI for the ST67 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI               "/download/st67w611m_mission_t01_v2.0.75.bin.ota"

/** Default HTTP server address */
#define FOTA_HTTP_SERVER_ADDR       "192.168.8.105"

/** Default HTTP port */
#define FOTA_HTTP_SERVER_PORT       8000

/** As specified in RFC 1035 Domain Implementation and Specification
  * from November 1987, domain names are 255 octets or less */
#define FOTA_MAX_DOMAIN_NAME_SIZE   255U
```

## __Known limitations__

  - By default the country code / region configured in the device is World with 1 to 13 active channels.
  - Enabling Wi-Fi DTIM can generates some failure during Network transaction.
  - Static IP addressing is not compatible with DTIM lowpower mode.
  - The Host STOP Power mode is not supported in this application.
  - The TCP echo server (tcpbin.com) used in example has two limitations:
    - Messages shall end by \n (0x0A)
    - messages shall not contain \r (0x0D).
