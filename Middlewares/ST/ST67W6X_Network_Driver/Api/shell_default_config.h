/**
  ******************************************************************************
  * @file    shell_default_config.h
  * @author  GPM Application Team
  * @brief   This file provides default configuration for the Shell component
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
#ifndef SHELL_DEFAULT_CONFIG_H
#define SHELL_DEFAULT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "shell_config.h"

/* Exported constants --------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Shell_Constants
  * @{
  */

#ifndef SHELL_ENABLE
/** Enable the shell component */
#define SHELL_ENABLE                               1
#endif /* SHELL_ENABLE */

#ifndef SHELL_DEFAULT_NAME
/** Default shell prompt string */
#define SHELL_DEFAULT_NAME                         "w61"
#endif /* SHELL_DEFAULT_NAME */

#ifndef SHELL_CONSOLEBUF_SIZE
/** Console buffer size */
#define SHELL_CONSOLEBUF_SIZE                      128
#endif /* SHELL_CONSOLEBUF_SIZE */

#ifndef SHELL_PROMPT_SIZE
/** Shell prompt maximum size */
#define SHELL_PROMPT_SIZE                          20
#endif /* SHELL_PROMPT_SIZE */

#ifndef SHELL_HISTORY_LINES
/** Shell history maximum lines */
#define SHELL_HISTORY_LINES                        5
#endif /* SHELL_HISTORY_LINES */

#ifndef SHELL_CMD_SIZE
/** Shell command maximum size */
#define SHELL_CMD_SIZE                             120
#endif /* SHELL_CMD_SIZE */

#ifndef SHELL_ARG_NUM
/** Shell argument maximum number */
#define SHELL_ARG_NUM                              16
#endif /* SHELL_ARG_NUM */

#ifndef SHELL_USING_COLOR
/** Enable the shell color */
#define SHELL_USING_COLOR                          1
#endif /* SHELL_USING_COLOR */

#ifndef SHELL_PRINT_STATUS
/** Print an additional status message at the end of the command execution */
#define SHELL_PRINT_STATUS                         0
#endif /* SHELL_PRINT_STATUS */

#ifndef SHELL_FREERTOS_MAX_PRINT_STRING_LENGTH
/** Maximum length of the string to be printed */
#define SHELL_FREERTOS_MAX_PRINT_STRING_LENGTH     2000
#endif /* SHELL_FREERTOS_MAX_PRINT_STRING_LENGTH */

#ifndef SHELL_FREERTOS_RX_BUFF_SIZE
/** Shell receive buffer size */
#define SHELL_FREERTOS_RX_BUFF_SIZE                128
#endif /* SHELL_FREERTOS_RX_BUFF_SIZE */

#ifndef SHELL_FREERTOS_RX_THREAD_STACK_SIZE
/** Shell receive thread stack size */
#define SHELL_FREERTOS_RX_THREAD_STACK_SIZE        2048
#endif /* SHELL_FREERTOS_RX_THREAD_STACK_SIZE */

#ifndef SHELL_FREERTOS_RX_THREAD_PRIO
/** Shell receive thread priority */
#define SHELL_FREERTOS_RX_THREAD_PRIO              5
#endif /* SHELL_FREERTOS_RX_THREAD_PRIO */

#ifndef SHELL_FLUSH_OUT
/** Flush the output */
#define SHELL_FLUSH_OUT                            do { fflush(stdout); } while(0)
#endif /* SHELL_FLUSH_OUT */

#ifndef SHELL_USING_DESCRIPTION
/** Enable the shell command description */
#define SHELL_USING_DESCRIPTION                    1
#endif /* SHELL_USING_DESCRIPTION */

#ifndef SHELL_HELP_MAX_COMPARED_NB_CHAR
/** Maximum number of characters to compare in the help command */
#define SHELL_HELP_MAX_COMPARED_NB_CHAR            10
#endif /* SHELL_HELP_MAX_COMPARED_NB_CHAR */

#ifndef SHELL_LOG
/** Print log message */
#define SHELL_LOG                                  SHELL_DBG
#endif /* SHELL_LOG */

/* Memory allocator */
#ifndef SHELL_MALLOC
/** Shell memory allocator */
#define SHELL_MALLOC                               pvPortMalloc
#endif /* SHELL_MALLOC */

#ifndef SHELL_FREE
/** Shell memory deallocator */
#define SHELL_FREE                                 vPortFree
#endif /* SHELL_FREE */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SHELL_DEFAULT_CONFIG_H */
