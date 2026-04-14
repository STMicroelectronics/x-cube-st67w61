/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dhcp_server.h
  * @author  ST67 Application Team
  * @brief   DHCP Server definition
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
#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <lwip/ip.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/** @brief  DHCP server status callback */
typedef void (*dhcpd_callback_t)(struct netif *netif_cur);

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Start the DHCP server
  * @param  netif_cur: the network interface on which the DHCP server is started
  * @param  start: the start offset of the DHCP IP address pool
  * @param  limit: the number of IP addresses in the DHCP IP address pool
  */
void dhcpd_start(struct netif *netif_cur, int32_t start, int32_t limit);

/**
  * @brief  Stop the DHCP server
  * @param  netif_cur: the network interface on which the DHCP server is stopped
  */
void dhcpd_stop(const struct netif *netif_cur);

/**
  * @brief  Set DHCP server status callback
  * @param  netif_cur: the network interface on which the DHCP server is running
  * @param  cb: the callback function
  * @return lwIP error code
  * - ERR_OK - No error
  * - ERR_VAL - netif has no dhcp server instance
  */
err_t dhcpd_status_callback_set(struct netif *netif_cur, dhcpd_callback_t cb);

/**
  * @brief  Get IP address leased to a client by its MAC address
  * @param  netif_cur: the network interface on which the DHCP server is running
  * @param  mac: the MAC address of the client
  * @param  ipaddr: pointer to store the leased IP address
  * @return lwIP error code
  * - ERR_OK - No error
  * - ERR_VAL - client not found
  */
err_t dhcpd_get_ip_by_mac(struct netif *netif_cur, uint8_t mac[], ip4_addr_t *ipaddr);

/**
  * @brief  Clear DNS server list
  * @param  netif_cur: The netif which use dhcp server
  */
void dhcpd_clear_dns_server(void *netif_cur);

/**
  * @brief  Add a DNS server to the DHCP server DNS server list
  * @param  netif_cur: The netif which use dhcp server
  * @param  dnsserver: The DNS server address
  * @return lwIP error code
  * - ERR_OK - No error
  * - ERR_VAL - No dhcp server instance found or DNS server already exists
  */
err_t dhcpd_add_dns_server(void *netif_cur, const ip_addr_t *dnsserver);

/**
  * @brief  Remove a DNS server from the DHCP server DNS server list
  * @param  netif_cur: The netif which use dhcp server
  * @param  dnsserver: The DNS server address
  * @return lwIP error code
  * - ERR_OK - No error
  * - ERR_VAL - No dhcp server instance found or DNS server not found
  */
err_t dhcpd_remove_dns_server(void *netif_cur, const ip_addr_t *dnsserver);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DHCP_SERVER_H */
