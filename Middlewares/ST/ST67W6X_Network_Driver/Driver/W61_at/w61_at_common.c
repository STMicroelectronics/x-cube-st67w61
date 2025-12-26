/**
  ******************************************************************************
  * @file    w61_at_common.c
  * @author  GPM Application Team
  * @brief   This file provides the common implementations of the AT driver
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
#include "w61_at_common.h"
#include "w61_at_internal.h"
#include "w61_io.h"
#include <stdlib.h>
#include "modem_cmd_handler.h"
#include "stdio.h"
#if (SYS_DBG_ENABLE_TA4 >= 1)
#include "trcRecorder.h"
#endif /* SYS_DBG_ENABLE_TA4 */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W61_AT_Common_Defines ST67W61 AT Driver Common Defines
  * @ingroup ST67W61_AT_Common
  * @{
  */

/** Timeout for io send operation */
#define IO_SEND_TIMEOUT                         2000

/** Timeout for io receive operation */
#define IO_RECEIVE_TIMEOUT                      portMAX_DELAY

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_Common_Functions
  * @{
  */

/**
  * @brief  Initialize the IO interface
  * @param iface: pointer to the modem interface
  * @return 0 on success, negative value on error
  */
static int32_t io_init(struct modem_iface *iface);

/**
  * @brief  Deinitialize the IO interface
  * @param iface: pointer to the modem interface
  * @return 0 on success, negative value on error
  */
static int32_t io_deinit(struct modem_iface *iface);

/**
  * @brief  Modem process task
  * @param arg: pointer to the task argument
  */
static void W61_Modem_Process_task(void *arg);

/**
  * @brief  Write data to the modem interface
  * @param iface: pointer to the modem interface
  * @param buf: pointer to the data buffer
  * @param size: size of the data to write
  * @return 0 on success, negative value on error
  */
static int32_t modem_iface_spi_write(struct modem_iface *iface,
                                     const uint8_t *buf, size_t size);

/**
  * @brief  Read data from the modem interface
  * @param iface: pointer to the modem interface
  * @param buf: pointer to the buffer to store read data
  * @param size: size of the buffer
  * @param bytes_read: pointer to store the number of bytes read
  * @return 0 on success, negative value on error
  */
static int32_t modem_iface_spi_read(struct modem_iface *iface,
                                    uint8_t *buf, size_t size, size_t *bytes_read);

/**
  * @brief  Callback function to handle query events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_query);

/**
  * @brief  Callback function to handle "OK" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_ok);

/**
  * @brief  Callback function to handle "ERROR" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_error);

/**
  * @brief  Callback function to handle "ready" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_ready);

/**
  * @brief  Callback function to handle "+CW" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of arguments
  * @return process length on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_wifi_event);

/**
  * @brief  Callback function to handle "+BLE" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of arguments
  * @return process length on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_ble_event);

/**
  * @brief  Callback function to handle "+CIP" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return process length on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_net_event);

/**
  * @brief  Callback function to handle "+MQTT" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of arguments
  * @return process length on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_mqtt_event);

/**
  * @brief  Callback function to handle ">" events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_tx_ready);

/**
  * @brief  Callback function to handle "RECV " events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_recv);

/**
  * @brief  Callback function to handle MQTT data events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return process length on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_mqtt_data_event);

/**
  * @brief  Callback function to handle Net data events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of arguments
  * @return process length on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_net_data_event);

/**
  * @brief  Callback function to handle BLE Write data events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return process length on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_ble_write_data_event);

/**
  * @brief  Callback function to handle BLE Read data events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return process length on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_ble_read_data_event);

/**
  * @brief  Callback function to handle BLE Notification data events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return process length on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_ble_noti_data_event);

/**
  * @brief  Callback function to handle ping events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return process length on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_net_ping_event);

/**
  * @brief  Callback function to handle Wi-Fi scan events
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of arguments (unused)
  * @return process length on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_wifi_scan_event);

/** @} */

/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W61_AT_Common_Variables ST67W61 AT Driver Common Variables
  * @ingroup ST67W61_AT_Common
  * @{
  */

/** List of response commands for the modem */
static const struct modem_cmd response_cmds[] =
{
  MODEM_CMD("OK", on_cmd_ok, 0U, ""),
  MODEM_CMD("ERROR", on_cmd_error, 0U, ""),
};

/** List of unsolicited commands for the modem */
static const struct modem_cmd unsol_cmds[] =
{
  MODEM_CMD("ready", on_cmd_ready, 0U, ""),
  MODEM_CMD("+CWLAP:", on_cmd_wifi_scan_event, 8U, ","),
  MODEM_CMD_DIRECT("+BLE:GATTWRITE:", on_cmd_ble_write_data_event),
  MODEM_CMD_DIRECT("+BLE:GATTREAD:", on_cmd_ble_read_data_event),
  MODEM_CMD_DIRECT("+BLE:NOTIDATA:", on_cmd_ble_noti_data_event),
  MODEM_CMD_DIRECT("+MQTT:SUBRECV:", on_cmd_mqtt_data_event),
  MODEM_CMD_ARGS_MAX("+IPD:", on_cmd_net_data_event, 1U, 10U, ","),
  MODEM_CMD_ARGS_MAX("+CW:", on_cmd_wifi_event, 1U, 10U, ", "),
  MODEM_CMD_ARGS_MAX("+BLE:", on_cmd_ble_event, 1U, 11U, ",:()"),
  MODEM_CMD_ARGS_MAX("+CIP:", on_cmd_net_event, 1U, 10U, ","),
  MODEM_CMD_ARGS_MAX("+MQTT:", on_cmd_mqtt_event, 1U, 10U, ","),
  MODEM_CMD("+PING:", on_cmd_net_ping_event, 1U, ""),
};

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W61_AT_Common_Functions
  * @{
  */

int32_t W61_AT_ModemInit(W61_Object_t *Obj)
{
  BaseType_t xReturned;
  int32_t ret = -1;
  struct modem *mdm = (struct modem *) &Obj->Modem;

  /* Cmd handler */
  const struct modem_cmd_handler_config cmd_handler_config =
  {
    .match_buf = (char *) &mdm->cmd_match_buf[0],
    .match_buf_len = sizeof(mdm->cmd_match_buf),
    .eol = "", /* CRLF sent within the command string to avoid 2 successive write */
    .user_data = mdm,
    .response_cmds = response_cmds,
    .response_cmds_len = ARRAY_SIZE(response_cmds),
    .unsol_cmds = unsol_cmds,
    .unsol_cmds_len = ARRAY_SIZE(unsol_cmds),
  };

  mdm->sem_response = xSemaphoreCreateBinary();
  if (mdm->sem_response == NULL)
  {
    goto __err;
  }
  mdm->sem_if_ready = xSemaphoreCreateBinary();
  if (mdm->sem_if_ready == NULL)
  {
    goto __err;
  }
  mdm->sem_tx_ready = xSemaphoreCreateBinary();
  if (mdm->sem_tx_ready == NULL)
  {
    goto __err;
  }

  /* Assign rx_buff */
  /** Linear RX buffer to process */
  mdm->modem_cmd_handler_data.rx_buf = pvPortMalloc(RX_BUF_SIZE);
  if (mdm->modem_cmd_handler_data.rx_buf == NULL)
  {
    goto __err;
  }

  ret = modem_cmd_handler_init(&mdm->modem_cmd_handler, &mdm->modem_cmd_handler_data,
                               &cmd_handler_config);
  if (ret < 0)
  {
    goto __err;
  }

  ret = io_init(&mdm->iface);
  if (ret < 0)
  {
    goto __err;
  }

  xReturned = xTaskCreate(W61_Modem_Process_task,
                          (char *)"Modem_Process",
                          W61_MDM_RX_TASK_STACK_SIZE_BYTES >> 2,
                          mdm,
                          W61_MDM_RX_TASK_PRIO,
                          &mdm->modem_task_handle);

  if (xReturned != pdPASS)
  {
    SYS_LOG_ERROR("xTaskCreate failed to create\n");
    ret = -1;
    goto __err;
  }

  /* Wait for the module to be ready */
  if (W61_WaitForReady(Obj, W61_READY_DEFAULT_TIMEOUT_MS) != W61_STATUS_OK)
  {
    SYS_LOG_ERROR("sem_if_ready not received\n");
    ret = -1;
    goto __err;
  }

  W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT\r\n", W61_NCP_TIMEOUT);

  return W61_Status(ret);
__err:
  if (mdm->modem_cmd_handler_data.rx_buf != NULL)
  {
    vPortFree(mdm->modem_cmd_handler_data.rx_buf);
    mdm->modem_cmd_handler_data.rx_buf = NULL;
  }
  if (mdm->sem_response != NULL)
  {
    vSemaphoreDelete(mdm->sem_response);
    mdm->sem_response = NULL;
  }
  if (mdm->sem_if_ready != NULL)
  {
    vSemaphoreDelete(mdm->sem_if_ready);
    mdm->sem_if_ready = NULL;
  }
  if (mdm->sem_tx_ready != NULL)
  {
    vSemaphoreDelete(mdm->sem_tx_ready);
    mdm->sem_tx_ready = NULL;
  }
  if (mdm->modem_task_handle != NULL)
  {
    vTaskDelete(mdm->modem_task_handle);
    mdm->modem_task_handle = NULL;
  }
  return W61_Status(ret);
}

void W61_AT_ModemDeInit(W61_Object_t *Obj)
{
  struct modem *mdm = (struct modem *) &Obj->Modem;
  io_deinit(&mdm->iface);
  if (mdm->modem_cmd_handler_data.rx_buf != NULL)
  {
    vPortFree(mdm->modem_cmd_handler_data.rx_buf);
    mdm->modem_cmd_handler_data.rx_buf = NULL;
  }
  if (mdm->sem_response != NULL)
  {
    vSemaphoreDelete(mdm->sem_response);
    mdm->sem_response = NULL;
  }
  if (mdm->sem_if_ready != NULL)
  {
    vSemaphoreDelete(mdm->sem_if_ready);
    mdm->sem_if_ready = NULL;
  }
  if (mdm->sem_tx_ready != NULL)
  {
    vSemaphoreDelete(mdm->sem_tx_ready);
    mdm->sem_tx_ready = NULL;
  }
  if (mdm->modem_task_handle != NULL)
  {
    vTaskDelete(mdm->modem_task_handle);
    mdm->modem_task_handle = NULL;
  }
}

W61_Status_t W61_Status(int32_t ret)
{
  W61_Status_t status;

  switch (ret)
  {
    case 0:
      status = W61_STATUS_OK;
      break;
    case -ETIMEDOUT:
      status = W61_STATUS_TIMEOUT;
      break;
    case -EIO:
      status = W61_STATUS_IO_ERROR;
      break;
    default:
      status = W61_STATUS_ERROR;
      break;
  }
  return status;
}

W61_Status_t W61_AT_Common_SetExecute(W61_Object_t *Obj, uint8_t *p_cmd, uint32_t timeout_ms)
{
  struct modem *mdm = &Obj->Modem;
  return W61_Status(modem_cmd_send(&mdm->iface,
                                   &mdm->modem_cmd_handler,
                                   NULL,
                                   0,
                                   p_cmd,
                                   mdm->sem_response,
                                   timeout_ms));
}

W61_Status_t W61_AT_Common_Query_Parse(W61_Object_t *Obj, char *p_cmd, char *p_resp,
                                       uint16_t *argc, char **argv, uint32_t timeout_ms)
{
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;
  W61_Status_t ret;

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);
  /* **argv reference p_cmd to ensure re-entrance */
  mdm->rx_data = p_cmd;
  mdm->argc = argc;
  mdm->argv = argv;

  struct modem_cmd handlers[] = {{
      .cmd = p_resp,
      .cmd_len = (uint16_t)strlen(p_resp),
      .func = on_cmd_query,
      .arg_count_min = 1,
      .arg_count_max = CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT,
      .delim = ",:",
      .direct = false,
    }
  };

  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)p_cmd,
                                      mdm->sem_response,
                                      timeout_ms,
                                      MODEM_NO_TX_LOCK));
  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_AT_Common_RequestSendData(W61_Object_t *Obj, uint8_t *p_cmd, uint8_t *pdata, uint32_t len,
                                           uint32_t timeout_ms, bool check_resp)
{
  struct modem *mdm = (struct modem *) &Obj->Modem;
  int32_t ret;
  int32_t bytes_consumed_by_the_bus = 0;
  int32_t bytes_to_send;

  static const struct modem_cmd cmds[] =
  {
    MODEM_CMD_DIRECT(">", on_cmd_tx_ready),
    MODEM_CMD("Recv ", on_cmd_recv, 1U, " "),
  };

  (void)xSemaphoreTake(mdm->modem_cmd_handler_data.sem_tx_lock, portMAX_DELAY);
  /*reset mdm->sem_tx_read */
  (void)xSemaphoreTake(mdm->sem_tx_ready, 0);

  ret = modem_cmd_send_ext(&mdm->iface, &mdm->modem_cmd_handler,
                           cmds, ARRAY_SIZE(cmds), p_cmd, mdm->sem_response,
                           check_resp ? pdMS_TO_TICKS(timeout_ms) : 0, /* If check_resp is false don't wait for OK */
                           MODEM_NO_TX_LOCK | MODEM_NO_UNSET_CMDS);
  if (ret < 0)
  {
    SYS_LOG_DEBUG("Failed to send command\n");
    goto out;
  }

  /* Reset semaphore that will be released by "Recv " */
  /*reset mdm->sem_response */
  (void)xSemaphoreTake(mdm->sem_response, 0);

  /* Set rx_data_len to be checked during "Recv " event */
  mdm->rx_data_len = len;

  /* Wait for '>' */
  if (xSemaphoreTake(mdm->sem_tx_ready, pdMS_TO_TICKS(5000)) != pdPASS)
  {
    SYS_LOG_DEBUG("Timeout waiting for tx\n");
    ret = -ETIMEDOUT;
    goto out;
  }

  while (bytes_consumed_by_the_bus < len)
  {
    bytes_to_send = len - bytes_consumed_by_the_bus;
    bytes_consumed_by_the_bus += modem_cmd_send_data_nolock(&mdm->iface,
                                                            &pdata[bytes_consumed_by_the_bus],
                                                            bytes_to_send);
  }

  /* Wait for "Recv " */
  if (xSemaphoreTake(mdm->sem_response, pdMS_TO_TICKS(timeout_ms)) != pdPASS)
  {
    SYS_LOG_DEBUG("No send response\n");
    ret = -ETIMEDOUT;
    goto out;
  }

  ret = modem_cmd_handler_get_error(&mdm->modem_cmd_handler_data);
  if (ret != 0)
  {
    SYS_LOG_DEBUG("Failed to send data\n");
  }

out:
  (void)modem_cmd_handler_update_cmds(&mdm->modem_cmd_handler_data,
                                      NULL, 0U, false);
  (void)xSemaphoreGive(mdm->modem_cmd_handler_data.sem_tx_lock);

  return W61_Status(ret);
}

void W61_AT_RemoveStrQuotes(char *inbuf)
{
  int32_t len = strlen(inbuf);

  if (len < 2)
  {
    return; /* Nothing to do */
  }
  inbuf[len - 1] = '\0'; /* Ensure the last character is null-terminated */
  /* Ensure the first character is not a double quote */
  memmove(inbuf, inbuf + 1, len);
}

void W61_AT_Logger(uint8_t *pBuf, uint32_t len, char *inOut)
{
  char log_message[W61_MAX_AT_LOG_LENGTH];
  uint32_t message_len = W61_MAX_AT_LOG_LENGTH - 1;
  if (len < W61_MAX_AT_LOG_LENGTH - 1)
  {
    message_len = len;
  }
  memcpy(log_message, pBuf, message_len);
  log_message[message_len] = 0;
  if (message_len == W61_MAX_AT_LOG_LENGTH - 1)
  {
    log_message[message_len - 1] = '.';
    log_message[message_len - 2] = '.';
    log_message[message_len - 3] = '.';
  }
  SYS_LOG_DEBUG("AT%s %s\n", inOut, log_message);
}

#if defined(__ICCARM__) || defined(__ICCRX__) || defined(__ARMCC_VERSION) /* For IAR/MDK Compiler */
char *strnstr(const char *big, const char *little, size_t len)
{
  size_t i;
  size_t j;

  if (little[0] == '\0')
  {
    return ((char *)big);
  }
  j = 0;
  while (j < len && big[j])
  {
    i = 0;
    while (j < len && little[i] && big[j] && little[i] == big[j])
    {
      ++i;
      ++j;
    }
    if (little[i] == '\0')
    {
      return ((char *)&big[j - i]);
    }
    j = j - i + 1;
  }
  return (0);
}
#endif /* __ICCARM__ || __ARMCC_VERSION */

/* Private Functions Definition ----------------------------------------------*/
static int32_t io_init(struct modem_iface *iface)
{
  if (iface == NULL)
  {
    return -EINVAL;
  }
  if (BusIo_SPI_Init() != 0)
  {
    return -1;
  }
  if (BusIo_SPI_Bind(SPI_MSG_CTRL_TRAFFIC_AT_CMD, 16, NULL) != 0)
  {
    return -1;
  }
  iface->read = modem_iface_spi_read;
  iface->write = modem_iface_spi_write;
  return 0;
}

static int32_t io_deinit(struct modem_iface *iface)
{
  if (iface == NULL)
  {
    return -EINVAL;
  }
  if (BusIo_SPI_DeInit() != 0)
  {
    return -1;
  }
  iface->read = NULL;
  iface->write = NULL;
  return 0;
}

static void W61_Modem_Process_task(void *arg)
{
  struct modem *mdm = (struct modem *) arg;

  while (1)
  {
    modem_cmd_handler_process(&mdm->modem_cmd_handler,
                              &mdm->iface);
  }
}

static int32_t modem_iface_spi_write(struct modem_iface *iface,
                                     const uint8_t *buf, size_t size)
{
  if (size == 0)
  {
    return 0;
  }
  AT_LOG_HOST_OUT((uint8_t *)buf, size);
  return BusIo_SPI_SendData(SPI_MSG_CTRL_TRAFFIC_AT_CMD, (uint8_t *)buf, size, IO_SEND_TIMEOUT);
}

static int32_t modem_iface_spi_read(struct modem_iface *iface,
                                    uint8_t *buf, size_t size, size_t *bytes_read)
{
  int32_t received = BusIo_SPI_ReceiveData(SPI_MSG_CTRL_TRAFFIC_AT_CMD, buf, size, portMAX_DELAY);
  AT_LOG_HOST_IN(buf, received);
  if (received < 0)
  {
    *bytes_read = 0;
    return received;
  }
  *bytes_read = received;
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_query)
{
  struct modem *mdm = (struct modem *) data->user_data;
  int32_t offset = ((uint8_t *) mdm->rx_data) - mdm->cmd_match_buf;
  /*len of the current mdm->cmd_match_buf + '\0'*/
  int32_t mlen = (argv[argc - 1] - mdm->cmd_match_buf) + strlen((char *) argv[argc - 1]) + 1;

  memcpy(mdm->rx_data, (char *) mdm->cmd_match_buf, mlen);
  /* Record and offset argv to mdm->rx_data */
  for (int32_t i = 0; i < argc; i++)
  {
    mdm->argv[i] = (char *)(argv[i] + offset);
  }
  *mdm->argc = argc;

  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_tx_ready)
{
  struct modem *mdm = (struct modem *) data->user_data;

  (void)xSemaphoreGive(mdm->sem_tx_ready);
  return len;
}

MODEM_CMD_DEFINE(on_cmd_recv)
{
  struct modem *mdm = (struct modem *) data->user_data;
  int32_t recv_len = 0;
  if (argc > 0)
  {
    recv_len = atoi((char *) argv[0]);
    /*check if length received by modem is matching the sent value */
    if (recv_len != mdm->rx_data_len)
    {
      modem_cmd_handler_set_error(data, -EIO);
    }
    else
    {
      modem_cmd_handler_set_error(data, 0);
    }
  }
  else
  {
    modem_cmd_handler_set_error(data, -EIO);
  }

  (void)xSemaphoreGive(mdm->sem_response);

  return 0;
}

/* Handler: OK */
MODEM_CMD_DEFINE(on_cmd_ok)
{
  struct modem *mdm = (struct modem *) data->user_data;

  modem_cmd_handler_set_error(data, 0);

  (void)xSemaphoreGive(mdm->sem_response);

  return 0;
}

/* Handler: ERROR */
MODEM_CMD_DEFINE(on_cmd_error)
{
  struct modem *mdm = (struct modem *) data->user_data;

  modem_cmd_handler_set_error(data, -EIO);

  (void)xSemaphoreGive(mdm->sem_response);

  return 0;
}

MODEM_CMD_DEFINE(on_cmd_ready)
{
  struct modem *mdm = (struct modem *) data->user_data;

  (void)xSemaphoreGive(mdm->sem_if_ready);

  return 0;
}

MODEM_CMD_DEFINE(on_cmd_wifi_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.WiFi_event_cb != NULL)
  {
    Obj->Callbacks.WiFi_event_cb(Obj, &argc, (char **)argv);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_wifi_scan_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.WiFi_event_scan_cb != NULL)
  {
    Obj->Callbacks.WiFi_event_scan_cb(Obj, &argc, (char **)argv);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_ble_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.Ble_event_cb != NULL)
  {
    Obj->Callbacks.Ble_event_cb(Obj, &argc, (char **)argv);
  }
  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_ble_write_data_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.Ble_event_data_cb != NULL)
  {
    return Obj->Callbacks.Ble_event_data_cb(W61_BLE_EVT_WRITE_ID, data, len);
  }
  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_ble_read_data_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.Ble_event_data_cb != NULL)
  {
    return Obj->Callbacks.Ble_event_data_cb(W61_BLE_EVT_READ_ID, data, len);
  }
  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_ble_noti_data_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.Ble_event_data_cb != NULL)
  {
    return Obj->Callbacks.Ble_event_data_cb(W61_BLE_EVT_NOTIFICATION_DATA_ID, data, len);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_net_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.Net_event_cb != NULL)
  {
    Obj->Callbacks.Net_event_cb(Obj, &argc, (char **)argv);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_net_ping_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.Net_event_ping_cb != NULL)
  {
    Obj->Callbacks.Net_event_ping_cb(Obj, &argc, (char **)argv);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_net_data_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.Net_event_data_cb != NULL)
  {
    Obj->Callbacks.Net_event_data_cb(Obj, &argc, (char **)argv);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_mqtt_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.MQTT_event_cb != NULL)
  {
    Obj->Callbacks.MQTT_event_cb(Obj, &argc, (char **)argv);
  }
  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_mqtt_data_event)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (Obj->Callbacks.MQTT_event_data_cb != NULL)
  {
    return Obj->Callbacks.MQTT_event_data_cb(0, data, len);
  }
  return 0;
}

/** @} */
