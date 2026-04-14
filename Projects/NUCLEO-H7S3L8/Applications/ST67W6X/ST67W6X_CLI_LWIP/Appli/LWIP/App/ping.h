/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ping.h
  * @author  ST67 Application Team
  * @brief   Ping module definition
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PING_H
#define PING_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "lwip.h"
#include "lwip/opt.h"
#include "lwip/inet.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */
/* Exported types ------------------------------------------------------------*/
/**
  * @brief  ping configuration structure
  */
typedef struct
{
  uint16_t count;                     /*!< Number of ping requests */
  uint32_t interval_ms;               /*!< Interval between ping requests in milliseconds */
  size_t size;                        /*!< Size of the ping payload */
  uint32_t timeout;                   /*!< Timeout for each ping request in milliseconds */
#if (LWIP_IPV6 == 1)
  uint32_t ipv6;                      /*!< false for IPv4, true for IPv6 */
  char dst_addr[IP6ADDR_STRLEN_MAX];  /*!< Destination address (supports IPv6 and IPv4) */
#else
  char dst_addr[IPADDR_STRLEN_MAX];   /*!< Destination address (supports IPv4) */
#endif /* LWIP_IPV6 */
  uint32_t ping_sent;                 /*!< Number of ping requests sent */
  uint32_t ping_rcv;                  /*!< Number of ping responses received */
  uint32_t ping_tot_time_ms;          /*!< Total time taken for all ping requests in milliseconds */
} ping_context_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */
/* Exported functions --------------------------------------------------------*/
/**
  * @brief  run the ping application (IPv4/IPv6)
  * @param  ping_context: Ping parameters and results
  * @return 0 on success, -1 on failure
  */
int32_t send_ping(ping_context_t *ping_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PING_H */
