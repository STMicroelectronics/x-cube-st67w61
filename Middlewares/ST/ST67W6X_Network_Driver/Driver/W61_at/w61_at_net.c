/**
  ******************************************************************************
  * @file    w61_at_net.c
  * @author  GPM Application Team
  * @brief   This file provides code for W61 Net AT module
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
#include <inttypes.h>
#include <stdio.h>
#include "w61_at_api.h"
#include "w61_at_common.h"
#include "w61_at_internal.h"
#include "w61_io.h" /* SPI_XFER_MTU_BYTES */
#include "common_parser.h" /* Common Parser functions */

#if (SYS_DBG_ENABLE_TA4 >= 1)
#include "trcRecorder.h"
#endif /* SYS_DBG_ENABLE_TA4 */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_Net_Types
  * @{
  */

/**
  * @brief  Receive data network structure
  */
typedef struct
{
  uint8_t *data;                  /*!< Pointer to the data buffer */
  uint32_t *length;               /*!< Pointer to the length of the data buffer */
} W61_Net_PullDataFromSocket_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_Net_Constants
  * @{
  */

/** Default ping interval in ms */
#define W61_NET_DEFAULT_PING_INTERVAL             1000

/** Maximum ping interval in ms */
#define W61_NET_MAX_PING_INTERNAL                 3500

/** Minimum ping interval in ms */
#define W61_NET_MIN_PING_INTERNAL                 100

/** Default Number of ping repetition */
#define W61_NET_DEFAULT_PING_REPETITION           4

/** Maximum ping repetition */
#define W61_NET_MAX_PING_REPETITION               65534

/** Default Ping packet size */
#define W61_NET_DEFAULT_PING_PACKET_SIZE          64

/** Maximum ping size */
#define W61_NET_MAX_PING_SIZE                     10000

/** Timeout for ping command */
#define W61_NET_PING_TIMEOUT                      20000

/** Timeout for DNS resolve command */
#define W61_NET_DNS_RESOLVE_TIMEOUT               20000

/** Timeout for start client command */
#define W61_NET_START_CLIENT_TIMEOUT              5000

/** Size of AT Header data in bytes: +CIPRECVDATA:xxxx,"xxx.xxx.xxx.xxx",xxxxx, */
#define W61_NET_AT_HEADER_DATA_SIZE               64

/** W61 Protocol string size */
#define W61_PROTOCOL_STRING_SIZE                  8

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_Net_Functions
  * @{
  */

/**
  * @brief  Callback function to handle Net Station get IP address responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_sta_ipaddress);

/**
  * @brief  Callback function to handle Net Station get Gateway address responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_sta_gateway);

/**
  * @brief  Callback function to handle Net Station get Netmask address responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_sta_netmask);

/**
  * @brief  AT response handler for Station link-local IPv6 address
  * @param  data  Modem command handler context (provides user_data pointer)
  * @param  len   Length of raw response buffer (unused here)
  * @param  argv  Parsed argument array (argv[0] expected: textual IPv6 address)
  * @param  argc  Number of arguments (expects >=1)
  * @retval int   0 always
  *
  */
MODEM_CMD_DECLARE(on_cmd_sta_ip6ll);

/**
  * @brief  AT response handler for Station global IPv6 address
  * @param  data  Modem command handler context
  * @param  len   Length of raw response buffer (unused)
  * @param  argv  Parsed argument array (argv[0]: textual IPv6 address)
  * @param  argc  Number of arguments (expects >=1)
  * @retval int   0 on completion
  *
  */
MODEM_CMD_DECLARE(on_cmd_sta_ip6gl);

/**
  * @brief  Callback function to handle Net Access Point get IP address responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_ap_ipaddress);

/**
  * @brief  Callback function to handle Net Access Point get Netmask address responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_ap_netmask);

/**
  * @brief  Callback function to handle Net pull available data responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings (unused)
  * @param  argc: number of argument (unused)
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DIRECT_DECLARE(on_cmd_pulldata);

/**
  * @brief  Callback function to handle Net SNTP get time responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_time);

/**
  * @brief  Callback function to handle Net socket information responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_socket_info);

/**
  * @brief  Parses Wi-Fi Network event and call related callback
  * @param  hObj: pointer to module handle
  * @param  argc: pointer to argument count
  * @param  argv: pointer to argument values
  */
static void W61_Net_AT_Event(void *hObj, uint16_t *argc, char **argv);

/**
  * @brief  Parses Wi-Fi Network data event and call related callback
  * @param  hObj: pointer to module handle
  * @param  argc: pointer to argument count
  * @param  argv: pointer to argument values
  */
static void W61_Net_Data_Event(void *hObj, uint16_t *argc, char **argv);

/**
  * @brief  Parses Net ping event and call related callback
  * @param  hObj: pointer to module handle
  * @param  argc: pointer to argument count
  * @param  argv: pointer to argument values
  */
static void W61_Net_Ping_Event(void *hObj, uint16_t *argc, char **argv);

/**
  * @brief  Converts a string to a W61_Net_Protocol_e enum value
  * @param  protocol_str: pointer to the protocol string
  * @param  Protocol: pointer to the W61_Net_Protocol_e enum value
  * @return W61_Status_t status of the operation
  */
static W61_Status_t W61_Net_StrToProtocol(char *protocol_str, W61_Net_Protocol_e *Protocol);

/**
  * @brief  Converts a W61_Net_Protocol_e enum value to a string
  * @param  Protocol: W61_Net_Protocol_e enum value
  * @param  protocol_str: pointer to the protocol string buffer
  * @return W61_Status_t status of the operation
  */
static W61_Status_t W61_Net_ProtocolToStr(W61_Net_Protocol_e Protocol, char *protocol_str);

/**
  * @brief  Parse IPv6 string into 4 host-order 32-bit words (each word holds two 16-bit hex).
  * @param  cp  Null-terminated IPv6 address string.
  * @param  dst Destination array of 4 uint32_t host-order words.
  * @retval 1 success, 0 failure.
  * @note   Based on LWIP ip6addr_aton
  */
static int W61_ip6addr_aton(const char *cp, uint32_t dst[4]);

/* Functions Definition ------------------------------------------------------*/
W61_Status_t W61_Net_Init(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  char *cmd_lst[] =
  {
#if (W61_NET_IPV6_ENABLE == 1)
    "AT+CIPV6=1\r\n",        /* Enable the IPv6  mode */
#else
    "AT+CIPV6=0\r\n",        /* Enable the IPv6  mode */
#endif /* W61_NET_IPV6_ENABLE */
    "AT+CIPMUX=1\r\n",        /* Enable the multi socket mode */
    "AT+CIPRECVMODE=1\r\n",   /* Set PASSIVE receive mode */
    "AT+CIPDINFO=1\r\n"       /* Set IPD Verbose mode */
  };

  Obj->Callbacks.Net_event_cb = W61_Net_AT_Event;
  Obj->Callbacks.Net_event_ping_cb = W61_Net_Ping_Event;
  Obj->Callbacks.Net_event_data_cb = W61_Net_Data_Event;

  for (uint8_t i = 0; i < ARRAY_SIZE(cmd_lst); i++)
  {

    ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd_lst[i], W61_NCP_TIMEOUT);

    if (ret != W61_STATUS_OK)
    {
      break;
    }
  }
  return ret;
}

W61_Status_t W61_Net_DeInit(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  Obj->Callbacks.Net_event_cb = NULL;
  Obj->Callbacks.Net_event_ping_cb = NULL;
  Obj->Callbacks.Net_event_data_cb = NULL;

  return W61_STATUS_OK;
}

W61_Status_t W61_Net_SetHostname(W61_Object_t *Obj, uint8_t Hostname[33])
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWHOSTNAME=\"%s\"\r\n", Hostname);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_GetHostname(W61_Object_t *Obj, uint8_t Hostname[33])
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);

  strncpy(cmd, "AT+CWHOSTNAME?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWHOSTNAME:", &argc, argv, W61_NET_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  if (strlen(argv[0]) < 33)
  {
    strncpy((char *)Hostname, argv[0], 32);
    Hostname[32] = '\0'; /* Ensure null termination */
  }
  else
  {
    ret = W61_STATUS_ERROR;
  }

  return ret;
}

