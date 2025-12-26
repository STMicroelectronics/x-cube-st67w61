/**
  ******************************************************************************
  * @file    w6x_wifi.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x WiFi API
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
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */
#include "common_parser.h" /* Common Parser functions */
#include "event_groups.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_WiFi_Types ST67W6X Wi-Fi Types
  * @ingroup  ST67W6X_Private_WiFi
  * @{
  */
/**
  * @brief  Internal Wi-Fi context
  */
typedef struct
{
  EventGroupHandle_t Wifi_event;            /*!< Wi-Fi event group */
  W6X_WiFi_StaStateType_e StaState;         /*!< Wi-Fi station state */
  W6X_WiFi_ApStateType_e ApState;           /*!< Wi-Fi access point state */
  uint32_t Expected_event_connect;          /*!< Expected event for connection */
  uint32_t Expected_event_gotip;            /*!< Expected event for IP address */
  uint32_t Expected_event_disconnect;       /*!< Expected event for disconnection */
  struct
  {
    uint32_t Expected_event_sta_disconnect; /*!< Expected event for station disconnection */
    uint8_t MAC[6];                         /*!< MAC address of the station */
  } evt_sta_disconnect;                     /*!< Station disconnection event structure */
} W6X_WiFiCtx_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_WiFi_Constants ST67W6X Wi-Fi Constants
  * @ingroup  ST67W6X_Private_WiFi
  * @{
  */
#define W6X_WIFI_EVENT_FLAG_CONNECT        (1<<1)    /*!< Connected event bitmask */
#define W6X_WIFI_EVENT_FLAG_GOT_IP         (1<<2)    /*!< Got IP event bitmask */
#define W6X_WIFI_EVENT_FLAG_DISCONNECT     (1<<3)    /*!< Disconnected event bitmask */
#define W6X_WIFI_EVENT_FLAG_REASON         (1<<4)    /*!< Reason event bitmask */
#define W6X_WIFI_EVENT_FLAG_STA_DISCONNECT (1<<5)    /*!< Station disconnected event bitmask */

/** Delay before to declare the connect in failure */
#define W6X_WIFI_CONNECT_TIMEOUT_MS        10000

/** Delay before to declare the WPS connect in failure */
#define W6X_WIFI_CONNECT_WPS_TIMEOUT_MS    130000

/** Delay before to declare the IP acquisition in failure */
#define W6X_WIFI_GOT_IP_TIMEOUT_MS         15000

/** Delay before to declare the disconnect in failure */
#define W6X_WIFI_DISCONNECT_TIMEOUT_MS     5000

/** Delay before to declare the station disconnect in failure */
#define W6X_WIFI_STA_DISCONNECT_TIMEOUT_MS 5000

/** @} */

/* Private macros ------------------------------------------------------------*/
/** @defgroup ST67W6X_Private_WiFi_Macros ST67W6X Wi-Fi Macros
  * @ingroup  ST67W6X_Private_WiFi
  * @{
  */
/* -------------------------------------------------------------------- */
/** Status Codes
  * When an error action is taking place the status code can indicate what the status code.
  */
#define W6X_WIFI_LIST(X) \
  X(WLAN_FW_SUCCESSFUL, 0) \
  X(WLAN_FW_TX_AUTH_FRAME_ALLOCATE_FAILURE, 1) \
  X(WLAN_FW_AUTHENTICATION_FAILURE, 2) \
  X(WLAN_FW_AUTH_ALGO_FAILURE, 3) \
  X(WLAN_FW_TX_ASSOC_FRAME_ALLOCATE_FAILURE, 4) \
  X(WLAN_FW_ASSOCIATE_FAILURE, 5) \
  X(WLAN_FW_DEAUTH_BY_AP_WHEN_NOT_CONNECTION, 6) \
  X(WLAN_FW_DEAUTH_BY_AP_WHEN_CONNECTION, 7) \
  X(WLAN_FW_4WAY_HANDSHAKE_ERROR_PSK_TIMEOUT_FAILURE, 8) \
  X(WLAN_FW_4WAY_HANDSHAKE_TX_DEAUTH_FRAME_TRANSMIT_FAILURE, 9) \
  X(WLAN_FW_4WAY_HANDSHAKE_TX_DEAUTH_FRAME_ALLOCATE_FAILURE, 10) \
  X(WLAN_FW_AUTH_OR_ASSOC_RESPONSE_TIMEOUT_FAILURE, 11) \
  X(WLAN_FW_SCAN_NO_BSSID_AND_CHANNEL, 12) \
  X(WLAN_FW_CREATE_CHANNEL_CTX_FAILURE_WHEN_JOIN_NETWORK, 13) \
  X(WLAN_FW_JOIN_NETWORK_FAILURE, 14) \
  X(WLAN_FW_ADD_STA_FAILURE, 15) \
  X(WLAN_FW_BEACON_LOSS, 16) \
  X(WLAN_FW_NETWORK_SECURITY_NOMATCH, 17) \
  X(WLAN_FW_NETWORK_WEPLEN_ERROR, 18) \
  X(WLAN_FW_DISCONNECT_BY_USER_WITH_DEAUTH, 19) \
  X(WLAN_FW_DISCONNECT_BY_USER_NO_DEAUTH, 20) \
  X(WLAN_FW_DISCONNECT_BY_FW_PS_TX_NULLFRAME_FAILURE, 21) \
  X(WLAN_FW_TRAFFIC_LOSS, 22) \
  X(WLAN_FW_SWITCH_CHANNEL_FAILURE, 23) \
  X(WLAN_FW_AUTH_OR_ASSOC_RESPONSE_CFM_FAILURE, 24) \
  X(WLAN_FW_REASSOCIATE_STARING, 25) \
  X(WLAN_FW_LAST, 26)

/** Generate enum */
#define ENUM_ENTRY(name, value) name = value,

/** Generate string array */
#define STRING_ENTRY(name, value) #name,

#ifndef HAL_SYS_RESET
/** HAL System software reset function */
extern void HAL_NVIC_SystemReset(void);
/** HAL System software reset macro */
#define HAL_SYS_RESET() do{ HAL_NVIC_SystemReset(); } while(0);
#endif /* HAL_SYS_RESET */

/** @} */

/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_WiFi_Variables ST67W6X Wi-Fi Variables
  * @ingroup  ST67W6X_Private_WiFi
  * @{
  */
static W61_Object_t *p_DrvObj = NULL; /*!< Global W61 context pointer */

/** Wi-Fi private context */
static W6X_WiFiCtx_t *p_wifi_ctx = NULL;

/** Wi-Fi security string */
static const char *const W6X_WiFi_Security_str[] =
{
  "OPEN", "WEP", "WPA", "WPA2", "WPA-WPA2", "WPA-EAP", "WPA3-SAE", "WPA2-WPA3-SAE", "UNKNOWN"
};

/** Wi-Fi state string */
static const char *const W6X_WiFi_State_str[] =
{
  "NO STARTED CONNECTION", "STA CONNECTED", "STA GOT IP", "STA CONNECTING", "STA DISCONNECTED", "STA OFF"
};

#if (W6X_ASSERT_ENABLE == 1)
/** W6X Wi-Fi init error string */
static const char W6X_WiFi_Uninit_str[] = "W6X Wi-Fi module not initialized";

/** Wi-Fi context pointer error string */
static const char W6X_WiFi_Ctx_Null_str[] = "Wi-Fi context not initialized";
#endif /* W6X_ASSERT_ENABLE */

/** Wi-Fi state error string */
typedef enum
{
  W6X_WIFI_LIST(ENUM_ENTRY)
} W6X_WiFi_Status_e;

/** Wi-Fi state string */
static const char *W6X_WiFi_Status_str[] =
{
  W6X_WIFI_LIST(STRING_ENTRY)
};

/** Wi-Fi protocol string */
static const char *const W6X_WiFi_Protocol_str[] =
{
  "Unknown", "B", "G", "N", "AX"
};

/** Wi-Fi antenna diversity string */
static const char *const W6X_WiFi_AntDiv_str[] =
{
  "Disabled", "Static", "Dynamic", "Unknown"
};

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Private_WiFi_Functions ST67W6X Wi-Fi Functions
  * @ingroup  ST67W6X_Private_WiFi
  * @{
  */
/**
  * @brief  Wi-Fi station callback function
  * @param  event_id: event ID
  * @param  event_args: event arguments
  */
static void W6X_WiFi_Station_cb(W61_event_id_t event_id, void *event_args);

/**
  * @brief  Wi-Fi Soft-AP callback function
  * @param  event_id: event ID
  * @param  event_args: event arguments
  */
static void W6X_WiFi_AP_cb(W61_event_id_t event_id, void *event_args);

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_WiFi_Public_Functions
  * @{
  */
W6X_Status_t W6X_WiFi_Init(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_App_Cb_t *p_cb_handler;
  uint32_t policy = W6X_WIFI_ADAPTIVE_COUNTRY_CODE;          /* Set the default policy */
  char *code = W6X_WIFI_COUNTRY_CODE;                        /* Set the default country code */

  /* Get the global W61 context pointer */
  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  /* Allocate the Wi-Fi context */
  p_wifi_ctx = pvPortMalloc(sizeof(W6X_WiFiCtx_t));
  if (p_wifi_ctx == NULL)
  {
    WIFI_LOG_ERROR("Could not initialize Wi-Fi context structure\n");
    goto _err;
  }
  memset(p_wifi_ctx, 0, sizeof(W6X_WiFiCtx_t));

  /* Initialize the W61 Wi-Fi module */
  ret = TranslateErrorStatus(W61_WiFi_Init(p_DrvObj));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Create the Wi-Fi event handle */
  p_wifi_ctx->Wifi_event = xEventGroupCreate();

  /* Check that application callback is registered */
  p_cb_handler = W6X_GetCbHandler();
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_wifi_cb == NULL))
  {
    WIFI_LOG_ERROR("Please register the APP callback before initializing the module\n");
    goto _err;
  }

  /* Register W61 driver callbacks */
  W61_RegisterULcb(p_DrvObj,
                   W6X_WiFi_Station_cb,
                   W6X_WiFi_AP_cb,
                   NULL,
                   NULL,
                   NULL);

  /* Configure the antenna if needed */
  if (p_DrvObj->ModuleInfo.ModuleID.ModuleID == W61_MODULE_ID_B)
  {
    W61_WiFi_AntennaMode_e mode = W61_WIFI_ANTENNA_DISABLED;
    ret = TranslateErrorStatus(W61_WiFi_GetAntennaEnable(p_DrvObj, &mode));
    if (ret != W6X_STATUS_OK)
    {
      goto _err;
    }

    if (mode != W61_WIFI_ANTENNA_DISABLED)
    {
      /* If the antenna mode is not disabled, set it to disabled */
      ret = TranslateErrorStatus(W61_WiFi_SetAntennaEnable(p_DrvObj, W61_WIFI_ANTENNA_DISABLED));
      if (ret != W6X_STATUS_OK)
      {
        goto _err;
      }
      HAL_SYS_RESET(); /* Reboot the host to apply the antenna change */
    }
  }

  /* Start the Wi-Fi as station */
  ret = W6X_WiFi_Station_Start();
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set auto connect to value defined in w6x_config.h */
  ret = TranslateErrorStatus(W61_WiFi_SetAutoConnect(p_DrvObj, W6X_WIFI_AUTOCONNECT));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set the country code */
  ret = TranslateErrorStatus(W61_WiFi_SetCountryCode(p_DrvObj, &policy, code));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set DTIM to 1 for best performance */
  if (W61_WiFi_SetDTIM(p_DrvObj, 1) == W61_STATUS_OK)
  {
    p_DrvObj->LowPowerCfg.WiFi_DTIM_Factor = 1;
    p_DrvObj->LowPowerCfg.WiFi_DTIM_Interval = 0;
  }

_err:
  return ret;
}

void W6X_WiFi_DeInit(void)
{
  if ((p_wifi_ctx == NULL) || (p_DrvObj == NULL))
  {
    return;
  }
  /* Delete the Wi-Fi event handle */
  vEventGroupDelete(p_wifi_ctx->Wifi_event);
  p_wifi_ctx->Wifi_event = NULL;

  /* Deinit the W61 Wi-Fi module */
  W61_WiFi_DeInit(p_DrvObj);

  p_DrvObj = NULL; /* Reset the global pointer */

  /* Free the Wi-Fi context */
  vPortFree(p_wifi_ctx);
  p_wifi_ctx = NULL;
}

W6X_Status_t W6X_WiFi_Scan(W6X_WiFi_Scan_Opts_t *Opts, W6X_WiFi_Scan_Result_cb_t cb)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(Opts, "Invalid scan options");
  NULL_ASSERT(cb, "Invalid callback");

  /* Set the scan callback */
  p_DrvObj->WifiCtx.scan_done_cb = (W61_WiFi_Scan_Result_cb_t)cb;

  /* Set the scan options */
  ret = TranslateErrorStatus(W61_WiFi_SetScanOpts(p_DrvObj, (W61_WiFi_Scan_Opts_t *)Opts));

  if (ret != W6X_STATUS_OK)
  {
    WIFI_LOG_ERROR("Failed to set scan options\n");
    return ret;
  }

  /* Start the scan */
  ret = TranslateErrorStatus(W61_WiFi_Scan(p_DrvObj));
  if (ret != W6X_STATUS_OK)
  {
    WIFI_LOG_ERROR("Failed to start scan\n");
  }

  /* Save the scan command status for callback */
  p_DrvObj->WifiCtx.scan_status = ret;
  return ret;
}

void W6X_WiFi_PrintScan(W6X_WiFi_Scan_Result_t *Scan_results)
{
  NULL_ASSERT_VOID(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT_VOID(Scan_results, "Invalid Scan result structure");

  if ((Scan_results == NULL) || (Scan_results->AP == NULL))
  {
    return;
  }

  if (Scan_results->Count == 0)
  {
    WIFI_LOG_INFO("No scan results\n");
  }
  else
  {
    /* Print the scan results */
    for (uint32_t count = 0; count < Scan_results->Count; count++)
    {
      /* Print the mandatory fields from the scan results */
      WIFI_LOG_INFO("MAC : [" MACSTR "] | Channel: %2" PRIu16 " | %13.13s | %4s | RSSI: %4" PRIi16 " | SSID:  %s\n",
                    MAC2STR(Scan_results->AP[count].MAC),
                    Scan_results->AP[count].Channel,
                    W6X_WiFi_SecurityToStr(Scan_results->AP[count].Security),
                    W6X_WiFi_ProtocolToStr(Scan_results->AP[count].Protocol),
                    Scan_results->AP[count].RSSI,
                    Scan_results->AP[count].SSID);
      vTaskDelay(5); /* Wait few ms to avoid logging buffer overflow */
    }
  }
}

W6X_Status_t W6X_WiFi_Connect(W6X_WiFi_Connect_Opts_t *ConnectOpts)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W61_WiFi_Connect_Opts_t *connect_opts = (W61_WiFi_Connect_Opts_t *)ConnectOpts;
  EventBits_t eventBits;
  EventBits_t eventMask = W6X_WIFI_EVENT_FLAG_CONNECT | W6X_WIFI_EVENT_FLAG_REASON;
  uint32_t connect_timeout = W6X_WIFI_CONNECT_TIMEOUT_MS;
  uint32_t ps_mode;

  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);
  NULL_ASSERT(ConnectOpts, "Invalid connect options");

  if ((p_cb_handler == NULL) || (p_cb_handler->APP_wifi_cb == NULL))
  {
    WIFI_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return ret;
  }

  /* Check possible channel conflict between STA and Soft-AP */
  if (p_DrvObj->WifiCtx.ApState == W61_WIFI_STATE_AP_RUNNING)
  {
    WIFI_LOG_WARN("In case of channel conflict, stations connected to Soft-AP "
                  "will be disconnected to switch channel\n");
  }

  /* Set DTIM to 1 for best performance */
  if (W61_WiFi_SetDTIM(p_DrvObj, 1) == W61_STATUS_OK)
  {
    p_DrvObj->LowPowerCfg.WiFi_DTIM_Factor = 1;
    p_DrvObj->LowPowerCfg.WiFi_DTIM_Interval = 1;
  }

  if (ConnectOpts->WPS)
  {
    /* Disable power save during WPS */
    if ((W6X_GetPowerMode(&ps_mode) != W6X_STATUS_OK) || (W6X_SetPowerMode(0) != W6X_STATUS_OK))
    {
      return ret;
    }
  }
  /* Start the Wi-Fi connection to the Access Point */
  ret = TranslateErrorStatus(W61_WiFi_Connect(p_DrvObj, connect_opts));
  if (ret == W6X_STATUS_OK)
  {
    p_wifi_ctx->Expected_event_connect = 1; /* Enable the expected event for connection */
    WIFI_LOG_DEBUG("NCP is treating the connection request\n");

#if (ST67_ARCH == W6X_ARCH_T01)
    /* If station is not in static IP mode, GOT_IP event is expected */
    if (p_DrvObj->NetCtx.DHCP_STA_IsEnabled == 1)
    {
      p_wifi_ctx->Expected_event_gotip = 1;
    }
#endif /* ST67_ARCH */

    /* If WPS, don't check the reason due to PSK Failure unexpected event. No impact on connection */
    if (ConnectOpts->WPS)
    {
      WIFI_LOG_DEBUG("WPS enabled, listening for 2 minutes ...\n");
      connect_timeout = W6X_WIFI_CONNECT_WPS_TIMEOUT_MS;
    }
    /* Wait for the connection to be done */
    eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, eventMask, pdTRUE,
                                    pdFALSE, pdMS_TO_TICKS(connect_timeout));

    p_wifi_ctx->Expected_event_connect = 0; /* Disable the expected event for connection */

    /* Check the event bits */
    if (eventBits & W6X_WIFI_EVENT_FLAG_CONNECT) /* If the connection is successful, the CONNECT event is expected */
    {
      /* Expected case */
    }
    else if (eventBits & W6X_WIFI_EVENT_FLAG_REASON) /* If an error occurred, the Reason event is expected */
    {
      WIFI_LOG_ERROR("Wi-Fi connect in error\n");
      /* Reset the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_DISCONNECTED;
      ret = W6X_STATUS_ERROR;
      goto _err;
    }
    else /* If the connection event is not done in time */
    {
      WIFI_LOG_ERROR("Wi-Fi connect timeouted\n");
      /* Reset the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_DISCONNECTED;
      ret = W6X_STATUS_ERROR;
      goto _err;
    }

#if (ST67_ARCH == W6X_ARCH_T01)
    /* If station is not in static IP mode, GOT_IP event is expected */
    if (p_DrvObj->NetCtx.DHCP_STA_IsEnabled == 1)
    {
      WIFI_LOG_DEBUG("DHCP client start, this may take few seconds\n");

      /* Wait for the IP address to be acquired */
      eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_GOT_IP, pdTRUE, pdFALSE,
                                      pdMS_TO_TICKS(W6X_WIFI_GOT_IP_TIMEOUT_MS));

      /* Check if Got IP is received. Skip all other possible events */
      if (eventBits & W6X_WIFI_EVENT_FLAG_GOT_IP)
      {
        /* Expected case */
      }
      else
      {
        WIFI_LOG_ERROR("Wi-Fi got IP timeouted\n");
        p_wifi_ctx->Expected_event_gotip = 0;
        ret = W6X_STATUS_ERROR;
      }
    }
#endif /* ST67_ARCH */
  }
_err:

  if (ConnectOpts->WPS)
  {
    /* Reset the power save mode*/
    if (W6X_SetPowerMode(ps_mode) != W6X_STATUS_OK)
    {
      WIFI_LOG_WARN("Failed to set back the power save mode after WPS session\n");
    }
  }

  return ret;
}

W6X_Status_t W6X_WiFi_Disconnect(uint32_t restore)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();
  EventBits_t eventBits;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  if ((p_cb_handler == NULL) || (p_cb_handler->APP_wifi_cb == NULL))
  {
    WIFI_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return ret;
  }

  /* Check if station is connected */
  if (!((p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_CONNECTED) || (p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_GOT_IP)))
  {
    WIFI_LOG_ERROR("Device is not in the appropriate state to run this command\n");
    return ret;
  }

  /* Disconnect the Wi-Fi station */
  p_wifi_ctx->Expected_event_disconnect = 1;
  ret = TranslateErrorStatus(W61_WiFi_Disconnect(p_DrvObj, restore));
  if (ret == W6X_STATUS_OK)
  {
    /* Wait for the disconnection to be done */
    eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_DISCONNECT, pdTRUE, pdFALSE,
                                    pdMS_TO_TICKS(W6X_WIFI_DISCONNECT_TIMEOUT_MS));
    if (!(eventBits & W6X_WIFI_EVENT_FLAG_DISCONNECT))
    {
      WIFI_LOG_ERROR("Wi-Fi disconnect timeouted\n");
      ret = W6X_STATUS_ERROR;
    }
  }

  p_wifi_ctx->Expected_event_disconnect = 0;
  return ret;
}

