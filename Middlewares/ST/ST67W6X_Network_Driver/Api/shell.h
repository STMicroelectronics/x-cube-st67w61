/**
  ******************************************************************************
  * @file    shell.h
  * @author  ST67 Application Team
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
  * Portions of this file are based on RT-Thread Development Team,
  * which is licensed under the Apache-2.0 license as indicated below.
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
#ifndef SHELL_H
#define SHELL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdbool.h>
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

/** @brief  Interrupt signal */
#define SHELL_SIGINT                1

/** @brief  Default action */
#define SHELL_SIG_DFL               ((shell_sig_func_ptr)0)

/** @brief  Ignore action */
#define SHELL_SIG_IGN               ((shell_sig_func_ptr)1)

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
  WAIT_EXT_KEY,   /*!< wait extended key */
};

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
  cmd_function_t func; /*!< the function address of system call */
} shell_syscall_t;

/**
  * @brief  Shell structure
  */
struct shell
{
  /** shell status */
  enum input_stat stat;
  /** shell overwrite mode flag */
  uint8_t overwrite_mode;
  /** shell control mode flag */
  uint8_t control_mode;
  /** shell history command position */
  uint16_t current_history;
  /** shell history command count */
  uint16_t history_count;
  /** shell history command buffer */
  char cmd_history[SHELL_HISTORY_LINES][SHELL_CMD_SIZE];
  /** shell command buffer */
  char line[SHELL_CMD_SIZE + 1];
  /** shell cursor position on the line */
  uint16_t line_position;
  /** shell current position on the line */
  uint16_t line_curpos;
  /** shell prompt */
  char *prompt_custom;
  /** shell printf function */
  void (*shell_printf_fn)(char *fmt, ...);
  /** shell start of system call table */
  shell_syscall_t *syscall_table_begin;
  /** shell end of system call table */
  shell_syscall_t *syscall_table_end;
};

/**
  * @brief  Shell queue internal typedef
  */
typedef struct QueueDefinition shell_queue_def_t;

/**
  * @brief  Shell queue handle typedef
  */
typedef shell_queue_def_t *shell_queue_handle_t;

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
#define SHELL_PRINT_NOCOLOR(fmt, ...)                     \
  do {                                                    \
    struct shell *shell_hdl = shell_get_instance();       \
    (void)shell_hdl->shell_printf_fn(fmt, ##__VA_ARGS__); \
    SHELL_FLUSH_OUT;                                      \
  } while(false)

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
#define SHELL_COLOR_HDR(n) shell_hdl->shell_printf_fn("\033[" #n "m")
/** End colorized output */
#define SHELL_COLOR_END    shell_hdl->shell_printf_fn("\033[0m")

/** Print output with color */
#define SHELL_PRINT(color_n, fmt, ...)                    \
  do {                                                    \
    struct shell *shell_hdl = shell_get_instance();       \
    SHELL_COLOR_HDR(color_n);                             \
    (void)shell_hdl->shell_printf_fn(fmt, ##__VA_ARGS__); \
    SHELL_FLUSH_OUT;                                      \
    SHELL_COLOR_END;                                      \
  } while(false)

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

/**
  * \def SHELL_CMD_EXPORT_ALIAS( ... )
  * @brief  This macro exports a command to module shell
  * @param  name the name of command
  * @param  cmd the alias of command
  * @param  desc the description of command, which will show in help
  */

#if (SHELL_ENABLE == 1)
/**
  * \def SHELL_CMD_EXPORT_ALIAS_WITH_DESC( ... )
  * @brief  This macro exports a command to module shell with description
  * @param  name the name of command
  * @param  cmd the alias of command
  * @param  desc_str the description of command, which will show in help
  */
#if (SHELL_USING_DESCRIPTION == 1)
#define SHELL_CMD_EXPORT_ALIAS_WITH_DESC(name, cmd, desc_str)                                           \
  const char fsym_##cmd##_name[] __attribute__((section(".rodata.name"))) = #cmd;                       \
  const char fsym_##cmd##_desc[] __attribute__((section(".rodata.name"))) = desc_str;                   \
  __attribute__((used)) const struct shell_syscall fsym_##cmd __attribute__((section("FSymTab"))) =     \
      { \
        (fsym_##cmd##_name),                                                                            \
        fsym_##cmd##_desc,                                                                              \
        (cmd_function_t)&(name)                                                                         \
      };
#define SHELL_CMD_EXPORT_ALIAS(name, cmd, desc) SHELL_CMD_EXPORT_ALIAS_WITH_DESC(name, cmd, #desc)
#else
#define SHELL_CMD_EXPORT_ALIAS_WITH_DESC(name, cmd, desc_str)                                           \
  const char fsym_##cmd##_name[] __attribute__((section(".rodata.name"))) = #cmd;                       \
  __attribute__((used)) const struct shell_syscall fsym_##cmd __attribute__((section("FSymTab"))) =     \
      { \
        (fsym_##cmd##_name),                                                                            \
        (cmd_function_t)&(name)                                                                         \
      };
#define SHELL_CMD_EXPORT_ALIAS(name, cmd, desc) SHELL_CMD_EXPORT_ALIAS_WITH_DESC(name, cmd, "")
#endif /* SHELL_USING_DESCRIPTION */
#else  /* SHELL_ENABLE */
#define SHELL_CMD_EXPORT_ALIAS(name, cmd, desc)
#endif /* SHELL_ENABLE */

/** @} */

/* Exported functions ------------------------------------------------------- */
/** @addtogroup ST67W6X_Utilities_Shell_Functions
  * @{
  */

/**
  * @brief  Initialize the shell based on FreeRTOS. It creates a task that read the input chars from the uart and parse
  *         the input using the upper layer shell API. The output of the shell is sent to the stream buffer ::tx_stream.
  * @param  shell_queue [IN] specifies the queue where to send all the output produced by the shell
  */
void shell_freertos_init(shell_queue_handle_t shell_queue);

/**
  * @brief  Deinitialize the shell. It stops the shell task and release all the resources used by the shell
  */
void shell_freertos_deinit(void);

/**
  * @brief  Callback called by the uart ISR when a new byte is received
  * @param  uart_rxbyte [IN] specifies the byte received.
  */
void shell_freertos_on_new_data(uint8_t uart_rxbyte);

/**
  * @brief  Get the shell instance
  * @return shell instance
  */
struct shell *shell_get_instance(void);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SHELL_H */
