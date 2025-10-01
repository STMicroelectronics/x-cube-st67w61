/**
  ******************************************************************************
  * @file    modem_cmd_handler.h
  * @author  GPM Application Team
  * @brief   Modem command handler header file
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
  * https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/modem/modem_cmd_handler.h
  */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MODEM_MODEM_CMD_HANDLER_H
#define MODEM_MODEM_CMD_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

/** @defgroup ST67W61_AT_Modem_Cmd_Handler ST67W61 AT Modem Command Handler
  * @ingroup  ST67W61_AT
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup ST67W61_AT_Modem_Cmd_Handler_Constants ST67W61 AT Modem Command Handler Constants
  * @ingroup  ST67W61_AT_Modem_Cmd_Handler
  * @{
  */

#ifndef CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT
/** Maximum number of parameters for modem commands */
#define CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT 11
#endif /* CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT */

/** Size of the receive buffer */
#define RX_BUF_SIZE        W61_MAX_SPI_XFER

#define EIO                 5       /*!< I/O error */
#define EAGAIN              11      /*!< No more contexts */
#define ENOMEM              12      /*!< Not enough core */
#define EBUSY               16      /*!< Mount device busy */
#define EINVAL              22      /*!< Invalid argument */
#ifndef ETIMEDOUT
#define ETIMEDOUT           116     /*!< Connection timed out */
#endif /* ETIMEDOUT */

#define CMD_RESP            0       /*!< Command response */
#define CMD_UNSOL           1       /*!< Unsolicited command */
#define CMD_HANDLER         2       /*!< Command handler */
#define CMD_MAX             3       /*!< Maximum command types */

/* Flags for modem_send_cmd_ext */
#define MODEM_NO_TX_LOCK    BIT(0)  /*!< No TX lock flag send */
#define MODEM_NO_SET_CMDS   BIT(1)  /*!< No set commands flag send */
#define MODEM_NO_UNSET_CMDS BIT(2)  /*!< No unset commands flag send */

/** @} */

/* Exported types ------------------------------------------------------------*/
/** @defgroup ST67W61_AT_Modem_Cmd_Handler_Types ST67W61 AT Modem Command Handler Types
  * @ingroup  ST67W61_AT_Modem_Cmd_Handler
  * @{
  */

/**
  * @brief  Modem interface structure
  */
struct modem_iface
{
  /** Callback to read data from the modem interface */
  int32_t (*read)(struct modem_iface *iface, uint8_t *buf, size_t size, size_t *bytes_read);
  /** Callback to write data to the modem interface */
  int32_t (*write)(struct modem_iface *iface, const uint8_t *buf, size_t size);
};

/**
  * @brief  Modem command handler structure
  */
struct modem_cmd_handler
{
  /** Callback to process commands */
  void (*process)(struct modem_cmd_handler *cmd_handler, struct modem_iface *iface);
  /** Data based on modem_cmd_handler_data */
  void *cmd_handler_data;
};

/**
  * @brief  Modem context structure
  */
struct modem_context
{
  /** Interface configuration */
  struct modem_iface iface;
  /** Command handler configuration */
  struct modem_cmd_handler cmd_handler;
};

/**
  * @brief  Modem command handler data
  */
struct modem_cmd_handler_data;

/**
  * @brief  Modem command structure
  */
struct modem_cmd
{
  /** Callback to process commands */
  int32_t (*func)(struct modem_cmd_handler_data *data, uint16_t len, uint8_t **argv, uint16_t argc);
  /** Command string to match */
  const char *cmd;
  /** Command delimiter */
  const char *delim;
  /** Command length */
  uint16_t cmd_len;
  /** Minimum argument count */
  uint16_t arg_count_min;
  /** Maximum argument count */
  uint16_t arg_count_max;
  /** Command direct match flag */
  bool direct;
};

/**
  * @brief  Series of modem setup commands to run
  */
struct setup_cmd
{
  /** Command to send */
  const char *send_cmd;
  /** Handle command */
  struct modem_cmd handle_cmd;
};

/**
  * @brief  Modem command handler data
  */
