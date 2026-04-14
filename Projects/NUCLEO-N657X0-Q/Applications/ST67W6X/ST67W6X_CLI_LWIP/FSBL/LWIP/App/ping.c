/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ping.c
  * @author  ST67 Application Team
  * @brief   Ping application
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
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>

#include "main.h"
#include "lwipopts.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include "ping.h"

#include "shell.h"
#include "logging.h"

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "icmp.h"
#include "icmp6.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private defines -----------------------------------------------------------*/
/** Identifier for the ICMP echo request */
#define PING_ID                       0xAFAFU

/** Maximum payload size for ICMP echo request */
#define MAX_PAYLOAD_SIZE              1024U

/** Default number of echo requests to send */
#define PING_DEFAULT_COUNT            4U

/** Default interval between echo requests - in milliseconds */
#define PING_DEFAULT_INTERVAL         1000U

/** Maximum ping receive timeout - in milliseconds */
#define PING_MAX_RCV_TIMEO            3500U

/** DNS resolve timeout - in milliseconds */
#define DNS_RESOLVE_TIMEOUT           5000U

/** Default payload size for ICMP echo request */
#define PING_DEFAULT_SIZE             64U

/** Maximum payload size for ICMP echo request */
#define PING_MAX_SIZE                 10000U

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Structure to store DNS resolve context
  */
typedef struct
{
  ip_addr_t resolved_ip;      /*!< Resolved IP address */
  volatile int32_t done;      /*!< Flag to indicate if resolution is done */
  int32_t result;             /*!< Result of the resolution */
} ping_dns_resolve_context_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
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

/**
  * @brief  Common CLI entry to parse args and start ping task
  * @param  argc: number of arguments
  * @param  argv: pointer to arguments
  * @param  ipv6: 0 for IPv4, 1 for IPv6
  * @retval shell status
  */
static int32_t ping_common_cmd(int32_t argc, char **argv, uint32_t ipv6);

/**
  * @brief  DNS lookup callback function for lwIP
  * @param  name: pointer to the hostname string
  * @param  ipaddr: pointer to the resolved IP address (NULL if failed)
  * @param  arg: user-supplied argument (unused)
  */
static void ping_dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg);

/**
  * @brief  Get IPv6 or IPv4 address from URL using DNS
  * @param  hostUrl: Host URL
  * @param  use_ipv6: 0 for ipv4, 1 for ipv6
  * @param  out_ip: the returned IP address
  * @retval 0 on success, -1 on failure, -2 on timeout
  */
