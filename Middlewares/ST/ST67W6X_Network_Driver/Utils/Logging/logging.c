/**
  ******************************************************************************
  * @file    logging.c
  * @author  GPM Application Team
  * @brief   This file is part of the FreeRTOS logging interface.
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

/**
  * Portions of this file are based on FreeRTOS, which is licensed under the MIT license as indicated below.
  * See https://www.FreeRTOS.org/logging.html for more information.
  *
  * Reference source:
  * https://github.com/FreeRTOS/FreeRTOS/blob/main/FreeRTOS-Plus/Demo/Common/Logging/windows/Logging_WinSim.c
  */

/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
/* Project Includes */
#include "logging_config.h"
#include "logging.h"

/* Private default defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Logging_Defines ST67W6X Utility Logging Configuration
  * @ingroup  ST67W6X_Utilities_Logging
  * @{
  */
/* can be configured in #include "logging_config.h" */

/** Enable 'task name' metadata in the log */
#ifndef LOG_INCLUDE_TASKNAME
#define LOG_INCLUDE_TASKNAME                    1
#endif /* LOG_INCLUDE_TASKNAME */

/** Enable 'file name' + 'line number' metadata in the log */
#ifndef LOG_INCLUDE_FILENAME
#define LOG_INCLUDE_FILENAME                    1
#endif /* LOG_INCLUDE_FILENAME */

/** Enable 'timestamp' metadata in the log */
#ifndef LOG_INCLUDE_TIMESTAMP
#define LOG_INCLUDE_TIMESTAMP                   1
#endif /* LOG_INCLUDE_TIMESTAMP */

/** Max metadata length */
#ifndef LOG_INCLUDE_MAX_LENGTH
#define LOG_INCLUDE_MAX_LENGTH                  100
#endif /* LOG_INCLUDE_MAX_LENGTH */

/** Max message length */
#ifndef MAX_LOG_MESSAGE_LENGTH
#define MAX_LOG_MESSAGE_LENGTH                  2000
#endif /* MAX_LOG_MESSAGE_LENGTH */

/** Max queue length */
#ifndef MAX_LOG_QUEUE_LENGTH
#define MAX_LOG_QUEUE_LENGTH                    200
#endif /* MAX_LOG_QUEUE_LENGTH */

/** Max log level */
#ifndef MAX_LOG_LEVEL
#define MAX_LOG_LEVEL                           LOG_DEBUG
#endif /* MAX_LOG_LEVEL */

/** Max log level */
#ifndef MAX_LOG_MEMORY
#define MAX_LOG_MEMORY                          4096
#endif /* MAX_LOG_MEMORY */

#ifndef LOG_MESSAGE_ID
/** Log id. must be unique per queue producer */
#define LOG_MESSAGE_ID                          1
#endif /* LOG_MESSAGE_ID */

#ifndef LOG_THREAD_STACK_SIZE
/** Log thread stack size */
#define LOG_THREAD_STACK_SIZE                   256
#endif /* LOG_THREAD_STACK_SIZE */

#ifndef LOG_THREAD_PRIO
/** Log thread priority */
#define LOG_THREAD_PRIO                         25
#endif /* LOG_THREAD_PRIO */

/** Log memory allocator */
#ifndef LOG_MALLOC
#define LOG_MALLOC                              pvPortMalloc
#endif /* LOG_MALLOC */

/** Log memory deallocator */
#ifndef LOG_FREE
#define LOG_FREE                                vPortFree
#endif /* LOG_FREE */

/** @} */

/* Private typedef -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Logging_Types ST67W6X Utility Logging Types
  * @ingroup  ST67W6X_Utilities_Logging
  * @{
  */

/**
  * @brief  Logging message structure
  */
typedef struct
{
  uint32_t message_id;    /*!< Log id. must be unique per queue producer */
  uint32_t message_len;   /*!< Length of the message */
  char *message;          /*!< Pointer to the message */
} LogMessage_t;

/**
  * @brief  LoggingConfig_t structure definition
  */
