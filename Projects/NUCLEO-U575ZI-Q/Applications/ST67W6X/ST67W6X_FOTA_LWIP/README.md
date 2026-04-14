## __ST67W6X_FOTA_LWIP Application Description__

This application demonstrates the FOTA (Firmware update Over-The-Air) feature over Wi-Fi using the STM32-hosted LwIP network stack and MbedTLS.

Compared to the __ST67W6X_FOTA__ T01 project, this variant adds:

- LwIP on the host side for TCP/IP, DNS and socket handling
- MbedTLS for TLS support
- A dedicated HTTP/HTTPS client based on LwIP sockets

It exercises the ST67W6X_Network_Driver capabilities and relies on the FreeRTOS Real-Time Operating System.

> [!IMPORTANT]
> This project requires to use the ST67W611M Coprocessor binary st67w611m_mission_t02_v2.0.106.bin.<br/>
> Please follow the [NCP Binaries README.md](../../../../ST67W6X_Scripts/Binaries/README.md) instructions using the __NCP_update_mission_profile_t02.bat__ script.

### __Keywords__

Connectivity, WiFi, ST67W6X_Network_Driver, FreeRTOS, FOTA, HTTP, HTTPS, TCP, LwIP, MbedTLS, SHA256, WPA2, WPA3

### __Links and references__

For further information, please visit the dedicated Wiki page [ST67W6X_FOTA](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_ST67W6X_FOTA_Application).

### __Directory structure__

|Directory  |                                                                     |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_FOTA_LWIP/Appli/App/|                                                    |Main application code directory|
|ST67W6X_FOTA_LWIP/Appli/Target/|                                                 |Logging, Shell, Low-Power and BSP interfaces|
|ST67W6X_FOTA_LWIP/Core/Src|                                                      |STM32CubeMX generated sources code|
|ST67W6X_FOTA_LWIP/Core/Inc|                                                      |STM32CubeMX generated header files|
|ST67W6X_FOTA_LWIP/ST67W6X/App|                                                   |Entry point to start the application associated to the ST67W6X_Network_Driver Middleware|
|ST67W6X_FOTA_LWIP/ST67W6X/Target|                                                |Configuration and port files to manage the ST67W6X_Network_Driver Middleware|
|ST67W6X_FOTA_LWIP/LWIP/App|                                                      |LwIP application, LwIP shell commands, netif interface and TLS socket adaptation|
|ST67W6X_FOTA_LWIP/LWIP/Target|                                                   |Configuration files to manage the LwIP Middleware|
|ST67W6X_FOTA_LWIP/MBEDTLS/App|                                                   |MbedTLS initialization and configuration files|
|ST67W6X_FOTA_LWIP/MBEDTLS/Target|                                                |Target-specific MbedTLS support files|
|ST67W6X_FOTA_LWIP/EWARM|                                                         |Project for the IAR Embedded workbench for Arm|
|ST67W6X_FOTA_LWIP/MDK-ARM|                                                       |Project for the RealView Microcontroller Development Kit|
|ST67W6X_FOTA_LWIP/STM32CubeIDE|                                                  |Project for the STM32CubeIDE toolchain|

### __Directory contents__

