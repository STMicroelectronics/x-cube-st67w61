/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lwip.c
  * @author  GPM Application Team
  * @brief   This file provides initialization code for LwIP middleware.
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
#include "lwip/init.h"
#if (defined ( __CC_ARM ) || defined (__ARMCC_VERSION))  /* MDK ARM Compiler */
#include "lwip/sio.h"
#endif /* MDK ARM Compiler */
#include <lwip/api.h>
#include <lwip/def.h>
#include <lwip/pbuf.h>
#include <lwip/dns.h>
#include <lwip/ethip6.h>
#include <lwip/dhcp6.h>
#include <lwip/netifapi.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>

#include "w6x_api.h"
#include "main.h"
#include "lwip_netif.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/** Maximum transfer unit */
#define LLC_ETHER_MTU           1500

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Network interface instances */
static struct netif *netif[NETIF_MAX] = {NULL};
/** Last IP addresses assigned to the network interfaces */
static ip4_addr_t last_ipv4_addr[NETIF_MAX];
#if (LWIP_IPV6 == 1)
/** Last IPv6 addresses assigned to the network interfaces */
static ip6_addr_t last_ipv6_addr[NETIF_MAX][LWIP_IPV6_NUM_ADDRESSES];
#endif /* LWIP_IPV6 */

#if ((LWIP_IPV4 == 1) && (LWIP_DHCP == 1))
static TimerHandle_t dhcp_timer = NULL;
#endif /* LWIP_IPV4 & LWIP_DHCP */

#if ((LWIP_IPV6 == 1) && (LWIP_IPV6_DHCP6 == 1))
static TimerHandle_t dhcp6_timer = NULL;
#endif /* LWIP_IPV6 & LWIP_IPV6_DHCP6 */

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Callback to bring the network link up
  * @return 0 on success, error code otherwise
  */
static int32_t netif_link_sta_up_cb(void);

/**
  * @brief  Callback to bring the network link down
  * @return 0 on success, error code otherwise
  */
static int32_t netif_link_sta_down_cb(void);

/**
  * @brief  Timer callback function
  * @param  handle: Timer handle
  */
static void __timer_callback(TimerHandle_t handle);

/**
  * @brief  Starts the DHCP client
  * @param  netif: Pointer to the network interface
  * @param  timeout: Timeout duration
  * @return 0 on success, error code otherwise
  */
static int32_t netif_dhcp_start(struct netif *netif, uint32_t timeout);

#if (LWIP_IPV6 == 1)
/**
  * @brief  Starts the DHCPv6 client
  * @param  netif: Pointer to the network interface
  * @param  timeout: Timeout duration
  * @return 0 on success, error code otherwise
  */
static int32_t netif_dhcp6_start(struct netif *netif, uint32_t timeout);
#endif /* LWIP_IPV6 */

/**
  * @brief  DHCP client callback when the DHCP process is done
  * @param  dhcp_timer: Pointer to the DHCP timer handle
  * @return 0 on success, error code otherwise
  */
static int32_t netif_dhcp_done(TimerHandle_t *dhcp_timer);

/**
  * @brief  Callback function to handle network interface status updates
  * @param  netif: Pointer to the network interface structure
  */
static void netif_status_callback(struct netif *netif);

/**
  * @brief  Initializes the network interface structure for SPI-based communication
  * @param  netif: Pointer to the network interface structure
  * @return ERR_OK on successful initialization
  */
static err_t netif_net_init(struct netif *netif);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/**
  * LwIP initialization function
  */
int32_t MX_LWIP_Init(void)
{
  W6X_Net_if_cb_t net_if_cb =
  {
    .link_sta_up_fn = netif_link_sta_up_cb,
    .link_sta_down_fn = netif_link_sta_down_cb,
  };
  uint8_t mac[6] = {0};
  memset(last_ipv4_addr, 0, sizeof(last_ipv4_addr));
#if (LWIP_IPV6 == 1)
  memset(last_ipv6_addr, 0, sizeof(last_ipv6_addr));
#endif /* LWIP_IPV6 */

  /* Initialize the LwIP stack with RTOS */
  tcpip_init(NULL, NULL);

  if (net_if_init(&net_if_cb) != 0)
  {
    LogError("Failed to init the LwIP network interface\n");
    return -1;
  }

  netif[W6X_NET_IF_STA] = pvPortMalloc(sizeof(struct netif));
  if (netif[W6X_NET_IF_STA] == NULL)
  {
    return -1;
  }
  memset(netif[W6X_NET_IF_STA], 0, sizeof(struct netif));

  /* Add netif for station interface */
  if (netifapi_netif_add(netif[W6X_NET_IF_STA], NULL, NULL, NULL, NULL, netif_net_init, tcpip_input) != ERR_OK)
  {
    LogError("Failed to add netif\n");
    return -1;
  }
  netif[W6X_NET_IF_STA]->hwaddr_len = ETHARP_HWADDR_LEN;
  W6X_WiFi_Station_GetMACAddress(mac);
  memcpy(netif[W6X_NET_IF_STA]->hwaddr, mac, ETHARP_HWADDR_LEN);
  netif[W6X_NET_IF_STA]->hostname = "ST67W61_WiFi";

  /* Set callback to be called when interface is brought up/down or address is changed while up */
  netif_set_status_callback(netif[W6X_NET_IF_STA], netif_status_callback);

  /* Registers the default network interface */
  netifapi_netif_set_default(netif[W6X_NET_IF_STA]);

  /* When the netif is fully configured this function must be called */
  netif_set_up(netif[NETIF_STA]);

  /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
  return 0;
}

