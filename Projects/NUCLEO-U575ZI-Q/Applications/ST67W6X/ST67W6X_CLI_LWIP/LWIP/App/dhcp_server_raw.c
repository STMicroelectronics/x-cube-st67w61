/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dhcp_server_raw.c
  * @author  GPM Application Team
  * @brief   A simple DHCP server implementation
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

/**
  * Portions of this file are based on RT-Thread Development Team,
  * which is licensed under the Apache-2.0 license as indicated below.
  * See https://github.com/RT-Thread/rt-thread for more information.
  *
  * Reference source:
  * https://github.com/RT-Thread/rt-thread/blob/master/components/net/lwip-dhcpd/dhcp_server_raw.c
  */

/*
 * File      : dhcp_server_raw.c
 *             A simple DHCP server implementation
 * COPYRIGHT (C) 2011-2023, Shanghai Real-Thread Technology Co., Ltd
 * http://www.rt-thread.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2014-04-01     Ren.Haibo    the first version
 * 2018-06-12     aozima       ignore DHCP_OPTION_SERVER_ID.
 */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdint.h>
#include <FreeRTOS.h>

#include <lwip/opt.h>
#include <lwip/sockets.h>
#include <lwip/inet_chksum.h>
#include <netif/etharp.h>
#include <lwip/init.h>
#include <lwip/prot/dhcp.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>

#include "dhcp_server.h"
#include "logging.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
/* Private defines -----------------------------------------------------------*/
/** Maximum number of DNS servers */
#define DHCPD_MAX_DNS_SERVER_NUM    2

/* DHCP server option */
/** DHCP client port */
#define DHCP_CLIENT_PORT            68
/** DHCP server port */
#define DHCP_SERVER_PORT            67

/* allocated client ip range */
#ifndef DHCPD_CLIENT_IP_MIN
/** Minimum offset of the DHCP IP address pool */
#define DHCPD_CLIENT_IP_MIN         2
#endif /* DHCPD_CLIENT_IP_MIN */

#ifndef DHCPD_CLIENT_IP_MAX
/** Maximum offset of the DHCP IP address pool */
#define DHCPD_CLIENT_IP_MAX         254
#endif /* DHCPD_CLIENT_IP_MAX */

#ifndef DHCPD_SERVER_IP
/** DHCP server address */
#define DHCPD_SERVER_IP             "10.19.96.1"
#endif /* DHCPD_SERVER_IP */

/* we need some routines in the DHCP of lwIP */
#undef  LWIP_DHCP
/** Enable DHCP module */
#define LWIP_DHCP                   1
#include <lwip/dhcp.h>

/** Mac address length  */
#define DHCP_MAX_HLEN               6

/** DHCP default live time */
#define DHCP_DEFAULT_LIVE_TIME      0x80510100

/** Minimum length for request before packet is parsed */
#define DHCP_MIN_REQUEST_LEN        44

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/** Lock the network interface. Unused */
#define LWIP_NETIF_LOCK(...)

/** Unlock the network interface. Unused */
#define LWIP_NETIF_UNLOCK(...)

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private typedef -----------------------------------------------------------*/
/**
  * The dhcp client node struct
  */
struct dhcp_client_node
{
  struct dhcp_client_node *next;                      /*!< Pointer to the next DHCP client node */
  u8_t chaddr[DHCP_MAX_HLEN];                         /*!< Client hardware address */
  ip4_addr_t ipaddr;                                  /*!< Client IP address */
  u32_t lease_end;                                    /*!< Lease expiration time */
};

/**
  * The dhcp server struct
  */
struct dhcp_server
{
  struct dhcp_server *next;                           /*!< Pointer to the next DHCP server */
  struct netif *netif;                                /*!< The network interface which use the dhcp server */
  struct udp_pcb *pcb;                                /*!< The UDP PCB for dhcp server */
  struct dhcp_client_node *node_list;                 /*!< The dhcp client node list */
  ip4_addr_t start;                                   /*!< The start ip address */
  ip4_addr_t end;                                     /*!< The end ip address */
  ip4_addr_t current;                                 /*!< The current ip address */
  ip4_addr_t dnsserver[DHCPD_MAX_DNS_SERVER_NUM];     /*!< The DNS server list */
  dhcpd_callback_t sta_status_cb;                     /*!< The DHCP server status callback */
};

/**
  * The dhcp server argument struct
  */
struct dhcp_server_arg
{
  struct netif *netif;                                /*!< The network interface which use the dhcp server */
  ip4_addr_t start;                                   /*!< The start ip address */
  ip4_addr_t end;                                     /*!< The end ip address */
};

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private variables ---------------------------------------------------------*/
/**
  * The dhcp server struct list.
  */
