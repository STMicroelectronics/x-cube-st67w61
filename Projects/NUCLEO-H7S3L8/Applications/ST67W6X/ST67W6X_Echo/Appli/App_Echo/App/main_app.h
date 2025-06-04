/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main_app.h
  * @author  GPM Application Team
  * @brief   Header for main_app.c
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
#ifndef __MAIN_APP_H
#define __MAIN_APP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
#define HOST_APP_VERSION_MAIN   1   /*!< [31:24] main version */
#define HOST_APP_VERSION_SUB1   0   /*!< [23:16] sub1 version */
#define HOST_APP_VERSION_SUB2   0   /*!< [15:8]  sub2 version */

#define HOST_APP_MAIN_SHIFT     24  /*!< main version shift */
#define HOST_APP_SUB1_SHIFT     16  /*!< sub1 version shift */
#define HOST_APP_SUB2_SHIFT     8   /*!< sub2 version shift */

/** Application version 32bits formatted */
#define HOST_APP_VERSION        ((HOST_APP_VERSION_MAIN  << HOST_APP_MAIN_SHIFT)\
                                 |(HOST_APP_VERSION_SUB1 << HOST_APP_SUB1_SHIFT)\
                                 |(HOST_APP_VERSION_SUB2 << HOST_APP_SUB2_SHIFT))

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
  * @brief  Main application
  */
void main_app(void);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MAIN_APP_H */
