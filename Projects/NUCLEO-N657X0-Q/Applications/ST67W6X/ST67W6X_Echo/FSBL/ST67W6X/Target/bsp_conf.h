/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bsp_conf.h
  * @author  GPM Application Team
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
#ifndef BSP_CONF_H
#define BSP_CONF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/** Interfaces the UART instance to be used for logging communication */
#define UART_HANDLE                             huart1

/** Interfaces the SPI instance to be used for NCP communication */
#define NCP_SPI_HANDLE                          hspi5

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_CONF_H */
