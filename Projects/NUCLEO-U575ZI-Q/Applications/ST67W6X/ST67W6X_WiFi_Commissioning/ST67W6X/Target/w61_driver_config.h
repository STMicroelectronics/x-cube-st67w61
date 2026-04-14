/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w61_driver_config.h
  * @author  ST67 Application Team
  * @brief   Header file for the W61 configuration module
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
#ifndef W61_DRIVER_CONFIG_H
#define W61_DRIVER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/** ============================
  * AT Wi-Fi
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Maximum number of detected AP during the scan. Cannot be greater than 50 */
#define W61_WIFI_MAX_DETECTED_AP                20U

/** Enable/Disable Wi-Fi module logging */
#define WIFI_LOG_ENABLE                         1

/** ============================
  * AT Net
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_default_config.h
  * ============================
  */

/** Enable IPv6 support : 0: Disabled, 1: Enabled */
#define W61_NET_IPV6_ENABLE                     0

/** Enable/Disable Network module logging */
#define NET_LOG_ENABLE                          1

/** ============================
  * AT Common
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Driver\W61_at\w61_at_common.h
  * ============================
  */
/** Maximum SPI buffer size */
#define W61_MAX_SPI_XFER                        1520U

/** Enable/Disable System module logging */
#define SYS_LOG_ENABLE                          1

/** Debugging only: Enable/Disable AT log, i.e. logs the AT commands incoming/outcoming from/to the NCP */
#define W61_AT_LOG_ENABLE                       0
#include "logging.h"

/** Enable/Disable Modem command log */
#define MDM_CMD_LOG_ENABLE                      0

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W61_DRIVER_CONFIG_H */
