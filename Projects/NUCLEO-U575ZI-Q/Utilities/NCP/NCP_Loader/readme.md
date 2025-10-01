# __NCP_Loader Application Description__

This application aims to flash the ST67W611M binaries and to execute the Manufacturing test over STM32 UART link.

For more details [NCP Binaries README.md](../../../../ST67W6X_Utilities/Binaries/README.md).

## __Keywords__

ST67W611M, UART

## __Links and references__

For further information, please visit the dedicated Wiki page [How to flash the ST67W611M](https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_MCU_Hardware_Setup#How_to_flash_the_ST67W611M1).

## __Directory contents__


|File  |                                                                          |Description|
|---   |:-:                                                                       |---        |
|NCP_Loader/Core/Inc/main.h|                                                      |Header for main.c file.<br>This file contains the common defines of the application.|
|NCP_Loader/Core/Inc/stm32u5xx_hal_conf.h|                                        |HAL configuration file.|
|NCP_Loader/Core/Inc/stm32u5xx_it.h|                                              |This file contains the headers of the interrupt handlers.|
|      |                                                                          |           |
|NCP_Loader/Core/Src/main.c|                                                      |Main program body|
|NCP_Loader/Core/Src/stm32u5xx_hal_msp.c|                                         |This file provides code for the MSP Initialization<br>and de-Initialization codes.|
|NCP_Loader/Core/Src/stm32u5xx_it.c|                                              |Interrupt Service Routines.|
|NCP_Loader/Core/Src/system_stm32u5xx.c|                                          |CMSIS Cortex-M33 Device Peripheral Access Layer System Source File|
|NCP_Loader/STM32CubeIDE/Application/User/Core/syscalls.c|                        |STM32CubeIDE Minimal System calls file|
|NCP_Loader/STM32CubeIDE/Application/User/Core/sysmem.c|                          |STM32CubeIDE System Memory calls file|


## __Hardware and Software environment__

  - This example runs on the NUCLEO-U575ZI-Q board combined with the X-NUCLEO-67W61M1 board
  - X-NUCLEO-67W61M1 board is plugged to the NUCLEO-U575ZI-Q board via the Arduino connectors:
    - The 5V, 3V3, GND through the CN6
    - The SPI (CLK, MOSI, MISO), SPI_CS and USER_BUTTON signals through the CN5
    - The BOOT, CHIP_EN, SPI_RDY and UART TX/RX signals through the CN9

## __How to use it?__

## __Known limitations__