W61_Status_t W61_Net_Station_SetIPAddress(W61_Object_t *Obj, uint8_t Ip_addr[4], uint8_t Gateway_addr[4],
                                          uint8_t Netmask_addr[4])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t Gateway_addr_def[4];
  uint8_t Netmask_addr_def[4];
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT_STR(Ip_addr, "Station IP NULL");

  if (Parser_CheckValidAddress(Ip_addr, 4) != W61_STATUS_OK)
  {
    NET_LOG_ERROR("Station IP is invalid\n");
    goto _err;
  }

  /* Get the actual IP configuration to replace fields not specified in the function */
  ret = W61_Net_Station_GetIPAddress(Obj);
  if (ret != W61_STATUS_OK)
  {
    goto _err;
  }

  if ((Gateway_addr == NULL) || (Parser_CheckValidAddress(Gateway_addr, 4) != W61_STATUS_OK))
  {
    NET_LOG_WARN("Gateway IP NULL or invalid. Previous one will be use\n");
    memcpy(Gateway_addr_def, Obj->NetCtx.Net_sta_info.Gateway_Addr, 4);
  }
  else
  {
    memcpy(Gateway_addr_def, Gateway_addr, 4);
  }

  if ((Netmask_addr == NULL) || (Parser_CheckValidAddress(Netmask_addr, 4) != W61_STATUS_OK))
  {
    NET_LOG_WARN("Netmask IP NULL or invalid. Previous one will be use\n");
    memcpy(Netmask_addr_def, Obj->NetCtx.Net_sta_info.IP_Mask, 4);
  }
  else
  {
    memcpy(Netmask_addr_def, Netmask_addr, 4);
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSTA=\"" IPSTR "\",\"" IPSTR "\",\"" IPSTR "\"\r\n",
           IP2STR(Ip_addr), IP2STR(Gateway_addr_def), IP2STR(Netmask_addr_def));
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);

  if (ret == W61_STATUS_OK)
  {
    Obj->NetCtx.DHCP_STA_IsEnabled = 0;
  }

_err:
  return ret;
}

W61_Status_t W61_Net_Station_GetIPAddress(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  struct modem *mdm = (struct modem *) &Obj->Modem;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+CIPSTA:ip:", on_cmd_sta_ipaddress, 1U, ""),
    MODEM_CMD("+CIPSTA:gateway:", on_cmd_sta_gateway, 1U, ""),
    MODEM_CMD("+CIPSTA:netmask:", on_cmd_sta_netmask, 1U, ""),
    MODEM_CMD("+CIPSTA:ip6ll:", on_cmd_sta_ip6ll, 1U, ""),
    MODEM_CMD("+CIPSTA:ip6gl:", on_cmd_sta_ip6gl, 1U, ""),
  };

  return W61_Status(modem_cmd_send(&mdm->iface,
                                   &mdm->modem_cmd_handler,
                                   handlers,
                                   ARRAY_SIZE(handlers),
                                   (const uint8_t *)"AT+CIPSTA?\r\n",
                                   mdm->sem_response,
                                   W61_NCP_TIMEOUT));
}

W61_Status_t W61_Net_AP_SetIPAddress(W61_Object_t *Obj, uint8_t Ip_addr[4], uint8_t Netmask_addr[4])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT_STR(Ip_addr, "Soft-AP IP NULL");
  W61_NULL_ASSERT_STR(Netmask_addr, "Soft-AP Netmask NULL");

  if (Parser_CheckValidAddress(Ip_addr, 4) != W61_STATUS_OK)
  {
    NET_LOG_ERROR("Soft-AP IP address invalid\n");
    return ret;
  }
  if (Ip_addr[3] != 1)
  {
    NET_LOG_WARN("Soft-AP IP address must end with .1, changing to .1\n");
    Ip_addr[3] = 1;
  }

  if (Parser_CheckValidAddress(Netmask_addr, 4) != W61_STATUS_OK)
  {
    NET_LOG_WARN("Netmask IP invalid. Default one will be use : 255.255.255.0\n");
    Netmask_addr[0] = 0xFF;
    Netmask_addr[1] = 0xFF;
    Netmask_addr[2] = 0xFF;
    Netmask_addr[3] = 0;
  }
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPAP=\"" IPSTR "\",\"" IPSTR "\",\"" IPSTR "\"\r\n",
           IP2STR(Ip_addr), IP2STR(Ip_addr), IP2STR(Netmask_addr));
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);

  if (ret == W61_STATUS_OK)
  {
    memcpy(Obj->NetCtx.Net_ap_info.IP_Addr, Ip_addr, 4);
  }
  return ret;
}

