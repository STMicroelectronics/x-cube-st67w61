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
#include "w61_at_rx_parser.h"
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

#define W61_WIFI_EVT_SCAN_RESULT_KEYWORD          "+CWLAP:"               /*!< Scan result event keyword */
#define W61_WIFI_EVT_SCAN_DONE_KEYWORD            "+CW:SCAN_DONE"         /*!< Scan done event keyword */
#define W61_WIFI_EVT_CONNECTED_KEYWORD            "+CW:CONNECTED"         /*!< Connected event keyword */
#define W61_WIFI_EVT_DISCONNECTED_KEYWORD         "+CW:DISCONNECTED"      /*!< Disconnected event keyword */
#define W61_WIFI_EVT_GOT_IP_KEYWORD               "+CW:GOTIP"             /*!< Got IP event keyword */
#define W61_WIFI_EVT_CONNECTING_KEYWORD           "+CW:CONNECTING"        /*!< Connecting event keyword */
#define W61_WIFI_EVT_ERROR_KEYWORD                "+CW:ERROR,"            /*!< Error event keyword */

#define W61_WIFI_EVT_AP_STA_CONNECTED_KEYWORD     "+CW:STA_CONNECTED"     /*!< Station connected event keyword */
#define W61_WIFI_EVT_AP_STA_DISCONNECTED_KEYWORD  "+CW:STA_DISCONNECTED"  /*!< Station disconnected event keyword */
#define W61_WIFI_EVT_AP_STA_IP_KEYWORD            "+CW:DIST_STA_IP"       /*!< Station IP event keyword */

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

/** sscanf parsing error string */
static const char W61_Parsing_Error_str[] = "Parsing of the result failed";

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_WiFi_Functions
  * @{
  */

/**
  * @brief  Parses WiFi event and call related callback.
  * @param  hObj: pointer to module handle
  * @param  p_evt: pointer to event buffer
  * @param  evt_len: event length
  */
static void W61_WiFi_AT_Event(void *hObj, const uint8_t *p_evt, int32_t evt_len);

/**
  * @brief  Parses Access Point configuration.
  * @param  pdata: pointer to the data
  * @param  len: length of the data
  * @param  APs: Scan results structure containing the received information of beacons from Access Points
  */
static void W61_WiFi_AtParseAp(char *pdata, int32_t len, W61_WiFi_Scan_Result_t *APs);

/* Functions Definition ------------------------------------------------------*/
W61_Status_t W61_WiFi_Init(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  Obj->WifiCtx.ScanResults.AP = pvPortMalloc(sizeof(W61_WiFi_AP_t) * W61_WIFI_MAX_DETECTED_AP);
  if ((Obj->WifiCtx.ScanResults.AP == NULL) && (W61_WIFI_MAX_DETECTED_AP != 0))
  {
    LogError("Error: Unable to allocate memory for scan results\n");
    return W61_STATUS_ERROR;
  }
  memset(Obj->WifiCtx.ScanResults.AP, 0, sizeof(W61_WiFi_AP_t)*W61_WIFI_MAX_DETECTED_AP);
  Obj->WifiCtx.ScanResults.Count = 0;

  /* Create the Wi-Fi event handle */
  Obj->WifiCtx.Wifi_event = xEventGroupCreate();

  Obj->WiFi_event_cb = W61_WiFi_AT_Event;

  return W61_STATUS_OK;
}

W61_Status_t W61_WiFi_DeInit(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (Obj->WifiCtx.ScanResults.AP != NULL)
  {
    vPortFree(Obj->WifiCtx.ScanResults.AP);
    Obj->WifiCtx.ScanResults.AP = NULL;
  }
  Obj->WifiCtx.ScanResults.Count = 0;

  Obj->WiFi_event_cb = NULL;

  return W61_STATUS_OK;
}

W61_Status_t W61_WiFi_SetAutoConnect(W61_Object_t *Obj, uint32_t OnOff)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (!((OnOff == 0) || (OnOff == 1)))
  {
    LogError("Invalid value passed to Autoconnect function\n");
    return ret;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWAUTOCONN=%" PRIu32 "\r\n", OnOff);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
    if (ret == W61_STATUS_OK)
    {
      Obj->WifiCtx.DeviceConfig.AutoConnect = OnOff;
    }
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetAutoConnect(W61_Object_t *Obj, uint32_t *OnOff)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(OnOff);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWAUTOCONN?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+CWAUTOCONN:%" SCNu32 "\r\n", OnOff) != 1)
      {
        LogError("%s\n", W61_Parsing_Error_str);
        ret = W61_STATUS_ERROR;
      }
      else
      {
        ret = W61_STATUS_OK;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetHostname(W61_Object_t *Obj, uint8_t Hostname[33])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWHOSTNAME=\"%s\"\r\n", Hostname);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetHostname(W61_Object_t *Obj, uint8_t Hostname[33])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWHOSTNAME?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+CWHOSTNAME:%32[^\r\n]", Hostname) != 1)
      {
        ret = W61_STATUS_ERROR;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_WiFi_ActivateSta(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWMODE=1,0\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetScanOpts(W61_Object_t *Obj, W61_WiFi_Scan_Opts_t *ScanOpts)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t max_cnt = 50;
  W61_WiFi_scan_type_e type = W61_WIFI_SCAN_ACTIVE;
  uint8_t SSID[W61_WIFI_MAX_SSID_SIZE + 1] = {'\0'};
  uint8_t MAC[6] = {'\0'};
  uint8_t channel = 0;
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

  /* The config of the scan options must be made before every scan */
  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+CWLAPOPT=1,2047,-100,255,%" PRIu32 "\r\n", max_cnt);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_Scan(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char MAC[20 + 1];
  char SSID[W61_WIFI_MAX_SSID_SIZE + 3] = {'\0'}; /* Size + 2 if contains double-quote characters */
  W61_NULL_ASSERT(Obj);

  if (Obj->WifiCtx.ScanResults.Count > 0)
  {
    vPortFree(Obj->WifiCtx.ScanResults.AP);
    Obj->WifiCtx.ScanResults.AP = pvPortMalloc(sizeof(W61_WiFi_AP_t) * W61_WIFI_MAX_DETECTED_AP);
    if ((Obj->WifiCtx.ScanResults.AP == NULL) && (W61_WIFI_MAX_DETECTED_AP != 0))
    {
      LogError("Error: Unable to allocate memory for scan results\n");
      return W61_STATUS_ERROR;
    }
    memset(Obj->WifiCtx.ScanResults.AP, 0, sizeof(W61_WiFi_AP_t)* W61_WIFI_MAX_DETECTED_AP);
    Obj->WifiCtx.ScanResults.Count = 0;
  }

  snprintf(MAC, 20, "\"" MACSTR "\"", MAC2STR(ScanOptions.MAC));
  if (strcmp((char *)ScanOptions.SSID, "\0") == 0)
  {
    snprintf(SSID, W61_WIFI_MAX_SSID_SIZE + 3, "%s", ScanOptions.SSID);
  }
  else
  {
    snprintf(SSID, W61_WIFI_MAX_SSID_SIZE + 3, "\"%s\"", ScanOptions.SSID);
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+CWLAP=%" PRIu32 ",%s,%s,%" PRIu16 "\r\n",
             (uint32_t)ScanOptions.scan_type, SSID,
             strcmp(MAC, "\"00:00:00:00:00:00\"") == 0 ? "" : MAC, ScanOptions.Channel);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetReconnectionOpts(W61_Object_t *Obj, W61_WiFi_Connect_Opts_t *ConnectOpts)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnectOpts);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+CWRECONNCFG=%" PRIu16 ",%" PRIu16 "\r\n",
             ConnectOpts->Reconnection_interval, ConnectOpts->Reconnection_nb_attempts);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_Connect(W61_Object_t *Obj, W61_WiFi_Connect_Opts_t *ConnectOpts)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t pos = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnectOpts);

  if (ConnectOpts->WPS)
  {
    if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
    {
      snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+WPS=1\r\n");
      ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
      W61_ATunlock(Obj);
    }
    else
    {
      ret = W61_STATUS_BUSY;
    }
    return ret;
  }

  if (ConnectOpts->SSID[0] == '\0')
  {
    LogError("SSID cannot be NULL\n");
    return ret;
  }

  /* The config of the connect options must be made before every connect */
  if (ConnectOpts->Reconnection_interval > W61_WIFI_RECONNECTION_INTERVAL)
  {
    LogError("Reconnection interval must be between [0;7200], parameter set to default value : 0\n");
    ConnectOpts->Reconnection_interval = 0;
  }
  if (ConnectOpts->Reconnection_nb_attempts > W61_WIFI_RECONNECTION_ATTEMPTS)
  {
    LogError("Number of reconnection attempts must be between [0;1000], parameter set to default value : 0\n");
    ConnectOpts->Reconnection_nb_attempts = 0;
  }

  /* Setup the SSID and Password as required parameters */
  pos += snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWJAP=\"%s\",\"%s\",",
                  ConnectOpts->SSID, ConnectOpts->Password);

  /* Add optional BSSID parameters if defined */
  if (ConnectOpts->MAC[0] != '\0')
  {
    pos += snprintf((char *)&Obj->CmdResp[pos], W61_ATD_CMDRSP_STRING_SIZE - pos,
                    "\"" MACSTR "\"", MAC2STR(ConnectOpts->MAC));
  }

  /* Add optional WEP mode */
  if (!((ConnectOpts->WEP == 0) || (ConnectOpts->WEP == 1)))
  {
    LogError("WEP value is out of range [0;1]\n");
    return ret;
  }
  snprintf((char *)&Obj->CmdResp[pos], W61_ATD_CMDRSP_STRING_SIZE - pos, ",%" PRIu32 "\r\n", ConnectOpts->WEP);

  if (W61_ATlock(Obj, W61_WIFI_CONNECT_TIMEOUT))
  {
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_CONNECT_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if ((ret == W61_STATUS_OK) && (W61_WiFi_SetReconnectionOpts(Obj, ConnectOpts) != W61_STATUS_OK))
  {
    LogError("Connection configuration command issued\n");
    return W61_STATUS_ERROR;
  }

  return ret;
}

