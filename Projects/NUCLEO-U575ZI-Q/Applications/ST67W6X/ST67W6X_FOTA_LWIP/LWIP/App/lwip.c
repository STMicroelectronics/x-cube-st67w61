/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lwip.c
  * @author  ST67 Application Team
  * @brief   This file provides initialization code for LwIP middleware.
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

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "main.h"
#include "lwip_netif.h"
#include "dhcp_server.h"
#include "app_config.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
  * @brief  Structure to store the MAC and assigned IPv4 address for stations connected to soft-AP
  */
typedef struct
{
  uint8_t used;           /*!< Indicates if the entry is used */
  uint8_t ipv4_logged;    /*!< Indicates if the IPv4 address is logged */
  uint8_t mac[6];         /*!< MAC address of the station */
  ip4_addr_t ipv4_addr;   /*!< Assigned IPv4 address */
} ap_sta_ipv4_entry_t;

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/** Maximum transfer unit */
#define LLC_ETHER_MTU           1500

/** DHCP Timeout */
#define DHCP_TIMEOUT            15000

/** Size of the table to store stations connected to soft-AP with their MAC and assigned IP */
#define AP_STA_IPV4_TABLE_SIZE  (sizeof(ap_sta_ipv4_entry_t) * W6X_WIFI_MAX_CONNECTED_STATIONS)

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Network interface instances */
static struct netif *netif_usr_list[NETIF_MAX];

/** Last IP addresses assigned to the network interfaces */
static ip4_addr_t last_ipv4_addr[NETIF_MAX];
#if (LWIP_IPV6 == 1)
/** Last IPv6 addresses assigned to the network interfaces */
static ip6_addr_t last_ipv6_addr[NETIF_MAX][LWIP_IPV6_NUM_ADDRESSES];
#endif /* LWIP_IPV6 */

#if ((LWIP_IPV4 == 1) && (LWIP_DHCP == 1))
/** Timer handle for DHCP client */
static TimerHandle_t lwip_dhcp_timer = NULL;
#endif /* LWIP_IPV4 & LWIP_DHCP */

#if ((LWIP_IPV6 == 1) && (LWIP_IPV6_DHCP6 == 1))
/** Timer handle for DHCPv6 client */
static TimerHandle_t lwip_dhcp6_timer = NULL;
#endif /* LWIP_IPV6 & LWIP_IPV6_DHCP6 */

/** Table to store stations connected to soft-AP with their MAC and assigned IP */
static ap_sta_ipv4_entry_t *ap_sta_ipv4_table = NULL;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Callback to bring the network sta link up.
  * @return 0 on success, error code otherwise
  */
static int32_t netif_link_sta_up_cb(void);

/**
  * @brief  Callback to bring the network sta link down.
  * @return 0 on success, error code otherwise
  */
static int32_t netif_link_sta_down_cb(void);

/**
  * @brief  Callback function to handle status updates for WiFi AP interface.
  * @param  netif_cur: Pointer to the network interface structure.
  */
static void wifi_ap_status_callback(struct netif *netif_cur);

/**
  * @brief  Callback to bring the network soft-ap link up.
  * @return 0 on success, error code otherwise
  */
static int32_t netif_link_ap_up_cb(void);

/**
  * @brief  Callback to bring the network soft-ap link down.
  * @return 0 on success, error code otherwise
  */
static int32_t netif_link_ap_down_cb(void);

/**
  * @brief  Timer callback function
  * @param  handle: Timer handle
  */
static void lwip_timer_callback(TimerHandle_t handle);

/**
  * @brief  DHCP client callback when the DHCP process is done
  * @param  dhcp_timer: Pointer to the DHCP timer handle
  * @return 0 on success, error code otherwise
  */
static int32_t netif_dhcp_done(TimerHandle_t *dhcp_timer);

/**
  * @brief  Callback function to handle network interface status updates
  * @param  netif_cur: Pointer to the network interface structure
  */
static void netif_status_callback(struct netif *netif_cur);

/**
  * @brief  Initializes the network interface structure for SPI-based communication
  * @param  netif_cur: Pointer to the network interface structure
  * @return ERR_OK on successful initialization
  */
static err_t netif_net_init(struct netif *netif_cur);

/**
  * @brief  Clear the table of stations connect to soft-AP
  */