static struct dhcp_server *lw_dhcp_server;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  start dhcp server for a netif
  * @param  ctx: dhcp_server_arg struct pointer
  * @return lwIP error code
  * - ERR_OK - No error
  * - ERR_MEM - Out of memory
  */
static err_t dhcp_server_start(void *ctx);

/**
  * @brief  stop dhcp server for a netif
  * @param  netif: The netif which use dhcp server
  * @return lwIP error code
  * - ERR_OK - No error
  * - ERR_VAL - No dhcp server instance found
  */
static err_t dhcp_server_stop(struct netif *netif);

/**
  * @brief  Find a dhcp client node by ip address
  * @param dhcpserver: The dhcp server
  * @param ip: IP address
  * @return dhcp client node
  */
static struct dhcp_client_node *dhcp_client_find_by_ip(struct dhcp_server *dhcpserver, const uint8_t *ip);

/**
  * @brief  Find a dhcp client node by mac address
  * @param  dhcpserver: The dhcp server
  * @param  chaddr: Mac address
  * @param  hlen: Mac address length
  * @return dhcp client node
  */
static struct dhcp_client_node *dhcp_client_find_by_mac(struct dhcp_server *dhcpserver, const u8_t *chaddr, u8_t hlen);

/**
  * @brief  Find a dhcp client node by dhcp message
  * @param  dhcpserver: is the dhcp server
  * @param  msg: is the dhcp message
  * @param  opt_buf: is the optional buffer
  * @param  len: is the buffer length
  * @return dhcp client node
  */
static struct dhcp_client_node *dhcp_client_find(struct dhcp_server *dhcpserver, struct dhcp_msg *msg,
                                                 u8_t *opt_buf, u16_t len);

/**
  * @brief  Allocate a dhcp client node by dhcp message
  * @param  dhcpserver: is the dhcp server
  * @param  msg: is the dhcp message
  * @param  opt_buf: is the optional buffer
  * @param  len: is the buffer length
  * @return dhcp client node
  */
static struct dhcp_client_node *dhcp_client_alloc(struct dhcp_server *dhcpserver, struct dhcp_msg *msg,
                                                  u8_t *opt_buf, u16_t len);

/**
  * @brief  Find a dhcp option in dhcp message
  * @param  buf: The dhcp options buffer
  * @param  len: The dhcp options buffer length
  * @param  option: The dhcp option to find
  * @return option pointer
  */
static u8_t *dhcp_server_option_find(u8_t *buf, u16_t len, u8_t option);

/**
  * @brief  Receive a DHCP message
  * @param  arg: The dhcp server
  * @param  pcb: The udp pcb
  * @param  p: The pbuf
  * @param  recv_addr: The remote IP address
  * @param  port: The remote port
  */
static void dhcp_server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *recv_addr, u16_t port);

/**
  * @brief  Operate DNS server list
  * @param  netif: The netif which use dhcp server
  * @param  dnsserver: The DNS server address
  * @param  reset: 0-add, 1-remove
  * @return lwIP error code
  * - ERR_OK - No error
  * - ERR_VAL - No dhcp server instance found or DNS server not found
  */
static err_t dhcp_server_op_dns_server(void *netif, const ip_addr_t *dnsserver, int32_t reset);

/**
  * @brief  Clear DNS server list
  */