W6X_Status_t W6X_WiFi_GetAutoConnect(uint32_t *OnOff)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Get auto Connect state */
  ret = TranslateErrorStatus(W61_WiFi_GetAutoConnect(p_DrvObj, OnOff));
  if (ret == W6X_STATUS_OK)
  {
    WIFI_LOG_DEBUG("Get auto Connect state succeed\n");
  }
  return ret;
}

W6X_Status_t W6X_WiFi_GetCountryCode(uint32_t *Policy, char *CountryString)
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Get the country code */
  return TranslateErrorStatus(W61_WiFi_GetCountryCode(p_DrvObj, Policy, CountryString));
}

W6X_Status_t W6X_WiFi_SetCountryCode(uint32_t *Policy, char *CountryString)
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Set the country code */
  return TranslateErrorStatus(W61_WiFi_SetCountryCode(p_DrvObj, Policy, CountryString));
}

/* ============================================================
 * =============== Station specific APIs ======================
 * ============================================================ */
W6X_Status_t W6X_WiFi_Station_Start(void)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  /* Start the Wi-Fi as station */
  ret = TranslateErrorStatus(W61_WiFi_Station_Start(p_DrvObj));

  if (ret == W6X_STATUS_OK)
  {
    /* Set the station state */
    p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_DISCONNECTED;
    p_DrvObj->WifiCtx.StaState = W61_WIFI_STATE_STA_DISCONNECTED;
    /* Set the access point state */
    p_wifi_ctx->ApState = W6X_WIFI_STATE_AP_OFF;
    p_DrvObj->WifiCtx.ApState = W61_WIFI_STATE_AP_OFF;
  }

  return ret;
}

