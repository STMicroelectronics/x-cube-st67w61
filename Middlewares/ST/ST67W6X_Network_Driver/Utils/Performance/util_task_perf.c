/**
  ******************************************************************************
  * @file    util_task_perf.c
  * @author  GPM Application Team
  * @brief   This file provides the implementation of Task Performance measurement
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

#include "util_task_perf.h"
#include "w6x_config.h"
#include "shell.h"
#include "logging.h"
#include "FreeRTOS.h"
#include "task.h"

/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Task_Perf_Constants ST67W6X Utility Performance Task Perf Constants
  * @ingroup  ST67W6X_Utilities_Performance_Task_Perf
  * @{
  */

#ifndef TASK_PERF_ENABLE
/** Enable task performance measurement */
#define TASK_PERF_ENABLE      0
#endif /* TASK_PERF_ENABLE */

#if (__CORTEX_M < 3)
#undef TASK_PERF_ENABLE
#define TASK_PERF_ENABLE      0
#pragma message("TASK_PERF is not supported because Data Watchpoint and Trace (DWT) is required on the cortex")
#endif /* __CORTEX_M < 3 */

#ifndef PERF_MAXTHREAD
/** Maximum number of thread to monitor */
#define PERF_MAXTHREAD        8U
#endif /* PERF_MAXTHREAD */

/**
  * @brief  64 bits number to string conversion
  * @note   20 DIGITS for 64bits and 1 bits for '\0'
  */
#define STR64BIT_DIGIT        20+1

/** Number of delimiter line characters to display the report */
#define SEPARATOR_SIZE        56

/** @} */

/* Private typedef -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Task_Perf_Types ST67W6X Utility Performance Task Perf Types
  * @ingroup  ST67W6X_Utilities_Performance_Task_Perf
  * @{
  */

/**
  * @brief  Task performance state
  */
typedef enum
{
  PERF_TASK_STATE_INIT = 0,                     /*!< Task performance not initialized */
  PERF_TASK_STATE_STOPPED,                      /*!< Task performance stopped */
  PERF_TASK_STATE_RUNNING,                      /*!< Task performance running */
} task_perf_state_e;

/**
  * @brief  Task performance structure
  */