W61_Status_t W61_Net_AP_GetIPAddress(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  struct modem *mdm = (struct modem *) &Obj->Modem;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+CIPAP:ip:", on_cmd_ap_ipaddress, 1U, ""),
    MODEM_CMD("+CIPAP:netmask:", on_cmd_ap_netmask, 1U, ""),
  };

  return W61_Status(modem_cmd_send(&mdm->iface,
                                   &mdm->modem_cmd_handler,
                                   handlers,
                                   ARRAY_SIZE(handlers),
                                   (const uint8_t *)"AT+CIPAP?\r\n",
                                   mdm->sem_response,
                                   W61_NCP_TIMEOUT));
}

W61_Status_t W61_Net_GetDhcpConfig(W61_Object_t *Obj, W61_Net_DhcpType_e *State)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  uint16_t argc = 0;
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(State);

  strncpy(cmd, "AT+CWDHCP?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWDHCP:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *State = (W61_Net_DhcpType_e)atoi(argv[0]);

  return ret;
}

W61_Status_t W61_Net_SetDhcpConfig(W61_Object_t *Obj, W61_Net_DhcpType_e *State, uint32_t *Operate)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(State);
  W61_NULL_ASSERT(Operate);

  if (!((*Operate == 0) || (*Operate == 1)) || !((*State == 0) || (*State == 1) || (*State == 2) || (*State == 3)))
  {
    NET_LOG_ERROR("Incorrect parameters\n");
    return ret;
  }
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWDHCP=%" PRIu32 ",%" PRIu32 "\r\n", *Operate, (uint32_t)*State);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);

  if (ret == W61_STATUS_OK)
  {
    if ((*Operate == 0) && ((*State == 1) || (*State == 3)))
    {
      Obj->NetCtx.DHCP_STA_IsEnabled = 0;
    }
    else if ((*Operate == 1) && ((*State == 1) || (*State == 3)))
    {
      Obj->NetCtx.DHCP_STA_IsEnabled = 1;
    }
  }

  return ret;
}

W61_Status_t W61_Net_GetDhcpsConfig(W61_Object_t *Obj, uint32_t *lease_time, uint8_t start_ip[4], uint8_t end_ip[4])
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(lease_time);

  strncpy(cmd, "AT+CWDHCPS?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWDHCPS:", &argc, argv, W61_NET_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 3)
  {
    return W61_STATUS_ERROR;
  }

  *lease_time = (uint32_t)atoi(argv[0]);
  /* Parse start ip */
  Parser_StrToIP(argv[1], start_ip);

  /* Parse end ip */
  Parser_StrToIP(argv[2], end_ip);

  return ret;
}

W61_Status_t W61_Net_SetDhcpsConfig(W61_Object_t *Obj, uint32_t lease_time)
{
  uint8_t StartIP[4] = {0};
  uint8_t EndIP[4] = {0};
  uint32_t Previous_lease_time = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if (W61_Net_GetDhcpsConfig(Obj, &Previous_lease_time, StartIP, EndIP) != W61_STATUS_OK)
  {
    NET_LOG_ERROR("Get DHCP server configuration failed\n");
    return W61_STATUS_ERROR;
  }

  if (Parser_CheckValidAddress(StartIP, 4) != W61_STATUS_OK)
  {
    NET_LOG_ERROR("Start IP NULL or invalid\n");
    return W61_STATUS_ERROR;
  }

  if (Parser_CheckValidAddress(EndIP, 4) != W61_STATUS_OK)
  {
    NET_LOG_ERROR("End IP NULL or invalid\n");
    return W61_STATUS_ERROR;
  }

  if ((lease_time < 1) || (lease_time > 2880))
  {
    NET_LOG_ERROR("Lease time is invalid, range : [1;2880] minutes\n");
    return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWDHCPS=1,%" PRIu32 ",\"" IPSTR "\",\"" IPSTR "\"\r\n",
           lease_time, IP2STR(StartIP), IP2STR(EndIP));
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_Net_GetDnsAddress(W61_Object_t *Obj)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);

  strncpy(cmd, "AT+CIPDNS?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CIPDNS:", &argc, argv, W61_NET_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 3)
  {
    return W61_STATUS_ERROR;
  }

  W61_AT_RemoveStrQuotes(argv[1]);
  Parser_StrToIP(argv[1], Obj->NetCtx.Net_sta_info.DNS1);
  W61_AT_RemoveStrQuotes(argv[2]);
  Parser_StrToIP(argv[2], Obj->NetCtx.Net_sta_info.DNS2);
  W61_AT_RemoveStrQuotes(argv[3]);
  Parser_StrToIP(argv[3], Obj->NetCtx.Net_sta_info.DNS3);

  return ret;
}