struct modem_cmd_handler_data
{
  /** Command list */
  const struct modem_cmd *cmds[CMD_MAX];
  /** Command list lengths */
  size_t cmds_len[CMD_MAX];
  /** Buffer for matching commands */
  char *match_buf;
  /** Length of buffer for matching commands */
  size_t match_buf_len;
  /** Last error code */
  int32_t last_error;
  /** End of line string */
  const char *eol;
  /** Length of end of line string */
  size_t eol_len;
  /** RX net buffer */
  uint8_t *rx_buf;
  /** Length of RX net buffer */
  size_t rx_buf_len;
  /** TX lock */
  SemaphoreHandle_t sem_tx_lock;
  /** Parse lock */
  SemaphoreHandle_t sem_parse_lock;
  /** User data */
  void *user_data;
};

/**
  * @brief  Modem command handler configuration
  * @details Contains user configuration which is used to set up
  *          command handler data context. The struct is initialized and then passed
  *          to modem_cmd_handler_init().
  */
struct modem_cmd_handler_config
{
  /** Buffer used for matching commands */
  char *match_buf;
  /** Length of buffer used for matching commands */
  size_t match_buf_len;
  /** End of line represented as string */
  const char *eol;
  /** Length of end of line string */
  size_t eol_len;
  /** Free to use data which can be retrieved from within command handlers */
  void *user_data;
  /** Array of response command handlers */
  const struct modem_cmd *response_cmds;
  /** Length of response command handlers array */
  size_t response_cmds_len;
  /** Array of unsolicited command handlers */
  const struct modem_cmd *unsol_cmds;
  /** Length of unsolicited command handlers array */
  size_t unsol_cmds_len;
};

/** @} */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup ST67W61_AT_Modem_Cmd_Handler_Macros ST67W61 AT Modem Command Handler Macros
  * @ingroup  ST67W61_AT_Modem_Cmd_Handler
  * @{
  */

#ifndef ARRAY_SIZE
/** Get the number of elements in an array */
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof((array)[0]))
#endif /* ARRAY_SIZE */

#ifndef BIT
/** Get the bit value at position n */
#define BIT(n)              (1U << (n))
#endif /* BIT */

/** Macro to declare a modem command handler function */
#define MODEM_CMD_DECLARE(name_) \
  static int32_t name_(struct modem_cmd_handler_data *data, uint16_t len, \
                       uint8_t **argv, uint16_t argc)

/** Macro to declare a direct modem command handler function */
#define MODEM_CMD_DIRECT_DECLARE(name_) MODEM_CMD_DECLARE(name_)

/** Macro to define a modem command handler function */
#define MODEM_CMD_DEFINE(name_) \
  static int32_t name_(struct modem_cmd_handler_data *data, uint16_t len, \
                       uint8_t **argv, uint16_t argc)

/** Macro to define a direct modem command handler function */
#define MODEM_CMD_DIRECT_DEFINE(name_) MODEM_CMD_DEFINE(name_)

/** Macro to call a direct modem command handler function */
#define MODEM_CMD_DIRECT(cmd_, func_cb_) { \
                                           .cmd = cmd_,                         \
                                           .cmd_len = (uint16_t)sizeof(cmd_)-1, \
                                           .func = func_cb_,                    \
                                           .arg_count_min = 0,                  \
                                           .arg_count_max = 0,                  \
                                           .delim = "",                         \
                                           .direct = true,                      \
                                           }

/** Macro to call a modem command handler function with fixed arguments number */
#define MODEM_CMD(cmd_, func_cb_, acount_, adelim_) { \
                                             .cmd = cmd_,                         \
                                             .cmd_len = (uint16_t)sizeof(cmd_)-1, \
                                             .func = func_cb_,                    \
                                             .arg_count_min = acount_,            \
                                             .arg_count_max = acount_,            \
                                             .delim = adelim_,                    \
                                             .direct = false,                     \
                                             }