typedef struct
{
  uint32_t      state;                          /*!< Task performance state */
  uint32_t      task_count;                     /*!< Task count */
  uint64_t      elapsed_cycle[PERF_MAXTHREAD];  /*!< Elapsed cycle for each thread */
  uint32_t      cycle_in[PERF_MAXTHREAD];       /*!< Cycle value when enter in the thread */
  TaskHandle_t  handle[PERF_MAXTHREAD];         /*!< Task handle for each thread */
  char          name[PERF_MAXTHREAD][11];       /*!< Task name for each thread */
  uint32_t      task_current_cycle;             /*!< Current cycle value */
} task_perf_t;

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Task_Perf_Variables ST67W6X Utility Performance Task Perf Variables
  * @ingroup  ST67W6X_Utilities_Performance_Task_Perf
  * @{
  */

#if (TASK_PERF_ENABLE == 1)
/** Task performance structure */
static task_perf_t task_perf;

/** Task Performance report title */
static const char report_title[] = "Task Performance report";
#endif /* TASK_PERF_ENABLE */

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Task_Perf_Functions ST67W6X Utility Performance Task Perf Functions
  * @ingroup  ST67W6X_Utilities_Performance_Task_Perf
  * @{
  */

/**
  * @brief  Get the current cycle value
  * @retval Current cycle value
  */
uint32_t get_cycle(void);

/**
  * @brief  Get the task name
  * @param  xHandle: Task handle
  * @retval Task name
  */
const char *task_perf_get_task_name(TaskHandle_t xHandle);

/**
  * @brief  num64 to string converter as not available in reduced C
  * @param  out: pointer to the output string
  * @param  out_strlen: length of the output string
  * @param  num: 64 bit number to convert
  */
void num2string64(char *out, int32_t out_strlen, const uint64_t num);

#if (TASK_PERF_ENABLE == 1)
/**
  * @brief  Task Performance measurement stop or start shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  */
int32_t task_perf_shell(int32_t argc, char **argv);

/**
  * @brief  Task Performance measurement report shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  */
int32_t task_perf_shell_report(int32_t argc, char **argv);
#endif /* TASK_PERF_ENABLE */

/**
  * @brief  Display a table of tasks in the system.
  * @param  argc: Number of arguments
  * @param  argv: Pointer to the list of arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t task_alloc_shell_report(int32_t argc, char **argv);

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Task_Perf_Functions
  * @{
  */

void task_perf_in_hook(void)
{
#if (TASK_PERF_ENABLE == 1)
  uint32_t i;
  static uint32_t first_time = 1;
  /* Get the current task handle */
  TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();

  /* Initialize the task_perf structure at startup */
  if (first_time)
  {
    first_time = 0;
    memset(&task_perf, 0, sizeof(task_perf));
  }

  for (i = 0; i < PERF_MAXTHREAD; i++)
  {
    /* Check if the thread is already in the list */
    if (NULL == task_perf.handle[i])
    {
      /* New thread */
      task_perf.task_count++;
      task_perf.handle[i] = xHandle;
      snprintf(task_perf.name[i], 11, "%10s", task_perf_get_task_name(xHandle));
    }

    if (xHandle == task_perf.handle[i])
    {
      /* Save the enter cycle count */
      task_perf.cycle_in[i] = get_cycle();
      break;
    }
  }
#endif /* TASK_PERF_ENABLE */
}

void task_perf_out_hook(void)
{
#if (TASK_PERF_ENABLE == 1)
  uint32_t i;
  /* Get the current task handle */
  TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();

  for (i = 0; i < PERF_MAXTHREAD; i++)
  {
    /* Check if the thread is already in the list */
    if (NULL == task_perf.handle[i])
    {
      /* New thread */
      task_perf.handle[i] = xHandle;
      snprintf(task_perf.name[i], 11, "%10s", task_perf_get_task_name(xHandle));
    }

    if (xHandle == task_perf.handle[i])
    {
      /* Save the exit cycle count */
      task_perf.elapsed_cycle[i] += (get_cycle() - task_perf.cycle_in[i]);
      break;
    }
  }
#endif /* TASK_PERF_ENABLE */
}

void task_perf_start(void)
{
#if (TASK_PERF_ENABLE == 1)
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

  /* Initialize the task_perf structure */
  (void)memset(&task_perf, 0, sizeof(task_perf));
  task_perf_in_hook();
  task_perf.state = PERF_TASK_STATE_RUNNING;

#endif /* TASK_PERF_ENABLE */
}

void task_perf_resume(void)
{
#if (TASK_PERF_ENABLE == 1)
  /* Resume DWT counter to continue the next in/out hook acquisition */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  task_perf.state = PERF_TASK_STATE_RUNNING;
#endif /* TASK_PERF_ENABLE */
}

void task_perf_stop(void)
{
#if (TASK_PERF_ENABLE == 1)
  /* Stop DWT counter to freeze all next in/out hook acquisition */
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
  task_perf.task_current_cycle = get_cycle();
  task_perf.state = PERF_TASK_STATE_STOPPED;
#endif /* TASK_PERF_ENABLE */
}

void task_perf_report(void)
{
#if (TASK_PERF_ENABLE == 1)
  uint64_t total_cycle = 0;
  char cycle_string64[STR64BIT_DIGIT];
  char separator[SEPARATOR_SIZE] = {0};
  memset(separator, '-', sizeof(separator) - 1);

  task_perf_out_hook();

  if (task_perf.state == PERF_TASK_STATE_INIT)
  {
    LogInfo("### %s: not started\n", report_title);
  }
  else if (task_perf.task_count == 1)
  {
    LogInfo("### %s: only one thread detected. verify if the hooks are called by the RTOS\n", report_title);
  }
  else
  {
    /* Display the CPU frequency */
    LogInfo("### %s CPU Freq %3" PRIu32 " Mhz\n", report_title, SystemCoreClock / 1000000U);

    /* Display the header */
    LogInfo("%-17s%-16s%-14stime (ms)\n", "thread id", "name", "cycles");
    LogInfo("%s\n", separator);

    {
      uint32_t count = 0;
      /* Display the thread performance */
      while ((count < PERF_MAXTHREAD) && task_perf.handle[count] != 0)
      {
        num2string64(cycle_string64, STR64BIT_DIGIT, task_perf.elapsed_cycle[count]);
        LogInfo("thread #%" PRIu32 "  %10s   %15s  %10" PRIu32 "\n", count, task_perf.name[count],
                cycle_string64, (uint32_t)(task_perf.elapsed_cycle[count] / (SystemCoreClock / 1000U)));
        total_cycle += task_perf.elapsed_cycle[count];
        count++;
      }
      LogInfo("%s\n", separator);
    }
    /* Display the total performance */
    num2string64(cycle_string64, STR64BIT_DIGIT, total_cycle);
    LogInfo("%-24s%15s  %10" PRIu32 "\n", "Total", cycle_string64,
            (uint32_t)(total_cycle / (SystemCoreClock / 1000U)));

    LogInfo("### %s end\n", report_title);
  }

  task_perf_in_hook();

#endif /* TASK_PERF_ENABLE */

  task_alloc_report();
}

void task_alloc_report(void)
{
#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )
  char *p_write_buffer;
  char *p_info;

  p_info = pvPortMalloc(512);
  if (NULL == p_info)
  {
    return;
  }

  /* Fill the buffer with the header description */
  snprintf(p_info, 512, "%-*s%s    %s   %s %s\n********************************************\n",
           (configMAX_TASK_NAME_LEN - 3), "Task", "State", "Prio", "Free(u32)", "Index");
  /* Get the address of the buffer after the header description */
  p_write_buffer = p_info + strlen(p_info);
  /* Fill the last character of the buffer with the null character */
  p_write_buffer[0] = '\0';
  vTaskList(p_write_buffer);
  LogInfo("%s", p_info);

  vPortFree(p_info);
#endif /* ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) */
  return;
}

/* Private Functions Definition ----------------------------------------------*/
/**
  * @brief  Get the current system time
  * @retval Current system time
  */
uint32_t sys_now(void);

__attribute__((weak)) uint32_t sys_now(void)
{
  return xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
}

uint32_t get_cycle(void)
{
#if (TASK_PERF_ENABLE == 1)
  if (task_perf.state == PERF_TASK_STATE_RUNNING)
  {
    return DWT->CYCCNT;
  }
  else
  {
    /* Return the last cycle value */
    return task_perf.task_current_cycle;
  }
#else
  return 0;
#endif /* TASK_PERF_ENABLE */
}

const char *task_perf_get_task_name(TaskHandle_t xHandle)
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

void num2string64(char *out, int32_t out_strlen, const uint64_t num)
{
  uint64_t temp = num;
  uint32_t count = 0;

  /* Count number of digit */
  while ((temp != 0) && (count < out_strlen))
  {
    temp = temp / 10;
    count++;
  }

  /* Go to end of string */
  out += count;

  /* Write end of string char */
  *out-- = '\0';

  temp = num;
  while (temp != 0)
  {
    *out-- = (char)((temp % 10) + '0');
    temp = temp / 10;
  }
}

#if (TASK_PERF_ENABLE == 1)
int32_t task_perf_shell(int32_t argc, char **argv)
{
  if ((argc == 2) && (strncmp(argv[1], "-s", 2) == 0))
  {
    task_perf_stop();
  }
  else
  {
    task_perf_start();
  }
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(task_perf_shell, task_perf, task_perf [ -s ]. Start or stop [ -s ] task performance measurement);

int32_t task_perf_shell_report(int32_t argc, char **argv)
{
  (void)argc;
  (void)argv;

  task_perf_report();

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(task_perf_shell_report, task_report, task_report. Display task performance report);

#endif /* TASK_PERF_ENABLE */

/** @} */