static int32_t ping_resolve_host_address(const char *hostUrl, uint32_t use_ipv6, ip_addr_t *out_ip);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
int32_t send_ping(ping_context_t *ping_context)
{
  int32_t ret = -1;
  int32_t sd = -1;
  char *send_packet = NULL;
  char *recv_packet = NULL;
  uint32_t packets_sent = 0;
  uint32_t packets_received = 0;
  uint32_t total_ping_time_ms = 0;
  uint32_t recv_time_ms = 0; /* initialized */
  TickType_t send_tick;
  TickType_t recv_tick;
  int32_t aborted = 0; /* Abort flag: set on send failure (after retries) or receive failure */
  int32_t matched = 0;
  int32_t domain = AF_INET;
  int32_t protocol = IPPROTO_ICMP;

  struct icmp_echo_hdr *icmphdr = NULL;
  struct sockaddr_storage to = {0};
  struct sockaddr_storage from = {0};
  int32_t fromlen = sizeof(from);
  size_t ping_size;
#if (LWIP_IPV6 == 1)
  struct icmp6_echo_hdr *icmph6 = NULL;
  ip6_addr_t *dst6;
  struct sockaddr_in6 src_addr6 = {0};
  int32_t chosen_index = -1;
#endif /* LWIP_IPV6 */
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int32_t timeout = LWIP_MAX(ping_context->timeout, ping_context->interval_ms);
#else
  struct timeval timeout;
  timeout.tv_sec = LWIP_MAX(ping_context->timeout, ping_context->interval_ms) / 1000;
  timeout.tv_usec = (LWIP_MAX(ping_context->timeout, ping_context->interval_ms) % 1000) * 1000;
#endif /* LWIP_SO_SNDRCVTIMEO_NONSTANDARD */

  struct netif *netif_sta = netif_get_interface(NETIF_STA);
  struct netif *netif_ap = netif_get_interface(NETIF_AP);
  struct netif *netif_cur = NULL;

#if (LWIP_IPV6 == 1)
  if (ping_context->ipv6)
  {
    /* IPv6 setup */
    /* Consider only STA interface */
    netif_cur = netif_sta;
    if ((netif_cur == NULL) || !netif_is_link_up(netif_cur))
    {
      LogError("Default network interface not set\n");
      goto _err;
    }

    /* Set up destination address structure */
    struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&to;
    to6->sin6_len    = sizeof(*to6);
    to6->sin6_family = AF_INET6;
    if (lwip_inet_pton(AF_INET6, ping_context->dst_addr, &to6->sin6_addr) != 1)
    {
      LogError("Unable to translate address\n");
      goto _err;
    }

    /* Choose source IPv6 address according to destination scope (link-local vs global) */
    dst6 = (ip6_addr_t *)&to6->sin6_addr;

    if (ip6_addr_islinklocal(dst6))
    {
      /* Find a valid link-local address on STA */
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif_sta, 0)))
      {
        netif_cur = netif_sta;
        chosen_index = 0;
      }
      else
      {
        LogError("Unable to find a valid ipv6 link-local address\n");
        goto _err;
      }
    }
    else
    {
      /* Choose a valid global address (indices 1..LWIP_IPV6_NUM_ADDRESSES-1) on STA */
      for (int32_t idx = 1; idx < LWIP_IPV6_NUM_ADDRESSES; idx++)
      {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif_sta, idx)))
        {
          netif_cur = netif_sta;
          chosen_index = idx;
          break;
        }
      }
    }

    if (chosen_index < 0)
    {
      LogError("No suitable IPv6 source address available\n");
      goto _err;
    }

    ip6_addr_t const *src_ip6 = netif_ip6_addr(netif_cur, chosen_index);
    if (ip6_addr_isany(src_ip6))
    {
      LogError("No valid IPv6 address assigned to network interface\n");
      goto _err;
    }
    /* Get the source IPv6 address from the network interface */
    src_addr6.sin6_family = AF_INET6;
    inet6_addr_from_ip6addr(&src_addr6.sin6_addr, src_ip6);

    domain = AF_INET6;
    protocol = IPPROTO_ICMPV6;
  }
  else
#endif /* LWIP_IPV6 */
  {
    /* IPv4 setup */
    /* Set up destination address structure */
    struct sockaddr_in *to4 = (struct sockaddr_in *)&to;
    to4->sin_len    = sizeof(*to4);
    to4->sin_family = AF_INET;
    if (lwip_inet_pton(AF_INET, ping_context->dst_addr, &to4->sin_addr) != 1)
    {
      LogError("Unable to translate address!\n");
      goto _err;
    }

    /* Compare the subnet mask to determine which interface to use */
    ip4_addr_t dest_ip4;
    (void)memcpy(&dest_ip4, &to4->sin_addr, sizeof(ip4_addr_t));
    if (netif_is_link_up(netif_ap) &&
        ip4_addr_netcmp(&dest_ip4, netif_ip4_addr(netif_ap), netif_ip4_netmask(netif_ap)) &&
        (!ip4_addr_isany(netif_ip4_addr(netif_ap))))
    {
      netif_cur = netif_ap;
    }
    else if (netif_is_link_up(netif_sta))
    {
      netif_cur = netif_sta;
    }
    else
    {
      LogError("Default network interface not set\n");
      goto _err;
    }

    if (ip4_addr_isany(netif_ip4_addr(netif_cur)))
    {
      LogError("No valid IPv4 address assigned to network interface\n");
      goto _err;
    }
  }

  ping_size = sizeof(struct icmp_echo_hdr) + ping_context->size;

  /* Prepare the ICMP echo request packet */
