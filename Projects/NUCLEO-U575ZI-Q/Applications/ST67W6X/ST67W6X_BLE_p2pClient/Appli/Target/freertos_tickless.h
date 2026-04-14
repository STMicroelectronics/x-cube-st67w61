/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    freertos_tickless.h
  * @author  ST67 Application Team
  * @brief   Management of timers and ticks header file
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
#ifndef FREERTOS_TICKLESS_H
#define FREERTOS_TICKLESS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/** SPI event bit to enable/disable low power mode entry when SPI is ongoing */
#define CFG_TICKLESS_SPIIF_ID   0
/** UART event bit to enable/disable low power mode entry when UART is ongoing */
#define CFG_TICKLESS_LOG_ID     1

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Disable FreeRTOS Low Power Entry
  * @param  bitmask: requester Id
  */
void DisableSuppressTicksAndSleep(uint32_t bitmask);

/**
  * @brief  Enable FreeRTOS Low Power Entry
  * @param  bitmask: requester Id
  */
void EnableSuppressTicksAndSleep(uint32_t bitmask);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FREERTOS_TICKLESS_H */
