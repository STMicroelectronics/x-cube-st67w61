/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    httpserver.h
  * @author  GPM Application Team
  * @brief   Http server declarations.
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
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
typedef struct PinInfos
{
  GPIO_PinState             btn_state;         /* user button state */
  GPIO_PinState             led_green_state;   /* green led state */
  GPIO_PinState             led_red_state;     /* red led state */
} PinInfosTypeDef;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions --------------------------------------------------------*/
void http_server_socket(void *arg);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HTTPSERVER_H */