W61_Status_t W61_WiFi_GetConnectInfo(W61_Object_t *Obj, int32_t *Rssi)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char tmp_mac[18];
  int32_t cmp = 0;
  uint32_t wep = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Rssi);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWJAP?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      cmp = sscanf((char *)Obj->CmdResp,
                   "+CWJAP:\"%32[^\"]\",\"%17[^\"]\",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                   (char *)Obj->WifiCtx.NetSettings.SSID, tmp_mac, &(Obj->WifiCtx.STASettings.Channel),
                   Rssi, &wep);

      if (cmp != 5)
      {
        /* Search with no SSID */
        cmp = sscanf((char *)Obj->CmdResp, "+CWJAP:\"\",\"%17[^\"]\",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                     tmp_mac, &(Obj->WifiCtx.STASettings.Channel), Rssi, &wep);
        if (cmp != 4)
        {
          LogError("Get connection information failed\n");
          ret = W61_STATUS_ERROR;
        }
        else
        {
          Obj->WifiCtx.NetSettings.SSID[0] = 0;
        }
      }

      if (cmp >= 4)
      {
        Parser_StrToMAC(tmp_mac, Obj->WifiCtx.APSettings.MAC_Addr);
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_Disconnect(W61_Object_t *Obj, uint32_t restore)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWQAP=%" PRIu32 "\r\n", restore);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetStaMode(W61_Object_t *Obj, W61_WiFi_Mode_t *Mode)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  int32_t tmp = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mode);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWSTAPROTO?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      ptr = strstr((char *)(Obj->CmdResp), "+CWSTAPROTO:");
      if (ptr == NULL)
      {
        ret = W61_STATUS_ERROR;
      }
      else
      {
        ptr += sizeof("+CWSTAPROTO:") - 1;
        token = strstr(ptr, "\r");
        if (token == NULL)
        {
          ret = W61_STATUS_ERROR;
        }
        else
        {
          *(token) = 0;
          if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
          {
            ret = W61_STATUS_ERROR;
          }
          else
          {
            Mode->byte = (uint8_t)tmp;
            ret = W61_STATUS_OK;
          }
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetStaMode(W61_Object_t *Obj, W61_WiFi_Mode_t *Mode)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mode);

  if (Mode->byte > 0xF) /* 4 bits */
  {
    LogError("Invalid mode\n");
    return ret;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWSTAPROTO=%" PRIu16 "\r\n", Mode->byte);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetStaMacAddress(W61_Object_t *Obj, uint8_t *Mac)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mac);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CIPSTAMAC?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      ptr = strstr((char *)(Obj->CmdResp), "+CIPSTAMAC:");
      if (ptr == NULL)
      {
        ret = W61_STATUS_ERROR;
      }
      else
      {
        ptr += sizeof("+CIPSTAMAC:");
        token = strstr(ptr, "\r");
        if (token == NULL)
        {
          ret = W61_STATUS_ERROR;
        }
        else
        {
          *(--token) = 0;
          Parser_StrToMAC(ptr, Mac);
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetStaIpAddress(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  uint32_t line = 0;
  int32_t recv_len;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CIPSTA?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, Obj->NcpTimeout);
      if (recv_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }
      while ((recv_len > 0) && (strncmp((char *)Obj->CmdResp, "+CIPSTA:", strlen("+CIPSTA:")) == 0))
      {
        Obj->CmdResp[recv_len] = 0;
        switch (line++)
        {
          case 0:
            ptr = strstr((char *)(Obj->CmdResp), "+CIPSTA:ip:");
            if (ptr == NULL)
            {
              goto _err;
            }
            ptr += sizeof("+CIPSTA:ip:");

            token = strstr(ptr, "\r");
            if (token == NULL)
            {
              goto _err;
            }

            *(--token) = 0;
            Parser_StrToIP(ptr, Obj->WifiCtx.NetSettings.IP_Addr);
            break;
          case 1:
            ptr = strstr((char *)(Obj->CmdResp), "+CIPSTA:gateway:");
            if (ptr == NULL)
            {
              goto _err;
            }
            ptr += sizeof("+CIPSTA:gateway:");

            token = strstr(ptr, "\r");
            if (token == NULL)
            {
              goto _err;
            }

            *(--token) = 0;
            Parser_StrToIP(ptr, Obj->WifiCtx.NetSettings.Gateway_Addr);
            break;
          case 2:
            ptr = strstr((char *)(Obj->CmdResp), "+CIPSTA:netmask:");
            if (ptr == NULL)
            {
              goto _err;
            }
            ptr += sizeof("+CIPSTA:netmask:");

            token = strstr(ptr, "\r");
            if (token == NULL)
            {
              goto _err;
            }
            *(--token) = 0;
            Parser_StrToIP(ptr, Obj->WifiCtx.NetSettings.IP_Mask);
            break;
          default:
            break;
        }
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, Obj->NcpTimeout);
      }

      if (recv_len > 0)
      {
        Obj->CmdResp[recv_len] = 0;
        ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
      }
      else
      {
        ret = W61_STATUS_TIMEOUT;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;

_err:
  W61_ATunlock(Obj);
  return W61_STATUS_ERROR;
}

W61_Status_t W61_WiFi_SetStaIpAddress(W61_Object_t *Obj, uint8_t Ip_addr[4], uint8_t Gateway_addr[4],
                                      uint8_t Netmask_addr[4])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t Gateway_addr_def[4];
  uint8_t Netmask_addr_def[4];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT_STR(Ip_addr, "Station IP NULL");

  if (Parser_CheckIP(Ip_addr) != W61_STATUS_OK)
  {
    LogError("Station IP is invalid\n");
    goto _err;
  }

  /* Get the actual IP configuration to replace fields not specified in the function */
  ret = W61_WiFi_GetStaIpAddress(Obj);
  if (ret != W61_STATUS_OK)
  {
    goto _err;
  }

  if ((Gateway_addr == NULL) || (Parser_CheckIP(Gateway_addr) != W61_STATUS_OK))
  {
    LogWarn("Gateway IP NULL or invalid. Previous one will be use\n");
    memcpy(Gateway_addr_def, Obj->WifiCtx.NetSettings.Gateway_Addr, W61_WIFI_SIZEOF_IPV4_BYTES);
  }
  else
  {
    memcpy(Gateway_addr_def, Gateway_addr, W61_WIFI_SIZEOF_IPV4_BYTES);
  }

  if ((Netmask_addr == NULL) || (Parser_CheckIP(Netmask_addr) != W61_STATUS_OK))
  {
    LogWarn("Netmask IP NULL or invalid. Previous one will be use\n");
    memcpy(Netmask_addr_def, Obj->WifiCtx.NetSettings.IP_Mask, W61_WIFI_SIZEOF_IPV4_BYTES);
  }
  else
  {
    memcpy(Netmask_addr_def, Netmask_addr, W61_WIFI_SIZEOF_IPV4_BYTES);
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+CIPSTA=\"" IPSTR "\",\"" IPSTR "\",\"" IPSTR "\"\r\n",
             IP2STR(Ip_addr), IP2STR(Gateway_addr_def), IP2STR(Netmask_addr_def));
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
    if (ret == W61_STATUS_OK)
    {
      Obj->WifiCtx.DeviceConfig.DHCP_STA_IsEnabled = 0;
    }
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

_err:
  return ret;
}

W61_Status_t W61_WiFi_GetStaState(W61_Object_t *Obj, W61_WiFi_StaStateType_e *State, char *Ssid)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t tmp = 0;
  int32_t result = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(State);
  W61_NULL_ASSERT(Ssid);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWSTATE?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      Ssid[0] = '\0';
      result = sscanf((const char *)Obj->CmdResp, "+CWSTATE:%" SCNu32 ",\"%[^\"]\"\r\n", &tmp, Ssid);

      if ((result != 1) && (result != 2))
      {
        LogError("%s\n", W61_Parsing_Error_str);
        ret = W61_STATUS_ERROR;
      }
      else
      {
        *State = (W61_WiFi_StaStateType_e)tmp;
        if (!((tmp == W61_WIFI_STATE_STA_CONNECTED) || (tmp == W61_WIFI_STATE_STA_GOT_IP)))
        {
          Ssid[0] = '\0';
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetDhcpConfig(W61_Object_t *Obj, W61_WiFi_DhcpType_e *State)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t tmp = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(State);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWDHCP?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((const char *)Obj->CmdResp, "+CWDHCP:%" SCNu32 "\r\n", &tmp) != 1)
      {
        LogError("%s\n", W61_Parsing_Error_str);
      }
      else
      {
        *State = (W61_WiFi_DhcpType_e)tmp;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetDhcpConfig(W61_Object_t *Obj, W61_WiFi_DhcpType_e *State, uint32_t *Operate)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(State);
  W61_NULL_ASSERT(Operate);

  if (!((*Operate == 0) || (*Operate == 1)) || !((*State == 0) || (*State == 1) || (*State == 2) || (*State == 3)))
  {
    LogError("Incorrect parameters\n");
    return ret;
  }
  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+CWDHCP=%" PRIu32 ",%" PRIu32 "\r\n",
             *Operate, (uint32_t)*State);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret == W61_STATUS_OK)
  {
    if ((*Operate == 0) && ((*State == 1) || (*State == 3)))
    {
      Obj->WifiCtx.DeviceConfig.DHCP_STA_IsEnabled = 0;
    }
    else if ((*Operate == 1) && ((*State == 1) || (*State == 3)))
    {
      Obj->WifiCtx.DeviceConfig.DHCP_STA_IsEnabled = 1;
    }
  }

  return ret;
}

W61_Status_t W61_WiFi_GetCountryCode(W61_Object_t *Obj, uint32_t *Policy, char *CountryString)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  CountryString[0] = '\0';
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Policy);
  W61_NULL_ASSERT(CountryString);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWCOUNTRY?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((const char *)Obj->CmdResp, "+CWCOUNTRY:%" SCNu32 ",\"%[^\"]\"\r\n",
                 Policy, CountryString) != 2)
      {
        LogError("%s\n", W61_Parsing_Error_str);
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetCountryCode(W61_Object_t *Obj, uint32_t *Policy, char *CountryString)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t code = 0;
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
    LogError("Incorrect country code string\n");
    return ret;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWCOUNTRY=%" PRIu32 ",\"%s\"\r\n",
             *Policy, CountryString);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_WiFi_GetDnsAddress(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t dns1[17] = {0};
  uint8_t dns2[17] = {0};
  uint8_t dns3[17] = {0};
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CIPDNS?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+CIPDNS:%" SCNu32 ",\"%16[^\"]\",\"%16[^\"]\",\"%16[^\"]\"\r\n",
                 &Obj->WifiCtx.NetSettings.DNS_isset, dns1, dns2, dns3) != 4)
      {
        LogError("Get DNS IP failed\n");
        ret = W61_STATUS_ERROR;
      }
      else
      {
        Parser_StrToIP((char *)dns1, Obj->WifiCtx.NetSettings.DNS1);
        Parser_StrToIP((char *)dns2, Obj->WifiCtx.NetSettings.DNS2);
        Parser_StrToIP((char *)dns3, Obj->WifiCtx.NetSettings.DNS3);
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetDnsAddress(W61_Object_t *Obj, uint32_t *State, uint8_t Dns1_addr[4], uint8_t Dns2_addr[4],
                                    uint8_t Dns3_addr[4])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t pos = 0;
  uint8_t *dns_addr_table[3] = {Dns1_addr, Dns2_addr, Dns3_addr};
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(State);

  if (!((*State == 0) || (*State == 1)))
  {
    LogError("First parameter enable / disable is mandatory\n");
    return ret;
  }

  pos += snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CIPDNS=%" PRIu32, *State);
  if (*State == 1)
  {
    for (int32_t i = 0; i < 3; i++)
    {
      if ((dns_addr_table[i] == NULL) || (Parser_CheckIP(dns_addr_table[i]) != W61_STATUS_OK))
      {
        LogWarn("Dns% " PRIi32 " addr IP NULL or invalid. DNS% " PRIi32" IP will not be set\n",
                i + 1, i + 1);
      }
      else
      {
        pos += snprintf((char *)&Obj->CmdResp[pos], W61_ATD_CMDRSP_STRING_SIZE - pos,
                        ",\"" IPSTR "\"", IP2STR(dns_addr_table[i]));
      }
    }
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)&Obj->CmdResp[pos], W61_ATD_CMDRSP_STRING_SIZE - pos, "\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

/* Soft-AP related functions definition ----------------------------------------------*/
W61_Status_t W61_WiFi_SetDualMode(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t sta_state = 0;
  W61_NULL_ASSERT(Obj);

  /* Check if station is connected */
  if ((Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_CONNECTED) || (Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_GOT_IP))
  {
    sta_state = 1;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWMODE=3,%" PRIu32 "\r\n", sta_state);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret == W61_STATUS_OK)
  {
    Obj->WifiCtx.ApState = W61_WIFI_STATE_AP_RUNNING;
  }

  return ret;
}

W61_Status_t W61_WiFi_ActivateAp(W61_Object_t *Obj, W61_WiFi_ApConfig_t *ApConfig)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t tmp_rssi = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ApConfig);

  if (ApConfig->SSID[0] == '\0')
  {
    LogError("SSID cannot be NULL\n");
    return ret;
  }

  if (strlen((char *)ApConfig->SSID) > W61_WIFI_MAX_SSID_SIZE)
  {
    LogError("SSID is too long, maximum length is 32\n");
    return ret;
  }

  if (ApConfig->MaxConnections > W61_WIFI_MAX_CONNECTED_STATIONS)
  {
    LogWarn("Max connections number is too high, set to default value 4\n");
    ApConfig->MaxConnections = W61_WIFI_MAX_CONNECTED_STATIONS;
  }

  /* Get the channel the station is connected to */
  if ((Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_CONNECTED) || (Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_GOT_IP))
  {
    if (W61_WiFi_GetConnectInfo(Obj, &tmp_rssi) != W61_STATUS_OK)
    {
      LogError("Get connection information failed\n");
      return W61_STATUS_ERROR;
    }
    ApConfig->Channel = Obj->WifiCtx.STASettings.Channel;
    LogWarn("Soft-AP channel number will align on STATION's one : %" PRIu32 "\n", ApConfig->Channel);
  }
  else if ((ApConfig->Channel < 1) || (ApConfig->Channel > 13))
  {
    LogWarn("Channel value out of range, set to default value 1\n");
    ApConfig->Channel = 1;
  }

  if ((ApConfig->Security > W61_WIFI_AP_SECURITY_WPA3_PSK) || (ApConfig->Security == W61_WIFI_AP_SECURITY_WEP))
  {
    LogError("Security not supported\n");
    return ret;
  }
  else
  {
    if ((ApConfig->Security != W61_WIFI_AP_SECURITY_OPEN) &&
        ((strlen((char *)ApConfig->Password) < 8) ||
         (strlen((char *)ApConfig->Password) > W61_WIFI_MAX_PASSWORD_SIZE)))
    {
      LogError("Password length incorrect, must be in following range [8;63]\n");
      return ret;
    }

    /* Need to set the password to null if security selected is OPEN */
    if ((ApConfig->Security == W61_WIFI_AP_SECURITY_OPEN) && (strlen((char *)ApConfig->Password) > 0))
    {
      LogWarn("Password is not needed for open security, set to NULL\n");
      ApConfig->Password[0] = '\0';
    }
  }

  if (ApConfig->Hidden > 1)
  {
    LogWarn("Hidden parameter is not supported, set to default value 0\n");
    ApConfig->Hidden = 0;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+CWSAP=\"%s\",\"%s\",%" PRIu32 ",%" PRIu16 ",%" PRIu32 ",%" PRIu32 "\r\n",
             ApConfig->SSID, ApConfig->Password, ApConfig->Channel, ApConfig->Security,
             ApConfig->MaxConnections, ApConfig->Hidden);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret == W61_STATUS_OK)
  {
    LogDebug("Soft-AP configuration done\n");
  }

  return ret;
}

