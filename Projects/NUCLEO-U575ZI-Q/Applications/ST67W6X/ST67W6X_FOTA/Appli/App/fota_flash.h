/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fota_flash.h
  * @author  GPM Application Team
  * @brief   Header file for Flash operations used in FOTA.
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef FOTA_FLASH_H
#define FOTA_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32u5xx.h"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_flash.h"
#include "stm32u5xx_hal_flash_ex.h"

/* Exported constants --------------------------------------------------------*/
/** @defgroup FOTA_Flash_Error_Codes FOTA Flash Error Codes
  * @{
  */

#define FOTA_FLASH_SUCCESS              0   /*!< Operation successful */
#define FOTA_FLASH_ERR                  -1  /*!< General error */
#define FOTA_FLASH_ERR_INVALID_ARGS     -2  /*!< Invalid arguments */
#define FOTA_FLASH_ERR_BUFFER_TOO_SMALL -3  /*!< Buffer too small */

/** @} */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup FOTA_Flash_Macros FOTA Flash Macros
  * @{
  */

#ifndef FLASH_BANK_SIZE
/** Required components/defines for flash operations using HAL */
#define FLASH_BANK_SIZE 0xFFFFFFFFU

/** Flash not found error code */
#define FLASH_NOT_FOUND 1
#endif /* FLASH_BANK_SIZE */

#ifndef FLASH_NOT_FOUND
/** Quadword size in bytes */
#define QUADWORD_SIZE_BYTE            16U
/** Burst size in bytes */
#define BURST_SIZE_BYTE               (QUADWORD_SIZE_BYTE * 8U)
#else
/** Quadword size in bytes */
#define QUADWORD_SIZE_BYTE            1U
/** Burst size in bytes */
#define BURST_SIZE_BYTE               (QUADWORD_SIZE_BYTE * 1U)
#endif

/** @} */

/* Exported functions --------------------------------------------------------*/
/** @defgroup FOTA_Flash_Functions FOTA Flash Functions
  * @{
  */

/**
  * @brief  Write data to Flash memory.
  * @param  address: Start address in Flash memory.
  * @param  data: Pointer to the data to write.
  * @param  size: Size of the data in bytes.
  * @retval 0 if successful, -1 otherwise
  */
int32_t fota_flash_WriteToFlash(uint32_t address, const uint8_t *data, uint32_t size);

/**
  * @brief  Write data to Flash memory in burst mode.
  * @param  address: Start address in Flash memory.
  * @param  data: Pointer to the data to write.
  * @param  size: Size of the data in bytes.
  * @retval 0 if successful, -1 otherwise
  */
int32_t fota_flash_WriteToFlashBurst(uint32_t address, const uint8_t *data, uint32_t size);

/**
  * @brief  Erase Flash memory pages.
  * @param  address: Start address in Flash memory.
  * @param  size: Size of the memory to erase in bytes.
  * @retval 0 if successful, -1 otherwise
  */
int32_t fota_flash_EraseFlash(uint32_t address, uint32_t size);

/**
  * @brief  Perform a mass erase of the inactive Flash bank.
  * @retval 0 if successful, -1 otherwise
  */
int32_t fota_flash_MassEraseFlashBank(void);

/**
  * @brief  Check if Flash banks are swapped.
  * @param  swap_status: true if banks are swapped, false otherwise.
  * @retval 0 if successful, -1 otherwise
  */
int32_t fota_flash_IsFlashBankSwapped(bool *swap_status);

/**
  * @brief  Swap Flash banks.
  * @retval 0 if successful, -1 otherwise
  */
int32_t fota_flash_SwapFlashBanks(void);

/**
  * @brief  Get the start address of the inactive Flash bank.
  * @return Start address of the inactive bank.
  */
uint32_t fota_flash_GetStartAddrFlashBank(void);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FOTA_FLASH_H */