W61_Status_t W61_Net_SetDnsAddress(W61_Object_t *Obj, uint8_t Dns1_addr[4], uint8_t Dns2_addr[4],
                                   uint8_t Dns3_addr[4])
{
  uint32_t pos = 0;
  uint8_t *dns_addr_table[3] = {Dns1_addr, Dns2_addr, Dns3_addr};
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  pos += snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPDNS=1");

  for (int32_t i = 0; i < 3; i++)
  {
    if ((dns_addr_table[i] == NULL) || (Parser_CheckValidAddress(dns_addr_table[i], 4) != W61_STATUS_OK))
    {
      NET_LOG_WARN("Dns% " PRIi32 " addr IP NULL or invalid. DNS% " PRIi32" IP will not be set\n",
                   i + 1, i + 1);
    }
    else
    {
      pos += snprintf(&cmd[pos], W61_CMDRSP_STRING_SIZE - pos, ",\"" IPSTR "\"", IP2STR(dns_addr_table[i]));
    }
  }

  snprintf((char *)&cmd[pos], W61_CMDRSP_STRING_SIZE - pos, "\r\n");
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_Net_Ping(W61_Object_t *Obj, char *location, uint16_t length, uint16_t count, uint16_t interval,
                          uint16_t timeout, W61_Net_PingResult_t *ping_result)
{
  W61_Status_t ret;
  int32_t ret_len;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(location);
  W61_NULL_ASSERT(ping_result);

  /*Handle default values */
  if (length == 0)
  {
    length = W61_NET_DEFAULT_PING_PACKET_SIZE;
  }
  /* If value is out of range, set to max value */
  else if (length > W61_NET_MAX_PING_SIZE)
  {
    length = W61_NET_MAX_PING_SIZE;
  }

  if (count == 0)
  {
    count = W61_NET_DEFAULT_PING_REPETITION;
  }
  /* If value is out of range, set to max value */
  else if (count > W61_NET_MAX_PING_REPETITION)
  {
    count = W61_NET_MAX_PING_REPETITION;
  }

  if ((interval < W61_NET_MIN_PING_INTERNAL) || (interval > W61_NET_MAX_PING_INTERNAL))
  {
    interval = W61_NET_DEFAULT_PING_INTERVAL;
  }

  if (timeout > W61_NET_MAX_PING_INTERNAL)
  {
    timeout = W61_NET_MAX_PING_INTERNAL;
  }

  mdm->ping_msg = ping_result;
  memset(ping_result, 0, sizeof(W61_Net_PingResult_t));
  ping_result->sem_ping = xSemaphoreCreateBinary();

  ret_len = snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+PING=\"%s\",%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
                     location, length, count, interval);
  if (W61_SdkMinVersion(Obj, 2, 0, 97) == W61_STATUS_OK)
  {
    snprintf(&cmd[ret_len], W61_CMDRSP_STRING_SIZE - ret_len, ",%" PRIu16 "\r\n", timeout);
  }
  else
  {
    snprintf(&cmd[ret_len], W61_CMDRSP_STRING_SIZE - ret_len, "\r\n");
  }
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_PING_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    NET_LOG_ERROR("Ping command failed\n");
    vSemaphoreDelete(ping_result->sem_ping);
    ping_result->sem_ping = NULL;
    mdm->ping_msg = NULL;
    return ret;
  }

  while (count--)
  {
    (void)xSemaphoreTake(ping_result->sem_ping, pdMS_TO_TICKS(W61_NET_PING_TIMEOUT + interval));
  }

  if (ping_result->response_count > 0)
  {
    ping_result->lost_count = count - ping_result->response_count;
    ping_result->average_time = ping_result->total_time / ping_result->response_count;
  }

  vSemaphoreDelete(ping_result->sem_ping);
  ping_result->sem_ping = NULL;
  mdm->ping_msg = NULL;
  return W61_STATUS_OK;
}

W61_Status_t W61_Net_ResolveHostAddress(W61_Object_t *Obj, const char *url, W61_Net_Ip_addr_t *ip_address,
                                        W61_Net_Dns_ResolveType_e *resolve_type)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(url);
  W61_NULL_ASSERT(ip_address);
  W61_NULL_ASSERT(resolve_type);

  snprintf(cmd, W61_CMD_MATCH_BUFF_SIZE, "AT+CIPDOMAIN=\"%s\",%d\r\n", url, *resolve_type);
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CIPDOMAIN:", &argc, argv, W61_NET_DNS_RESOLVE_TIMEOUT);
  if (ret == W61_STATUS_OK)
  {
    if (argc < 1)
    {
      return W61_STATUS_ERROR;
    }
    W61_AT_RemoveStrQuotes(argv[0]); /* Remove quotes from the returned address string */

    /* Parse into caller buffer according to requested family. */
    if (*resolve_type == W61_NET_DNS_IPV4_IPV6)
    {
      uint8_t tmp4[4] = {0};
      Parser_StrToIP(argv[0], tmp4);
      if (Parser_CheckValidAddress(tmp4, 4) == W61_STATUS_OK)
      {
        memcpy(&ip_address->u_addr.ip4.addr, tmp4, 4); /* Valid IPv4 */
        *resolve_type = W61_NET_DNS_IPV4;
      }
      else
      {
#if (W61_NET_IPV6_ENABLE == 1)
        uint32_t words[4] = {0};
        if (W61_ip6addr_aton(argv[0], words) != 1)
        {
          return W61_STATUS_ERROR; /* Neither valid IPv4 nor IPv6 */
        }
        memcpy(ip_address->u_addr.ip6.addr, words, sizeof(words));
        *resolve_type = W61_NET_DNS_IPV6;
#else
        return W61_STATUS_ERROR; /* Invalid IPv4  */
#endif /* W61_NET_IPV6_ENABLE */
      }
    }
    else if (*resolve_type == W61_NET_DNS_IPV4)
    {
      uint8_t tmp4[4] = {0};
      Parser_StrToIP(argv[0], tmp4);
      if (Parser_CheckValidAddress(tmp4, 4) != W61_STATUS_OK)
      {
        return W61_STATUS_ERROR; /* Invalid IPv4 */
      }
      memcpy(&ip_address->u_addr.ip4.addr, tmp4, 4);
    }
#if (W61_NET_IPV6_ENABLE == 1)
    else /* resolve_type == W61_NET_DNS_IPV6 */
    {
      uint32_t words[4] = {0};
      if (W61_ip6addr_aton(argv[0], words) != 1)
      {
        return W61_STATUS_ERROR;
      }
      memcpy(ip_address->u_addr.ip6.addr, words, sizeof(words));
    }
#endif /* W61_NET_IPV6_ENABLE */
  }
  return ret;
}

W61_Status_t W61_Net_StartClientConnection(W61_Object_t *Obj, W61_Net_Connection_t *conn)
{
  char protocol_str[W61_PROTOCOL_STRING_SIZE];
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(conn);

  if (W61_Net_ProtocolToStr(conn->Protocol, protocol_str) != W61_STATUS_OK)
  {
    NET_LOG_ERROR("Invalid protocol type\n");
    return W61_STATUS_ERROR;
  }

  if ((conn->Protocol != W61_NET_TCP_CONNECTION) && (conn->Protocol != W61_NET_TCP_SSL_CONNECTION)
#if (W61_NET_IPV6_ENABLE == 1)
      && (conn->Protocol != W61_NET_TCPV6_CONNECTION) && (conn->Protocol != W61_NET_TCPV6_SSL_CONNECTION)
#endif /* W61_NET_IPV6_ENABLE */
     )
  {
    NET_LOG_ERROR("Only TCP and SSL protocols are supported for client connections\n");
    return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+CIPSTART=%" PRIu16 ",\"%s\",\"%s\",%" PRIu16 ",%" PRIu16 ",,%" PRIu16 "\r\n",
           conn->Number, protocol_str, conn->RemoteIP, conn->RemotePort, conn->KeepAlive,
           W61_NET_START_CLIENT_TIMEOUT);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_START_CLIENT_TIMEOUT + W61_NET_TIMEOUT);
}

