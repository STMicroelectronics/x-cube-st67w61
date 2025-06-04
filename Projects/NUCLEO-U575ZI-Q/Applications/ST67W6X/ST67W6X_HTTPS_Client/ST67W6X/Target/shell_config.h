/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    shell_config.h
  * @author  GPM Application Team
  * @brief   Header file for the W6X Shell configuration module
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
#ifndef SHELL_CONFIG_H
#define SHELL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/*
 * All available configuration defines can be found in
 * Middlewares\ST\ST67W6X_Network_Driver\Conf\shell_config_template.h
 */

/** Enable the shell component */
#define SHELL_ENABLE                            1

/** Shell command maximum size */
#define SHELL_CMD_SIZE                          256

/** Shell using color */
#define SHELL_USING_COLOR                       1

/** Print an additional status message at the end of the command execution */
#define SHELL_PRINT_STATUS                      0

/** Shell receive buffer size */
#define SHELL_FREERTOS_RX_BUFF_SIZE             256

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SHELL_CONFIG_H */
