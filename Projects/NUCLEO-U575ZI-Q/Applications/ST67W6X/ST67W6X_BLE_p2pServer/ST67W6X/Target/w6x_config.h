/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    w6x_config.h
  * @author  GPM Application Team
  * @brief   Header file for the W6X configuration module
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
#ifndef W6X_CONFIG_H
#define W6X_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/*
 * All available configuration defines can be found in
 * Middlewares\ST\ST67W6X_Network_Driver\Conf\w6x_config_template.h
 */

/** ============================
  * System
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** NCP will go by default in low power mode when NCP is in idle mode */
#define W6X_POWER_SAVE_AUTO                     0

/** NCP clock mode : 1: Internal RC oscillator, 2: External passive crystal, 3: External active crystal */
#define W6X_CLOCK_MODE                          1

/** ============================
  * BLE
  *
  * All available configuration defines in
  * Middlewares\ST\ST67W6X_Network_Driver\Core\w6x_default_config.h
  * ============================
  */

/** String defining BLE hostname */
#define W6X_BLE_HOSTNAME                        "p2pS"

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_CONFIG_H */