W6X_Status_t W6X_WiFi_Station_GetState(W6X_WiFi_StaStateType_e *State, W6X_WiFi_Connect_t *ConnectData)
{
  W6X_Status_t ret;
  int32_t rssi = 0;
  W61_WiFi_SecurityType_e security = W61_WIFI_SECURITY_UNKNOWN;
  W61_WiFi_Protocol_e protocol = W61_WIFI_PROTOCOL_UNKNOWN;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  *State = p_wifi_ctx->StaState;

  if ((ConnectData != NULL) &&
      ((p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_GOT_IP) || (p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_CONNECTED)))
  {
    /* Get the connection information if the Wi-Fi station is connected */
    ret = TranslateErrorStatus(W61_WiFi_GetConnectInfo(p_DrvObj, &rssi, &security, &protocol));
    if (ret == W6X_STATUS_OK)
    {
      memcpy(ConnectData->SSID, p_DrvObj->WifiCtx.SSID, W6X_WIFI_MAX_SSID_SIZE + 1);
      memcpy(ConnectData->MAC, p_DrvObj->WifiCtx.APSettings.MAC_Addr, 6);
      ConnectData->Rssi = rssi;
      ConnectData->Channel = p_DrvObj->WifiCtx.STASettings.Channel;
      ConnectData->Reconnection_interval = p_DrvObj->WifiCtx.STASettings.ReconnInterval;
      ConnectData->Security = (W6X_WiFi_SecurityType_e)security;
      ConnectData->Protocol = (W6X_WiFi_Protocol_e)protocol;
    }
    else
    {
      WIFI_LOG_WARN("Get connection information failed\n");
    }
  }
  else
  {
    ret = W6X_STATUS_OK;
  }
  return ret;
}

