/**
  ******************************************************************************
  * @file    shell.c
  * @author  GPM Application Team
  * @brief   This file is part of the shell module.
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
  * https://github.com/RT-Thread/rt-thread/blob/master/components/finsh/shell.c
  */

/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-04-30     Bernard      the first version for FinSH
 * 2006-05-08     Bernard      change finsh thread stack to 2048
 * 2006-06-03     Bernard      add support for skyeye
 * 2006-09-24     Bernard      remove the code related with hardware
 * 2010-01-18     Bernard      fix down then up key bug.
 * 2010-03-19     Bernard      fix backspace issue and fix device read in shell.
 * 2010-04-01     Bernard      add prompt output when start and remove the empty history
 * 2011-02-23     Bernard      fix variable section end issue of finsh shell
 *                             initialization when use GNU GCC compiler.
 * 2016-11-26     armink       add password authentication
 * 2018-07-02     aozima       add custom prompt support.
 */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"

#include "shell_internal.h"
#include "shell.h"

extern void shell_abort_exec(int32_t sig);
extern int32_t shell_start_exec(cmd_function_t func, int32_t argc, char *argv[]);
extern void shell_dup_line(char *cmd, uint32_t length);

/* Exported constants --------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Shell_Constants
  * @{
  */

#define SHELL_SIGINT  1 /*!< Interrupt signal */
#define SHELL_SIG_DFL ((shell_sig_func_ptr)0) /*!< Default action */
#define SHELL_SIG_IGN ((shell_sig_func_ptr)1) /*!< Ignore action */

/** @} */

/* Exported macros -----------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Shell_Macros
  * @{
  */

