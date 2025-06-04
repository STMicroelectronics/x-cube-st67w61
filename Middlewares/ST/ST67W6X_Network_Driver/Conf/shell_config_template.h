/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    shell_config_template.h
  * @author  GPM Application Team
  * @brief   Header file for the W6X Shell configuration module
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
#ifndef SHELL_CONFIG_TEMPLATE_H
#define SHELL_CONFIG_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/*
 * All available configuration defines can be found in
 * Middlewares\ST\ST67W6X_Network_Driver\Conf\shell_config_template.h
 */

/** Enable the shell component */
#define SHELL_ENABLE                            1

/** Default shell prompt string */
#define SHELL_DEFAULT_NAME                      "w61"

/** Console buffer size */
#define SHELL_CONSOLEBUF_SIZE                   128

/** Shell prompt maximum size */
#define SHELL_PROMPT_SIZE                       20

/** Shell history maximum lines */
#define SHELL_HISTORY_LINES                     5

/** Shell command maximum size */
#define SHELL_CMD_SIZE                          120

/** Shell argument maximum number */
#define SHELL_ARG_NUM                           16

/** Shell using color */
#define SHELL_USING_COLOR                       1

/** Print an additional status message at the end of the command execution */
#define SHELL_PRINT_STATUS                      0

/** Maximum length of the string to be printed */
#define SHELL_FREERTOS_MAX_PRINT_STRING_LENGTH  2000

/** Shell receive buffer size */
#define SHELL_FREERTOS_RX_BUFF_SIZE             512

/** Shell receive thread stack size */
#define SHELL_FREERTOS_RX_THREAD_STACK_SIZE     3072

/** Shell receive thread priority */
#define SHELL_FREERTOS_RX_THREAD_PRIO           5

/** Flush the output */
#define SHELL_FLUSH_OUT                         do { fflush(stdout); } while(0)

/** Enable the shell command description */
#define SHELL_USING_DESCRIPTION                 1

/** Maximum number of characters to compare in the help command */
#define SHELL_HELP_MAX_COMPARED_NB_CHAR         10

/* Memory allocator */
/** Shell memory allocator */
#define SHELL_MALLOC                            pvPortMalloc

/** Shell memory deallocator */
#define SHELL_FREE                              vPortFree

/* To remap the log message of the shell into the logging system uncomment the following lines */
/*
 * #include "logging.h"
 * #define SHELL_LOG                               LogInfo
 */

/* USER CODE BEGIN EC */

/* USER CODE END EC */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SHELL_CONFIG_TEMPLATE_H */
