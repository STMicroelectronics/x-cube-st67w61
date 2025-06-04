/**
  ******************************************************************************
  * @file    stm32n6xx_nucleo.c
  * @author  MCD Application Team
  * @brief   This file provides a set of firmware functions to manage:
  *          LEDs
  *          push-buttons
  *          COM ports
  *          available on STM32N6XX-Nucleo Kit from STMicroelectronics.
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

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_nucleo.h"

#if defined(__ICCARM__)
#include <LowLevelIOInterface.h>
#endif /* __ICCARM__ */

/** @addtogroup BSP
  * @{
  */

/** @defgroup STM32N6XX_NUCLEO STM32N6XX_NUCLEO
  * @{
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON STM32N6XX_NUCLEO COMMON
  * @brief This file provides set of firmware functions to manage Leds and push-button
  *        available on STM32N6xx-Nucleo Kit from STMicroelectronics.
  * @{
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Private_Defines STM32N6XX_NUCLEO COMMON Private Defines
  * @{
  */
#if (USE_BSP_COM_FEATURE == 1)
#if (USE_COM_LOG == 1)
#define COM_POLL_TIMEOUT 1000
#endif /* USE_COM_LOG */
#endif /* USE_BSP_COM_FEATURE */
/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Private_TypesDefinitions STM32N6XX_NUCLEO COMMON Private Types Definitions
  * @{
  */
typedef void (* BSP_EXTI_LineCallback)(void);
/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Private_FunctionPrototypes STM32N6XX_NUCLEO COMMON Private Function Prototypes
  * @{
  */
static void BUTTON_USER_EXTI_Callback(void);

#if (USE_BSP_COM_FEATURE == 1)
static void UART_MspInit(UART_HandleTypeDef *huart);
static void UART_MspDeInit(UART_HandleTypeDef *huart);
#endif /* (USE_BSP_COM_FEATURE == 1) */
/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Exported_Variables STM32N6XX_NUCLEO COMMON Exported Variables
  * @{
  */
#if (USE_BSP_COM_FEATURE == 1)
UART_HandleTypeDef  hcom_uart[COMn];
USART_TypeDef*      COM_UART[COMn]   = {COM1_UART};
#endif /* USE_BSP_COM_FEATURE */
/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Private_Variables STM32N6XX_NUCLEO COMMON Private Variables
  * @{
  */
static uint16_t      LED_PIN[LEDn]  = {LED1_PIN, LED2_PIN, LED3_PIN};
static GPIO_TypeDef *LED_PORT[LEDn] = {LED1_GPIO_PORT, LED2_GPIO_PORT, LED3_GPIO_PORT};

static uint16_t           BUTTON_PIN[BUTTONn]  = {BUTTON_USER_PIN};
static GPIO_TypeDef      *BUTTON_PORT[BUTTONn] = {BUTTON_USER_GPIO_PORT};
static IRQn_Type          BUTTON_IRQn[BUTTONn] = {BUTTON_USER_EXTI_IRQn};
static EXTI_HandleTypeDef hpb_exti[BUTTONn];

#if (USE_BSP_COM_FEATURE == 1)
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
static uint32_t           IsComMspCbValid[COMn] = {0U};
#endif /* (USE_HAL_UART_REGISTER_CALLBACKS == 1) */

#if (USE_COM_LOG == 1)
static COM_TypeDef COM_ActiveLogPort = COM1;

/**
  * @brief  Redirect console output to COM
  */
#if defined(__ICCARM__)
/* New definition from EWARM V9, compatible with EWARM8 */
int iar_fputc(int ch);
#define PUTCHAR_PROTOTYPE int iar_fputc(int ch)

#elif defined (__CC_ARM) || defined(__ARMCC_VERSION)
/* ARM Compiler 5/6 */
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)

#elif defined(__GNUC__)
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#endif /* __ICCARM__ */

#endif /* USE_COM_LOG */

#endif /* (USE_BSP_COM_FEATURE == 1) */
/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Exported_Functions STM32N6XX_NUCLEO COMMON Exported Functions
  * @{
  */

/**
  * @brief  This method returns the STM32N6xx NUCLEO BSP Driver revision
  * @retval version : 0xXYZR (8bits for each decimal, R for RC)
  */
int32_t BSP_GetVersion(void)
{
  return ((int32_t)STM32N6xx_NUCLEO_BSP_VERSION);
}

/**
  * @brief  This method returns the board name
  * @retval pointer to the board name string
  */
const uint8_t* BSP_GetBoardName(void)
{
  return (const uint8_t*)STM32N6XX_NUCLEO_BSP_BOARD_NAME;
}

/**
  * @brief  This method returns the board ID
  * @retval pointer to the board ID string
  */
const uint8_t* BSP_GetBoardID(void)
{
  return (const uint8_t*)STM32N6XX_NUCLEO_BSP_BOARD_ID;
}

/**
  * @brief  This method sets the external SMPS Voltage
  * @note   This function is only usable from board revisions C01
  * @param Voltage configuration
  *          This parameter can be one of the following values:
  *            @arg  SMPS_VOLTAGE_NOMINAL
  *            @arg  SMPS_VOLTAGE_OVERDRIVE
  */
void BSP_SMPS_Init(SMPSVoltage_TypeDef Voltage){
  SMPS_GPIO_CLK_ENABLE();
  GPIO_InitTypeDef  gpio_init_structure = {0};
  /* configure the external SMPS control pin */
  gpio_init_structure.Pin = SMPS_GPIO_PIN;
  gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SMPS_GPIO_PORT, &gpio_init_structure);

  HAL_GPIO_WritePin(SMPS_GPIO_PORT, SMPS_GPIO_PIN, (GPIO_PinState) Voltage);
}

/** @defgroup STM32N6XX_NUCLEO_COMMON_LED_Functions STM32N6XX_NUCLEO COMMON LED Functions
  * @{
  */

/**
  * @brief  Configure LED GPIO.
  * @param  Led Specifies the Led to be configured.
  *   This parameter can be one of following parameters:
  *     @arg  LED1
  *     @arg  LED2
  *     @arg  LED3
  * @retval BSP error code
  */
int32_t BSP_LED_Init(Led_TypeDef Led)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef GPIO_Init;

  /* Enable the GPIO_LED Clock */
  if (Led == LED1)
  {
    LED1_GPIO_CLK_ENABLE();
  }
  else if (Led == LED2)
  {
    LED2_GPIO_CLK_ENABLE();
  }
  else /* Led = LED3 */
  {
    LED3_GPIO_CLK_ENABLE();
  }

  /* configure the GPIO_LED pin */
  GPIO_Init.Pin   = LED_PIN[Led];
  GPIO_Init.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_Init.Pull  = GPIO_NOPULL;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(LED_PORT[Led], &GPIO_Init);

  /* By default, turn off LED */
  BSP_LED_Off(Led);
  return ret;
}

/**
  * @brief  DeInitialize LEDs.
  * @param  Led LED to be de-init.
  *   This parameter can be one of the following values:
  *     @arg  LED1
  *     @arg  LED2
  *     @arg  LED3
  * @note BSP_LED_DeInit() does not disable the GPIO clock
  * @retval BSP error code
  */
int32_t BSP_LED_DeInit(Led_TypeDef Led)
{
  GPIO_InitTypeDef GPIO_Init;

  /* Turn off LED */
  BSP_LED_Off(Led);

  /* DeInit the GPIO_LED pin */
  GPIO_Init.Pin = LED_PIN[Led];

  HAL_GPIO_DeInit(LED_PORT[Led], GPIO_Init.Pin);
  return BSP_ERROR_NONE;
}

/**
  * @brief  Turn selected LED On.
  * @param  Led Specifies the Led to be set on.
  *   This parameter can be one of following parameters:
  *     @arg  LED1
  *     @arg  LED2
  *     @arg  LED3
  * @retval BSP error code
  */
int32_t BSP_LED_On(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(LED_PORT[Led], LED_PIN[Led], GPIO_PIN_RESET);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Turn selected LED Off.
  * @param  Led Specifies the Led to be set off.
  *   This parameter can be one of following parameters:
  *     @arg  LED1
  *     @arg  LED2
  *     @arg  LED3
  * @retval BSP error code
  */
int32_t BSP_LED_Off(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(LED_PORT[Led], LED_PIN[Led], GPIO_PIN_SET);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Toggle the selected LED.
  * @param  Led Specifies the Led to be toggled.
  *   This parameter can be one of following parameters:
  *     @arg  LED1
  *     @arg  LED2
  *     @arg  LED3
  * @retval BSP error code
  */
int32_t BSP_LED_Toggle(Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(LED_PORT[Led], LED_PIN[Led]);
  return BSP_ERROR_NONE;
}

/**
  * @brief  Get the state of selected LED.
  * @param  Led Specifies the Led.
  *   This parameter can be one of following parameters:
  *     @arg  LED1
  *     @arg  LED2
  *     @arg  LED3
  * @retval 0 means off, 1 means on.
  */
int32_t BSP_LED_GetState(Led_TypeDef Led)
{
  int32_t status;

  status = (HAL_GPIO_ReadPin(LED_PORT[Led], LED_PIN[Led]) == GPIO_PIN_RESET) ? 1 : 0;

  return status;
}

/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_BUTTON_Functions STM32N6XX_NUCLEO_COMMON BUTTON Functions
  * @{
  */

/**
  * @brief  Configure Button GPIO and EXTI Line.
  * @param  Button Specifies the Button to be configured.
  *   This parameter should be: BUTTON_USER
  * @param  ButtonMode Specifies Button mode.
  *   This parameter can be one of following parameters:
  *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
  *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line with interrupt
  *                            generation capability
  * @retval BSP error code
  */
int32_t BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode)
{
  int32_t               status = BSP_ERROR_NONE;
  GPIO_InitTypeDef      GPIO_Init;
  uint32_t              BSP_BUTTON_IT_PRIO[BUTTONn] = {BSP_BUTTON_USER_IT_PRIORITY};
  uint32_t              BUTTON_EXTI_LINE[BUTTONn]   = {BUTTON_USER_EXTI_LINE};
  BSP_EXTI_LineCallback ButtonCallback[BUTTONn]     = {BUTTON_USER_EXTI_Callback};

  /* Enable the BUTTON clock */
  BUTTON_USER_GPIO_CLK_ENABLE();

  GPIO_Init.Pin   = BUTTON_PIN[Button];
  GPIO_Init.Pull  = GPIO_PULLDOWN;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;

  if (ButtonMode == BUTTON_MODE_GPIO)
  {
    /* Configure Button pin as input */
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(BUTTON_PORT[Button], &GPIO_Init);
  }

  if (ButtonMode == BUTTON_MODE_EXTI)
  {
    /* Configure Button pin as input with External interrupt */
    GPIO_Init.Mode = GPIO_MODE_IT_FALLING;
    HAL_GPIO_Init(BUTTON_PORT[Button], &GPIO_Init);

    if (HAL_EXTI_GetHandle(&hpb_exti[Button], BUTTON_EXTI_LINE[Button]) == HAL_OK)
    {
      if (HAL_EXTI_RegisterCallback(&hpb_exti[Button], HAL_EXTI_COMMON_CB_ID, ButtonCallback[Button]) == HAL_OK)
      {
        /* Enable and set Button EXTI Interrupt to the lowest priority */
        HAL_NVIC_SetPriority(BUTTON_IRQn[Button], BSP_BUTTON_IT_PRIO[Button], 0x00);
        HAL_NVIC_EnableIRQ(BUTTON_IRQn[Button]);
      }
      else
      {
        status = BSP_ERROR_PERIPH_FAILURE;
      }
    }
    else
    {
      status = BSP_ERROR_PERIPH_FAILURE;
    }
  }

  return status;
}

/**
  * @brief  DeInitialize Push Button.
  * @param  Button Button to be configured
  *   This parameter should be: BUTTON_USER
  * @note BSP_PB_DeInit() does not disable the GPIO clock
  * @retval BSP error code
  */
int32_t BSP_PB_DeInit(Button_TypeDef Button)
{
  GPIO_InitTypeDef GPIO_Init;

  GPIO_Init.Pin = BUTTON_PIN[Button];
  HAL_NVIC_DisableIRQ(BUTTON_IRQn[Button]);
  HAL_GPIO_DeInit(BUTTON_PORT[Button], GPIO_Init.Pin);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Return the selected Button state.
  * @param  Button Specifies the Button to be checked.
  *   This parameter should be: BUTTON_USER
  * @retval The Button GPIO pin value
  */
int32_t BSP_PB_GetState(Button_TypeDef Button)
{
  return HAL_GPIO_ReadPin(BUTTON_PORT[Button], BUTTON_PIN[Button]);
}

/**
  * @brief  BSP Push Button callback
  * @param  Button Specifies the pin connected EXTI line
  * @retval None.
  */
__weak void BSP_PB_Callback(Button_TypeDef Button)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Button);

  /* This function should be implemented by the user application.
     It is called into this driver when an event on Button is triggered. */
}

/**
  * @brief  BSP PB interrupt handler.
  * @param  Button Button where interrupt occurs.
  *   This parameter should be:
  *     @arg BUTTON_USER
  * @retval None.
  */
void BSP_PB_IRQHandler(Button_TypeDef Button)
{
  HAL_EXTI_IRQHandler(&hpb_exti[Button]);
}

/**
  * @}
  */

#if (USE_BSP_COM_FEATURE == 1)
/** @defgroup STM32N6XX_NUCLEO_COMMON_COM_Functions STM32N6XX_NUCLEO COMMON COM Functions
  * @{
  */

/**
  * @brief  Configure COM port.
  * @param  COM COM port to be configured.
  *          This parameter can be COM1 or COM2
  * @param  COM_Init Pointer to a COM_InitTypeDef structure that contains the
  *                  configuration information for the specified COM peripheral.
  * @retval BSP error code
  */
int32_t BSP_COM_Init(COM_TypeDef COM, COM_InitTypeDef *COM_Init)
{
  int32_t status = BSP_ERROR_NONE;

  if (COM_Init == NULL)
  {
    status = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Initialize COM instance */
    hcom_uart[COM].Instance = COM_UART[COM];

#if (USE_HAL_UART_REGISTER_CALLBACKS == 0)
    UART_MspInit(&hcom_uart[COM]);
#else
    if (IsComMspCbValid[COM] == 0U)
    {
      /* Register the MSP callbacks */
      if (BSP_COM_RegisterDefaultMspCallbacks(COM) != BSP_ERROR_NONE)
      {
        status = BSP_ERROR_MSP_FAILURE;
      }
    }
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

    if (status == BSP_ERROR_NONE)
    {
      if (COM == COM1)
      {
        if (MX_USART1_Init(&hcom_uart[COM], COM_Init) != HAL_OK)
        {
          status = BSP_ERROR_PERIPH_FAILURE;
        }
      }
      else /* COM = COM2 */
      {
//        if (MX_USART3_Init(&hcom_uart[COM], COM_Init) != HAL_OK)
//        {
//          status = BSP_ERROR_PERIPH_FAILURE;
//        }
      }
    }
  }

  return status;
}

/**
  * @brief  DeInitialize COM port.
  * @param  COM COM port to be de_init.
  *          This parameter can be COM1 or COM2
  * @retval BSP error code
  */
int32_t BSP_COM_DeInit(COM_TypeDef COM)
{
  int32_t status = BSP_ERROR_NONE;

  /* COM de-init */
  hcom_uart[COM].Instance = COM_UART[COM];
  if (HAL_UART_DeInit(&hcom_uart[COM]) != HAL_OK)
  {
    status = BSP_ERROR_PERIPH_FAILURE;
  }
#if (USE_HAL_UART_REGISTER_CALLBACKS == 0)
  else
  {
    UART_MspDeInit(&hcom_uart[COM]);
  }
#endif /*  (USE_HAL_UART_REGISTER_CALLBACKS == 0) */

  return status;
}

#if (USE_COM_LOG == 1)
/**
  * @brief  Select the active COM port.
  * @param  COM COM port to be activated.
  *          This parameter can be COM1 or COM2
  * @retval BSP error code
  */
int32_t BSP_COM_SelectLogPort(COM_TypeDef COM)
{
  if (COM_ActiveLogPort != COM)
  {
    COM_ActiveLogPort = COM;
  }
  return BSP_ERROR_NONE;
}

#if defined(__ICCARM__)
size_t __write(int file, unsigned char const *ptr, size_t len)
{
  size_t idx;
  unsigned char const *pdata = ptr;

  for (idx = 0; idx < len; idx++)
  {
    iar_fputc((int)*pdata);
    pdata++;
  }
  return len;
}
#endif /* __ICCARM__ */

/**
  * @brief  Redirect console output to COM
  */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&hcom_uart[COM_ActiveLogPort], (uint8_t *) &ch, 1, COM_POLL_TIMEOUT);
  return ch;
}

#endif /* (USE_COM_LOG == 1) */


#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
/**
  * @brief  Register default COM msp callbacks.
  * @param  COM COM port.
  *          This parameter can be COM1 or COM2
  * @retval BSP status.
  */
int32_t BSP_COM_RegisterDefaultMspCallbacks(COM_TypeDef COM)
{
  int32_t status = BSP_ERROR_NONE;

  /* Register MspInit/MspDeInit callbacks */
  if (HAL_UART_RegisterCallback(&hcom_uart[COM], HAL_UART_MSPINIT_CB_ID, UART_MspInit) != HAL_OK)
  {
    status = BSP_ERROR_PERIPH_FAILURE;
  }
  else if (HAL_UART_RegisterCallback(&hcom_uart[COM], HAL_UART_MSPDEINIT_CB_ID, UART_MspDeInit) != HAL_OK)
  {
    status = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    IsComMspCbValid[COM] = 1U;
  }
  /* Return BSP status */
  return status;
}

/**
  * @brief  Register BSP COM msp callbacks.
  * @param  COM COM port.
  *          This parameter can be COM1 or COM2
  * @param  CallBacks Pointer to MspInit/MspDeInit callback functions.
  * @retval BSP status
  */
int32_t BSP_COM_RegisterMspCallbacks(COM_TypeDef COM, BSP_COM_Cb_t *CallBacks)
{
  int32_t status = BSP_ERROR_NONE;

  if (CallBacks == NULL)
  {
    status = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit callbacks */
    if (HAL_UART_RegisterCallback(&hcom_uart[COM], HAL_UART_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      status = BSP_ERROR_PERIPH_FAILURE;
    }
    else if (HAL_UART_RegisterCallback(&hcom_uart[COM], HAL_UART_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      status = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      IsComMspCbValid[COM] = 1U;
    }
  }
  /* Return BSP status */
  return status;
}
#endif /* (USE_HAL_UART_REGISTER_CALLBACKS == 1) */

/**
  * @}
  */
#endif /* (USE_BSP_COM_FEATURE == 1) */

/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_COMMON_Private_Functions STM32N6XX_NUCLEO COMMON Private Functions
  * @{
  */

/**
  * @brief  User EXTI line detection callbacks.
  * @retval None
  */
static void BUTTON_USER_EXTI_Callback(void)
{
  BSP_PB_Callback(BUTTON_USER);
}

#if (USE_BSP_COM_FEATURE == 1)

/**
  * @brief  Initialize UART
  * @param  huart USART handle
  * @param  COM_Init Pointer to a UART_HandleTypeDef structure that contains the
  *                  configuration information for the specified USART peripheral.
  * @retval HAL error code
  */
__weak HAL_StatusTypeDef MX_USART1_Init(UART_HandleTypeDef *huart, MX_UART_InitTypeDef *COM_Init)
{
  /* USART configuration */
  huart->Instance            = COM_UART[COM1];
  huart->Init.BaudRate       = COM_Init->BaudRate;
  huart->Init.Mode           = UART_MODE_TX_RX;
  huart->Init.Parity         = (uint32_t)COM_Init->Parity;
  huart->Init.WordLength     = (uint32_t)COM_Init->WordLength;
  huart->Init.StopBits       = (uint32_t)COM_Init->StopBits;
  huart->Init.HwFlowCtl      = (uint32_t)COM_Init->HwFlowCtl;
  huart->Init.OverSampling   = UART_OVERSAMPLING_8;
  huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  return HAL_UART_Init(huart);
}

/**
  * @brief  Initialize UART MSP.
  * @param  huart UART handle.
  * @retval None.
  */
static void UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef GPIO_Init;
  GPIO_TypeDef    *COM_TX_PORT[COMn] = {COM1_TX_GPIO_PORT};
  GPIO_TypeDef    *COM_RX_PORT[COMn] = {COM1_RX_GPIO_PORT};
  uint16_t         COM_TX_PIN[COMn]  = {COM1_TX_PIN};
  uint16_t         COM_RX_PIN[COMn]  = {COM1_RX_PIN};
  uint8_t          COM_TX_AF[COMn]   = {COM1_TX_AF};
  uint8_t          COM_RX_AF[COMn]   = {COM1_RX_AF};
  COM_TypeDef      COM;

  /* Get COM according instance */
  if (huart->Instance == COM1_UART)
  {
    COM = COM1;
    /* Enable COM and GPIO clocks */
    COM1_TX_GPIO_CLK_ENABLE();
    COM1_RX_GPIO_CLK_ENABLE();
    COM1_CLK_ENABLE();
  }
  else /* COM = ??? */
  {
      return;
  }

  /* Configure COM Tx as alternate function */
  GPIO_Init.Pin       = COM_TX_PIN[COM];
  GPIO_Init.Mode      = GPIO_MODE_AF_PP;
  GPIO_Init.Speed     = GPIO_SPEED_FREQ_HIGH;
  GPIO_Init.Pull      = GPIO_PULLUP;
  GPIO_Init.Alternate = COM_TX_AF[COM];
  HAL_GPIO_Init(COM_TX_PORT[COM], &GPIO_Init);

  /* Configure COM Rx as alternate function */
  GPIO_Init.Pin       = COM_RX_PIN[COM];
  GPIO_Init.Alternate = COM_RX_AF[COM];
  HAL_GPIO_Init(COM_RX_PORT[COM], &GPIO_Init);
}

/**
  * @brief  Deinitialize UART MSP.
  * @param  huart UART handle.
  * @retval None.
  */
static void UART_MspDeInit(UART_HandleTypeDef *huart)
{
  GPIO_TypeDef    *COM_TX_PORT[COMn] = {COM1_TX_GPIO_PORT};
  GPIO_TypeDef    *COM_RX_PORT[COMn] = {COM1_RX_GPIO_PORT};
  uint16_t         COM_TX_PIN[COMn]  = {COM1_TX_PIN};
  uint16_t         COM_RX_PIN[COMn]  = {COM1_RX_PIN};
  COM_TypeDef      COM;

  /* Get COM according instance */
  if (huart->Instance == COM1_UART)
  {
    COM = COM1;
  }
  else
  {
    return;
  }

  /* Disable GPIOs and COM clock */
  HAL_GPIO_DeInit(COM_TX_PORT[COM], COM_TX_PIN[COM]);
  HAL_GPIO_DeInit(COM_RX_PORT[COM], COM_RX_PIN[COM]);
  switch (COM)
  {
    case COM1:
      COM1_CLK_DISABLE();
      break;
    default:
      break;
  }
}

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
