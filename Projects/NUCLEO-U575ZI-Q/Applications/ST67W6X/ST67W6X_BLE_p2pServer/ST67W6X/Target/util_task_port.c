/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    util_task_port.c
  * @author  ST67 Application Team
  * @brief   Task Performance porting layer implementation
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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "util_task_perf.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
#if (TASK_PERF_ENABLE == 1)

#if (__CORTEX_M < 3)
#pragma message("Task Performance is not supported because Data Watchpoint and Trace (DWT) is required on the cortex")
#endif /* __CORTEX_M < 3 */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void util_task_port_reset(void)
{
#if (__CORTEX_M >= 3)
  /* Reset the counter value */
  DWT->CYCCNT = 0x0;

  /* Global enable of DWT and ITM block. Required if the debugger is not connected */
#if (__CORTEX_M <= 7)
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
#else
  DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk;
#endif /* (__CORTEX_M <= 7) */

  /* Start it */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif /* (__CORTEX_M >= 3) */
}

void util_task_port_resume(void)
{
#if (__CORTEX_M >= 3)
  /* Resume DWT counter to continue the next in/out hook acquisition */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif /* (__CORTEX_M >= 3) */
}

void util_task_port_stop(void)
{
#if (__CORTEX_M >= 3)
  /* Stop DWT counter to freeze all next in/out hook acquisition */
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
#endif /* (__CORTEX_M >= 3) */
}

uint32_t util_task_port_get_cycle(void)
{
#if (__CORTEX_M >= 3)
  return DWT->CYCCNT;
#else
  return 0;
#endif /* (__CORTEX_M >= 3) */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */
#endif /* TASK_PERF_ENABLE */
