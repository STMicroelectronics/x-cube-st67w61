/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32_lpm_if.c
  * @author  GPM Application Team
  * @brief   Low layer function to enter/exit low power modes (stop, sleep)
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

/* Includes ------------------------------------------------------------------*/
#include "stm32_lpm.h"
#include "stm32_lpm_if.h"
#include "stm32u5xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Power driver callbacks handler
  */
const struct UTIL_LPM_Driver_s UTIL_PowerDriver =
{
  PWR_EnterSleepMode,
  PWR_ExitSleepMode,

  PWR_EnterStopMode,
  PWR_ExitStopMode,

  PWR_EnterOffMode,
  PWR_ExitOffMode,
};

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Functions Definition ------------------------------------------------------*/
void PWR_EnterOffMode(void)
{
  /* USER CODE BEGIN PWR_EnterOffMode_1 */

  /* USER CODE END PWR_EnterOffMode_1 */
}

void PWR_ExitOffMode(void)
{
  /* USER CODE BEGIN PWR_ExitOffMode_1 */

  /* USER CODE END PWR_ExitOffMode_1 */
}

void PWR_EnterStopMode(void)
{
  /* USER CODE BEGIN PWR_EnterStopMode_1 */

  /* USER CODE END PWR_EnterStopMode_1 */
  HAL_SuspendTick();
  HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
  /* USER CODE BEGIN PWR_EnterStopMode_End */

  /* USER CODE END PWR_EnterStopMode_End */
}

void PWR_ExitStopMode(void)
{
  /* USER CODE BEGIN PWR_ExitStopMode_1 */

  /* USER CODE END PWR_ExitStopMode_1 */
  extern void SystemClock_Config(void);
  SystemClock_Config();

  HAL_ResumeTick();
  /* USER CODE BEGIN PWR_ExitStopMode_End */

  /* USER CODE END PWR_ExitStopMode_End */
}

void PWR_EnterSleepMode(void)
{
  /* USER CODE BEGIN PWR_EnterSleepMode_1 */

  /* USER CODE END PWR_EnterSleepMode_1 */
  /* Suspend sysTick */
  HAL_SuspendTick();

  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  /* USER CODE BEGIN PWR_EnterSleepMode_End */

  /* USER CODE END PWR_EnterSleepMode_End */
}

void PWR_ExitSleepMode(void)
{
  /* USER CODE BEGIN PWR_ExitSleepMode_1 */

  /* USER CODE END PWR_ExitSleepMode_1 */
  /* Resume sysTick */
  HAL_ResumeTick();
  /* USER CODE BEGIN PWR_ExitSleepMode_End */

  /* USER CODE END PWR_ExitSleepMode_End */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
