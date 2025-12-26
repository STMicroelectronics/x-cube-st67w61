/**
  ******************************************************************************
  * @file    modem_cmd_handler.c
  * @author  GPM Application Team
  * @brief   Modem command handler for modem context driver
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
  * Portions of this file are based on Zephyr Project,
  * which is licensed under the Apache-2.0 license as indicated below.
  * See https://github.com/zephyrproject-rtos/zephyr for more information.
  *
  * Reference source:
  * https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/modem/modem_cmd_handler.c
  */

/*
 * Copyright (c) 2019-2020 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "modem_cmd_handler.h"
#include "w61_default_config.h"
#include "logging.h"

/* Private macros ------------------------------------------------------------*/
/** @addtogroup ST67W61_AT_Modem_Cmd_Handler_Macros
  * @{
  */

/**
  * \def LOG_ERR( ... )
  * Send a message to the log with level ::LOG_ERR.
  */

/**
  * \def LOG_DBG( ... )
  * Send a message to the log with level ::LOG_DEBUG.
  */

/**
  * \def LOG_HEXDUMP_DBG( ... )
  * Send a hex dump message to the log with level ::LOG_DEBUG.
  */

#if (MDM_CMD_LOG_ENABLE == 1)
#define LOG_ERR(...) LogError(__VA_ARGS__)
#define LOG_DBG(...) LogDebug(__VA_ARGS__)
#define LOG_HEXDUMP_DBG(...)
#else
#define LOG_ERR(...)
#define LOG_DBG(...)
#define LOG_HEXDUMP_DBG(...)
#endif /* MDM_CMD_LOG_ENABLE */

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_Modem_Cmd_Handler_Functions
  * @{
  */

/**
  * @brief  Checks if the given character is a carriage return (CR) or line feed (LF)
  * @param  c: The character to check
  * @return true if the character is either (CR) or (LF), false otherwise
  */
static bool is_crlf(uint8_t c);

/**
  * @brief  Skips carriage return and line feed characters in the modem command handler data
  * @details This function advances the internal pointer or index within the provided
  *          modem_cmd_handler_data structure to bypass any CR or LF characters.
  *          It is typically used to ignore line endings when parsing modem responses or commands.
  * @param  data: Pointer to the modem_cmd_handler_data structure containing the buffer
  *               and current parsing position
  */
static void skipcrlf(struct modem_cmd_handler_data *data);

/**
  * @brief  Finds the position of the next carriage return (CR) or line feed (LF) character
  * @details This function scans the modem command handler data buffer for the next occurrence
  *          of a CR or LF character and returns its position.
  * @param  data: Pointer to the modem_cmd_handler_data structure containing the buffer
  *               and current parsing position
  * @param  offset: Pointer to a variable to store the offset of the found character
  * @return uint16_t: The position of the found character, or 0 if not found
  */
static uint16_t findcrlf(struct modem_cmd_handler_data *data, uint16_t *offset);

/**
  * @brief  Checks if the given buffer starts with the specified string
  * @details This function compares the beginning of the buffer with the provided
  *          null-terminated string and returns true if the buffer starts with the string.
  *
  * @param  buf: Pointer to the buffer to check
  * @param  buf_len: Length of the buffer in bytes
  * @param  str: Null-terminated string to compare with the start of the buffer
  * @return true if the buffer starts with the specified string, false otherwise
  */
static bool starts_with(const uint8_t *buf, size_t buf_len, const char *str);

/**
  * @brief  Skips the first n bytes in the given linear buffer
  * @details This function advances the buffer pointer by n bytes and decreases the buffer length accordingly.
  * @param  rx_buf: Pointer to the buffer to be modified
  * @param  rx_buf_len: Pointer to the length of the buffer; will be updated after skipping
  * @param  n: Number of bytes to skip from the beginning of the buffer
  */
static void linear_buf_skip(uint8_t *rx_buf, size_t *rx_buf_len, size_t n);

/**
  * @brief  Parses parameters from the matched command in the modem command handler data
  * @details This function extracts parameters from the matched command string in the
  *          modem_cmd_handler_data structure, based on the specified command definition.
  *          It populates the argv array with pointers to the parsed parameters and updates
  *          the argc count. The function handles quoted parameters and ensures that the
  *          number of parsed parameters falls within the defined minimum and maximum limits.
  * @param  data: Pointer to the modem_cmd_handler_data structure containing the matched command
  * @param  match_len: Length of the matched command string
  * @param  cmd: Pointer to the modem_cmd structure defining the command and its parameters
  * @param  argv: Array of pointers to store the addresses of parsed parameters
  * @param  argv_len: Length of the argv array (maximum number of parameters it can hold)
  * @param  argc: Pointer to a variable to store the count of parsed parameters
  * @return On success, returns the number of bytes parsed for parameters;
  *         On error, returns a negative error code (e.g., -EINVAL for invalid arguments)
  */
static int32_t parse_params(struct modem_cmd_handler_data *data, size_t match_len,
                            const struct modem_cmd *cmd, uint8_t **argv, size_t argv_len, uint16_t *argc);

/**
  * @brief  Processes a matched command in the modem command handler
  * @details This function is responsible for executing the matched command
  *          and handling its parameters. It validates the command and
  *          invokes the appropriate command handler function.
  * @param  cmd: Pointer to the modem_cmd structure defining the command
  * @param  match_len: Length of the matched command string
  * @param  data: Pointer to the modem_cmd_handler_data structure containing
  *               the current state and data for command processing
  * @return On success, returns 0; on error, returns a negative error code
  */
static int32_t process_cmd(const struct modem_cmd *cmd, size_t match_len, struct modem_cmd_handler_data *data);

/**
  * @brief  Finds a matching command in the modem command handler data
  * @details This function searches through the command lists in the modem_cmd_handler_data
  *          structure to find a command that matches the current input buffer (match_buf).
  *          It checks the response handlers, unsolicited handlers, and currently assigned handlers.
  * @param  data: Pointer to the modem_cmd_handler_data structure containing the command lists
  * @return Pointer to the matched modem_cmd structure if a match is found; NULL otherwise
  */
static const struct modem_cmd *find_cmd_match(struct modem_cmd_handler_data *data);

/**
  * @brief  Finds a direct match command in the modem command handler data
  * @details This function searches through the command lists in the modem_cmd_handler_data
  *          structure to find a command that directly matches the start of the current
  *          input buffer (rx_buf). It checks for commands marked with the 'direct' flag.
  * @param  data: Pointer to the modem_cmd_handler_data structure containing the command lists
  * @return Pointer to the matched modem_cmd structure if a direct match is found; NULL otherwise
  */
static const struct modem_cmd *find_cmd_direct_match(struct modem_cmd_handler_data *data);

/**
  * @brief  Process incoming data from the modem interface
  * @details This function reads data from the modem interface and appends it to the
  *          receive buffer in the modem_cmd_handler_data structure. It handles reading
  *          as much data as possible without exceeding the buffer size.
  * @param  data: Pointer to the modem_cmd_handler_data structure containing the receive buffer
  * @param  iface: Pointer to the modem_iface structure representing the modem interface
  * @return On success, returns 0; on error, returns a negative error code
  */
static int32_t cmd_handler_process_iface_data(struct modem_cmd_handler_data *data, struct modem_iface *iface);

/**
  * @brief  Process the received data in the modem command handler
  * @details This function processes the data in the receive buffer of the
  *          modem_cmd_handler_data structure. It looks for complete commands
  *          terminated by CR/LF, matches them against known commands, and
  *          invokes the corresponding command handlers.
  * @param  data: Pointer to the modem_cmd_handler_data structure containing the receive buffer
  */
static void cmd_handler_process_rx_buf(struct modem_cmd_handler_data *data);

/**
  * @brief  Process the modem command handler
  * @details This function processes the modem command handler by invoking the
  *          appropriate command handler functions based on the current state
  *          and received data.
  * @param  cmd_handler: Pointer to the modem_cmd_handler structure representing the command handler
  * @param  iface: Pointer to the modem_iface structure representing the modem interface
  */
static void cmd_handler_process(struct modem_cmd_handler *cmd_handler, struct modem_iface *iface);

/* Private Functions Definition ----------------------------------------------*/
static bool is_crlf(uint8_t c)
{
  if ((c == '\n') || (c == '\r'))
  {
    return true;
  }
  else
  {
    return false;
  }
}

static void skipcrlf(struct modem_cmd_handler_data *data)
{
  size_t skip_count = 0;
  if ((data == NULL) || (data->rx_buf == NULL) || (data->rx_buf_len == 0))
  {
    return;
  }
  while ((skip_count < data->rx_buf_len) && is_crlf(data->rx_buf[skip_count]))
  {
    skip_count++;
  }
  if (skip_count > 0)
  {
    linear_buf_skip(data->rx_buf, &data->rx_buf_len, skip_count);
  }
}

static uint16_t findcrlf(struct modem_cmd_handler_data *data, uint16_t *offset)
{
  if ((data == NULL) || (data->rx_buf == NULL) || (data->rx_buf_len == 0) || (offset == NULL))
  {
    return 0;
  }
  for (uint16_t i = 0; i < data->rx_buf_len; ++i)
  {
    if (is_crlf(data->rx_buf[i]))
    {
      *offset = i;
      return i;
    }
  }
  return 0;
}

static bool starts_with(const uint8_t *buf, size_t buf_len, const char *str)
{
  size_t i = 0;
  while (i < buf_len && *str)
  {
    if (buf[i] != (uint8_t)*str)
    {
      return false;
    }
    i++;
    str++;
  }
  return *str == 0;
}

static void linear_buf_skip(uint8_t *rx_buf, size_t *rx_buf_len, size_t n)
{
  if (n >= *rx_buf_len)
  {
    *rx_buf_len = 0;
  }
  else
  {
    for (size_t i = 0; i < *rx_buf_len - n; ++i)
    {
      rx_buf[i] = rx_buf[i + n];
    }
    *rx_buf_len -= n;
  }
}

static int32_t parse_params(struct modem_cmd_handler_data *data, size_t match_len,
                            const struct modem_cmd *cmd, uint8_t **argv, size_t argv_len, uint16_t *argc)
{
  int32_t count = 0;
  size_t delim_len, begin, end, i;
  bool quoted = false;

  if ((data == NULL) || (data->match_buf == NULL) || (match_len == 0) || (cmd == NULL) ||
      (argv == NULL) || (argc == NULL))
  {
    return -EINVAL;
  }

  begin = cmd->cmd_len;
  end = cmd->cmd_len;
  delim_len = strlen(cmd->delim);
  while (end < match_len)
  {
    /* Don't look for delimiters in the middle of a quoted parameter */
    if (data->match_buf[end] == '"')
    {
      quoted = !quoted;
    }
    if (quoted)
    {
      end++;
      continue;
    }
    /* Look for delimiter characters */
    for (i = 0; i < delim_len; i++)
    {
      if (data->match_buf[end] == cmd->delim[i])
      {
        /* Mark a parameter beginning */
        argv[*argc] = (uint8_t *)&data->match_buf[begin];
        /* End parameter with NUL char */
        data->match_buf[end] = '\0';
        /* Bump begin */
        begin = end + 1;
        count += 1;
        (*argc)++;
        break;
      }
    }

    if (count >= cmd->arg_count_max)
    {
      break;
    }

    if (*argc == argv_len)
    {
      break;
    }

    end++;
  }

  /* Consider the ending portion a param if end > begin */
  if (end > begin)
  {
    /* Mark a parameter beginning */
    argv[*argc] = (uint8_t *)&data->match_buf[begin];
    /* End parameter with NUL char
     * NOTE: if this is at the end of match_len will probably
     * be overwriting a NUL that's already there
     */
    data->match_buf[end] = '\0';
    (*argc)++;
  }

  /* Missing arguments */
  if (*argc < cmd->arg_count_min)
  {
    /* Do not return -EAGAIN here as there is no way new argument
     * can be parsed later because match_len is computed to be
     * the minimum of the distance to the first CRLF and the size
     * of the buffer.
     * Therefore, waiting more data on the interface won't change
     * match_len value, which mean there is no point in waiting
     * for more arguments, this will just end in a infinite loop
     * parsing data and finding that some arguments are missing.
     */
    return -EINVAL;
  }

  /*
   * Return the beginning of the next unfinished param so don't
   * "skip" any data that could be parsed later.
   */
  return begin - cmd->cmd_len;
}

static int32_t process_cmd(const struct modem_cmd *cmd, size_t match_len, struct modem_cmd_handler_data *data)
{
  int32_t parsed_len = 0;
  int32_t ret = 0;
  uint8_t *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  uint16_t argc = 0U;

  /* Reset params */
  memset(argv, 0, sizeof(argv[0]) * ARRAY_SIZE(argv));

  /* Needed to parse arguments? */
  if (cmd->arg_count_max > 0U)
  {
    /* Returns < 0 on error and > 0 for parsed len */
    parsed_len = parse_params(data, match_len, cmd,
                              argv, ARRAY_SIZE(argv), &argc);
    if (parsed_len < 0)
    {
      return parsed_len;
    }
  }

  /* Skip cmd_len + parsed len */
  size_t skip_len = cmd->cmd_len + parsed_len;
  /* Store original rx_buf */
  uint8_t *orig_rx_buf = data->rx_buf;
  size_t orig_rx_buf_len = data->rx_buf_len;

  if ((skip_len > 0) && (skip_len <= data->rx_buf_len))
  {
    data->rx_buf += skip_len;
    data->rx_buf_len -= skip_len;
  }

  /* Call handler */
  if (cmd->func)
  {
    ret = cmd->func(data, match_len - cmd->cmd_len - parsed_len,
                    argv, argc);
  }
  /* Restore rx_buf */
  data->rx_buf = orig_rx_buf;
  data->rx_buf_len = orig_rx_buf_len;
  if ((ret != -EAGAIN) && (data->rx_buf != NULL))
  {
    /* Skip in case consumed */
    linear_buf_skip(data->rx_buf, &data->rx_buf_len, skip_len);
  }

  return ret;
}

static const struct modem_cmd *find_cmd_match(struct modem_cmd_handler_data *data)
{
  int32_t j;
  size_t i;

  for (j = 0; j < ARRAY_SIZE(data->cmds); j++)
  {
    if ((data->cmds[j] == NULL) || (data->cmds_len[j] == 0U))
    {
      continue;
    }

    for (i = 0; i < data->cmds_len[j]; i++)
    {
      /* Match on "empty" cmd */
      if (((data->cmds[j][i].cmd != NULL) && (data->match_buf != NULL)) &&
          ((strlen(data->cmds[j][i].cmd) == 0) ||
           (strncmp(data->match_buf, data->cmds[j][i].cmd, data->cmds[j][i].cmd_len) == 0)))
      {
        return &data->cmds[j][i];
      }
    }
  }

  return NULL;
}

static const struct modem_cmd *find_cmd_direct_match(struct modem_cmd_handler_data *data)
{
  size_t j, i;

  if (data->rx_buf == NULL)
  {
    return NULL;
  }

  for (j = 0; j < ARRAY_SIZE(data->cmds); j++)
  {
    if ((data->cmds[j] == NULL) || (data->cmds_len[j] == 0U))
    {
      continue;
    }

    for (i = 0; i < data->cmds_len[j]; i++)
    {
      /* Match start of cmd */
      if ((data->cmds[j][i].cmd != NULL) &&
          data->cmds[j][i].direct &&
          ((data->cmds[j][i].cmd[0] == '\0') ||
           starts_with(data->rx_buf, data->rx_buf_len, data->cmds[j][i].cmd)))
      {
        return &data->cmds[j][i];
      }
    }
  }

  return NULL;
}

static int32_t cmd_handler_process_iface_data(struct modem_cmd_handler_data *data, struct modem_iface *iface)
{
  size_t bytes_read = 0;
  int32_t ret;

  if ((data->rx_buf == NULL) || (iface == NULL) || (iface->read == NULL))
  {
    return -EINVAL;
  }
  /* Read as much as possible into the linear buffer */
  size_t room = RX_BUF_SIZE - data->rx_buf_len;
  if (room == 0)
  {
    return -ENOMEM;
  }
  ret = iface->read(iface, data->rx_buf + data->rx_buf_len, room, &bytes_read);
  if ((ret < 0) || (bytes_read == 0))
  {
    return 0;
  }
  data->rx_buf_len += bytes_read;

  return 0;
}

static void cmd_handler_process_rx_buf(struct modem_cmd_handler_data *data)
{
  const struct modem_cmd *cmd;
  size_t match_len;
  int32_t ret;
  uint16_t offset;
  uint16_t len;

  if ((data->rx_buf == NULL) || (data->match_buf == NULL))
  {
    return;
  }

  while (data->rx_buf_len)
  {
    skipcrlf(data);
    if (!data->rx_buf_len)
    {
      break;
    }

    /* Direct match */
    cmd = find_cmd_direct_match(data);
    if (cmd && cmd->func)
    {
      ret = cmd->func(data, cmd->cmd_len, NULL, 0);
      if (ret == -EAGAIN)
      {
        break;
      }
      else if (ret > 0)
      {
        LOG_DBG("match direct cmd [%s] (ret:%" PRIi32 ")\n", cmd->cmd, ret);
        linear_buf_skip(data->rx_buf, &data->rx_buf_len, ret);
      }
      continue;
    }

    /* Find CRLF */
    len = findcrlf(data, &offset);
    if (!len)
    {
      break;
    }

    /* Copy up to CRLF into match_buf */
    match_len = (len < data->match_buf_len - 1) ? len : data->match_buf_len - 1;
    memcpy(data->match_buf, data->rx_buf, match_len);
    data->match_buf[match_len] = '\0';

#if defined(CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG)
    LOG_HEXDUMP_DBG(data->match_buf, match_len, "RECV");
#endif /* CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG */

    (void)xSemaphoreTake(data->sem_parse_lock, portMAX_DELAY);

    cmd = find_cmd_match(data);
    if (cmd)
    {
      LOG_DBG("match cmd [%s] (len:%" PRIu32 ")\n", cmd->cmd, match_len);

      ret = process_cmd(cmd, match_len, data);
      if (ret == -EAGAIN)
      {
        (void)xSemaphoreGive(data->sem_parse_lock);
        break;
      }
      else if (ret < 0)
      {
        LOG_ERR("process cmd [%s] (len:%" PRIu32 ", ret:%" PRIi32 ")\n", cmd->cmd, match_len, ret);
      }

      if (!data->rx_buf_len)
      {
        (void)xSemaphoreGive(data->sem_parse_lock);
        break;
      }

      /* Find CRLF again to skip processed line */
      findcrlf(data, &offset);
    }

    (void)xSemaphoreGive(data->sem_parse_lock);

    if ((data->rx_buf != NULL) && (data->rx_buf_len != 0))
    {
      linear_buf_skip(data->rx_buf, &data->rx_buf_len, offset + 1);
    }
  }
}

static void cmd_handler_process(struct modem_cmd_handler *cmd_handler, struct modem_iface *iface)
{
  struct modem_cmd_handler_data *data;
  int32_t err;

  if ((cmd_handler == NULL) || (cmd_handler->cmd_handler_data == NULL) ||
      (iface == NULL) || (iface->read == NULL))
  {
    return;
  }

  data = (struct modem_cmd_handler_data *)(cmd_handler->cmd_handler_data);

  do
  {
    err = cmd_handler_process_iface_data(data, iface);
    cmd_handler_process_rx_buf(data);
  } while (err);
}

/* Functions Definition ------------------------------------------------------*/
int32_t modem_cmd_handler_get_error(struct modem_cmd_handler_data *data)
{
  if (!data)
  {
    return -EINVAL;
  }

  return data->last_error;
}

int32_t modem_cmd_handler_set_error(struct modem_cmd_handler_data *data, int32_t error_code)
{
  if (!data)
  {
    return -EINVAL;
  }

  data->last_error = error_code;
  return 0;
}

int32_t modem_cmd_handler_update_cmds(struct modem_cmd_handler_data *data,
                                      const struct modem_cmd *handler_cmds,
                                      size_t handler_cmds_len,
                                      bool reset_error_flag)
{
  if (!data)
  {
    return -EINVAL;
  }

  data->cmds[CMD_HANDLER] = handler_cmds;
  data->cmds_len[CMD_HANDLER] = handler_cmds_len;
  if (reset_error_flag)
  {
    data->last_error = 0;
  }

  return 0;
}

int32_t modem_cmd_handler_await(struct modem_cmd_handler_data *data,
                                SemaphoreHandle_t sem, TickType_t timeout)
{
  int32_t ret = xSemaphoreTake(sem, timeout);

  if (ret == pdTRUE)
  {
    ret = modem_cmd_handler_get_error(data);
  }
  else
  {
    ret = -ETIMEDOUT;
  }

  return ret;
}

int32_t modem_cmd_send_data_nolock(struct modem_iface *iface,
                                   const uint8_t *buf, size_t len)
{
#if defined(CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG)
  if (len > 256)
  {
    /* Truncate the message, since too long log messages gets dropped somewhere. */
    LOG_HEXDUMP_DBG(buf, 256, "SENT DIRECT DATA (truncated)");
  }
  else
  {
    LOG_HEXDUMP_DBG(buf, len, "SENT DIRECT DATA");
  }
#endif /* CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG */
  return iface->write(iface, buf, len);
}

int32_t modem_cmd_send_ext(struct modem_iface *iface,
                           struct modem_cmd_handler *handler,
                           const struct modem_cmd *handler_cmds,
                           size_t handler_cmds_len, const uint8_t *buf,
                           SemaphoreHandle_t sem, TickType_t timeout, int32_t flags)
{
  struct modem_cmd_handler_data *data;
  int32_t ret = 0;

  if ((iface == NULL) || (handler == NULL) || (handler->cmd_handler_data == NULL) || (buf == NULL))
  {
    return -EINVAL;
  }

  if (timeout == 0)
  {
    /* Semaphore is not needed if there is no timeout */
    sem = NULL;
  }
  else if (sem == NULL)
  {
    /* Cannot respect timeout without semaphore */
    return -EINVAL;
  }

  data = (struct modem_cmd_handler_data *)(handler->cmd_handler_data);
  if (!(flags & MODEM_NO_TX_LOCK))
  {
    (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);
  }

  if (!(flags & MODEM_NO_SET_CMDS))
  {
    ret = modem_cmd_handler_update_cmds(data, handler_cmds,
                                        handler_cmds_len, true);
    if (ret < 0)
    {
      goto unlock_tx_lock;
    }
  }

#if defined(CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG)
  LOG_HEXDUMP_DBG(buf, strlen(buf), "SENT DATA");

  if (data->eol_len > 0)
  {
    if (data->eol[0] != '\r')
    {
      LOG_HEXDUMP_DBG(data->eol, data->eol_len, "SENT EOL");
    }
  }
  else
  {
    LOG_DBG("EOL not set!!!");
  }
#endif /* CONFIG_MODEM_CONTEXT_VERBOSE_DEBUG */
  if (sem)
  {
    (void)xSemaphoreTake(sem, 0); /* Reset semaphore (binary) */
  }

  iface->write(iface, buf, strlen((char *)buf));
  iface->write(iface, (uint8_t const *)data->eol, data->eol_len);

  if (sem)
  {
    ret = modem_cmd_handler_await(data, sem, timeout);
  }

  if (!(flags & MODEM_NO_UNSET_CMDS))
  {
    (void)modem_cmd_handler_update_cmds(data, NULL, 0U, false);
  }

unlock_tx_lock:
  if (!(flags & MODEM_NO_TX_LOCK))
  {
    (void)xSemaphoreGive(data->sem_tx_lock);
  }

  return ret;
}

/* Run a set of AT commands */
int32_t modem_cmd_handler_setup_cmds(struct modem_iface *iface,
                                     struct modem_cmd_handler *handler,
                                     const struct setup_cmd *cmds, size_t cmds_len,
                                     SemaphoreHandle_t sem, TickType_t timeout)
{
  int32_t ret = 0;
  size_t i;

  for (i = 0; i < cmds_len; i++)
  {

    if (cmds[i].handle_cmd.cmd && cmds[i].handle_cmd.func)
    {
      ret = modem_cmd_send(iface, handler,
                           &cmds[i].handle_cmd, 1U,
                           (uint8_t const *)cmds[i].send_cmd,
                           sem, timeout);
    }
    else
    {
      ret = modem_cmd_send(iface, handler,
                           NULL, 0, (uint8_t const *)cmds[i].send_cmd,
                           sem, timeout);
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    if (ret < 0)
    {
      LOG_ERR("command %s ret:%" PRIi32 "\n", cmds[i].send_cmd, ret);
      break;
    }
  }

  return ret;
}

/* Run a set of AT commands, without lock */
int32_t modem_cmd_handler_setup_cmds_nolock(struct modem_iface *iface,
                                            struct modem_cmd_handler *handler,
                                            const struct setup_cmd *cmds,
                                            size_t cmds_len, SemaphoreHandle_t sem,
                                            TickType_t timeout)
{
  int32_t ret = 0;
  size_t i;

  for (i = 0; i < cmds_len; i++)
  {

    if (cmds[i].handle_cmd.cmd && cmds[i].handle_cmd.func)
    {
      ret = modem_cmd_send_nolock(iface, handler,
                                  &cmds[i].handle_cmd, 1U,
                                  (uint8_t const *)cmds[i].send_cmd,
                                  sem, timeout);
    }
    else
    {
      ret = modem_cmd_send_nolock(iface, handler,
                                  NULL, 0, (uint8_t const *)cmds[i].send_cmd,
                                  sem, timeout);
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    if (ret < 0)
    {
      LOG_ERR("command %s ret:%" PRIi32 "\n", cmds[i].send_cmd, ret);
      break;
    }
  }

  return ret;
}

int32_t modem_cmd_handler_tx_lock(struct modem_cmd_handler *handler,
                                  TickType_t timeout)
{
  struct modem_cmd_handler_data *data;
  data = (struct modem_cmd_handler_data *)(handler->cmd_handler_data);

  if (data == NULL)
  {
    return -EINVAL;
  }
  return xSemaphoreTake(data->sem_tx_lock, timeout) == pdTRUE ? 0 : -ETIMEDOUT;
}

void modem_cmd_handler_tx_unlock(struct modem_cmd_handler *handler)
{
  struct modem_cmd_handler_data *data;
  data = (struct modem_cmd_handler_data *)(handler->cmd_handler_data);

  if (data == NULL)
  {
    return;
  }
  (void)xSemaphoreGive(data->sem_tx_lock);
}

int32_t modem_cmd_handler_init(struct modem_cmd_handler *handler,
                               struct modem_cmd_handler_data *data,
                               const struct modem_cmd_handler_config *config)
{
  /* Verify arguments */
  if ((handler == NULL) || (data == NULL) || (config == NULL))
  {
    return -EINVAL;
  }

  /* Verify config */
  if ((config->match_buf == NULL) ||
      (config->match_buf_len == 0) ||
      /*(config->buf_pool == NULL) ||*/
      ((NULL != config->response_cmds) && (0 == config->response_cmds_len)) ||
      ((NULL != config->unsol_cmds) && (0 == config->unsol_cmds_len)))
  {
    return -EINVAL;
  }

  /* Assign data to command handler */
  handler->cmd_handler_data = data;
  /* Init rx_buf_len */
  data->rx_buf_len = 0U;

  /* Assign command process implementation to command handler */
  handler->process = cmd_handler_process;

  /* Store arguments */
  data->match_buf = config->match_buf;
  data->match_buf_len = config->match_buf_len;
  data->eol = config->eol;
  data->cmds[CMD_RESP] = config->response_cmds;
  data->cmds_len[CMD_RESP] = config->response_cmds_len;
  data->cmds[CMD_UNSOL] = config->unsol_cmds;
  data->cmds_len[CMD_UNSOL] = config->unsol_cmds_len;

  /* Process end of line */
  data->eol_len = data->eol == NULL ? 0 : strlen(data->eol);

  /* Store optional user data */
  data->user_data = config->user_data;

  /* Initialize command handler data members */
  data->sem_tx_lock = xSemaphoreCreateMutex();
  data->sem_parse_lock = xSemaphoreCreateMutex();

  return 0;
}

/** @} */
