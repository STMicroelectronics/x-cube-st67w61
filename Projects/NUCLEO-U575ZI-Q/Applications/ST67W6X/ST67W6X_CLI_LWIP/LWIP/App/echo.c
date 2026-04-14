/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    echo.c
  * @author  ST67 Application Team
  * @brief   Test an echo with a server
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
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "echo.h"
#include "main.h"
#include "app_config.h"

#include "w6x_api.h"
#include "common_parser.h" /* Common Parser functions */

#include "shell.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "errno.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Structure to store DNS resolve context
  */
typedef struct
{
  ip_addr_t resolved_ip;      /*!< Resolved IP address */
  volatile int32_t done;      /*!< Flag to indicate if resolution is done */
  int32_t result;             /*!< Result of the resolution */
} echo_dns_resolve_context_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#ifndef ECHO_SERVER_URL
/** URL of Echo TCP remote server */
#define ECHO_SERVER_URL             "tcpbin.com"
#endif /* ECHO_SERVER_URL */

#ifndef ECHO_SERVER_PORT
/** Port of Echo TCP remote server */
#define ECHO_SERVER_PORT            4242
#endif /* ECHO_SERVER_PORT */

#ifndef ECHO_TRANSFER_SIZE_START
/** Minimal size of a packet */
#define ECHO_TRANSFER_SIZE_START    1000U
#endif /* ECHO_TRANSFER_SIZE_START */

#ifndef ECHO_TRANSFER_SIZE_MAX
/** Maximal size of a packet */
#define ECHO_TRANSFER_SIZE_MAX      2000U
#endif /* ECHO_TRANSFER_SIZE_MAX */

#ifndef ECHO_TRANSFER_SIZE_ITER
/** To increment the size of a group of packets */
#define ECHO_TRANSFER_SIZE_ITER     250U
#endif /* ECHO_TRANSFER_SIZE_ITER */

#ifndef ECHO_ITERATION_COUNT
/** Number of packets to be sent */
#define ECHO_ITERATION_COUNT        10U
#endif /* ECHO_ITERATION_COUNT */

/** Maximal number of iterations for the echo test */
#define ECHO_MAX_ITERATION_COUNT    1000U

/** DNS resolve timeout in milliseconds */
#define DNS_RESOLVE_TIMEOUT_MS      5000

/** Timeout for socket send/receive operations in milliseconds */
#define ECHO_TIMEOUT_MS             5000

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Buffer to send and receive data */
static uint8_t *echo_buffer = NULL;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Fill a buffer with an incrementing pattern
  * @param  buff: buffer to fill
  * @param  len: size of the buffer
  * @param  offset: offset to start the pattern
  */
static void fill_buffer(uint8_t *buff, uint32_t len, uint32_t offset);

/**
  * @brief  Check a buffer with an incrementing pattern
  * @param  buff: buffer to check
  * @param  len: size of the buffer
  * @param  offset: offset to start the pattern
  * @retval number of errors
  */
static uint32_t check_buffer(uint8_t *buff, uint32_t len, uint32_t offset);

/**
  * @brief  Process the echo test
  * @param  send_loop: number of loop to send
  * @param  len: size of the buffer
  * @param  use_ipv6: 0 for ipv4, 1 for ipv6
  * @retval number of errors
  */
static int32_t echo_process(uint32_t send_loop, uint32_t len, bool use_ipv6);

/**
  * @brief  DNS lookup callback function for lwIP
  * @param  name: pointer to the hostname string
  * @param  ipaddr: pointer to the resolved IP address (NULL if failed)
  * @param  arg: user-supplied argument (unused)
  */
static void echo_dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg);

/**
  * @brief  Get IPv6 or IPv4 address from URL using DNS
  * @param  hostUrl: Host URL
  * @param  use_ipv6: 0 for ipv4, 1 for ipv6
  * @param  out_ip: the returned IP address
  * @retval 0 on success, -1 on failure, -2 on timeout
  */