/** Macro to get the minimum of two values */
#define MIN( a, b )    ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Shell_Functions
  * @{
  */

/**
  * @brief  Set the print function of shell
  * @param  shell_printf the print function
  * @return 0 on success, -1 on error
  */
int32_t shell_set_print(void (*shell_printf)(char *fmt, ...));

/**
  * @brief  Set the prompt of shell
  * @param  prompt the prompt string
  * @return 0 on success, -1 on error
  */
int32_t shell_set_prompt(const char *prompt);

/**
  * @brief  Get the shell prompt
  * @return the shell prompt
 */
static char *shell_get_prompt(void);

/**
  * @brief  Compare two strings
  * @param  str1 the first string
  * @param  str2 the second string
  * @return the common length of the two strings
 */
static int32_t str_common(const char *str1, const char *str2);

/**
  * @brief  Print the shell history
  * @param  pShell the shell instance
 */
static void shell_handle_history(struct shell *pShell);

/**
  * @brief  Push the command into the shell history
  * @param  shell the shell instance
 */
static void shell_push_history(struct shell *shell);

/**
  * @brief  Auto complete the command
  * @param  prefix the prefix of the command
 */
static void shell_auto_complete(char *prefix);

/**
  * @brief  Split the command string
  * @param  cmd the command string
  * @param  length the length of the command string
  * @param  argv the argument list
  * @return the argument count
 */
static int32_t shell_split(char *cmd, uint32_t length, char *argv[SHELL_ARG_NUM]);

/**
  * @brief  Get the command function
  * @param  cmd the command string
  * @param  size the size of the command string
  * @return the command function
 */
static cmd_function_t shell_get_cmd(char *cmd, int32_t size);

/**
  * @brief  Execute built-in command
  * @param  cmd the command string
  * @param  length the length of the command string
  * @param  retp the return value of the command
  * @return 0 on success, -1 on error
 */
static int32_t shell_exec_cmd(char *cmd, uint32_t length, int32_t *retp);

/**
  * @brief  Execute the command
  * @param  cmd the command string
  * @param  length the length of the command string
  * @return the return value of the command
  * @note   This function is used to execute the command
  */
int32_t shell_exec(char *cmd, uint32_t length);

#if (SHELL_ENABLE == 1)
/**
  * @brief  Initialize the shell function
  * @param  begin the start address of the shell function table
  * @param  end the end address of the shell function table
 */
static void shell_function_init(const void *begin, const void *end);
#endif /* SHELL_ENABLE */

/**
  * @brief  This function will execute the shell signal
  * @param  sig the signal (SHELL_SIGINT)
  * @param  func the signal function
  * @return the previous signal function
  */
shell_sig_func_ptr shell_signal(int32_t sig, shell_sig_func_ptr func);

/**
  * @brief  Display all the available commands and the relative help message.
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0
  */
int32_t shell_help(int32_t argc, char **argv);

#if 0 /* NOT SUPPORTED */
/**
  * @brief  Read / Write memory
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0
  */
int32_t shell_memtrace(int32_t argc, char **argv);
#endif /* NOT SUPPORTED */

#if defined(__ICCARM__) || defined(__ICCRX__) /* For IAR Compiler */
size_t strlcpy(char *dst, const char *src, size_t maxlen);
size_t strlcat(char *dst, const char *src, size_t size);
#endif /* __ICCARM__ */

/* Functions Definition ------------------------------------------------------*/
#if defined(__ICCARM__) || defined(__ICCRX__) /* For IAR Compiler */
size_t strlcpy(char *dst, const char *src, size_t maxlen)
{
  const size_t srclen = strlen(src);
  if (srclen + 1 < maxlen)
  {
    memcpy(dst, src, srclen + 1);
  }
  else if (maxlen != 0)
  {
    memcpy(dst, src, maxlen - 1);
    dst[maxlen - 1] = '\0';
  }
  return srclen;
}

size_t strlcat(char *dst, const char *src, size_t size)
{
  register char *d = dst;
  register const char *s = src;
  register size_t rsize = size;
  size_t dlen;

  /* Find the end of dst and adjust bytes left but don't go past end */
  while (rsize-- != 0 && *d != '\0')
  {
    d++;
  }
  dlen = d - dst;
  rsize = size - dlen;

  if (rsize == 0)
  {
    return (dlen + strlen(s));
  }
  while (*s != '\0')
  {
    if (rsize != 1)
    {
      *d++ = *s;
      rsize--;
    }
    s++;
  }
  *d = '\0';

  return (dlen + (s - src)); /* Count does not include NUL */
}
#endif /* __ICCARM__ */

struct shell *shell_get_instance(void)
{
  static struct shell shell =
  {
    .syscall_table_begin = NULL,
    .syscall_table_end = NULL,
    .sysvar_table_begin = NULL,
    .sysvar_table_end = NULL,
    .shell_sig_func = NULL
  };

  return &shell;
}

void shell_handler(uint8_t data)
{
  struct shell *shell = shell_get_instance();
  /**
    * handle control key
    * up key  : 0x1b 0x5b 0x41
    * down key: 0x1b 0x5b 0x42
    * right key:0x1b 0x5b 0x43
    * left key: 0x1b 0x5b 0x44
    */

  if (data == 0x03)
  {
    /*!< ctrl + c */
    if (shell->shell_sig_func)
    {
      shell->shell_sig_func(SHELL_SIGINT);
      shell->shell_sig_func = NULL;
    }
    SHELL_PRINTF("^C");
    data = '\r';
  }

  if (data == 0x1b)
  {
    shell->stat = WAIT_SPEC_KEY;
    return;
  }
  else if (shell->stat == WAIT_SPEC_KEY)
  {
    if (data == 0x5b)
    {
      shell->stat = WAIT_FUNC_KEY;
      return;
    }

    shell->stat = WAIT_NORMAL;
  }
  else if (shell->stat == WAIT_FUNC_KEY)
  {
    shell->stat = WAIT_NORMAL;

    if (data == 0x41) /* Up key */
    {
      /* Prev history */
      if (shell->current_history > 0)
      {
        shell->current_history--;
      }
      else
      {
        shell->current_history = 0;
        return;
      }

      /* Copy the history command */
      memcpy(shell->line, &shell->cmd_history[shell->current_history][0],
             SHELL_CMD_SIZE);
      shell->line_curpos = shell->line_position = strlen(shell->line);
      shell_handle_history(shell);

      return;
    }
    else if (data == 0x42) /* Down key */
    {
      /* Next history */
      if (shell->current_history < shell->history_count - 1)
      {
        shell->current_history++;
      }
      else
      {
        /* Set to the end of history */
        if (shell->history_count != 0)
        {
          shell->current_history = shell->history_count - 1;
        }
        else
        {
          return;
        }
      }

      memcpy(shell->line, &shell->cmd_history[shell->current_history][0],
             SHELL_CMD_SIZE);
      shell->line_curpos = shell->line_position = strlen(shell->line);
      shell_handle_history(shell);

      return;
    }
    else if (data == 0x44) /* Left key */
    {
      if (shell->line_curpos)
      {
        SHELL_PRINTF("\b");
        shell->line_curpos--;
      }

      return;
    }
    else if (data == 0x43) /* Right key */
    {
      if (shell->line_curpos < shell->line_position)
      {
        SHELL_PRINTF("%c", shell->line[shell->line_curpos]);
        shell->line_curpos++;
      }
      return;
    }
  }

  /* Received null or error */
  if ((data == '\0') || (data == 0xFF))
  {
    return;
  }
  /* Handle tab key */
  else if (data == '\t')
  {
    int32_t i;

    /* Move the cursor to the beginning of line */
    for (i = 0; i < shell->line_curpos; i++)
    {
      SHELL_PRINTF("\b");
    }

    /* Auto complete */
    shell_auto_complete(&shell->line[0]);
    /* Re-calculate position */
    shell->line_curpos = shell->line_position = strlen(shell->line);

    return;
  }
  /* Handle backspace key */
  else if (data == 0x7f || data == 0x08)
  {
    /* Note that shell->line_curpos >= 0 */
    if (shell->line_curpos == 0)
    {
      return;
    }

    shell->line_position--;
    shell->line_curpos--;

    if (shell->line_position > shell->line_curpos)
    {
      int32_t i;

      memmove(&shell->line[shell->line_curpos],
              &shell->line[shell->line_curpos + 1],
              shell->line_position - shell->line_curpos);
      shell->line[shell->line_position] = 0;

      SHELL_PRINTF("\b%s  \b", &shell->line[shell->line_curpos]);

      /* Move the cursor to the origin position */
      for (i = shell->line_curpos; i <= shell->line_position; i++)
      {
        SHELL_PRINTF("\b");
      }
    }
    else
    {
      SHELL_PRINTF("\b \b");
      shell->line[shell->line_position] = 0;
    }

    return;
  }

  /* Handle end of line, break */
  if (data == '\r' || data == '\n')
  {
    shell_push_history(shell);

    SHELL_PRINTF("\n");

#if (SHELL_PRINT_STATUS == 1)
    int32_t cmd_ret = shell_exec(shell->line, shell->line_position);
    if (cmd_ret == SHELL_STATUS_OK)
    {
      SHELL_PRINTF("SUCCESS\n");
    }
    else if (cmd_ret != SHELL_STATUS_NO_COMMAND)
    {
      SHELL_PRINTF("ERROR\n");
    }
#else
    shell_exec(shell->line, shell->line_position);
#endif /* SHELL_PRINT_STATUS */

    SHELL_PROMPT(shell_get_prompt());
    memset(shell->line, 0, sizeof(shell->line));
    shell->line_curpos = shell->line_position = 0;
    return;
  }
  /* Return not display character */
  if ((data < 0x20) || (data >= 0x80))
  {
    return;
  }
  /* It's a large line, discard it */
  if (shell->line_position >= SHELL_CMD_SIZE)
  {
    shell->line_position = 0;
  }

  /* Normal character */
  if (shell->line_curpos < shell->line_position)
  {
    int32_t i;

    memmove(&shell->line[shell->line_curpos + 1],
            &shell->line[shell->line_curpos],
            shell->line_position - shell->line_curpos);
    shell->line[shell->line_curpos] = data;

    SHELL_PRINTF("%s", &shell->line[shell->line_curpos]);

    /* Move the cursor to new position */
    for (i = shell->line_curpos; i < shell->line_position; i++)
    {
      SHELL_PRINTF("\b");
    }
  }
  else
  {
    shell->line[shell->line_position] = data;
    shell->shell_printf("%c", data);
    SHELL_FLUSH_OUT;
  }

  shell->line_position++;
  shell->line_curpos++;

  if (shell->line_position >= SHELL_CMD_SIZE)
  {
    /* Clear command line */
    shell->line_position = 0;
    shell->line_curpos = 0;
  }
}

int32_t shell_set_prompt(const char *prompt)
{
  struct shell *shell = shell_get_instance();

  if (shell->prompt_custom)
  {
    SHELL_FREE(shell->prompt_custom);
    shell->prompt_custom = NULL;
  }

  /* Strdup */
  if (prompt)
  {
    shell->prompt_custom = (char *)SHELL_MALLOC(strlen(prompt) + 1);
    if (shell->prompt_custom)
    {
      if (strlcpy(shell->prompt_custom, prompt, strlen(prompt) + 1) >= strlen(prompt) + 1)
      {
        SHELL_LOG("[OS]: strlcpy truncated \n");
      }
    }
  }

  return 0;
}

int32_t shell_set_print(void (*shell_printf)(char *fmt, ...))
{
  if (shell_printf)
  {
    shell_get_instance()->shell_printf = shell_printf;
    return 0;
  }
  else
    return -1;
}

#if (SHELL_ENABLE == 1)
#if defined(__ICCARM__) || defined(__ICCRX__) /* For IAR compiler */
#pragma section="FSymTab"
#endif /* __ICCARM__ */
#endif /* SHELL_ENABLE */

void shell_init(void (*shell_printf)(char *fmt, ...))
{
#if (SHELL_ENABLE == 1)
#if defined(__CC_ARM) || defined(__CLANG_ARM) || defined(__ARMCC_VERSION) /* ARM C Compiler */
  extern const int32_t FSymTab$$Base;
  extern const int32_t FSymTab$$Limit;
  shell_function_init(&FSymTab$$Base, &FSymTab$$Limit);
#elif defined(__ICCARM__) || defined(__ICCRX__) /* For IAR Compiler */
  shell_function_init(__section_begin("FSymTab"), __section_end("FSymTab"));
#elif defined(__GNUC__)
  /* GNU GCC Compiler and TI CCS */
  extern const int32_t __fsymtab_start;
  extern const int32_t __fsymtab_end;
  shell_function_init(&__fsymtab_start, &__fsymtab_end);
#endif /* __CC_ARM */
#endif /* SHELL_ENABLE */
  shell_set_prompt(SHELL_DEFAULT_NAME);
  if (shell_printf == NULL)
  {
    shell_printf = (void (*)(char *fmt, ...))printf;
  }
  shell_set_print(shell_printf);
  SHELL_PRINTF(shell_get_prompt());
}

shell_sig_func_ptr shell_signal(int32_t sig, shell_sig_func_ptr func)
{
  struct shell *p_shell = shell_get_instance();
  shell_sig_func_ptr shell_sig_func_prev = p_shell->shell_sig_func;

  if (sig == SHELL_SIGINT)
  {
    if (func == SHELL_SIG_DFL)
    {
      p_shell->shell_sig_func = shell_abort_exec;
    }
    else if (func == SHELL_SIG_IGN)
    {
      p_shell->shell_sig_func = NULL;
    }
    else
    {
      p_shell->shell_sig_func = func;
    }
    return shell_sig_func_prev;
  }

  return NULL;
}

/* Private Functions Definition ----------------------------------------------*/
static char *shell_get_prompt(void)
{
  static char shell_prompt[SHELL_CONSOLEBUF_SIZE + 1] = { 0 };
  struct shell *shell = shell_get_instance();

  if (shell->prompt_custom)
  {
    if (strlcpy(shell_prompt, shell->prompt_custom,
                sizeof(shell_prompt)) >= sizeof(shell_prompt))
    {
      SHELL_LOG("[OS]: strlcpy truncated \n");
    }
  }
  else
  {
    if (strlcpy(shell_prompt, SHELL_DEFAULT_NAME, sizeof(shell_prompt)) >= sizeof(shell_prompt))
    {
      SHELL_LOG("[OS]: strlcpy truncated \n");
    }
  }
  if (strlcat(shell_prompt, "/>", sizeof(shell_prompt)) >= sizeof(shell_prompt))
  {
    SHELL_LOG("[OS]: strlcat truncated \n");
  }

  return shell_prompt;
}

static int32_t str_common(const char *str1, const char *str2)
{
  const char *str = str1;

  while ((*str != 0) && (*str2 != 0) && (*str == *str2))
  {
    str++;
    str2++;
  }

  return (str - str1);
}

static void shell_handle_history(struct shell *pShell)
{
  SHELL_PRINTF("\033[2K\r");
  SHELL_PROMPT("%s", shell_get_prompt());
  SHELL_PRINTF("%s", pShell->line);
}

static void shell_push_history(struct shell *shell)
{
  if (shell->line_position != 0)
  {
    /* Push history */
    if (shell->history_count >= SHELL_HISTORY_LINES)
    {
      /* If current cmd is same as last cmd, don't push */
      if (memcmp(&shell->cmd_history[SHELL_HISTORY_LINES - 1], shell->line,
                 SHELL_CMD_SIZE))
      {
        /* Move history */
        int32_t index;

        for (index = 0; index < SHELL_HISTORY_LINES - 1; index++)
        {
          memcpy(&shell->cmd_history[index][0],
                 &shell->cmd_history[index + 1][0], SHELL_CMD_SIZE);
        }

        memset(&shell->cmd_history[index][0], 0, SHELL_CMD_SIZE);
        memcpy(&shell->cmd_history[index][0], shell->line,
               shell->line_position);

        /* It's the maximum history */
        shell->history_count = SHELL_HISTORY_LINES;
      }
    }
    else
    {
      /* If current cmd is same as last cmd, don't push */
      if (shell->history_count == 0 ||
          memcmp(&shell->cmd_history[shell->history_count - 1], shell->line,
                 SHELL_CMD_SIZE))
      {
        shell->current_history = shell->history_count;
        memset(&shell->cmd_history[shell->history_count][0], 0, SHELL_CMD_SIZE);
        memcpy(&shell->cmd_history[shell->history_count][0], shell->line,
               shell->line_position);

        /* Increase count and set current history position */
        shell->history_count++;
      }
    }
  }

  shell->current_history = shell->history_count;
}

static void shell_auto_complete(char *prefix)
{
  int32_t length;
  int32_t min_length;
  const char *name_ptr;
  const char *cmd_name;
  struct shell_syscall *index;
  struct shell *p_shell = shell_get_instance();

  min_length = 0;
  name_ptr = NULL;

  SHELL_PRINTF("\n");

  if (*prefix == '\0')
  {
    shell_help(0, NULL);
    return;
  }

  /* Checks in internal command */
  {
    for (index = p_shell->syscall_table_begin; index < p_shell->syscall_table_end; index++)
    {
      cmd_name = (const char *)&index->name[0];

      if (strncmp(prefix, cmd_name, strlen(prefix)) == 0)
      {
        if (min_length == 0)
        {
          /* Set name_ptr */
          name_ptr = cmd_name;
          /* Set initial length */
          min_length = strlen(name_ptr);
        }

        length = str_common(name_ptr, cmd_name);

        if (length < min_length)
        {
          min_length = length;
        }

        SHELL_CMD("%s\n", cmd_name);
      }
    }
  }

  /* Auto complete string */
  if (name_ptr != NULL)
  {
    strlcpy(prefix, name_ptr, min_length + 1);
  }

  SHELL_PROMPT("%s", shell_get_prompt());
  SHELL_PRINTF("%s", prefix);
  return;
}

static int32_t shell_split(char *cmd, uint32_t length, char *argv[SHELL_ARG_NUM])
{
  char *ptr;
  uint32_t position;
  uint32_t argc;
  uint32_t i;

  ptr = cmd;
  position = 0;
  argc = 0;

  while (position < length)
  {
    /* Strip bank and tab */
    while ((*ptr == ' ' || *ptr == '\t') && position < length)
    {
      *ptr = '\0';
      ptr++;
      position++;
    }

    if (argc >= SHELL_ARG_NUM)
    {
      SHELL_E("Too many args ! We only Use:\n");

      for (i = 0; i < argc; i++)
      {
        SHELL_E("%s ", argv[i]);
      }

      SHELL_E("\n");
      break;
    }

    if (position >= length)
    {
      break;
    }

    /* Handle string */
    if (*ptr == '"')
    {
      ptr++;
      position++;
      argv[argc] = ptr;
      argc++;

      /* Skip this string */
      while (*ptr != '"' && position < length)
      {
        if (*ptr == '\\')
        {
          if (*(ptr + 1) == '"')
          {
            ptr++;
            position++;
          }
        }

        ptr++;
        position++;
      }

      if (position >= length)
      {
        break;
      }

      /* Skip '"' */
      *ptr = '\0';
      ptr++;
      position++;
    }
    else
    {
      argv[argc] = ptr;
      argc++;

      while ((*ptr != ' ' && *ptr != '\t') && position < length)
      {
        ptr++;
        position++;
      }

      if (position >= length)
      {
        break;
      }
    }
  }

  return argc;
}

static cmd_function_t shell_get_cmd(char *cmd, int32_t size)
{
  struct shell_syscall *index;
  cmd_function_t cmd_func = NULL;
  struct shell *p_shell = shell_get_instance();

  for (index = p_shell->syscall_table_begin; index < p_shell->syscall_table_end; index++)
  {
    if (strncmp(&index->name[0], cmd, size) == 0 &&
        index->name[0 + size] == '\0')
    {
      cmd_func = (cmd_function_t)index->func;
      break;
    }
  }

  return cmd_func;
}

static int32_t shell_exec_cmd(char *cmd, uint32_t length, int32_t *retp)
{
  int32_t argc;
  uint32_t cmd0_size = 0;
  cmd_function_t cmd_func;
  char *argv[SHELL_ARG_NUM];

  /* Find the size of first command */
  while ((cmd[cmd0_size] != ' ' && cmd[cmd0_size] != '\t') &&
         cmd0_size < length)
  {
    cmd0_size++;
  }

  if (cmd0_size == 0)
  {
    return -1;
  }

  cmd_func = shell_get_cmd(cmd, cmd0_size);

  if (cmd_func == NULL)
  {
    return -1;
  }

  /* Split arguments */
  memset(argv, 0x00, sizeof(argv));
  argc = shell_split(cmd, length, argv);

  if (argc == 0)
  {
    return -1;
  }

  /* Exec this command */
  shell_signal(SHELL_SIGINT, SHELL_SIG_DFL);
  shell_dup_line(cmd, length);
  *retp = shell_start_exec(cmd_func, argc, argv);
  if (*retp == SHELL_STATUS_UNKNOWN_ARGS)
  {
    struct shell_syscall *index;
    struct shell *p_shell = shell_get_instance();

    for (index = p_shell->syscall_table_begin; index < p_shell->syscall_table_end; index++)
    {
      if (strncmp(&index->name[0], cmd, cmd0_size) == 0 &&
          index->name[0 + cmd0_size] == '\0')
      {
        SHELL_E("Unknown argument. Usage: %s\n", index->desc);
        break;
      }
    }
  }

  return 0;
}

int32_t shell_exec(char *cmd, uint32_t length)
{
  int32_t cmd_ret;

  /* Strip the beginning of command */
  while (*cmd == ' ' || *cmd == '\t')
  {
    cmd++;
    length--;
  }

  if (length == 0)
  {
    return SHELL_STATUS_NO_COMMAND;
  }

  /** Exec sequence:
    * 1. built-in command
    * 2. module(if enabled)
    */
  if (shell_exec_cmd(cmd, length, &cmd_ret) == 0)
  {
    return cmd_ret;
  }

  /* Truncate the cmd at the first space. */
  {
    char *tcmd;
    tcmd = cmd;

    while (*tcmd != ' ' && *tcmd != '\0')
    {
      tcmd++;
    }

    *tcmd = '\0';
  }
  SHELL_E("%s: command not found.\n", cmd);
  return -1;
}

#if (SHELL_ENABLE == 1)
static void shell_function_init(const void *begin, const void *end)
{
  struct shell *p_shell = shell_get_instance();
  p_shell->syscall_table_begin = (struct shell_syscall *)begin;
  p_shell->syscall_table_end = (struct shell_syscall *)end;
}
#endif /* SHELL_ENABLE */

int32_t shell_help(int32_t argc, char **argv)
{
  struct shell *p_shell = shell_get_instance();

#if (SHELL_USING_DESCRIPTION == 1)
  if (argc == 1)
  {
    SHELL_PRINTF("shell commands list:\n");
  }
#else
  SHELL_PRINTF("shell commands list:\n");
#endif /* SHELL_USING_DESCRIPTION */
  {
    struct shell_syscall *index;

    for (index = p_shell->syscall_table_begin; index < p_shell->syscall_table_end; index++)
    {
#if (SHELL_USING_DESCRIPTION == 1)
      if ((argc > 1) && (strncmp(index->name, argv[1],
                                 MIN(strlen(argv[1]), SHELL_HELP_MAX_COMPARED_NB_CHAR)) != 0))
      {
        continue;
      }
      SHELL_PRINTF("%-20s - %s\n", &index->name[0], index->desc);
#else
      SHELL_PRINTF("%s\n", &index->name[0]);
#endif /* SHELL_USING_DESCRIPTION */
    }
  }
  SHELL_PRINTF("\n");

  return 0;
}

SHELL_CMD_EXPORT_ALIAS(shell_help, help, help [ command ].
                       Display all available commands and the relative help message);

#if 0 /* NOT SUPPORTED */
int32_t shell_memtrace(int32_t argc, char **argv)
{
  uint32_t addr;
  uint32_t count;
  uint32_t value;
  int32_t i;
  int32_t j;

  /* Check args */
  if ((argc < 3) || (argc > 5))
  {
    SHELL_PRINTF("write memory: 20000000 10 abcd\n");
    SHELL_PRINTF("read memory: 20000000 10\n");
    return 0;
  }

  /* Get address */
  addr = Parser_StrToHex(argv[1], NULL);
  count = Parser_StrToHex(argv[2], NULL);

  /* Display memory content at specified address */
  if (argc == 3)
  {
    for (i = 0; i < count;)
    {
      SHELL_PRINTF("0x%08x ", (addr + 4 * i));
      for (j = 0; j < 4; j++)
      {
        if (i < count)
        {
          /* Read word */
          SHELL_PRINTF("%08x ", *(uint32_t *)(addr + 4 * i));
        }
        else
        {
          break;
        }
        i++;
      }
      SHELL_PRINTF("\n");
    }
  }
  /* Write memory content at specified address */
  if (argc == 4)
  {
    /* Write value */
    value = Parser_StrToHex(argv[3], NULL);

    for (i = 0; i < count; i++)
    {
      *(uint32_t *)(addr + 4 * i) = (uint32_t) value;
    }
    return 0;
  }
  return 0;
}

SHELL_CMD_EXPORT_ALIAS(shell_memtrace, memtrace, read / write memory);
#endif /* NOT SUPPORTED */

/**
  * @brief  Abort the command execution
  * @param  sig the signal
  */
__attribute__((weak)) void shell_abort_exec(int32_t sig)
{
  (void)sig;
}

/**
  * @brief  Start the command execution
  * @param  func the command function
  * @param  argc the argument count
  * @param  argv the argument list
  * @return the return value of the command
 */
__attribute__((weak)) int32_t shell_start_exec(cmd_function_t func, int32_t argc, char *argv[])
{
  return func(argc, argv);
}

/**
  * @brief  Duplicate the command line
  * @param  cmd the command string
  * @param  length the length of the command string
 */
__attribute__((weak)) void shell_dup_line(char *cmd, uint32_t length)
{
  (void)cmd;
  (void)length;
}

/** @} */