#if (LWIP_IPV6 == 1)
  if (ping_context->ipv6)
  {
    /* Allocate memory for send packet */
    send_packet = pvPortMalloc(ping_size);
    if (send_packet == NULL)
    {
      LogError("Failed to allocate memory for send packet\n");
      goto _err;
    }
    /* Allocate memory for receive packet */
    recv_packet = pvPortMalloc(ping_size + sizeof(struct ip6_hdr));
    if (recv_packet == NULL)
    {
      LogError("Failed to allocate memory for receive packet\n");
      goto _err;
    }

    (void)memset(send_packet, 0, sizeof(struct icmp6_echo_hdr));
    (void)memset(&send_packet[sizeof(struct icmp6_echo_hdr)], 'A', ping_context->size);

    icmph6 = (struct icmp6_echo_hdr *)send_packet;
    /* Fill in the ICMPv6 header */
    icmph6->type = ICMP6_TYPE_EREQ;
    icmph6->id = PING_ID;
  }
  else
#endif /* LWIP_IPV6 */
  {
    /* Allocate memory for send packet */
    send_packet = pvPortMalloc(ping_size);
    if (send_packet == NULL)
    {
      LogError("Failed to allocate memory for send packet\n");
      goto _err;
    }
    /* Allocate memory for receive packet */
    recv_packet = pvPortMalloc(ping_size + sizeof(struct ip_hdr));
    if (recv_packet == NULL)
    {
      LogError("Failed to allocate memory for receive packet\n");
      goto _err;
    }

    (void)memset(send_packet, 0, sizeof(struct icmp_echo_hdr));
    (void)memset(&send_packet[sizeof(struct icmp_echo_hdr)], 'A', ping_context->size);

    icmphdr = (struct icmp_echo_hdr *)send_packet;
    /* Fill in the ICMP header */
    icmphdr->type = ICMP_ECHO;
    icmphdr->id = PING_ID;
  }

  /* Request a socket descriptor sd */
  sd = lwip_socket(domain, SOCK_RAW, protocol);
  if (sd < 0)
  {
    LogError("Failed to get socket descriptor: %" PRIi32 "\n", errno);
    goto _err;
  }

  (void)lwip_setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  /* Bind the socket to the source address */
#if (LWIP_IPV6 == 1)
  if (ping_context->ipv6)
  {
    if (lwip_bind(sd, (struct sockaddr *)&src_addr6, sizeof(src_addr6)) < 0)
    {
      LogError("Failed to bind socket to source address: %" PRIi32 "\n", errno);
      goto _err;
    }
  }
