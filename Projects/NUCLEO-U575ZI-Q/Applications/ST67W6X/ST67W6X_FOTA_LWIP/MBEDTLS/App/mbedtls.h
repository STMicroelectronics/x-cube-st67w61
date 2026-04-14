/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    mbedtls.h
  * @author  ST67 Application Team
  * @brief   This file provides code for the configuration of the mbedtls instances.
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
#ifndef MBEDTLS_H
#define MBEDTLS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include MBEDTLS_CONFIG_FILE

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported functions --------------------------------------------------------*/
/* MBEDTLS init function */
void MX_MBEDTLS_Init(void);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MBEDTLS_H */
