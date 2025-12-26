/**
  ******************************************************************************
  * @file    w61_at_wifi.c
  * @author  GPM Application Team
  * @brief   This file provides code for W61 WiFi AT module
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
#include "common_parser.h" /* Common Parser functions */
#include "event_groups.h"

#if (SYS_DBG_ENABLE_TA4 >= 1)
#include "trcRecorder.h"
#endif /* SYS_DBG_ENABLE_TA4 */

/* Global variables ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_WiFi_Constants
  * @{
  */

#define W61_WIFI_RECONNECTION_INTERVAL            7200                    /*!< Reconnection interval in seconds */
#define W61_WIFI_RECONNECTION_ATTEMPTS            1000                    /*!< Reconnection attempts */
#define W61_WIFI_CONNECT_TIMEOUT                  3000                    /*!< Connect command status timeout */

#define W61_WIFI_COUNTRY_CODE_MAX                 5                       /*!< Maximum number of country codes */
#define W61_WIFI_MAC_LENGTH                       17                      /*!< Length of a complete MAC address string */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W61_AT_WiFi_Variables ST67W61 AT Driver Wi-Fi Variables
  * @ingroup  ST67W61_AT_WiFi
  * @{
  */

/** @brief  Scan options structure */
W61_WiFi_Scan_Opts_t ScanOptions = {"\0", "\0", W61_WIFI_SCAN_ACTIVE, 0, 50};

/** @brief  Array of Alpha-2 country codes */
static const char *const Country_code_str[] = {"CN", "JP", "US", "EU", "00"};

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_WiFi_Functions
  * @{
  */

/**
  * @brief  Callback function to handle Wi-Fi AP get list of stations responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_ap_liststa);

/**
  * @brief  Callback function to handle Wi-Fi TWT get status responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_twt_status);

/**
  * @brief  Parses WiFi event and call related callback
  * @param  hObj: pointer to module handle
  * @param  argc: pointer to argument count
  * @param  argv: pointer to argument values
  */
static void W61_WiFi_AT_Event(void *hObj, uint16_t *argc, char **argv);

/**
  * @brief  Callback function to handle scan results
  * @param  hObj: pointer to module handle
  * @param  argc: pointer to argument count
  * @param  argv: pointer to argument values
  */
static void W61_WiFi_scan_result(void *hObj, uint16_t *argc, char **argv);

/* Functions Definition ------------------------------------------------------*/
W61_Status_t W61_WiFi_Init(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  Obj->WifiCtx.ScanResults.AP = NULL;
  Obj->WifiCtx.ScanResults.Count = 0;

  Obj->Callbacks.WiFi_event_cb = W61_WiFi_AT_Event;
  Obj->Callbacks.WiFi_event_scan_cb = W61_WiFi_scan_result;

  return W61_STATUS_OK;
}

W61_Status_t W61_WiFi_DeInit(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  if (Obj->WifiCtx.ScanResults.AP != NULL)
  {
    vPortFree(Obj->WifiCtx.ScanResults.AP);
    Obj->WifiCtx.ScanResults.AP = NULL;
  }
  Obj->WifiCtx.ScanResults.Count = 0;

  Obj->Callbacks.WiFi_event_cb = NULL;
  Obj->Callbacks.WiFi_event_scan_cb = NULL;

  return W61_STATUS_OK;
}

W61_Status_t W61_WiFi_SetAutoConnect(W61_Object_t *Obj, uint32_t OnOff)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if (!((OnOff == 0) || (OnOff == 1)))
  {
    WIFI_LOG_ERROR("Invalid value passed to Autoconnect function\n");
    return ret;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWAUTOCONN=%" PRIu32 "\r\n", OnOff);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);

  if (ret == W61_STATUS_OK)
  {
    Obj->WifiCtx.STASettings.AutoConnect = OnOff;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetAutoConnect(W61_Object_t *Obj, uint32_t *OnOff)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(OnOff);

  strncpy(cmd, "AT+CWAUTOCONN?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWAUTOCONN:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *OnOff = atoi(argv[0]);

  return ret;
}

W61_Status_t W61_WiFi_Station_Start(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+CWMODE=1,0\r\n", W61_WIFI_TIMEOUT);
}

W61_Status_t W61_WiFi_SetScanOpts(W61_Object_t *Obj, W61_WiFi_Scan_Opts_t *ScanOpts)
{
  uint32_t max_cnt = 50;
  W61_WiFi_scan_type_e type = W61_WIFI_SCAN_ACTIVE;
  uint8_t SSID[W61_WIFI_MAX_SSID_SIZE + 1] = {'\0'};
  uint8_t MAC[6] = {'\0'};
  uint8_t channel = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ScanOpts);

  if ((ScanOpts->MaxCnt > 0) && (ScanOpts->MaxCnt < 50))
  {
    max_cnt = ScanOpts->MaxCnt;
  }
  if ((ScanOpts->scan_type == 0) || (ScanOpts->scan_type == 1))
  {
    ScanOptions.scan_type = ScanOpts->scan_type;
  }
  else
  {
    ScanOptions.scan_type = type;
  }
  if (ScanOpts->SSID[0] != '\0')
  {
    memcpy(ScanOptions.SSID, ScanOpts->SSID, W61_WIFI_MAX_SSID_SIZE + 1);
  }
  else
  {
    memcpy(ScanOptions.SSID, SSID, W61_WIFI_MAX_SSID_SIZE + 1);
  }
  if (ScanOpts->MAC[0] != '\0')
  {
    memcpy(ScanOptions.MAC, ScanOpts->MAC, 6);
  }
  else
  {
    memcpy(ScanOptions.MAC, MAC, 6);
  }
  if ((ScanOpts->Channel > 0) && (ScanOpts->Channel < 13))
  {
    ScanOptions.Channel = ScanOpts->Channel;
  }
  else
  {
    ScanOptions.Channel = channel;
  }

  /* The config of the scan options must be made before every scan
   * The second parameter is a bit mask to select the fields to display,
   * with the bytes 5,6 and 8 unused */
  snprintf((char *)cmd, W61_CMDRSP_STRING_SIZE, "AT+CWLAPOPT=1,1695,-100,255,%" PRIu32 "\r\n", max_cnt);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_WiFi_Scan(W61_Object_t *Obj)
{
  char MAC[20 + 1];
  char SSID[W61_WIFI_MAX_SSID_SIZE + 3] = {'\0'}; /* Size + 2 if contains double-quote characters */
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if (Obj->WifiCtx.ScanResults.AP == NULL)
  {
    Obj->WifiCtx.ScanResults.AP = pvPortMalloc(sizeof(W61_WiFi_AP_t) * W61_WIFI_MAX_DETECTED_AP);
    if ((Obj->WifiCtx.ScanResults.AP == NULL) && (W61_WIFI_MAX_DETECTED_AP != 0))
    {
      WIFI_LOG_ERROR("Error: Unable to allocate memory for scan results\n");
      return W61_STATUS_ERROR;
    }
  }

  if (Obj->WifiCtx.ScanResults.Count > 0)
  {
    memset(Obj->WifiCtx.ScanResults.AP, 0, sizeof(W61_WiFi_AP_t)* W61_WIFI_MAX_DETECTED_AP);
    Obj->WifiCtx.ScanResults.Count = 0;
  }
  Obj->WifiCtx.ScanResults.More = 0;

  snprintf(MAC, 20, "\"" MACSTR "\"", MAC2STR(ScanOptions.MAC));
  if (strcmp((char *)ScanOptions.SSID, "\0") == 0)
  {
    snprintf(SSID, W61_WIFI_MAX_SSID_SIZE + 3, "%s", ScanOptions.SSID);
  }
  else
  {
    snprintf(SSID, W61_WIFI_MAX_SSID_SIZE + 3, "\"%s\"", ScanOptions.SSID);
  }

  snprintf((char *)cmd, W61_CMDRSP_STRING_SIZE, "AT+CWLAP=%" PRIu32 ",%s,%s,%" PRIu16 "\r\n",
           (uint32_t)ScanOptions.scan_type, SSID,
           strcmp(MAC, "\"00:00:00:00:00:00\"") == 0 ? "" : MAC, ScanOptions.Channel);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_WiFi_SetReconnectionOpts(W61_Object_t *Obj, W61_WiFi_Connect_Opts_t *ConnectOpts)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnectOpts);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWRECONNCFG=%" PRIu16 ",%" PRIu16 "\r\n",
           ConnectOpts->Reconnection_interval, ConnectOpts->Reconnection_nb_attempts);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_WiFi_Connect(W61_Object_t *Obj, W61_WiFi_Connect_Opts_t *ConnectOpts)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t pos = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnectOpts);

  if (ConnectOpts->WPS)
  {
    return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+WPS=1\r\n", W61_WIFI_TIMEOUT);
  }

  if (ConnectOpts->SSID[0] == '\0')
  {
    WIFI_LOG_ERROR("SSID cannot be NULL\n");
    return ret;
  }

  /* The config of the connect options must be made before every connect */
  if (ConnectOpts->Reconnection_interval > W61_WIFI_RECONNECTION_INTERVAL)
  {
    WIFI_LOG_ERROR("Reconnection interval must be between [0;7200], parameter set to default value : 0\n");
    ConnectOpts->Reconnection_interval = 0;
  }
  if (ConnectOpts->Reconnection_nb_attempts > W61_WIFI_RECONNECTION_ATTEMPTS)
  {
    WIFI_LOG_ERROR("Number of reconnection attempts must be between [0;1000], parameter set to default value : 0\n");
    ConnectOpts->Reconnection_nb_attempts = 0;
  }

  /* Setup the SSID and Password as required parameters */
  pos += snprintf((char *)cmd, W61_CMDRSP_STRING_SIZE, "AT+CWJAP=\"%s\",\"%s\",",
                  ConnectOpts->SSID, ConnectOpts->Password);

  /* Add optional BSSID parameters if defined */
  if (ConnectOpts->MAC[0] != '\0')
  {
    pos += snprintf((char *)&cmd[pos], W61_CMDRSP_STRING_SIZE - pos,
                    "\"" MACSTR "\"", MAC2STR(ConnectOpts->MAC));
  }

  /* Add optional WEP mode */
  if (!((ConnectOpts->WEP == 0) || (ConnectOpts->WEP == 1)))
  {
    WIFI_LOG_ERROR("WEP value is out of range [0;1]\n");
    return ret;
  }
  snprintf((char *)&cmd[pos], W61_CMDRSP_STRING_SIZE - pos, ",%" PRIu32 "\r\n", ConnectOpts->WEP);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_CONNECT_TIMEOUT);

  if ((ret == W61_STATUS_OK) && (W61_WiFi_SetReconnectionOpts(Obj, ConnectOpts) != W61_STATUS_OK))
  {
    WIFI_LOG_ERROR("Connection configuration command issued\n");
    return W61_STATUS_ERROR;
  }

  return ret;
}