W61_Status_t W61_Net_StopClientConnection(W61_Object_t *Obj, W61_Net_Connection_t *conn)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(conn);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPCLOSE=%" PRIu16 "\r\n", conn->Number);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_Net_StartServer(W61_Object_t *Obj, uint32_t Port, W61_Net_Protocol_e protocol, uint8_t ca_enable,
                                 uint32_t keepalive)
{
  char protocol_str[W61_PROTOCOL_STRING_SIZE];
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if (W61_Net_ProtocolToStr(protocol, protocol_str) != W61_STATUS_OK)
  {
    NET_LOG_ERROR("Invalid protocol type\n");
    return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSERVER=1,%" PRIu32 ",\"%s\",%" PRIu16 ",%" PRIu32 "\r\n",
           Port, protocol_str, ca_enable, keepalive);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_Net_StopServer(W61_Object_t *Obj, uint8_t close_connections)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSERVER=0,%" PRIu16 "\r\n", close_connections);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_Net_SendData(W61_Object_t *Obj, uint8_t Socket, uint8_t *pdata, uint32_t Reqlen,
                              uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (Reqlen > SPI_XFER_MTU_BYTES)
  {
    Reqlen = SPI_XFER_MTU_BYTES;
  }

  *SentLen = Reqlen;

  /* W61_AT_Common_SetExecute timeout should let the time to NCP to return SEND:ERROR message */
  if (Timeout < W61_NET_TIMEOUT)
  {
    Timeout = W61_NET_TIMEOUT;
  }
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSEND=%" PRIu16 ",%" PRIu32 "\r\n", Socket, Reqlen);
  ret = W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, pdata, Reqlen, Timeout, true);

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }

  return ret;
}

W61_Status_t W61_Net_SendData_Non_Connected(W61_Object_t *Obj, uint8_t Socket, char *IpAddress, uint32_t Port,
                                            uint8_t *pdata, uint32_t Reqlen, uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(IpAddress);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (Reqlen > SPI_XFER_MTU_BYTES)
  {
    Reqlen = SPI_XFER_MTU_BYTES;
  }

  *SentLen = Reqlen;

  /* W61_AT_Common_SetExecute timeout should let the time to NCP to return SEND:ERROR message */
  if (Timeout < W61_NET_TIMEOUT)
  {
    Timeout = W61_NET_TIMEOUT;
  }
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSEND=%" PRIu16 ",%" PRIu32 ",\"%s\",%" PRIu32 "\r\n",
           Socket, Reqlen, IpAddress, Port);
  ret = W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, pdata, Reqlen, Timeout, true);

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }

  return ret;
}

W61_Status_t W61_Net_SetReceiveBufferLen(W61_Object_t *Obj, uint8_t Socket, uint32_t BufLen)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPRECVBUF=%" PRIu16 ",%" PRIu32 "\r\n", Socket, BufLen);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_GetReceiveBufferLen(W61_Object_t *Obj, uint8_t Socket, uint32_t *BufLen)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMD_MATCH_BUFF_SIZE, "AT+CIPRECVBUF=%" PRIu16 "?\r\n", Socket);
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CIPRECVBUF:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *BufLen = (uint32_t)atoi(argv[0]);

  return ret;
}

W61_Status_t W61_Net_PullDataFromSocket(W61_Object_t *Obj, uint8_t Socket, uint32_t Reqlen, uint8_t *pData,
                                        uint32_t *Receivedlen, uint32_t Timeout)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pData);
  W61_NULL_ASSERT(Receivedlen);
  W61_Status_t ret;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;
  W61_Net_PullDataFromSocket_t pull_data =
  {
    .data = pData,
    .length = Receivedlen,
  };
  struct modem_cmd handlers[] =
  {
    MODEM_CMD_DIRECT("+CIPRECVDATA:", on_cmd_pulldata),
  };

  if (Reqlen > W61_MAX_SPI_XFER - 64)
  {
    Reqlen = W61_MAX_SPI_XFER - 64;
  }
  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  *Receivedlen = 0;
  mdm->rx_data = &pull_data;

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPRECVDATA=%" PRIu16 ",%" PRIu32 "\r\n", Socket, Reqlen);
  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)cmd,
                                      mdm->sem_response,
                                      W61_NET_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_Net_SetServerMaxConnections(W61_Object_t *Obj, uint8_t MaxConnections)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSERVERMAXCONN=%" PRIu16 "\r\n", MaxConnections);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_SNTP_GetConfiguration(W61_Object_t *Obj, uint8_t *Enable, int16_t *Timezone,
                                           uint8_t SntpServer1[64], uint8_t SntpServer2[64], uint8_t SntpServer3[64])
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Enable);
  W61_NULL_ASSERT(Timezone);

  strncpy(cmd, "AT+CIPSNTPCFG?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CIPSNTPCFG:", &argc, argv, W61_NET_TIMEOUT);
  if ((ret == W61_STATUS_OK) && (argc > 2))
  {
    switch (argc)
    {
      case 6:
        if (SntpServer3 != NULL)
        {
          W61_AT_RemoveStrQuotes(argv[4]);
          strncpy((char *)SntpServer3, argv[4], 63);
        }
      case 5:
        if (SntpServer2 != NULL)
        {
          W61_AT_RemoveStrQuotes(argv[3]);
          strncpy((char *)SntpServer2, argv[3], 63);
        }
      case 4:
        if (SntpServer1 != NULL)
        {
          W61_AT_RemoveStrQuotes(argv[2]);
          strncpy((char *)SntpServer1, argv[2], 63);
        }
        *Timezone = (int16_t)atoi(argv[1]);
        *Enable = (uint8_t)atoi(argv[0]);
        break;
      default:
        ret = W61_STATUS_ERROR;
    }
  }

  return ret;
}

