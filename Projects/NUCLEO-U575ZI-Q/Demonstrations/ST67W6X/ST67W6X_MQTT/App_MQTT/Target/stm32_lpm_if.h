/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32_lpm_if.h
  * @author  GPM Application Team
  * @brief   Header for stm32_lpm_if.c module (device specific LP management)
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
#ifndef STM32_LPM_IF_H
#define STM32_LPM_IF_H

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
  * @brief  Enters Low Power Off Mode
  */
void PWR_EnterOffMode(void);
/**
  * @brief  Exits Low Power Off Mode
  */
void PWR_ExitOffMode(void);

/**
  * @brief  Enters Low Power Stop Mode
  */
void PWR_EnterStopMode(void);
/**
  * @brief  Exits Low Power Stop Mode
  */
void PWR_ExitStopMode(void);

/**
  * @brief  Enters Low Power Sleep Mode
  */
void PWR_EnterSleepMode(void);

/**
  * @brief  Exits Low Power Sleep Mode
  */
void PWR_ExitSleepMode(void);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32_LPM_IF_H */