W61_Status_t W61_WiFi_GetConnectInfo(W61_Object_t *Obj, int32_t *Rssi, W61_WiFi_SecurityType_e *Security,
                                     W61_WiFi_Protocol_e *Protocol)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  uint8_t index = 0;

  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Rssi);
  W61_NULL_ASSERT(Security);
  W61_NULL_ASSERT(Protocol);

  W61_Status_t ret;
  W61_WiFi_StaStateType_e state;

  strncpy(cmd, "AT+CWJAP?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWJAP:", &argc, argv, W61_WIFI_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 4)
  {
    return W61_STATUS_ERROR;
  }
  /* Parse the response */
  if (argc >= 5)
  {
    W61_AT_RemoveStrQuotes(argv[index]);
    strncpy((char *)Obj->WifiCtx.SSID, argv[index], W61_WIFI_MAX_SSID_SIZE);
    Obj->WifiCtx.SSID[W61_WIFI_MAX_SSID_SIZE] = '\0'; /* Ensure null termination */
    index++;
  }

  W61_AT_RemoveStrQuotes(argv[index]);
  Parser_StrToMAC(argv[index++], Obj->WifiCtx.APSettings.MAC_Addr);
  Obj->WifiCtx.STASettings.Channel = atoi(argv[index++]);
  *Rssi = atoi(argv[index++]);

  if (W61_SdkMinVersion(Obj, 2, 0, 97) == W61_STATUS_OK)
  {
    /* Get security type and protocol before disconnecting */
    strncpy(cmd, "AT+CWSTATE?\r\n", sizeof(cmd));
    ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWSTATE:", &argc, argv, W61_WIFI_TIMEOUT);
    if (ret != W61_STATUS_OK)
    {
      return ret;
    }
    if (argc < 4)
    {
      return W61_STATUS_ERROR;
    }
    state = (W61_WiFi_StaStateType_e)atoi(argv[0]);
    if ((state == W61_WIFI_STATE_STA_CONNECTED) || (state == W61_WIFI_STATE_STA_GOT_IP))
    {
      *Security = (W61_WiFi_SecurityType_e)atoi(argv[2]);
      *Protocol = (W61_WiFi_Protocol_e)atoi(argv[3]);
    }
    else
    {
      *Security = W61_WIFI_SECURITY_UNKNOWN;
      *Protocol = W61_WIFI_PROTOCOL_UNKNOWN;
    }
  }
  return ret;
}

