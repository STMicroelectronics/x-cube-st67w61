## __ST67W6X_BLE_Commissioning Application Description__

This application aims to demonstrate the Wi-Fi credentials commissioning and FOTA over Bluetooth Low Energy.

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

> [!IMPORTANT]
> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t01_v2.0.106.bin.<br/>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Scripts/Binaries/README.md) instructions using the __NCP_update_mission_profile_t01.bat__ script.

### __Keywords__

Connectivity, BLE, WiFi, ST67W6X_Network_Driver, FreeRTOS, CLI, Station mode, DHCP, Ping, Scan, TCP, UDP, WPA2, WPA3

### __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_BLE_Commissioning](https://wiki.st.com/stm32mcu/wiki/Connectivity:ST67W6X_BLE_Commissioning_Application).

### __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_BLE_Commissioning/Appli/App/|                                            |Main application code directory|
|ST67W6X_BLE_Commissioning/Appli/Target/|                                         |Logging, Shell, Low-Power and BSP interfaces|
|ST67W6X_BLE_Commissioning/Core/Src|                                              |STM32CubeMX generated sources code|
|ST67W6X_BLE_Commissioning/Core/Inc|                                              |STM32CubeMX generated header files|
|ST67W6X_BLE_Commissioning/ST67W6X/App|                                           |Entry point to start the application associated to the ST67W6X_Network_Driver Middleware|
|ST67W6X_BLE_Commissioning/ST67W6X/Target|                                        |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_BLE_Commissioning/EWARM|                                                 |Project for the IAR Embedded workbench for Arm|
|ST67W6X_BLE_Commissioning/MDK-ARM|                                               |Project for the RealView Microcontroller Development Kit|
|ST67W6X_BLE_Commissioning/STM32CubeIDE|                                          |Project for the STM32CubeIDE toolchain|

### __Directory contents__

|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_BLE_Commissioning/Appli/App/app_config.h|                                |Configuration for main application|
|ST67W6X_BLE_Commissioning/Appli/App/fota_flash.h|                                |Header file for Flash operations used in FOTA.|
|ST67W6X_BLE_Commissioning/Appli/App/main_app.h|                                  |Header for main_app.c|
|ST67W6X_BLE_Commissioning/Appli/Target/freertos_tickless.h|                      |Management of timers and ticks header file|
|ST67W6X_BLE_Commissioning/Appli/Target/logshell_ctrl.h|                          |Header for logshell_ctrl module|
|ST67W6X_BLE_Commissioning/Appli/Target/stm32_lpm_if.h|                           |Header for stm32_lpm_if.c module (device specific LP management)|
|ST67W6X_BLE_Commissioning/Appli/Target/utilities_conf.h|                         |Header for configuration file to utilities|
|ST67W6X_BLE_Commissioning/Core/Inc/app_freertos.h|                               |FreeRTOS applicative header file|
|ST67W6X_BLE_Commissioning/Core/Inc/FreeRTOSConfig.h|                             |Header for FreeRTOS application specific definitions|
|ST67W6X_BLE_Commissioning/Core/Inc/main.h|                                       |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_BLE_Commissioning/Core/Inc/stm32u5xx_hal_conf.h|                         |HAL configuration file.|
|ST67W6X_BLE_Commissioning/Core/Inc/stm32u5xx_it.h|                               |This file contains the headers of the interrupt handlers.|
|ST67W6X_BLE_Commissioning/ST67W6X/App/app_st67w6x.h|                             |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_BLE_Commissioning/ST67W6X/Target/bsp_conf.h|                             |This file contains definitions for the BSP interface|
|ST67W6X_BLE_Commissioning/ST67W6X/Target/logging_config.h|                       |Header file for the W6X Logging configuration module|
|ST67W6X_BLE_Commissioning/ST67W6X/Target/shell_config.h|                         |Header file for the W6X Shell configuration module|
|ST67W6X_BLE_Commissioning/ST67W6X/Target/w61_driver_config.h|                    |Header file for the W61 configuration module|
|ST67W6X_BLE_Commissioning/ST67W6X/Target/w6x_config.h|                           |Header file for the W6X configuration module|
|      |                                                                          |           |
|ST67W6X_BLE_Commissioning/Appli/App/fota_flash.c|                                |This file provides code for Flash operations used in FOTA.|
|ST67W6X_BLE_Commissioning/Appli/App/main_app.c|                                  |main_app program body|
|ST67W6X_BLE_Commissioning/Appli/Target/freertos_tickless.c|                      |Management of timers and ticks|
|ST67W6X_BLE_Commissioning/Appli/Target/logshell_ctrl.c|                          |logshell_ctrl (uart interface)|
|ST67W6X_BLE_Commissioning/Appli/Target/stm32_lpm_if.c|                           |Low layer function to enter/exit low power modes (stop, sleep)|
|ST67W6X_BLE_Commissioning/Core/Src/app_freertos.c|                               |Code for freertos applications|
|ST67W6X_BLE_Commissioning/Core/Src/main.c|                                       |Main program body|
|ST67W6X_BLE_Commissioning/Core/Src/stm32u5xx_hal_msp.c|                          |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_BLE_Commissioning/Core/Src/stm32u5xx_hal_timebase_tim.c|                 |HAL time base based on the hardware TIM.|
|ST67W6X_BLE_Commissioning/Core/Src/stm32u5xx_it.c|                               |Interrupt Service Routines.|
|ST67W6X_BLE_Commissioning/Core/Src/system_stm32u5xx.c|                           |CMSIS Cortex-M33 Device Peripheral Access Layer System Source File|
|ST67W6X_BLE_Commissioning/ST67W6X/App/app_st67w6x.c|                             |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_BLE_Commissioning/ST67W6X/Target/spi_port.c|                             |SPI bus interface porting layer implementation|
|ST67W6X_BLE_Commissioning/ST67W6X/Target/util_task_port.c|                       |Task Performance porting layer implementation|
|ST67W6X_BLE_Commissioning/STM32CubeIDE/Application/User/Core/syscalls.c|         |STM32CubeIDE Minimal System calls file|
|ST67W6X_BLE_Commissioning/STM32CubeIDE/Application/User/Core/sysmem.c|           |STM32CubeIDE System Memory calls file|

