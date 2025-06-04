/**
  ******************************************************************************
  * @file    shell.h
  * @author  GPM Application Team
  * @brief   This file provides the definition of the Shell API
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
  * Portions of this file are based on FreeRTOS, which is licensed under the Apache-2.0 license as indicated below.
  * See https://github.com/RT-Thread/rt-thread for more information.
  *
  * Reference source:
  * https://github.com/RT-Thread/rt-thread/blob/master/components/finsh/shell.h
  */

/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-06-02     Bernard      Add finsh_get_prompt function declaration
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SHELL_H__
#define __SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "shell_default_config.h"

/* Exported constants --------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Shell_Constants ST67W6X Utility Shell Constants
  * @ingroup  ST67W6X_Utilities_Shell
  * @{
  */

/** @brief  Shell no command found */
#define SHELL_STATUS_NO_COMMAND     -255

/** @brief  Shell unknown arguments */
#define SHELL_STATUS_UNKNOWN_ARGS   -254

/** @brief  Shell error code */
#define SHELL_STATUS_ERROR          -1

/** @brief  Shell success code */
#define SHELL_STATUS_OK             0

/** @} */

/* Exported types ------------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Shell_Types ST67W6X Utility Shell Types
  * @ingroup  ST67W6X_Utilities_Shell
  * @{
  */

/** shell input status */
enum input_stat
{
  WAIT_NORMAL,    /*!< normal status */
  WAIT_SPEC_KEY,  /*!< wait special key */
  WAIT_FUNC_KEY,  /*!< wait function key */
};

/** shell signal function typedef */
typedef void (*shell_sig_func_ptr)(int32_t);

/** shell function system call typedef */
typedef int32_t (*syscall_func)(void);

/** shell command function typedef */
typedef int32_t (*cmd_function_t)(int32_t argc, char **argv);

/**
  * @brief  System call table
  */
typedef struct shell_syscall
{
  const char *name; /*!< the name of system call */
#if (SHELL_USING_DESCRIPTION == 1)
  const char *desc; /*!< description of system call */
#endif /* SHELL_USING_DESCRIPTION */
  syscall_func func; /*!< the function address of system call */
} shell_syscall_t;

/**
  * @brief  System variable table
  */
typedef struct shell_sysvar
{
  const char *name; /*!< the name of variable */
#if (SHELL_USING_DESCRIPTION == 1)
  const char *desc; /*!< description of system variable */
#endif /* SHELL_USING_DESCRIPTION */
  uint8_t type; /*!< the type of variable */
  void *var;    /*!< the address of variable */
} shell_sysvar_t;

/**
  * @brief  shell structure
  */
struct shell
{
  /** shell status */
  enum input_stat stat;
  /** shell history command position */
  uint16_t current_history;
  /** shell history command count */
  uint16_t history_count;
  /** shell history command buffer */
  char cmd_history[SHELL_HISTORY_LINES][SHELL_CMD_SIZE];
  /** shell command buffer */
  char line[SHELL_CMD_SIZE];
  /** shell cursor position on the line */
  uint16_t line_position;
  /** shell current position on the line */
  uint16_t line_curpos;
  /** shell prompt */
  char *prompt_custom;
  /** shell printf function */
  void (*shell_printf)(char *fmt, ...);
  /** shell signal function */
  volatile shell_sig_func_ptr shell_sig_func;
  /** shell start of system call table */
  shell_syscall_t *syscall_table_begin;
  /** shell end of system call table */
  shell_syscall_t *syscall_table_end;
  /** shell start of system variable table */
  shell_sysvar_t *sysvar_table_begin;
  /** shell end of system variable table */
  shell_sysvar_t *sysvar_table_end;
};

/** @} */

/* Exported variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Shell_Macros ST67W6X Utility Shell Macros
  * @ingroup  ST67W6X_Utilities_Shell
  * @{
  */

/**
  * \def SHELL_PRINTF( ... )
  * Print standard output without color.
  */

/**
  * \def SHELL_PROMPT( ... )
  * Print prompt message with cyan color or without color according to the SHELL_USING_COLOR configuration.
  */

/**
  * \def SHELL_DBG( ... )
  * Print debug message with green color or without color according to the SHELL_USING_COLOR configuration.
  */

/**
  * \def SHELL_CMD( ... )
  *  Print command with yellow color or without color according to the SHELL_USING_COLOR configuration.
  */

/**
  * \def SHELL_E( ... )
  * Print error message with red color or without color according to the SHELL_USING_COLOR configuration.
  */

