/**
  ******************************************************************************
  * @file    w61_at_common.h
  * @author  GPM Application Team
  * @brief   This file provides the common definitions of the AT driver
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
#ifndef W61_AT_COMMON_H
#define W61_AT_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/

#include "w61_at_api.h"

/** @defgroup ST67W61_AT_Common ST67W61 AT Driver Common
  * @ingroup  ST67W61_AT
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup ST67W61_AT_Common_Constants ST67W61 AT Driver Common Constants
  * @ingroup  ST67W61_AT_Common
  * @{
  */

#ifndef W61_CMDRSP_STRING_SIZE
/** W61 AT command response string size */
#define W61_CMDRSP_STRING_SIZE                  192
#endif /* W61_CMDRSP_STRING_SIZE */

#ifndef W61_MDM_RX_TASK_STACK_SIZE_BYTES
/** Stack required especially for Log messages */
#define W61_MDM_RX_TASK_STACK_SIZE_BYTES        2048
#endif /* W61_MDM_RX_TASK_STACK_SIZE_BYTES */

#ifndef W61_MDM_RX_TASK_PRIO
/** W61 W61_Modem_Process task priority, recommended to be higher than application tasks */
#define W61_MDM_RX_TASK_PRIO                    54
#endif /* W61_MDM_RX_TASK_PRIO */

#ifndef W61_MAX_AT_LOG_LENGTH
/** Maximum size of AT log */
#define W61_MAX_AT_LOG_LENGTH                   300
#endif /* W61_MAX_AT_LOG_LENGTH */

/** Cmd/query default timeouts */
#ifndef W61_NCP_TIMEOUT
/** Timeout for reply/execute of NCP */
#define W61_NCP_TIMEOUT                         2000
#endif /* W61_NCP_TIMEOUT */

#ifndef W61_SYS_TIMEOUT
/** Timeout for special cases like flash write, firmware updates, etc */
#define W61_SYS_TIMEOUT                         2000
#endif /* W61_SYS_TIMEOUT */

#ifndef W61_WIFI_TIMEOUT
/** Timeout for remote WIFI device operation (e.g. AP) */
#define W61_WIFI_TIMEOUT                        6000
#endif /* W61_WIFI_TIMEOUT */

#ifndef W61_NET_TIMEOUT
/** Timeout for remote network operation (e.g. server) */
#define W61_NET_TIMEOUT                         6000
#endif /* W61_NET_TIMEOUT */

#ifndef W61_BLE_TIMEOUT
/** Timeout for remote BLE device operation */
#define W61_BLE_TIMEOUT                         2000
#endif /* W61_BLE_TIMEOUT */

/** @} */

/* Exported variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/** @defgroup ST67W61_AT_Common_Macros ST67W61 AT Driver Common Macros
  * @ingroup  ST67W61_AT_Common
  * @{
  */

#ifndef CONTAINER_OF
/** Return the pointer to the structure containing the specified member */
#define CONTAINER_OF(ptr, type, field)                  \
  ({                                                    \
    ((type *)(((char *)(ptr)) - offsetof(type, field)));\
  })
#endif

/**
  * \def AT_LOG_HOST_OUT(pBuf, len)
  * AT log trace to announce an AT command sent to the module
  */

/**
  * \def AT_LOG_HOST_IN(pBuf, len)
  * AT log trace to announce an AT message received from the module
  */

#if (W61_AT_LOG_ENABLE == 1)
#define AT_LOG_HOST_OUT(pBuf, len) W61_AT_Logger( pBuf, len, ">")
#define AT_LOG_HOST_IN(pBuf, len)  W61_AT_Logger( pBuf, len, "<")
#else
#define AT_LOG_HOST_OUT(pBuf, len)
#define AT_LOG_HOST_IN(pBuf, len)
#endif /* W61_AT_LOG_ENABLE */

/** @} */

/* Exported functions ------------------------------------------------------- */
/** @defgroup ST67W61_AT_Common_Functions ST67W61 AT Driver Common Functions
  * @ingroup  ST67W61_AT_Common
  * @{
  */

/**
  * @brief  Initialize the W61 AT modem
  * @param  Obj: pointer to module handle
  * @return Operation status
  */
int32_t W61_AT_ModemInit(W61_Object_t *Obj);

/**
  * @brief  Deinitialize the W61 AT modem
  * @param  Obj: pointer to module handle
  */
void W61_AT_ModemDeInit(W61_Object_t *Obj);

/**
  * @brief  Translate modem_cmd status into W61_Status_t
  * @param  ret status
  * @return Operation status W61_Status_t
  */
W61_Status_t W61_Status(int32_t ret);

/**
  * @brief  Send the AT command for Set and Execute mode, and check the status response
  * @param  Obj: pointer to module handle
  * @param  p_cmd: pointer to pass command string
  * @param  timeout_ms: timeout for the W61_SetExecute
  * @return Operation status
  */
W61_Status_t W61_AT_Common_SetExecute(W61_Object_t *Obj, uint8_t *p_cmd, uint32_t timeout_ms);

/**
  * @brief  Send the AT command for Queries, wait Query response and check the status response
  * @param  Obj: pointer to module handle
  * @param  p_cmd: pointer to pass command string. Reused as rx buffer to store the argv content
  * @param  p_resp: pointer to pass expected response string. \
  *                 Must be provided as buffer allocated locally to ensure re-entrance
  * @param  argc: pointer to number of found argument
  * @param  argv: pointer to argument table
  * @param  timeout_ms: timeout for the first W61_AT_Common_Query_Parse
  * @return Operation status
  */
W61_Status_t W61_AT_Common_Query_Parse(W61_Object_t *Obj, char *p_cmd, char *p_resp,
                                       uint16_t *argc, char **argv, uint32_t timeout_ms);

/**
  * @brief  Send the AT command for Set and Execute mode and send data
  * @param  Obj: pointer to module handle
  * @param  p_cmd: pointer to pass command string
  * @param  pdata: pointer to data to send
  * @param  len: binary data length
  * @param  timeout_ms: timeout
  * @param  check_resp: if true, check the response status
  * @return Operation status
  */

W61_Status_t W61_AT_Common_RequestSendData(W61_Object_t *Obj, uint8_t *p_cmd, uint8_t *pdata, uint32_t len,
                                           uint32_t timeout_ms, bool check_resp);

/**
  * @brief Remove the double quotes at the beginning and the end of the string
  * @param inbuf: input string
  */
void W61_AT_RemoveStrQuotes(char *inbuf);

/**
  * @brief  Log AT command
  * @param  pBuf: pointer to buffer
  * @param  len: buffer length
  * @param  inOut: pointer to string
  */
void W61_AT_Logger(uint8_t *pBuf, uint32_t len, char *inOut);

#if defined(__ICCARM__) || defined(__ICCRX__) || defined(__ARMCC_VERSION) /* For IAR/MDK Compiler */
/**
  * @brief  Function locates the first occurrence of the null-terminated string little in the string big
  */
char *strnstr(const char *big, const char *little, size_t len);
#endif /* __ICCARM__ || __ARMCC_VERSION */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W61_AT_COMMON_H */