W61_Status_t W61_WiFi_Disconnect(W61_Object_t *Obj, uint32_t restore)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWQAP=%" PRIu32 "\r\n", restore);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_TIMEOUT);
}

W61_Status_t W61_WiFi_Station_GetMACAddress(W61_Object_t *Obj, uint8_t *Mac)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mac);

  strncpy(cmd, "AT+CIPSTAMAC?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CIPSTAMAC:", &argc, argv, W61_WIFI_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  W61_AT_RemoveStrQuotes(argv[0]);
  Parser_StrToMAC(argv[0], Mac);

  return ret;
}

W61_Status_t W61_WiFi_GetCountryCode(W61_Object_t *Obj, uint32_t *Policy, char *CountryString)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Policy);
  W61_NULL_ASSERT(CountryString);

  CountryString[0] = '\0';
  strncpy(cmd, "AT+CWCOUNTRY?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWCOUNTRY:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 2)
  {
    return W61_STATUS_ERROR;
  }

  *Policy = atoi(argv[0]);
  W61_AT_RemoveStrQuotes(argv[1]);
  strncpy(CountryString, argv[1], 3);

  return ret;
}

W61_Status_t W61_WiFi_SetCountryCode(W61_Object_t *Obj, uint32_t *Policy, char *CountryString)
{
  int32_t code = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Policy);
  W61_NULL_ASSERT(CountryString);

  for (code = 0; code < W61_WIFI_COUNTRY_CODE_MAX; code++)
  {
    if (strcasecmp(CountryString, Country_code_str[code]) == 0)
    {
      break;
    }
  }
  if (code >= W61_WIFI_COUNTRY_CODE_MAX)
  {
    WIFI_LOG_ERROR("Incorrect country code string\n");
    return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWCOUNTRY=%" PRIu32 ",\"%s\"\r\n", *Policy, CountryString);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_TIMEOUT);
}