/** Macro to call a modem command handler function with variable arguments range */
#define MODEM_CMD_ARGS_MAX(cmd_, func_cb_, acount_, acountmax_, adelim_) { \
                                               .cmd = cmd_,                         \
                                               .cmd_len = (uint16_t)sizeof(cmd_)-1, \
                                               .func = func_cb_,                    \
                                               .arg_count_min = acount_,            \
                                               .arg_count_max = acountmax_,         \
                                               .delim = adelim_,                    \
                                               .direct = false,                     \
                                               }

/** @} */

/* Exported functions ------------------------------------------------------- */
/** @defgroup ST67W61_AT_Modem_Cmd_Handler_Functions ST67W61 AT Modem Command Handler Functions
  * @ingroup  ST67W61_AT_Modem_Cmd_Handler
  * @{
  */

/**
  * @brief  get the last error code
  * @param  data: command handler data reference
  * @return last handled error
  */
int32_t modem_cmd_handler_get_error(struct modem_cmd_handler_data *data);

/**
  * @brief  set the last error code
  * @param  data: command handler data reference
  * @param  error_code: error
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_handler_set_error(struct modem_cmd_handler_data *data, int32_t error_code);

/**
  * @brief  update the parser's handler commands
  * @param  data: handler data to use
  * @param  handler_cmds: commands to attach
  * @param  handler_cmds_len: size of commands array
  * @param  reset_error_flag: reset last error code
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_handler_update_cmds(struct modem_cmd_handler_data *data,
                                      const struct modem_cmd *handler_cmds,
                                      size_t handler_cmds_len,
                                      bool reset_error_flag);

/**
  * @brief  Wait until semaphore is given
  * @details This function does the same wait behavior as @ref modem_cmd_send_ext, but can wait without sending
  *          any command first. Useful for waiting for asynchronous responses.
  * @param  data: handler data to use
  * @param  sem: wait for semaphore
  * @param  timeout: wait timeout
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_handler_await(struct modem_cmd_handler_data *data,
                                SemaphoreHandle_t sem, TickType_t timeout);

/**
  * @brief  send data directly to interface w/o TX lock
  * @details This function just writes directly to the modem interface.
  *          Recommended to use to get verbose logging for all data sent to the interface.
  * @param  iface: interface to use
  * @param  buf: send buffer (not NULL terminated)
  * @param  len: length of send buffer
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_send_data_nolock(struct modem_iface *iface,
                                   const uint8_t *buf, size_t len);

/**
  * @brief  send AT command to interface with behavior defined by flags
  * @details This function is similar to @ref modem_cmd_send_ext, but it allows to choose a
  *          specific behavior regarding acquiring tx_lock, setting and unsetting
  *          @a handler_cmds.
  * @param  iface: interface to use
  * @param  handler: command handler to use
  * @param  handler_cmds: commands to attach
  * @param  handler_cmds_len: size of commands array
  * @param  buf: NULL terminated send buffer
  * @param  sem: wait for response semaphore
  * @param  timeout: timeout of command
  * @param  flags: flags which influence behavior of command sending
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_send_ext(struct modem_iface *iface,
                           struct modem_cmd_handler *handler,
                           const struct modem_cmd *handler_cmds,
                           size_t handler_cmds_len, const uint8_t *buf,
                           SemaphoreHandle_t sem, TickType_t timeout, int32_t flags);

/**
  * @brief  send AT command to interface w/o locking TX
  * @param  iface: interface to use
  * @param  handler: command handler to use
  * @param  handler_cmds: commands to attach
  * @param  handler_cmds_len: size of commands array
  * @param  buf: NULL terminated send buffer
  * @param  sem: wait for response semaphore
  * @param  timeout: timeout of command
  * @return 0 if ok, < 0 if error
  */
static inline int32_t modem_cmd_send_nolock(struct modem_iface *iface,
                                            struct modem_cmd_handler *handler,
                                            const struct modem_cmd *handler_cmds,
                                            size_t handler_cmds_len,
                                            const uint8_t *buf, SemaphoreHandle_t sem,
                                            TickType_t timeout)
{
  return modem_cmd_send_ext(iface, handler, handler_cmds,
                            handler_cmds_len, buf, sem, timeout,
                            MODEM_NO_TX_LOCK);
}

