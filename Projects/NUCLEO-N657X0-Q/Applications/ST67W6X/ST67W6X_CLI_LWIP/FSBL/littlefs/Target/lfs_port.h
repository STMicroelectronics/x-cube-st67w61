/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lfs_port.h
  * @author  ST67 Application Team
  * @brief   lfs flash port definition
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
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
#ifndef LFS_PORT_H
#define LFS_PORT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "lfs.h"

#include <FreeRTOS.h>
#include <semphr.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/** LittleFS context structure */
struct lfs_context
{
  lfs_t lfs;                        /*!< LittleFS object */
  uint32_t flash_addr;              /*!< Start address of the flash partition */
  SemaphoreHandle_t fs_giant_lock;  /*!< Semaphore for file system access */
  char *partition_name;             /*!< Name of the flash partition */
};

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize the LittleFS flash context
  * @param  lfs_xip_ctx Pointer to the LittleFS context structure
  * @param  cfg Pointer to the LittleFS configuration structure
  * @return Pointer to the LittleFS object
  */
lfs_t *lfs_flash_init(struct lfs_context *lfs_xip_ctx, struct lfs_config *cfg);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LFS_PORT_H */
