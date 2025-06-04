# __ST67W6X_MQTT Application Description__

This application aims to demonstrate an example of MQTT client over Wi-Fi which connects to an MQTT broker in order to publish telemetry data and receive parameter updates or commands from the cloud.

This application will send and receive data to an MQTT broker (this example uses HiveMQ which can be changed to your preferred MQTT broker).\
To monitor the data sent and received, to/from the topics, a MQTT client can be installed on a PC, smartphone or tablet.\
Subscribing to the configured topics, will allows the client to be notified every time the device publish data to this topic.\
In the opposite direction, from this same client you can also publish data on the "control field" to change:
 - the LED state on the Host board
 - and to issue a command to reboot the host.

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t01_v2.0.75.bin.
>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Utilities/Binaries/README.md) instructions using the __NCP_update_mission_profile.bat__ script.

## __Keywords__

Connectivity, WiFi, ST67W6X_Network_Driver, FreeRTOS, CLI, Station mode, DHCP, SNTP, MQTT, JSON, Scan, TCP, UDP, WPA2, WPA3, IKS

## __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_MQTT](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wifi_ST67W6X_MQTT_Demonstration).

## __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_MQTT/App_MQTT/App/|                                                      |Main application code directory|
|ST67W6X_MQTT/App_MQTT/Target/|                                                   |Logging, Shell, Low-Power and SPI BSP interfaces|
|ST67W6X_MQTT/App_MQTT/Core/Src|                                                  |STM32CubeMX generated sources code|
|ST67W6X_MQTT/App_MQTT/Core/Inc|                                                  |STM32CubeMX generated header files|
|ST67W6X_MQTT/App_MQTT/ST67W6X/Target|                                            |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_MQTT/App_MQTT/littlefs/lfs|                                              |Certificates used to execute secure operations|
|ST67W6X_MQTT/App_MQTT/littlefs/Target|                                           |Configuration and port files to manage the littlefs in flash|
|ST67W6X_MQTT/App_MQTT/X-CUBE-MEMS1/App|                                          |Sensors interface|
|ST67W6X_MQTT/App_MQTT/X-CUBE-MEMS1/Target|                                       |Configuration of IKS MEMS1 BSP|
|ST67W6X_MQTT/EWARM|                                                              |Project for the IAR Embedded workbench for Arm|
|ST67W6X_MQTT/MDK-ARM|                                                            |Project for the RealView Microcontroller Development Kit|
|ST67W6X_MQTT/STM32CubeIDE|                                                       |Project for the STM32CubeIDE toolchain|

## __Directory contents__


