/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ping.c
  * @author  GPM Application Team
  * @brief   Ping application
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
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>

#include "main.h"
#include "lwip.h"
#include "lwip/opt.h"
#include "lwipopts.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include "ping.h"

#include "shell.h"
#include "logging.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/sys.h"
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
/** Event flag indicating ping successfully completed */
#define EVT_PING_DONE                 (1<<0)

/** Event flag indicating ping error occurred */
#define EVT_PING_ERROR                (1<<1)

/** Identifier for the ICMP echo request */
#define PING_ID                       0x0001

/** Maximum payload size for ICMP echo request */
#define MAX_PAYLOAD_SIZE              1024

/** Default number of echo requests to send */
#define PING_DEFAULT_COUNT            4

/** Default interval between echo requests - in milliseconds */
#define PING_DEFAULT_INTERVAL         1000

/** ping receive timeout - in milliseconds */
#define PING_RCV_TIMEO                3500

/** Default payload size for ICMP echo request */
#define PING_DEFAULT_SIZE             64

/** Maximum payload size for ICMP echo request */
#define PING_MAX_SIZE                 10000

/* USER CODE BEGIN PD */

/* USER CODE END PD */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Global socket descriptor for access in callback */
static int32_t global_sd = -1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Echo timeout callback
  * @param  arg: unused parameter
 */
static void echo_timeout_callback(void *arg);

/**
  * @brief  Set the echo timer object
  */
static void set_echo_timer(void);

/**
  * @brief  Send ICMP echo IPv4 request
  * @param  count: Number of echo requests to send
  * @param  size: Size of the payload
  * @param  interval: Interval between requests
  * @param  dst_addr_str: Destination address as a string
  * @return ping status. -1 if error, 0 if success
 */
static int32_t send_ping_ipv4(int32_t count, int32_t size, int32_t interval, char *dst_addr_str);

/**
  * @brief  Task to initialize and run the ping IPv4 application
  * @param  pvParameters: Task parameters
  */
static void ping_ipv4_task(void *pvParameters);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
/* Functions Definition ------------------------------------------------------*/
static void echo_timeout_callback(void *arg)
{
  LogError("Ping timeout\n");
  if (global_sd >= 0)
  {
    lwip_close(global_sd); /* Close the socket immediately */
    global_sd = -1;
  }
  /* Handle timeout event, such as retrying or logging */
}

static void set_echo_timer(void)
{
  /* Set a timer to avoid timeout message if delay between successive ping messages is high */
  sys_timeout(PING_RCV_TIMEO, echo_timeout_callback, NULL);
}

