/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lwip_shell.c
  * @author  GPM Application Team
  * @brief   This file provides code for LwIP Shell Commands
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "lwip.h"
#include <lwip/api.h>
#include <lwip/opt.h>
#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/tcpip.h>
#include <lwip/dns.h>
#include <lwip/etharp.h>
#include <lwip/ethip6.h>
#include <lwip/dhcp6.h>
#include <lwip/netifapi.h>
#include <lwip/sockets.h>

#include "lwip.h"
#include "shell.h"
#include "logging.h"
#include "common_parser.h" /* Common Parser functions */
#include "FreeRTOS.h"
#include "task.h"

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Wi-Fi get/set STA IPv4 shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t lwip_shell_sta_ip(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get STA IPv6 shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t lwip_shell_sta_ip6(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get/set STA hostname shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_Hostname(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi resolve host address shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_ResolveHostAddress(int32_t argc, char **argv);

/**
  * @brief  DNS lookup callback function for lwIP
  * @param  name: pointer to the hostname string
  * @param  ipaddr: pointer to the resolved IP address (NULL if failed)
  * @param  arg: user-supplied argument (unused)
  */
void lwip_dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg);

/* Private Functions Definition ----------------------------------------------*/
int32_t lwip_shell_sta_ip(int32_t argc, char **argv)
{
  struct netif *netif = netif_get_interface(NETIF_STA);
  if (netif == NULL)
  {
    return SHELL_STATUS_ERROR;
  }

  if (argc == 1)
  {
    if (print_ipv4_addresses(netif) != 0)
    {
      SHELL_E("Get STA IP error\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else if (argc > 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  else
  {
    ip4_addr_t ipaddr;

    /* Set the STA IP configuration in IP, Gateway, Netmask fixed order. Gateway and Netmask are optional */
    if (argc > 1)
    {
      if (lwip_inet_pton(AF_INET, argv[1], &ipaddr) != 1)
      {
        SHELL_E("IP address invalid\n");
        return SHELL_STATUS_ERROR;
      }
      netif_set_ipaddr(netif, &ipaddr);
    }

    if (argc > 2)
    {
      if (lwip_inet_pton(AF_INET, argv[2], &ipaddr) != 1)
      {
        SHELL_E("Gateway IP address invalid\n");
        return SHELL_STATUS_ERROR;
      }
      netif_set_gw(netif, &ipaddr);
    }

    if (argc > 3)
    {
      if (lwip_inet_pton(AF_INET, argv[3], &ipaddr) != 1)
      {
        SHELL_E("Netmask IP address invalid\n");
        return SHELL_STATUS_ERROR;
      }
      netif_set_netmask(netif, &ipaddr);
    }

    /* Bring the interface up */
    netif_set_up(netif);
    SHELL_PRINTF("STA IP configuration set successfully\n");
  }

  return SHELL_STATUS_OK;
}

/** Shell command to get/set the STA IP configuration */
SHELL_CMD_EXPORT_ALIAS(lwip_shell_sta_ip, net_sta_ip,
                       net_sta_ip [ IP addr ] [ Gateway addr ] [ Netmask addr ].
                       Display or set the IPv4 Address);

int32_t lwip_shell_sta_ip6(int32_t argc, char **argv)
{
#if (LWIP_IPV6 == 1)
  struct netif *netif = netif_get_interface(NETIF_STA);
  if (netif == NULL)
  {
    return SHELL_STATUS_ERROR;
  }

  if (argc == 1)
  {
    if (print_ipv6_addresses(netif) != 0)
    {
      SHELL_E("Get STA IP error\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  return SHELL_STATUS_OK;
#else
  SHELL_E("IPv6 not supported\n");
  return SHELL_STATUS_ERROR;
#endif /* LWIP_IPV6 */
}

/** Shell command to get the STA IPv6 configuration */
SHELL_CMD_EXPORT_ALIAS(lwip_shell_sta_ip6, net_sta_ip6, net_sta_ip6. Display the IPv6 Addresses);

int32_t W6X_Shell_Net_Hostname(int32_t argc, char **argv)
{
  struct netif *netif = netif_get_interface(NETIF_STA);
  static char hostname[34] = {0};

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get the host name */
    const char *current_hostname = netif_get_hostname(netif);
    if (current_hostname != NULL)
    {
      SHELL_PRINTF("Host name : %s\n", current_hostname);
    }
    else
    {
      SHELL_PRINTF("Get host name failed\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 2)
  {
    /* Check the host name length */
    if (strlen(argv[1]) > 33)
    {
      SHELL_E("Host name maximum length is 32\n");
      SHELL_PRINTF("Set host name failed\n");
      return SHELL_STATUS_ERROR;
    }

    /* Set the host name */
    strncpy(hostname, argv[1], sizeof(hostname) - 1);
    netif_set_hostname(netif, hostname);
    SHELL_PRINTF("Host name set successfully\n");
  }
  else
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get/set the hostname */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_Hostname, net_hostname, net_hostname [ hostname ]);
#endif /* SHELL_CMD_LEVEL */

void lwip_dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
  if (ipaddr)
  {
    SHELL_PRINTF("IP address: %s \n", ipaddr_ntoa(ipaddr));
  }
  else
  {
    SHELL_E("DNS Lookup failed\n");
  }
}

int32_t W6X_Shell_Net_ResolveHostAddress(int32_t argc, char **argv)
{
  int32_t err = 0;
  ip_addr_t resolved_ip;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the IP address from the host name */
  err = dns_gethostbyname(argv[1], &resolved_ip, lwip_dns_lookup_callback, NULL);
  if (err == ERR_OK)
  {
    /* IP address was found in cache, callback will NOT be called */
    SHELL_PRINTF("IP address: %s\n", ipaddr_ntoa(&resolved_ip));
  }
  else if (err == ERR_INPROGRESS)
  {
    /* Lookup is in progress, callback will be called
     * Do nothing here, wait for callback */
  }
  else
  {
    SHELL_E("DNS Lookup failed\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get the IP address from the host name */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_ResolveHostAddress, dnslookup, dnslookup <hostname>);
#endif /* SHELL_CMD_LEVEL */