/* Soft-AP related functions definition ----------------------------------------------*/
W61_Status_t W61_WiFi_SetDualMode(W61_Object_t *Obj)
{
  W61_Status_t ret;
  uint32_t sta_state = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Check if station is connected */
  if ((Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_CONNECTED) || (Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_GOT_IP))
  {
    sta_state = 1;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWMODE=3,%" PRIu32 "\r\n", sta_state);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_TIMEOUT);

  if (ret == W61_STATUS_OK)
  {
    Obj->WifiCtx.ApState = W61_WIFI_STATE_AP_RUNNING;
  }

  return ret;
}

W61_Status_t W61_WiFi_AP_Start(W61_Object_t *Obj, W61_WiFi_ApConfig_t *ApConfig)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t tmp_rssi = 0;
  W61_WiFi_SecurityType_e tmp_security = W61_WIFI_SECURITY_UNKNOWN;
  W61_WiFi_Protocol_e tmp_protocol = W61_WIFI_PROTOCOL_UNKNOWN;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ApConfig);

  if (ApConfig->SSID[0] == '\0')
  {
    WIFI_LOG_ERROR("SSID cannot be NULL\n");
    return ret;
  }

  if (strlen((char *)ApConfig->SSID) > W61_WIFI_MAX_SSID_SIZE)
  {
    WIFI_LOG_ERROR("SSID is too long, maximum length is 32\n");
    return ret;
  }

  if (ApConfig->MaxConnections > W61_WIFI_MAX_CONNECTED_STATIONS)
  {
    WIFI_LOG_WARN("Max connections number is too high, set to default value 4\n");
    ApConfig->MaxConnections = W61_WIFI_MAX_CONNECTED_STATIONS;
  }

  /* Get the channel the station is connected to */
  if ((Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_CONNECTED) || (Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_GOT_IP))
  {
    if (W61_WiFi_GetConnectInfo(Obj, &tmp_rssi, &tmp_security, &tmp_protocol) != W61_STATUS_OK)
    {
      WIFI_LOG_ERROR("Get connection information failed\n");
      return W61_STATUS_ERROR;
    }
    ApConfig->Channel = Obj->WifiCtx.STASettings.Channel;
    WIFI_LOG_WARN("Soft-AP channel number will align on STATION's one : %" PRIu32 "\n", ApConfig->Channel);
  }
  else if ((ApConfig->Channel < 1) || (ApConfig->Channel > 13))
  {
    WIFI_LOG_WARN("Channel value out of range, set to default value 1\n");
    ApConfig->Channel = 1;
  }

  if ((ApConfig->Security > W61_WIFI_AP_SECURITY_WPA3_PSK) || (ApConfig->Security == W61_WIFI_AP_SECURITY_WEP))
  {
    WIFI_LOG_ERROR("Security not supported\n");
    return ret;
  }
  else
  {
    if ((ApConfig->Security != W61_WIFI_AP_SECURITY_OPEN) &&
        ((strlen((char *)ApConfig->Password) < 8) ||
         (strlen((char *)ApConfig->Password) > W61_WIFI_MAX_PASSWORD_SIZE)))
    {
      WIFI_LOG_ERROR("Password length incorrect, must be in following range [8;63]\n");
      return ret;
    }

    /* Need to set the password to null if security selected is OPEN */
    if ((ApConfig->Security == W61_WIFI_AP_SECURITY_OPEN) && (strlen((char *)ApConfig->Password) > 0))
    {
      WIFI_LOG_WARN("Password is not needed for open security, set to NULL\n");
      ApConfig->Password[0] = '\0';
    }
  }

  if (ApConfig->Hidden > 1)
  {
    WIFI_LOG_WARN("Hidden parameter is not supported, set to default value 0\n");
    ApConfig->Hidden = 0;
  }

  if (ApConfig->Protocol == W61_WIFI_PROTOCOL_UNKNOWN)
  {
    WIFI_LOG_WARN("Protocol set to default value (802.11ax)\n");
    ApConfig->Protocol = W61_WIFI_PROTOCOL_11AX;
  }
  if (W61_WiFi_AP_SetMode(Obj, ApConfig->Protocol) != W61_STATUS_OK)
  {
    return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWSAP=\"%s\",\"%s\",%" PRIu32 ",%" PRIu16 ",%" PRIu32 ",%" PRIu32 "\r\n",
           ApConfig->SSID, ApConfig->Password, ApConfig->Channel, ApConfig->Security,
           ApConfig->MaxConnections, ApConfig->Hidden);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_TIMEOUT);

  if (ret == W61_STATUS_OK)
  {
    WIFI_LOG_DEBUG("Soft-AP configuration done\n");
  }

  return ret;
}

