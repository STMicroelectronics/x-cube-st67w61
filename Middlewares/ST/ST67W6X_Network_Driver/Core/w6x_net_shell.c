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
#include "w6x_types.h"     /* W6X_ARCH_** */
#if (ST67_ARCH == W6X_ARCH_T01)
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
#define PING_MAX_SIZE  10000 /*!< Max size of the ping request to send */

#define PING_TIMEOUT   1000  /*!< Default timeout value for ping request */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Private_Net_Functions
  * @{
  */

/**
  * @brief  Wi-Fi get/set hostname shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_Hostname(int32_t argc, char **argv);

/**
  * @brief  Get/set STA IP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_Station_IP(int32_t argc, char **argv);

/**
  * @brief  Get/set STA Gateway shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_Station_DNS(int32_t argc, char **argv);

/**
  * @brief  Get Soft-AP IP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_AP_IP(int32_t argc, char **argv);

/**
  * @brief  Get/set DHCP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_DHCP_Config(int32_t argc, char **argv);

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
int32_t W6X_Shell_Net_SNTP_GetTime(int32_t argc, char **argv);

/**
  * @brief  Get the IP address from the host name
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_Net_ResolveHostAddress(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
int32_t W6X_Shell_Net_Hostname(int32_t argc, char **argv)
{
  uint8_t hostname[34] = {0};

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get the host name */
    if (W6X_Net_GetHostname(hostname) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Host name : %s\n", hostname);
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
      return SHELL_STATUS_ERROR;
    }

    /* Set the host name */
    strncpy((char *)hostname, argv[1], sizeof(hostname) - 1);
    if (W6X_Net_SetHostname(hostname) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Host name set successfully\n");
    }
    else
    {
      SHELL_PRINTF("Set host name failed\n");
      return SHELL_STATUS_ERROR;
    }
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

int32_t W6X_Shell_Net_Station_IP(int32_t argc, char **argv)
{
  uint8_t ip_addr[4] = {0};
  uint8_t gateway_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};
#if (W6X_NET_IPV6_ENABLE == 1)
  ip6_addr_t Ip6ll_addr = {0};
  ip6_addr_t Ip6gl_addr = {0};
  char ipv6ll[INET6_ADDRSTRLEN] = {0};
  char ipv6gl[INET6_ADDRSTRLEN] = {0};
#endif /* W6X_NET_IPV6_ENABLE */
#if (SHELL_CMD_LEVEL >= 0)
  if (argc == 1)
  {
    /* Get the STA IP configuration */
    if (W6X_Net_Station_GetIPAddress(ip_addr, gateway_addr, netmask_addr) == W6X_STATUS_OK)
    {
      /* Display the IP configuration */
      SHELL_PRINTF("STA IP :\n");
      SHELL_PRINTF("IP :              " IPSTR "\n", IP2STR(ip_addr));
      SHELL_PRINTF("Gateway :         " IPSTR "\n", IP2STR(gateway_addr));
      SHELL_PRINTF("Netmask :         " IPSTR "\n", IP2STR(netmask_addr));
#if (W6X_NET_IPV6_ENABLE == 1)
      if (W6X_Net_Station_GetIPv6Address(&Ip6ll_addr, &Ip6gl_addr) == W6X_STATUS_OK)
      {
        /* Each ip6_addr_t holds 4 host-order 32-bit words; inet_ntop applies byte-order conversion */
        uint8_t ll_zero = (Ip6ll_addr.addr[0] == 0 && Ip6ll_addr.addr[1] == 0
                           && Ip6ll_addr.addr[2] == 0 && Ip6ll_addr.addr[3] == 0);
        uint8_t gl_zero = (Ip6gl_addr.addr[0] == 0 && Ip6gl_addr.addr[1] == 0
                           && Ip6gl_addr.addr[2] == 0 && Ip6gl_addr.addr[3] == 0);
        if (!ll_zero)
        {
          (void)W6X_Net_Inet_ntop(AF_INET6, Ip6ll_addr.addr, ipv6ll, sizeof(ipv6ll));
        }
        if (!gl_zero)
        {
          (void)W6X_Net_Inet_ntop(AF_INET6, Ip6gl_addr.addr, ipv6gl, sizeof(ipv6gl));
        }
        SHELL_PRINTF("IPv6 link-local : %s\n", ll_zero ? "not assigned" : ipv6ll);
        SHELL_PRINTF("IPv6 global 1   : %s\n", gl_zero ? "not assigned" : ipv6gl);
        SHELL_PRINTF("IPv6 global 2   : not assigned\n");
      }
#endif /* W6X_NET_IPV6_ENABLE */
    }
    else
    {
      SHELL_E("Get STA IP error\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if ((argc > 1) && (argc <= 4))
  {
    /* Set the STA IP configuration in IP, Gateway, Netmask fixed order. Gateway and Netmask are optional */
    for (int32_t i = 1; i < argc; i++)
    {
      switch (i)
      {
        case 1:
          /* Parse the IP address */
          Parser_StrToIP(argv[i], ip_addr);
          if (Parser_CheckValidAddress(ip_addr, 4) != 0)
          {
            SHELL_E("IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 2:
          /* Parse the Gateway address */
          Parser_StrToIP(argv[i], gateway_addr);
          if (Parser_CheckValidAddress(gateway_addr, 4) != 0)
          {
            SHELL_E("Gateway IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 3:
          /* Parse the Netmask address */
          Parser_StrToIP(argv[i], netmask_addr);
          if (Parser_CheckValidAddress(netmask_addr, 4) != 0)
          {
            SHELL_E("Netmask IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
      }
    }

    /* Set the IP configuration */
    if (W6X_Net_Station_SetIPAddress(ip_addr, gateway_addr, netmask_addr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("STA IP configuration set successfully\n");
    }
    else
    {
      SHELL_E("Set STA IP configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to get/set the STA IP configuration */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_Station_IP, net_sta_ip, net_sta_ip [ IP addr ] [ Gateway addr ] [ Netmask addr ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Net_Station_DNS(int32_t argc, char **argv)
{
  uint8_t dns1_addr[4] = {0};
  uint8_t dns2_addr[4] = {0};
  uint8_t dns3_addr[4] = {0};

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get the STA DNS configuration */
    if (W6X_Net_GetDnsAddress(dns1_addr, dns2_addr, dns3_addr) == W6X_STATUS_OK)
    {
      /* Display the DNS configuration */
      SHELL_PRINTF("DNS1 IP :   " IPSTR "\n", IP2STR(dns1_addr));
      SHELL_PRINTF("DNS2 IP :   " IPSTR "\n", IP2STR(dns2_addr));
      SHELL_PRINTF("DNS3 IP :   " IPSTR "\n", IP2STR(dns3_addr));
      return SHELL_STATUS_OK;
    }
    else
    {
      SHELL_E("Get STA DNS configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
#endif /* SHELL_CMD_LEVEL */

  if ((argc > 1) && (argc <= 4))
  {
    /* Set the STA DNS configuration in enable, DNS1, DNS2, DNS3 fixed order. DNS2 and DNS3 are optional */
    for (int32_t i = 1; i < argc; i++)
    {
      switch (i)
      {
        case 1:
          /* Parse the DNS1 address */
          Parser_StrToIP(argv[i], dns1_addr);
          if (Parser_CheckValidAddress(dns1_addr, 4) != 0)
          {
            SHELL_E("DNS IP 1 invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 2:
          /* Parse the DNS2 address */
          Parser_StrToIP(argv[i], dns2_addr);
          if (Parser_CheckValidAddress(dns2_addr, 4) != 0)
          {
            SHELL_E("DNS IP 2 invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 3:
          /* Parse the DNS3 address */
          Parser_StrToIP(argv[i], dns3_addr);
          if (Parser_CheckValidAddress(dns3_addr, 4) != 0)
          {
            SHELL_E("DNS IP 3 invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
      }
    }

    /* Set the DNS configuration */
    if (W6X_Net_SetDnsAddress(dns1_addr, dns2_addr, dns3_addr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("DNS configuration set successfully\n");
      return SHELL_STATUS_OK;
    }
    else
    {
      SHELL_E("Set DNS configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get/set the STA DNS configuration */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_Station_DNS, net_sta_dns,
                       net_sta_dns [ DNS1 addr ] [ DNS2 addr ] [ DNS3 addr ]);
#endif /* SHELL_CMD_LEVEL */

/** Shell command to get the Soft-AP IP configuration */
int32_t W6X_Shell_Net_AP_IP(int32_t argc, char **argv)
{
  uint8_t ip_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};

#if (SHELL_CMD_LEVEL >= 0)
  if (argc == 1)
  {
    /* Get the Soft-AP IP configuration */
    if (W6X_Net_AP_GetIPAddress(ip_addr, netmask_addr) == W6X_STATUS_OK)
    {
      /* Display the IP configuration */
      SHELL_PRINTF("Soft-AP :\n");
      SHELL_PRINTF("IP :      " IPSTR "\n", IP2STR(ip_addr));
      SHELL_PRINTF("Netmask : " IPSTR "\n", IP2STR(netmask_addr));
    }
    else
    {
      SHELL_E("Get Soft-AP IP error\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if ((argc > 1) && (argc <= 3))
  {
    /* Set the Soft-AP IP configuration in IP, Netmask fixed order */
    for (int32_t i = 1; i < argc; i++)
    {
      switch (i)
      {
        case 1:
          /* Parse the Soft-AP IP address */
          Parser_StrToIP(argv[i], ip_addr);
          if (Parser_CheckValidAddress(ip_addr, 4) != 0)
          {
            SHELL_E("Soft-AP IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 2:
          /* Parse the Netmask address */
          Parser_StrToIP(argv[i], netmask_addr);
          if (Parser_CheckValidAddress(netmask_addr, 4) != 0)
          {
            SHELL_E("Netmask IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
      }
    }

    /* Set the Soft-AP IP configuration */
    if (W6X_Net_AP_SetIPAddress(ip_addr, netmask_addr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Soft-AP IP configuration set successfully\n");
    }
    else
    {
      SHELL_E("Set Soft-AP IP configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_AP_IP, net_ap_ip, net_ap_ip);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Net_DHCP_Config(int32_t argc, char **argv)
{
  uint32_t lease_time = 0;
  uint32_t operate = 0;
  W6X_Net_DhcpType_e state = W6X_NET_DHCP_DISABLED;

#if (SHELL_CMD_LEVEL >= 1)
  uint8_t start_ip[4] = {0};
  uint8_t end_ip[4] = {0};
  if (argc == 1)
  {
    /* Get the DHCP server configuration */
    if (W6X_Net_GetDhcp(&state, &lease_time, start_ip, end_ip) == W6X_STATUS_OK)
    {
      /* Display the DHCP server configuration */
      SHELL_PRINTF("DHCP STA STATE :     %" PRIu32 "\n", state & 0x01);
      SHELL_PRINTF("DHCP AP STATE :      %" PRIu32 "\n", (state & 0x02) ? 1 : 0);
      SHELL_PRINTF("DHCP AP RANGE :      [" IPSTR " - " IPSTR "]\n", IP2STR(start_ip), IP2STR(end_ip));
      SHELL_PRINTF("DHCP AP LEASE TIME : %" PRIu32 "\n", lease_time);
    }
    else
    {
      SHELL_E("Get DHCP server configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc >= 3)
  {
    /* Get the DHCP client requested mode */
    operate = (uint32_t)atoi(argv[1]);
    if (operate > 1)
    {
      SHELL_E("First parameter should be 0 to disable, or 1 to enable DHCP client\n");
      return SHELL_STATUS_ERROR;
    }

    /* Get the DHCP client requested mask: 0b01 for STA only, 0b10 for Soft-AP only, 0b11 for STA + Soft-AP */
    state = (W6X_Net_DhcpType_e)atoi(argv[2]);
    if (!((state == W6X_NET_DHCP_STA_ENABLED) ||
          (state == W6X_NET_DHCP_AP_ENABLED) || (state == W6X_NET_DHCP_STA_AP_ENABLED)))
    {
      SHELL_E("Second parameter should be 1 to configure STA only, 2 to Soft-AP only, 3 for STA + Soft-AP. "
              "0 won't configure any\n");
      return SHELL_STATUS_ERROR;
    }

    if (argc == 4)
    {
      /* DHCP Server configuration */
      /* Parse the lease time */
      lease_time = (uint32_t)atoi(argv[3]);
      if ((lease_time < 1) || (lease_time > 2880))
      {
        SHELL_E("Lease time out of range [1;2880]\n");
        return SHELL_STATUS_ERROR;
      }
    }

    if (W6X_Net_SetDhcp(&state, &operate, lease_time) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("DHCP configuration succeed\n");
    }
    else
    {
      SHELL_E("DHCP configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_DHCP_Config, net_dhcp,
                       net_dhcp [ 0:DHCP disabled; 1:DHCP enabled ]
                       [ 1:STA only; 2:AP only; 3:STA + AP ] [ lease_time [1; 2880] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Net_Ping(int32_t argc, char **argv)
{
  uint16_t ping_count = 4;
  uint32_t ping_size = 0;
  uint32_t ping_interval = 0;
  uint32_t average_ping = 0;
  uint32_t ping_timeout = PING_TIMEOUT;
  uint16_t ping_received_response = 0;
  int32_t current_arg = 2;

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
      ping_count = (uint16_t)atoi(argv[current_arg + 1]);
      if (ping_count < 1)
      {
        SHELL_E("Ping count is invalid.\n");
        return SHELL_STATUS_ERROR;
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
      ping_size = (uint32_t)atoi(argv[current_arg + 1]);
      if ((ping_size < 1) || (ping_size > PING_MAX_SIZE))
      {
        SHELL_E("Ping size is invalid, valid range : [1;%" PRIu32 "].\n", PING_MAX_SIZE);
        return SHELL_STATUS_ERROR;
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
      ping_interval = (uint32_t)atoi(argv[current_arg + 1]);
      if (ping_interval < 100 || ping_interval > 3500)
      {
        SHELL_E("Ping interval is invalid, valid range : [100;3500]\n");
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    /* Parse the timeout argument */
    else if (strncmp(argv[current_arg], "-t", 2) == 0)
    {
      if (current_arg + 1 >= argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the timeout value */
      ping_timeout = (uint32_t)atoi(argv[current_arg + 1]);
      if (ping_timeout < 100 || ping_timeout > 3500)
      {
        SHELL_E("Ping timeout is invalid, valid range : [100;3500]\n");
        return SHELL_STATUS_ERROR;
      }
      current_arg += 2;
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
  }

  if (ping_timeout < ping_interval)
  {
    SHELL_E("Ping timeout must be greater than or equal to ping interval. Value adjusted.\n");
    ping_timeout = ping_interval;
  }

  if (W6X_STATUS_OK == W6X_Net_Ping((uint8_t *)argv[1], ping_size, ping_count, ping_interval, ping_timeout,
                                    &average_ping, &ping_received_response))
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

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to ping a host */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_Ping, ping,
                       ping <hostname> [ -c count [1; max(uint16_t) - 1] ]
                       [ -s size [1; 10000] ] [ -i interval [100; 3500] ] [ -t timeout [100; 3500] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Net_SNTP_GetTime(int32_t argc, char **argv)
{
  int32_t ret = SHELL_STATUS_ERROR;
  W6X_Net_Time_t Time = {0};
  uint8_t Enable = 0;
  int16_t Timezone_current = 0;
  int16_t Timezone_expected = 0;
  uint32_t ps_mode = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the current timezone */
  Timezone_expected = (int16_t)atoi(argv[1]);

  /* Get the current SNTP configuration */
  if (W6X_Net_SNTP_GetConfiguration(&Enable, &Timezone_current, NULL, NULL, NULL) != W6X_STATUS_OK)
  {
    SHELL_E("Get SNTP Configuration failed\n");
    return ret;
  }

  /* Save and disable low power config */
  if ((W6X_GetPowerMode(&ps_mode) != W6X_STATUS_OK) || (W6X_SetPowerMode(0) != W6X_STATUS_OK))
  {
    return ret;
  }

  /* Set the SNTP configuration if not already set or if the timezone is different */
  if ((Enable == 0) || (Timezone_current != Timezone_expected))
  {
    if (W6X_Net_SNTP_SetConfiguration(1, Timezone_expected, (uint8_t *)"0.pool.ntp.org",
                                      (uint8_t *)"time.google.com", NULL) != W6X_STATUS_OK)
    {
      SHELL_E("Set SNTP Configuration failed\n");
      goto _err;
    }
    SHELL_PRINTF("SNTP: Getting time from server (can take up to 5000 ms)...\n");
    vTaskDelay(5000); /* Wait few seconds to execute the first request */
  }

  /* Get the time */
  if (W6X_STATUS_OK == W6X_Net_SNTP_GetTime(&Time))
  {
    SHELL_PRINTF("Time: %s\n", Time.raw);
    ret = SHELL_STATUS_OK;
  }
  else
  {
    SHELL_E("Time Failure\n");
  }

_err:
  /* Restore low power config */
  (void)W6X_SetPowerMode(ps_mode);
  return ret;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get the time from SNTP server */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Net_SNTP_GetTime, time, time < timezone : UTC format : range [-12; 14] or
                       HHmm format : with HH in range [-12; +14] and mm in range [00; 59] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Net_ResolveHostAddress(int32_t argc, char **argv)
{
  ip_addr_t ipaddr = {0};
  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the IP address from the host name */
  if (W6X_Net_ResolveHostAddressByType(argv[1], &ipaddr, W6X_NET_DNS_ADDRTYPE_IPV4) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("IPv4 address: " IPSTR "\n", IP2STR((uint8_t *)&ipaddr.u_addr.ip4.addr));
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

/** @} */

#endif /* ST67_ARCH */
