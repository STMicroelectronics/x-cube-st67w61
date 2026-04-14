/**
  ******************************************************************************
  * @file    shell.c
  * @author  ST67 Application Team
  * @brief   This file is part of the shell module
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
 * Copyright (c) 2006-2025, RT-Thread Development Team
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

/* Exported constants --------------------------------------------------------*/
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
  * @param  shell_printf_fn the print function
  * @return 0 on success, -1 on error
  */
int32_t shell_set_print(void (*shell_printf_fn)(char *fmt, ...));

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
  * @param  pShell the shell instance
 */
static void shell_push_history(struct shell *pShell);

#if (SHELL_USING_WORD_OPERATION == 1)
/**
  * @brief  Find the previous word start position
  * @param  line the command line
  * @param  curpos the current cursor position
  * @return the previous word start position
  */
static int32_t find_prev_word_start(const char *line, int32_t curpos);

/**
  * @brief  Find the next word end position
  * @param  line the command line
  * @param  curpos the current cursor position
  * @param  max the maximum position
  * @return the next word end position
  */
static int32_t find_next_word_end(const char *line, int32_t curpos, int32_t max);
#endif /* SHELL_USING_WORD_OPERATION */

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
  * @brief  Display all the available commands and the relative help message
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
  if ((srclen + 1U) < maxlen)
  {
    (void)memcpy(dst, src, srclen + 1U);
  }
  else if (maxlen != 0U)
  {
    (void)memcpy(dst, src, maxlen - 1U);
    dst[maxlen - 1U] = '\0';
  }
  else
  {
    /* maxlen is 0, do nothing */
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
  while ((rsize-- != 0U) && (*d != '\0'))
  {
    d++;
  }
  dlen = d - dst;
  rsize = size - dlen;

  if (rsize == 0U)
  {
    return (dlen + strlen(s));
  }
  while (*s != '\0')
  {
    if (rsize != 1U)
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
  static struct shell pShell =
  {
    .syscall_table_begin = NULL,
    .syscall_table_end = NULL,
  };

  return &pShell;
}

void shell_handler(uint8_t data)
{
  struct shell *pShell = shell_get_instance();
  /**
    * handle control key
    * Key              | Key Code             | Notes
    * --------------------------------------------------------------
    * ctrl + c         | 0x03                 | interrupt current input
    * tab              | 0x09                 | auto complete command or display help if empty line
    * backspace        | 0x08 or 0x7f         | delete character before cursor
    * up key           | 0x1b 0x5b 0x41       | display previous history command
    * down key         | 0x1b 0x5b 0x42       | display next history command
    * right key        | 0x1b 0x5b 0x43       | move cursor right
    * left key         | 0x1b 0x5b 0x44       | move cursor left
    * ctrl + right key | 0x1b 0x4f 0x43       | move cursor to next word
    * ctrl + left key  | 0x1b 0x4f 0x44       | move cursor to previous word
    * home             | 0x1b 0x5b 0x31 0x7E  | move cursor to beginning of line
    * insert           | 0x1b 0x5b 0x32 0x7E  | toggle insert/overwrite mode
    * del              | 0x1b 0x5b 0x33 0x7E  | delete character at current cursor position
    * end              | 0x1b 0x5b 0x34 0x7E  | move cursor to end of line
    */

  if (data == 0x03U)
  {
    /*!< ctrl + c */
    SHELL_PRINTF("^C");
    data = 0x0DU; /* CR */
    /* Clear command line */
    pShell->line_position = 0U;
    pShell->line_curpos = 0U;
  }

  if (data == 0x1BU)
  {
    pShell->stat = WAIT_SPEC_KEY;
    return;
  }
  else if (pShell->stat == WAIT_SPEC_KEY)
  {
    if ((data == 0x5BU) || (data == 0x41U) || (data == 0x42U) || (data == 0x43U) || (data == 0x44U) || (data == 0x4FU))
    {
      pShell->stat = WAIT_FUNC_KEY;
#if (SHELL_USING_WORD_OPERATION == 1)
      if (data == 0x4FU)
      {
        pShell->control_mode = 1U;
      }
#endif /* SHELL_USING_WORD_OPERATION */
      return;
    }

    pShell->stat = WAIT_NORMAL;
  }
  else if (pShell->stat == WAIT_FUNC_KEY)
  {
    pShell->stat = WAIT_NORMAL;

    if (data == 0x41U) /* Up key */
    {
      /* Prev history */
      if (pShell->current_history > 0U)
      {
        pShell->current_history--;
      }
      else
      {
        pShell->current_history = 0U;
        return;
      }

      /* Copy the history command */
      (void)memcpy(pShell->line, &pShell->cmd_history[pShell->current_history][0], SHELL_CMD_SIZE);
      pShell->line_position = (uint16_t)strlen(pShell->line);
      pShell->line_curpos = pShell->line_position;
      shell_handle_history(pShell);

      return;
    }
    if (data == 0x42U) /* Down key */
    {
      /* Next history */
      if (pShell->current_history < (pShell->history_count - 1U))
      {
        pShell->current_history++;
      }
      else
      {
        /* Set to the end of history */
        if (pShell->history_count != 0U)
        {
          pShell->current_history = pShell->history_count - 1U;
        }
        else
        {
          return;
        }
      }

      (void)memcpy(pShell->line, &pShell->cmd_history[pShell->current_history][0], SHELL_CMD_SIZE);
      pShell->line_position = (uint16_t)strlen(pShell->line);
      pShell->line_curpos = pShell->line_position;
      shell_handle_history(pShell);

      return;
    }
    if (data == 0x44U) /* Left key */
    {
      if (pShell->line_curpos > 0U)
      {
#if (SHELL_USING_WORD_OPERATION == 1)
        if (pShell->control_mode == 1U) /* Ctrl + left handling */
        {
          int32_t new_pos = find_prev_word_start(pShell->line, pShell->line_curpos);
          if (new_pos != pShell->line_curpos)
          {
            SHELL_PRINTF("\033[%dD", pShell->line_curpos - new_pos);
            pShell->line_curpos = new_pos;
          }
          pShell->control_mode = 0U;
        }
        else
#endif /* SHELL_USING_WORD_OPERATION */
        {
          SHELL_PRINTF("\b");
          pShell->line_curpos--;
        }
      }

      return;
    }
    if (data == 0x43U) /* Right key */
    {
      if (pShell->line_curpos < pShell->line_position)
      {
#if (SHELL_USING_WORD_OPERATION == 1)
        if (pShell->control_mode == 1U) /* Ctrl + right handling */
        {
          int32_t new_pos = find_next_word_end(pShell->line, pShell->line_curpos, pShell->line_position);
          if (new_pos != pShell->line_curpos)
          {
            SHELL_PRINTF("\033[%dC", new_pos - pShell->line_curpos);
            pShell->line_curpos = new_pos;
          }
          pShell->control_mode = 0U;
        }
        else
#endif /* SHELL_USING_WORD_OPERATION */
        {
          SHELL_PRINTF("%c", pShell->line[pShell->line_curpos]);
          pShell->line_curpos++;
        }
      }
      return;
    }
#if (SHELL_USING_FUNC_EXT == 1)
    if ((data >= 0x31U) && (data <= 0x34U)) /* Extended key: home(0x31), insert(0x32), del(0x33), end(0x34) */
    {
      pShell->stat                           = WAIT_EXT_KEY;
      pShell->line[pShell->line_position + 1U] = (char)data; /* Store the key code */
      return;
    }
  }
  else if (pShell->stat == WAIT_EXT_KEY)
  {
    pShell->stat = WAIT_NORMAL;

    if (data == 0x7EU) /* Extended key terminator */
    {
      char key_code = pShell->line[pShell->line_position + 1U];

      if (key_code == (char)0x31U) /* Home key */
      {
        /* Move cursor to beginning of line */
        while (pShell->line_curpos > 0U)
        {
          SHELL_PRINTF("\b");
          pShell->line_curpos--;
        }
        return;
      }
      if (key_code == (char)0x32U) /* Insert key */
      {
        /* Toggle insert mode */
        pShell->overwrite_mode = (pShell->overwrite_mode == 0U) ? 1U : 0U;
        return;
      }
      if (key_code == (char)0x33U) /* Del key */
      {
        /* Delete character at current cursor position */
        if (pShell->line_curpos < pShell->line_position)
        {
          pShell->line_position--;
          (void)memmove(&pShell->line[pShell->line_curpos],
                        &pShell->line[pShell->line_curpos + 1U],
                        (size_t)pShell->line_position - (size_t)pShell->line_curpos);

          pShell->line[pShell->line_position] = '\0';

          SHELL_PRINTF("%s ", &pShell->line[pShell->line_curpos]);

          /* Move cursor back to original position */
          for (uint16_t i = pShell->line_curpos; i <= pShell->line_position; i++)
          {
            SHELL_PRINTF("\b");
          }
        }
        return;
      }
      if (key_code == (char)0x34U) /* End key */
      {
        /* Move cursor to end of line */
        while (pShell->line_curpos < pShell->line_position)
        {
          SHELL_PRINTF("%c", pShell->line[pShell->line_curpos]);
          pShell->line_curpos++;
        }
        return;
      }
      return; /* Unknown extended key */
    }
#endif /* SHELL_USING_FUNC_EXT */
  }
  else
  {
    /* stat undefined */
  }

  /* Received null or error */
  if ((data == 0U) || (data == 0xFFU))
  {
    return;
  }
  /* Handle tab key */
  if (data == 0x09U)
  {
    int32_t i;

    /* Move the cursor to the beginning of line */
    for (i = 0; i < pShell->line_curpos; i++)
    {
      SHELL_PRINTF("\b");
    }

    /* Auto complete */
    shell_auto_complete(&pShell->line[0]);
    /* Re-calculate position */
    pShell->line_position = (uint16_t)strlen(pShell->line);
    pShell->line_curpos = pShell->line_position;

    return;
  }
  /* Handle backspace key */
  if ((data == 0x7FU) || (data == 0x08U))
  {
    /* Note that pShell->line_curpos >= 0 */
    if (pShell->line_curpos == 0U)
    {
      return;
    }

    pShell->line_position--;
    pShell->line_curpos--;

    if (pShell->line_position > pShell->line_curpos)
    {
      int32_t i;

      (void)memmove(&pShell->line[pShell->line_curpos],
                    &pShell->line[pShell->line_curpos + 1U],
                    pShell->line_position - pShell->line_curpos);
      pShell->line[pShell->line_position] = '\0';

      SHELL_PRINTF("\b%s  \b", &pShell->line[pShell->line_curpos]);

      /* Move the cursor to the origin position */
      for (i = pShell->line_curpos; i <= pShell->line_position; i++)
      {
        SHELL_PRINTF("\b");
      }
    }
    else
    {
      SHELL_PRINTF("\b \b");
      pShell->line[pShell->line_position] = '\0';
    }

    return;
  }

  /* Handle end of line, break */
  if ((data == 0x0AU) || (data == 0x0DU))
  {
    shell_push_history(pShell);

    SHELL_PRINTF("\n");

#if (SHELL_PRINT_STATUS == 1)
    int32_t cmd_ret = shell_exec(pShell->line, pShell->line_position);
    if (cmd_ret == SHELL_STATUS_OK)
    {
      SHELL_PRINTF("SUCCESS\n");
    }
    else if (cmd_ret != SHELL_STATUS_NO_COMMAND)
    {
      SHELL_PRINTF("ERROR\n");
    }
    else
    {
      /* No command found, do nothing */
    }
#else
    (void)shell_exec(pShell->line, pShell->line_position);
#endif /* SHELL_PRINT_STATUS */

    SHELL_PROMPT(shell_get_prompt());
    (void)memset(pShell->line, 0, sizeof(pShell->line));
    pShell->line_position = 0U;
    pShell->line_curpos = 0U;
    return;
  }
  /* Return not printable character */
  if ((data < 0x20U) || (data >= 0x80U))
  {
    return;
  }
  /* It's a large line, discard it */
  if (pShell->line_position >= SHELL_CMD_SIZE)
  {
    pShell->line_position = 0U;
  }

  /* Normal character */
  if (pShell->line_curpos < pShell->line_position)
  {
    int32_t i;
#if (SHELL_USING_FUNC_EXT == 1)
    if (pShell->overwrite_mode == 1U) /* Overwrite mode */
    {
      /* Directly overwrite the character */
      pShell->line[pShell->line_curpos] = data;
      SHELL_PRINTF("%c", data);
      pShell->line_curpos++;
    }
    else /* Insert mode */
#endif /* SHELL_USING_FUNC_EXT */
    {
      pShell->line_position++;
      /* Move existing characters to the right */
      (void)memmove(&pShell->line[pShell->line_curpos + 1U],
                    &pShell->line[pShell->line_curpos],
                    pShell->line_position - pShell->line_curpos);
      pShell->line[pShell->line_curpos] = data;

      SHELL_PRINTF("%s", &pShell->line[pShell->line_curpos]);

      /* Move cursor back to correct position */
      for (i = pShell->line_curpos + 1U; i < pShell->line_position; i++)
      {
        SHELL_PRINTF("\b");
      }
      pShell->line_curpos++;
    }
  }
  else
  {
    /* Append character at end of line */
    pShell->line[pShell->line_position] = data;
    pShell->shell_printf_fn("%c", data);
    pShell->line_position++;
    pShell->line_curpos++;
    SHELL_FLUSH_OUT;
  }

  if (pShell->line_position >= SHELL_CMD_SIZE)
  {
    /* Clear command line */
    pShell->line_position = 0U;
    pShell->line_curpos = 0U;
  }
}

int32_t shell_set_prompt(const char *prompt)
{
  struct shell *pShell = shell_get_instance();

  if (pShell->prompt_custom != NULL)
  {
    SHELL_FREE(pShell->prompt_custom);
    pShell->prompt_custom = NULL;
  }

  /* Strdup */
  if (prompt != NULL)
  {
    pShell->prompt_custom = (char *)SHELL_MALLOC(strlen(prompt) + 1U);
    if (pShell->prompt_custom != NULL)
    {
      if (strlcpy(pShell->prompt_custom, prompt, strlen(prompt) + 1U) >= (strlen(prompt) + 1U))
      {
        SHELL_LOG("[OS]: strlcpy truncated \n");
      }
    }
  }

  return 0;
}

int32_t shell_set_print(void (*shell_printf_fn)(char *fmt, ...))
{
  if (shell_printf_fn != NULL)
  {
    shell_get_instance()->shell_printf_fn = shell_printf_fn;
    return 0;
  }
  else
  {
    return -1;
  }
}

#if (SHELL_ENABLE == 1)
#if defined(__ICCARM__) || defined(__ICCRX__) /* For IAR compiler */
#pragma section="FSymTab"
#endif /* __ICCARM__ */
#endif /* SHELL_ENABLE */

void shell_init(void (*shell_printf_fn)(char *fmt, ...))
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
  (void)shell_set_prompt(SHELL_DEFAULT_NAME);
  if (shell_printf_fn == NULL)
  {
    shell_printf_fn = (void (*)(char *fmt, ...))printf;
  }
  (void)shell_set_print(shell_printf_fn);
  SHELL_PRINTF(shell_get_prompt());
}

