/**
  ******************************************************************************
  * @file    util_mem_perf.h
  * @author  GPM Application Team
  * @brief   This file provides the definition of Memory Performance
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
#ifndef UTIL_MEM_PERF_H
#define UTIL_MEM_PERF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Mem_Perf_Functions ST67W6X Utility Performance Mem Perf Functions
  * @ingroup  ST67W6X_Utilities_Performance_Mem_Perf
  * @{
  */

/**
  * @brief  Memory allocation hook
  * @param  pvAddress: pointer to the allocated memory
  * @param  uiSize: size of the allocated memory
  */
void mem_perf_malloc_hook(void *pvAddress, size_t uiSize);

/**
  * @brief  Memory free hook
  * @param  pvAddress: pointer to the memory to free
  * @param  uiSize: size of the memory to free
  */
void mem_perf_free_hook(void *pvAddress, size_t uiSize);

/**
  * @brief  Memory performance report
  */
void mem_perf_report(void);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UTIL_MEM_PERF_H */
