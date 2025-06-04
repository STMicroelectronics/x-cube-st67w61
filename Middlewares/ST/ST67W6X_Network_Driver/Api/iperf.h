/**
  ******************************************************************************
  * @file    iperf.h
  * @author  GPM Application Team
  * @brief   Header for iperf.c
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
#ifndef __IPERF_H__
#define __IPERF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "w6x_config.h"

/* Exported types ------------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Iperf_Types ST67W6X Utility Performance Iperf Types
  * @ingroup  ST67W6X_Utilities_Performance_Iperf
  * @{
  */

/**
  * @brief  iperf configuration structure
  */
typedef struct
{
  uint32_t flag;                    /*!< iperf flag */
  union
  {
    uint32_t destination_ip4;       /*!< destination IPv4 address */
    char *destination_ip6;          /*!< destination IPv6 address */
  };
  union
  {
    uint32_t source_ip4;            /*!< source IPv4 address */
    char *source_ip6;               /*!< source IPv6 address */
  };
  uint8_t type;                     /*!< iperf protocol type */
  uint16_t dport;                   /*!< destination port */
  uint16_t sport;                   /*!< source port */
  uint32_t interval;                /*!< interval between each frame */
  uint32_t time;                    /*!< duration of the test */
  uint16_t len_buf;                 /*!< length of the buffer */
  int32_t bw_lim;                   /*!< bandwidth limit */
  uint8_t tos;                      /*!< type of service */
  uint8_t traffic_task_priority;    /*!< traffic task priority */
  uint32_t num_bytes;               /*!< number of bytes to send */
} iperf_cfg_t;

/** @} */

/* Exported constants --------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Iperf_Constants ST67W6X Utility Performance Iperf Constants
  * @ingroup  ST67W6X_Utilities_Performance_Iperf
  * @{
  */

#ifndef IPERF_ENABLE
/** Enable Iperf feature */
#define IPERF_ENABLE                0
#endif /* IPERF_ENABLE */

#ifndef IPERF_V6
/** Enable IPv6 for Iperf */
#define IPERF_V6                    0
#endif /* IPERF_V6 */

#ifndef IPERF_DUAL_MODE
/** Enable dual mode for Iperf */
#define IPERF_DUAL_MODE             0
#endif /* IPERF_DUAL_MODE */

#ifndef IPERF_TRAFFIC_TASK_PRIORITY
/** Iperf traffic task priority */
#define IPERF_TRAFFIC_TASK_PRIORITY 40
#endif /* IPERF_TRAFFIC_TASK_PRIORITY */

#ifndef IPERF_TRAFFIC_TASK_STACK
/** Iperf traffic task stack size */
#define IPERF_TRAFFIC_TASK_STACK    2048
#endif /* IPERF_TRAFFIC_TASK_STACK */

#ifndef IPERF_REPORT_TASK_STACK
/** Iperf report task stack size */
#define IPERF_REPORT_TASK_STACK     2048
#endif /* IPERF_REPORT_TASK_STACK */

/* Memory allocator */
#ifndef IPERF_MALLOC
/** Iperf memory allocator */
#define IPERF_MALLOC                pvPortMalloc
#endif /* IPERF_MALLOC */

#ifndef IPERF_FREE
/** Iperf memory deallocator */
#define IPERF_FREE                  vPortFree
#endif /* IPERF_FREE */

#define IPERF_TRAFFIC_TASK_NAME     "iperf_traffic" /*!< iperf traffic task name */
#define IPERF_REPORT_TASK_NAME      "iperf_report"  /*!< iperf report task name */

#define IPERF_IP_TYPE_IPV4          0               /*!< IPv4 type */
#define IPERF_IP_TYPE_IPV6          1               /*!< IPv6 type */
#define IPERF_TRANS_TYPE_TCP        0               /*!< TCP type */
#define IPERF_TRANS_TYPE_UDP        1               /*!< UDP type */

#define IPERF_FLAG_CLIENT           (1)             /*!< Client flag bitmask */
#define IPERF_FLAG_SERVER           (1 << 1)        /*!< Server flag bitmask */
#define IPERF_FLAG_TCP              (1 << 2)        /*!< TCP flag bitmask */
#define IPERF_FLAG_UDP              (1 << 3)        /*!< UDP flag bitmask */
#define IPERF_FLAG_DUAL             (1 << 4)        /*!< Dual flag bitmask */

#define IPERF_UDP_TX_LEN            1470            /*!< UDP transmit length */
#define IPERF_UDP_RX_LEN            1470            /*!< UDP receive length */
#define IPERF_TCP_TX_LEN            (6 << 10)       /*!< TCP transmit length */
#define IPERF_TCP_RX_LEN            (6 << 10) - 64  /*!< TCP receive length (6kB minus AT Response header length) */

#define IPERF_MAX_DELAY             64              /*!< Maximum delay */

#define IPERF_SOCKET_RX_TIMEOUT     5               /*!< Socket receive timeout */

/** @} */

/* Exported variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/** @defgroup ST67W6X_Utilities_Performance_Iperf_Functions ST67W6X Utility Performance Iperf Functions
  * @ingroup  ST67W6X_Utilities_Performance_Iperf
  * @{
  */

/**
  * @brief  Start iperf
  * @param  cfg: iperf configuration
  * @retval 0 if success, -1 if fail
  */
int32_t iperf_start(iperf_cfg_t *cfg);

/**
  * @brief  Abort iperf execution
  * @retval 0 if success, -1 if fail
  */
int32_t iperf_stop(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __IPERF_H__ */