/**
  * @brief  send AT command to interface w/ a TX lock
  * @param  iface: interface to use
  * @param  handler: command handler to use
  * @param  handler_cmds: commands to attach
  * @param  handler_cmds_len: size of commands array
  * @param  buf: NULL terminated send buffer
  * @param  sem: wait for response semaphore
  * @param  timeout: timeout of command
  * @return 0 if ok, < 0 if error
  */
static inline int32_t modem_cmd_send(struct modem_iface *iface,
                                     struct modem_cmd_handler *handler,
                                     const struct modem_cmd *handler_cmds,
                                     size_t handler_cmds_len, const uint8_t *buf,
                                     SemaphoreHandle_t sem, TickType_t timeout)
{
  return modem_cmd_send_ext(iface, handler, handler_cmds,
                            handler_cmds_len, buf, sem, timeout, 0);
}

/**
  * @brief  send a series of AT commands w/ a TX lock
  * @param  iface: interface to use
  * @param  handler: command handler to use
  * @param  cmds: array of setup commands to send
  * @param  cmds_len: size of the setup command array
  * @param  sem: wait for response semaphore
  * @param  timeout: timeout of command
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_handler_setup_cmds(struct modem_iface *iface,
                                     struct modem_cmd_handler *handler,
                                     const struct setup_cmd *cmds, size_t cmds_len,
                                     SemaphoreHandle_t sem, TickType_t timeout);

/**
  * @brief  send a series of AT commands w/o locking TX
  * @param  iface: interface to use
  * @param  handler: command handler to use
  * @param  cmds: array of setup commands to send
  * @param  cmds_len: size of the setup command array
  * @param  sem: wait for response semaphore
  * @param  timeout: timeout of command
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_handler_setup_cmds_nolock(struct modem_iface *iface,
                                            struct modem_cmd_handler *handler,
                                            const struct setup_cmd *cmds,
                                            size_t cmds_len, SemaphoreHandle_t sem,
                                            TickType_t timeout);

/**
  * @brief  Initialize modem command handler
  * @details This function is called once for each command handler, before any
  *          incoming data is processed.
  * @note   All arguments passed to this function, including the referenced data
  *         contained in the setup struct, must persist as long as the command handler itself.
  * @param  handler: Command handler to initialize
  * @param  data: Command handler data to use
  * @param  config: Command handler configuration
  * @return -EINVAL if any argument is invalid
  * @return 0 if successful
  */
int32_t modem_cmd_handler_init(struct modem_cmd_handler *handler,
                               struct modem_cmd_handler_data *data,
                               const struct modem_cmd_handler_config *config);

/**
  * @brief  Lock the modem for sending cmds
  * @details This is semaphore-based rather than mutex based, which means there's no
  *          requirements of thread ownership for the user. This function is useful
  *          when one needs to prevent threads from sending UART data to the modem for an
  *          extended period of time (for example during modem reset).
  * @param  handler: command handler to lock
  * @param  timeout: give up after timeout
  * @return 0 if ok, < 0 if error
  */
int32_t modem_cmd_handler_tx_lock(struct modem_cmd_handler *handler,
                                  TickType_t timeout);

/**
  * @brief  Unlock the modem for sending cmds
  * @param  handler: command handler to unlock
  */
void modem_cmd_handler_tx_unlock(struct modem_cmd_handler *handler);

/**
  * @brief Process incoming data
  * @details This function will process any data available from the interface
  *          using the command handler. The command handler will invoke any matching modem
  *          command which has been registered using @ref modem_cmd_handler_update_cmds.
  *          Once handled, the function will return.
  * @note This function should be invoked from a dedicated thread, which only handles
  *       commands.
  * @param handler: The handler which will handle the command when processed
  * @param iface: The interface which receives incoming data
  */
static inline void modem_cmd_handler_process(struct modem_cmd_handler *handler,
                                             struct modem_iface *iface)
{
  handler->process(handler, iface);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* MODEM_MODEM_CMD_HANDLER_H */
