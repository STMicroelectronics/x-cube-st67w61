/**
  ******************************************************************************
  * @file    stm32n6xx_nucleo_xspi.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32n6xx_nucleo_xspi.c driver.
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
#ifndef STM32N6XX_NUCLEO_XSPI_H
#define STM32N6XX_NUCLEO_XSPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_nucleo_conf.h"
#include "stm32n6xx_nucleo_errno.h"
#include "../Components/mx25um51245g/mx25um51245g.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6XX_NUCLEO
  * @{
  */

/** @addtogroup STM32N6XX_NUCLEO_XSPI
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup STM32N6XX_NUCLEO_XSPI_Exported_Types XSPI Exported Types
  * @{
  */
typedef enum
{
  XSPI_ACCESS_NONE = 0,          /*!<  Instance not initialized,              */
  XSPI_ACCESS_INDIRECT,          /*!<  Instance use indirect mode access      */
  XSPI_ACCESS_MMP                /*!<  Instance use Memory Mapped Mode read   */
} XSPI_Access_t;

#if (USE_HAL_XSPI_REGISTER_CALLBACKS == 1)
typedef struct
{
  pXSPI_CallbackTypeDef  pMspInitCb;
  pXSPI_CallbackTypeDef  pMspDeInitCb;
} BSP_XSPI_Cb_t;
#endif /* (USE_HAL_XSPI_REGISTER_CALLBACKS == 1) */

