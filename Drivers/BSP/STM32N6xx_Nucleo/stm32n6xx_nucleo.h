/**
  ******************************************************************************
  * @file    stm32n6xx_nucleo.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for STM32N6XX-Nucleo Kit:
  *          LEDs
  *          push-buttons
  *          COM ports
  *          hardware resources.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32N6XX_NUCLEO_H
#define STM32N6XX_NUCLEO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_nucleo_conf.h"
#include "stm32n6xx_nucleo_errno.h"

#if (USE_BSP_COM_FEATURE == 1)
#include <stdio.h>
#endif /* USE_BSP_COM_FEATURE */

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6XX_NUCLEO
  * @{
  */

/** @addtogroup STM32N6XX_NUCLEO_COMMON
  * @{
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Exported_Types STM32N6XX_NUCLEO COMMON Exported Types
  * @{
  */
typedef enum
{
  LED1 = 0U,
  LED_BLUE = LED1,
  LED2 = 1U,
  LED_RED = LED2,
  LED3 = 2U,
  LED_GREEN = LED3,
  LEDn
} Led_TypeDef;

typedef enum
{
  BUTTON_USER = 0U,
  BUTTONn
} Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

typedef enum
{
  SMPS_VOLTAGE_NOMINAL   = GPIO_PIN_RESET,
  SMPS_VOLTAGE_OVERDRIVE = GPIO_PIN_SET,
} SMPSVoltage_TypeDef;

#if (USE_BSP_COM_FEATURE == 1)
typedef enum
{
  COM1 = 0U,
  COM2 = 1U,
  COMn
} COM_TypeDef;

typedef enum
{
  COM_WORDLENGTH_7B = UART_WORDLENGTH_7B,
  COM_WORDLENGTH_8B = UART_WORDLENGTH_8B,
  COM_WORDLENGTH_9B = UART_WORDLENGTH_9B
} COM_WordLengthTypeDef;

typedef enum
{
  COM_STOPBITS_1 = UART_STOPBITS_1,
  COM_STOPBITS_2 = UART_STOPBITS_2
} COM_StopBitsTypeDef;

typedef enum
{
  COM_PARITY_NONE = UART_PARITY_NONE,
  COM_PARITY_EVEN = UART_PARITY_EVEN,
  COM_PARITY_ODD  = UART_PARITY_ODD
} COM_ParityTypeDef;

typedef enum
{
  COM_HWCONTROL_NONE    = UART_HWCONTROL_NONE,
  COM_HWCONTROL_RTS     = UART_HWCONTROL_RTS,
  COM_HWCONTROL_CTS     = UART_HWCONTROL_CTS,
  COM_HWCONTROL_RTS_CTS = UART_HWCONTROL_RTS_CTS
} COM_HwFlowCtlTypeDef;

typedef struct
{
  uint32_t              BaudRate;
  COM_WordLengthTypeDef WordLength;
  COM_StopBitsTypeDef   StopBits;
  COM_ParityTypeDef     Parity;
  COM_HwFlowCtlTypeDef  HwFlowCtl;
} COM_InitTypeDef;

#define MX_UART_InitTypeDef COM_InitTypeDef

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
typedef struct
{
  pUART_CallbackTypeDef  pMspInitCb;
  pUART_CallbackTypeDef  pMspDeInitCb;
} BSP_COM_Cb_t;
#endif /*  (USE_HAL_UART_REGISTER_CALLBACKS == 1) */
#endif /* (USE_BSP_COM_FEATURE == 1) */

/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Exported_Constants STM32N6XX_NUCLEO COMMON Exported Constants
  * @{
  */

/**
  * @brief  Define for STM32N6 NUCLEO board
  */

#if !defined (USE_STM32N6xx_NUCLEO)
#define USE_STM32N6xx_NUCLEO
#endif /* USE_STM32N6xx_NUCLEO */

/**
  * @brief STM32N6xx NUCLEO BSP Driver version number
  */
#define STM32N6xx_NUCLEO_BSP_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define STM32N6xx_NUCLEO_BSP_VERSION_SUB1   (0x00U) /*!< [23:16] sub1 version */
#define STM32N6xx_NUCLEO_BSP_VERSION_SUB2   (0x00U) /*!< [15:8]  sub2 version */
#define STM32N6xx_NUCLEO_BSP_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define STM32N6xx_NUCLEO_BSP_VERSION       ((STM32N6xx_NUCLEO_BSP_VERSION_MAIN << 24U)\
                                               |(STM32N6xx_NUCLEO_BSP_VERSION_SUB1 << 16U)\
                                               |(STM32N6xx_NUCLEO_BSP_VERSION_SUB2 << 8U )\
                                               |(STM32N6xx_NUCLEO_BSP_VERSION_RC))

#define STM32N6XX_NUCLEO_BSP_BOARD_NAME  "STM32N6XX-NUCLEO"
#define STM32N6XX_NUCLEO_BSP_BOARD_ID    "MB1940"

/** @defgroup STM32N6XX_NUCLEO_COMMON_LED STM32N6XX_NUCLEO COMMON LED
  * @{
  */
#define LED1_GPIO_PORT                          GPIOG
#define LED1_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOG_CLK_ENABLE()
#define LED1_PIN                                GPIO_PIN_8

#define LED2_GPIO_PORT                          GPIOG
#define LED2_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOG_CLK_ENABLE()
#define LED2_PIN                                GPIO_PIN_10

#define LED3_GPIO_PORT                          GPIOG
#define LED3_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOG_CLK_ENABLE()
#define LED3_PIN                                GPIO_PIN_0
/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_BUTTON STM32N6XX_NUCLEO COMMON BUTTON
  * @{
  */

/* Button state */
#define BUTTON_RELEASED                         0U
#define BUTTON_PRESSED                          1U

#define BUTTON_USER_GPIO_PORT                   GPIOC
#define BUTTON_USER_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOC_CLK_ENABLE()
#define BUTTON_USER_PIN                         GPIO_PIN_13
#define BUTTON_USER_EXTI_LINE                   EXTI_LINE_13
#define BUTTON_USER_EXTI_IRQn                   EXTI13_IRQn
/**
  * @}
  */

#define SMPS_GPIO_PORT                    GPIOB
#define SMPS_GPIO_PIN                     GPIO_PIN_12
#define SMPS_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOB_CLK_ENABLE()

#if (USE_BSP_COM_FEATURE == 1)
/** @defgroup STM32N6XX_NUCLEO_COMMON_COM STM32N6XX_NUCLEO COMMON COM
  * @{
  */

/**
 * @brief Definition for COM port1, connected to USART1
 */
#define COM1_UART                               USART1
#define COM1_CLK_ENABLE()                       __HAL_RCC_USART1_CLK_ENABLE()
#define COM1_CLK_DISABLE()                      __HAL_RCC_USART1_CLK_DISABLE()

#define COM1_TX_GPIO_PORT                       GPIOE
#define COM1_TX_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOE_CLK_ENABLE()
#define COM1_TX_PIN                             GPIO_PIN_5
#define COM1_TX_AF                              GPIO_AF7_USART1

#define COM1_RX_GPIO_PORT                       GPIOD
#define COM1_RX_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOD_CLK_ENABLE()
#define COM1_RX_PIN                             GPIO_PIN_6
#define COM1_RX_AF                              GPIO_AF7_USART1

/**
 * @brief Definition for COM port2, connected to USART3
 */
#define COM2_UART                               USART3
#define COM2_CLK_ENABLE()                       __HAL_RCC_USART3_CLK_ENABLE()
#define COM2_CLK_DISABLE()                      __HAL_RCC_USART3_CLK_DISABLE()

#define COM2_TX_PIN                             GPIO_PIN_5
#define COM2_TX_GPIO_PORT                       GPIOD
#define COM2_TX_GPIO_CLK_ENABLE()               _HAL_RCC_GPIOD_CLK_ENABLE()
#define COM2_TX_GPIO_CLK_DISABLE()              _HAL_RCC_GPIOD_CLK_DISABLE()
#define COM2_TX_AF                              GPIO_AF7_USART3

#define COM2_RX_PIN                             GPIO_PIN_6
#define COM2_RX_GPIO_PORT                       GPIOF
#define COM2_RX_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOF_CLK_ENABLE()
#define COM2_RX_GPIO_CLK_DISABLE()              __HAL_RCC_GPIOF_CLK_DISABLE()
#define COM2_RX_AF                              GPIO_AF7_USART3
/**
  * @}
  */
#endif /* (USE_BSP_COM_FEATURE == 1) */

/**
  * @}
  */

/** @addtogroup STM32N6XX_NUCLEO_COMMON_Exported_Variables
  * @{
  */
#if (USE_BSP_COM_FEATURE == 1)
extern UART_HandleTypeDef hcom_uart[COMn];
#endif /* (USE_BSP_COM_FEATURE == 1) */
/**
  * @}
  */

/** @addtogroup STM32N6XX_NUCLEO_COMMON_Exported_Functions
  * @{
  */
int32_t        BSP_GetVersion(void);
const uint8_t* BSP_GetBoardName(void);
const uint8_t* BSP_GetBoardID(void);
void           BSP_SMPS_Init(SMPSVoltage_TypeDef Voltage);

/** @addtogroup STM32N6XX_NUCLEO_COMMON_LED_Functions
  * @{
  */
int32_t BSP_LED_Init(Led_TypeDef Led);
int32_t BSP_LED_DeInit(Led_TypeDef Led);
int32_t BSP_LED_On(Led_TypeDef Led);
int32_t BSP_LED_Off(Led_TypeDef Led);
int32_t BSP_LED_Toggle(Led_TypeDef Led);
int32_t BSP_LED_GetState(Led_TypeDef Led);
/**
  * @}
  */

/** @addtogroup STM32N6XX_NUCLEO_COMMON_BUTTON_Functions
  * @{
  */
int32_t BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);
int32_t BSP_PB_DeInit(Button_TypeDef Button);
int32_t BSP_PB_GetState(Button_TypeDef Button);
void    BSP_PB_Callback(Button_TypeDef Button);
void    BSP_PB_IRQHandler(Button_TypeDef Button);

/**
  * @}
  */

#if (USE_BSP_COM_FEATURE == 1)
/** @addtogroup STM32N6XX_NUCLEO_COMMON_COM_Functions
  * @{
  */
int32_t BSP_COM_Init(COM_TypeDef COM, COM_InitTypeDef *COM_Init);
int32_t BSP_COM_DeInit(COM_TypeDef COM);
#if (USE_COM_LOG == 1)
int32_t BSP_COM_SelectLogPort(COM_TypeDef COM);
#endif /*  (USE_COM_LOG == 1) */
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
int32_t BSP_COM_RegisterDefaultMspCallbacks(COM_TypeDef COM);
int32_t BSP_COM_RegisterMspCallbacks(COM_TypeDef COM, BSP_COM_Cb_t *CallBacks);
#endif /*  (USE_HAL_UART_REGISTER_CALLBACKS == 1) */
HAL_StatusTypeDef MX_USART1_Init(UART_HandleTypeDef* huart, MX_UART_InitTypeDef *MXInit);
HAL_StatusTypeDef MX_USART3_Init(UART_HandleTypeDef* huart, MX_UART_InitTypeDef *MXInit);
/**
  * @}
  */
#endif /* (USE_BSP_COM_FEATURE == 1) */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32N6XX_NUCLEO_H */

