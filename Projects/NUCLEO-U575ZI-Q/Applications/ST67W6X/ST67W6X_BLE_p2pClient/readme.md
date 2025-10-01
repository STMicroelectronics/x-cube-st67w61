# __ST67W6X_BLE_p2pClient Application Description__

This application aims to demonstrate Point-to-Point communication using Bluetooth Low Energy (as GATT Client).

ST67W6X_BLE_p2pClient application scans and connects to device using the ST67W6X_BLE_p2pServer application.
Once connected, P2P Client can write a message to the P2P Server and receive notification from it.

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t01_v2.0.89.bin.
>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Utilities/Binaries/README.md) instructions using the __NCP_update_mission_profile.bat__ script.

## __Keywords__

Connectivity, BLE, ST67W6X_Network_Driver, FreeRTOS

## __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_BLE_P2PClient](https://wiki.st.com/stm32mcu/wiki/Connectivity:ST67W6X_BLE_Peer_To_Peer).

## __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_BLE_p2pClient/Appli/App/|                                                |Main application code directory|
|ST67W6X_BLE_p2pClient/Appli/Target/|                                             |Logging, Shell, Low-Power and BSP interfaces|
|ST67W6X_BLE_p2pClient/Core/Src|                                                  |STM32CubeMX generated sources code|
|ST67W6X_BLE_p2pClient/Core/Inc|                                                  |STM32CubeMX generated header files|
|ST67W6X_BLE_p2pClient/ST67W6X/App|                                               |Entry point to start the application associated to the ST67W6X_Network_Driver Middleware|
|ST67W6X_BLE_p2pClient/ST67W6X/Target|                                            |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_BLE_p2pClient/EWARM|                                                     |Project for the IAR Embedded workbench for Arm|
|ST67W6X_BLE_p2pClient/MDK-ARM|                                                   |Project for the RealView Microcontroller Development Kit|
|ST67W6X_BLE_p2pClient/STM32CubeIDE|                                              |Project for the STM32CubeIDE toolchain|

## __Directory contents__


|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_BLE_p2pClient/Appli/App/app_config.h|                                    |Configuration for main application|
|ST67W6X_BLE_p2pClient/Appli/App/main_app.h|                                      |Header for main_app.c|
|ST67W6X_BLE_p2pClient/Appli/Target/freertos_tickless.h|                          |Management of timers and ticks header file|
|ST67W6X_BLE_p2pClient/Appli/Target/logshell_ctrl.h|                              |Header for logshell_ctrl.h|
|ST67W6X_BLE_p2pClient/Appli/Target/stm32_lpm_if.h|                               |Header for stm32_lpm_if.c module (device specific LP management)|
|ST67W6X_BLE_p2pClient/Appli/Target/utilities_conf.h|                             |Header for configuration file to utilities|
|ST67W6X_BLE_p2pClient/Core/Inc/app_freertos.h|                                   |FreeRTOS applicative header file|
|ST67W6X_BLE_p2pClient/Core/Inc/FreeRTOSConfig.h|                                 |Header for FreeRTOS application specific definitions|
|ST67W6X_BLE_p2pClient/Core/Inc/main.h|                                           |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_BLE_p2pClient/Core/Inc/stm32u5xx_hal_conf.h|                             |HAL configuration file.|
|ST67W6X_BLE_p2pClient/Core/Inc/stm32u5xx_it.h|                                   |This file contains the headers of the interrupt handlers.|
|ST67W6X_BLE_p2pClient/ST67W6X/App/app_st67w6x.h|                                 |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.1.0 instances.|
|ST67W6X_BLE_p2pClient/ST67W6X/Target/bsp_conf.h|                                 |This file contains definitions for the BSP interface|
|ST67W6X_BLE_p2pClient/ST67W6X/Target/logging_config.h|                           |Header file for the W6X Logging configuration module|
|ST67W6X_BLE_p2pClient/ST67W6X/Target/shell_config.h|                             |Header file for the W6X Shell configuration module|
|ST67W6X_BLE_p2pClient/ST67W6X/Target/w61_driver_config.h|                        |Header file for the W61 configuration module|
|ST67W6X_BLE_p2pClient/ST67W6X/Target/w6x_config.h|                               |Header file for the W6X configuration module|
|      |                                                                          |           |
|ST67W6X_BLE_p2pClient/Appli/App/main_app.c|                                      |main_app program body|
|ST67W6X_BLE_p2pClient/Appli/Target/freertos_tickless.c|                          |Management of timers and ticks|
|ST67W6X_BLE_p2pClient/Appli/Target/logshell_ctrl.c|                              |logshell_ctrl (uart interface)|
|ST67W6X_BLE_p2pClient/Appli/Target/stm32_lpm_if.c|                               |Low layer function to enter/exit low power modes (stop, sleep)|
|ST67W6X_BLE_p2pClient/Core/Src/app_freertos.c|                                   |Code for freertos applications|
|ST67W6X_BLE_p2pClient/Core/Src/main.c|                                           |Main program body|
|ST67W6X_BLE_p2pClient/Core/Src/stm32u5xx_hal_msp.c|                              |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_BLE_p2pClient/Core/Src/stm32u5xx_hal_timebase_tim.c|                     |HAL time base based on the hardware TIM.|
|ST67W6X_BLE_p2pClient/Core/Src/stm32u5xx_it.c|                                   |Interrupt Service Routines.|
|ST67W6X_BLE_p2pClient/Core/Src/system_stm32u5xx.c|                               |CMSIS Cortex-M33 Device Peripheral Access Layer System Source File|
|ST67W6X_BLE_p2pClient/ST67W6X/App/app_st67w6x.c|                                 |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.1.0 instances.|
|ST67W6X_BLE_p2pClient/ST67W6X/Target/spi_port.c|                                 |SPI bus interface porting layer implementation|
|ST67W6X_BLE_p2pClient/STM32CubeIDE/Application/User/Core/syscalls.c|             |STM32CubeIDE Minimal System calls file|
|ST67W6X_BLE_p2pClient/STM32CubeIDE/Application/User/Core/sysmem.c|               |STM32CubeIDE System Memory calls file|


