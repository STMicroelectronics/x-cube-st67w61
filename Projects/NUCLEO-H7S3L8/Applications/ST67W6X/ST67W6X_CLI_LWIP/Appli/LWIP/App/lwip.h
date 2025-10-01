/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lwip.h
  * @author  GPM Application Team
  * @brief   This file provides code for the configuration of the LWIP.
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
#ifndef __LWIP_H
#define __LWIP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/**
  * @brief  Network interface type
  */
typedef enum
{
  NETIF_STA = 0,                   /*!< Station interface */
  NETIF_AP  = 1,                   /*!< Access Point interface */
  NETIF_MAX,                       /*!< Maximum number of interfaces */
} Netif_type_t;

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initialize the LwIP stack and create a network task.
  * @return Operation status
  */
int32_t MX_LWIP_Init(void);

/**
  * @brief  Retrieve the network interface structure.
  * @param  link_id: Link identifier (0 for STA, 1 for AP)
  * @return Pointer to the network interface structure.
  */
struct netif *netif_get_interface(uint32_t link_id);

/**
  * @brief  Print IPv4 addresses assigned to the network interface.
  * @param  netif: Pointer to the network interface structure.
  * @return Operation status
  */
int32_t print_ipv4_addresses(struct netif *netif);

/**
  * @brief  Print all valid IPv6 addresses assigned to the network interface.
  * @param  netif: Pointer to the network interface structure.
  * @return Operation status
  */
int32_t print_ipv6_addresses(struct netif *netif);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LWIP_H */