static void ap_sta_ipv4_table_clear(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/**
  * @brief  LwIP initialization function
  * @retval int32_t: 0 on success, -1 on failure
  */
int32_t MX_LWIP_Init(void)
{
  W6X_Net_if_cb_t net_if_cb =
  {
    .link_sta_up_fn = netif_link_sta_up_cb,
    .link_sta_down_fn = netif_link_sta_down_cb,
    .link_ap_up_fn = netif_link_ap_up_cb,
    .link_ap_down_fn = netif_link_ap_down_cb,
  };
  uint8_t mac[6] = {0};
  (void)memset(last_ipv4_addr, 0, sizeof(last_ipv4_addr));
#if (LWIP_IPV6 == 1)
  (void)memset(last_ipv6_addr, 0, sizeof(last_ipv6_addr));
#endif /* LWIP_IPV6 */

  /* Initialize the LwIP stack with RTOS */
  tcpip_init(NULL, NULL);

  netif_usr_list[W6X_NET_IF_STA] = pvPortMalloc(sizeof(struct netif));
  if (netif_usr_list[W6X_NET_IF_STA] == NULL)
  {
    return -1;
  }
  (void)memset(netif_usr_list[W6X_NET_IF_STA], 0, sizeof(struct netif));

  netif_usr_list[W6X_NET_IF_AP] = pvPortMalloc(sizeof(struct netif));
  if (netif_usr_list[W6X_NET_IF_AP] == NULL)
  {
    return -1;
  }
  (void)memset(netif_usr_list[W6X_NET_IF_AP], 0, sizeof(struct netif));

  /* Add netif for station interface */
  if (netifapi_netif_add(netif_usr_list[W6X_NET_IF_STA], NULL,
                         NULL, NULL, NULL, netif_net_init, tcpip_input) != ERR_OK)
  {
    LogError("Failed to add netif\n");
    return -1;
  }
  netif_usr_list[W6X_NET_IF_STA]->hwaddr_len = ETHARP_HWADDR_LEN;
  (void)W6X_WiFi_Station_GetMACAddress(mac);
  (void)memcpy(netif_usr_list[W6X_NET_IF_STA]->hwaddr, mac, ETHARP_HWADDR_LEN);
  netif_usr_list[W6X_NET_IF_STA]->hostname = "ST67W61_WiFi";

  /* Set callback to be called when interface is brought up/down or address is changed while up */
  netif_set_status_callback(netif_usr_list[W6X_NET_IF_STA], netif_status_callback);

  /* Add netif for access point interface */
  if (netifapi_netif_add(netif_usr_list[W6X_NET_IF_AP],
                         NULL, NULL, NULL, NULL, netif_net_init, tcpip_input) != ERR_OK)
  {
    LogError("Failed to add netif\n");
    return -1;
  }
  netif_usr_list[W6X_NET_IF_AP]->hwaddr_len = ETHARP_HWADDR_LEN;
  (void)W6X_WiFi_AP_GetMACAddress(mac);
  (void)memcpy(netif_usr_list[W6X_NET_IF_AP]->hwaddr, mac, ETHARP_HWADDR_LEN);
  netif_usr_list[W6X_NET_IF_AP]->hostname = "ST67W61_AP_WiFi";

  /* Registers the default network interface */
  (void)netifapi_netif_set_default(netif_usr_list[W6X_NET_IF_STA]);

  /* When the netif is fully configured this function must be called */
  netif_set_up(netif_usr_list[NETIF_STA]);

  if (net_if_init(&net_if_cb) != 0)
  {
    LogError("Failed to init the LwIP network interface\n");
    return -1;
  }

  /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
  return 0;
}

struct netif *netif_get_interface(uint32_t link_id)
{
  return netif_usr_list[link_id];
}

static int32_t netif_link_sta_up_cb(void)
{
  int32_t ret = 0;
  if (netif_usr_list[NETIF_STA] == NULL)
  {
    return -1;
  }
  LogInfo("\nNetif : Link is up\n");

  netif_set_link_up(netif_usr_list[NETIF_STA]);

#if (LWIP_IPV6 == 1)
  /* Assign a link local address via the mac address */
  netif_create_ip6_linklocal_address(netif_usr_list[NETIF_STA], 1);
  netif_set_ip6_autoconfig_enabled(netif_usr_list[NETIF_STA], 1);
#if (LWIP_IPV6_MLD == 1)
  netif_set_flags(netif_usr_list[NETIF_STA], NETIF_FLAG_MLD6);
  if (netif_usr_list[NETIF_STA]->mld_mac_filter != NULL)
  {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    (void)netif_usr_list[NETIF_STA]->mld_mac_filter(netif_usr_list[NETIF_STA], &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6_MLD */

#if (LWIP_IPV6_DHCP6 == 1)
  if (lwip_dhcp6_timer == NULL)
  {
    dhcp6_enable_stateless(netif_usr_list[NETIF_STA]);
    lwip_dhcp6_timer = xTimerCreate("dhcp6", pdMS_TO_TICKS(DHCP_TIMEOUT), pdFALSE, NULL, lwip_timer_callback);
    if (lwip_dhcp6_timer)
    {
      xTimerStart(lwip_dhcp6_timer, 0);
    }
  }
#endif /* LWIP_IPV6_DHCP6 */
#endif /* LWIP_IPV6 */

#if ((LWIP_IPV4 == 1) && (LWIP_DHCP == 1))
  if (lwip_dhcp_timer == NULL)
  {
    (void)netifapi_dhcp_release(netif_usr_list[NETIF_STA]);
    (void)netifapi_dhcp_start(netif_usr_list[NETIF_STA]);

    lwip_dhcp_timer = xTimerCreate("dhcp", pdMS_TO_TICKS(DHCP_TIMEOUT),
                                   pdFALSE, NULL, lwip_timer_callback);
    if (lwip_dhcp_timer != NULL)
    {
      xTimerStart(lwip_dhcp_timer, 0);
    }
  }
#endif /* LWIP_IPV4 & LWIP_DHCP */
  return ret;
}

static int32_t netif_link_sta_down_cb(void)
{
  if (netif_usr_list[NETIF_STA] == NULL)
  {
    return -1;
  }
  LogInfo("\nNetif : Link is down\n");

#if (LWIP_IPV4 == 1)
  netif_set_link_down(netif_usr_list[NETIF_STA]);
#if (LWIP_DHCP == 1)
  (void)netifapi_dhcp_release_and_stop(netif_usr_list[NETIF_STA]);
  (void)netif_dhcp_done(&lwip_dhcp_timer);
#else
  /* remove IP address from interface */
  netif_set_addr(netif_usr_list[NETIF_STA], IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
#endif /* LWIP_DHCP */
  ip4_addr_set_zero(&last_ipv4_addr[NETIF_STA]);
#endif /* LWIP_IPV4 */

#if (LWIP_IPV6 == 1)
  for (int8_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
  {
    ip6_addr_set_zero(&last_ipv6_addr[NETIF_STA][i]);
    netif_ip6_addr_set_state(netif_usr_list[NETIF_STA], i, IP6_ADDR_INVALID);
  }
#if (LWIP_IPV6_DHCP6 == 1)
  netif_dhcp_done(&lwip_dhcp6_timer);
#endif /* LWIP_IPV6_DHCP6 */
#endif /* LWIP_IPV6 */

  return 0;
}

int32_t ap_sta_ipv4_table_add_entry(const uint8_t *mac)
{
  int32_t count;

  if ((mac == NULL) || (ap_sta_ipv4_table == NULL))
  {
    return -1;
  }

  /* Verify if the MAC is already in the table */
  for (count = 0; count < (int32_t)W6X_WIFI_MAX_CONNECTED_STATIONS; count++)
  {
    if ((ap_sta_ipv4_table[count].used != 0U) &&
        (memcmp(ap_sta_ipv4_table[count].mac, mac, sizeof(ap_sta_ipv4_table[count].mac)) == 0))
    {
      return count;
    }
  }

  /* Add the MAC in the table otherwise */
  for (count = 0; count < (int32_t)W6X_WIFI_MAX_CONNECTED_STATIONS; count++)
  {
    if (ap_sta_ipv4_table[count].used == 0U)
    {
      (void)memset(&ap_sta_ipv4_table[count], 0, sizeof(ap_sta_ipv4_table[count]));
      ap_sta_ipv4_table[count].used = 1U;
      (void)memcpy(ap_sta_ipv4_table[count].mac, mac, sizeof(ap_sta_ipv4_table[count].mac));
      return count;
    }
  }

  return -1;
}

int32_t ap_sta_ipv4_table_del_entry(const uint8_t *mac)
{
  if ((mac == NULL) || (ap_sta_ipv4_table == NULL))
  {
    return -1;
  }

  /* Delete the MAC from the table */
  for (int32_t count = 0; count < (int32_t)W6X_WIFI_MAX_CONNECTED_STATIONS; count++)
  {
    if ((ap_sta_ipv4_table[count].used != 0U) &&
        (memcmp(ap_sta_ipv4_table[count].mac, mac, sizeof(ap_sta_ipv4_table[count].mac)) == 0))
    {
      (void)memset(&ap_sta_ipv4_table[count], 0, sizeof(ap_sta_ipv4_table[count]));
      return count;
    }
  }
  return -1;
}

static void ap_sta_ipv4_table_clear(void)
{
  if (ap_sta_ipv4_table != NULL)
  {
    (void)memset(ap_sta_ipv4_table, 0, AP_STA_IPV4_TABLE_SIZE);
  }
}

static void wifi_ap_status_callback(struct netif *netif_cur)
{
  const ip4_addr_t *ipv4_addr;

  if (netif_cur == NULL)
  {
    return;
  }

  ipv4_addr = netif_ip4_addr(netif_cur);
  if ((ipv4_addr == NULL) || ip4_addr_isany(ipv4_addr))
  {
    return;
  }

  for (int32_t count = 0; count < (int32_t)W6X_WIFI_MAX_CONNECTED_STATIONS; count++)
  {
    if ((ap_sta_ipv4_table[count].used != 0U)
        && (memcmp(ap_sta_ipv4_table[count].mac, netif_cur->hwaddr, sizeof(ap_sta_ipv4_table[count].mac)) == 0))
    {
      ap_sta_ipv4_table[count].ipv4_addr = *ipv4_addr;
      if (ap_sta_ipv4_table[count].ipv4_logged == 0U)
      {
        ap_sta_ipv4_table[count].ipv4_logged = 1U;

        LogInfo("Station connected to soft-AP got an IP:\"%02x:%02x:%02x:%02x:%02x:%02x\",\"%s\"\r\n",
                netif_cur->hwaddr[0], netif_cur->hwaddr[1], netif_cur->hwaddr[2],
                netif_cur->hwaddr[3], netif_cur->hwaddr[4], netif_cur->hwaddr[5],
                ip4addr_ntoa(ipv4_addr));
        return;
      }
    }
  }

  return;
}

static int32_t netif_link_ap_up_cb(void)
{
  if (netif_usr_list[NETIF_AP] == NULL)
  {
    return -1;
  }
  LogInfo("\nNetif AP : Link is up\n");

  netif_set_link_up(netif_usr_list[NETIF_AP]);

  if (ap_sta_ipv4_table != NULL)
  {
    vPortFree(ap_sta_ipv4_table);
    ap_sta_ipv4_table = NULL;
  }
  ap_sta_ipv4_table = (ap_sta_ipv4_entry_t *)pvPortMalloc(AP_STA_IPV4_TABLE_SIZE);
  if (ap_sta_ipv4_table == NULL)
  {
    return -1;
  }
  ap_sta_ipv4_table_clear();

  dhcpd_start(netif_usr_list[NETIF_AP], -1, -1);
  vTaskDelay(pdMS_TO_TICKS(100));
  (void)dhcpd_status_callback_set(netif_usr_list[NETIF_AP], wifi_ap_status_callback);

  return 0;
}

static int32_t netif_link_ap_down_cb(void)
{
  if (netif_usr_list[NETIF_AP] == NULL)
  {
    return -1;
  }
  LogInfo("\nNetif AP : Link is down\n");

  dhcpd_stop(netif_usr_list[NETIF_AP]);

  ap_sta_ipv4_table_clear();
  if (ap_sta_ipv4_table != NULL)
  {
    vPortFree(ap_sta_ipv4_table);
    ap_sta_ipv4_table = NULL;
  }

  netif_set_link_down(netif_usr_list[NETIF_AP]);

  return 0;
}

static void lwip_timer_callback(TimerHandle_t handle)
{
}

static int32_t netif_dhcp_done(TimerHandle_t *dhcp_timer)
{
  if ((dhcp_timer != NULL) && (*dhcp_timer != NULL))
  {
    xTimerStop(*dhcp_timer, 0);
    xTimerDelete(*dhcp_timer, 0);
    *dhcp_timer = NULL;
  }
  return 0;
}

int32_t aplist_sta(W6X_WiFi_Connected_Sta_t *ConnectedSta)
{
  ip4_addr_t ipaddr;

  /* GET Method */
  if (W6X_WiFi_AP_ListConnectedStations(ConnectedSta) != W6X_STATUS_OK)
  {
    return -1;
  }

  for (uint32_t i = 0; i < ConnectedSta->Count; i++)
  {
    W6X_WiFi_Connected_Sta_Info_t *res = &ConnectedSta->STA[i];
    if (ERR_OK == dhcpd_get_ip_by_mac(netif_usr_list[NETIF_AP], res->MAC, &ipaddr))
    {
      uint32_t ip = ipaddr.addr;
      res->IP[0] = (uint8_t)((ip >> 24U) & 0xFFU);
      res->IP[1] = (uint8_t)((ip >> 16U) & 0xFFU);
      res->IP[2] = (uint8_t)((ip >> 8U) & 0xFFU);
      res->IP[3] = (uint8_t)(ip & 0xFFU);
    }
  }

  return ERR_OK;
}

int32_t print_ipv4_addresses(struct netif *netif_cur)
{
  if (!netif_is_link_up(netif_cur))
  {
    LogError("Station is not connected. Connect to an Access Point before querying IPs\n");
    return -1;
  }
  else
  {
    if (!ip4_addr_isany(netif_ip4_addr(netif_cur)))
    {
      LogInfo("STA IP :\n");
      LogInfo("IP :              %s\n", ipaddr_ntoa(netif_ip_addr4(netif_cur)));
      LogInfo("Gateway :         %s\n", ipaddr_ntoa(netif_ip_gw4(netif_cur)));
      LogInfo("Netmask :         %s\n", ipaddr_ntoa(netif_ip_netmask4(netif_cur)));
    }
    else
    {
      return -1;
    }
  }
  return 0;
}

int32_t print_ipv6_addresses(struct netif *netif_cur)
{
  if (!netif_is_link_up(netif_cur))
  {
    LogError("Station is not connected. Connect to an Access Point before querying IPs\n");
    return -1;
  }
#if (LWIP_IPV6 == 1)
  else
  {
    LogInfo("IPv6 link-local : %s\n", ip6addr_ntoa(netif_ip6_addr(netif_cur, 0)));
    LogInfo("IPv6 global 1   : %s\n", ip6addr_ntoa(netif_ip6_addr(netif_cur, 1)));
    LogInfo("IPv6 global 2   : %s\n", ip6addr_ntoa(netif_ip6_addr(netif_cur, 2)));
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
static void netif_status_callback(struct netif *netif_cur)
{
#if (LWIP_IPV6 == 1)
  static const char *ipv6_addr_labels[3] =
  {
    "link-local",
    "global 1  ",
    "global 2  "
  };
#endif /* LWIP_IPV6 */

  if (!netif_is_link_up(netif_cur))
  {
    return;
  }

  if (!ip4_addr_isany(netif_ip4_addr(netif_cur)) &&
      !ip4_addr_cmp(netif_ip4_addr(netif_cur), &last_ipv4_addr[NETIF_STA]))
  {
    (void)print_ipv4_addresses(netif_cur);
    last_ipv4_addr[NETIF_STA] = *netif_ip4_addr(netif_cur);
    (void)netif_dhcp_done(&lwip_dhcp_timer);
  }

#if (LWIP_IPV6 == 1)
  for (int32_t i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
  {
    if (ip6_addr_isvalid(netif_ip6_addr_state(netif_cur, i)) &&
        !ip6_addr_cmp(netif_ip6_addr(netif_cur, i), &last_ipv6_addr[NETIF_STA][i]))
    {
      /* print only the new/changed address */
      LogInfo("IPv6 %s : %s\n", ipv6_addr_labels[i], ip6addr_ntoa(netif_ip6_addr(netif_cur, i)));
      last_ipv6_addr[NETIF_STA][i] = *netif_ip6_addr(netif_cur, i);
#if (LWIP_IPV6_DHCP6 == 1)
      netif_dhcp_done(&lwip_dhcp6_timer);
#endif /* LWIP_IPV6_DHCP6 */
    }
  }
#endif /* LWIP_IPV6 */
}

static err_t netif_net_init(struct netif *netif_cur)
{
  netif_cur->name[0] = 'w';
  netif_cur->name[1] = 'l';

  /* set netif maximum transfer unit */
#if LWIP_IGMP
  netif_cur->flags = NETIF_FLAG_IGMP;
#endif /* LWIP_IGMP */
  netif_cur->mtu = LLC_ETHER_MTU;

#if (LWIP_IPV6 == 1)
  netif_cur->flags |= NETIF_FLAG_UP | NETIF_FLAG_BROADCAST |
                      NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_MLD6;
  netif_cur->output_ip6 = ethip6_output;
#else
  netif_cur->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#endif /* LWIP_IPV6 */

  netif_cur->output = etharp_output;
  netif_cur->linkoutput = net_if_output;

  return ERR_OK;
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