|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_MQTT/App_MQTT/App/app_config.h|                                          |Configuration for main application|
|ST67W6X_MQTT/App_MQTT/App/main_app.h|                                            |Header for main_app.c|
|ST67W6X_MQTT/App_MQTT/Target/freertos_tickless.h|                                |Management of timers and ticks header file|
|ST67W6X_MQTT/App_MQTT/Target/logshell_ctrl.h|                                    |Header for logshell_ctrl.h|
|ST67W6X_MQTT/App_MQTT/Target/spi_port_conf.h|                                    |Interfaces/maps the SPI instance to be used for NCP communication|
|ST67W6X_MQTT/App_MQTT/Target/stm32_lpm_if.h|                                     |Header for stm32_lpm_if.c module (device specific LP management)|
|ST67W6X_MQTT/App_MQTT/Target/utilities_conf.h|                                   |Header for configuration file to utilities|
|ST67W6X_MQTT/Core/Inc/app_freertos.h|                                            |FreeRTOS applicative header file|
|ST67W6X_MQTT/Core/Inc/FreeRTOSConfig.h|                                          |Header for FreeRTOS application specific definitions|
|ST67W6X_MQTT/Core/Inc/main.h|                                                    |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_MQTT/Core/Inc/stm32u5xx_hal_conf.h|                                      |HAL configuration file.|
|ST67W6X_MQTT/Core/Inc/stm32u5xx_it.h|                                            |This file contains the headers of the interrupt handlers.|
|ST67W6X_MQTT/Core/Inc/stm32u5xx_nucleo_bus.h|                                    |header file for the BSP BUS IO driver|
|ST67W6X_MQTT/Core/Inc/stm32u5xx_nucleo_conf.h|                                   |Configuration file|
|ST67W6X_MQTT/Core/Inc/stm32u5xx_nucleo_errno.h|                                  |Error Code|
|ST67W6X_MQTT/littlefs/Target/easyflash.h|                                        |Header file that adapts LittleFS to EasyFlash4|
|ST67W6X_MQTT/littlefs/Target/lfs_port.h|                                         |lfs flash port definition|
|ST67W6X_MQTT/littlefs/Target/lfs_util_config.h|                                  |lfs utility user configuration|
|ST67W6X_MQTT/ST67W6X/Target/logging_config.h|                                    |Header file for the W6X Logging configuration module|
|ST67W6X_MQTT/ST67W6X/Target/shell_config.h|                                      |Header file for the W6X Shell configuration module|
|ST67W6X_MQTT/ST67W6X/Target/w61_driver_config.h|                                 |Header file for the W61 configuration module|
|ST67W6X_MQTT/ST67W6X/Target/w6x_config.h|                                        |Header file for the W6X configuration module|
|ST67W6X_MQTT/X-CUBE-MEMS1/App/sys_sensors.h|                                     |Header for sensors application|
|ST67W6X_MQTT/X-CUBE-MEMS1/Target/iks4a1_conf.h|                                  |This file contains definitions for the MEMS components bus interfaces|
|      |                                                                          |           |
|ST67W6X_MQTT/App_MQTT/App/main_app.c|                                            |main_app program body|
|ST67W6X_MQTT/App_MQTT/Target/freertos_tickless.c|                                |Management of timers and ticks|
|ST67W6X_MQTT/App_MQTT/Target/logshell_ctrl.c|                                    |logshell_ctrl (uart interface)|
|ST67W6X_MQTT/App_MQTT/Target/stm32_lpm_if.c|                                     |Low layer function to enter/exit low power modes (stop, sleep)|
|ST67W6X_MQTT/Core/Src/app_freertos.c|                                            |Code for freertos applications|
|ST67W6X_MQTT/Core/Src/main.c|                                                    |Main program body|
|ST67W6X_MQTT/Core/Src/stm32u5xx_hal_msp.c|                                       |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_MQTT/Core/Src/stm32u5xx_hal_timebase_tim.c|                              |HAL time base based on the hardware TIM.|
|ST67W6X_MQTT/Core/Src/stm32u5xx_it.c|                                            |Interrupt Service Routines.|
|ST67W6X_MQTT/Core/Src/stm32u5xx_nucleo_bus.c|                                    |source file for the BSP BUS IO driver|
|ST67W6X_MQTT/Core/Src/system_stm32u5xx.c|                                        |CMSIS Cortex-M33 Device Peripheral Access Layer System Source File|
|ST67W6X_MQTT/littlefs/Target/lfs_easyflash.c|                                    |Adapts LittleFS to EasyFlash4|
|ST67W6X_MQTT/littlefs/Target/lfs_flash.c|                                        |Host flash interface|
|ST67W6X_MQTT/ST67W6X/Target/spi_port.c|                                          |SPI bus interface porting layer implementation|
|ST67W6X_MQTT/STM32CubeIDE/Application/User/Core/syscalls.c|                      |STM32CubeIDE Minimal System calls file|
|ST67W6X_MQTT/STM32CubeIDE/Application/User/Core/sysmem.c|                        |STM32CubeIDE System Memory calls file|
|ST67W6X_MQTT/X-CUBE-MEMS1/App/sys_sensors.c|                                     |Manages the sensors on the application|


## __Hardware and Software environment__

  - This example runs on the NUCLEO-U575ZI-Q board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-U575ZI-Q board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9

For further information, please visit the dedicated Wiki page [ST67W611M Hardware set-up](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wifi_MCU_Hardware_Setup).

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

There is a json configuration file available (resources/IotMQTTPanel-241108_143953.json). This file can be used to configure the IoT MQTT Panel Android Application to connect to the MQTT broker and subscribe to the topics used by the device.

