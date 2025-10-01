# __ST67W6X_CLI Application Description__

This application aims to evaluate and to test the X-NUCLEO-67W61M1 Wi-Fi and Bluetooth LE solutions via command line interface (CLI).

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

The application allows to perform some basic Wi-Fi operations like scanning available local access points (AP), connecting to an AP, but also to test network functionalities like Ping, DHCP, Socket, MQTT.

> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t01_v2.0.89.bin.
>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Utilities/Binaries/README.md) instructions using the __NCP_update_mission_profile.bat__ script.

## __Keywords__

Connectivity, WiFi, BLE, ST67W6X_Network_Driver, FreeRTOS, CLI, Station mode, Soft Access Point mode, DHCP, Ping, Echo, FOTA, MQTT, Scan, TCP, UDP, WPA2, WPA3

## __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_CLI](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_ST67W6X_CLI_Application).

## __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI/Appli/App/|                                                          |Main application code directory|
|ST67W6X_CLI/Appli/Target/|                                                       |Logging, Shell, Low-Power and BSP interfaces|
|ST67W6X_CLI/Core/Src|                                                            |STM32CubeMX generated sources code|
|ST67W6X_CLI/Core/Inc|                                                            |STM32CubeMX generated header files|
|ST67W6X_CLI/ST67W6X/App|                                                         |Entry point to start the application associated to the ST67W6X_Network_Driver Middleware|
|ST67W6X_CLI/ST67W6X/Target|                                                      |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_CLI/littlefs/lfs|                                                        |Certificates used to execute secure operations|
|ST67W6X_CLI/littlefs/Target|                                                     |Configuration and port files to manage the littlefs in flash|
|ST67W6X_CLI/EWARM|                                                               |Project for the IAR Embedded workbench for Arm|
|ST67W6X_CLI/MDK-ARM|                                                             |Project for the RealView Microcontroller Development Kit|
|ST67W6X_CLI/STM32CubeIDE|                                                        |Project for the STM32CubeIDE toolchain|

## __Directory contents__


|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_CLI/Appli/App/app_config.h|                                              |Configuration for main application|
|ST67W6X_CLI/Appli/App/echo.h|                                                    |Echo test definition|
|ST67W6X_CLI/Appli/App/fota.h|                                                    |FOTA test definition|
|ST67W6X_CLI/Appli/App/main_app.h|                                                |Header for main_app.c|
|ST67W6X_CLI/Appli/Target/logshell_ctrl.h|                                        |Header for logshell_ctrl.h|
|ST67W6X_CLI/Core/Inc/app_freertos.h|                                             |FreeRTOS applicative header file|
|ST67W6X_CLI/Core/Inc/FreeRTOSConfig.h|                                           |Header for FreeRTOS application specific definitions|
|ST67W6X_CLI/Core/Inc/main.h|                                                     |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_CLI/Core/Inc/stm32h5xx_hal_conf.h|                                       |HAL configuration file.|
|ST67W6X_CLI/Core/Inc/stm32h5xx_it.h|                                             |This file contains the headers of the interrupt handlers.|
|ST67W6X_CLI/littlefs/Target/easyflash.h|                                         |Header file that adapts LittleFS to EasyFlash4|
|ST67W6X_CLI/littlefs/Target/lfs_port.h|                                          |lfs flash port definition|
|ST67W6X_CLI/littlefs/Target/lfs_util_config.h|                                   |lfs utility user configuration|
|ST67W6X_CLI/ST67W6X/App/app_st67w6x.h|                                           |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.1.0 instances.|
|ST67W6X_CLI/ST67W6X/Target/bsp_conf.h|                                           |This file contains definitions for the BSP interface|
|ST67W6X_CLI/ST67W6X/Target/logging_config.h|                                     |Header file for the W6X Logging configuration module|
|ST67W6X_CLI/ST67W6X/Target/shell_config.h|                                       |Header file for the W6X Shell configuration module|
|ST67W6X_CLI/ST67W6X/Target/w61_driver_config.h|                                  |Header file for the W61 configuration module|
|ST67W6X_CLI/ST67W6X/Target/w6x_config.h|                                         |Header file for the W6X configuration module|
|      |                                                                          |           |
|ST67W6X_CLI/Appli/App/echo.c|                                                    |Test an echo with a server|
|ST67W6X_CLI/Appli/App/fota.c|                                                    |Test a FOTA with a server|
|ST67W6X_CLI/Appli/App/main_app.c|                                                |main_app program body|
|ST67W6X_CLI/Appli/Target/logshell_ctrl.c|                                        |logshell_ctrl (uart interface)|
|ST67W6X_CLI/Core/Src/app_freertos.c|                                             |Code for freertos applications|
|ST67W6X_CLI/Core/Src/main.c|                                                     |Main program body|
|ST67W6X_CLI/Core/Src/stm32h5xx_hal_msp.c|                                        |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_CLI/Core/Src/stm32h5xx_hal_timebase_tim.c|                               |HAL time base based on the hardware TIM.|
|ST67W6X_CLI/Core/Src/stm32h5xx_it.c|                                             |Interrupt Service Routines.|
|ST67W6X_CLI/Core/Src/system_stm32h5xx.c|                                         |CMSIS Cortex-M33 Device Peripheral Access Layer System Source File|
|ST67W6X_CLI/littlefs/Target/lfs_easyflash.c|                                     |Adapts LittleFS to EasyFlash4|
|ST67W6X_CLI/littlefs/Target/lfs_flash.c|                                         |Host flash interface|
|ST67W6X_CLI/ST67W6X/App/app_st67w6x.c|                                           |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.1.0 instances.|
|ST67W6X_CLI/ST67W6X/Target/spi_port.c|                                           |SPI bus interface porting layer implementation|
|ST67W6X_CLI/ST67W6X/Target/util_task_port.c|                                     |Task Performance porting layer implementation|
|ST67W6X_CLI/STM32CubeIDE/Application/User/Core/syscalls.c|                       |STM32CubeIDE Minimal System calls file|
|ST67W6X_CLI/STM32CubeIDE/Application/User/Core/sysmem.c|                         |STM32CubeIDE System Memory calls file|