W6X_Status_t W6X_WiFi_Station_GetMACAddress(uint8_t Mac[6])
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Get the MAC address */
  return TranslateErrorStatus(W61_WiFi_Station_GetMACAddress(p_DrvObj, Mac));
}

/* ============================================================
 * =============== Soft-AP specific APIs ======================
 * ============================================================ */
W6X_Status_t W6X_WiFi_AP_Start(W6X_WiFi_ApConfig_t *ap_config)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  /* Check when switching from STA to STA+AP (as their is no reconnection of the STA) */
  if ((p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_CONNECTED) || (p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_GOT_IP))
  {
    WIFI_LOG_WARN("Ensure Station and Soft-AP does not have the same subnet IP\n");
  }

  /* Initialize the dual interface mode */
  ret = TranslateErrorStatus(W61_WiFi_SetDualMode(p_DrvObj));
  if (ret == W6X_STATUS_OK)
  {
    /* Activate the Soft-AP */
    ret = TranslateErrorStatus(W61_WiFi_AP_Start(p_DrvObj, (W61_WiFi_ApConfig_t *)ap_config));
    if (ret != W6X_STATUS_OK)
    {
      WIFI_LOG_WARN("Failed to start soft-AP, switching back to STA mode only\n");
      if (W6X_WiFi_AP_Stop() != W6X_STATUS_OK)
      {
        WIFI_LOG_WARN("Failed to switch to STA mode only, default soft-AP still started\n");
      }
    }
    else
    {
      if ((p_DrvObj->NetCtx.Supported == 0) && (p_DrvObj->Callbacks.Netif_cb.link_ap_up_fn != NULL))
      {
        p_DrvObj->Callbacks.Netif_cb.link_ap_up_fn();
      }
    }
  }
  return ret;
}