W61_Status_t W61_WiFi_AP_Stop(W61_Object_t *Obj, uint8_t Reconnect)
{
  W61_Status_t ret;
  uint32_t sta_state = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Check if station is connected */
  if ((Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_CONNECTED) || (Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_GOT_IP))
  {
    sta_state = 1;
  }

  if (Reconnect == 0)
  {
    snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWMODE=1,0\r\n");
  }
  else
  {
    snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWMODE=1,%" PRIu32 "\r\n", sta_state);
  }
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_TIMEOUT);

  if (ret == W61_STATUS_OK)
  {
    Obj->WifiCtx.ApState = W61_WIFI_STATE_AP_RESET;
  }

  return ret;
}

W61_Status_t W61_WiFi_AP_GetConfig(W61_Object_t *Obj, W61_WiFi_ApConfig_t *ApConfig)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ApConfig);

  strncpy(cmd, "AT+CWSAP?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWSAP:", &argc, argv, W61_WIFI_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 6)
  {
    return W61_STATUS_ERROR;
  }

  W61_AT_RemoveStrQuotes(argv[0]);
  strncpy((char *)ApConfig->SSID, argv[0], W61_WIFI_MAX_SSID_SIZE);
  ApConfig->SSID[W61_WIFI_MAX_SSID_SIZE] = '\0'; /* Ensure null termination */
  W61_AT_RemoveStrQuotes(argv[1]);
  strncpy((char *)ApConfig->Password, argv[1], W61_WIFI_MAX_PASSWORD_SIZE);
  ApConfig->Password[W61_WIFI_MAX_PASSWORD_SIZE] = '\0'; /* Ensure null termination */
  ApConfig->Channel = (uint32_t)atoi(argv[2]);
  ApConfig->Security = (W61_WiFi_ApSecurityType_e)atoi(argv[3]);
  ApConfig->MaxConnections = (uint32_t)atoi(argv[4]);
  ApConfig->Hidden = (uint32_t)atoi(argv[5]);

  if (W61_WiFi_AP_GetMode(Obj, &ApConfig->Protocol) != W61_STATUS_OK)
  {
    return W61_STATUS_ERROR;
  }

  memset(ApConfig->Password, 0, W61_WIFI_MAX_PASSWORD_SIZE + 1);
  return ret;
}

W61_Status_t W61_WiFi_AP_ListConnectedStations(W61_Object_t *Obj, W61_WiFi_Connected_Sta_t *Stations)
{
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Stations);
  W61_Status_t ret;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+CWLIF:", on_cmd_ap_liststa, 2U, ","),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = Stations;
  Stations->Count = 0;

  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+CWLIF\r\n",
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_WiFi_AP_DisconnectStation(W61_Object_t *Obj, uint8_t *MAC)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(MAC);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWQIF=\"" MACSTR "\"\r\n", MAC2STR(MAC));
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_TIMEOUT);
}

W61_Status_t W61_WiFi_AP_GetMode(W61_Object_t *Obj, W61_WiFi_Protocol_e *Protocol)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  uint32_t temp_protocol = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Protocol);

  strncpy(cmd, "AT+CWAPPROTO?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWAPPROTO:", &argc, argv, W61_WIFI_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  temp_protocol = (uint32_t)atoi(argv[0]);
  switch (temp_protocol)
  {
    case 15:
      *Protocol = W61_WIFI_PROTOCOL_11AX;
      break;
    case 7:
      *Protocol = W61_WIFI_PROTOCOL_11N;
      break;
    case 3:
      *Protocol = W61_WIFI_PROTOCOL_11G;
      break;
    case 1:
      *Protocol = W61_WIFI_PROTOCOL_11B;
      break;
    default:
      SYS_LOG_ERROR("Invalid Wi-Fi protocol: %d\n", temp_protocol);
      return W61_STATUS_ERROR;
  }

  return ret;
}

W61_Status_t W61_WiFi_AP_SetMode(W61_Object_t *Obj, W61_WiFi_Protocol_e Protocol)
{
  uint32_t temp_protocol = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  switch (Protocol)
  {
    case W61_WIFI_PROTOCOL_11AX:
      temp_protocol = 15;
      break;
    case W61_WIFI_PROTOCOL_11N:
      temp_protocol = 7;
      break;
    case W61_WIFI_PROTOCOL_11G:
      temp_protocol = 3;
      break;
    case W61_WIFI_PROTOCOL_11B:
      temp_protocol = 1;
      break;
    default:
      return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWAPPROTO=%" PRIu32 "\r\n", temp_protocol);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_WIFI_TIMEOUT);
}

W61_Status_t W61_WiFi_AP_GetMACAddress(W61_Object_t *Obj, uint8_t *Mac)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mac);

  strncpy(cmd, "AT+CIPAPMAC?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CIPAPMAC:", &argc, argv, W61_WIFI_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  W61_AT_RemoveStrQuotes(argv[0]);
  Parser_StrToMAC(argv[0], Mac);

  return ret;
}

