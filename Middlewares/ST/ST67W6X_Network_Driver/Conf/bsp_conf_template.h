/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bsp_conf_template.h
  * @author  ST67 Application Team
  * @brief   This file contains definitions for the BSP interface
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
#ifndef BSP_CONF_TEMPLATE_H
#define BSP_CONF_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/** Interfaces the LPTIM instance to be used for FreeRTOS tickless */
#define LPTIM_HANDLE                            hlptim1
/** LPTIM instance to be used for FreeRTOS tickless */
#define LPTIM_IDLE                              LPTIM1
/** LPTIM IRQn to be used for FreeRTOS tickless */
#define LPTIM_IDLE_IRQn                         LPTIM1_IRQn
/** LPTIM clock enable macro to be used for FreeRTOS tickless */
#define LPTIM_CLK_ENABLE                        __HAL_RCC_LPTIM1_CLKAM_ENABLE

/** Interfaces the UART instance to be used for logging communication */
#define UART_HANDLE                             huart1

/** Interfaces the SPI instance to be used for NCP communication */
#define NCP_SPI_HANDLE                          hspi1

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Global variables ----------------------------------------------------------*/
extern SPI_HandleTypeDef NCP_SPI_HANDLE;

#ifdef LPTIM_HANDLE
extern LPTIM_HandleTypeDef LPTIM_HANDLE;
#endif /* LPTIM_HANDLE */

#ifdef UART_HANDLE
extern UART_HandleTypeDef UART_HANDLE;
#endif /* UART_HANDLE */

/* USER CODE BEGIN GV */

/* USER CODE END GV */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_CONF_TEMPLATE_H */