static int32_t send_ping_ipv4(int32_t count, int32_t size, int32_t interval, char *dst_addr_str)
{
  int32_t ret = -1;
  int32_t sd = -1;
  char *send_packet = NULL;
  char *recv_packet = NULL;
  uint32_t total_ping_time_ms = 0;
  ip4_addr_t const *src_ip4;
  struct icmp_echo_hdr *icmphdr;
  struct sockaddr_in dest_addr_ipv4 = {0};
  struct sockaddr_in src_addr = {0};
  struct sockaddr_in from;
  int32_t packets_received = 0;
  socklen_t from_len = sizeof(from);
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int32_t timeout = LWIP_MAX(PING_RCV_TIMEO, interval);
#else
  struct timeval timeout;
  timeout.tv_sec = LWIP_MAX(PING_RCV_TIMEO, interval) / 1000;
  timeout.tv_usec = (LWIP_MAX(PING_RCV_TIMEO, interval) % 1000) * 1000;
#endif /* LWIP_SO_SNDRCVTIMEO_NONSTANDARD */

  struct netif *netif = netif_get_interface(NETIF_STA);
  if (netif == NULL)
  {
    LogError("Default network interface not set\n");
    goto _err;
  }
  /* Get the source IPv4 address from the network interface */
  src_ip4 = netif_ip4_addr(netif);
  if (ip4_addr_isany(src_ip4))
  {
    LogError("No valid IPv4 address assigned to network interface\n");
    goto _err;
  }
  src_addr.sin_family = AF_INET;
  inet_addr_from_ip4addr(&src_addr.sin_addr, src_ip4);

  /* Allocate memory for send packet */
  send_packet = pvPortMalloc(size + sizeof(struct icmp_echo_hdr) + 1);
  if (send_packet == NULL)
  {
    LogError("Failed to allocate memory for send packet\n");
    goto _err;
  }
  /* Allocate memory for receive packet */
  recv_packet = pvPortMalloc(size + sizeof(struct icmp_echo_hdr) + sizeof(struct ip_hdr));
  if (recv_packet == NULL)
  {
    LogError("Failed to allocate memory for receive packet\n");
    goto _err;
  }

  /* Prepare the ICMP echo request packet */
  memset(send_packet, 0, sizeof(struct icmp_echo_hdr));
  memset(&send_packet[sizeof(struct icmp_echo_hdr)], 'A', size);

  /* Fill in the ICMP header */
  icmphdr = (struct icmp_echo_hdr *)send_packet;
  icmphdr->type = ICMP_ECHO;
  icmphdr->id = lwip_htons(PING_ID);

  /* Set up destination address structure */
  dest_addr_ipv4.sin_family = AF_INET;
  if (lwip_inet_pton(AF_INET, dst_addr_str, &dest_addr_ipv4.sin_addr) != 1)
  {
    LogError("Unable to translate address!\n");
    goto _err;
  }

  /* Request a socket descriptor sd */
  if ((sd = lwip_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
  {
    LogError("Failed to get socket descriptor: %" PRIi32 "\n", errno);
    goto _err;
  }

  (void)lwip_setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  /* Bind the socket to the source address */
  if (lwip_bind(sd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0)
  {
    LogError("Failed to bind socket to source address: %" PRIi32 "\n", errno);
    goto _err;
  }

  for (int32_t seq_num = 0; seq_num < count; seq_num++)
  {
    /* Update the ICMP header for each packet */
    icmphdr->seqno = lwip_htons(seq_num);
    icmphdr->chksum = 0;
    icmphdr->chksum = inet_chksum(icmphdr, sizeof(struct icmp_echo_hdr) + size);

    /* Record the send time */
    TickType_t send_tick = xTaskGetTickCount();

    /* Send the ICMP echo request */
    if ((ret = lwip_sendto(sd, send_packet, sizeof(struct icmp_echo_hdr) + size, 0,
                           (struct sockaddr *)&dest_addr_ipv4, sizeof(dest_addr_ipv4))) < 0)
    {
      LogError("sendto() failed: %" PRIi32 "\n", errno);
      goto _err;
    }
    global_sd = sd; /* Save socket descriptor for echo timeout callback */
    set_echo_timer(); /* Set the echo timeout callback */

    /* Listen for incoming message from socket sd */
    /* Keep at it until we get a Echo Reply */
    ret = lwip_recvfrom(sd, recv_packet, size + sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr), 0,
                        (struct sockaddr *)&from, (socklen_t *)&from_len);

    sys_untimeout(echo_timeout_callback, NULL); /* Cancel the timeout */

    if (ret < 0)
    {
      LogError("recvfrom() failed: %" PRIi32 "\n", errno);
      goto _err;
    }

    /* Record the receive time and calculate round-trip time */
    TickType_t recv_tick = xTaskGetTickCount();
    uint32_t recv_time_ms = (recv_tick - send_tick) * portTICK_PERIOD_MS;

    /* Add ping time for average calculation */
    total_ping_time_ms += recv_time_ms;

    packets_received++;
    LogInfo("Ping: %" PRIu32 " ms\n", recv_time_ms);

    /* Wait for a specified interval before sending the next ping */
    if (interval > recv_time_ms)
    {
      vTaskDelay(pdMS_TO_TICKS(interval - recv_time_ms));
    }
  }

  if (packets_received > 0) /* Avoid division by zero */
  {
    LogInfo("%" PRIi32 " packets transmitted, %" PRIi32 " received, %" PRIi32 "%% packet loss, time %" PRIu32 "ms\n",
            count, packets_received, ((count - packets_received) * 100) / packets_received,
            total_ping_time_ms / packets_received);
    ret = 0;
  }
  else
  {
    LogError("No ping received\n");
    ret = -1;
  }

_err:
  if (send_packet)
  {
    vPortFree(send_packet);
  }
  if (recv_packet)
  {
    vPortFree(recv_packet);
  }
  if (sd >= 0)
  {
    lwip_close(sd);
  }
  global_sd = -1;
  return ret;
}

static void ping_ipv4_task(void *pvParameters)
{
  int32_t ret;
  ping_ipv4_params_t *params = (ping_ipv4_params_t *)pvParameters;

  /* Start the ping request */
  ret = send_ping_ipv4(params->count, params->size, params->interval_ms, params->dst_addr);

  /* Set the event flag to notify the end of the ping task if the event group is created */
  if (params->event != NULL)
  {
    if (ret == 0)
    {
      xEventGroupSetBits(params->event, EVT_PING_DONE);
    }
    else
    {
      xEventGroupSetBits(params->event, EVT_PING_ERROR);
    }
  }

  vTaskDelete(NULL);
}

int32_t ping_ipv4_cmd(int32_t argc, char **argv)
{
  /* Create the ping IPv4 task */
  static ping_ipv4_params_t params = {0};
  int32_t current_arg = 2;
  ip_addr_t ipaddr;
  int32_t ret = SHELL_STATUS_ERROR;

  if (argc < 2)
  {
    LogError("Missing IPv4 address argument\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  if (!ipaddr_aton(argv[1], &ipaddr))
  {
    LogError("Invalid IPv4 address: %s\n", argv[1]);
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  strncpy(params.dst_addr, argv[1], IP4ADDR_STRLEN_MAX - 1);
  params.dst_addr[IP4ADDR_STRLEN_MAX - 1] = '\0';
  params.count = PING_DEFAULT_COUNT;
  params.interval_ms = PING_DEFAULT_INTERVAL;
  params.size = PING_DEFAULT_SIZE;

  while (current_arg < argc)
  {
    if (strncmp(argv[current_arg], "-c", 2) == 0)
    {
      if (current_arg + 1 >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      params.count = atoi(argv[current_arg + 1]);
      if (params.count < 1)
      {
        LogError("Ping count is invalid.\n");
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    else if (strncmp(argv[current_arg], "-i", 2) == 0)
    {
      if (current_arg + 1 >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      params.interval_ms = atoi(argv[current_arg + 1]);
      if (params.interval_ms < 100 || params.interval_ms > 3500)
      {
        LogError("Ping interval is invalid, valid range : [100;3500]\n");
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    else if (strcmp(argv[current_arg], "-s") == 0)
    {
      if (current_arg + 1 >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      params.size = atoi(argv[current_arg + 1]);
      if ((params.size < 1) || (params.size > PING_MAX_SIZE))
      {
        LogError("Ping size is invalid, valid range : [1;%" PRIu32 "].\n", PING_MAX_SIZE);
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
  }

  params.event = xEventGroupCreate();
  if (pdPASS != xTaskCreate(ping_ipv4_task, "Ping IPv4", 512, &params, 24, NULL))
  {
    return ret;
  }

  /* Wait for the ping task to complete */
  if (EVT_PING_DONE == xEventGroupWaitBits(params.event, EVT_PING_DONE | EVT_PING_ERROR,
                                           pdTRUE, pdFALSE, (params.count + 2) * PING_RCV_TIMEO))
  {
    ret = SHELL_STATUS_OK;
  }

  if (params.event)
  {
    vEventGroupDelete(params.event);
    params.event = NULL;
  }
  return ret;
}

SHELL_CMD_EXPORT_ALIAS(ping_ipv4_cmd, ping, ping <hostname> [ -c count [1; max(uint16_t) - 1] ]
                       [ -s size [1; 1470] ] [ -i interval [100; 3500] ]);