/** Print output without color */
#define SHELL_PRINT_NOCOLOR(fmt, ...)           \
  do {                                          \
    struct shell *shell = shell_get_instance(); \
    shell->shell_printf(fmt, ##__VA_ARGS__);    \
    SHELL_FLUSH_OUT;                            \
  } while (0)

#ifndef SHELL_PRINTF
#define SHELL_PRINTF(fmt, ...) SHELL_PRINT_NOCOLOR(fmt, ##__VA_ARGS__)
#endif /* SHELL_PRINTF */

#if (SHELL_USING_COLOR == 1)
/*
 * The color for terminal (foreground)
 * BLACK    30
 * RED      31
 * GREEN    32
 * YELLOW   33
 * BLUE     34
 * PURPLE   35
 * CYAN     36
 * WHITE    37
 */
/** Start colorized output */
#define _SHELL_COLOR_HDR(n) shell->shell_printf("\033[" #n "m")
/** End colorized output */
#define _SHELL_COLOR_END    shell->shell_printf("\033[0m")

/** Print output with color */
#define SHELL_PRINT(color_n, fmt, ...)          \
  do {                                          \
    struct shell *shell = shell_get_instance(); \
    _SHELL_COLOR_HDR(color_n);                  \
    shell->shell_printf(fmt, ##__VA_ARGS__);    \
    SHELL_FLUSH_OUT;                            \
    _SHELL_COLOR_END;                           \
  } while (0)

#define SHELL_PROMPT(fmt, ...) SHELL_PRINT(36, fmt, ##__VA_ARGS__)

#define SHELL_DBG(fmt, ...)    SHELL_PRINT(32, fmt, ##__VA_ARGS__)

#define SHELL_CMD(fmt, ...)    SHELL_PRINT(33, fmt, ##__VA_ARGS__)

#define SHELL_E(fmt, ...)      SHELL_PRINT(31, fmt, ##__VA_ARGS__)
#else /* SHELL_USING_COLOR */

#define SHELL_PROMPT(fmt, ...) SHELL_PRINT_NOCOLOR(fmt, ##__VA_ARGS__)

#define SHELL_DBG(fmt, ...)    SHELL_PRINT_NOCOLOR(fmt, ##__VA_ARGS__)

#define SHELL_CMD(fmt, ...)    SHELL_PRINT_NOCOLOR(fmt, ##__VA_ARGS__)

#define SHELL_E(fmt, ...)      SHELL_PRINT_NOCOLOR(fmt, ##__VA_ARGS__)
#endif /* SHELL_USING_COLOR */

#if (SHELL_ENABLE == 1)
#if (SHELL_USING_DESCRIPTION == 1)
/** Export a command to module shell with description */
#define SHELL_FUNCTION_EXPORT_CMD(name, cmd, desc)                                                      \
  const char __fsym_##cmd##_name[] __attribute__((section(".rodata.name"))) = #cmd;                     \
  const char __fsym_##cmd##_desc[] __attribute__((section(".rodata.name"))) = #desc;                    \
  __attribute__((used)) const struct shell_syscall __fsym_##cmd __attribute__((section("FSymTab"))) =   \
      { \
        __fsym_##cmd##_name,                                                                            \
        __fsym_##cmd##_desc,                                                                            \
        (syscall_func)&name                                                                             \
      };

#else /* SHELL_USING_DESCRIPTION */
/** Export a command to module shell */
#define SHELL_FUNCTION_EXPORT_CMD(name, cmd, desc)                                                      \
  const char __fsym_##cmd##_name[] = #cmd;                                                              \
  __attribute__((used)) const struct shell_syscall __fsym_##cmd __attribute__((section("FSymTab"))) =   \
      { \
        __fsym_##cmd##_name,                                                                            \
        (syscall_func)&name                                                                             \
      };
#endif /* SHELL_USING_DESCRIPTION */
#else  /* SHELL_ENABLE */
/** weak command export */
#define SHELL_FUNCTION_EXPORT_CMD(name, cmd, desc)
#endif /* SHELL_ENABLE */

/**
  * @brief  This macro exports a command to module shell.
  * @param  command the name of command.
  * @param  alias the alias of command.
  * @param  desc the description of command, which will show in help.
  */
#define SHELL_CMD_EXPORT_ALIAS(command, alias, desc) \
  SHELL_FUNCTION_EXPORT_CMD(command, alias, desc)

/** @} */

/* Exported functions ------------------------------------------------------- */
/** @addtogroup ST67W6X_Utilities_Shell_Functions
  * @{
  */

/**
  * @brief  Initialize the shell based on FreeRTOS. It creates a task that read the input chars from the uart and parse
  *         the input using the upper layer shell API. The output of the shell is sent to the stream buffer ::tx_stream.
  * @param  p_rx_uart [IN] specifies the uart used directly by the shell to read the user input.
  * @param  xLogQueue [IN] specifies the queue where to send all the output produced by the shell.
  */
void shell_freertos_init(UART_HandleTypeDef *p_rx_uart, void *xLogQueue);

/**
  * @brief  Deinitialize the shell. It stops the shell task and release all the resources used by the shell.
  */
void shell_freertos_deinit(void);

/**
  * @brief  Get the shell instance
  * @return shell instance
  */
struct shell *shell_get_instance(void);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SHELL_H__ */