The application is available on the Google Play Store: [IoT MQTT Panel](https://play.google.com/store/apps/details?id=snr.lab.iotmqttpanel.prod) or on the Apple App Store: [IoT MQTT Panel](https://apps.apple.com/pl/app/iot-mqtt-panel/id6466780124).

After installing the application, you can import the configuration file with the "Backup and Restore" feature from the "Connections" menu.

You can change the MQTT broker configuration to be aligned with application setup. Additionally, you can change the MQTT topic to subscribe to the topic used by the device.

##  __User setup__

###  __ST67W6X configuration__

The default System configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file.
```
/** NCP will go by default in low power mode when NCP is in idle mode */
#define W6X_POWER_SAVE_AUTO                     1

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1
```

The default Wi-Fi configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file :
```
/** Define the DHCP configuration : 0: NO DHCP, 1: DHCP CLIENT STA, 2:DHCP SERVER AP, 3: DHCP STA+AP */
#define W6X_WIFI_DHCP                           1

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

The Wi-Fi configuration used in this application is define in the _App_MQTT/App/app_config.h_ file :
```
#define WIFI_SSID                   "YOUR_SSID"

#define WIFI_PASSWORD               "YOUR_PASSWORD"
```

The logging output mode can be modified in the _App_MQTT/App/app_config.h_ file :
```
/** Select output log mode [0: printf / 1: UART / 2: ITM] */
#define LOG_OUTPUT_MODE             LOG_OUTPUT_UART
```

The host low power mode can be modified in the _App_MQTT/App/app_config.h_ file :
```
/** Low power configuration [0: disable / 1: sleep / 2: stop / 3: standby] */
#define LOW_POWER_MODE              LOW_POWER_SLEEP_ENABLE
```

The MQTT client configuration can be modified in the _App_MQTT/App/app_config.h_ file :
```
/** Subscribed topic max buffer size */
#define MQTT_TOPIC_BUFFER_SIZE      100

/** Subscribed message max buffer size */
#define MQTT_MSG_BUFFER_SIZE        600

/** Host name of remote MQTT Broker
  * Multiple options are possibles:
  *   - broker.hivemq.com
  *   - broker.emqx.io
  *   - test.mosquitto.org
  */
#define MQTT_HOST_NAME              "broker.emqx.io"

/** Port of remote MQTT Broker */
#define MQTT_HOST_PORT              1883

/** Security level */
#define MQTT_SECURITY_LEVEL         0

/** MQTT Client ID to be identified on MQTT Broker */
#define MQTT_CLIENT_ID              "mySTM32_772"

/** MQTT Username to be identified on MQTT Broker. Not used in non-secure */
#define MQTT_USERNAME               "MyName"

/** MQTT Password to be identified on MQTT Broker. Not used in non-secure */
#define MQTT_USER_PASSWORD          "MyPsw"

/** MQTT Client Certificate. Required when the scheme is greater or equal to 3 */
#define MQTT_CERTIFICATE            "client_1.crt"

/** MQTTClient Private key. Required when the scheme is greater or equal to 3 */
#define MQTT_KEY                    "client_1.key"

/** MQTT Client CA certificate. Required when the scheme is greater or equal to 2 */
#define MQTT_CA_CERTIFICATE         "ca_1.crt"

/** MQTT Server Name Indication (SNI) enabled */
#define MQTT_SNI_ENABLED            1
```

By default, the device subscribe to the topic "/sensors/\<MQClientID\>" then send data periodically to the broker on the same topic.
The generated data are in JSON format and are sent every 2 seconds, depending of the MEMS sensor configuration.
If the MEMS board is not connected, the application will send a restricted data format:
```
{
  "time": "21-09-01 14:00:00"
}
```
Else, the application will send the following data format:
```
{
  "time": "21-09-01 14:00:00",
  "temperature": 25.0,
  "humidity": 50.0,
  "pressure": 1013.0,
}
```

Each time the device receives a message on a subscribed topic, the message is displayed on the UART console.
Additionally, the device can receive a command to change the LED state or to reboot the device.

## __Known limitations__

  - By default the country code / region configured in the device is World with 1 to 13 active channels.
  - Enabling Wi-Fi DTIM can generates some failure during Network transaction.
  - Static IP addressing is not compatible with DTIM lowpower mode.
