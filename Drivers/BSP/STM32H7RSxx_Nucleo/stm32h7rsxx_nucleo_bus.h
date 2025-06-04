/**
  ******************************************************************************
  * @file    stm32h7rsxx_nucleo_bus.h
  * @author  MCD Application Team
  * @brief   This file is the header of stm32h7rsxx_nucleo_bus.c file
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
#ifndef STM32H7S78_NUCLEO_BUS_H
#define STM32H7S78_NUCLEO_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7rsxx_nucleo_conf.h"
#include "stm32h7rsxx_nucleo_errno.h"
#if defined(BSP_USE_CMSIS_OS)
#include "cmsis_os.h"
#endif /* BSP_USE_CMSIS_OS */
/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32H7RSXX_NUCLEO
  * @{
  */

/** @addtogroup STM32H7RSXX_NUCLEO_BUS
  * @{
  */
/** @defgroup STM32H7RSXX_NUCLEO_BUS_Exported_Types Exported Types
  * @{
  */
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* pMspI2cInitCb)(I2C_HandleTypeDef *);
  void (* pMspI2cDeInitCb)(I2C_HandleTypeDef *);
} BSP_I2C_Cb_t;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

/**
  * @}
  */
/** @defgroup STM32H7RSXX_NUCLEO_BUS_Exported_Constants Exported Constants
  * @{
  */
/* Definition for I2C3 clock resources */
#define BUS_I2C3                              I2C3
#define BUS_I2C3_CLK_ENABLE()                  __HAL_RCC_I2C3_CLK_ENABLE()
#define BUS_I2C3_CLK_DISABLE()                 __HAL_RCC_I2C3_CLK_DISABLE()
#define BUS_I2C3_SCL_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOA_CLK_ENABLE()
#define BUS_I2C3_SCL_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOA_CLK_DISABLE()
#define BUS_I2C3_SDA_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOA_CLK_ENABLE()
#define BUS_I2C3_SDA_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOA_CLK_DISABLE()

#define BUS_I2C3_FORCE_RESET()                 __HAL_RCC_I2C3_FORCE_RESET()
#define BUS_I2C3_RELEASE_RESET()               __HAL_RCC_I2C3_RELEASE_RESET()

/* Definition for I2C3 Pins */
#define BUS_I2C3_SCL_PIN                       GPIO_PIN_8
#define BUS_I2C3_SDA_PIN                       GPIO_PIN_9
#define BUS_I2C3_SCL_GPIO_PORT                 GPIOA
#define BUS_I2C3_SDA_GPIO_PORT                 GPIOA
#define BUS_I2C3_SCL_AF                        GPIO_AF4_I2C3
#define BUS_I2C3_SDA_AF                        GPIO_AF4_I2C3

#ifndef BUS_I2C3_FREQUENCY
#define BUS_I2C3_FREQUENCY                     100000U /* Frequency of I2Cn = 100 KHz*/
#endif /* BUS_I2C3_FREQUENCY */

/**
  * @}
  */

/** @addtogroup STM32H7RSXX_NUCLEO_BUS_Exported_Variables
  * @{
  */
extern I2C_HandleTypeDef hbus_i2c3;
/**
  * @}
  */

/** @addtogroup STM32H7RSXX_NUCLEO_BUS_Exported_Functions
  * @{
  */
int32_t BSP_I2C3_Init(void);
int32_t BSP_I2C3_DeInit(void);
int32_t BSP_I2C3_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C3_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C3_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C3_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C3_Recv(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C3_Send(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C3_IsReady(uint16_t DevAddr, uint32_t Trials);
int32_t BSP_GetTick(void);
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
int32_t BSP_I2C3_RegisterDefaultMspCallbacks(void);
int32_t BSP_I2C3_RegisterMspCallbacks(BSP_I2C_Cb_t *Callback);
#endif /* USE_HAL_I2C_REGISTER_CALLBACKS */
HAL_StatusTypeDef MX_I2C3_Init(I2C_HandleTypeDef *hi2c, uint32_t timing);
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

#endif /* STM32H7S78_NUCLEO_BUS_H */
