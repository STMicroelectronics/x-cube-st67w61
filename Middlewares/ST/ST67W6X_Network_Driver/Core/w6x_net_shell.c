/**
  ******************************************************************************
  * @file    w6x_net_shell.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x Net Shell Commands
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "w6x_api.h"
#include "shell.h"
#include "logging.h"
#include "common_parser.h" /* Common Parser functions */
#include "FreeRTOS.h"
#include "task.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W6X_Private_Net_Constants
  * @{
  */
#define PING_COUNT     4     /*!< Number of ping requests to send */
#define PING_SIZE      64    /*!< Size of the ping request */
#define PING_INTERVAL  1000  /*!< Time interval between two ping requests */
#define PING_MAX_SIZE  10000 /*!< Max size of the ping request to send */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Private_Net_Functions
  * @{
  */

/**
  * @brief  Ping shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_Ping(int32_t argc, char **argv);

/**
  * @brief  Get SNTP Time shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_GetTime(int32_t argc, char **argv);

/**
  * @brief  Get the IP address from the host name
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_DNS_LookUp(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
int32_t W6X_Shell_Net_Ping(int32_t argc, char **argv)
{
  uint16_t ping_count = PING_COUNT;
  uint32_t ping_size = PING_SIZE;
  uint32_t ping_interval = PING_INTERVAL;
  uint32_t average_ping = 0;
  uint16_t ping_received_response = 0;
  int32_t current_arg = 2;
  int32_t tmp = 0;

  if (argc < 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  while (current_arg < argc)
  {
    /* Parse the count argument */
    if (strncmp(argv[current_arg], "-c", 2) == 0)
    {
      if (current_arg + 1 >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the count value */
      if (Parser_StrToInt(argv[current_arg + 1], NULL, &tmp) == 0)
      {
        SHELL_E("Ping count is invalid.\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if (tmp < 1)
      {
        SHELL_E("Ping count is invalid.\n");
        return SHELL_STATUS_ERROR;
      }
      else
      {
        ping_count = tmp;
      }
      current_arg += 2;
    }
    /* Parse the size argument */
    else if (strncmp(argv[current_arg], "-s", 2) == 0)
    {
      if (current_arg + 1 >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if (Parser_StrToInt(argv[current_arg + 1], NULL, &tmp) == 0)
      {
        /* Invalid size value */
        SHELL_E("Ping size is invalid, valid range : [1;%" PRIu32 "].\n", PING_MAX_SIZE);
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if ((tmp < 1) || (tmp > PING_MAX_SIZE))
      {
        SHELL_E("Ping size is invalid, valid range : [1;%" PRIu32 "].\n", PING_MAX_SIZE);
        return SHELL_STATUS_ERROR;
      }
      else
      {
        ping_size = tmp;
      }
      current_arg += 2;
    }
    /* Parse the time interval argument */
    else if (strncmp(argv[current_arg], "-i", 2) == 0)
    {
      if (current_arg + 1 >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the interval value */
      if (Parser_StrToInt(argv[current_arg + 1], NULL, &tmp) == 0)
      {
        /* Invalid interval value */
        SHELL_E("Ping interval is invalid, valid range : [100;3500].\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if (tmp < 100 || tmp > 3500)
      {
        SHELL_E("Ping interval is invalid, valid range : [100;3500]\n");
        return SHELL_STATUS_ERROR;
      }
      else
      {
        ping_interval = tmp;
      }
      current_arg += 2;
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
  }

  if (W6X_STATUS_OK == W6X_Net_Ping((uint8_t *)argv[1], ping_size, ping_count, ping_interval, &average_ping,
                                    &ping_received_response))
  {
    if (ping_received_response == 0)
    {
      SHELL_E("No ping received\n");
      return SHELL_STATUS_ERROR;
    }
    else
    {
      SHELL_PRINTF("%" PRIu16 " packets transmitted, %" PRIu16 " received, %" PRIu16
                   "%% packet loss, time %" PRIu32 "ms\n",
                   ping_count, ping_received_response,
                   100 * (ping_count - ping_received_response) / ping_count, average_ping);
    }
  }
  else
  {
    SHELL_E("Ping Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

/** Shell command to ping a host */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_Ping, ping,
                       ping <hostname> [ -c count [1; max(uint16_t) - 1] ]
                       [ -s size [1; 10000] ] [ -i interval [100; 3500] ]);

int32_t W6X_Shell_Net_GetTime(int32_t argc, char **argv)
{
  int32_t ret = SHELL_STATUS_ERROR;
  uint8_t Time[32] = {0};
  uint8_t Enable = 0;
  int16_t Timezone_current = 0;
  int16_t Timezone_expected = 0;
  uint32_t dtim = 0;
  int32_t tmp = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the current timezone */
  if (Parser_StrToInt(argv[1], NULL, &tmp) == 0)
  {
    SHELL_E("Invalid timezone value\n");
    return -1;
  }
  Timezone_expected = (int16_t)tmp;

  /* Get the current SNTP configuration */
  if (W6X_Net_GetSNTPConfiguration(&Enable, &Timezone_current, NULL, NULL, NULL) != W6X_STATUS_OK)
  {
    SHELL_E("Get SNTP Configuration failed\n");
    return ret;
  }

  if ((W6X_WiFi_GetDTIM(&dtim) != W6X_STATUS_OK) || (W6X_WiFi_SetDTIM(0) != W6X_STATUS_OK))
  {
    return SHELL_STATUS_ERROR;
  }

  /* Set the SNTP configuration if not already set or if the timezone is different */
  if ((Enable == 0) || (Timezone_current != Timezone_expected))
  {
    if (W6X_Net_SetSNTPConfiguration(1, Timezone_expected, (uint8_t *)"0.pool.ntp.org",
                                     (uint8_t *)"time.google.com", NULL) != W6X_STATUS_OK)
    {
      SHELL_E("Set SNTP Configuration failed\n");
      goto _err;
    }
    vTaskDelay(5000); /* Wait few seconds to execute the first request */
  }

  /* Get the time */
  if (W6X_STATUS_OK == W6X_Net_GetTime(Time))
  {
    SHELL_PRINTF("Time: %s\n", Time);
    ret = SHELL_STATUS_OK;
  }
  else
  {
    SHELL_E("Time Failure\n");
  }

_err:
  (void)W6X_WiFi_SetDTIM(dtim);
  return ret;
}

/** Shell command to get the time from SNTP server */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_GetTime, time, time < timezone : UTC format : range [-12; 14] or
                       HHmm format : with HH in range [-12; +14] and mm in range [00; 59] >);

/* W61_Net_DNS_LookUp */
int32_t W6X_Shell_Net_DNS_LookUp(int32_t argc, char **argv)
{
  uint8_t ipaddr[4] = {0};

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the IP address from the host name */
  if (W6X_Net_GetHostAddress(argv[1], ipaddr) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("IP address: %" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16 "\n",
                 ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
  }
  else
  {
    SHELL_E("DNS Lookup failed\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

/** Shell command to get the IP address from the host name */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_DNS_LookUp, dnslookup, dnslookup <hostname>);

/** @} */