W61_Status_t W61_WiFi_DeactivateAp(W61_Object_t *Obj, uint8_t Reconnect)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t sta_state = 0;
  W61_NULL_ASSERT(Obj);

  /* Check if station is connected */
  if ((Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_CONNECTED) || (Obj->WifiCtx.StaState == W61_WIFI_STATE_STA_GOT_IP))
  {
    sta_state = 1;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    if (Reconnect == 0)
    {
      snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWMODE=1,0\r\n");
    }
    else
    {
      snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWMODE=1,%" PRIu32 "\r\n", sta_state);
    }
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret == W61_STATUS_OK)
  {
    Obj->WifiCtx.ApState = W61_WIFI_STATE_AP_RESET;
  }

  return ret;
}

W61_Status_t W61_WiFi_GetApConfig(W61_Object_t *Obj, W61_WiFi_ApConfig_t *ApConfig)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t tmp = 0;
  int32_t result = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ApConfig);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWSAP?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      result = sscanf((char *)Obj->CmdResp,
                      "+CWSAP:\"%32[^\"]\",\"%63[^\"]\",%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                      (char *)ApConfig->SSID, (char *)ApConfig->Password, &ApConfig->Channel, &tmp,
                      &ApConfig->MaxConnections, &ApConfig->Hidden);
      ApConfig->Security = (W61_WiFi_ApSecurityType_e)tmp;
      /* Case when the Soft-AP has the open security and does not have any password */
      if (result != 6)
      {
        result = sscanf((char *)Obj->CmdResp,
                        "+CWSAP:\"%32[^\"]\",\"\",%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                        (char *)ApConfig->SSID, &ApConfig->Channel, &tmp,
                        &ApConfig->MaxConnections, &ApConfig->Hidden);
        if (result != 5)
        {
          LogError("Get Soft-AP configuration failed\n");
          ret = W61_STATUS_ERROR;
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  memset(ApConfig->Password, 0, W61_WIFI_MAX_PASSWORD_SIZE + 1);
  return ret;
}

W61_Status_t W61_WiFi_ListConnectedSta(W61_Object_t *Obj, W61_WiFi_Connected_Sta_t *Stations)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t count = 0;  /* Number of connected stations */
  uint8_t tmp_mac[18] = {0};
  uint8_t tmp_ip[16] = {0};
  int32_t recv_len;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Stations);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWLIF\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_WIFI_TIMEOUT);
      if (recv_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }

      while ((recv_len > 0) && (strncmp((char *)Obj->CmdResp, "+CWLIF:", sizeof("+CWLIF:") - 1) == 0) &&
             (count < W61_WIFI_MAX_CONNECTED_STATIONS))
      {
        if (sscanf((char *)Obj->CmdResp, "+CWLIF:%15[^,],%17s\r\n", tmp_ip, tmp_mac) == 2)
        {
          Parser_StrToIP((char *)tmp_ip, Stations->STA[count].IP);
          Parser_StrToMAC((char *)tmp_mac, Stations->STA[count].MAC);
          count++;
        }
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_WIFI_TIMEOUT);
      }

      if (recv_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }
      ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);

      Stations->Count = count;
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_DisconnectSta(W61_Object_t *Obj, uint8_t *MAC)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(MAC);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWQIF=\"" MACSTR "\"\r\n", MAC2STR(MAC));
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetApMode(W61_Object_t *Obj, W61_WiFi_Mode_t *Mode)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  int32_t tmp = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mode);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWAPPROTO?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      ptr = strstr((char *)(Obj->CmdResp), "+CWAPPROTO:");
      if (ptr == NULL)
      {
        ret = W61_STATUS_ERROR;
      }
      else
      {
        ptr += sizeof("+CWAPPROTO:") - 1;
        token = strstr(ptr, "\r");
        if (token == NULL)
        {
          ret = W61_STATUS_ERROR;
        }
        else
        {
          *(token) = 0;
          if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
          {
            ret = W61_STATUS_ERROR;
          }
          else
          {
            Mode->byte = (uint8_t)tmp;
          }
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetApMode(W61_Object_t *Obj, W61_WiFi_Mode_t *Mode)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mode);

  if (Mode->byte > 0xF) /* 4 bits */
  {
    LogError("Invalid mode\n");
    return ret;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWAPPROTO=%" PRIu16 "\r\n", Mode->byte);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetApIpAddress(W61_Object_t *Obj, uint8_t Ip_addr[4], uint8_t Netmask_addr[4])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT_STR(Ip_addr, "Soft-AP IP NULL");
  W61_NULL_ASSERT_STR(Netmask_addr, "Soft-AP Netmask NULL");

  if (Parser_CheckIP(Ip_addr) != W61_STATUS_OK)
  {
    LogError("Soft-AP IP invalid\n");
    return ret;
  }

  if (Parser_CheckIP(Netmask_addr) != W61_STATUS_OK)
  {
    LogWarn("Netmask IP invalid. Default one will be use : 255.255.255.0\n");
    Netmask_addr[0] = 0xFF;
    Netmask_addr[1] = 0xFF;
    Netmask_addr[2] = 0xFF;
    Netmask_addr[3] = 0;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CIPAP=\"" IPSTR "\",\"" IPSTR "\",\"" IPSTR "\"\r\n",
             IP2STR(Ip_addr), IP2STR(Ip_addr), IP2STR(Netmask_addr));
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret == W61_STATUS_OK)
  {
    memcpy(Obj->WifiCtx.APSettings.IP_Addr, Ip_addr, W61_WIFI_SIZEOF_IPV4_BYTES);
  }
  return ret;
}