|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|ST67W6X_FOTA_LWIP/Appli/App/app_config.h|                                        |Configuration for main application|
|ST67W6X_FOTA_LWIP/Appli/App/fota.h|                                              |FOTA test definition|
|ST67W6X_FOTA_LWIP/Appli/App/fota_flash.h|                                        |Header file for Flash operations used in FOTA.|
|ST67W6X_FOTA_LWIP/Appli/App/fota_header.h|                                       |Header file for FOTA header parsing and handling.|
|ST67W6X_FOTA_LWIP/Appli/App/fota_header_struct.h|                                |FOTA header structure definition|
|ST67W6X_FOTA_LWIP/Appli/App/main_app.h|                                          |Header for main_app.c|
|ST67W6X_FOTA_LWIP/Appli/Target/freertos_tickless.h|                              |Management of timers and ticks header file|
|ST67W6X_FOTA_LWIP/Appli/Target/logshell_ctrl.h|                                  |Header for logshell_ctrl module|
|ST67W6X_FOTA_LWIP/Appli/Target/stm32_lpm_if.h|                                   |Header for stm32_lpm_if.c module (device specific LP management)|
|ST67W6X_FOTA_LWIP/Appli/Target/utilities_conf.h|                                 |Header for configuration file to utilities|
|ST67W6X_FOTA_LWIP/Core/Inc/app_freertos.h|                                       |FreeRTOS applicative header file|
|ST67W6X_FOTA_LWIP/Core/Inc/FreeRTOSConfig.h|                                     |Header for FreeRTOS application specific definitions|
|ST67W6X_FOTA_LWIP/Core/Inc/main.h|                                               |Header for main.c file.<br>This file contains the common defines of the application.|
|ST67W6X_FOTA_LWIP/Core/Inc/stm32u5xx_hal_conf.h|                                 |HAL configuration file.|
|ST67W6X_FOTA_LWIP/Core/Inc/stm32u5xx_it.h|                                       |This file contains the headers of the interrupt handlers.|
|ST67W6X_FOTA_LWIP/LWIP/App/altls_mbedtls.h|                                      |Application layered TCP/TLS connection API|
|ST67W6X_FOTA_LWIP/LWIP/App/dhcp_server.h|                                        |DHCP Server definition|
|ST67W6X_FOTA_LWIP/LWIP/App/http_client.h|                                        |Simple HTTP client API|
|ST67W6X_FOTA_LWIP/LWIP/App/lwip.h|                                               |This file provides code for the configuration of the LWIP.|
|ST67W6X_FOTA_LWIP/LWIP/App/lwip_netif.h|                                         |This file provides code for the configuration of the ST67W6X Network interface over LwIP|
|ST67W6X_FOTA_LWIP/LWIP/App/ping.h|                                               |Ping module definition|
|ST67W6X_FOTA_LWIP/LWIP/App/sntp.h|                                               |This is simple "SNTP" client definition|
|ST67W6X_FOTA_LWIP/LWIP/Target/lwipopts.h|                                        |This file overrides LwIP stack default configuration|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/bpstruct.h|                                   |Packed structs support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/cc.h|                                         |specific compiler functions support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/cpu.h|                                        |specific architecture definitions<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/epstruct.h|                                   |Packed structs support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/init.h|                                       |modules initializations<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/lib.h|                                        |specific libraries support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/perf.h|                                       |perf measurement support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/LWIP/Target/arch/sys_arch.h|                                   |specific os functions support<br>This file is part of the lwIP TCP/IP stack.|
|ST67W6X_FOTA_LWIP/MBEDTLS/App/mbedtls.h|                                         |This file provides code for the configuration of the mbedtls instances.|
|ST67W6X_FOTA_LWIP/MBEDTLS/App/mbedtls_config.h|                                  |The mbedTLS custom configuration header file|
|ST67W6X_FOTA_LWIP/ST67W6X/App/app_st67w6x.h|                                     |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_FOTA_LWIP/ST67W6X/Target/bsp_conf.h|                                     |This file contains definitions for the BSP interface|
|ST67W6X_FOTA_LWIP/ST67W6X/Target/logging_config.h|                               |Header file for the W6X Logging configuration module|
|ST67W6X_FOTA_LWIP/ST67W6X/Target/shell_config.h|                                 |Header file for the W6X Shell configuration module|
|ST67W6X_FOTA_LWIP/ST67W6X/Target/w61_driver_config.h|                            |Header file for the W61 configuration module|
|ST67W6X_FOTA_LWIP/ST67W6X/Target/w6x_config.h|                                   |Header file for the W6X configuration module|
|      |                                                                          |           |
|ST67W6X_FOTA_LWIP/Appli/App/fota.c|                                              |Test a FOTA with a server|
|ST67W6X_FOTA_LWIP/Appli/App/fota_flash.c|                                        |This file provides code for Flash operations used in FOTA.|
|ST67W6X_FOTA_LWIP/Appli/App/fota_header.c|                                       |This file provides code for parsing and handling FOTA headers.|
|ST67W6X_FOTA_LWIP/Appli/App/main_app.c|                                          |main_app program body|
|ST67W6X_FOTA_LWIP/Appli/Target/freertos_tickless.c|                              |Management of timers and ticks|
|ST67W6X_FOTA_LWIP/Appli/Target/logshell_ctrl.c|                                  |logshell_ctrl (uart interface)|
|ST67W6X_FOTA_LWIP/Appli/Target/stm32_lpm_if.c|                                   |Low layer function to enter/exit low power modes (stop, sleep)|
|ST67W6X_FOTA_LWIP/Core/Src/app_freertos.c|                                       |Code for freertos applications|
|ST67W6X_FOTA_LWIP/Core/Src/main.c|                                               |Main program body|
|ST67W6X_FOTA_LWIP/Core/Src/stm32u5xx_hal_msp.c|                                  |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|ST67W6X_FOTA_LWIP/Core/Src/stm32u5xx_hal_timebase_tim.c|                         |HAL time base based on the hardware TIM.|
|ST67W6X_FOTA_LWIP/Core/Src/stm32u5xx_it.c|                                       |Interrupt Service Routines.|
|ST67W6X_FOTA_LWIP/Core/Src/system_stm32u5xx.c|                                   |CMSIS Cortex-M33 Device Peripheral Access Layer System Source File|
|ST67W6X_FOTA_LWIP/LWIP/App/altls_mbedtls.c|                                      |Application layered TLS connection API|
|ST67W6X_FOTA_LWIP/LWIP/App/dhcp_server_raw.c|                                    |A simple DHCP server implementation|
|ST67W6X_FOTA_LWIP/LWIP/App/http_client.c|                                        |This file provides code fora simple HTTP client API|
|ST67W6X_FOTA_LWIP/LWIP/App/lwip.c|                                               |This file provides initialization code for LwIP middleware.|
|ST67W6X_FOTA_LWIP/LWIP/App/lwip_netif.c|                                         |This file provides initialization code for ST67W6X Network interface over LwIP|
|ST67W6X_FOTA_LWIP/LWIP/App/ping.c|                                               |Ping application|
|ST67W6X_FOTA_LWIP/LWIP/App/sntp.c|                                               |This is simple "SNTP" client for the lwIP raw API.<br>It is a minimal implementation of SNTPv4 as specified in RFC 4330.|
|ST67W6X_FOTA_LWIP/MBEDTLS/App/mbedtls.c|                                         |This file provides code for the configuration of the mbedtls instances.|
|ST67W6X_FOTA_LWIP/MBEDTLS/Target/hardware_rng.c|                                 |mbedtls alternate entropy data function.<br>the mbedtls_hardware_poll() is customized to use the STM32 RNG<br>to generate random data, required for TLS encryption algorithms.|
|ST67W6X_FOTA_LWIP/ST67W6X/App/app_st67w6x.c|                                     |This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.|
|ST67W6X_FOTA_LWIP/ST67W6X/Target/spi_port.c|                                     |SPI bus interface porting layer implementation|
|ST67W6X_FOTA_LWIP/STM32CubeIDE/Application/User/Core/syscalls.c|                 |STM32CubeIDE Minimal System calls file|
|ST67W6X_FOTA_LWIP/STM32CubeIDE/Application/User/Core/sysmem.c|                   |STM32CubeIDE System Memory calls file|

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