#endif /* LWIP_IPV6 */

  for (int32_t seq_num = 0; seq_num < ping_context->count; seq_num++)
  {
    uint32_t sent_ok = 0;

    /* Reset per-sequence state */
    matched = 0;
    recv_time_ms = 0;

    /* Update the ICMP header for each packet */
#if (LWIP_IPV6 == 1)
    if (ping_context->ipv6)
    {
      icmph6->seqno = lwip_htons(seq_num);
      /* lets LwIP handle the ICMPv6 checksum. */
      icmph6->chksum = 0;
    }
    else
#endif /* LWIP_IPV6 */
    {
      icmphdr->seqno = lwip_htons(seq_num);
      icmphdr->chksum = 0;
      icmphdr->chksum = inet_chksum(icmphdr, ping_size);
    }

    /* Record the send time */
    send_tick = xTaskGetTickCount();

    /* Send the ICMP echo request with up to 5 retries on transient errors */
    for (uint32_t send_attempts = 0; send_attempts < 5U; send_attempts++)
    {
      ret = lwip_sendto(sd, send_packet, ping_size, 0, (struct sockaddr *)&to, sizeof(to));

      if (ret < 0)
      {
        LogError("sendto() failed (attempt %d): %" PRIi32 "\n", send_attempts + 1U, errno);
        vTaskDelay(pdMS_TO_TICKS(50)); /* brief backoff before retry */
      }
      else
      {
        sent_ok = true;
        packets_sent++;
        break;
      }
    }

    if (!sent_ok)
    {
      LogError("Ping seq %" PRIi32 " send failed after retries\n", seq_num);
      aborted = 1; /* Abort test immediately on unrecoverable send failure */
      break;
    }

    /* We may receive out-of-order ICMP Echo Replies (e.g., late replies
     * from previous sequences or duplicates). To avoid one mismatch breaking
     * subsequent sequences, we keep receiving until we either get the matching
     * Echo Reply for the current sequence, or the socket read times out.
     */
    for (uint32_t tries = 0; tries < 5U; tries++)
    {
      from.ss_family = AF_INET;
#if (LWIP_IPV6 == 1)
      if (ping_context->ipv6)
      {
        ret = lwip_recvfrom(sd, recv_packet, ping_size + sizeof(struct ip6_hdr), 0,
                            (struct sockaddr *)&from, (socklen_t *)&fromlen);
      }
      else
#endif /* LWIP_IPV6 */
      {
        ret = lwip_recvfrom(sd, recv_packet, ping_size + sizeof(struct ip_hdr), 0,
                            (struct sockaddr *)&from, (socklen_t *)&fromlen);
      }

      if (ret < 0)
      {
        /* Timeout or error: no valid reply for this sequence within the window */
        break; /* timeout */
      }

      if (ret >= (int32_t)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
      {
#if (LWIP_IPV6 == 1)
        if (from.ss_family == AF_INET6)
        {
          struct icmp6_echo_hdr *iecho = (struct icmp6_echo_hdr *)(recv_packet + sizeof(struct ip6_hdr));
          /* Keep only the valid Echo Reply that matches our ping id and sequence */
          if ((ICMPH_TYPE(iecho) == ICMP6_TYPE_EREP) && (iecho->id == PING_ID) &&
              (iecho->seqno == lwip_htons(seq_num)))
          {
            recv_tick = xTaskGetTickCount();
            recv_time_ms = (recv_tick - send_tick) * portTICK_PERIOD_MS;
            matched = 1;
            break;
          }
        }
        else
#endif /* LWIP_IPV6 */
        {
          struct ip_hdr *iphdr;
          struct icmp_echo_hdr *iecho;

          iphdr = (struct ip_hdr *)recv_packet;
          iecho = (struct icmp_echo_hdr *)(recv_packet + (IPH_HL(iphdr) * 4));
          if ((ICMPH_TYPE(iecho) == ICMP_ER) && (iecho->id == PING_ID) &&
              (iecho->seqno == lwip_htons(seq_num)))
          {
            recv_tick = xTaskGetTickCount();
            recv_time_ms = (recv_tick - send_tick) * portTICK_PERIOD_MS;
            matched = 1;
            break;
          }
        }
      }
      fromlen = sizeof(from);
    }

    if (matched)
    {
      total_ping_time_ms += recv_time_ms;
      packets_received++;
      LogInfo("Ping: %" PRIu32 " ms\n", recv_time_ms);
      if (ping_context->interval_ms > recv_time_ms)
      {
        vTaskDelay(pdMS_TO_TICKS(ping_context->interval_ms - recv_time_ms));
      }
    }
    else
    {
      /* No valid reply observed for this sequence; log a single concise message */
      LogError("Ping seq %" PRIi32 " timeout or no valid reply\n", seq_num);
      aborted = 1; /* Abort immediately after failing all receive attempts for this sequence */
      break;
    }
  }

  /* If the ping was aborted or no packets were sent */
  if (packets_sent == 0U)
  {
    LogError("No ping received\n");
    ping_context->ping_sent = 0;
    ret = -1;
  }
  else /* Ping completed. Display the statistics */
  {
    LogInfo("%" PRIi32 " packets transmitted, %" PRIi32 " received, %" PRIi32 "%% packet loss, time %" PRIu32 "ms\n",
            packets_sent, packets_received, ((packets_sent - packets_received) * 100U) / packets_sent,
            (packets_received > 0U) ? (total_ping_time_ms / packets_received) : 0U);
    ping_context->ping_rcv = packets_received;
    ping_context->ping_tot_time_ms = total_ping_time_ms;
    ping_context->ping_sent = packets_sent;
    /* Set return value according to whether the ping was aborted */
    ret = aborted ? -1 : 0;
  }

_err:
  if (send_packet != NULL)
  {
    vPortFree(send_packet);
  }
  if (recv_packet != NULL)
  {
    vPortFree(recv_packet);
  }
  if (sd >= 0)
  {
    (void)lwip_close(sd);
  }
  return ret;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
int32_t ping_ipv4_cmd(int32_t argc, char **argv)
{
  return ping_common_cmd(argc, argv, 0);
}

SHELL_CMD_EXPORT_ALIAS(ping_ipv4_cmd, ping, ping <hostname> [ -c count [1; max(uint16_t) - 1] ]
                       [ -s size [1; 1470] ] [ -i interval [100; 3500] ] [ -t timeout [100; 3500] ]);

int32_t ping_ipv6_cmd(int32_t argc, char **argv)
{
#if (LWIP_IPV6 == 1)
  return ping_common_cmd(argc, argv, 1);
#else
  LogError("IPv6 is not supported\n");
  return -1;
#endif /*LWIP_IPV6 */
}

SHELL_CMD_EXPORT_ALIAS(ping_ipv6_cmd, ping6, ping6 <hostname> [ -c count [1; max(uint16_t) - 1] ]
                       [ -s size [1; 1470] ] [ -i interval [100; 3500] ] [ -t timeout [100; 3500] ]);

static int32_t ping_common_cmd(int32_t argc, char **argv, uint32_t ipv6)
{
  /* Create the ping task */
  static ping_context_t ping_context = {0};
  int32_t current_arg = 2;
  ip_addr_t ipaddr;

  if (argc < 2)
  {
    LogError("Missing %s address argument\n", ipv6 ? "IPv6" : "IPv4");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  if (ipaddr_aton(argv[1], &ipaddr) == 0)
  {
    int32_t dns_res = ping_resolve_host_address(argv[1], ipv6, &ipaddr);
    if (dns_res != 0)
    {
      LogError("Invalid %s address or DNS failed: %s\n", ipv6 ? "IPv6" : "IPv4", argv[1]);
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    /* Log the resolved numeric destination address for the given hostname */
    LogInfo("Resolved %s address for %s: %s\n", ipv6 ? "IPv6" : "IPv4", argv[1], ipaddr_ntoa(&ipaddr));
  }
  /* Store destination string */
  (void)strncpy(ping_context.dst_addr, ipaddr_ntoa(&ipaddr), sizeof(ping_context.dst_addr) - 1U);
  ping_context.dst_addr[sizeof(ping_context.dst_addr) - 1U] = '\0';

  ping_context.count = PING_DEFAULT_COUNT;
  ping_context.interval_ms = PING_DEFAULT_INTERVAL;
  ping_context.size = PING_DEFAULT_SIZE;
  ping_context.timeout = PING_MAX_RCV_TIMEO;
#if (LWIP_IPV6 == 1)
  ping_context.ipv6 = ipv6;
#else
  ipv6 = 0;
#endif /* LWIP_IPV6 */

  while (current_arg < argc)
  {
    /* Count option */
    if (strncmp(argv[current_arg], "-c", 2) == 0)
    {
      if ((current_arg + 1) >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      ping_context.count = (uint16_t)atoi(argv[current_arg + 1]);
      if (ping_context.count < 1U)
      {
        LogError("Ping count is invalid.\n");
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    /* Interval option */
    else if (strncmp(argv[current_arg], "-i", 2) == 0)
    {
      if ((current_arg + 1) >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      ping_context.interval_ms = (uint32_t)atoi(argv[current_arg + 1]);
      if ((ping_context.interval_ms < 100U) || (ping_context.interval_ms > PING_MAX_RCV_TIMEO))
      {
        LogError("Ping interval is invalid, valid range : [100;%" PRIu32 "]\n", PING_MAX_RCV_TIMEO);
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    /* Size option */
    else if (strcmp(argv[current_arg], "-s") == 0)
    {
      if ((current_arg + 1) >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      ping_context.size = (size_t)atoi(argv[current_arg + 1]);
      if ((ping_context.size < 1U) || (ping_context.size > PING_MAX_SIZE))
      {
        LogError("Ping size is invalid, valid range : [1;%" PRIu32 "].\n", PING_MAX_SIZE);
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    /* Timeout option */
    else if (strcmp(argv[current_arg], "-t") == 0)
    {
      if ((current_arg + 1) >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      ping_context.timeout = (uint32_t)atoi(argv[current_arg + 1]);
      if ((ping_context.timeout < 1U) || (ping_context.timeout > PING_MAX_RCV_TIMEO))
      {
        LogError("Ping timeout is invalid, valid range : [1;%" PRIu32 "].\n", PING_MAX_RCV_TIMEO);
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
  }

  return send_ping(&ping_context);
}

/**
  * @brief  DNS lookup callback function for lwIP
  * @param  name: pointer to the hostname string
  * @param  ipaddr: pointer to the resolved IP address (NULL if failed)
  * @param  arg: user-supplied argument (unused)
  */
static void ping_dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
  ping_dns_resolve_context_t *ctx = (ping_dns_resolve_context_t *)arg;
  if (ipaddr)
  {
    ctx->resolved_ip = *ipaddr;
    ctx->result = 0;
  }
  else
  {
    ctx->result = -1;
  }
  ctx->done = 1;
}

static int32_t ping_resolve_host_address(const char *hostUrl, uint32_t use_ipv6, ip_addr_t *out_ip)
{
  if ((hostUrl == NULL) || (out_ip == NULL))
  {
    return -1;
  }

  ping_dns_resolve_context_t ctx;
  (void)memset(&ctx, 0, sizeof(ctx));
  ctx.result = -1;

  uint8_t addrtype = use_ipv6 ? LWIP_DNS_ADDRTYPE_IPV6 : LWIP_DNS_ADDRTYPE_IPV4;
  err_t err = dns_gethostbyname_addrtype(hostUrl, &ctx.resolved_ip, ping_dns_lookup_callback, &ctx, addrtype);
  if (err == ERR_OK)
  {
    if ((use_ipv6 && IP_IS_V6(&ctx.resolved_ip)) || (!use_ipv6 && IP_IS_V4(&ctx.resolved_ip)))
    {
      *out_ip = ctx.resolved_ip;
      return 0;
    }
    LogError("Resolved address family mismatch\n");
    return -1; /* type mismatch */
  }
  else if (err == ERR_INPROGRESS)
  {
    TickType_t startTick = xTaskGetTickCount();
    TickType_t timeoutTick = startTick + pdMS_TO_TICKS(DNS_RESOLVE_TIMEOUT);
    while (!ctx.done)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
      if ((int32_t)(xTaskGetTickCount() - timeoutTick) >= 0)
      {
        LogError("DNS Lookup timed out (%s)\n", use_ipv6 ? "IPv6" : "IPv4");
        return -2; /* timeout */
      }
    }
    if (ctx.result == 0)
    {
      if ((use_ipv6 && IP_IS_V6(&ctx.resolved_ip)) || (!use_ipv6 && IP_IS_V4(&ctx.resolved_ip)))
      {
        *out_ip = ctx.resolved_ip;
        return 0;
      }
      LogError("Resolved address family mismatch\n");
      return -1;
    }
    LogError("DNS Lookup failed\n");
    return -1;
  }
  else
  {
    /* Immediate failure */
  }

  LogError("DNS Lookup immediate failure\n");
  return -1;
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