W61_Status_t W61_WiFi_GetApIpAddress(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  uint32_t line = 0;
  int32_t recv_len;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CIPAP?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, Obj->NcpTimeout);
      if (recv_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }

      while ((recv_len > 0) && (strncmp((char *)Obj->CmdResp, "+CIPAP:", strlen("+CIPAP:")) == 0))
      {
        Obj->CmdResp[recv_len] = 0;
        switch (line++)
        {
          case 0:
            ptr = strstr((char *)(Obj->CmdResp), "+CIPAP:ip:");
            if (ptr == NULL)
            {
              goto _err;
            }
            ptr += sizeof("+CIPAP:ip:");

            token = strstr(ptr, "\r");
            if (token == NULL)
            {
              goto _err;
            }

            *(--token) = 0;
            Parser_StrToIP(ptr, Obj->WifiCtx.APSettings.IP_Addr);
            break;
          case 1:
            ptr = strstr((char *)(Obj->CmdResp), "+CIPAP:gateway:");
            if (ptr == NULL)
            {
              goto _err;
            }
            ptr += sizeof("+CIPAP:gateway:");

            token = strstr(ptr, "\r");
            if (token == NULL)
            {
              goto _err;
            }

            *(--token) = 0;
            break;
          case 2:
            ptr = strstr((char *)(Obj->CmdResp), "+CIPAP:netmask:");
            if (ptr == NULL)
            {
              goto _err;
            }
            ptr += sizeof("+CIPAP:netmask:");

            token = strstr(ptr, "\r");
            if (token == NULL)
            {
              goto _err;
            }
            *(--token) = 0;
            Parser_StrToIP(ptr, Obj->WifiCtx.APSettings.IP_Mask);
            break;
          default:
            break;
        }
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, Obj->NcpTimeout);
      }

      if (recv_len > 0)
      {
        Obj->CmdResp[recv_len] = 0;
        ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
      }
      else
      {
        ret = W61_STATUS_TIMEOUT;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;

_err:
  W61_ATunlock(Obj);
  return W61_STATUS_ERROR;
}

