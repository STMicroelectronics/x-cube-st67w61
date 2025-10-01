/**
  ******************************************************************************
  * @file    util_task_perf.h
  * @author  GPM Application Team
  * @brief   This file provides the definition of Task Performance measurement
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef UTIL_TASK_PERF_H
#define UTIL_TASK_PERF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "w6x_config.h"

/* Exported constants --------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Task_Perf_Constants ST67W6X Utility Performance Task Perf Constants
  * @ingroup  ST67W6X_Utilities_Performance_Task_Perf
  * @{
  */

#ifndef TASK_PERF_ENABLE
/** Enable task performance measurement */
#define TASK_PERF_ENABLE      0
#endif /* TASK_PERF_ENABLE */

/** @} */

/* Exported functions --------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Task_Perf_Functions ST67W6X Utility Performance Task Perf Functions
  * @ingroup  ST67W6X_Utilities_Performance_Task_Perf
  * @{
  */

/**
  * @brief  Task performance hook when enter in FreeRTOS task
  */
void task_perf_in_hook(void);

/**
  * @brief  Task performance hook when exit from FreeRTOS task
  */
void task_perf_out_hook(void);

/**
  * @brief  Task performance start
  */
void task_perf_start(void);

/**
  * @brief  Task performance resume
  */
void task_perf_resume(void);

/**
  * @brief  Task performance stop
  */
void task_perf_stop(void);

/**
  * @brief  Task performance report
  */
void task_perf_report(void);

/**
  * @brief  Task allocation report
  */
void task_alloc_report(void);

/**
  * @brief  Task performance reset cortex counter
  */
void util_task_port_reset(void);

/**
  * @brief  Task performance resume cortex counter
  */
void util_task_port_resume(void);

/**
  * @brief  Task performance stop cortex counter
  */
void util_task_port_stop(void);

/**
  * @brief  Task performance get cortex counter cycle
  * @return  Cortex counter cycle
  */
uint32_t util_task_port_get_cycle(void);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UTIL_TASK_PERF_H */