W61_Status_t W61_WiFi_SetDTIM(W61_Object_t *Obj, uint32_t dtim)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  uint32_t max_dtim = 10;
  W61_NULL_ASSERT(Obj);

  if (W61_SdkMinVersion(Obj, 2, 0, 97) != W61_STATUS_OK)
  {
    if (dtim == 0)
    {
      WIFI_LOG_ERROR("DTIM value should be greater than 0\n");
      return W61_STATUS_ERROR;
    }
    max_dtim = 25;
  }

  /* Verify the DTIM to configure does not exceed the maximum value */
  if (dtim > max_dtim)
  {
    WIFI_LOG_ERROR("DTIM value exceeds maximum value : %" PRIu32 "\n", max_dtim);
    return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+SLWKDTIM=%" PRIu32 "\r\n", dtim);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_WiFi_GetDTIM_AP(W61_Object_t *Obj, uint32_t *dtim)
{
  W61_Status_t ret;
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_NULL_ASSERT(Obj);

  strncpy(cmd, "AT+GET_AP_DTIM?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+DTIM:", &argc, argv, W61_WIFI_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *dtim = (uint32_t)atoi(argv[0]);
  return W61_STATUS_OK;
}

W61_Status_t W61_WiFi_TWT_Setup(W61_Object_t *Obj, W61_WiFi_TWT_Setup_Params_t *twt_params)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(twt_params);

  if (twt_params->setup_type > 2)
  {
    WIFI_LOG_ERROR("TWT setup type should be between [0; 2]\n");
    goto _err;
  }
  if (twt_params->flow_type > 1)
  {
    WIFI_LOG_ERROR("TWT flow type should be between [0; 1]\n");
    goto _err;
  }
  if (twt_params->wake_int_exp > 31)
  {
    WIFI_LOG_ERROR("TWT wake interval exponent should be between [0; 31]\n");
    goto _err;
  }
  if (twt_params->min_twt_wake_dur > 0xFF)
  {
    WIFI_LOG_ERROR("TWT minimum wake duration should be between [0; 255]\n");
    goto _err;
  }
  if (twt_params->wake_int_mantissa > 0xFFFF)
  {
    WIFI_LOG_ERROR("TWT wake interval mantissa should be between [0; 65535]\n");
    goto _err;
  }

  snprintf((char *)cmd, W61_CMDRSP_STRING_SIZE,
           "AT+TWT_PARAM=%" PRIu16 ",%" PRIu16 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
           twt_params->setup_type, twt_params->flow_type, twt_params->wake_int_exp,
           twt_params->min_twt_wake_dur, twt_params->wake_int_mantissa);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);

_err:
  return ret;
}

W61_Status_t W61_WiFi_TWT_GetStatus(W61_Object_t *Obj, W61_WiFi_TWT_Status_t *twt_status)
{
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(twt_status);

  struct modem_cmd handlers[] =
  {
    MODEM_CMD_ARGS_MAX("+TWT:", on_cmd_twt_status, 1U, 7U, ":,"),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = twt_status;

  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+TWT_STATUS?\r\n",
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_WiFi_TWT_Teardown(W61_Object_t *Obj, W61_WiFi_TWT_Teardown_Params_t *twt_params)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(twt_params);

#if 0 /* TWT multi flow is not supported in current version */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+TWT_TEARDOWN=0,%" PRIu16 ",%" PRIu16 "\r\n",
           twt_params->all_twt, twt_params->id);
#else
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+TWT_TEARDOWN=0,1,0\r\n");
#endif /* TWT multi flow is not supported in current version */

  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_WiFi_TWT_IsSupported(W61_Object_t *Obj, uint32_t *is_supported)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(is_supported);

  strncpy(cmd, "AT+GET_TWT_SUPPORTED?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+TWT_SUPPORT:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *is_supported = (uint32_t)atoi(argv[0]);
  return ret;
}

W61_Status_t W61_WiFi_SetAntennaEnable(W61_Object_t *Obj, W61_WiFi_AntennaMode_e mode)
{
  uint32_t pin = 0;
  uint32_t dynamic_enable = 0;
  uint32_t static_enable = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if (Obj->ModuleInfo.ModuleID.ModuleID == W61_MODULE_ID_B)
  {
    SYS_LOG_ERROR("Antenna configuration not supported by the module\n");
    return W61_STATUS_ERROR;
  }

  if (mode == W61_WIFI_ANTENNA_STATIC)
  {
    static_enable = 1;
  }
  else if ((W61_SdkMinVersion(Obj, 2, 0, 97) == W61_STATUS_OK) && (mode == W61_WIFI_ANTENNA_DYNAMIC))
  {
    static_enable = 1;
    dynamic_enable = 1;
  }
  else
  {
    SYS_LOG_ERROR("Invalid antenna mode\n");
    return W61_STATUS_ERROR;
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+CWANTENABLE=%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
           dynamic_enable, static_enable, pin);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NCP_TIMEOUT);
}