typedef struct
{
  uint32_t VerboseLogLevel;                 /*!< Log level */
  uint32_t allocated_log_mem;               /*!< Allocated log memory */
  void (*log_output)(const char *message);  /*!< Log output callback */
} LoggingConfig_t;

/** @} */

/*Private variables -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Logging_Variables ST67W6X Utility Logging Variables
  * @ingroup  ST67W6X_Utilities_Logging
  * @{
  */

/**
  * @brief  LogMessage_t structure definition
  */
static QueueHandle_t xLogQueue = NULL;

/**
  * @brief  LogMessage_t structure definition
  */
static TaskHandle_t OutputTaskHandle;

/**
  * @brief  LogMessage_t structure definition
  */
static const char *logLevelStrings[] =
{
  "NONE",
  "ERROR",
  "WARN",
  "INFO",
  "DEBUG"
};

/**
  * @brief  LoggingConfig_t structure definition
  */
static LoggingConfig_t LoggingConfig =
{
  .VerboseLogLevel = LOG_LEVEL,
  .log_output = NULL,
  .allocated_log_mem = 0
};

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Logging_Functions ST67W6X Utility Logging Functions
  * @ingroup  ST67W6X_Utilities_Logging
  * @{
  */

/**
  * @brief  Logging print task pushing the log message in the queue to the output callback.
  * @param  pvParameters: Task parameter
  */
static void OutputTask(void *pvParameters);

/* Functions Definition ------------------------------------------------------*/
void *vLoggingInit(void (*LogOutput)(const char *message))
{
  BaseType_t status;
  /* default setting */
  LoggingConfig.VerboseLogLevel = LOG_DEBUG;
  LoggingConfig.allocated_log_mem = 0;

  xLogQueue = NULL;

  if (LogOutput != NULL)
  {
    LoggingConfig.log_output = LogOutput;
    xLogQueue = xQueueCreate(MAX_LOG_QUEUE_LENGTH, sizeof(LogMessage_t *));
    if (xLogQueue == NULL)
    {
      printf("Log queue creation failed\n");
      return NULL;
    }
    status = xTaskCreate(OutputTask, "OutputTask", LOG_THREAD_STACK_SIZE >> 2, NULL,
                         LOG_THREAD_PRIO, &OutputTaskHandle);
    if (status != pdPASS)
    {
      vQueueDelete(xLogQueue);
      xLogQueue = NULL;
      printf("Log task creation failed\n");
      return NULL;
    }
  }
  return xLogQueue;
}

void vLoggingSetVerbosity(uint32_t level)
{
  if (level <= MAX_LOG_LEVEL)
  {
    LoggingConfig.VerboseLogLevel  = level;
  }
}

int32_t vLoggingPrintf(uint32_t log_level, const uint8_t metadata_print, const uint32_t line_number,
                       const char *const p_file_name, const char *const p_format, ...)
{
  int32_t offset = 0;
  va_list args;
  char log_include_str[LOG_INCLUDE_MAX_LENGTH] = {0};
  char dummy_str[2]; /* dummy string for format length calculation */

  if ((log_level > LoggingConfig.VerboseLogLevel) || (log_level > MAX_LOG_LEVEL))
  {
    return -1;  /* Filter out messages above the threshold */
  }

  if (LoggingConfig.allocated_log_mem > MAX_LOG_MEMORY)
  {
    printf("Max log malloc reached\n");
    return 0;
  }

  /* Allocate memory for the log structure */
  LogMessage_t *newLog = (LogMessage_t *)LOG_MALLOC(sizeof(LogMessage_t));
  if (newLog == NULL)
  {
    printf("Log malloc failed\n");
    return 0;
  }

  va_start(args, p_format);

  if (metadata_print)
  {
    offset += snprintf(log_include_str, LOG_INCLUDE_MAX_LENGTH, "[%s] ", logLevelStrings[log_level]);

#if LOG_INCLUDE_TIMESTAMP
    /* Get the current timestamp and task name (if required) */
    uint32_t timestamp = xTaskGetTickCount();
    offset += snprintf(&log_include_str[offset], LOG_INCLUDE_MAX_LENGTH - offset, "[%" PRIu32 "] ", timestamp);
#endif /* LOG_INCLUDE_TIMESTAMP */

#if LOG_INCLUDE_TASKNAME
    const char *taskName = pcTaskGetName(xTaskGetCurrentTaskHandle());
    if ((taskName) && (offset < LOG_INCLUDE_MAX_LENGTH))
    {
      offset += snprintf(&log_include_str[offset], LOG_INCLUDE_MAX_LENGTH - offset, "[%s] ", taskName);
    }
#endif /* LOG_INCLUDE_TASKNAME */

#if LOG_INCLUDE_FILENAME
    if ((offset < LOG_INCLUDE_MAX_LENGTH) && (p_file_name != NULL) && (line_number != 0))
    {
      /* Extract the filename if p_file_name contains a full path */
      char *p_file = (char *)p_file_name;
      char *lastSlash = strrchr(p_file, '/');
      if (lastSlash != NULL)
      {
        p_file = lastSlash + 1; /* Move past the '/' */
      }
      else
      {
        lastSlash = strrchr(p_file, '\\');
        if (lastSlash != NULL)
        {
          p_file = lastSlash + 1; /* Move past the '\\' */
        }
      }

      offset += snprintf(&log_include_str[offset], LOG_INCLUDE_MAX_LENGTH - offset,
                         "(%s:%" PRIu32 ") ",
                         p_file, line_number);
    }
#endif /* LOG_INCLUDE_FILENAME */
  }

  configASSERT(offset < LOG_INCLUDE_MAX_LENGTH)

  /* calculate the log message size */
  int32_t alloc_len = vsnprintf(dummy_str, sizeof(dummy_str), p_format, args); /*Get format length */
  alloc_len += offset + 1; /*offset for log include string + 1 for \0 */

  /* max alloc_len is MAX_LOG_MESSAGE_LENGTH */
  if (alloc_len > MAX_LOG_MESSAGE_LENGTH)
  {
    alloc_len = MAX_LOG_MESSAGE_LENGTH;
  }

  /* Allocate memory for the log message */
  newLog->message = (char *)LOG_MALLOC(alloc_len);
  if (newLog->message == NULL)
  {
    LOG_FREE(newLog);
    printf("Log message malloc failed\n");
    va_end(args);
    return 0;
  }

  /* track log memory allocation */
  LoggingConfig.allocated_log_mem += alloc_len;
  newLog->message_len = alloc_len;

  strncpy(newLog->message, log_include_str, offset);

  offset += vsnprintf(newLog->message + offset, alloc_len, p_format, args);

  va_end(args);

  newLog->message[offset++] = '\0';

  newLog->message_id = LOG_MESSAGE_ID;

  /* Send the log to the queue */
  if (xQueueSendToBack(xLogQueue, &newLog, 0) != pdPASS)
  {
    LOG_FREE(newLog->message);
    LOG_FREE(newLog);
    printf("Log queue full, message dropped\n");
  }
  return offset;
}

void vLoggingDeInit(void)
{
  if (OutputTaskHandle != NULL)
  {
    vTaskDelete(OutputTaskHandle);
  }

  if (xLogQueue != NULL)
  {
    /* Check Queue empty to clean? */
    vQueueDelete(xLogQueue);
  }
}
/* Private Functions Definition ----------------------------------------------*/
static void OutputTask(void *pvParameters)
{
  LogMessage_t *receivedLog = NULL;

  for (;;)
  {
    /* Wait for a log message from the queue */
    if (xQueueReceive(xLogQueue, &receivedLog, portMAX_DELAY) == pdPASS)
    {
      if ((receivedLog) && (receivedLog->message != 0))
      {
        if (LoggingConfig.log_output != NULL)
        {
          LoggingConfig.log_output(receivedLog->message);
          fflush(stdout);
        }
        /* Free the memory after transfer completes */
        LOG_FREE(receivedLog->message);
        LOG_FREE(receivedLog);

        if ((receivedLog->message_id == LOG_MESSAGE_ID) &&
            (LoggingConfig.allocated_log_mem >= receivedLog->message_len))
        {
          LoggingConfig.allocated_log_mem -= receivedLog->message_len;
        }
      }
    }
  }
}

/** @} */