W61_Status_t W61_Net_SNTP_SetConfiguration(W61_Object_t *Obj, uint8_t Enable, int16_t Timezone, uint8_t *SntpServer1,
                                           uint8_t *SntpServer2, uint8_t *SntpServer3)
{
  int32_t len;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if ((SntpServer1 == NULL) && (SntpServer2 == NULL) && (SntpServer3 == NULL))
  {
    NET_LOG_ERROR("SNTP servers URL missing\n");
    return W61_STATUS_ERROR;
  }

  len = snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSNTPCFG=%" PRIu16 ",%" PRIi16, Enable, Timezone);

  if (SntpServer1 != NULL)
  {
    len += snprintf(&cmd[len], W61_CMDRSP_STRING_SIZE - len, ",\"%s\"", SntpServer1);
  }

  if (SntpServer2 != NULL)
  {
    len += snprintf(&cmd[len], W61_CMDRSP_STRING_SIZE - len, ",\"%s\"", SntpServer2);
  }

  if (SntpServer3 != NULL)
  {
    len += snprintf(&cmd[len], W61_CMDRSP_STRING_SIZE - len, ",\"%s\"", SntpServer3);
  }

  snprintf(&cmd[len], W61_CMDRSP_STRING_SIZE - len, "\r\n");
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_SNTP_GetTime(W61_Object_t *Obj, W61_Net_Time_t *Time)
{
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Time);
  W61_Status_t ret;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD_ARGS_MAX("+CIPSNTPTIME:", on_cmd_time, 7, 8, " :"),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = Time;

  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+CIPSNTPTIME?\r\n",
                                      mdm->sem_response,
                                      W61_NET_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_Net_SNTP_GetInterval(W61_Object_t *Obj, uint16_t *Interval)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Interval);

  strncpy(cmd, "AT+CIPSNTPINTV?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CIPSNTPINTV:", &argc, argv, W61_NET_TIMEOUT);
  if (ret == W61_STATUS_OK)
  {
    *Interval = (uint16_t)atoi(argv[0]);
  }

  return ret;
}

W61_Status_t W61_Net_SNTP_SetInterval(W61_Object_t *Obj, uint16_t Interval)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSNTPINTV=%" PRIu16 "\r\n", Interval);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_SetTCPOpt(W61_Object_t *Obj, uint8_t Socket, int16_t Linger, uint16_t TcpNoDelay,
                               uint16_t SoSndTimeout, uint16_t KeepAlive)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPTCPOPT=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
           Socket, Linger, TcpNoDelay, SoSndTimeout, KeepAlive);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_GetSocketInformation(W61_Object_t *Obj, uint8_t Socket, W61_Net_Connection_t *conn)
{
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(conn);

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+CIPSTATUS:", on_cmd_socket_info, 6U, ","),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  memset(conn, 0, sizeof(W61_Net_Connection_t));
  conn->Number = Socket;
  mdm->rx_data = conn;

  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+CIPSTATE?\r\n",
                                      mdm->sem_response,
                                      W61_NET_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_Net_SSL_SetConfiguration(W61_Object_t *Obj, uint8_t Socket, uint8_t AuthMode, uint8_t *Certificate,
                                          uint8_t *PrivateKey, uint8_t *CaCertificate)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  if (!Certificate)
  {
    Certificate = (uint8_t *)"";
  }
  if (!PrivateKey)
  {
    PrivateKey = (uint8_t *)"";
  }
  if (!CaCertificate)
  {
    CaCertificate = (uint8_t *)"";
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSSLCCONF=%" PRIu16 ",%" PRIu16 ",\"%s\",\"%s\",\"%s\"\r\n",
           Socket, AuthMode, Certificate, PrivateKey, CaCertificate);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_SSL_SetServerName(W61_Object_t *Obj, uint8_t Socket, uint8_t *SslSni)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(SslSni);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSSLCSNI=%" PRIu16 ",\"%s\"\r\n", Socket, SslSni);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_Net_SSL_SetALPN(W61_Object_t *Obj, uint8_t Socket, uint8_t *Alpn1,
                                 uint8_t *Alpn2, uint8_t *Alpn3)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t alpn_count = 0;
  char alpns[50] = {0};
  int32_t size = 0;
  int32_t offset = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if ((Alpn1 != NULL) && (strlen((char *)Alpn1) > 0))
  {
    size = snprintf(&alpns[offset], sizeof(alpns) - offset, ",\"%s\"", Alpn1);
    if (size > 0)
    {
      offset += size;
      alpn_count++;
    }
    else
    {
      goto _err;
    }
  }

  if ((Alpn2 != NULL) && (strlen((char *)Alpn2) > 0))
  {
    size = snprintf(&alpns[offset], sizeof(alpns) - offset, ",\"%s\"", Alpn2);
    if (size > 0)
    {
      offset += size;
      alpn_count++;
    }
    else
    {
      goto _err;
    }
  }

  if ((Alpn3 != NULL) && (strlen((char *)Alpn3) > 0))
  {
    size = snprintf(&alpns[offset], sizeof(alpns) - offset, ",\"%s\"", Alpn3);
    if (size > 0)
    {
      offset += size;
      alpn_count++;
    }
    else
    {
      goto _err;
    }
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSSLCALPN=%" PRIu16 ",%" PRIu16 "%s\r\n", Socket, alpn_count, alpns);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);

_err:
  return ret;
}

W61_Status_t W61_Net_SSL_SetPSK(W61_Object_t *Obj, uint8_t Socket, uint8_t *Psk, uint8_t *Hint)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Psk);
  W61_NULL_ASSERT(Hint);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CIPSSLCPSK=%" PRIu16 ",\"%s\",\"%s\"\r\n", Socket, Psk, Hint);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

/* Public IPv6 aton wrapper exposed to upper layer (W6X). */
int32_t W61_Net_Inet6_aton(const char *src, uint32_t dst_words[4])
{
  return W61_ip6addr_aton(src, dst_words);
}

