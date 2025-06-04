/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    echo.c
  * @author  GPM Application Team
  * @brief   Test an echo with a server
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
#include <string.h>
#include <inttypes.h>

#include "echo.h"
#include "main.h"
#include "app_config.h"

#include "w6x_api.h"
#include "common_parser.h" /* Common Parser functions */

#include "shell.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Buffer to send and receive data */
static uint8_t buffer[ECHO_TRANSFER_SIZE_MAX + 1];

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
  * @retval number of errors
  */
static int32_t echo_process(uint32_t send_loop, uint32_t len);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
int32_t echo_sizes_loop(int32_t argc, char **argv)
{
  int32_t status;
  uint32_t iteration_count = ECHO_ITERATION_COUNT; /* Number of packets to be sent */
  uint32_t packet_size = ECHO_TRANSFER_SIZE_START; /* Minimal size of a packet */

  /* USER CODE BEGIN echo_sizes_loop_1 */

  /* USER CODE END echo_sizes_loop_1 */

  if ((argc == 2) && (sscanf(argv[1], "%" SCNu32 "", &iteration_count) != 1))
  {
    return -1;
  }

  LogInfo("\n\n***************ECHO TEST ***************\n");

  do /* Loop on the size of a group of packets */
  {
    /* Start the echo test */
    status = echo_process(iteration_count, packet_size);
    if (status != 0)
    {
      return status;
    }
    packet_size += ECHO_TRANSFER_SIZE_ITER; /* Increment the size of a group of packets */
  } while (packet_size <= ECHO_TRANSFER_SIZE_MAX);

  /* USER CODE BEGIN echo_sizes_loop_last */

  /* USER CODE END echo_sizes_loop_last */

  return 0;
}

