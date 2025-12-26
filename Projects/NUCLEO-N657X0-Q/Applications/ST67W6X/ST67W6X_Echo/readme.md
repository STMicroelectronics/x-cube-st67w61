## __ST67W6X_Echo Application Description__

This application aims to demonstrate the TCP Echo feature over Wi-Fi.

It exercises the ST67W6X_Network_Driver capabilities. It relies on the FreeRTOS RealTime Operating System.

> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t01_v2.0.97.bin.
>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Scripts/Binaries/README.md) instructions using the __NCP_update_mission_profile_t01.bat__ script.

### __Keywords__

Connectivity, WiFi, ST67W6X_Network_Driver, FreeRTOS, CLI, Station mode, DHCP, Ping, Echo, Scan, TCP, UDP, WPA2, WPA3

### __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_Echo](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_ST67W6X_Echo_Application).

### __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_Echo/FSBL/Appli/App/|                                                    |Main application code directory|
|ST67W6X_Echo/FSBL/Appli/Target/|                                                 |Logging, Shell, Low-Power and BSP interfaces|
|ST67W6X_Echo/FSBL/Core/Src|                                                      |STM32CubeMX generated sources code|
|ST67W6X_Echo/FSBL/Core/Inc|                                                      |STM32CubeMX generated header files|
|ST67W6X_Echo/FSBL/ST67W6X/App|                                                   |Entry point to start the application associated to the ST67W6X_Network_Driver Middleware|
|ST67W6X_Echo/FSBL/ST67W6X/Target|                                                |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_Echo/EWARM|                                                              |Project for the IAR Embedded workbench for Arm|
|ST67W6X_Echo/MDK-ARM|                                                            |Project for the RealView Microcontroller Development Kit|
|ST67W6X_Echo/STM32CubeIDE|                                                       |Project for the STM32CubeIDE toolchain|

### __Directory contents__