/* Private Functions Definition ----------------------------------------------*/
static char *shell_get_prompt(void)
{
  static char shell_prompt[SHELL_CONSOLEBUF_SIZE + 1] = { 0 };
  struct shell *pShell = shell_get_instance();
  uint32_t len;

  if (pShell->prompt_custom != NULL)
  {
    len = strlcpy(shell_prompt, pShell->prompt_custom, sizeof(shell_prompt));
  }
  else
  {
    len = strlcpy(shell_prompt, SHELL_DEFAULT_NAME, sizeof(shell_prompt));
  }

  if (len >= sizeof(shell_prompt))
  {
    SHELL_LOG("[OS]: strlcpy truncated \n");
  }
  else if ((len + 3U) < SHELL_CONSOLEBUF_SIZE)
  {
    shell_prompt[len] = '/';
    shell_prompt[len + 1U] = '>';
    shell_prompt[len + 2U] = '\0';
  }
  else
  {
    /* Prompt is complete */
  }

  return shell_prompt;
}

static int32_t str_common(const char *str1, const char *str2)
{
  const char *str = str1;

  while ((*str != '\0') && (*str2 != '\0') && (*str == *str2))
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

static void shell_push_history(struct shell *pShell)
{
  if (pShell->line_position != 0U)
  {
    /* Push history */
    if (pShell->history_count >= SHELL_HISTORY_LINES)
    {
      /* If current cmd is same as last cmd, don't push */
      if (memcmp(&pShell->cmd_history[SHELL_HISTORY_LINES - 1], pShell->line, SHELL_CMD_SIZE) != 0)
      {
        /* Move history */
        int32_t index;

        for (index = 0; index < (SHELL_HISTORY_LINES - 1U); index++)
        {
          (void)memcpy(&pShell->cmd_history[index][0], &pShell->cmd_history[index + 1U][0], SHELL_CMD_SIZE);
        }

        (void)memset(&pShell->cmd_history[index][0], 0, SHELL_CMD_SIZE);
        (void)memcpy(&pShell->cmd_history[index][0], pShell->line, pShell->line_position);

        /* It's the maximum history */
        pShell->history_count = SHELL_HISTORY_LINES;
      }
    }
    else
    {
      /* If current cmd is same as last cmd, don't push */
      if ((pShell->history_count == 0U) ||
          memcmp(&pShell->cmd_history[pShell->history_count - 1], pShell->line, SHELL_CMD_SIZE) != 0)
      {
        pShell->current_history = pShell->history_count;
        (void)memset(&pShell->cmd_history[pShell->history_count][0], 0, SHELL_CMD_SIZE);
        (void)memcpy(&pShell->cmd_history[pShell->history_count][0], pShell->line, pShell->line_position);

        /* Increase count and set current history position */
        pShell->history_count++;
      }
    }
  }

  pShell->current_history = pShell->history_count;
}

#if (SHELL_USING_WORD_OPERATION == 1)
static int32_t find_prev_word_start(const char *line, int32_t curpos)
{
  if (curpos <= 0) { return 0; }

  /* Skip whitespace */
  while ((--curpos > 0) && ((line[curpos] == ' ') || (line[curpos] == '\t')))
  {
    /* skip character */
  }

  /* Find word start */
  while ((curpos > 0) && !((line[curpos] == ' ') || (line[curpos] == '\t')))
  {
    curpos--;
  }

  return (curpos <= 0) ? 0 : curpos + 1;
}

static int32_t find_next_word_end(const char *line, int32_t curpos, int32_t max)
{
  if (curpos >= max)
  {
    return max;
  }

  /* Skip to next word */
  while ((curpos < max) && ((line[curpos] == ' ') || (line[curpos] == '\t')))
  {
    curpos++;
  }

  /* Find word end */
  while ((curpos < max) && !((line[curpos] == ' ') || (line[curpos] == '\t')))
  {
    curpos++;
  }

  return curpos;
}
#endif /* SHELL_USING_WORD_OPERATION */

static void shell_auto_complete(char *prefix)
{
  int32_t length;
  int32_t min_length;
  const char *name_ptr;
  const char *cmd_name;
  struct shell_syscall *index;
  struct shell *pShell = shell_get_instance();

  min_length = 0;
  name_ptr = NULL;

  SHELL_PRINTF("\n");

  if (*prefix == '\0')
  {
    (void)shell_help(0, NULL);
    return;
  }

  /* Checks in internal command */
  {
    for (index = pShell->syscall_table_begin; index < pShell->syscall_table_end; index++)
    {
      if (index->name == NULL)
      {
        continue;
      }
      cmd_name = index->name;

      if (strncmp(prefix, cmd_name, strlen(prefix)) == 0)
      {
        if (min_length == 0U)
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
    (void)strlcpy(prefix, name_ptr, min_length + 1);
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
    while (((*ptr == ' ') || (*ptr == '\t')) && (position < length))
    {
      *ptr = '\0';
      ptr++;
      position++;
    }

    if (argc >= SHELL_ARG_NUM)
    {
      SHELL_E("Too many args ! Only Use:\n");

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
      while ((*ptr != '"') && (position < length))
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

      while (((*ptr != ' ') && (*ptr != '\t')) && (position < length))
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
  struct shell *pShell = shell_get_instance();

  for (index = pShell->syscall_table_begin; index < pShell->syscall_table_end; index++)
  {
    if (index->name == NULL)
    {
      continue;
    }
    if ((strncmp(index->name, cmd, size) == 0) && (index->name[0U + size] == '\0'))
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
  while (((cmd[cmd0_size] != ' ') && (cmd[cmd0_size] != '\t')) && (cmd0_size < length))
  {
    cmd0_size++;
  }

  if (cmd0_size == 0U)
  {
    return -1;
  }

  cmd_func = shell_get_cmd(cmd, cmd0_size);

  if (cmd_func == NULL)
  {
    return -1;
  }

  /* Split arguments */
  (void)memset(argv, 0x00, sizeof(argv));
  argc = shell_split(cmd, length, argv);

  if (argc == 0)
  {
    return -1;
  }

  /* Exec this command */
  *retp = cmd_func(argc, argv);
#if (SHELL_USING_DESCRIPTION == 1)
  /* Print usage when return value is SHELL_STATUS_UNKNOWN_ARGS */
  if (*retp == SHELL_STATUS_UNKNOWN_ARGS)
  {
    struct shell_syscall *index;
    struct shell *pShell = shell_get_instance();

    for (index = pShell->syscall_table_begin; index < pShell->syscall_table_end; index++)
    {
      if ((index->name == NULL) ||
          ((strncmp(index->name, cmd, cmd0_size) == 0) &&
           (index->name[0U + cmd0_size] == '\0')))
      {
        SHELL_E("Unknown argument. Usage: %s\n", index->desc);
        break;
      }
    }
  }
#endif /* SHELL_USING_DESCRIPTION */

  return 0;
}

int32_t shell_exec(char *cmd, uint32_t length)
{
  int32_t cmd_ret;

  /* Strip the beginning of command */
  while ((length > 0U) && ((*cmd == ' ') || (*cmd == '\t')))
  {
    cmd++;
    length--;
  }

  if (length == 0U)
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

  /* Truncate the cmd at the first space */
  {
    char *tcmd;
    tcmd = cmd;

    while ((*tcmd != ' ') && (*tcmd != '\0'))
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
  struct shell *pShell = shell_get_instance();
  pShell->syscall_table_begin = (struct shell_syscall *)begin;
  pShell->syscall_table_end = (struct shell_syscall *)end;
}
#endif /* SHELL_ENABLE */

int32_t shell_help(int32_t argc, char **argv)
{
  struct shell *pShell = shell_get_instance();

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

    for (index = pShell->syscall_table_begin; index < pShell->syscall_table_end; index++)
    {
      if (index->name == NULL)
      {
        continue;
      }
#if (SHELL_USING_DESCRIPTION == 1)
      if ((argc > 1) &&
          (strncmp(index->name, argv[1], MIN(strlen(argv[1]), SHELL_HELP_MAX_COMPARED_NB_CHAR)) != 0))
      {
        continue;
      }
      if (strlen(index->desc) > SHELL_HELP_DESC_SIZE)
      {
        /* Split the description in multi lines */
        char desc_buf[SHELL_HELP_DESC_SIZE + 1U];
        size_t offset = 0U;
        size_t desc_len = strlen(index->desc);
        uint8_t first_line = 1U;
        do
        {
          size_t line_len = MIN(desc_len, SHELL_HELP_DESC_SIZE);

          /* Find the last space to split */
          if (line_len == SHELL_HELP_DESC_SIZE)
          {
            for (size_t i = line_len; i > 0U; i--)
            {
              if (index->desc[offset + i - 1U] == ' ')
              {
                line_len = i - 1U;
                break;
              }
            }
          }
          /* Copy description line */
          (void)memcpy(desc_buf, &index->desc[offset], line_len);
          desc_buf[line_len] = '\0';

          if (first_line == 1U) /* Print description line */
          {
            SHELL_PRINTF("%-30s - %s\n", index->name, desc_buf);
            first_line = 0U;
          }
          else /* Print following lines */
          {
            SHELL_PRINTF("%-30s   %s\n", "", desc_buf);
          }
          offset += line_len;
          desc_len -= line_len;
        } while (desc_len > 0U);
      }
      else
      {
        SHELL_PRINTF("%-30s - %s\n", index->name, index->desc);
      }
#else
      SHELL_PRINTF("%s\n", index->name);
#endif /* SHELL_USING_DESCRIPTION */
    }
  }
  SHELL_PRINTF("\n");

  return 0;
}

SHELL_CMD_EXPORT_ALIAS(shell_help, help, help [ command ].
                       Display all available commands and the relative help message);

/** @} */