static void dhcp_server_clear_dns_server(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
/* Functions Definition ------------------------------------------------------*/
void dhcpd_start(struct netif *netif, int32_t start, int32_t limit)
{
  err_t res;
  char str_tmp[IP4ADDR_STRLEN_MAX] = DHCPD_SERVER_IP;
  char *p;
  int32_t start_num = DHCPD_CLIENT_IP_MIN;
  int32_t end_num = DHCPD_CLIENT_IP_MAX;
  ip4_addr_t ip_start = {0};
  ip4_addr_t ip_end = {0};
  struct dhcp_server_arg *arg;
  ip4_addr_t default_addr, default_mask, default_gw;

  netifapi_dhcp_stop(netif);

  /* If the start or limit is not set, use the default address */
  if ((start == -1) || (limit == -1) || ip4_addr_isany_val(*netif_ip4_addr(netif)))
  {
    ip4addr_aton(DHCPD_SERVER_IP, &default_addr);
    ip4addr_aton("255.255.255.0", &default_mask);
    ip4addr_aton("0.0.0.0", &default_gw); /* no gateway */
    netifapi_netif_set_addr(netif, &default_addr, &default_mask, &default_gw);
  }
  else
  {
    /* Get the netif IP address string */
    memset(str_tmp, 0, IP4ADDR_STRLEN_MAX);
    ip4addr_ntoa_r(netif_ip4_addr(netif), str_tmp, IP4ADDR_STRLEN_MAX);
    /* Calculate the start and end ip address */
    start_num = start;
    end_num = start + limit - 1;
  }

  if ((start_num > 254) || (start_num < 2) || (end_num > 254) || (end_num < 2) || (start_num >= end_num))
  {
    LogDebug("dhcp start arg error, range %d - %d \n", start_num, end_num);
    return;
  }

  /* Get the last octet of the IP address */
  p = strchr(str_tmp, '.');
  if (p)
  {
    p = strchr(p + 1, '.');
    if (p)
    {
      p = strchr(p + 1, '.');
    }
  }
  if (!p)
  {
    LogDebug("DHCPD_SERVER_IP: %s error!\n", str_tmp);
    goto _exit;
  }
  p = p + 1; /* move to xxx.xxx.xxx.^ */

  /* Set the start IP address */
  snprintf(p, IP4ADDR_STRLEN_MAX - (p - str_tmp), "%" PRIi32, start_num);
  ip4addr_aton(str_tmp, &ip_start);
  LogDebug("ip_start: [%s]\n", str_tmp);

  /* Set the end IP address */
  snprintf(p, IP4ADDR_STRLEN_MAX - (p - str_tmp), "%" PRIi32, end_num);
  ip4addr_aton(str_tmp, &ip_end);
  LogDebug("ip_end: [%s]\n", str_tmp);

  /* Allocate memory for the DHCP server argument */
  arg = (struct dhcp_server_arg *)mem_malloc(sizeof(struct dhcp_server_arg));
  if (arg == NULL)
  {
    goto _exit;
  }
  arg->netif = netif;
  arg->start = ip_start;
  arg->end = ip_end;
  /* Start the DHCP server in the tcpip thread */
  res = tcpip_callback((tcpip_callback_fn)dhcp_server_start, arg);
  if (res != ERR_OK)
  {
    mem_free(arg);
    LogDebug("dhcp_server_start res: %d.\n", res);
  }

_exit:
  LWIP_NETIF_UNLOCK();
  return;
}

void dhcpd_stop(const struct netif *netif)
{
  int32_t res;
  static char netif_name[3] = {0};

  if (!netif)
  {
    return;
  }

  /* Get the netif name */
  netif_name[0] = netif->name[0];
  netif_name[1] = netif->name[1];
  /* Stop the DHCP server in the tcpip thread */
  res = tcpip_callback((tcpip_callback_fn)dhcpd_stop_by_name, netif_name);
  if (res != ERR_OK)
  {
    LogDebug("dhcp_server_stop res: %d.\n", res);
  }
}

void dhcpd_stop_by_name(const char *netif_name)
{
  struct netif *netif = netif_list;

  LogDebug("%s: %s\n", __func__, netif_name);

  LWIP_NETIF_LOCK();
  if (strlen(netif_name) > sizeof(netif->name))
  {
    LogDebug("network interface name too long!\n");
    goto _exit;
  }

  /* Get the netif that matches the name */
  while (netif != NULL)
  {
    if (strncmp(netif_name, netif->name, sizeof(netif->name)) == 0)
    {
      break;
    }

    netif = netif->next;
    if (netif == NULL)
    {
      LogDebug("network interface: %s not found!\n", netif_name);
      break;
    }
  }

  if (netif == NULL)
  {
    goto _exit;
  }

  /* Clear DNS server list */
  dhcp_server_clear_dns_server();
  /* Stop the DHCP server */
  dhcp_server_stop(netif);

_exit:
  LWIP_NETIF_UNLOCK();
  return;
}

err_t dhcpd_status_callback_set(struct netif *netif, dhcpd_callback_t cb)
{
  struct dhcp_server *dhcp_server;

  /* If this netif is in the dhcp server list. */
  for (dhcp_server = lw_dhcp_server; dhcp_server != NULL; dhcp_server = dhcp_server->next)
  {
    if (dhcp_server->netif == netif)
    {
      break;
    }
  }
  if (NULL == dhcp_server)
  {
    LogDebug("[DHCPD] CRITICAL: no dhcp_server instance found\n");
    return ERR_VAL;
  }
  /* Set the status callback */
  dhcp_server->sta_status_cb = cb;
  return ERR_OK;
}

err_t dhcpd_get_ip_by_mac(struct netif *netif, uint8_t mac[], ip4_addr_t *ipaddr)
{
  err_t res = ERR_OK;
  struct dhcp_server *dhcp_server;
  struct dhcp_client_node *node;

  LWIP_NETIF_LOCK();

  if (netif == NULL)
  {
    res = ERR_VAL;
    goto _exit;
  }

  /* If this netif already use the dhcp server. */
  for (dhcp_server = lw_dhcp_server; dhcp_server != NULL; dhcp_server = dhcp_server->next)
  {
    if (dhcp_server->netif == netif)
    {
      break;
    }
  }
  if (dhcp_server == NULL)
  {
    res = ERR_VAL;
    goto _exit;
  }

  /* Find the dhcp client node by mac address */
  node = dhcp_client_find_by_mac(dhcp_server, mac, DHCP_MAX_HLEN);

  if (node == NULL)
  {
    res = ERR_VAL;
    goto _exit;
  }

  /* Return the IP address */
  *ipaddr = node->ipaddr;

_exit:
  LWIP_NETIF_UNLOCK();
  return res;
}

void dhcpd_clear_dns_server(void *netif)
{
  struct dhcp_server *dhcp_server;

  for (dhcp_server = lw_dhcp_server; dhcp_server != NULL; dhcp_server = dhcp_server->next)
  {
    if (dhcp_server->netif == netif)
    {
      for (int32_t i = 0; i < DHCPD_MAX_DNS_SERVER_NUM; i++)
      {
        /* Clear DNS server IP address */
        ip4_addr_set_zero(&dhcp_server->dnsserver[i]);
      }
    }
  }
}

err_t dhcpd_add_dns_server(void *netif, const ip_addr_t *dnsserver)
{
  return dhcp_server_op_dns_server(netif, dnsserver, 0);
}

err_t dhcpd_remove_dns_server(void *netif, const ip_addr_t *dnsserver)
{
  return dhcp_server_op_dns_server(netif, dnsserver, 1);
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static err_t dhcp_server_start(void *ctx)
{
  struct dhcp_server *dhcp_server;
  struct dhcp_server_arg *arg = (struct dhcp_server_arg *)ctx;
  ip4_addr_t start = arg->start, end = arg->end;
  struct netif *netif = arg->netif;
  free(ctx);

  /* If this netif already use the dhcp server. */
  for (dhcp_server = lw_dhcp_server; dhcp_server != NULL; dhcp_server = dhcp_server->next)
  {
    if (dhcp_server->netif == netif)
    {
      /* Update the ip range */
      dhcp_server->start = start;
      dhcp_server->end = end;
      dhcp_server->current = start;
      return ERR_OK;
    }
  }

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_server_start(): starting new DHCP server\n"));
  /* allocate dhcp server structure */
  dhcp_server = (struct dhcp_server *)mem_malloc(sizeof(struct dhcp_server));
  if (dhcp_server == NULL)
  {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_server_start(): could not allocate dhcp\n"));
    return ERR_MEM;
  }

  /* clear data structure */
  memset(dhcp_server, 0, sizeof(struct dhcp_server));

  /* store this dhcp server to list */
  dhcp_server->next = lw_dhcp_server;
  lw_dhcp_server = dhcp_server;
  dhcp_server->netif = netif;
  dhcp_server->node_list = NULL;
  dhcp_server->start = start;
  dhcp_server->end = end;
  dhcp_server->current = start;

  /* allocate UDP PCB */
  dhcp_server->pcb = udp_new();
  if (dhcp_server->pcb == NULL)
  {
    LWIP_DEBUGF(DHCP_DEBUG  | LWIP_DBG_TRACE, ("dhcp_server_start(): could not obtain pcb\n"));
    return ERR_MEM;
  }

  ip_set_option(dhcp_server->pcb, SOF_BROADCAST);
  /* set up local and remote port for the pcb */
  udp_bind(dhcp_server->pcb, IP_ADDR_ANY, DHCP_SERVER_PORT);
  /* set up the recv callback and argument */
  udp_recv(dhcp_server->pcb, dhcp_server_recv, dhcp_server);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_server_start(): starting DHCP server\n"));

  return ERR_OK;
}

static err_t dhcp_server_stop(struct netif *netif)
{
  struct dhcp_server *dhcp_server;

  /* If this netif is in the dhcp server list. */
  for (dhcp_server = lw_dhcp_server; dhcp_server != NULL; dhcp_server = dhcp_server->next)
  {
    if (dhcp_server->netif == netif)
    {
      break;
    }
  }

  if (NULL == dhcp_server)
  {
    LogDebug("[DHCPD] CRITICAL: no dhcp_server instance found\n");
    return ERR_VAL;
  }

  /* clean PCB first */
  if (dhcp_server->pcb)
  {
    udp_remove(dhcp_server->pcb);
  }
  /* clean linked list */
  lw_dhcp_server = NULL;
  mem_free(dhcp_server);

  return ERR_OK;
}

static struct dhcp_client_node *dhcp_client_find_by_ip(struct dhcp_server *dhcpserver, const uint8_t *ip)
{
  struct dhcp_client_node *node;
  ip4_addr_t ipaddr;
  uint32_t ipval;

  /* Copy ipaddr to avoid alignment issue */
  memcpy(&ipval, ip, sizeof(ipval));
  ip4_addr_set_u32(&ipaddr, ipval);
  for (node = dhcpserver->node_list; node != NULL; node = node->next)
  {
    if (ip4_addr_cmp(&node->ipaddr, &ipaddr))
    {
      return node;
    }
  }

  return NULL;
}

static struct dhcp_client_node *dhcp_client_find_by_mac(struct dhcp_server *dhcpserver, const u8_t *chaddr, u8_t hlen)
{
  struct dhcp_client_node *node;

  for (node = dhcpserver->node_list; node != NULL; node = node->next)
  {
    if (memcmp(node->chaddr, chaddr, hlen) == 0)
    {
      return node;
    }
  }

  return NULL;
}

static struct dhcp_client_node *dhcp_client_find(struct dhcp_server *dhcpserver, struct dhcp_msg *msg,
                                                 u8_t *opt_buf, u16_t len)
{
  u8_t *opt;
  struct dhcp_client_node *node, *m_node = NULL;

  m_node = dhcp_client_find_by_mac(dhcpserver, msg->chaddr, msg->hlen);

  opt = dhcp_server_option_find(opt_buf, len, DHCP_OPTION_REQUESTED_IP);
  if (opt != NULL)
  {
    node = dhcp_client_find_by_ip(dhcpserver, &opt[2]);
    if (node != NULL)
    {
      if (0 == memcmp(node->chaddr, msg->chaddr, msg->hlen))
      {
        return node;
      }
      else
      {
        puts("IP Found, but MAC address is NOT the same\n");
        /* return node */
      }
    }
    else
    {
      u32_t ipval;
      memcpy(&ipval, &opt[2], sizeof(ipval));
      if ((lwip_ntohl(ipval) < lwip_ntohl(dhcpserver->start.addr)) ||
          (lwip_ntohl(ipval) > lwip_ntohl(dhcpserver->end.addr)))
      {
        /* IP address exceeds the address range */
        LogWarn("IP address exceeds\n");
        return m_node;
      }
      return NULL;
    }
  }

  return m_node;
}

static struct dhcp_client_node *dhcp_client_alloc(struct dhcp_server *dhcpserver, struct dhcp_msg *msg,
                                                  u8_t *opt_buf, u16_t len)
{
  u8_t *opt;
  u32_t ipaddr;
  struct dhcp_client_node *node = NULL, *m_node = NULL;

  m_node = dhcp_client_find_by_mac(dhcpserver, msg->chaddr, msg->hlen);

  opt = dhcp_server_option_find(opt_buf, len, DHCP_OPTION_REQUESTED_IP);
  if (opt != NULL)
  {
    node = dhcp_client_find_by_ip(dhcpserver, &opt[2]);
    if (node != NULL)
    {
      if (0 == memcmp(node->chaddr, msg->chaddr, msg->hlen))
      {
        return node;
      }
    }
    else
    {

      u32_t ipval;
      memcpy(&ipval, &opt[2], sizeof(ipval));

      if ((lwip_ntohl(ipval) < lwip_ntohl(dhcpserver->start.addr)) ||
          (lwip_ntohl(ipval) > lwip_ntohl(dhcpserver->end.addr)))
      {
        /* IP address exceeds the address range */
        LogWarn("IP address exceeds\n");
        goto dhcp_alloc_again;
      }

      if (m_node)
      {
        ip4_addr_set_u32(&m_node->ipaddr, ipval);

      }
      else
      {

        node = (struct dhcp_client_node *)mem_malloc(sizeof(struct dhcp_client_node));
        if (node == NULL)
        {
          return NULL;
        }
        SMEMCPY(node->chaddr, msg->chaddr, msg->hlen);
        ip4_addr_set_u32(&node->ipaddr, ipval);

        node->next = dhcpserver->node_list;
        dhcpserver->node_list = node;
        return node;
      }
    }
  }
dhcp_alloc_again:
  if (m_node)
  {
    return m_node;
  }

  node = dhcp_client_find_by_ip(dhcpserver, (uint8_t *)&dhcpserver->current);
  if (node != NULL)
  {
    ipaddr = (ntohl(dhcpserver->current.addr) + 1);
    if (ipaddr > ntohl(dhcpserver->end.addr))
    {
      ipaddr = ntohl(dhcpserver->start.addr);
    }
    dhcpserver->current.addr = htonl(ipaddr);
    goto dhcp_alloc_again;
  }
  node = (struct dhcp_client_node *)mem_malloc(sizeof(struct dhcp_client_node));
  if (node == NULL)
  {
    return NULL;
  }
  SMEMCPY(node->chaddr, msg->chaddr, msg->hlen);
  node->ipaddr = dhcpserver->current;

  node->next = dhcpserver->node_list;
  dhcpserver->node_list = node;

  return node;
}

static u8_t *dhcp_server_option_find(u8_t *buf, u16_t len, u8_t option)
{
  u8_t *end = buf + len;
  while ((buf < end) && (*buf != DHCP_OPTION_END))
  {
    if (*buf == option)
    {
      return buf;
    }
    buf += (buf[1] + 2);
  }
  return NULL;
}

static void dhcp_server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *recv_addr, u16_t port)
{
  struct dhcp_server *dhcp_server = (struct dhcp_server *)arg;
  struct dhcp_msg *msg;
  struct pbuf *q;
  u8_t *opt_buf;
  u8_t *opt;
  struct dhcp_client_node *node;
  u8_t msg_type;
  u16_t length;
  ip_addr_t addr = *recv_addr;
  u32_t tmp;

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("[%s:%d] %c%c recv %d\n", __FUNCTION__, __LINE__,
                                            dhcp_server->netif->name[0],
                                            dhcp_server->netif->name[1],
                                            p->tot_len));
  /* prevent warnings about unused arguments */
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  if (p->len < DHCP_MIN_REQUEST_LEN)
  {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("DHCP request message or pbuf too short\n"));
    pbuf_free(p);
    return;
  }

  q = pbuf_alloc(PBUF_TRANSPORT, 1500, PBUF_RAM);
  if (q == NULL)
  {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("pbuf_alloc dhcp_msg failed!\n"));
    pbuf_free(p);
    return;
  }
  if (q->tot_len < p->tot_len)
  {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("pbuf_alloc dhcp_msg too small %d:%d\n",
                                                                       q->tot_len,
                                                                       p->tot_len));
    pbuf_free(p);
    return;
  }

  pbuf_copy(q, p);
  pbuf_free(p);

  msg = (struct dhcp_msg *)q->payload;
  if (msg->op != DHCP_BOOTREQUEST)
  {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
                ("not a DHCP request message, but type %"U16_F"\n",
                 (u16_t)msg->op));
    goto free_pbuf_and_return;
  }

  if (msg->cookie != PP_HTONL(DHCP_MAGIC_COOKIE))
  {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("bad DHCP_MAGIC_COOKIE!\n"));
    goto free_pbuf_and_return;
  }

  if (msg->hlen > DHCP_MAX_HLEN)
  {
    goto free_pbuf_and_return;
  }

  opt_buf = (u8_t *)msg + DHCP_OPTIONS_OFS;
  length = q->tot_len - DHCP_OPTIONS_OFS;
  opt = dhcp_server_option_find(opt_buf, length, DHCP_OPTION_MESSAGE_TYPE);
  if (opt)
  {
    msg_type = *(opt + 2);
    if (msg_type == DHCP_DISCOVER)
    {
      node = dhcp_client_alloc(dhcp_server, msg, opt_buf, length);
      if (node == NULL)
      {
        goto free_pbuf_and_return;
      }
      node->lease_end = DHCP_DEFAULT_LIVE_TIME;
      /* create dhcp offer and send */
      msg->op = DHCP_BOOTREPLY;
      msg->hops = 0;
      msg->secs = 0;
      SMEMCPY(&msg->siaddr, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
      msg->sname[0] = '\0';
      msg->file[0] = '\0';
      msg->cookie = PP_HTONL(DHCP_MAGIC_COOKIE);
      SMEMCPY(&msg->yiaddr, &node->ipaddr, 4);

      opt_buf = (u8_t *)msg + DHCP_OPTIONS_OFS;
      /* add msg type */
      *opt_buf++ = DHCP_OPTION_MESSAGE_TYPE;
      *opt_buf++ = 1;
      *opt_buf++ = DHCP_OFFER;

      /* add server id */
      *opt_buf++ = DHCP_OPTION_SERVER_ID;
      *opt_buf++ = 4;

      SMEMCPY(opt_buf, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
      opt_buf += 4;

      /* add_lease_time */
      *opt_buf++ = DHCP_OPTION_LEASE_TIME;
      *opt_buf++ = 4;
      tmp = PP_HTONL(DHCP_DEFAULT_LIVE_TIME);
      SMEMCPY(opt_buf, &tmp, 4);
      opt_buf += 4;

      /* add config */
      *opt_buf++ = DHCP_OPTION_SUBNET_MASK;
      *opt_buf++ = 4;

      SMEMCPY(opt_buf, &ip_2_ip4(&dhcp_server->netif->netmask)->addr, 4);
      opt_buf += 4;

      *opt_buf++ = DHCP_OPTION_DNS_SERVER;
#ifdef DHCP_DNS_SERVER_IP
      *opt_buf++ = 4;
      {
        const ip_addr_t *dns_getserver(u8_t numdns);
        ip_addr_t dns_addr = *(dns_getserver(0));
        SMEMCPY(opt_buf, &ip_2_ip4(&dns_addr)->addr, 4);
      }
      opt_buf += 4;
#else
      u8_t *dns_len = opt_buf++;
      *dns_len = 0;
      for (u8_t i = 0; i < DHCPD_MAX_DNS_SERVER_NUM; i++)
      {
        if (!ip4_addr_isany(&dhcp_server->dnsserver[i]))
        {
          SMEMCPY(opt_buf, &(dhcp_server->dnsserver[i].addr), 4);
          opt_buf += 4;
          *dns_len += 4;
        }
      }
      if (*dns_len == 0)
      {
        /* default use gateway dns server */
        SMEMCPY(opt_buf, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
        opt_buf += 4;
        *dns_len += 4;
      }
#endif /* DHCP_DNS_SERVER_IP */

      *opt_buf++ = DHCP_OPTION_ROUTER;
      *opt_buf++ = 4;

      SMEMCPY(opt_buf, &ip_2_ip4(&dhcp_server->netif->ip_addr)->addr, 4);
      opt_buf += 4;

      /* add option end */
      *opt_buf++ = DHCP_OPTION_END;

      length = (u32_t)opt_buf - (u32_t)msg;
      if (length < q->tot_len)
      {
        pbuf_realloc(q, length);
      }

      ip_2_ip4(&addr)->addr = INADDR_BROADCAST;
      udp_sendto_if(pcb, q, &addr, port, dhcp_server->netif);
    }
    else
    {
      if (1)
      {
        if (msg_type == DHCP_REQUEST)
        {
          node = dhcp_client_find(dhcp_server, msg, opt_buf, length);
          if (node != NULL)
          {
            /* Send ack */
            node->lease_end = DHCP_DEFAULT_LIVE_TIME;
            /* create dhcp offer and send */
            msg->op = DHCP_BOOTREPLY;
            msg->hops = 0;
            msg->secs = 0;

            SMEMCPY(&msg->siaddr, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
            msg->sname[0] = '\0';
            msg->file[0] = '\0';
            msg->cookie = PP_HTONL(DHCP_MAGIC_COOKIE);
            SMEMCPY(&msg->yiaddr, &node->ipaddr, 4);
            opt_buf = (u8_t *)msg + DHCP_OPTIONS_OFS;

            /* add msg type */
            *opt_buf++ = DHCP_OPTION_MESSAGE_TYPE;
            *opt_buf++ = 1;
            *opt_buf++ = DHCP_ACK;

            /* add server id */
            *opt_buf++ = DHCP_OPTION_SERVER_ID;
            *opt_buf++ = 4;

            SMEMCPY(opt_buf, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
            opt_buf += 4;

            /* add_lease_time */
            *opt_buf++ = DHCP_OPTION_LEASE_TIME;
            *opt_buf++ = 4;
            tmp = PP_HTONL(DHCP_DEFAULT_LIVE_TIME);
            SMEMCPY(opt_buf, &tmp, 4);
            opt_buf += 4;

            /* add config */
            *opt_buf++ = DHCP_OPTION_SUBNET_MASK;
            *opt_buf++ = 4;

            SMEMCPY(opt_buf, &ip_2_ip4(&dhcp_server->netif->netmask)->addr, 4);
            opt_buf += 4;

            *opt_buf++ = DHCP_OPTION_DNS_SERVER;
#ifdef DHCP_DNS_SERVER_IP
            *opt_buf++ = 4;
            {
              const ip_addr_t *dns_getserver(u8_t numdns);
              ip_addr_t dns_addr = *(dns_getserver(0));
              SMEMCPY(opt_buf, &ip_2_ip4(&dns_addr)->addr, 4);
            }
            opt_buf += 4;
#else
            u8_t *dns_len = opt_buf++;
            *dns_len = 0;
            for (u8_t i = 0; i < DHCPD_MAX_DNS_SERVER_NUM; i++)
            {
              if (!ip4_addr_isany(&dhcp_server->dnsserver[i]))
              {
                SMEMCPY(opt_buf, &(dhcp_server->dnsserver[i].addr), 4);
                opt_buf += 4;
                *dns_len += 4;
              }
            }
            if (*dns_len == 0)
            {
              /* default use gateway dns server */
              SMEMCPY(opt_buf, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
              opt_buf += 4;
              *dns_len += 4;
            }
#endif /* DHCP_DNS_SERVER_IP */

            *opt_buf++ = DHCP_OPTION_ROUTER;
            *opt_buf++ = 4;
            SMEMCPY(opt_buf, &ip_2_ip4(&dhcp_server->netif->ip_addr)->addr, 4);
            opt_buf += 4;

            /* add option end */
            *opt_buf++ = DHCP_OPTION_END;

            length = (u32_t)opt_buf - (u32_t)msg;
            if (length < q->tot_len)
            {
              pbuf_realloc(q, length);
            }

            ip_2_ip4(&addr)->addr = INADDR_BROADCAST;
            udp_sendto_if(pcb, q, &addr, port, dhcp_server->netif);

            struct netif netif;
            SMEMCPY(ip_2_ip4(&netif.ip_addr), &node->ipaddr, 4);
            SMEMCPY(&netif.hwaddr, &node->chaddr, sizeof(netif.hwaddr));
            if (dhcp_server->sta_status_cb)
            {
              dhcp_server->sta_status_cb(&netif);
            }
          }
          else
          {
            /* Send no ack */
            /* create dhcp offer and send */
            msg->op = DHCP_BOOTREPLY;
            msg->hops = 0;
            msg->secs = 0;
            SMEMCPY(&msg->siaddr, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
            msg->sname[0] = '\0';
            msg->file[0] = '\0';
            msg->cookie = PP_HTONL(DHCP_MAGIC_COOKIE);
            memset(&msg->yiaddr, 0, 4);
            opt_buf = (u8_t *)msg + DHCP_OPTIONS_OFS;

            /* add msg type */
            *opt_buf++ = DHCP_OPTION_MESSAGE_TYPE;
            *opt_buf++ = 1;
            *opt_buf++ = DHCP_NAK;

            /* add server id */
            *opt_buf++ = DHCP_OPTION_SERVER_ID;
            *opt_buf++ = 4;
            SMEMCPY(opt_buf, ip_2_ip4(&dhcp_server->netif->ip_addr), 4);
            opt_buf += 4;

            /* add option end */
            *opt_buf++ = DHCP_OPTION_END;
            length = (u32_t)opt_buf - (u32_t)msg;
            if (length < q->tot_len)
            {
              pbuf_realloc(q, length);
            }

            ip_2_ip4(&addr)->addr = INADDR_BROADCAST;
            udp_sendto_if(pcb, q, &addr, port, dhcp_server->netif);
          }
        }
        else if (msg_type == DHCP_RELEASE)
        {
          struct dhcp_client_node *node_prev = NULL;

          for (node = dhcp_server->node_list; node != NULL; node = node->next)
          {
            if (memcmp(node->chaddr, msg->chaddr, msg->hlen) == 0)
            {
              if (node == dhcp_server->node_list)
              {
                dhcp_server->node_list = node->next;
              }
              else
              {
                node_prev->next = node->next;
              }
              break;
            }
            node_prev = node;
          }

          if (node != NULL)
          {
            mem_free(node);
          }
        }
        else if (msg_type ==  DHCP_DECLINE)
        {
          ;
        }
        else if (msg_type == DHCP_INFORM)
        {
          ;
        }
      }
    }
  }

free_pbuf_and_return:
  pbuf_free(q);
}

static err_t dhcp_server_op_dns_server(void *netif, const ip_addr_t *dnsserver, int32_t reset)
{
  struct dhcp_server *dhcp_server;
  int32_t found = 0;
  int32_t free = 0;

  if (!netif || !dnsserver)
  {
    return ERR_VAL;
  }

  for (dhcp_server = lw_dhcp_server; dhcp_server != NULL; dhcp_server = dhcp_server->next)
  {
    if (dhcp_server->netif == netif)
    {
      break;
    }
  }
  if (!dhcp_server)
  {
    return ERR_VAL;
  }

  for (int32_t i = 0; i < DHCPD_MAX_DNS_SERVER_NUM; i++)
  {
    if (ip4_addr_cmp(ip_2_ip4(dnsserver), &dhcp_server->dnsserver[i]))
    {
      found = i;
    }
    if (!free && ip4_addr_isany(&dhcp_server->dnsserver[i]))
    {
      free = i;
    }
  }

  /* Set */
  if (!reset && free)
  {
    ip4_addr_set(&dhcp_server->dnsserver[free], ip_2_ip4(dnsserver));
  }
  /* Reset */
  else if (reset && found)
  {
    ip4_addr_set_zero(&dhcp_server->dnsserver[found]);
  }

  return ERR_OK;
}

static void dhcp_server_clear_dns_server(void)
{
  struct dhcp_server *dhcp_server;

  for (dhcp_server = lw_dhcp_server; dhcp_server != NULL; dhcp_server = dhcp_server->next)
  {
    for (int32_t i = 0; i < DHCPD_MAX_DNS_SERVER_NUM; i++)
    {
      ip4_addr_set_zero(&dhcp_server->dnsserver[i]);
    }
  }
}