typedef struct
{
  uint32_t MemorySize;
  uint32_t ClockPrescaler;
  uint32_t SampleShifting;
  uint32_t TransferRate;
} MX_XSPI_InitTypeDef;
/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_XSPI_NOR_Exported_Types XSPI_NOR Exported Types
  * @{
  */
#define BSP_XSPI_NOR_Info_t                MX25UM51245G_Info_t
#define BSP_XSPI_NOR_Interface_t           MX25UM51245G_Interface_t
#define BSP_XSPI_NOR_Transfer_t            MX25UM51245G_Transfer_t
#define BSP_XSPI_NOR_Erase_t               MX25UM51245G_Erase_t

typedef struct
{
  XSPI_Access_t              IsInitialized;  /*!<  Instance access Flash method     */
  BSP_XSPI_NOR_Interface_t   InterfaceMode;  /*!<  Flash Interface mode of Instance */
  BSP_XSPI_NOR_Transfer_t    TransferRate;   /*!<  Flash Transfer mode of Instance  */
} XSPI_NOR_Ctx_t;

typedef struct
{
  BSP_XSPI_NOR_Interface_t   InterfaceMode;      /*!<  Current Flash Interface mode */
  BSP_XSPI_NOR_Transfer_t    TransferRate;       /*!<  Current Flash Transfer rate  */
} BSP_XSPI_NOR_Init_t;
/**
  * @}
  */



/* Exported constants --------------------------------------------------------*/
/** @defgroup STM32N6XX_NUCLEO_XSPI_Exported_Constants XSPI Exported Constants
  * @{
  */


/* Definition for XSPI clock resources */
#define XSPI_CLK_ENABLE()                 __HAL_RCC_XSPI2_CLK_ENABLE()
#define XSPI_CLK_DISABLE()                __HAL_RCC_XSPI2_CLK_DISABLE()

#define XSPI_CLK_GPIO_CLK_ENABLE()        __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_DQS_GPIO_CLK_ENABLE()        __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_CS_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D0_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D1_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D2_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D3_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D4_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D5_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D6_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()
#define XSPI_D7_GPIO_CLK_ENABLE()         __HAL_RCC_GPION_CLK_ENABLE()

#define XSPI_FORCE_RESET()                __HAL_RCC_XSPI2_FORCE_RESET()
#define XSPI_RELEASE_RESET()              __HAL_RCC_XSPI2_RELEASE_RESET()

/* Definition for XSPI Pins */
/* XSPI_CLK */
#define XSPI_CLK_PIN                      GPIO_PIN_6
#define XSPI_CLK_GPIO_PORT                GPION
#define XSPI_CLK_PIN_AF                   GPIO_AF9_XSPIM_P2
/* XSPI_DQS */
#define XSPI_DQS_PIN                      GPIO_PIN_0
#define XSPI_DQS_GPIO_PORT                GPION
#define XSPI_DQS_PIN_AF                   GPIO_AF9_XSPIM_P2
/* XSPI_CS */
#define XSPI_CS_PIN                       GPIO_PIN_1
#define XSPI_CS_GPIO_PORT                 GPION
#define XSPI_CS_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D0 */
#define XSPI_D0_PIN                       GPIO_PIN_2
#define XSPI_D0_GPIO_PORT                 GPION
#define XSPI_D0_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D1 */
#define XSPI_D1_PIN                       GPIO_PIN_3
#define XSPI_D1_GPIO_PORT                 GPION
#define XSPI_D1_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D2 */
#define XSPI_D2_PIN                       GPIO_PIN_4
#define XSPI_D2_GPIO_PORT                 GPION
#define XSPI_D2_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D3 */
#define XSPI_D3_PIN                       GPIO_PIN_5
#define XSPI_D3_GPIO_PORT                 GPION
#define XSPI_D3_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D4 */
#define XSPI_D4_PIN                       GPIO_PIN_8
#define XSPI_D4_GPIO_PORT                 GPION
#define XSPI_D4_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D5 */
#define XSPI_D5_PIN                       GPIO_PIN_9
#define XSPI_D5_GPIO_PORT                 GPION
#define XSPI_D5_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D6 */
#define XSPI_D6_PIN                       GPIO_PIN_10
#define XSPI_D6_GPIO_PORT                 GPION
#define XSPI_D6_PIN_AF                    GPIO_AF9_XSPIM_P2
/* XSPI_D7 */
#define XSPI_D7_PIN                       GPIO_PIN_11
#define XSPI_D7_GPIO_PORT                 GPION
#define XSPI_D7_PIN_AF                    GPIO_AF9_XSPIM_P2

/**
  * @}
  */

/** @defgroup STM32N6XX_NUCLEO_XSPI_NOR_Exported_Constants XSPI_NOR Exported Constants
  * @{
  */
#define XSPI_NOR_INSTANCES_NUMBER         1U

/* Definition for XSPI modes */
#define BSP_XSPI_NOR_SPI_MODE       (BSP_XSPI_NOR_Interface_t)MX25UM51245G_SPI_MODE /* 1 Cmd Line, 1 Address Line
                                                                                       and 1 Data Line    */
#define BSP_XSPI_NOR_OPI_MODE       (BSP_XSPI_NOR_Interface_t)MX25UM51245G_OPI_MODE /* 8 Cmd Lines, 8 Address Lines
                                                                                         and 8 Data Lines */

/* Definition for XSPI transfer rates */
#define BSP_XSPI_NOR_STR_TRANSFER   (BSP_XSPI_NOR_Transfer_t)MX25UM51245G_STR_TRANSFER   /* Single Transfer Rate */
#define BSP_XSPI_NOR_DTR_TRANSFER   (BSP_XSPI_NOR_Transfer_t)MX25UM51245G_DTR_TRANSFER   /* Double Transfer Rate */

/* XSPI erase types */
#define BSP_XSPI_NOR_ERASE_4K       MX25UM51245G_ERASE_4K
#define BSP_XSPI_NOR_ERASE_64K      MX25UM51245G_ERASE_64K
#define BSP_XSPI_NOR_ERASE_CHIP     MX25UM51245G_ERASE_BULK

/* XSPI block sizes */
#define BSP_XSPI_NOR_BLOCK_4K       MX25UM51245G_SUBSECTOR_4K
#define BSP_XSPI_NOR_BLOCK_64K      MX25UM51245G_SECTOR_64K
/**
  * @}
  */


/* Exported variables --------------------------------------------------------*/
/** @defgroup STM32N6XX_NUCLEO_XSPI_NOR_Exported_Variables XSPI_NOR Exported Variables
  * @{
  */
extern XSPI_HandleTypeDef hxspi_nor[XSPI_NOR_INSTANCES_NUMBER];
extern XSPI_NOR_Ctx_t Xspi_Nor_Ctx[XSPI_NOR_INSTANCES_NUMBER];
/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @defgroup STM32N6XX_NUCLEO_XSPI_NOR_Exported_Functions XSPI_NOR Exported Functions
  * @{
  */
int32_t BSP_XSPI_NOR_Init(uint32_t Instance, BSP_XSPI_NOR_Init_t *Init);
int32_t BSP_XSPI_NOR_DeInit(uint32_t Instance);
#if (USE_HAL_XSPI_REGISTER_CALLBACKS == 1)
int32_t BSP_XSPI_NOR_RegisterMspCallbacks(uint32_t Instance, BSP_XSPI_Cb_t *CallBacks);
int32_t BSP_XSPI_NOR_RegisterDefaultMspCallbacks(uint32_t Instance);
#endif /* (USE_HAL_XSPI_REGISTER_CALLBACKS == 1) */
int32_t BSP_XSPI_NOR_Read(uint32_t Instance, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
int32_t BSP_XSPI_NOR_Write(uint32_t Instance, uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
int32_t BSP_XSPI_NOR_Erase_Block(uint32_t Instance, uint32_t BlockAddress, BSP_XSPI_NOR_Erase_t BlockSize);
int32_t BSP_XSPI_NOR_Erase_Chip(uint32_t Instance);
int32_t BSP_XSPI_NOR_GetStatus(uint32_t Instance);
int32_t BSP_XSPI_NOR_GetInfo(uint32_t Instance, BSP_XSPI_NOR_Info_t *pInfo);
int32_t BSP_XSPI_NOR_EnableMemoryMappedMode(uint32_t Instance);
int32_t BSP_XSPI_NOR_DisableMemoryMappedMode(uint32_t Instance);
int32_t BSP_XSPI_NOR_ReadID(uint32_t Instance, uint8_t *Id);
int32_t BSP_XSPI_NOR_ConfigFlash(uint32_t Instance, BSP_XSPI_NOR_Interface_t Mode, BSP_XSPI_NOR_Transfer_t Rate);
int32_t BSP_XSPI_NOR_SuspendErase(uint32_t Instance);
int32_t BSP_XSPI_NOR_ResumeErase(uint32_t Instance);
int32_t BSP_XSPI_NOR_EnterDeepPowerDown(uint32_t Instance);
int32_t BSP_XSPI_NOR_LeaveDeepPowerDown(uint32_t Instance);

/* These functions can be modified in case the current settings
   need to be changed for specific application needs */
HAL_StatusTypeDef MX_XSPI_NOR_Init(XSPI_HandleTypeDef *hXSPI, MX_XSPI_InitTypeDef *Init);

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

#endif /* STM32N6XX_NUCLEO_XSPI_H */
