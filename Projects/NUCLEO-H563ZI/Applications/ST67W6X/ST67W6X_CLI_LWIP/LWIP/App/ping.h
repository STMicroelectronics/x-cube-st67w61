/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ping.h
  * @author  GPM Application Team
  * @brief   Ping module definition
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PING_H
#define PING_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
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
  EventGroupHandle_t event;           /*!< Event group handle */
  int32_t count;                      /*!< Number of ping requests */
  int32_t interval_ms;                /*!< Interval between ping requests in milliseconds */
  int32_t size;                       /*!< Size of the ping payload */
  int32_t timeout;                    /*!< Timeout for each ping request in milliseconds */
#if (LWIP_IPV6 == 1)
  uint32_t ipv6;                      /*!< false for IPv4, true for IPv6 */
  char dst_addr[IP6ADDR_STRLEN_MAX];  /*!< Destination address (supports IPv6 and IPv4) */
#else
  char dst_addr[IPADDR_STRLEN_MAX];   /*!< Destination address (supports IPv4) */
#endif /* LWIP_IPV6 */
} ping_params_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */
/* Exported functions --------------------------------------------------------*/
/**
  * Ping ipv4 function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 on success, -1 otherwise
  */
int32_t ping_ipv4_cmd(int32_t argc, char **argv);

/**
  * Ping ipv6 function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 on success, -1 otherwise
  */
int32_t ping_ipv6_cmd(int32_t argc, char **argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PING_H */