static int32_t echo_resolve_host_address(const char *hostUrl, bool use_ipv6, ip_addr_t *out_ip);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
int32_t echo_sizes_loop(int32_t argc, char **argv)
{
  int32_t status;
  uint32_t iteration_count = ECHO_ITERATION_COUNT; /* Number of packets to be sent */
  uint32_t packet_size = ECHO_TRANSFER_SIZE_START; /* Minimal size of a packet */
  bool use_ipv6 = false;

  /* USER CODE BEGIN echo_sizes_loop_1 */

  /* USER CODE END echo_sizes_loop_1 */

  if (argc > 1)
  {
    int32_t current_arg = 1;
    while (current_arg < argc)
    {
      if (argv[current_arg][0] != '-')
      {
        iteration_count = (uint32_t)atoi(argv[current_arg]);
        if ((iteration_count == 0U) || (iteration_count > ECHO_MAX_ITERATION_COUNT))
        {
          return -254;
        }
      }
      else if (strncmp(argv[current_arg], "-v", 2) == 0)
      {
        use_ipv6 = true;
      }
      else
      {
        return -254;
      }
      current_arg++;
    }
  }

  LogInfo("\n\n***************ECHO TEST ***************\n");
  do /* Loop on the size of a group of packets */
  {
    /* Start the echo test */
    status = echo_process(iteration_count, packet_size, use_ipv6);
    if (status != 0)
    {
      break;
    }
    packet_size += ECHO_TRANSFER_SIZE_ITER; /* Increment the size of a group of packets */
  } while (packet_size <= ECHO_TRANSFER_SIZE_MAX);

  if (status == 0)
  {
    LogInfo("Echo test successful\n");
  }
  else
  {
    LogError("Echo test in error %" PRIi32 "\n", status);
  }

  /* USER CODE BEGIN echo_sizes_loop_last */

  /* USER CODE END echo_sizes_loop_last */

  return status;
}

SHELL_CMD_EXPORT_ALIAS(echo_sizes_loop, echo, echo [ iteration [1; 1000] ] [-v [IPv6 mode]]);

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void fill_buffer(uint8_t *buff, uint32_t len, uint32_t offset)
{
  /* USER CODE BEGIN fill_buffer_1 */

  /* USER CODE END fill_buffer_1 */
  if (len == 0U)
  {
    return;
  }

  for (uint32_t byte = 0; byte < (len - 1U); byte++)
  {
    uint8_t value = (uint8_t)(byte + offset);

    /* Work around for tcpbin.com echo server, avoid CR values */
    if (value == 0x0DU)
    {
      value = 0x0CU;
    }
    *buff++ = value;
  }

  /* Work around for tcpbin.com echo server, replace last value with LF (\n) */
  *buff = 0x0A;

  /* USER CODE BEGIN fill_buffer_last */

  /* USER CODE END fill_buffer_last */
}

static uint32_t check_buffer(uint8_t *buff, uint32_t len, uint32_t offset)
{
  uint32_t error_count = 0;

  /* USER CODE BEGIN check_buffer_1 */

  /* USER CODE END check_buffer_1 */

  for (uint32_t byte = 0; byte < len; byte++)
  {
    uint8_t expected_value = (uint8_t)(byte + offset);
    /* Work around for tcpbin.com echo server, last value is LF (\n) */
    if (byte == (len - 1U))
    {
      expected_value =  0x0AU;
    }

    /* Work around for tcpbin.com echo server, avoid CR values */
    if (expected_value != 0x0DU)
    {
      if (buff[byte] != expected_value)
      {
        LogInfo("Received data are different from data sent \"%" PRIu16 "\" <> \"%" PRIu16 "\" at index %" PRIu32 "\n",
                (uint8_t)(byte & 0xFFU), buff[byte], byte);
        error_count++;
      }
    }
  }

  /* USER CODE BEGIN check_buffer_last */

  /* USER CODE END check_buffer_last */

  return error_count;
}