/* Private Functions Definition ----------------------------------------------*/
MODEM_CMD_DEFINE(on_cmd_sta_ipaddress)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc >= 1)
  {
    W61_AT_RemoveStrQuotes((char *)argv[0]); /* Remove quotes from the string */
    Parser_StrToIP((char *)argv[0], Obj->NetCtx.Net_sta_info.IP_Addr);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_sta_gateway)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc >= 1)
  {
    W61_AT_RemoveStrQuotes((char *)argv[0]); /* Remove quotes from the string */
    Parser_StrToIP((char *)argv[0], Obj->NetCtx.Net_sta_info.Gateway_Addr);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_sta_netmask)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc >= 1)
  {
    W61_AT_RemoveStrQuotes((char *)argv[0]); /* Remove quotes from the string */
    Parser_StrToIP((char *)argv[0], Obj->NetCtx.Net_sta_info.IP_Mask);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_sta_ip6ll)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);
  if (argc >= 1)
  {
    W61_AT_RemoveStrQuotes((char *)argv[0]);
    if (W61_ip6addr_aton((char *)argv[0], Obj->NetCtx.Net_sta_info.IP6_LinkLocal) != 1)
    {
      NET_LOG_WARN("Invalid IPv6 link-local format received: %s\n", argv[0]);
    }
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_sta_ip6gl)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);
  if (argc >= 1)
  {
    W61_AT_RemoveStrQuotes((char *)argv[0]);
    if (W61_ip6addr_aton((char *)argv[0], Obj->NetCtx.Net_sta_info.IP6_Global) != 1)
    {
      NET_LOG_WARN("Invalid IPv6 global format received: %s\n", argv[0]);
    }
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_ap_ipaddress)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc >= 1)
  {
    W61_AT_RemoveStrQuotes((char *)argv[0]); /* Remove quotes from the string */
    Parser_StrToIP((char *)argv[0], Obj->NetCtx.Net_ap_info.IP_Addr);
  }
  return 0;
}

MODEM_CMD_DEFINE(on_cmd_ap_netmask)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);

  if (argc >= 1)
  {
    W61_AT_RemoveStrQuotes((char *)argv[0]); /* Remove quotes from the string */
    Parser_StrToIP((char *)argv[0], Obj->NetCtx.Net_ap_info.IP_Mask);
  }
  return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_pulldata)
{
  struct modem *mdm = (struct modem *) data->user_data;

  uint8_t *ptr;
  uint32_t offset;
  uint8_t *endptr;
  uint16_t rx_data_len;

  if (data->rx_buf == NULL)
  {
    return -EINVAL;
  }
  ptr = data->rx_buf + len;
  data->rx_buf[data->rx_buf_len] = 0;

  rx_data_len = strtol((char *)ptr, (char **)&endptr, 10);
  if ((endptr == ptr) || (*endptr != ','))
  {
    NET_LOG_ERROR("Invalid +CIPRECVDATA response format\n");
    return -EINVAL;
  }
  offset = endptr - data->rx_buf + 1;

  if (data->rx_buf_len >= (offset + rx_data_len))
  {
    W61_Net_PullDataFromSocket_t *pull_data = (W61_Net_PullDataFromSocket_t *)mdm->rx_data;
    memcpy(pull_data->data, endptr + 1, rx_data_len);
    *pull_data->length = rx_data_len;
    return offset + rx_data_len;
  }
  else
  {
    return -EAGAIN;
  }
}

MODEM_CMD_DEFINE(on_cmd_time)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Net_Time_t *Time = (W61_Net_Time_t *)mdm->rx_data;
  uint32_t argc_count = 0;
  static const char *WeekDayString[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
  static const char *MonthString[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                                       "Aug", "Sep", "Oct", "Nov", "Dec"
                                     };

  if (argc >= 7)
  {
    /* Convert the week day string into value */
    char day_of_week[4] = {0};
    strncpy(day_of_week, (char *)argv[argc_count++], sizeof(day_of_week) - 1);
    for (uint32_t weekday = 0; weekday < 7; weekday++)
    {
      if (strcmp(day_of_week, WeekDayString[weekday]) == 0)
      {
        Time->day_of_week = weekday + 1; /* 1 = Monday, 7 = Sunday */
        break;
      }
    }

    /* Convert the month string into value */
    char mon[4] = {0};
    strncpy(mon, (char *)argv[argc_count++], sizeof(mon) - 1);
    for (uint32_t month = 0; month < 12; month++)
    {
      if (strcmp(mon, MonthString[month]) == 0)
      {
        Time->month = month + 1; /* 1 = January, 12 = December */
        break;
      }
    }
    if (argv[argc_count][0] == '\0')
    {
      argc_count++;
    }

    Time->day = atoi((char *)argv[argc_count++]);
    Time->hours = atoi((char *)argv[argc_count++]);
    Time->minutes = atoi((char *)argv[argc_count++]);
    Time->seconds = atoi((char *)argv[argc_count++]);
    Time->year = atoi((char *)argv[argc_count]);
    snprintf(Time->raw, sizeof(Time->raw), "%s %s %02" PRIu32 " %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32 " %04" PRIu32,
             argv[0], argv[1], Time->day, Time->hours, Time->minutes, Time->seconds, Time->year);
  }

  return 0;
}

MODEM_CMD_DEFINE(on_cmd_socket_info)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Net_Connection_t *conn = (W61_Net_Connection_t *)mdm->rx_data;

  if ((argc >= 6) && (conn->Number == atoi((char *)argv[0])))
  {
    W61_AT_RemoveStrQuotes((char *)argv[1]); /* Remove quotes from the string */
    W61_Net_StrToProtocol((char *)argv[1], &conn->Protocol);
    W61_AT_RemoveStrQuotes((char *)argv[2]); /* Remove quotes from the string */
    strncpy(conn->RemoteIP, (char *)argv[2], sizeof(conn->RemoteIP) - 1);
    conn->RemotePort = (uint32_t)atoi((char *)argv[3]);
    conn->LocalPort = (uint32_t)atoi((char *)argv[4]);
    conn->TeType = (uint8_t)atoi((char *)argv[5]);
  }

  return 0;
}

static void W61_Net_AT_Event(void *hObj, uint16_t *argc, char **argv)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  W61_Net_CbParamData_t cb_param_net_data;

  if ((Obj == NULL) || (Obj->ulcbs.UL_net_cb == NULL) || (*argc < 2))
  {
    return;
  }

  /* Get the socket ID (Multi socket mode always enabled) */
  cb_param_net_data.socket_id = (uint32_t)atoi(argv[0]);
  if (cb_param_net_data.socket_id > W61_NET_MAX_CONNECTIONS - 1)
  {
    return;
  }

  if (strcmp(argv[1], "CONNECTED") == 0)
  {
    Obj->ulcbs.UL_net_cb(W61_NET_EVT_SOCK_CONNECTED_ID, &cb_param_net_data);
    return;
  }

  if (strcmp(argv[1], "DISCONNECTED") == 0)
  {
    Obj->ulcbs.UL_net_cb(W61_NET_EVT_SOCK_DISCONNECTED_ID, &cb_param_net_data);
    return;
  }
}

