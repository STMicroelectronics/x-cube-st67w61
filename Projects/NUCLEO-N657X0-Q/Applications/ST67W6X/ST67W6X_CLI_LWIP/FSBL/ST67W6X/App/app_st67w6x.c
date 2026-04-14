/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_st67w6x.c
  * @author  ST67 Application Team
  * @brief   This file provides code for the configuration of the STMicroelectronics.X-CUBE-ST67W61.1.3.0 instances.
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "main.h"
#include "main_app.h"
#include "app_st67w6x.h"

#ifndef REDEFINE_FREERTOS_INTERFACE
#include "FreeRTOS.h"
#include "task.h"
#endif /* REDEFINE_FREERTOS_INTERFACE */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Task function for the ST67W6X instance
  * @param  argument Pointer to the task argument
  */
static void ST67W6X_Task(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
/* Private defines -----------------------------------------------------------*/
#ifndef ST67W6X_TASK_STACK_SIZE
/** ST67W6X stack size */
#define ST67W6X_TASK_STACK_SIZE        4096U
#endif /* ST67W6X_TASK_STACK_SIZE */

#ifndef ST67W6X_TASK_PRIO
/** ST67W6X priority */
#define ST67W6X_TASK_PRIO              24U
#endif /* ST67W6X_TASK_PRIO */

/* USER CODE BEGIN PD */

/* USER CODE END PD */
/* Functions Definition ------------------------------------------------------*/
void MX_ST67W6X_Init(void)
{
  /* USER CODE BEGIN ST67W6X_Init_1 */

  /* USER CODE END ST67W6X_Init_1 */
  (void)xTaskCreate(ST67W6X_Task,
                    "ST67W6XTask",
                    ST67W6X_TASK_STACK_SIZE >> 2U,
                    NULL,
                    ST67W6X_TASK_PRIO,
                    NULL);

  /* USER CODE BEGIN ST67W6X_Init_End */

  /* USER CODE END ST67W6X_Init_End */
  return;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void ST67W6X_Task(void *argument)
{
  /* USER CODE BEGIN ST67W6X_Task_1 */

  /* USER CODE END ST67W6X_Task_1 */

  main_app();

  /* Infinite loop */
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  /* USER CODE BEGIN ST67W6X_Task_End */

  /* USER CODE END ST67W6X_Task_End */
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