W61_Status_t W61_WiFi_GetAntennaEnable(W61_Object_t *Obj, W61_WiFi_AntennaMode_e *mode)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  uint32_t dynamic_enable;
  uint32_t static_enable;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);

  strncpy(cmd, "AT+CWANTENABLE?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWANTENABLE:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 2)
  {
    return W61_STATUS_ERROR;
  }

  dynamic_enable = (uint32_t)atoi(argv[0]);
  static_enable = (uint32_t)atoi(argv[1]);
  if ((dynamic_enable == 0) && (static_enable == 0))
  {
    *mode = W61_WIFI_ANTENNA_DISABLED;
  }
  else if (dynamic_enable == 1)
  {
    *mode = W61_WIFI_ANTENNA_DYNAMIC;
  }
  else if ((dynamic_enable == 0) && (static_enable == 1))
  {
    *mode = W61_WIFI_ANTENNA_STATIC;
  }
  else
  {
    *mode = W61_WIFI_ANTENNA_UNKNOWN;
  }
  /* Unused: pin = (uint32_t)atoi(argv[2]); */
  return ret;
}

W61_Status_t W61_WiFi_GetAntennaUsed(W61_Object_t *Obj, uint32_t *antenna_id)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);

  strncpy(cmd, "AT+CWANT?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+CWANT:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *antenna_id = (uint32_t)atoi(argv[0]);
  return ret;
}

/* Private Functions Definition ----------------------------------------------*/
MODEM_CMD_DEFINE(on_cmd_ap_liststa)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_WiFi_Connected_Sta_t *stations = (W61_WiFi_Connected_Sta_t *)mdm->rx_data;

  if ((argc >= 2) && (stations->Count < W61_WIFI_MAX_CONNECTED_STATIONS))
  {
    uint8_t count = stations->Count++;
    Parser_StrToIP((char *)argv[0], stations->STA[count].IP);
    Parser_StrToMAC((char *)argv[1], stations->STA[count].MAC);
  }

  return 0;
}

MODEM_CMD_DEFINE(on_cmd_twt_status)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_WiFi_TWT_Status_t *twt_status = (W61_WiFi_TWT_Status_t *)mdm->rx_data;

  if ((argc >= 1) && (strcmp((char *)argv[0], "INACTIVE") == 0))
  {
    twt_status->flow_count = 0;
    return 0;
  }
  else if ((argc >= 7) && (strcmp((char *)argv[0], "ACTIVE") == 0))
  {
    uint32_t flow_index = atoi((char *)argv[1]);
    if (flow_index < W61_WIFI_MAX_TWT_FLOWS)
    {
      twt_status->flow[flow_index].flow_type = atoi((char *)argv[2]);
      twt_status->flow[flow_index].wake_int_exp = atoi((char *)argv[4]);
      twt_status->flow[flow_index].min_twt_wake_dur = atoi((char *)argv[5]);
      twt_status->flow[flow_index].wake_int_mantissa = atoi((char *)argv[6]);
      twt_status->flow_count++;
    }
  }
  else
  {
    WIFI_LOG_ERROR("Invalid TWT status response\n");
  }
  return 0;
}

