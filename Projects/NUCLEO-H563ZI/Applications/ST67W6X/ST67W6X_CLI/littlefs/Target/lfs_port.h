/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lfs_port.h
  * @author  GPM Application Team
  * @brief   lfs flash port definition
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
struct lfs_context
{
  lfs_t lfs;
  uint32_t flash_addr;
  SemaphoreHandle_t fs_giant_lock;
  char *partition_name;
};

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported functions --------------------------------------------------------*/
lfs_t *lfs_flash_init(struct lfs_context *lfs_xip_ctx, struct lfs_config *cfg);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LFS_PORT_H */