W6X_Status_t W6X_WiFi_AP_Stop(void)
{
  W6X_Status_t ret;
  W61_WiFi_Connected_Sta_t Stations = {0};
  uint8_t Reconnect = 1;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Get the connected station list to the Soft-AP interface */
  ret = TranslateErrorStatus(W61_WiFi_AP_ListConnectedStations(p_DrvObj, &Stations));

  if (ret == W6X_STATUS_OK)
  {
    if (Stations.Count != 0)
    {
      WIFI_LOG_WARN("Soft-AP is still connected to a station, they will be disconnected before stopping the Soft-AP\n");
      for (int32_t i = 0; i < Stations.Count; i++)
      {
        /* Disconnect the station from the Soft-AP interface with the MAC address */
        ret = TranslateErrorStatus(W61_WiFi_AP_DisconnectStation(p_DrvObj, Stations.STA[i].MAC));
        if (ret != W6X_STATUS_OK)
        {
          WIFI_LOG_ERROR("Failed to disconnect station");
          goto _err;
        }
      }
    }
  }

  /* Deactivate the Soft-AP */
  ret = TranslateErrorStatus(W61_WiFi_AP_Stop(p_DrvObj, Reconnect));
  if ((p_DrvObj->NetCtx.Supported == 0) && (p_DrvObj->Callbacks.Netif_cb.link_ap_down_fn != NULL))
  {
    p_DrvObj->Callbacks.Netif_cb.link_ap_down_fn();
  }

_err:
  return ret;
}

W6X_Status_t W6X_WiFi_AP_GetConfig(W6X_WiFi_ApConfig_t *ap_config)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  if (p_DrvObj->WifiCtx.ApState != W61_WIFI_STATE_AP_RUNNING)
  {
    WIFI_LOG_ERROR("Soft-AP is not started\n");
    return ret;
  }

  /* Get the Soft-AP configuration */
  return TranslateErrorStatus(W61_WiFi_AP_GetConfig(p_DrvObj, (W61_WiFi_ApConfig_t *)ap_config));
}

W6X_Status_t W6X_WiFi_AP_ListConnectedStations(W6X_WiFi_Connected_Sta_t *ConnectedSta)
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Get the connected station list */
  return TranslateErrorStatus(W61_WiFi_AP_ListConnectedStations(p_DrvObj, (W61_WiFi_Connected_Sta_t *)ConnectedSta));
}