|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_Echo/FSBL/Appli/App/app_config.h|                                        |Configuration for main application|
|ST67W6X_Echo/FSBL/Appli/App/echo.h|                                              |Echo test definition|
|ST67W6X_Echo/FSBL/Appli/App/main_app.h|                                          |Header for main_app.c|
|ST67W6X_Echo/FSBL/Appli/Target/logshell_ctrl.h|                                  |Header for logshell_ctrl.h|
|ST67W6X_Echo/FSBL/Core/Inc/app_freertos.h|                                       |FreeRTOS applicative header file|
|ST67W6X_Echo/FSBL/Core/Inc/FreeRTOSConfig.h|                                     |Header for FreeRTOS application specific definitions|
|ST67W6X_Echo/FSBL/Core/Inc/main.h|                                               |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_Echo/FSBL/Core/Inc/stm32n6xx_hal_conf.h|                                 |HAL configuration file.|
|ST67W6X_Echo/FSBL/Core/Inc/stm32n6xx_it.h|                                       |This file contains the headers of the interrupt handlers.|
|ST67W6X_Echo/FSBL/ST67W6X/App/app_st67w6x.h|                                     |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.2.0 instances.|
|ST67W6X_Echo/FSBL/ST67W6X/Target/bsp_conf.h|                                     |This file contains definitions for the BSP interface|
|ST67W6X_Echo/FSBL/ST67W6X/Target/logging_config.h|                               |Header file for the W6X Logging configuration module|
|ST67W6X_Echo/FSBL/ST67W6X/Target/shell_config.h|                                 |Header file for the W6X Shell configuration module|
|ST67W6X_Echo/FSBL/ST67W6X/Target/w61_driver_config.h|                            |Header file for the W61 configuration module|
|ST67W6X_Echo/FSBL/ST67W6X/Target/w6x_config.h|                                   |Header file for the W6X configuration module|
|      |                                                                          |           |
|ST67W6X_Echo/FSBL/Appli/App/echo.c|                                              |Test an echo with a server|
|ST67W6X_Echo/FSBL/Appli/App/main_app.c|                                          |main_app program body|
|ST67W6X_Echo/FSBL/Appli/Target/logshell_ctrl.c|                                  |logshell_ctrl (uart interface)|
|ST67W6X_Echo/FSBL/Core/Src/app_freertos.c|                                       |Code for freertos applications|
|ST67W6X_Echo/FSBL/Core/Src/main.c|                                               |Main program body|
|ST67W6X_Echo/FSBL/Core/Src/stm32n6xx_hal_msp.c|                                  |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_Echo/FSBL/Core/Src/stm32n6xx_hal_timebase_tim.c|                         |HAL time base based on the hardware TIM.|
|ST67W6X_Echo/FSBL/Core/Src/stm32n6xx_it.c|                                       |Interrupt Service Routines.|
|ST67W6X_Echo/FSBL/Core/Src/system_stm32n6xx_fsbl.c|                              |CMSIS Cortex-M55 Device Peripheral Access Layer System Source File<br>to be used after the boot ROM execution in an applicative code called<br>"FSBL" for First Stage Boot Loader.|
|ST67W6X_Echo/FSBL/ST67W6X/App/app_st67w6x.c|                                     |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.2.0 instances.|
|ST67W6X_Echo/FSBL/ST67W6X/Target/spi_port.c|                                     |SPI bus interface porting layer implementation|
|ST67W6X_Echo/FSBL/ST67W6X/Target/util_task_port.c|                               |Task Performance porting layer implementation|
|ST67W6X_Echo/MDK-ARM/startup_stm32n657xx_fsbl.c|                                 |CMSIS-Core Device Startup File for STM32N6xx Device <br>after bootROM execution|
|ST67W6X_Echo/STM32CubeIDE/FSBL/Application/User/Core/syscalls.c|                 |STM32CubeIDE Minimal System calls file|
|ST67W6X_Echo/STM32CubeIDE/FSBL/Application/User/Core/sysmem.c|                   |STM32CubeIDE System Memory calls file|

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
    - Resort to STM32CubeProgrammer to add a header to the generated binary "ST67W6X_Echo_FSBL.bin" with the following command
      ```
      STM32_SigningTool_CLI.exe -bin ST67W6X_Echo_FSBL.bin -nk -of 0x80000000 -t fsbl -o ST67W6X_Echo_FSBL_trusted.bin -hv 2.3 -dump ST67W6X_Echo_FSBL_trusted.bin
      ```
      __Warning__ If using STM32CubeProgrammer __v2.21__ version or more recent, add ___--align___ option in the command line.

      The resulting binary is ST67W6X_Echo_FSBL_trusted.bin
    - Open STM32CubeProgrammer
      - Select the External loader MX25UM51245G_STM32N6570-NUCLEO
      - Load the binary and its header (ST67W6X_Echo_FSBL_trusted.bin) in the board external Flash at address 0x7000'0000
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

#### __ST67W6X configuration__

The default System configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
/** NCP power save mode : 0: NCP stays always active / 1: NCP goes in low power mode when idle */
#define W6X_POWER_SAVE_AUTO                     1

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1
```

> __IMPORTANT__
>
> In order to setup and use external clock, SW and HW settings have to be modified.
> Refer to [Wiki ST67W611M1 32.768 kHz and low power operation](https://wiki.st.com/stm32mcu/wiki/Connectivity:ST67W611M1_32KHz_management) page in order to be informed about required changes.

The default Wi-Fi configuration can be modified in the _ST67W6X/Target/w6x_config.h_ file:
```
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
#define W6X_NET_DHCP                            1

/** String defining Wi-Fi hostname */
#define W6X_NET_HOSTNAME                        "ST67W61_WiFi"
```

Additionally, some others options can be modified in the _ST67W6X/Target_ directory with different configuration files as below:

- _logging_config.h_ : This files provides configuration for the logging component to set the log level.
- _shell_config.h_ : This file provides configuration for Shell component.
- _w61_driver_config.h_ : This file provides configuration for the W61 configuration module.

#### __Application configuration__

The Wi-Fi configuration used in this application is define in the _Appli/App/app_config.h_ file:
```
#define WIFI_SSID                   "YOUR_SSID"

#define WIFI_PASSWORD               "YOUR_PASSWORD"
```

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

### __Known limitations__

  - The TCP echo server (tcpbin.com) used in example has two limitations: Messages shall end by \n (0x0A) and not contain \r (0x0D)
