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
#include "w61_at_rx_parser.h"
#include "common_parser.h" /* Common Parser functions */

#if (SYS_DBG_ENABLE_TA4 >= 1)
#include "trcRecorder.h"
#endif /* SYS_DBG_ENABLE_TA4 */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#ifndef LOCK_RETRY_MAX_NR
/** number of retries to get the resource when the mutex is taken */
#define LOCK_RETRY_MAX_NR                       0
#endif /* LOCK_RETRY_MAX_NR */

#ifndef LOCK_RETRY_PERIOD
/** time to wait between two attempts to get the mutex */
#define LOCK_RETRY_PERIOD                       100
#endif /* LOCK_RETRY_PERIOD */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_Common_Functions
  * @{
  */

/**
  * @brief  Find the number of bytes received in the AT response
  * @param  pdata: pointer to the data
  * @retval number of bytes received by W61
  */
static uint32_t W61_AT_GetBytesNumberReceived(uint8_t *pdata);

/* Functions Definition ------------------------------------------------------*/
W61_Status_t W61_AT_ParseOkErr(char *p_resp)
{
  /* Check if the response is OK */
  if (strncmp((char *)p_resp, AT_OK_STRING, sizeof(AT_OK_STRING) - 1) == 0)
  {
    return W61_STATUS_OK;
  }
  /* Check if the response is ERROR */
  else if (strncmp((char *)p_resp, AT_ERROR_STRING, sizeof(AT_ERROR_STRING) - 1) == 0)
  {
    LogError("ParseOkErr Status : ERROR\n");
    return W61_STATUS_ERROR;
  }
  else /* Unknown response */
  {
    LogError("ParseOkErr Status : %s\n", p_resp);
    return W61_STATUS_UNEXPECTED_RESPONSE;
  }
}

W61_Status_t W61_AT_Common_SetExecute(W61_Object_t *Obj, uint8_t *p_cmd, uint32_t timeout_ms,
                                      const uint32_t line_number, const char *const p_file_name)
{
  int32_t cmd_len;
  int32_t status_len;
  char status_buf[sizeof(AT_ERROR_STRING) - 1] = {'\0'};

#if (SYS_DBG_ENABLE_TA4 >= 1)
  vTracePrintF("W61_AT_SetExecute", "W61_AT_SetExecute: send command");
#endif /* SYS_DBG_ENABLE_TA4 */
  cmd_len = strlen((char *)p_cmd);

  /* Send the command and check if the returned length is the same as the command length */
  if (W61_ATsend(Obj, p_cmd, cmd_len, Obj->NcpTimeout) == cmd_len)
  {
    /* Receive the status and check if the returned length is greater than 0 */
    status_len = W61_ATD_Recv(Obj, (uint8_t *)status_buf, sizeof(AT_ERROR_STRING) - 1, timeout_ms);
    if (status_len > 0)
    {
#if (SYS_DBG_ENABLE_TA4 >= 1)
      vTracePrintF("W61_AT_SetExecute", "W61_AT_SetExecute: received response");
#endif /* SYS_DBG_ENABLE_TA4 */
      /* Check the status of the command */
      return W61_AT_ParseOkErr(status_buf);
    }
    else
    {
      /* No status received */
      LogErrorEx(line_number, p_file_name,
                 "W61_AT_SetExecute: W61_STATUS_TIMEOUT. It can lead to unexpected behavior.\n");
      return W61_STATUS_TIMEOUT;
    }
  }

  return W61_STATUS_IO_ERROR;
}

W61_Status_t W61_AT_Common_Query(W61_Object_t *Obj, uint8_t *p_cmd, uint8_t *p_resp, uint32_t timeout_ms,
                                 const uint32_t line_number, const char *const p_file_name)
{
  /* Note: this code is suitable for queries that expect just one response (plus the response status)
   * When required to call W61_ATD_Recv several times, the sequence W61_ATsend + N*W61_ATD_Recv + W61_AT_ParseOkErr
   * has to be done in the "API" function */
  char status_buf[sizeof(AT_ERROR_STRING) - 1] = {'\0'};
  int32_t cmd_len;
  int32_t resp_len;
  int32_t status_len;
  W61_Status_t ret = W61_STATUS_IO_ERROR;

#if (SYS_DBG_ENABLE_TA4 >= 1)
  vTracePrintF("AT_GetQuery", "AT_GetQuery: send command");
#endif /* SYS_DBG_ENABLE_TA4 */
  cmd_len = strlen((char *)p_cmd);

  /* Send the command and check if the returned length is the same as the command length */
  if (W61_ATsend(Obj, p_cmd, cmd_len, Obj->NcpTimeout) == cmd_len)
  {
    /* Receive the response and check if the returned length is greater than 0 */
    resp_len = W61_ATD_Recv(Obj, p_resp, W61_ATD_CMDRSP_STRING_SIZE, timeout_ms);

    if (resp_len > 0)
    {
      p_resp[resp_len] = '\0'; /* Null terminate the response */
      /* Receive the status and check if the returned length is greater than 0 */
      status_len = W61_ATD_Recv(Obj, (uint8_t *)status_buf, sizeof(AT_ERROR_STRING) - 1, Obj->NcpTimeout);
      if (status_len > 0)
      {
#if (SYS_DBG_ENABLE_TA4 >= 1)
        vTracePrintF("AT_GetQuery", "AT_GetQuery: received response");
#endif /* SYS_DBG_ENABLE_TA4 */
        /* Check the status of the command */
        ret = W61_AT_ParseOkErr(status_buf);
      }
      else
      {
        /* No status received */
        LogErrorEx(line_number, p_file_name,
                   "W61_AT_Query: W61_STATUS_TIMEOUT (no status). It can lead to unexpected behavior\n");
        ret = W61_STATUS_TIMEOUT;
      }
    }
    else
    {
      /* No response received */
      LogErrorEx(line_number, p_file_name,
                 "W61_AT_Query: W61_STATUS_TIMEOUT (no response). It can lead to unexpected behavior\n");
      ret = W61_STATUS_TIMEOUT;
    }
  }

  return ret;
}

W61_Status_t W61_AT_RequestSendData(W61_Object_t *Obj, uint8_t *pdata, uint32_t len, uint32_t timeout_ms)
{
  int32_t bytes_to_send;
  int32_t bytes_consumed_by_the_bus = 0;
  int32_t recv_len;
  int32_t received_by_W61 = 0;

  recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, 3, timeout_ms);

  if ((recv_len == 3) && (Obj->CmdResp[2] == '>'))
  {
    while (bytes_consumed_by_the_bus < len)
    {
      bytes_to_send = len - bytes_consumed_by_the_bus;
      bytes_consumed_by_the_bus += W61_ATsend(Obj, pdata, bytes_to_send, timeout_ms);
    }

    if (bytes_consumed_by_the_bus > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, timeout_ms);
      if (recv_len > 0)
      {
        Obj->CmdResp[recv_len] = 0;
        received_by_W61 = W61_AT_GetBytesNumberReceived(Obj->CmdResp);
      }

      if (received_by_W61 != bytes_consumed_by_the_bus)
      {
        return W61_STATUS_IO_ERROR;
      }
    }
    return W61_STATUS_OK;
  }

  return W61_STATUS_ERROR;
}