W61_Status_t W61_WiFi_GetDhcpsConfig(W61_Object_t *Obj, uint32_t *lease_time, uint8_t start_ip[4], uint8_t end_ip[4])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  int32_t tmp = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(lease_time);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CWDHCPS?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      ptr = strstr((char *)(Obj->CmdResp), "+CWDHCPS:");
      if (ptr == NULL)
      {
        goto _err;
      }

      ptr += sizeof("+CWDHCPS:") - 1;
      token = strstr(ptr, ",");
      if (token == NULL)
      {
        goto _err;
      }
      *(token) = 0;
      if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
      {
        goto _err;
      }
      *lease_time = (uint32_t)tmp;

      ptr = token + 1;
      token = strstr(ptr, ",");
      if (token == NULL)
      {
        goto _err;
      }
      *(token) = 0;
      Parser_StrToIP(ptr, start_ip);

      ptr = token + 1;
      token = strstr(ptr, "\r");
      if (token == NULL)
      {
        goto _err;
      }
      *(token) = 0;
      Parser_StrToIP(ptr, end_ip);
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;

_err:
  W61_ATunlock(Obj);
  return W61_STATUS_ERROR;
}

W61_Status_t W61_WiFi_SetDhcpsConfig(W61_Object_t *Obj, uint32_t lease_time)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t StartIP[4] = {0};
  uint8_t EndIP[4] = {0};
  uint32_t Previous_lease_time = 0;
  W61_NULL_ASSERT(Obj);

  if (W61_WiFi_GetDhcpsConfig(Obj, &Previous_lease_time, StartIP, EndIP) != W61_STATUS_OK)
  {
    LogError("Get DHCP server configuration failed\n");
    return ret;
  }

  if (Parser_CheckIP(StartIP) != W61_STATUS_OK)
  {
    LogError("Start IP NULL or invalid\n");
    return ret;
  }

  if (Parser_CheckIP(EndIP) != W61_STATUS_OK)
  {
    LogError("End IP NULL or invalid\n");
    return ret;
  }

  if ((lease_time < 1) || (lease_time > 2880))
  {
    LogError("Lease time is invalid, range : [1;2880] minutes\n");
    return ret;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+CWDHCPS=1,%" PRIu32 ",\"" IPSTR "\",\"" IPSTR "\"\r\n",
             lease_time, IP2STR(StartIP), IP2STR(EndIP));
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_WIFI_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_GetApMacAddress(W61_Object_t *Obj, uint8_t *Mac)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mac);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+CIPAPMAC?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_WIFI_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      ptr = strstr((char *)(Obj->CmdResp), "+CIPAPMAC:");
      if (ptr == NULL)
      {
        ret = W61_STATUS_ERROR;
      }
      else
      {
        ptr += sizeof("+CIPAPMAC:");
        token = strstr(ptr, "\r");
        if (token == NULL)
        {
          ret = W61_STATUS_ERROR;
        }
        else
        {
          *(--token) = 0;
          Parser_StrToMAC(ptr, Mac);
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetDTIM(W61_Object_t *Obj, uint32_t dtim)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    if (dtim == 0)
    {
      snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+SLCLDTIM\r\n");
    }
    else
    {
      snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+SLWKDTIM=%" PRIu32 "\r\n", dtim);
    }

    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    if (ret == W61_STATUS_OK)
    {
      Obj->LowPowerCfg.WiFi_DTIM = dtim;
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_WiFi_GetDTIM(W61_Object_t *Obj, uint32_t *dtim)
{
  *dtim = Obj->LowPowerCfg.WiFi_DTIM;
  return W61_STATUS_OK;
}

W61_Status_t W61_WiFi_SetupTWT(W61_Object_t *Obj, W61_WiFi_TWT_Setup_Params_t *twt_params)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(twt_params);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+TWT_PARAM=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
             twt_params->setup_type, twt_params->flow_type, twt_params->wake_int_exp,
             twt_params->min_twt_wake_dur, twt_params->wake_int_mantissa);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_TeardownTWT(W61_Object_t *Obj, W61_WiFi_TWT_Teardown_Params_t *twt_params)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(twt_params);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+TWT_TEARDOWN=0,%" PRIu16 ",%" PRIu16 "\r\n",
             twt_params->all_twt, twt_params->id);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_WiFi_SetTWT(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+TWT_SLEEP\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Obj->NcpTimeout);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

/* Private Functions Definition ----------------------------------------------*/
static void W61_WiFi_AT_Event(void *hObj, const uint8_t *rxbuf, int32_t rxbuf_len)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  uint32_t temp_mac[6] = {0};
  uint32_t temp_ip[4] = {0};
  char *ptr = (char *)rxbuf;
  W61_WiFi_CbParamData_t cb_param_wifi_data;
  if (Obj == NULL)
  {
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_SCAN_DONE_KEYWORD, sizeof(W61_WIFI_EVT_SCAN_DONE_KEYWORD) - 1) == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_SCAN_DONE_ID, NULL);
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_SCAN_RESULT_KEYWORD, sizeof(W61_WIFI_EVT_SCAN_RESULT_KEYWORD) - 1) == 0)
  {
    if (Obj->WifiCtx.ScanResults.Count < W61_WIFI_MAX_DETECTED_AP)
    {
      W61_WiFi_AtParseAp((char *)rxbuf, rxbuf_len, &(Obj->WifiCtx.ScanResults));
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_CONNECTED_KEYWORD, sizeof(W61_WIFI_EVT_CONNECTED_KEYWORD) - 1) == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_CONNECTED_ID, NULL);
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_GOT_IP_KEYWORD, sizeof(W61_WIFI_EVT_GOT_IP_KEYWORD) - 1) == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_GOT_IP_ID, NULL);
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_DISCONNECTED_KEYWORD, sizeof(W61_WIFI_EVT_DISCONNECTED_KEYWORD) - 1) == 0)
  {
    /* Reset the context structure with all the information relative to the previous connection */
    memset(&Obj->WifiCtx.NetSettings, 0, sizeof(Obj->WifiCtx.NetSettings));
    memset(&Obj->WifiCtx.STASettings, 0, sizeof(Obj->WifiCtx.STASettings));
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_DISCONNECTED_ID, NULL);
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_CONNECTING_KEYWORD, sizeof(W61_WIFI_EVT_CONNECTING_KEYWORD) - 1) == 0)
  {
    if (Obj->ulcbs.UL_wifi_sta_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_CONNECTING_ID, NULL);
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_ERROR_KEYWORD, sizeof(W61_WIFI_EVT_ERROR_KEYWORD) - 1) == 0)
  {
    static int32_t reason_code = 0;

    if (Obj->ulcbs.UL_wifi_sta_cb == NULL)
    {
      return;
    }

    ptr += sizeof(W61_WIFI_EVT_ERROR_KEYWORD) - 1;

    /* Parse the error code number from +CW:ERROR,19 */
    if (Parser_StrToInt(ptr, NULL, &reason_code) == 0)
    {
      LogError("Parsing of the error code failed\n");
      return;
    }
    Obj->ulcbs.UL_wifi_sta_cb(W61_WIFI_EVT_REASON_ID, &reason_code);
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_AP_STA_CONNECTED_KEYWORD, sizeof(W61_WIFI_EVT_AP_STA_CONNECTED_KEYWORD) - 1) == 0)
  {
    ptr += sizeof(W61_WIFI_EVT_AP_STA_CONNECTED_KEYWORD);
    if (sscanf(ptr, "\"%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 "\"\r\n",
               &temp_mac[0], &temp_mac[1], &temp_mac[2], &temp_mac[3], &temp_mac[4], &temp_mac[5]) != 6)
    {
      LogError("Parsing of the MAC address failed\n");
      return;
    }

    /* Check the MAC address validity */
    for (int32_t i = 0; i < 6; i++)
    {
      if (temp_mac[i] > 0xFF)
      {
        LogError("MAC address is not valid\n");
        return;
      }
      cb_param_wifi_data.MAC[i] = (uint8_t)temp_mac[i];
    }

    if (Obj->ulcbs.UL_wifi_ap_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_ap_cb(W61_WIFI_EVT_STA_CONNECTED_ID, &cb_param_wifi_data);
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_AP_STA_DISCONNECTED_KEYWORD,
              sizeof(W61_WIFI_EVT_AP_STA_DISCONNECTED_KEYWORD) - 1) == 0)
  {
    ptr += sizeof(W61_WIFI_EVT_AP_STA_DISCONNECTED_KEYWORD);
    if (sscanf(ptr, "\"%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 "\"\r\n",
               &temp_mac[0], &temp_mac[1], &temp_mac[2], &temp_mac[3], &temp_mac[4], &temp_mac[5]) != 6)
    {
      LogError("Parsing of the MAC address failed\n");
      return;
    }

    /* Check the MAC address validity */
    for (int32_t i = 0; i < 6; i++)
    {
      if (temp_mac[i] > 0xFF)
      {
        LogError("MAC address is not valid\n");
        return;
      }
      cb_param_wifi_data.MAC[i] = (uint8_t)temp_mac[i];
    }

    if (Obj->ulcbs.UL_wifi_ap_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_ap_cb(W61_WIFI_EVT_STA_DISCONNECTED_ID, &cb_param_wifi_data);
    }
    return;
  }

  if (strncmp(ptr, W61_WIFI_EVT_AP_STA_IP_KEYWORD, sizeof(W61_WIFI_EVT_AP_STA_IP_KEYWORD) - 1) == 0)
  {
    /* String returned : +CW:DIST_STA_IP "aa:bb:cc:dd:ee:ff","192.168.4.1"*/
    if (sscanf(ptr, "+CW:DIST_STA_IP \"%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32
               "\",\"%" SCNu32 ".%" SCNu32 ".%" SCNu32 ".%" SCNu32 "\"\r\n",
               &temp_mac[0], &temp_mac[1], &temp_mac[2], &temp_mac[3], &temp_mac[4], &temp_mac[5],
               &temp_ip[0], &temp_ip[1], &temp_ip[2], &temp_ip[3]) != 10)
    {
      LogError("%s\n", W61_Parsing_Error_str);
      return;
    }

    /* Check the MAC address validity */
    for (int32_t i = 0; i < 6; i++)
    {
      if (temp_mac[i] > 0xFF)
      {
        LogError("MAC address is not valid\n");
        return;
      }
      cb_param_wifi_data.MAC[i] = (uint8_t)temp_mac[i];
    }

    /* Check the IP address validity */
    for (int32_t i = 0; i < 4; i++)
    {
      if (temp_ip[i] > 0xFF)
      {
        LogError("IP address is not valid\n");
        return;
      }
      cb_param_wifi_data.IP[i] = (uint8_t)temp_ip[i];
    }

    if (Obj->ulcbs.UL_wifi_ap_cb != NULL)
    {
      Obj->ulcbs.UL_wifi_ap_cb(W61_WIFI_EVT_DIST_STA_IP_ID, &cb_param_wifi_data);
    }
    return;
  }
}