### __Hardware and Software environment__

  - This example runs on the NUCLEO-U575ZI-Q board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-U575ZI-Q board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9
  - The USER_BUTTON refers to the blue button mounted on the __X-NUCLEO-67W61M1__.<br/>
    Indeed, the user button on the STM32 Nucleo board is not used as external interrupt mode due to conflict with other EXTI pin requirement.

For further information, please visit the dedicated Wiki page [ST67W611M Hardware setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup).

### __How to use it?__

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

#### BLE Wi-Fi Commissioning

Enable Bluetooth option on your remote client (Laptop or smartphone),
open the [Web Bluetooth page](https://applible.github.io/Web_Bluetooth_App_ST67/) and follow the below instructions to connect to the device:

  - Click on "Connect" Button to scan and connect to your device through Bluetooth Low Energy
  - Once connected through BLE, click on Wifi Commissioning banner to open the commissioning interface
  - If the device is already connected to a Wi-Fi access point the Access Point name and the Wi-Fi connection status is displayed
  - Else "Wi-Fi Scan Request" button allows to scan all Wi-Fi networks around and displays them as a list
  - Select the Wi-Fi network you want to connect to and type a password if necessary
  - Click on "Wi-Fi Connection Request" button once your Wi-Fi network is selected and password
  - After some times the connection status is displayed in the "Wifi State" interface
  - Once connected to a Wi-Fi access point "Ping" button allows to launch a Wi-Fi ping and displays results on the interface
  - Disconnect from Wi-Fi access point by clicking "Disconnect" button from the Wifi State interface.

Press User Button while connected to end BLE connection.
Press User Button while not connected to clear the bonded devices list.

#### BLE FUOTA

The application supports Firmware Update Over The Air (FUOTA) via Bluetooth Low Energy. This allows you to update the firmware of the ST67W6X device wirelessly using BLE.

##### How it works

The BLE FUOTA service exposes three main characteristics:

  - **Base Address**: Used to initiate, stop, or cancel the FUOTA process and to specify the target address and sectors for the update.
  - **Configuration**: Used for confirmation and status indication during the update process.
  - **Raw Data**: Used to transfer firmware data chunks to the device.

The update process is managed by a dedicated task that receives BLE write events and processes FUOTA commands:

  - **Start**: Initiates the FUOTA process and allocates a buffer for incoming firmware data.
  - **Data Transfer**: Firmware data is sent in chunks via the Raw Data characteristic and written to the buffer.
  - **Finish**: When all data is received, the buffer is written to flash memory and the device confirms completion.
  - **Stop/Cancel**: The process can be stopped or cancelled at any time, freeing resources and resetting the state.

##### Usage

To perform a firmware update:

1. Connect to the device using a BLE client that supports FUOTA (such as the [ST67 Web Bluetooth page](https://applible.github.io/Web_Bluetooth_App_ST67/)).
2. Use the FUOTA interface to send the firmware image.

For more details, refer to the implementation in [`main_app.c`](Appli/App/main_app.c) and the website describing the characteristics for the [`STM32 WBA FUOTA`](https://wiki.st.com/stm32mcu/wiki/Connectivity:STM32WBA_FUOTA).

### __User setup__

#### __ST67W6X configuration__

The default System configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** NCP will go by default in low power mode when NCP is in idle mode
  * Note: External clock oscillator must be used to support BLE in low power mode, if W6X_POWER_SAVE_AUTO is set to 1 but clock mode is not correct W6X_Ble_Init() API disable the low power */
#define W6X_POWER_SAVE_AUTO                     0

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1U
```

> [!IMPORTANT]
> Bluetooth Low Energy connection with NCP in Power save mode requires accurate external low-frequency clock.<br/>
> In order to setup and use external clock, SW and HW settings have to be modified.<br/>
> Refer to [Wiki Bluetooth LE with low-power setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:How_to_measure_ST67W611M_current_consumption#Bluetooth-C2-AE_LE_with_low-power_setup) page in order to be informed about required changes.

The default BLE configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** String defining BLE hostname */
#define W6X_BLE_HOSTNAME                        "ST_WiFi"
```

The default Wi-Fi configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** Boolean to enable/disable autoconnect functionality */
#define W6X_WIFI_AUTOCONNECT                    1

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
#define W6X_NET_DHCP                            1U

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