static void W61_Net_Ping_Event(void *hObj, uint16_t *argc, char **argv)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  W61_Net_PingResult_t *ping_result = (W61_Net_PingResult_t *)Obj->Modem.ping_msg;

  if ((ping_result == NULL) || (ping_result->sem_ping == NULL))
  {
    NET_LOG_ERROR("Ping result or semaphore is NULL\n");
    return;
  }

  if (*argc >= 1)
  {
    if (strncmp((char *)argv[0], "TIMEOUT", sizeof("TIMEOUT") - 1) == 0)
    {
      NET_LOG_INFO("Ping timeout\n");
    }
    else
    {

      uint32_t ping = atoi((char *)argv[0]);
      if (ping > 0)
      {
        ping_result->response_count++;
        ping_result->total_time += ping;
        NET_LOG_INFO("Ping: %" PRIu32 "ms\n", ping);
      }
    }
  }
  (void)xSemaphoreGive(ping_result->sem_ping);

  return;
}

static void W61_Net_Data_Event(void *hObj, uint16_t *argc, char **argv)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  W61_Net_CbParamData_t cb_param_net_data;

  if ((Obj == NULL) || (Obj->ulcbs.UL_net_cb == NULL) || (*argc < 4))
  {
    return;
  }

  /* Socket ID */
  cb_param_net_data.socket_id = (uint32_t)atoi((char *)argv[0]);
  if (cb_param_net_data.socket_id > W61_NET_MAX_CONNECTIONS - 1)
  {
    return;
  }

  /* Data length */
  cb_param_net_data.available_data_length = (uint32_t)atoi((char *)argv[1]);

  /* Remote IP */
  W61_AT_RemoveStrQuotes((char *)argv[2]);
  strncpy(cb_param_net_data.remote_ip, (char *)argv[2], sizeof(cb_param_net_data.remote_ip) - 1);
  cb_param_net_data.remote_ip[sizeof(cb_param_net_data.remote_ip) - 1] = '\0'; /* Ensure null termination */

  /* Remote port */
  cb_param_net_data.remote_port = (uint16_t)atoi((char *)argv[3]);

  Obj->ulcbs.UL_net_cb(W61_NET_EVT_SOCK_DATA_ID, &cb_param_net_data);

  return;
}

static W61_Status_t W61_Net_StrToProtocol(char *protocol_str, W61_Net_Protocol_e *Protocol)
{
  if (strcmp(protocol_str, "TCP") == 0)
  {
    *Protocol = W61_NET_TCP_CONNECTION;
  }
  else if (strcmp(protocol_str, "UDP") == 0)
  {
    *Protocol = W61_NET_UDP_CONNECTION;
  }
  else if (strcmp(protocol_str, "SSL") == 0)
  {
    *Protocol = W61_NET_TCP_SSL_CONNECTION;
  }
  else
  {
    *Protocol = W61_NET_UNKNOWN_CONNECTION;
    return W61_STATUS_ERROR; /* Unknown protocol */
  }
  return W61_STATUS_OK;
}

static W61_Status_t W61_Net_ProtocolToStr(W61_Net_Protocol_e Protocol, char *protocol_str)
{
  if (Protocol == W61_NET_TCP_CONNECTION)
  {
    strncpy(protocol_str, "TCP", W61_PROTOCOL_STRING_SIZE - 1);
  }
  else if (Protocol == W61_NET_UDP_CONNECTION)
  {
    strncpy(protocol_str, "UDP", W61_PROTOCOL_STRING_SIZE - 1);
  }
  else if (Protocol == W61_NET_TCP_SSL_CONNECTION)
  {
    strncpy(protocol_str, "SSL", W61_PROTOCOL_STRING_SIZE - 1);
  }
#if (W61_NET_IPV6_ENABLE == 1)
  else if (Protocol == W61_NET_TCPV6_CONNECTION)
  {
    strncpy(protocol_str, "TCPv6", W61_PROTOCOL_STRING_SIZE - 1);
  }
  else if (Protocol == W61_NET_UDPV6_CONNECTION)
  {
    strncpy(protocol_str, "UDPv6", W61_PROTOCOL_STRING_SIZE - 1);
  }
  else if (Protocol == W61_NET_TCPV6_SSL_CONNECTION)
  {
    strncpy(protocol_str, "SSLv6", W61_PROTOCOL_STRING_SIZE - 1);
  }
#endif /* W61_NET_IPV6_ENABLE */
  else
  {
    strncpy(protocol_str, "UNDEF", W61_PROTOCOL_STRING_SIZE - 1);
    return W61_STATUS_ERROR; /* Unknown protocol */
  }
  return W61_STATUS_OK;
}

static int W61_ip6addr_aton(const char *cp, uint32_t dst[4])
{
  uint32_t zero_blocks = 8;
  const char *s;
  uint32_t current_block_index = 0;
  uint32_t current_block_value = 0;
  uint16_t hextets[8] = {0};

  if (!cp || !dst)
  {
    return 0;
  }

  for (s = cp; *s != '\0'; s++)
  {
    if (*s == ':')
    {
      zero_blocks--;
    }
    else if (!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F')))
    {
      break;
    }
  }

  for (s = cp; *s != '\0'; s++)
  {
    if (*s == ':')
    {
      if (current_block_index > 7)
      {
        return 0;
      }
      hextets[current_block_index++] = (uint16_t)current_block_value;
      current_block_value = 0;
      if (s[1] == ':')
      {
        if (s[2] == ':')
        {
          return 0;
        }
        s++;
        while (zero_blocks > 0)
        {
          zero_blocks--;
          if (current_block_index > 7)
          {
            return 0;
          }
          hextets[current_block_index++] = 0;
        }
      }
    }
    else if ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))
    {
      current_block_value = (current_block_value << 4) +
                            ((*s >= '0' && *s <= '9') ? (uint32_t)(*s - '0') :
                             (uint32_t)(10 + ((*s < 'a') ? *s - 'A' : *s - 'a')));
    }
    else
    {
      break;
    }
  }

  if (current_block_index <= 7)
  {
    hextets[current_block_index++] = (uint16_t)current_block_value;
  }
  if (current_block_index != 8)
  {
    return 0;
  }

  for (int i = 0; i < 4; ++i)
  {
    uint32_t net_word = ((uint32_t)hextets[2 * i] << 16) | (uint32_t)hextets[2 * i + 1];
    dst[i] = net_word; /* host-order word: high 16 bits first hextet, low 16 bits second hextet */
  }
  return 1;
}

/** @} */