static void W61_WiFi_AtParseAp(char *pdata, int32_t len, W61_WiFi_Scan_Result_t *APs)
{
  uint8_t num = 0;
  char buf[100] = {0};
  char *ptr;
  int32_t tmp = 0;
  W61_NULL_ASSERT_VOID(pdata);
  W61_NULL_ASSERT_VOID(APs);
  W61_NULL_ASSERT_VOID(APs->AP);

  memcpy(buf, pdata, len);

  /* Parsing the string separated by , */
  ptr = strtok(buf, ",");
  /* Looping while the ptr reach the length of the data received or there is no new token (,) to parse (end of string)*/
  while ((ptr != NULL) && (buf + len - 3 > ptr) && (num < 10))
  {
    switch (num++)
    {
      case 0:
        ptr += 8; /* sizeof("+CWLAP:(") - 1 */
        if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
        {
          LogError("Parsing of the security type failed\n");
          return;
        }
        APs->AP[APs->Count].Security = (W61_WiFi_SecurityType_e)tmp;
        break;
      case 1:
        /* Those two operations on ptr are used to remove the "" at the beginning and the end of the string */
        ptr++;
        ptr[strlen(ptr) - 1] = 0;
        strncpy((char *)APs->AP[APs->Count].SSID, ptr, W61_WIFI_MAX_SSID_SIZE);
        break;
      case 2:
        if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
        {
          LogError("Parsing of the RSSI failed\n");
          return;
        }
        APs->AP[APs->Count].RSSI = (int8_t)tmp;
        break;
      case 3:
        ptr++;
        ptr[strlen(ptr) - 1] = 0;
        Parser_StrToMAC(ptr, APs->AP[APs->Count].MAC);
        break;
      case 4:
        if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
        {
          LogError("Parsing of the channel failed\n");
          return;
        }
        APs->AP[APs->Count].Channel = (uint8_t)tmp;
        break;
      /* Cases 5 and 6 are reserved for future use (hardcoded to -1) in the NCP */
      case 5:
      case 6:
        break;
      case 7:
        if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
        {
          LogError("Parsing of the group cipher failed\n");
          return;
        }
        APs->AP[APs->Count].Group_cipher = (W61_WiFi_CipherType_e)tmp;
        break;
      case 8:
        if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
        {
          LogError("Parsing of the WiFi version failed\n");
          return;
        }
        APs->AP[APs->Count].WiFi_version.byte = (uint8_t)tmp;
        break;
      case 9:
        ptr[1] = 0; /* Remove the last ) at the end of the string */
        if (Parser_StrToInt(ptr, NULL, &tmp) == 0)
        {
          LogError("Parsing of the WPS failed\n");
          return;
        }
        APs->AP[APs->Count].WPS = (uint8_t)tmp;
        APs->Count++;
        continue;
    }
    /* When the pointer is pointing to the second to last parameter, the next token is the last ")" */
    if (num == 9)
    {
      ptr = strtok(NULL, ")");
    }
    else
    {
      ptr = strtok(NULL, ",");
    }
  }
}

/** @} */