static int32_t echo_process(uint32_t send_loop, uint32_t len, bool use_ipv6)
{
  struct sockaddr_in addr_t = {0};
  int32_t domain = AF_INET;
  int32_t protocol = IPPROTO_TCP;
  ip_addr_t resolved_ip;
#if (LWIP_IPV6 == 1)
  struct sockaddr_in6 addr6_t = {0};
#else
  use_ipv6 = 0;
#endif /* LWIP_IPV6 */

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int32_t timeout = ECHO_TIMEOUT_MS;
#else
  struct timeval timeout;
  timeout.tv_sec = ECHO_TIMEOUT_MS / 1000;
  timeout.tv_usec = (ECHO_TIMEOUT_MS % 1000) * 1000;
#endif /* LWIP_SO_SNDRCVTIMEO_NONSTANDARD */

  uint32_t tstart = 0U;
  uint32_t tstop = 0U;
  uint32_t transferred_bytes;
  uint32_t transferred_total = 0;
  uint32_t error_count = 0;

  char echo_server_url[] = ECHO_SERVER_URL;
  uint16_t echo_server_port = ECHO_SERVER_PORT;
  int32_t net_ret = 0;
  int32_t ret_code = -1;
  int32_t sock = -1;

  /* USER CODE BEGIN echo_process_1 */

  /* USER CODE END echo_process_1 */

  if (echo_buffer != NULL)
  {
    LogError("echo buffer already allocated\n");
    goto end;
  }

  echo_buffer = pvPortMalloc(ECHO_TRANSFER_SIZE_MAX + 1U);
  if (echo_buffer == NULL)
  {
    LogError("echo buffer allocation failed\n");
    goto end;
  }

  /* Resolve IP Address from the input URL */
  net_ret = echo_resolve_host_address(echo_server_url, use_ipv6, &resolved_ip);
  if (net_ret != 0)
  {
    LogError("DNS resolution failed for %s\n", echo_server_url);
    goto end; /* Socket not created yet: direct return avoids unnecessary cleanup jump */
  }
  LogInfo("Resolved %s address: %s\n", use_ipv6 ? "IPv6" : "IPv4", ipaddr_ntoa(&resolved_ip));

  /* Prepare a TCP socket */
  LogInfo("\nCreate a new socket\n");
#if (LWIP_IPV6 == 1)
  if (use_ipv6)
  {
    domain = AF_INET6;
    protocol = IPPROTO_IPV6;
  }
#endif /* LWIP_IPV6 */

  sock = lwip_socket(domain, SOCK_STREAM, protocol);
  if (sock < 0)
  {
    LogError("Socket creation failed (errno=%d)\n", errno);
    goto end;
  }
  LogInfo("Socket creation done\n");

  /* Set socket timeouts */
  (void)lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  (void)lwip_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

#if (LWIP_IPV6 == 1)
  if (use_ipv6)
  {
    addr6_t.sin6_family = AF_INET6;
    addr6_t.sin6_port = PP_HTONS(echo_server_port);
    (void)memcpy(&addr6_t.sin6_addr, ip_2_ip6(&resolved_ip), sizeof(addr6_t.sin6_addr));

    LogInfo("Connecting the socket to %s (IPv6)\n", echo_server_url);
    net_ret = lwip_connect(sock, (struct sockaddr *)&addr6_t, sizeof(addr6_t));
  }
  else
#endif /* LWIP_IPV6 */
  {
    addr_t.sin_family = AF_INET;
    addr_t.sin_port = PP_HTONS(echo_server_port);
    addr_t.sin_addr.s_addr = ip4_addr_get_u32(ip_2_ip4(&resolved_ip));

    LogInfo("Connecting the socket to %s (IPv4)\n", echo_server_url);
    net_ret = lwip_connect(sock, (struct sockaddr *)&addr_t, sizeof(addr_t));
  }
  if (net_ret != 0)
  {
    LogError("Socket connection failed (ret=%d errno=%d)\n", net_ret, errno);
    ret_code = net_ret;
    goto end;
  }
  else
  {
    LogInfo("Socket opened\n");
  }

  LogInfo("- Device connected\n");
  LogInfo("- Starting transfer\n");

  tstart = HAL_GetTick();
  tstop = HAL_GetTick();

  for (uint32_t i = 0; i < send_loop; i++)
  {
    fill_buffer(echo_buffer, len, i);
    {
      transferred_bytes = 0;
      do
      {
        int32_t count_done = lwip_send(sock, &echo_buffer[transferred_bytes], len - transferred_bytes, 0);

        if (count_done < 0)
        {
          LogError("Failed to send data to echo server (%" PRId32 "), try again\n", count_done);
          count_done = 0;
        }
        transferred_bytes += (uint32_t)count_done;
      } while (transferred_bytes < len);

      /* Update cumulative number with data that have been sent. */
      transferred_total += transferred_bytes;
    }

    /* Reset the buffer of data reception. */
    (void)memset(echo_buffer, 0x00, ECHO_TRANSFER_SIZE_MAX + 1U);

    transferred_bytes = 0;
    do
    {
      int32_t count_done = lwip_recv(sock, &echo_buffer[transferred_bytes], len - transferred_bytes, 0);
      if (count_done < 0)
      {
        LogError("\nReceive failed with %" PRId32 "\n", count_done);
        ret_code = -1;
        goto end;
      }
      transferred_bytes += (uint32_t)count_done;
    } while (transferred_bytes < len);

    tstop = HAL_GetTick();

    error_count += check_buffer(echo_buffer, len, i);

    /* Update cumulative number with data that have been received. */
    transferred_total += transferred_bytes;
  }

  ret_code = 0; /* Success */

  /* USER CODE BEGIN echo_process_last */

  /* USER CODE END echo_process_last */

end:
  if (sock >= 0)
  {
    net_ret = lwip_close(sock); /* Close the TCP socket */
    if (net_ret != 0)
    {
      LogError("Socket close failed\n");
    }
    else
    {
      LogInfo("Socket closed\n");
    }
    /* Delay to Allow lwIP to free resources */
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  if (echo_buffer != NULL)
  {
    vPortFree(echo_buffer);
    echo_buffer = NULL;
  }

  if ((ret_code == 0) && (error_count == 0U))
  {
    uint32_t duration_ms = (tstop > tstart) ? (tstop - tstart) : 1U;
    LogInfo("Successful Echo Transfer and receive %" PRId32 " x %" PRId32 " with %" PRId32 " bytes"
            " in %" PRId32 " ms, br = %" PRId32 " Kbit/sec\n",
            send_loop, len, transferred_total, duration_ms, (transferred_total * 8U) / duration_ms);
  }
  else
  {
    if (error_count > 0U)
    {
      LogError("Error: Echo transfer, find %" PRId32 " different bytes\n", error_count);
      ret_code = -2; /* Distinct code for data mismatch */
    }
  }

  return ret_code;
}

static void echo_dns_lookup_callback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
  echo_dns_resolve_context_t *ctx = (echo_dns_resolve_context_t *)arg;
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

static int32_t echo_resolve_host_address(const char *hostUrl, bool use_ipv6, ip_addr_t *out_ip)
{
  if ((hostUrl == NULL) || (out_ip == NULL))
  {
    return -1;
  }

  echo_dns_resolve_context_t ctx;
  (void)memset(&ctx, 0, sizeof(ctx));
  ctx.result = -1;

  uint8_t addrtype = use_ipv6 ? LWIP_DNS_ADDRTYPE_IPV6 : LWIP_DNS_ADDRTYPE_IPV4;
  err_t err = dns_gethostbyname_addrtype(hostUrl, &ctx.resolved_ip, echo_dns_lookup_callback, &ctx, addrtype);
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
    TickType_t timeoutTick = startTick + pdMS_TO_TICKS(DNS_RESOLVE_TIMEOUT_MS);
    while (ctx.done == 0)
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
