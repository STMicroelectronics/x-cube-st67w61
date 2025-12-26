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
#define ECHO_TRANSFER_SIZE_START    1000
#endif /* ECHO_TRANSFER_SIZE_START */

#ifndef ECHO_TRANSFER_SIZE_MAX
/** Maximal size of a packet */
#define ECHO_TRANSFER_SIZE_MAX      2000
#endif /* ECHO_TRANSFER_SIZE_MAX */

#ifndef ECHO_TRANSFER_SIZE_ITER
/** To increment the size of a group of packets */
#define ECHO_TRANSFER_SIZE_ITER     250
#endif /* ECHO_TRANSFER_SIZE_ITER */

#ifndef ECHO_ITERATION_COUNT
/** Number of packets to be sent */
#define ECHO_ITERATION_COUNT        10
#endif /* ECHO_ITERATION_COUNT */

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Buffer to send and receive data */
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

  if (argc == 2)
  {
    iteration_count = (uint32_t)atoi(argv[1]);
  }

  LogInfo("\n\n***************ECHO TEST ***************\n");
  do /* Loop on the size of a group of packets */
  {
    /* Start the echo test */
    status = echo_process(iteration_count, packet_size);
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
  struct sockaddr_in addr_t = {0};
  uint8_t echo_server_addr[4] = {192, 168, 8, 1};

  uint32_t tstart;
  uint32_t tstop;
  uint32_t transferred_bytes;
  uint32_t transferred_total = 0;
  uint32_t error_count = 0;

  char echo_server_url[] = ECHO_SERVER_URL;
  uint16_t echo_server_port = ECHO_SERVER_PORT;
  int32_t net_ret = 0;
  int32_t ret_code = -1;
  int32_t sock;

  /* USER CODE BEGIN echo_process_1 */

  /* USER CODE END echo_process_1 */

  /* Resolve IP Address from the input URL */
  net_ret = W6X_Net_ResolveHostAddress(echo_server_url, echo_server_addr);
  if (net_ret != W6X_STATUS_OK)
  {
    LogError("DNS resolution failed for %s\n", echo_server_url);
    return ret_code; /* Socket not created yet: direct return avoids unnecessary cleanup jump */
  }
  LogInfo("IP Address from Hostname [%s]: " IPSTR "\n", echo_server_url, IP2STR(echo_server_addr));

  /* Prepare a TCP socket */
  LogInfo("\nCreate a new socket\n");
  sock = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
  {
    LogInfo("Socket creation failed\n");
    goto end;
  }
  LogInfo("Socket creation done\n");

  addr_t.sin_family = AF_INET;
  addr_t.sin_port = PP_HTONS(echo_server_port);
  addr_t.sin_addr.s_addr = ATON(echo_server_addr);

  LogInfo("Connecting the socket to %s\n", ECHO_SERVER_URL);
  net_ret = W6X_Net_Connect(sock, (struct sockaddr *)&addr_t, sizeof(addr_t));
  if (net_ret != 0)
  {
    LogError("Socket connection failed\n");
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
    fill_buffer(buffer, len, i);
    {
      transferred_bytes = 0;
      do
      {
        int32_t count_done = W6X_Net_Send(sock, &buffer[transferred_bytes], len - transferred_bytes, 0);

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
    uint32_t duration_ms = (tstop > tstart) ? (tstop - tstart) : 1U;
    LogInfo("Successful Echo Transfer and receive %" PRId32 " x %" PRId32 " with %" PRId32 " bytes"
            " in %" PRId32 " ms, br = %" PRId32 " Kbit/sec\n",
            send_loop, len, transferred_total, duration_ms, (transferred_total * 8) / duration_ms);
    ret_code = 0;
  }
  else
  {
    LogError("Error: Echo transfer, find %" PRId32 " different bytes\n", error_count);
    ret_code = -2; /* Distinct code for data mismatch */
  }

  /* USER CODE BEGIN echo_process_last */

  /* USER CODE END echo_process_last */

end:
  if (sock >= 0)
  {
    net_ret = W6X_Net_Close(sock); /* Close the TCP socket */
    if (net_ret != 0)
    {
      LogError("Socket close failed\n");
    }
    else
    {
      LogInfo("Socket closed\n");
    }
  }
  return ret_code;
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