W6X_Status_t W6X_WiFi_AP_DisconnectStation(uint8_t MAC[6])
{
  W6X_Status_t ret;
  EventBits_t eventBits;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  p_wifi_ctx->evt_sta_disconnect.Expected_event_sta_disconnect = 1;
  /* Assign the MAC address to the event structure */
  for (int32_t i = 0; i < 6; i++)
  {
    p_wifi_ctx->evt_sta_disconnect.MAC[i] = MAC[i];
  }

  /* Disconnect the station */
  ret = TranslateErrorStatus(W61_WiFi_AP_DisconnectStation(p_DrvObj, MAC));
  if (ret == W6X_STATUS_OK)
  {
    /* Wait for the disconnection to be done */
    eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_STA_DISCONNECT, pdTRUE, pdFALSE,
                                    pdMS_TO_TICKS(W6X_WIFI_STA_DISCONNECT_TIMEOUT_MS));
    if (!(eventBits & W6X_WIFI_EVENT_FLAG_STA_DISCONNECT))
    {
      WIFI_LOG_ERROR("Wi-Fi station disconnect timeouted\n");
      ret = W6X_STATUS_ERROR;
    }
    else
    {
      WIFI_LOG_DEBUG("Wi-Fi station disconnected successfully\n");
    }
  }
  p_wifi_ctx->evt_sta_disconnect.Expected_event_sta_disconnect = 0;
  return ret;
}

W6X_Status_t W6X_WiFi_AP_GetMACAddress(uint8_t Mac[6])
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Get the MAC address */
  return TranslateErrorStatus(W61_WiFi_AP_GetMACAddress(p_DrvObj, Mac));
}

/* ============================================================
 * =============== Low-Power specific APIs ====================
 * ============================================================ */
W6X_Status_t W6X_WiFi_SetDTIM(uint32_t dtim_factor)
{
  uint32_t dtim_ap = 0;
  uint32_t current_dtim = dtim_factor;
  uint32_t ps_mode = 0;
  int32_t try = 30; /* Try up to 3 seconds to get the DTIM from the Soft-AP */
  W61_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  if ((p_DrvObj->WifiCtx.StaState != W61_WIFI_STATE_STA_CONNECTED) &&
      (p_DrvObj->WifiCtx.StaState != W61_WIFI_STATE_STA_GOT_IP))
  {
    WIFI_LOG_WARN("Device is not in the appropriate state to run this command\n");
    return W6X_STATUS_ERROR;
  }

  if (W61_GetPowerMode(p_DrvObj, &ps_mode) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }

  if (W61_SdkMinVersion(p_DrvObj, 2, 0, 97) != W61_STATUS_OK)
  {
    /* Get the current DTIM period for Soft-AP */
    do
    {
      vTaskDelay(pdMS_TO_TICKS(100));
      (void)W61_WiFi_GetDTIM_AP(p_DrvObj, &dtim_ap);
    } while ((try--) && (dtim_ap == 0));

    if (dtim_ap == 0)
    {
      return W6X_STATUS_ERROR;
    }

    current_dtim = dtim_factor * dtim_ap;
  }

  ret = W61_WiFi_SetDTIM(p_DrvObj, current_dtim);
  if (ret == W61_STATUS_OK)
  {
    p_DrvObj->LowPowerCfg.WiFi_DTIM_Factor = dtim_factor;
    /* Below code is only valid for SDK 2.0.89 */
    p_DrvObj->LowPowerCfg.WiFi_DTIM_Interval = current_dtim;
    if (ps_mode == 0)
    {
      WIFI_LOG_WARN("Device is not in power save mode, DTIM configuration"
                    " will be applied when changing to power save mode.\n");
    }
  }
  return TranslateErrorStatus(ret);
}

W6X_Status_t W6X_WiFi_GetDTIM(uint32_t *dtim_factor, uint32_t *dtim_interval)
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  *dtim_factor = p_DrvObj->LowPowerCfg.WiFi_DTIM_Factor;

  if ((p_DrvObj->WifiCtx.StaState != W61_WIFI_STATE_STA_CONNECTED) &&
      (p_DrvObj->WifiCtx.StaState != W61_WIFI_STATE_STA_GOT_IP))
  {
    *dtim_interval = 0;
    return W6X_STATUS_OK;
  }

  if (W61_SdkMinVersion(p_DrvObj, 2, 0, 97) == W61_STATUS_OK)
  {
    uint32_t dtim_ap = 0;
    int32_t try = 30; /* Try up to 3 seconds to get the DTIM from the Soft-AP */
    W61_Status_t ret;

    do
    {
      vTaskDelay(pdMS_TO_TICKS(100));
      ret = W61_WiFi_GetDTIM_AP(p_DrvObj, &dtim_ap);
    } while ((try--) && (ret == W61_STATUS_ERROR));

    if (dtim_ap == 0)
    {
      return W6X_STATUS_ERROR;
    }

    /* In the case the dtim_factor is null, the device will wake up every beacon */
    if (*dtim_factor == 0)
    {
      *dtim_interval = 1;
    }
    else
    {
      *dtim_interval = dtim_ap * (*dtim_factor);
    }
  }
  else
  {
    *dtim_interval = p_DrvObj->LowPowerCfg.WiFi_DTIM_Interval;
  }

  return W6X_STATUS_OK;
}

W6X_Status_t W6X_WiFi_GetDTIM_AP(uint32_t *dtim)
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  if ((p_DrvObj->WifiCtx.StaState != W61_WIFI_STATE_STA_CONNECTED) &&
      (p_DrvObj->WifiCtx.StaState != W61_WIFI_STATE_STA_GOT_IP))
  {
    *dtim = 0;
    return W6X_STATUS_ERROR;
  }
  /* Get the DTIM */
  return TranslateErrorStatus(W61_WiFi_GetDTIM_AP(p_DrvObj, dtim));
}