## __Hardware and Software environment__

  - This example runs on the NUCLEO-U575ZI-Q board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-U575ZI-Q board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9
  - The USER_BUTTON refers to the blue button mounted on the __X-NUCLEO-67W61M1__.<br/>
    Indeed, the user button on the STM32 Nucleo board is not used as external interrupt mode due to conflict with other EXTI pin requirement.
  - This example requires to have a green LED defined with the user label LED_GREEN on the host board.

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

###  __ST67W6X configuration__

The default System configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1
```

The default BLE configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** String defining BLE hostname */
#define W6X_BLE_HOSTNAME                        "p2pC"
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

The host low power mode can be modified in the _Appli/App/app_config.h_ file:
```
/** Low power configuration [0: disable / 1: sleep / 2: stop / 3: standby] */
#define LOW_POWER_MODE              LOW_POWER_SLEEP_ENABLE
```

The NCP low power mode can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** NCP will go by default in low power mode when NCP is in idle mode
  * Note: External clock oscillator must be used to support BLE in low power mode, if W6X_POWER_SAVE_AUTO is set to 1 but clock mode is not correct W6X_Ble_Init() API disable the low power */
#define W6X_POWER_SAVE_AUTO                     0
```
> [!IMPORTANT]
> Bluetooth Low Energy connection with NCP in Power save mode requires accurate external low-frequency clock.
> In order to setup and use external clock, SW and HW settings have to be modified.
> Refer to [Wiki Bluetooth LE with low-power setup](https://wiki.st.com/stm32mcu/wiki/Connectivity:How_to_measure_ST67W611M_current_consumption#Bluetooth-C2-AE_LE_with_low-power_setup) page in order to be informed about required changes.

Power up p2pClient devices next to a p2pServer device.
 - On p2pClient device, click on the User Button of the X-NUCLEO-67W61M1 to launch a scan. 
   A connection is initiated if a p2pServer device is detected.
   If no p2pServer is detected, relaunch a scan by pressing the User button.

 - Once the p2pClient application is connected to a p2pServer:
    - On p2pClient device, click on the X-NUCLEO-67W61M1 User button, to write a message toward connected p2pServer device, the green LED on the Server device should switch on/off.
    - On p2pServer device, click on the X-NUCLEO-67W61M1 User button to send a notification message toward connected p2pClient device, the green LED on the Client device should switch on/off.

## __Known limitations__

  - W6X_Ble_SetDeviceName API cannot use special characters [,"\\] in the device name. If needed, they must be preceded by a \\ to be interpreted correctly