struct netif *netif_get_interface(uint32_t link_id)
{
  return netif[link_id];
}

static int32_t netif_link_sta_up_cb(void)
{
  int32_t ret = 0;
  LogInfo("\nNetif : Link is up\n");
  if (netif[NETIF_STA] == NULL)
  {
    return -1;
  }

  netif_set_link_up(netif[NETIF_STA]);

#if (LWIP_IPV6 == 1)
  /* Assign a link local address via the mac address */
  netif_create_ip6_linklocal_address(netif[NETIF_STA], 0);
  netif_set_ip6_autoconfig_enabled(netif[NETIF_STA], 1);
#if (LWIP_IPV6_MLD == 1)
  netif_set_flags(netif[NETIF_STA], NETIF_FLAG_MLD6);
  if (netif[NETIF_STA]->mld_mac_filter != NULL)
  {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif[NETIF_STA]->mld_mac_filter(netif[NETIF_STA], &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6_MLD */

  netif_dhcp6_start(netif[NETIF_STA], 15000);
#endif /* LWIP_IPV6 */

  netif_dhcp_start(netif[NETIF_STA], 15000);

  return ret;
}

static int32_t netif_link_sta_down_cb(void)
{
  LogInfo("\nNetif : Link is down\n");
  if (netif[NETIF_STA] == NULL)
  {
    return -1;
  }

  netif_set_link_down(netif[NETIF_STA]);
  ip4_addr_set_zero(&last_ipv4_addr[NETIF_STA]);
#if (LWIP_IPV6 == 1)
  for (int32_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
  {
    ip6_addr_set_zero(&last_ipv6_addr[NETIF_STA][i]);
    netif_ip6_addr_set_state(netif[NETIF_STA], i, IP6_ADDR_INVALID);
  }
#endif /* LWIP_IPV6 */
  return 0;
}

static void __timer_callback(TimerHandle_t handle)
{
}

static int32_t netif_dhcp_start(struct netif *netif, uint32_t timeout)
{
#if ((LWIP_IPV4 == 1) && (LWIP_DHCP == 1))
  if (netif == NULL)
  {
    return -1;
  }
  if (dhcp_timer)
  {
    return 0;
  }

  netifapi_dhcp_release(netif);
  netifapi_dhcp_start(netif);

  dhcp_timer = xTimerCreate("dhcp", timeout, pdFALSE, NULL, __timer_callback);
  if (dhcp_timer)
  {
    xTimerStart(dhcp_timer, 0);
  }
#endif /* LWIP_IPV4 & LWIP_DHCP */
  return 0;
}

static int32_t netif_dhcp_done(TimerHandle_t *dhcp_timer)
{
  if ((dhcp_timer != NULL) && (*dhcp_timer))
  {
    xTimerStop(*dhcp_timer, 0);
    xTimerDelete(*dhcp_timer, 0);
    *dhcp_timer = NULL;
  }
  return 0;
}

#if (LWIP_IPV6 == 1)
static int32_t netif_dhcp6_start(struct netif *netif, uint32_t timeout)
{
#if (LWIP_IPV6_DHCP6 == 1)
  if (netif == NULL)
  {
    return -1;
  }

  dhcp6_enable_stateless(netif);
  dhcp6_timer = xTimerCreate("dhcp6", timeout, pdFALSE, NULL, __timer_callback);
  if (dhcp6_timer)
  {
    xTimerStart(dhcp6_timer, 0);
  }
#endif /* LWIP_IPV6_DHCP6 */
  return 0;
}
#endif /* LWIP_IPV6 */

int32_t print_ipv4_addresses(struct netif *netif)
{
  if (!netif_is_link_up(netif))
  {
    LogError("Station is not connected. Connect to an Access Point before querying IPs\n");
    return -1;
  }
  else
  {
    if (!ip4_addr_isany(netif_ip4_addr(netif)))
    {
      LogInfo("STA IP address : %s\n", ipaddr_ntoa(netif_ip_addr4(netif)));
      LogInfo("Gateway :        %s\n", ipaddr_ntoa(netif_ip_gw4(netif)));
      LogInfo("Netmask :        %s\n", ipaddr_ntoa(netif_ip_netmask4(netif)));
    }
    else
    {
      return -1;
    }
  }
  return 0;
}

int32_t print_ipv6_addresses(struct netif *netif)
{
  if (!netif_is_link_up(netif))
  {
    LogError("Station is not connected. Connect to an Access Point before querying IPs\n");
    return -1;
  }
#if (LWIP_IPV6 == 1)
  else
  {
    for (int32_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
    {
      LogInfo("IPv6 address %d : %s\n", i, ip6addr_ntoa(netif_ip6_addr(netif, i)));
    }
  }
#endif /* LWIP_IPV6 */
  return 0;
}

#if (defined ( __CC_ARM ) || defined (__ARMCC_VERSION))  /* MDK ARM Compiler */
/**
  * Opens a serial device for communication.
  *
  * @param devnum device number
  * @return handle to serial device if successful, NULL otherwise
  */
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd;

  /* USER CODE BEGIN 7 */
  sd = 0; /* dummy code */
  /* USER CODE END 7 */

  return sd;
}

/**
  * Sends a single character to the serial device.
  *
  * @param ch character to send
  * @param fd serial device handle
  *
  * @note This function will block until the character can be sent.
  */
void sio_send(u8_t ch, sio_fd_t fd)
{
  /* USER CODE BEGIN 8 */
  /* USER CODE END 8 */
}

/**
  * Reads from the serial device.
  *
  * @param fd serial device handle
  * @param data pointer to data buffer for receiving
  * @param len maximum length (in bytes) of data to receive
  * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
  *
  * @note This function will block until data can be received. The blocking
  * can be cancelled by calling sio_read_abort().
  */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

  /* USER CODE BEGIN 9 */
  recved_bytes = 0; /* dummy code */
  /* USER CODE END 9 */
  return recved_bytes;
}

/**
  * Tries to read from the serial device. Same as sio_read but returns
  * immediately if no data is available and never blocks.
  *
  * @param fd serial device handle
  * @param data pointer to data buffer for receiving
  * @param len maximum length (in bytes) of data to receive
  * @return number of bytes actually received
  */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

  /* USER CODE BEGIN 10 */
  recved_bytes = 0; /* dummy code */
  /* USER CODE END 10 */
  return recved_bytes;
}
#endif /* MDK ARM Compiler */

/**
  * @brief  Returns the current time in milliseconds
  *         when LWIP_TIMERS == 1 and NO_SYS == 1
  * @retval Current Time value
  */
u32_t sys_now(void)
{
  return HAL_GetTick();
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void netif_status_callback(struct netif *netif)
{
  if (!netif_is_link_up(netif))
  {
    return;
  }

  if (!ip4_addr_isany(netif_ip4_addr(netif)) && !ip4_addr_cmp(netif_ip4_addr(netif), &last_ipv4_addr[NETIF_STA]))
  {
    print_ipv4_addresses(netif);
    last_ipv4_addr[NETIF_STA] = *netif_ip4_addr(netif);
    netif_dhcp_done(&dhcp_timer);
  }

#if (LWIP_IPV6 == 1)
  for (int32_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
  {
    if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)) &&
        !ip6_addr_cmp(netif_ip6_addr(netif, i), &last_ipv6_addr[NETIF_STA][i]))
    {
      /* print only the new/changed address */
      LogInfo("IPv6 address %d : %s\n", i, ip6addr_ntoa(netif_ip6_addr(netif, i)));
      last_ipv6_addr[NETIF_STA][i] = *netif_ip6_addr(netif, i);
    }
  }
#endif /* LWIP_IPV6 */
}

static err_t netif_net_init(struct netif *netif)
{
  netif->name[0] = 'w';
  netif->name[1] = 'l';

  /* set netif maximum transfer unit */
#if LWIP_IGMP
  netif->flags = NETIF_FLAG_IGMP;
#endif /* LWIP_IGMP */
  netif->mtu = LLC_ETHER_MTU;

#if (LWIP_IPV6 == 1)
  netif->flags |= NETIF_FLAG_UP | NETIF_FLAG_BROADCAST |
                  NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_MLD6;
  netif->output_ip6 = ethip6_output;
#else
  netif->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#endif /* LWIP_IPV6 */

  netif->output = etharp_output;
  netif->linkoutput = net_if_output;

  return ERR_OK;
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
