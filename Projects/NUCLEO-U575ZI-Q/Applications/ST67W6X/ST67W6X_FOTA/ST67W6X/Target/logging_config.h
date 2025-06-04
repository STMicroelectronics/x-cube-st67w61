/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    logging_config.h
  * @author  GPM Application Team
  * @brief   Header file for the W6X Logging configuration module
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
#ifndef LOGGING_CONFIG_H
#define LOGGING_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"
#include "logging_levels.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/*
 * All available configuration defines can be found in
 * Middlewares\ST\ST67W6X_Network_Driver\Conf\logging_config_template.h
 */

/** Global verbosity level (LOG_NONE, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG) */
#define LOG_LEVEL                               LOG_DEBUG

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LOGGING_CONFIG_H */