static void W61_WiFi_AT_Event(void *hObj, uint16_t *argc, char **argv)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  W61_WiFi_CbParamData_t cb_param_wifi_data;
  if ((Obj == NULL) || (*argc < 1))
  {
    return;
  }

  if (strcmp(argv[0], "SCAN_DONE") == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_SCAN_DONE_ID, NULL);
    }
    return;
  }

  if (strcmp(argv[0], "CONNECTED") == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_CONNECTED_ID, NULL);
    }
    return;
  }

  if (strcmp(argv[0], "GOTIP") == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_GOT_IP_ID, NULL);
    }
    return;
  }

  if (strcmp(argv[0], "DISCONNECTED") == 0)
  {
    /* Reset the context structure with all the information relative to the previous connection */
    memset(&Obj->NetCtx.Net_sta_info, 0, sizeof(Obj->NetCtx.Net_sta_info));
    memset(&Obj->WifiCtx.SSID, 0, sizeof(Obj->WifiCtx.SSID));
    memset(&Obj->WifiCtx.STASettings, 0, sizeof(Obj->WifiCtx.STASettings));
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_DISCONNECTED_ID, NULL);
    }
    return;
  }

  if (strcmp(argv[0], "CONNECTING") == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_CONNECTING_ID, NULL);
    }
    return;
  }

  if (strcmp(argv[0], "ERROR") == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb == NULL)
    {
      return;
    }
    int32_t reason_code = atoi(argv[1]);
    Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_REASON_ID, &reason_code);
    return;
  }

  if ((strcmp(argv[0], "STA_CONNECTED") == 0) && (*argc >= 2))
  {
    W61_AT_RemoveStrQuotes(argv[1]);

    /* Parse the MAC address */
    Parser_StrToMAC(argv[1], cb_param_wifi_data.MAC);

    /* Check the MAC address validity */
    if (Parser_CheckValidAddress(cb_param_wifi_data.MAC, 6) != 0)
    {
      WIFI_LOG_ERROR("MAC address is not valid\n");
      return;
    }

    if (Obj->ulcbs.UL_wifi_ap_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_ap_cb(W61_WIFI_EVT_STA_CONNECTED_ID, &cb_param_wifi_data);
    }
    return;
  }

  if ((strcmp(argv[0], "STA_DISCONNECTED") == 0) && (*argc >= 2))
  {
    /* Parse the MAC address */
    W61_AT_RemoveStrQuotes(argv[1]);
    Parser_StrToMAC(argv[1], cb_param_wifi_data.MAC);

    /* Check the MAC address validity */
    if (Parser_CheckValidAddress(cb_param_wifi_data.MAC, 6) != 0)
    {
      WIFI_LOG_ERROR("MAC address is not valid\n");
      return;
    }

    if (Obj->ulcbs.UL_wifi_ap_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_ap_cb(W61_WIFI_EVT_STA_DISCONNECTED_ID, &cb_param_wifi_data);
    }
    return;
  }

  if ((strcmp(argv[0], "DIST_STA_IP") == 0) && (*argc >= 3))
  {
    /* Parse the MAC address */
    W61_AT_RemoveStrQuotes(argv[1]);
    Parser_StrToMAC(argv[1], cb_param_wifi_data.MAC);

    /* Check the MAC address validity */
    if (Parser_CheckValidAddress(cb_param_wifi_data.MAC, 6) != 0)
    {
      WIFI_LOG_ERROR("MAC address is not valid\n");
      return;
    }

    /* Parse the IP address */
    W61_AT_RemoveStrQuotes(argv[2]);
    Parser_StrToIP(argv[2], cb_param_wifi_data.IP);

    /* Check the IP address validity */
    if (Parser_CheckValidAddress(cb_param_wifi_data.IP, 4) != 0)
    {
      WIFI_LOG_ERROR("IP address is not valid\n");
      return;
    }

    if (Obj->ulcbs.UL_wifi_ap_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_ap_cb(W61_WIFI_EVT_DIST_STA_IP_ID, &cb_param_wifi_data);
    }
    return;
  }
}

static void W61_WiFi_scan_result(void *hObj, uint16_t *argc, char **argv)
{
  int32_t protocol;
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  W61_WiFi_Scan_Result_t *scan_result = &Obj->WifiCtx.ScanResults;

  if ((scan_result == NULL) || (scan_result->AP == NULL))
  {
    return;
  }

  /* Parsing of arguments for scan result with 0x */
  if (*argc >= 8)
  {
    if (scan_result->Count >= W61_WIFI_MAX_DETECTED_AP)
    {
      if (scan_result->More == 0)
      {
        SYS_LOG_WARN("Maximum number of APs reached: %d\n", W61_WIFI_MAX_DETECTED_AP);
        scan_result->More = 1;
      }
      return;
    }

    /* Remove first '(' */
    scan_result->AP[scan_result->Count].Security = (W61_WiFi_SecurityType_e)atoi((char *)&argv[0][1]);
    W61_AT_RemoveStrQuotes((char *)argv[1]);
    strncpy((char *)scan_result->AP[scan_result->Count].SSID, (char *)argv[1], W61_WIFI_MAX_SSID_SIZE);
    scan_result->AP[scan_result->Count].RSSI = atoi((char *)argv[2]);
    W61_AT_RemoveStrQuotes((char *)argv[3]);
    Parser_StrToMAC((char *)argv[3], scan_result->AP[scan_result->Count].MAC);
    scan_result->AP[scan_result->Count].Channel = atoi((char *)argv[4]);
    scan_result->AP[scan_result->Count].Pairwise_cipher = (W61_WiFi_CipherType_e)atoi((char *)argv[5]);
    /* Rework to cast argv[6] and convert 15 in 3, 7 in 2, 2 in 1 and 1 in 0 */
    protocol = atoi((char *)argv[6]);
    switch (protocol)
    {
      case 15:
        scan_result->AP[scan_result->Count].Protocol = W61_WIFI_PROTOCOL_11AX;
        break;
      case 7:
        scan_result->AP[scan_result->Count].Protocol = W61_WIFI_PROTOCOL_11N;
        break;
      case 3:
        scan_result->AP[scan_result->Count].Protocol = W61_WIFI_PROTOCOL_11G;
        break;
      case 1:
        scan_result->AP[scan_result->Count].Protocol = W61_WIFI_PROTOCOL_11B;
        break;
      default:
        SYS_LOG_ERROR("Invalid Wi-Fi version: %d\n", protocol);
        return;
    }
    /* Remove last ')' */
    argv[7][1] = '\0';
    scan_result->AP[scan_result->Count].WPS = atoi((char *)argv[7]);
    scan_result->Count++;
  }
  else
  {
    SYS_LOG_ERROR("Invalid number of arguments: %d\n", *argc);
    return;
  }

  return;
}
/** @} */