SHELL_CMD_EXPORT_ALIAS(echo_sizes_loop, echo, echo [ iteration ]);

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void fill_buffer(uint8_t *buff, uint32_t len, uint32_t offset)
{
  uint8_t value;

  /* USER CODE BEGIN fill_buffer_1 */

  /* USER CODE END fill_buffer_1 */

  for (uint32_t byte = 0; byte < len - 1; byte++)
  {
    value = (uint8_t)(byte + offset);

    /* Work around for tcpbin.com echo server, avoid CR values */
    if (value == 0x0D)
    {
      value = 0x0C;
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
  uint8_t expected_value;
  uint32_t error_count = 0;

  /* USER CODE BEGIN check_buffer_1 */

  /* USER CODE END check_buffer_1 */

  for (uint32_t byte = 0; byte < len ; byte++)
  {
    expected_value = ((byte + offset) & 0xff);
    /* Work around for tcpbin.com echo server, last value is LF (\n) */
    if (byte == len - 1)
    {
      expected_value =  0x0A;
    }

    /* Work around for tcpbin.com echo server, avoid CR values */
    if (expected_value != 0x0D)
    {
      if (buff[byte] != expected_value)
      {
        LogInfo("Received data are different from data sent \"%" PRIu16 "\" <> \"%" PRIu16 "\" at index %" PRIu32 "\n",
                (uint8_t)(byte & 0xff), buff[byte], byte);
        error_count++;
      }
    }
  }

  /* USER CODE BEGIN check_buffer_last */

  /* USER CODE END check_buffer_last */

  return error_count;
}

static int32_t echo_process(uint32_t send_loop, uint32_t len)
{
#if (defined(NET_USE_IPV6) && (NET_USE_IPV6 == 1))
  net_sockaddr_in6_t s_addr_in = {0};
  const int32_t domain = AF_INET6;
#else
  uint8_t echo_server_addr[4] = {192, 168, 8, 1};
  struct sockaddr_in addr_t = {0};
#endif /* NET_USE_IPV6 */
  uint32_t tstart;
  uint32_t tstop;
  uint32_t transferred_bytes;
  uint32_t transferred_total = 0;
  uint32_t error_count = 0;

  char echo_server_url[] = ECHO_SERVER_URL;
  uint16_t echo_server_port = ECHO_SERVER_PORT;
  int32_t net_ret = 0;
  int32_t sock;

  /* USER CODE BEGIN echo_process_1 */

  /* USER CODE END echo_process_1 */

  /* Prepare a TCP socket */
  LogInfo("\nCreate a new socket\n");
  sock = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
  {
    LogInfo("Socket creation failed\n");
    goto _err;
  }
  else
  {
    LogInfo("Socket creation done\n");
  }

#if (defined(NET_USE_IPV6) && (NET_USE_IPV6 == 1))
  s_addr_in.sin6_len = sizeof(s_addr_in);
  s_addr_in.sin6_port = NET_HTONS(REMOTE_PORT);
  s_addr_in.sin6_family = domain;
  s_addr_in.sin6_flowinfo = 0;
  s_addr_in.sin6_scope_id = 0;

  s_addr_in.sin6_addr.un.u32_addr[0] = REMOTE_IPV6_ADDR0;
  s_addr_in.sin6_addr.un.u32_addr[1] = REMOTE_IPV6_ADDR1;
  s_addr_in.sin6_addr.un.u32_addr[2] = REMOTE_IPV6_ADDR2;
  s_addr_in.sin6_addr.un.u32_addr[3] = REMOTE_IPV6_ADDR3;
#else
  /* Resolve IP Address from the input URL */
  net_ret = W6X_Net_GetHostAddress(echo_server_url, echo_server_addr);
  if (net_ret == W6X_STATUS_OK)
  {
    LogInfo("IP Address from Hostname [%s]: " IPSTR "\n", echo_server_url, IP2STR(echo_server_addr));
  }
  else
  {
    LogError("IP Address identification failed\n");
    goto _err;
  }

  addr_t.sin_family = AF_INET;
  addr_t.sin_port = PP_HTONS(echo_server_port);
  addr_t.sin_addr_t.s_addr = ATON_R(echo_server_addr);
#endif /* NET_USE_IPV6 */

  LogInfo("\nConnecting the socket to %s\n", ECHO_SERVER_URL);
  net_ret = W6X_Net_Connect(sock, (struct sockaddr *)&addr_t, sizeof(addr_t));
  if (net_ret != 0)
  {
    LogInfo("Socket connection failed\n");
    goto _err;
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
    fill_buffer(buffer, len, i);
    {
      transferred_bytes = 0;
      do
      {
        int32_t count_done = W6X_Net_Send(sock, (struct sockaddr *)&buffer[transferred_bytes],
                                          len - transferred_bytes, 0);

        if (count_done < 0)
        {
          LogError("Failed to send data to echo server (%" PRId32 "), try again\n", count_done);
          count_done = 0;
        }
        transferred_bytes += count_done;
      } while (transferred_bytes < len);

      /* Update cumulative number with data that have been sent. */
      transferred_total += transferred_bytes;
    }

    /* Reset the buffer of data reception. */
    memset(buffer, 0x00, sizeof(buffer));

    transferred_bytes = 0;
    do
    {
      int32_t count_done = W6X_Net_Recv(sock, &buffer[transferred_bytes], len - transferred_bytes, 0);
      if (count_done < 0)
      {
        LogError("\nReceive failed with %" PRId32 "\n", count_done);
        goto end;
      }
      transferred_bytes += (uint32_t)count_done;
    } while (transferred_bytes < len);

    tstop = HAL_GetTick();

    error_count = check_buffer(buffer, len, i);

    /* Update cumulative number with data that have been received. */
    transferred_total += transferred_bytes;
  }

  if (error_count == 0)
  {
    LogInfo("\nSuccessful Echo Transfer and receive %" PRId32 " x %" PRId32 " with %" PRId32 " bytes"
            " in %" PRId32 " ms, br = %" PRId32 " Kbit/sec\n",
            send_loop, len, transferred_total, tstop - tstart, (transferred_total * 8) / (tstop - tstart));
  }
  else
  {
    LogInfo("\nError: Echo transfer, find %" PRId32 " different bytes\n", error_count);
  }

  /* USER CODE BEGIN echo_process_last */

  /* USER CODE END echo_process_last */

end:

  net_ret = W6X_Net_Close(sock); /* Close the TCP socket */
  if (net_ret != 0)
  {
    LogInfo("Socket close failed\n");
  }
  else
  {
    LogInfo("Socket closed\n");
  }

_err:

  return net_ret;
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