W6X_Status_t W6X_WiFi_TWT_Setup(W6X_WiFi_TWT_Setup_Params_t *twt_params)
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  uint32_t is_supported = 0;
  uint32_t ps_mode = 0;
  W61_WiFi_TWT_Status_t twt_status = {0};

  /* Verify TWT is supported by the AP */
  if ((W61_WiFi_TWT_IsSupported(p_DrvObj, &is_supported) != W61_STATUS_OK) || (is_supported == 0))
  {
    return W6X_STATUS_ERROR;
  }

  /* TWT multi flow is not supported in current version */
  if (W61_WiFi_TWT_GetStatus(p_DrvObj, &twt_status) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }
  if (twt_status.flow_count == 1)
  {
    WIFI_LOG_ERROR("Only one TWT flow setup is supported\n");
    return W6X_STATUS_ERROR;
  }

  if (W61_GetPowerMode(p_DrvObj, &ps_mode) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }

  if (ps_mode == 0)
  {
    WIFI_LOG_WARN("Device is not in the appropriate power save mode\n");
  }

  /* Setup TWT */
  return TranslateErrorStatus(W61_WiFi_TWT_Setup(p_DrvObj, (W61_WiFi_TWT_Setup_Params_t *)twt_params));
}

W6X_Status_t W6X_WiFi_TWT_GetStatus(W6X_WiFi_TWT_Status_t *twt_status)
{
  uint32_t is_supported = 0;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Verify TWT is supported by the AP */
  if (W61_WiFi_TWT_IsSupported(p_DrvObj, &is_supported) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }
  if (is_supported == 0)
  {
    twt_status->is_supported = 0;
    return W6X_STATUS_OK;
  }
  twt_status->is_supported = 1;

  /* Get TWT status */
  return TranslateErrorStatus(W61_WiFi_TWT_GetStatus(p_DrvObj, (W61_WiFi_TWT_Status_t *)twt_status));
}

W6X_Status_t W6X_WiFi_TWT_Teardown(W6X_WiFi_TWT_Teardown_Params_t *twt_params)
{
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  W61_WiFi_TWT_Status_t twt_status = {0};

  /* Check if a TWT flow is currently active */
  if ((W61_WiFi_TWT_GetStatus(p_DrvObj, &twt_status) != W61_STATUS_OK) || (twt_status.flow_count == 0))
  {
    return W6X_STATUS_ERROR;
  }
  /* Teardown TWT */
  return TranslateErrorStatus(W61_WiFi_TWT_Teardown(p_DrvObj, (W61_WiFi_TWT_Teardown_Params_t *)twt_params));
}

W6X_Status_t W6X_WiFi_GetAntennaDiversity(W6X_WiFi_AntennaInfo_t *antenna_info)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(antenna_info, "Antenna info pointer is NULL");

  /* Get the antenna information */
  ret = TranslateErrorStatus(W61_WiFi_GetAntennaEnable(p_DrvObj, (W61_WiFi_AntennaMode_e *)&antenna_info->mode));
  if (ret != W6X_STATUS_OK)
  {
    return ret;
  }
  ret = TranslateErrorStatus(W61_WiFi_GetAntennaUsed(p_DrvObj, &antenna_info->antenna_id));
  if (ret != W6X_STATUS_OK)
  {
    return ret;
  }

  return W6X_STATUS_OK;
}

W6X_Status_t W6X_WiFi_SetAntennaDiversity(W6X_WiFi_AntennaMode_e mode)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_WiFi_Uninit_str);

  /* Set the antenna configuration */
  ret = TranslateErrorStatus(W61_WiFi_SetAntennaEnable(p_DrvObj, (W61_WiFi_AntennaMode_e)mode));
  if (ret != W6X_STATUS_OK)
  {
    return ret;
  }

  HAL_SYS_RESET();  /* Reset the system to apply the new antenna configuration */
  return W6X_STATUS_OK;
}

/* ============================================================
 * =============== Display specific APIs ======================
 * ============================================================ */
const char *W6X_WiFi_StateToStr(uint32_t state)
{
  /* Check if the state is unknown */
  if (state > W6X_WIFI_STATE_STA_OFF)
  {
    return "Unknown";
  }
  /* Return the state string */
  return W6X_WiFi_State_str[state];
}

const char *W6X_WiFi_SecurityToStr(uint32_t security)
{
  /* Check if the security is unknown */
  if (security > W6X_WIFI_SECURITY_UNKNOWN)
  {
    return "Unknown";
  }
  /* Return the security string */
  return W6X_WiFi_Security_str[security];
}

const char *W6X_WiFi_ReasonToStr(void *reason)
{
  /* Check if the reason is unknown */
  if (*(uint32_t *)reason >= WLAN_FW_LAST)
  {
    return "Unknown";
  }
  /* Return the reason string */
  return W6X_WiFi_Status_str[*(uint32_t *)reason];
}

const char *W6X_WiFi_ProtocolToStr(W6X_WiFi_Protocol_e rev)
{
  /* Check if the protocol is unknown */
  if (rev > W6X_WIFI_PROTOCOL_11AX)
  {
    return W6X_WiFi_Protocol_str[W6X_WIFI_PROTOCOL_UNKNOWN];
  }
  /* Return the protocol string */
  return W6X_WiFi_Protocol_str[rev];
}

const char *W6X_WiFi_AntDivToStr(W6X_WiFi_AntennaMode_e mode)
{
  /* Check if the mode is unknown */
  if (mode > W6X_WIFI_ANTENNA_UNKNOWN)
  {
    return W6X_WiFi_AntDiv_str[W6X_WIFI_ANTENNA_DISABLED];
  }
  /* Return the mode string */
  return W6X_WiFi_AntDiv_str[mode];
}