## __Hardware and Software environment__

  - This example runs on the NUCLEO-H563ZI board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-H563ZI board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9

For further information, please visit the dedicated Wiki page [ST67W611M Hardware setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup).

## __How to use it?__

In order to make the program work, you must do the following :
  - Build the chosen Host project
    - Open your preferred toolchain
    - Rebuild all files and load your image into Host target memory
  - (Optional) Attach to the running target if you want to debug
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
echo                           - echo [ iteration ]
fota_http                      - fota_http < server IP > < server port > < ST67 resource URI > [ STM32 resource URI ] [ FOTA header resource URI ]. Run firmware update over HTTP
iperf                          - iperf [ options ]. Iperf command line tool for network performance measurement. [ -h ] for help
info_app                       - info_app. Display application info
quit                           - quit. Stop application execution
help                           - help [ command ]. Display all available commands and the relative help message
task_perf                      - task_perf [ -s ]. Start or stop [ -s ] task performance measurement
task_report                    - task_report. Display task performance report
ble_init                       - ble_init [ 1: client mode; 2: server mode ]
ble_deinit                     - ble_deinit
ble_adv                        - ble_adv [ -a abort adv ]
ble_scan                       - ble_scan [ -a abort scan ]
ble_connect                    - ble_connect < Conn Handle [0; 1] > < BD Addr >
ble_disconnect                 - ble_disconnect < Conn Handle [0; 1] >
ble_tx_power                   - ble_tx_power [ Tx Power [0; 20] ]
ble_bd_addr                    - ble_bd_addr [ BD Addr ]
ble_device_name                - ble_device_name [ Device Name ]
ble_adv_data                   - ble_adv_data < Advertising Data >
ble_adv_param                  - ble_adv_param [ AdvIntMin [32; 16384] ] [ AdvIntMax [32; 16384] ] [ Adv Type [0; 2] ] [Adv Channel [1: chan 37; 2: chan 38; 3: chan 39; 7: all] ]
ble_scan_param                 - ble_scan_param [ Scan Type [0; 1] ] [ OwnAddr Type [0; 3] ] [ Filter Policy [0; 3] ] [ Scan Interval [4; 16384] ] [ Scan Window [4; 16384] ]
ble_scanrespdata               - ble_scanrespdata < Scan Response Data >
ble_conn_param                 - ble_conn_param [ Conn Handle [0; 1] ] [ ConnIntMin [6; 3200] ] [ConnIntMax [6; 3200] ] [ Latency [0; 499] ] [ Timeout [10; 3200] ]
ble_get_conn                   - ble_get_conn
ble_exchange_mtu               - ble_exchange_mtu < Conn Handle [0; 1] >
ble_data_length                - ble_data_length < Conn Handle [0; 1] > < TxBytes [27; 251] > < TxTransTime >
ble_srv_create                 - ble_srv_create < Service Index [0; 2] > < UUID > < UUID type >
ble_srv_delete                 - ble_srv_delete < Service Index [0; 2] >
ble_char_create                - ble_char_create < Service Index [0; 2] > < Charac Index : [0; 4] > < UUID > < UUID type > < Charac Property > < Charac Permission: read 1; write 2 >
ble_srv_list                   - ble_srv_list
ble_srv_reg                    - ble_srv_reg
ble_rem_srv_list               - ble_rem_srv_list < Conn Handle [0; 1] >
ble_rem_char_list              - ble_rem_char_list < Conn Handle [0; 1] > < Service Index [1; 4] >
ble_send_notif                 - ble_send_notif < Service Index [0; 1] > < Char Index [0; 4] > < Timeout > < Data >
ble_send_indication            - ble_send_indication < Service Index [0; 1] > < Char Index [0; 4] > < Timeout > < Data >
ble_read_data                  - ble_read_data < Service Index [0; 1] > < Char Index [0; 4] > < Timeout > < Data >
ble_client_write_data          - ble_client_write_data < Conn Handle [0; 1] > < Service Index [1; 5] > < Char Index [1; 5] > < Timeout > < Data >
ble_client_read_data           - ble_client_read_data < Conn Handle [0; 1] > < Service Index [1; 5] > < Char Index [1; 5] >
ble_client_subscribe_char      - ble_client_subscribe_char < Conn Handle [0; 1] > < CharValue Handle > < Char Prop [1; 2] >
ble_client_unsubscribe_char    - ble_client_unsubscribe_char < Conn Handle [0; 1] > < CharValue Handle >
ble_sec_param                  - ble_sec_param [ Security Parameter [0; 4] ]
ble_sec_start                  - ble_sec_start < Conn Handle [0; 1] > < Security Level [1; 4] >
ble_sec_set_passkey            - ble_sec_set_passkey < Conn Handle [0; 1] > < PassKey [0; 999999] >
ble_sec_passkey_confirm        - ble_sec_passkey_confirm < Conn Handle [0; 1] >
ble_sec_pairing_confirm        - ble_sec_pairing_confirm < Conn Handle [0; 1] >
ble_sec_pairing_cancel         - ble_sec_pairing_cancel < Conn Handle [0; 1] >
ble_sec_unpair                 - ble_sec_unpair < Remote BD Addr > < Addr Type [0; 3] >
ble_bonded_device_list         - ble_bonded_device_list
mqtt_configure                 - mqtt_configure < -s Scheme > < -i ClientId > [ -u <Username> ] [ -pw <Password> ] [ -c <Certificate> ] [ -k <PrivateKey> ] [ -ca <CACertificate> ] [ -sni ] [ -ka <KeepAlive> ] [ -q <LWT_QoS> ] [ -cs ] [ -r ]
mqtt_connect                   - mqtt_connect < -h Host > < -p Port >
mqtt_disconnect                - mqtt_disconnect
mqtt_subscribe                 - mqtt_subscribe < Topic >
mqtt_unsubscribe               - mqtt_unsubscribe < Topic >
mqtt_publish                   - mqtt_publish < Topic > < Message > [ -q Qos ] [ -r ]
net_hostname                   - net_hostname [ hostname ]
net_sta_ip                     - net_sta_ip [ IP addr ] [ Gateway addr ] [ Netmask addr ]
net_sta_dns                    - net_sta_dns [ DNS1 addr ] [ DNS2 addr ] [ DNS3 addr ]
net_ap_ip                      - net_ap_ip
net_dhcp                       - net_dhcp [ 0:DHCP disabled; 1:DHCP enabled ] [ 1:STA only; 2:AP only; 3:STA + AP ] [ lease_time [1; 2880] ]
ping                           - ping <hostname> [ -c count [1; max(uint16_t) - 1] ] [ -s size [1; 10000] ] [ -i interval [100; 3500] ]
time                           - time < timezone : UTC format : range [-12; 14] or HHmm format : with HH in range [-12; +14] and mm in range [00; 59] >
dnslookup                      - dnslookup <hostname>
info                           - info. Display ST67W6X module info
reset                          - reset < 0: HAL_Reset; 1: NCP_Restore; 2: NCP_Reset > . Reset the system
fs_write                       - fs_write < filename >. Write file content from the Host to the NCP
fs_read                        - fs_read < filename >. Read file content
fs_delete                      - fs_delete < filename >. Delete file from the NCP file system
fs_list                        - fs_list. List all files in the file system
powersave                      - powersave [ 0: disable; 1: enable ]
atcmd                          - atcmd < "AT+CMD?" >. Execute AT command
spi_dump                       - spi_dump
wifi_scan                      - wifi_scan [ -p ] [ -s SSID ] [ -b BSSID ] [ -c channel [1; 13] ] [ -n max_count [1; 50] ]
wifi_sta_connect               - wifi_sta_connect < SSID > [ Password ] [ -b BSSID ] [ -i interval [0; 7200] ] [ -n nb_attempts [0; 1000] ] [ -wps ] [ -wep ]
wifi_sta_disconnect            - wifi_sta_disconnect [ -r ]
wifi_auto_connect              - wifi_auto_connect
wifi_sta_mac                   - wifi_sta_mac
wifi_sta_state                 - wifi_sta_state
wifi_country_code              - wifi_country_code [ 0:AP aligned country code; 1:User country code ] [ Country code [CN; JP; US; EU; 00] ]
wifi_ap_start                  - wifi_ap_start [ -s SSID ] [ -p Password ] [ -c channel [1; 13] ] [ -e security [0:Open; 2:WPA; 3:WPA2; 4:WPA3] ] [ -h hidden [0; 1] ] [ -m protocol [1: 802 11 b; 2: 802 11 g; 3: 802 11 n; 4: 802 11 ax] ]
wifi_ap_stop                   - wifi_ap_stop
wifi_ap_list_sta               - wifi_ap_list_sta
wifi_ap_disconnect_sta         - wifi_ap_disconnect_sta < MAC >
wifi_ap_mac                    - wifi_ap_mac
wifi_dtim                      - wifi_dtim < value [greater than 0] >
wifi_twt_setup                 - wifi_twt_setup < setup_type(0: request; 1: suggest; 2: demand) > < flow_type(0: announced; 1: unannounced) > < wake_int_exp > < min_wake_duration > < wake_int_mantissa >
wifi_twt_status                - wifi_twt_status
wifi_twt_teardown              - wifi_twt_teardown
wifi_antenna                   - wifi_antenna [ mode [0: disabled; 1: static; 2: dynamic ] ]
echostart                      - echostart < port >. WFA - Starts the UDP echo server on the specified port.
echostop                       - echostop. WFA - Stops the UDP echo server.
```

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

The default System configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** NCP will go by default in low power mode when NCP is in idle mode */
#define W6X_POWER_SAVE_AUTO                     1

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1
```

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