### __User setup__

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
#define W6X_WIFI_AUTOCONNECT                    0

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
#define LOW_POWER_MODE              LOW_POWER_SLEEP_ENABLE
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
#define FOTA_HTTP_TIMEOUT_IN_MS     120000U

/** Set the priority of the FOTA task using FreeRTOS priority evaluation system */
#define FOTA_TASK_PRIORITY          24

/** Size of the buffer used to transfer the FWU header to the ST67 in one shot (required by ST67) */
#define FWU_HEADER_SIZE             512

/** Multiple of data length that should be written in ST67, recommendation to ensure correct write into ST67 memory */
#define FWU_SECTOR_ALIGNMENT        256
```

Additionally, in case of FOTA feature is used to update the ST67 and STM32 binaries :
```
/** The folder containing the FOTA header, ST67 binary and the STM32 binary */
#define FOTA_HTTP_URI_TARGET        "/STM32U575ZI_NUCLEO"

/** Root folder of interest on the HTTP server */
#define FOTA_HTTP_COMMON_URI        "/download" FOTA_HTTP_URI_TARGET

/** Default URI for the ST67 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI               FOTA_HTTP_COMMON_URI "/st67w611m_mission_t02_v2.0.106.bin.ota"

/** Default URI for the STM32 binary, should be smaller in bytes size than the value defined by FOTA_URI_MAX_SIZE */
#define FOTA_HTTP_URI_STM32         FOTA_HTTP_COMMON_URI "/fota_ST67W6X_FOTA_LWIP.bin"

/** Default URI for the FOTA header */
#define FOTA_HTTP_URI_HEADER        FOTA_HTTP_COMMON_URI "/ST67W611_STM32U575ZI_NUCLEO.json"
```

### __Known limitations__