bool W61_ATlock(W61_Object_t *Obj, uint32_t busy_timeout_ms)
{
  BaseType_t ret;

  /* Following check and ASSERT are necessary to make sure W61_ATD_Recv is not called form ATD_RxPooling_task_handle()
   * which would cause the task to loop infinitely waiting for itself
   * That scenario would happen if users writing the application calls the W61 API functions on Event-callbacks
   * The check has been placed here rather of putting it in the W61_ATD_Recv() because in any case
   * W61_ATD_Recv() is always consequence of W61_ATsend() and W61_ATsend() is always preceded by W61_ATlock()
   */
  W61_ASSERT(xTaskGetCurrentTaskHandle() != Obj->ATD_RxPooling_task_handle);
  /* The main purpose of the mutex is to allow multiple OS threads to call W61_ATD_Recv()
   * without stealing messages each others.
   * The sequence W61_ATlock, W61_ATsend, W61_ATD_Recv.
   * W61_ATunlock assures that the thread sending the AT command gets its response
   */
  ret = xSemaphoreTake(Obj->xCmdMutex, busy_timeout_ms);

#if (LOCK_RETRY_MAX_NR > 0)
  /* In multitask applications the NCP access might be locked (resource taken by another task),
   * the W61_STATUS_BUSY will be return such the application can handle (retry later).
   * In alternative, if the application is suitable, the user could also wait and retry at driver layer
   * by redefining LOCK_RETRY_MAX_NR and LOCK_RETRY_PERIOD in the Target/w61_driver_config.h file.
  */
  uint32_t count = 0;
  while ((ret == false) && (count < LOCK_RETRY_MAX_NR))
  {
    count++;
    vTaskDelay(LOCK_RETRY_PERIOD);
    ret = xSemaphoreTake(Obj->xCmdMutex, busy_timeout_ms);
  }
#endif /* LOCK_RETRY_MAX_NR */

  return (ret == pdTRUE);
}

bool W61_ATunlock(W61_Object_t *Obj)
{
  /* This ends the sequence W61_ATlock, W61_ATsend, W61_ATD_Recv. W61_ATunlock */
  /* The sequence shall be on the same OS thread */
  /* If another OS thread tries to get the lock it has to wait current OS thread unlocks */

  return (xSemaphoreGive(Obj->xCmdMutex) == pdTRUE);
}

int32_t W61_ATsend(W61_Object_t *Obj, uint8_t *pBuf, uint32_t len, uint32_t timeout_ms)
{
  /* Limit the length because trying sending more data then the xStreamBuffer size
   * would result in an error (sending nothing) */
  AT_LOG_HOST_OUT(pBuf, len);
  return Obj->fops.IO_Send(pBuf, len, timeout_ms);
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
  LogDebug("AT%s %s\n", inOut, log_message);
}

/* Private Functions Definition ----------------------------------------------*/
static uint32_t W61_AT_GetBytesNumberReceived(uint8_t *pdata)
{
  char bytes_received_by_W61[6] = {'\0'};
  int32_t tmp = 0;

  if (strncmp((char *)pdata, "Recv ", strlen("Recv ")) == 0)
  {
    pdata += strlen("Recv ");
  }
  else if (strncmp((char *)pdata, "\r\nRecv ", sizeof("\r\nRecv ") - 1) == 0)
  {
    pdata += strlen("\r\nRecv ");
  }
  else
  {
    return 0;
  }

  strncpy(bytes_received_by_W61, (char *) pdata, 5);
  Parser_StrToInt(bytes_received_by_W61, NULL, &tmp);
  return (uint32_t)tmp;
}

#if defined(__ICCARM__) || defined(__ICCRX__) || defined(__ARMCC_VERSION) /* For IAR/MDK Compiler */
char *strnstr(const char *big, const char *little, size_t len)
{
  size_t  i;
  size_t  j;

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

/* The __weak would allow customers to change it */
/* Even thought this implementation is sufficient to cover the needs */
__attribute__((weak)) void W61_assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,*/
  LogError("Error AT driver API cannot be called on callbacks task\n");
  while (1) {}
}

/** @} */