/** @} */

/* Private Functions Definition ----------------------------------------------*/
/* =================== Callbacks ===================================*/
/** @addtogroup ST67W6X_Private_WiFi_Functions
  * @{
  */
static void W6X_WiFi_Station_cb(W61_event_id_t event_id, void *event_args)
{
  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();
  uint32_t reason_code;
  NULL_ASSERT_VOID(p_DrvObj, W6X_WiFi_Uninit_str);
  NULL_ASSERT_VOID(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  if ((p_cb_handler == NULL) || (p_cb_handler->APP_wifi_cb == NULL))
  {
    WIFI_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return;
  }

  switch (event_id) /* Check the event ID and call the appropriate callback */
  {
    case W61_WIFI_EVT_SCAN_DONE_ID:
      /* Call the scan done callback */
      p_DrvObj->WifiCtx.scan_done_cb(p_DrvObj->WifiCtx.scan_status, &p_DrvObj->WifiCtx.ScanResults);
      break;

    case W61_WIFI_EVT_CONNECTED_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_CONNECTED;
      p_DrvObj->WifiCtx.StaState = W61_WIFI_STATE_STA_CONNECTED;

      /* Delay added to avoid any AT command to be sent to soon after the CONNECTED event.
       * This could create unexpected behaviors (wrong value returned, command not responsive, ...) */
      vTaskDelay(pdMS_TO_TICKS(100));

      if (p_wifi_ctx->Expected_event_connect == 1)
      {
        /* If the connected event was expected, set the event bit to release the wait */
        xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_CONNECT);
      }

      if ((p_DrvObj->NetCtx.Supported == 0) && (p_DrvObj->Callbacks.Netif_cb.link_sta_up_fn != NULL))
      {
        p_DrvObj->Callbacks.Netif_cb.link_sta_up_fn();
      }

      /* Call the application callback to inform that the station is connected */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_CONNECTED_ID, NULL);

      break;

#if (ST67_ARCH == W6X_ARCH_T01)
    case W61_WIFI_EVT_GOT_IP_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_GOT_IP;
      p_DrvObj->WifiCtx.StaState = W61_WIFI_STATE_STA_GOT_IP;

      /* Call the application callback to inform that the station got an IP */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_GOT_IP_ID, (void *)NULL);

      if (p_wifi_ctx->Expected_event_gotip == 1)
      {
        /* If the IP address event was expected, set the event bit to release the wait */
        xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_GOT_IP);
        p_wifi_ctx->Expected_event_gotip = 0;
      }

      break;
#endif /* ST67_ARCH */

    case W61_WIFI_EVT_DISCONNECTED_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_DISCONNECTED;
      p_DrvObj->WifiCtx.StaState = W61_WIFI_STATE_STA_DISCONNECTED;
      p_DrvObj->LowPowerCfg.WiFi_DTIM_Interval = 0;
      p_DrvObj->LowPowerCfg.WiFi_DTIM_Factor = 1;

      if (p_wifi_ctx->Expected_event_disconnect == 1)
      {
        /* If the disconnected event was expected, set the event bit to release the wait */
        xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_DISCONNECT);
        p_wifi_ctx->Expected_event_disconnect = 0;
      }

      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_DISCONNECTED_ID, NULL);

      if ((p_DrvObj->NetCtx.Supported == 0) && (p_DrvObj->Callbacks.Netif_cb.link_sta_down_fn != NULL))
      {
        p_DrvObj->Callbacks.Netif_cb.link_sta_down_fn();
      }

      break;

    case W61_WIFI_EVT_CONNECTING_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_CONNECTING;
      p_DrvObj->WifiCtx.StaState = W61_WIFI_STATE_STA_CONNECTING;

      /* Call the application callback to inform that the station is connecting */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_CONNECTING_ID, (void *)NULL);
      break;

    case W61_WIFI_EVT_REASON_ID:
      reason_code = *(uint32_t *)event_args;
      if (p_wifi_ctx->Expected_event_connect == 1)
      {
        /* If the error event was expected, set the event bit to release the wait */
        xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_REASON);
      }

      /* Call the application callback to inform that an error occurred */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_REASON_ID, (void *)&reason_code);

      break;

    default:
      break;
  }
}

static void W6X_WiFi_AP_cb(W61_event_id_t event_id, void *event_args)
{
  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();
  NULL_ASSERT_VOID(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  if ((p_cb_handler == NULL) || (p_cb_handler->APP_wifi_cb == NULL))
  {
    WIFI_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return;
  }

  switch (event_id) /* Check the event ID and call the appropriate callback */
  {
    case W61_WIFI_EVT_STA_CONNECTED_ID:
      /* Call the application callback to inform that a station is connected to the Soft-AP */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_STA_CONNECTED_ID, event_args);
      break;

    case W61_WIFI_EVT_STA_DISCONNECTED_ID:
      if ((p_wifi_ctx->evt_sta_disconnect.Expected_event_sta_disconnect == 1) &&
          (memcmp(p_wifi_ctx->evt_sta_disconnect.MAC,
                  ((W61_WiFi_CbParamData_t *)event_args)->MAC, 6) == 0))
      {
        /* If the disconnected event was expected, set the event bit to release the wait */
        xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_STA_DISCONNECT);
        p_wifi_ctx->evt_sta_disconnect.Expected_event_sta_disconnect = 0;
      }
      /* Call the application callback to inform that a station is disconnected from the Soft-AP */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_STA_DISCONNECTED_ID, event_args);
      break;

    case W61_WIFI_EVT_DIST_STA_IP_ID:
      /* Call the application callback to inform that a station has an IP address */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_DIST_STA_IP_ID, event_args);
      break;

    default:
      break;
  }
}

/** @} */
