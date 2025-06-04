/**
  ******************************************************************************
  * @file    stm32n6xx_nucleo_bus.h
  * @author  MCD Application Team
  * @brief   This file is the header of stm32n6xx_nucleo_bus.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32N6XX_NUCLEO_BUS_H
#define STM32N6XX_NUCLEO_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_nucleo_conf.h"
#include "stm32n6xx_nucleo_errno.h"
#if defined(BSP_USE_CMSIS_OS)
#include "cmsis_os.h"
#endif /* BSP_USE_CMSIS_OS */
/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6XX_NUCLEO
  * @{
  */

/** @addtogroup STM32N6XX_NUCLEO_BUS
  * @{
  */
/** @defgroup STM32N6XX_NUCLEO_BUS_Exported_Types Exported Types
  * @{
  */
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* MspInitCallback)(I2C_HandleTypeDef *);
  void (* MspDeInitCallback)(I2C_HandleTypeDef *);
}BSP_I2C_Cb_t;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1) */

/**
  * @}
  */
/** @defgroup STM32N6XX_NUCLEO_BUS_Exported_Constants Exported Constants
  * @{
  */
/* Definition for I2C2 clock resources */
#define BUS_I2C2                               I2C2
#define BUS_I2C2_CLK_ENABLE()                  __HAL_RCC_I2C2_CLK_ENABLE()
#define BUS_I2C2_CLK_DISABLE()                 __HAL_RCC_I2C2_CLK_DISABLE()
#define BUS_I2C2_SCL_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define BUS_I2C2_SCL_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()
#define BUS_I2C2_SDA_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOB_CLK_ENABLE()
#define BUS_I2C2_SDA_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOB_CLK_DISABLE()

#define BUS_I2C2_FORCE_RESET()                 __HAL_RCC_I2C2_FORCE_RESET()
#define BUS_I2C2_RELEASE_RESET()               __HAL_RCC_I2C2_RELEASE_RESET()

/* Definition for I2C2 Pins */
#define BUS_I2C2_SCL_PIN                       GPIO_PIN_10
#define BUS_I2C2_SDA_PIN                       GPIO_PIN_11
#define BUS_I2C2_SCL_GPIO_PORT                 GPIOB
#define BUS_I2C2_SDA_GPIO_PORT                 GPIOB
#define BUS_I2C2_SCL_AF                        GPIO_AF4_I2C2
#define BUS_I2C2_SDA_AF                        GPIO_AF4_I2C2

#ifndef BUS_I2C2_FREQUENCY
#define BUS_I2C2_FREQUENCY  100000U /* Frequency of I2Cn = 100 KHz*/
#endif /* BUS_I2C2_FREQUENCY */

/**
  * @}
  */

/** @addtogroup STM32N6XX_NUCLEO_BUS_Exported_Variables
  * @{
  */
extern I2C_HandleTypeDef hbus_i2c2;
/**
  * @}
  */

/** @addtogroup STM32N6XX_NUCLEO_BUS_Exported_Functions
  * @{
  */
int32_t BSP_I2C2_Init(void);
int32_t BSP_I2C2_DeInit(void);
int32_t BSP_I2C2_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_WriteReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_ReadReg16(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_Recv(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_Send(uint16_t DevAddr, uint16_t Reg, uint16_t MemAddSize, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C2_IsReady(uint16_t DevAddr, uint32_t Trials);
int32_t BSP_GetTick(void);
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1)
int32_t BSP_I2C2_RegisterDefaultMspCallbacks (void);
int32_t BSP_I2C2_RegisterMspCallbacks (BSP_I2C_Cb_t *Callback);
#endif /* USE_HAL_I2C_REGISTER_CALLBACKS */
HAL_StatusTypeDef MX_I2C2_Init(I2C_HandleTypeDef *phi2c, uint32_t timing);
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

#endif /* STM32N6XX_NUCLEO_BUS_H */