The default Net configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_NET_DHCP                            3

/** String defining Soft-AP subnet to use.
  *  Last digit of IP address automatically set to 1 */
#define W6X_NET_SAP_IP_SUBNET                   {10, 19, 96}

/** String defining Wi-Fi hostname */
#define W6X_NET_HOSTNAME                        "ST67W61_WiFi"
```

The default HTTP configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** HTTP Client thread stack size */
#define W6X_HTTP_CLIENT_THREAD_STACK_SIZE       1536

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

- _logging_config.h_ : This files provides configuration for the logging component to set the log level.
- _shell_config.h_ : This file provides configuration for Shell component.
- _w61_driver_config.h_ : This file provides configuration for the W61 configuration module.

###  __Application configuration__

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

/** Delay to wait before rebooting the host device, waiting for NCP device to finish update */
#define FOTA_DELAY_BEFORE_REBOOT    16000

/** Stack size of the FOTA application, this value needs to take into account the HTTP client
  * and NCP OTA static data allocation */
#define FOTA_TASK_STACK_SIZE        1800

/** The max size of the URI supported, this because the buffer
  * that will receive this info is allocated at compile time (static) */
#define FOTA_URI_MAX_SIZE           256

/** Default URI for the ST67 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI               "/download/st67w611m_mission_t01_v2.0.89.bin.ota"

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

/** Size of the buffer used to transfer the OTA header to the ST67 in one shot (required by ST67) */
#define OTA_HEADER_SIZE             512

/** Multiple of data length that should be written in ST67, recommendation to ensure correct write into ST67 memory */
#define OTA_SECTOR_ALIGNMENT        256
```

## __Known limitations__

  - W6X_WiFi_Connect API cannot use special characters [,"\\] in the SSID and password. If needed, they must be preceded by a \\ to be interpreted correctly
  - By default the country code / region configured in the device is World with 1 to 13 active channels
  - Enabling Wi-Fi DTIM can generates some failure during Network transaction
  - Static IP addressing is not compatible with power save mode (ARP broadcast issue)
  - W6X_Ble_SetDeviceName API cannot use special characters [,"\\] in the device name. If needed, they must be preceded by a \\ to be interpreted correctly
  - The Host STOP Power mode is not supported in this application
  - The TCP echo server (tcpbin.com) used in example has two limitations:
    - Messages shall end by \n (0x0A)
    - messages shall not contain \r (0x0D)
  - W6X_MQTT_Configure API cannot use special characters [,"\\] in the username and password. If needed, they must be preceded by a \\ to be interpreted correctly
  - W6X_MQTT_Publish cannot send message larger than 1470 bytes
