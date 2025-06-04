/**
  ******************************************************************************
  * @file    util_mem_perf.c
  * @author  GPM Application Team
  * @brief   This file provides the implementation of Memory Performance
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

/* Includes ------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "util_mem_perf.h"
#include "w6x_config.h"
#include "shell.h"
#include "logging.h"
#include "FreeRTOS.h"
#include "task.h"

/* Private typedef -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Mem_Perf_Types ST67W6X Utility Performance Mem Perf Types
  * @ingroup  ST67W6X_Utilities_Performance_Mem_Perf
  * @{
  */

/**
  * @brief  Memory Performance structure
  */
typedef struct
{
  uint16_t      iter;       /*!< Iteration number */
  uint32_t      size;       /*!< Size of the allocated memory */
  char          name[11];   /*!< Task name */
  uint32_t      *p;         /*!< Pointer to the allocated memory */
} mem_perf_t;

/**
  * @brief  Block link structure
  */
typedef struct A_BLOCK_LINK
{
  struct A_BLOCK_LINK *pxNextFreeBlock; /*!< The next free block in the list. */
  size_t xBlockSize;                    /*!< The size of the free block. */
} BlockLink_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Mem_Perf_Constants ST67W6X Utility Performance Mem Perf Constants
  * @ingroup  ST67W6X_Utilities_Performance_Mem_Perf
  * @{
  */

#ifndef MEM_PERF_ENABLE
/** Enable memory performance measurement */
#define MEM_PERF_ENABLE       0
#endif /* MEM_PERF_ENABLE */

#ifndef LEAKAGE_ARRAY
/** Number of memory allocation to keep track of */
#define LEAKAGE_ARRAY         500U
#endif /* LEAKAGE_ARRAY */

#ifndef ALLOC_BREAK
/** Allocator maximum iteration before to break */
#define ALLOC_BREAK           0xFFFFFFFFU
#endif /* ALLOC_BREAK */

/** Number of allocation before stopping the program */
#define MAX_ALLOC_VALUE       0x4000U

/** Number of delimiter line characters to display the report */
#define SEPARATOR_SIZE        38

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Mem_Perf_Variables ST67W6X Utility Performance Mem Perf Variables
  * @ingroup  ST67W6X_Utilities_Performance_Mem_Perf
  * @{
  */

#if (MEM_PERF_ENABLE == 1)
size_t freeHeapSize             = 0;              /*!< Free heap size */
size_t freeHeapSizeMin          = 0;              /*!< Minimum free heap size */

static mem_perf_t mem_perf[LEAKAGE_ARRAY] = {0};  /*!< Memory performance array */
static uint32_t iteralloc       = 0;              /*!< Number of allocations */
static uint32_t iterfree        = 0;              /*!< Number of free */
static uint32_t current_alloc   = 0;              /*!< Current allocation */
static uint32_t max_alloc       = 0;              /*!< Maximum allocation */

static uint32_t alloc_report_enable = 1;          /*!< Allocation report enable */

/** Memory Performance report title */
static const char report_title[] = "Memory Performance report";
#endif /* MEM_PERF_ENABLE */

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Mem_Perf_Functions ST67W6X Utility Performance Mem Perf Functions
  * @ingroup  ST67W6X_Utilities_Performance_Mem_Perf
  * @{
  */

/**
  * @brief  Memory Performance measurement report shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  */
int32_t mem_perf_shell_report(int32_t argc, char **argv);

/**
  * @brief  Get the task name
  * @param  xHandle: Task handle
  * @retval Task name
  */
const char *mem_perf_get_task_name(TaskHandle_t xHandle);

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Mem_Perf_Functions
  * @{
  */

void mem_perf_malloc_hook(void *pvAddress, size_t uiSize)
{
#if (MEM_PERF_ENABLE == 1)
  if (alloc_report_enable == 1)
  {
    uint32_t i;
    /* Get the current free heap size */
    freeHeapSize = xPortGetFreeHeapSize();
    /* Get the minimum free heap size */
    freeHeapSizeMin = xPortGetMinimumEverFreeHeapSize();

    iteralloc++;
    current_alloc += uiSize;

    /* Check if the current allocation is greater than the maximum number of allocation */
    if (current_alloc > max_alloc)
    {
      max_alloc = current_alloc;
    }

    if (pvAddress == NULL)
    {
      printf("Allocation failure");

      while (1);
    }

    for (i = 0; i < LEAKAGE_ARRAY; i++)
    {
      /* Find the first free slot memory performance array */
      if (mem_perf[i].p == 0U)
      {
        /* Save the allocation information */
        TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();
        mem_perf[i].p = (uint32_t *)pvAddress;
        mem_perf[i].size = uiSize;
        mem_perf[i].iter = (uint16_t) iteralloc;
        if (xHandle != NULL)
        {
          snprintf(mem_perf[i].name, 11, "%10s", mem_perf_get_task_name(xHandle));
        }
        break;
      }
    }
  }
#endif /* MEM_PERF_ENABLE */
}

void mem_perf_free_hook(void *pvAddress, size_t uiSize)
{
#if (MEM_PERF_ENABLE == 1)
  uint32_t i;

  /* Get the current free heap size */
  freeHeapSize = xPortGetFreeHeapSize();

#ifdef FREE_STOP_ON_NULL_POINTER
  if (NULL == pvAddress)
  {
    LogError("Free function : Freeing a NULL pointer\n");
    while (1);
  }
#endif /* STOP_ON_FREEING_NULL_POINTER */

  if (NULL != pvAddress)
  {

    for (i = 0; i < LEAKAGE_ARRAY; i++)
    {
      /* Find the segment to free */
      if (mem_perf[i].p == (uint32_t *)pvAddress)
      {
        iterfree++;
        mem_perf[i].p = NULL;
        current_alloc -= mem_perf[i].size;

        mem_perf[i].size = 0;
        break;
      }
    }
  }
#endif /* MEM_PERF_ENABLE */
}

void mem_perf_report(void)
{
#if (MEM_PERF_ENABLE == 1)
  char separator[SEPARATOR_SIZE] = {0};

  memset(separator, '-', sizeof(separator) - 1);
  /* Display the summary */
  LogInfo("### %s\n", report_title);
  LogInfo("# Max allocated:        %" PRIu32 " bytes\n", max_alloc);
  LogInfo("# Current leakage:      %" PRIu32 " bytes\n", current_alloc);
  LogInfo("# Nb Max allocated:     %" PRIu32 "\n", iteralloc);
  LogInfo("# Nb Current allocated: %" PRIu32 "\n", iteralloc - iterfree);

  LogInfo("%s\n", separator);
  LogInfo("### Remaining Mem report\n");
  LogInfo("%-7s%-14s%-6s%-10s\n", "id", "address", "size", "task");
  LogInfo("%s\n", separator);

  /* Display all remaining entries still allocated */
  for (uint32_t i = 0; i < LEAKAGE_ARRAY; i++)
  {
    if (mem_perf[i].p != 0)
    {
      alloc_report_enable = 0;
      LogInfo("#%04" PRIu32 "  %" PRIu32 "%8" PRIu32 "  %10s\n",
              (uint32_t)mem_perf[i].iter, (uint32_t)mem_perf[i].p, mem_perf[i].size, mem_perf[i].name);
      alloc_report_enable = 1;
      vTaskDelay(10);
    }
  }
  LogInfo("%s\n", separator);
  LogInfo("### %s end\n", report_title);
#endif /* MEM_PERF_ENABLE */
}

/* Private Functions Definition ----------------------------------------------*/
int32_t mem_perf_shell_report(int32_t argc, char **argv)
{
  (void) argc;
  (void) argv;

  mem_perf_report();

  return SHELL_STATUS_OK;
}

#if (MEM_PERF_ENABLE == 1)
SHELL_CMD_EXPORT_ALIAS(mem_perf_shell_report, mem_report, display memory performance report);
#endif /* MEM_PERF_ENABLE */

const char *mem_perf_get_task_name(TaskHandle_t xHandle)
{
  TaskStatus_t xTaskDetails = {0};
  vTaskGetInfo(
    /* The handle of the task being queried. */
    xHandle,
    /* The TaskStatus_t structure to complete with information on xTask. */
    &xTaskDetails,
    /* Include the stack high water mark value in the TaskStatus_t structure. */
    pdTRUE,
    /* Include the task state in the TaskStatus_t structure. */
    eInvalid);
  return (const char *)xTaskDetails.pcTaskName;
}

/** @} */
