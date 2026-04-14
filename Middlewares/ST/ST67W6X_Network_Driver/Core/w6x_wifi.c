/**
  ******************************************************************************
  * @file    w6x_wifi.c
  * @author  ST67 Application Team
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
#include "w61_at_common.h"

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
#define W6X_WIFI_EVENT_FLAG_CONNECT        (1UL << 1U)    /*!< Connected event bitmask */
#define W6X_WIFI_EVENT_FLAG_GOT_IP         (1UL << 2U)    /*!< Got IP event bitmask */
#define W6X_WIFI_EVENT_FLAG_DISCONNECT     (1UL << 3U)    /*!< Disconnected event bitmask */
#define W6X_WIFI_EVENT_FLAG_REASON         (1UL << 4U)    /*!< Reason event bitmask */
#define W6X_WIFI_EVENT_FLAG_STA_DISCONNECT (1UL << 5U)    /*!< Station disconnected event bitmask */

/** Delay before to declare the connect in failure */
#define W6X_WIFI_CONNECT_TIMEOUT_MS        10000U

/** Delay before to declare the WPS connect in failure */
#define W6X_WIFI_CONNECT_WPS_TIMEOUT_MS    130000U

/** Delay before to declare the IP acquisition in failure */
#define W6X_WIFI_GOT_IP_TIMEOUT_MS         15000U

/** Delay before to declare the disconnect in failure */
#define W6X_WIFI_DISCONNECT_TIMEOUT_MS     5000U

/** Delay before to declare the station disconnect in failure */
#define W6X_WIFI_STA_DISCONNECT_TIMEOUT_MS 5000U

/** @} */

/* Private macros ------------------------------------------------------------*/
/** @defgroup ST67W6X_Private_WiFi_Macros ST67W6X Wi-Fi Macros
  * @ingroup  ST67W6X_Private_WiFi
  * @{
  */
/* -------------------------------------------------------------------- */
/** Generate string array */
#define STRING_ENTRY(name, value) #name,

#ifndef HAL_SYS_RESET
/** HAL System software reset function */
extern void HAL_NVIC_SystemReset(void);
/** HAL System software reset macro */
#define HAL_SYS_RESET() do{ HAL_NVIC_SystemReset(); } while(false);
#endif /* HAL_SYS_RESET */

/** @} */

/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_WiFi_Variables ST67W6X Wi-Fi Variables
  * @ingroup  ST67W6X_Private_WiFi
  * @{
  */
static W61_Object_t *W6X_WiFi_drv_obj = NULL; /*!< Global W61 context pointer */

/** Wi-Fi private context */
static W6X_WiFiCtx_t *p_wifi_ctx = NULL;

/** Wi-Fi security string */
static const char *const W6X_WiFi_Security_str[] =
{
  "OPEN", "WEP", "WPA", "WPA2", "WPA-WPA2", "WPA-EAP", "WPA3-SAE", "WPA2-WPA3-SAE", "UNKNOWN"
};

/** Wi-Fi AP security string */
static const char *const W6X_WiFi_AP_Security_str[] =
{
  "OPEN", "WEP", "WPA", "WPA2", "WPA3", "UNKNOWN"
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
  W6X_WiFi_drv_obj = W61_ObjGet();
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_Obj_Null_str);

  if (W6X_WiFi_drv_obj->ResetCfg.WiFi_status == W61_MODULE_STATE_INIT)
  {
    WIFI_LOG_DEBUG("Wi-Fi module already initialized\n");
    return W6X_STATUS_OK; /* Wi-Fi already initialized, nothing to do */
  }

  /* Allocate the Wi-Fi context */
  p_wifi_ctx = pvPortMalloc(sizeof(W6X_WiFiCtx_t));
  if (p_wifi_ctx == NULL)
  {
    WIFI_LOG_ERROR("Could not initialize Wi-Fi context structure\n");
    goto _err;
  }
  (void)memset(p_wifi_ctx, 0, sizeof(W6X_WiFiCtx_t));

  /* Initialize the W61 Wi-Fi module */
  ret = TranslateErrorStatus(W61_WiFi_Init(W6X_WiFi_drv_obj));
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
  (void)W61_RegisterULcb(W6X_WiFi_drv_obj,
                         W6X_WiFi_Station_cb,
                         W6X_WiFi_AP_cb,
                         NULL,
                         NULL,
                         NULL);

  /* Configure the antenna if needed */
  if (W6X_WiFi_drv_obj->ModuleInfo.ModuleID.ModuleID == W61_MODULE_ID_B)
  {
    W61_WiFi_AntennaMode_e mode = W61_WIFI_ANTENNA_DISABLED;
    ret = TranslateErrorStatus(W61_WiFi_GetAntennaEnable(W6X_WiFi_drv_obj, &mode));
    if (ret != W6X_STATUS_OK)
    {
      goto _err;
    }

    if (mode != W61_WIFI_ANTENNA_DISABLED)
    {
      /* If the antenna mode is not disabled, set it to disabled */
      ret = TranslateErrorStatus(W61_WiFi_SetAntennaEnable(W6X_WiFi_drv_obj, W61_WIFI_ANTENNA_DISABLED));
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
  ret = TranslateErrorStatus(W61_WiFi_SetAutoConnect(W6X_WiFi_drv_obj, W6X_WIFI_AUTOCONNECT));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set the country code */
  ret = TranslateErrorStatus(W61_WiFi_SetCountryCode(W6X_WiFi_drv_obj, &policy, code));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set DTIM to 1 for best performance */
  if (W61_WiFi_SetDTIM(W6X_WiFi_drv_obj, 1) == W61_STATUS_OK)
  {
    W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Factor = 1;
    W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Interval = 0;
  }

  W6X_WiFi_drv_obj->ResetCfg.WiFi_status = W61_MODULE_STATE_INIT; /* Update Wi-Fi state */
  return W6X_STATUS_OK;

_err:
  W6X_WiFi_DeInit(); /* Deinitialize Wi-Fi in case of failure */
  return ret;
}

void W6X_WiFi_DeInit(void)
{
  if ((p_wifi_ctx == NULL) || (W6X_WiFi_drv_obj == NULL))
  {
    return;
  }

  (void)W61_WiFi_Stop(W6X_WiFi_drv_obj);

  /* Delete the Wi-Fi event handle */
  vEventGroupDelete(p_wifi_ctx->Wifi_event);
  p_wifi_ctx->Wifi_event = NULL;

  /* Deinit the W61 Wi-Fi module */
  (void)W61_WiFi_DeInit(W6X_WiFi_drv_obj);

  W6X_WiFi_drv_obj->ResetCfg.WiFi_status = W61_MODULE_STATE_NOT_INIT; /* Update Wi-Fi state */
  W6X_WiFi_drv_obj = NULL; /* Reset the global pointer */

  /* Free the Wi-Fi context */
  vPortFree(p_wifi_ctx);
  p_wifi_ctx = NULL;
}

W6X_Status_t W6X_WiFi_Scan(W6X_WiFi_Scan_Opts_t *opts, W6X_WiFi_Scan_Result_cb_t scan_result_cb)
{
  W6X_Status_t ret;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(opts, "Invalid scan options");
  NULL_ASSERT(scan_result_cb, "Invalid callback");

  /* Set the scan callback */
  W6X_WiFi_drv_obj->WifiCtx.scan_done_cb = (W61_WiFi_Scan_Result_cb_t)scan_result_cb;

  /* Set the scan options */
  ret = TranslateErrorStatus(W61_WiFi_SetScanOpts(W6X_WiFi_drv_obj, (W61_WiFi_Scan_Opts_t *)opts));

  if (ret != W6X_STATUS_OK)
  {
    WIFI_LOG_ERROR("Failed to set scan options\n");
    return ret;
  }

  /* Start the scan */
  ret = TranslateErrorStatus(W61_WiFi_Scan(W6X_WiFi_drv_obj));
  if (ret != W6X_STATUS_OK)
  {
    WIFI_LOG_ERROR("Failed to start scan\n");
  }

  /* Save the scan command status for callback */
  W6X_WiFi_drv_obj->WifiCtx.scan_status = ret;
  return ret;
}

void W6X_WiFi_PrintScan(W6X_WiFi_Scan_Result_t *results)
{
  NULL_ASSERT_VOID(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT_VOID(results, "Invalid Scan result structure");

  if ((results == NULL) || (results->AP == NULL))
  {
    return;
  }

  if (results->Count == 0U)
  {
    WIFI_LOG_INFO("No scan results\n");
  }
  else
  {
    /* Print the scan results */
    for (uint32_t count = 0U; count < results->Count; count++)
    {
      /* Print the mandatory fields from the scan results */
      WIFI_LOG_INFO("MAC : [" MACSTR "] | Channel: %2" PRIu16 " | %13.13s | %4s | RSSI: %4" PRIi16 " | SSID:  %s\n",
                    MAC2STR(results->AP[count].MAC),
                    results->AP[count].Channel,
                    W6X_WiFi_SecurityToStr(results->AP[count].Security),
                    W6X_WiFi_ProtocolToStr(results->AP[count].Protocol),
                    results->AP[count].RSSI,
                    results->AP[count].SSID);
      vTaskDelay(pdMS_TO_TICKS(5)); /* Wait few ms to avoid logging buffer overflow */
    }
  }
}

W6X_Status_t W6X_WiFi_Connect(W6X_WiFi_Connect_Opts_t *connect_opts)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  EventBits_t eventBits;
  EventBits_t eventMask = W6X_WIFI_EVENT_FLAG_CONNECT | W6X_WIFI_EVENT_FLAG_REASON;
  uint32_t connect_timeout = W6X_WIFI_CONNECT_TIMEOUT_MS;
  uint32_t ps_mode;

  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);
  NULL_ASSERT(ConnectOpts, "Invalid connect options");

  if ((p_cb_handler == NULL) || (p_cb_handler->APP_wifi_cb == NULL))
  {
    WIFI_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return ret;
  }

  /* Check possible channel conflict between STA and Soft-AP */
  if (W6X_WiFi_drv_obj->WifiCtx.ApState == W61_WIFI_STATE_AP_RUNNING)
  {
    WIFI_LOG_WARN("In case of channel conflict, stations connected to Soft-AP "
                  "will be disconnected to switch channel\n");

    if ((W6X_WiFi_drv_obj->NetCtx.Supported == 0U) && (W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_down_fn != NULL))
    {
      (void)W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_down_fn();
    }
  }

  /* Set DTIM to 1 for best performance */
  if (W61_WiFi_SetDTIM(W6X_WiFi_drv_obj, 1) == W61_STATUS_OK)
  {
    W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Factor = 1U;
    W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Interval = 1U;
  }

  if (connect_opts->WPS == 1U)
  {
    /* Disable power save during WPS */
    if ((W6X_GetPowerMode(&ps_mode) != W6X_STATUS_OK) || (W6X_SetPowerMode(0) != W6X_STATUS_OK))
    {
      if ((W6X_WiFi_drv_obj->NetCtx.Supported == 0U) && (W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_up_fn != NULL))
      {
        if (W6X_WiFi_drv_obj->WifiCtx.ApState == W61_WIFI_STATE_AP_RUNNING) /* Restart AP */
        {
          WIFI_LOG_WARN("RESTART AP\n");
          (void)W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_up_fn();
        }
      }
      return ret;
    }
  }
  /* Start the Wi-Fi connection to the Access Point */
  ret = TranslateErrorStatus(W61_WiFi_Connect(W6X_WiFi_drv_obj, (W61_WiFi_Connect_Opts_t *)connect_opts));
  if (ret == W6X_STATUS_OK)
  {
    p_wifi_ctx->Expected_event_connect = 1U; /* Enable the expected event for connection */
    WIFI_LOG_DEBUG("NCP is treating the connection request\n");

#if (ST67_ARCH == W6X_ARCH_T01)
    /* If station is not in static IP mode, GOT_IP event is expected */
    if (W6X_WiFi_drv_obj->NetCtx.DHCP_STA_IsEnabled == 1U)
    {
      p_wifi_ctx->Expected_event_gotip = 1U;
    }
#endif /* ST67_ARCH */

    /* If WPS, don't check the reason due to PSK Failure unexpected event. No impact on connection */
    if (connect_opts->WPS == 1U)
    {
      WIFI_LOG_DEBUG("WPS enabled, listening for 2 minutes ...\n");
      connect_timeout = W6X_WIFI_CONNECT_WPS_TIMEOUT_MS;
    }
    /* Wait for the connection to be done */
    eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, eventMask, pdTRUE,
                                    pdFALSE, pdMS_TO_TICKS(connect_timeout));

    p_wifi_ctx->Expected_event_connect = 0U; /* Disable the expected event for connection */

    /* Check the event bits */
    /* If the connection is successful, the CONNECT event is expected */
    if ((eventBits & W6X_WIFI_EVENT_FLAG_CONNECT) != 0U)
    {
      /* Expected case */
    }
    else if ((eventBits & W6X_WIFI_EVENT_FLAG_REASON) != 0U) /* If an error occurred, the Reason event is expected */
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
    if (W6X_WiFi_drv_obj->NetCtx.DHCP_STA_IsEnabled == 1U)
    {
      WIFI_LOG_DEBUG("DHCP client start, this may take few seconds\n");

      /* Wait for the IP address to be acquired */
      eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_GOT_IP, pdTRUE, pdFALSE,
                                      pdMS_TO_TICKS(W6X_WIFI_GOT_IP_TIMEOUT_MS));

      /* Check if Got IP is received. Skip all other possible events */
      if ((eventBits & W6X_WIFI_EVENT_FLAG_GOT_IP) != 0U)
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
  if ((W6X_WiFi_drv_obj->NetCtx.Supported == 0U) && (W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_up_fn != NULL))
  {
    if (W6X_WiFi_drv_obj->WifiCtx.ApState == W61_WIFI_STATE_AP_RUNNING) /* RESTART AP */
    {
      WIFI_LOG_WARN("RESTART AP\n");
      (void)W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_up_fn();
    }
  }
  if (connect_opts->WPS == 1U)
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
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
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
  ret = TranslateErrorStatus(W61_WiFi_Disconnect(W6X_WiFi_drv_obj, restore));
  if (ret == W6X_STATUS_OK)
  {
    /* Wait for the disconnection to be done */
    eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_DISCONNECT, pdTRUE, pdFALSE,
                                    pdMS_TO_TICKS(W6X_WIFI_DISCONNECT_TIMEOUT_MS));
    if ((eventBits & W6X_WIFI_EVENT_FLAG_DISCONNECT) == 0U)
    {
      WIFI_LOG_ERROR("Wi-Fi disconnect timeouted\n");
      ret = W6X_STATUS_ERROR;
    }
  }

  p_wifi_ctx->Expected_event_disconnect = 0U;
  return ret;
}

W6X_Status_t W6X_WiFi_AddCredentials(uint8_t ssid[W6X_WIFI_MAX_SSID_SIZE + 1],
                                     uint8_t password[W6X_WIFI_MAX_PASSWORD_SIZE + 1])
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  return TranslateErrorStatus(W61_WiFi_AddCredentials(W6X_WiFi_drv_obj, ssid, password));
}

W6X_Status_t W6X_WiFi_DeleteCredentials(uint8_t ssid[W6X_WIFI_MAX_SSID_SIZE + 1])
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  return TranslateErrorStatus(W61_WiFi_DeleteCredentials(W6X_WiFi_drv_obj, ssid));
}

W6X_Status_t W6X_WiFi_GetCredentials(W6X_WiFi_CredentialsList_t *credentials_list)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  return TranslateErrorStatus(W61_WiFi_GetCredentials(W6X_WiFi_drv_obj,
                                                      (W61_WiFi_CredentialsList_t *)credentials_list));
}

W6X_Status_t W6X_WiFi_GetAutoConnect(uint32_t *on_off)
{
  W6X_Status_t ret;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Get auto Connect state */
  ret = TranslateErrorStatus(W61_WiFi_GetAutoConnect(W6X_WiFi_drv_obj, on_off));
  if (ret == W6X_STATUS_OK)
  {
    WIFI_LOG_DEBUG("Get auto Connect state succeed\n");
  }
  return ret;
}

W6X_Status_t W6X_WiFi_GetCountryCode(uint32_t *policy, char *country_string)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Get the country code */
  return TranslateErrorStatus(W61_WiFi_GetCountryCode(W6X_WiFi_drv_obj, policy, country_string));
}

W6X_Status_t W6X_WiFi_SetCountryCode(uint32_t *policy, char *country_string)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Set the country code */
  return TranslateErrorStatus(W61_WiFi_SetCountryCode(W6X_WiFi_drv_obj, policy, country_string));
}

/* ============================================================
 * =============== Station specific APIs ======================
 * ============================================================ */
W6X_Status_t W6X_WiFi_Station_Start(void)
{
  W6X_Status_t ret;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  /* Start the Wi-Fi as station */
  ret = TranslateErrorStatus(W61_WiFi_Station_Start(W6X_WiFi_drv_obj));

  if (ret == W6X_STATUS_OK)
  {
    /* Set the station state */
    p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_DISCONNECTED;
    W6X_WiFi_drv_obj->WifiCtx.StaState = W61_WIFI_STATE_STA_DISCONNECTED;
    /* Set the access point state */
    p_wifi_ctx->ApState = W6X_WIFI_STATE_AP_OFF;
    W6X_WiFi_drv_obj->WifiCtx.ApState = W61_WIFI_STATE_AP_OFF;
  }

  return ret;
}

W6X_Status_t W6X_WiFi_Station_GetState(W6X_WiFi_StaStateType_e *state, W6X_WiFi_Connect_t *connect_data)
{
  W6X_Status_t ret;
  int32_t rssi = 0;
  W61_WiFi_SecurityType_e security = W61_WIFI_SECURITY_UNKNOWN;
  W61_WiFi_Protocol_e protocol = W61_WIFI_PROTOCOL_UNKNOWN;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  *state = p_wifi_ctx->StaState;

  if ((connect_data != NULL) &&
      ((p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_GOT_IP) || (p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_CONNECTED)))
  {
    /* Get the connection information if the Wi-Fi station is connected */
    ret = TranslateErrorStatus(W61_WiFi_GetConnectInfo(W6X_WiFi_drv_obj, &rssi, &security, &protocol));
    if (ret == W6X_STATUS_OK)
    {
      (void)memcpy(connect_data->SSID, W6X_WiFi_drv_obj->WifiCtx.SSID, W6X_WIFI_MAX_SSID_SIZE + 1U);
      (void)memcpy(connect_data->MAC, W6X_WiFi_drv_obj->WifiCtx.APSettings.MAC_Addr, 6);
      connect_data->Rssi = rssi;
      connect_data->Channel = W6X_WiFi_drv_obj->WifiCtx.STASettings.Channel;
      connect_data->Reconnection_interval = W6X_WiFi_drv_obj->WifiCtx.STASettings.ReconnInterval;
      connect_data->Security = (W6X_WiFi_SecurityType_e)security;
      connect_data->Protocol = (W6X_WiFi_Protocol_e)protocol;
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

W6X_Status_t W6X_WiFi_Station_GetMACAddress(uint8_t mac[6])
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Get the MAC address */
  return TranslateErrorStatus(W61_WiFi_Station_GetMACAddress(W6X_WiFi_drv_obj, mac));
}

/* ============================================================
 * =============== Soft-AP specific APIs ======================
 * ============================================================ */
W6X_Status_t W6X_WiFi_AP_Start(W6X_WiFi_ApConfig_t *ap_config)
{
  W6X_Status_t ret;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  /* Check when switching from STA to STA+AP (as their is no reconnection of the STA) */
  if ((p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_CONNECTED) || (p_wifi_ctx->StaState == W6X_WIFI_STATE_STA_GOT_IP))
  {
    WIFI_LOG_WARN("Ensure Station and Soft-AP does not have the same subnet IP\n");
  }

  /* Initialize the dual interface mode */
  ret = TranslateErrorStatus(W61_WiFi_SetDualMode(W6X_WiFi_drv_obj));
  if (ret == W6X_STATUS_OK)
  {
    /* Activate the Soft-AP */
    ret = TranslateErrorStatus(W61_WiFi_AP_Start(W6X_WiFi_drv_obj, (W61_WiFi_ApConfig_t *)ap_config));
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
      if ((W6X_WiFi_drv_obj->NetCtx.Supported == 0U) && (W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_up_fn != NULL))
      {
        (void)W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_up_fn();
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
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Get the connected station list to the Soft-AP interface */
  ret = TranslateErrorStatus(W61_WiFi_AP_ListConnectedStations(W6X_WiFi_drv_obj, &Stations));

  if (ret == W6X_STATUS_OK)
  {
    if (Stations.Count != 0U)
    {
      WIFI_LOG_WARN("Soft-AP is still connected to a station, they will be disconnected before stopping the Soft-AP\n");
      for (int32_t i = 0; i < Stations.Count; i++)
      {
        /* Disconnect the station from the Soft-AP interface with the MAC address */
        ret = TranslateErrorStatus(W61_WiFi_AP_DisconnectStation(W6X_WiFi_drv_obj, Stations.STA[i].MAC));
        if (ret != W6X_STATUS_OK)
        {
          WIFI_LOG_ERROR("Failed to disconnect station");
          goto _err;
        }
      }
    }
  }

  /* Deactivate the Soft-AP */
  ret = TranslateErrorStatus(W61_WiFi_AP_Stop(W6X_WiFi_drv_obj, Reconnect));
  if ((W6X_WiFi_drv_obj->NetCtx.Supported == 0U) && (W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_down_fn != NULL))
  {
    (void)W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_ap_down_fn();
  }

_err:
  return ret;
}

W6X_Status_t W6X_WiFi_AP_GetConfig(W6X_WiFi_ApConfig_t *ap_config)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  if (W6X_WiFi_drv_obj->WifiCtx.ApState != W61_WIFI_STATE_AP_RUNNING)
  {
    WIFI_LOG_ERROR("Soft-AP is not started\n");
    return ret;
  }

  /* Get the Soft-AP configuration */
  return TranslateErrorStatus(W61_WiFi_AP_GetConfig(W6X_WiFi_drv_obj, (W61_WiFi_ApConfig_t *)ap_config));
}

W6X_Status_t W6X_WiFi_AP_ListConnectedStations(W6X_WiFi_Connected_Sta_t *connected_sta)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Get the connected station list */
  return TranslateErrorStatus(W61_WiFi_AP_ListConnectedStations(W6X_WiFi_drv_obj,
                                                                (W61_WiFi_Connected_Sta_t *)connected_sta));
}

W6X_Status_t W6X_WiFi_AP_DisconnectStation(uint8_t mac[6])
{
  W6X_Status_t ret;
  EventBits_t eventBits;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(p_wifi_ctx, W6X_WiFi_Ctx_Null_str);

  p_wifi_ctx->evt_sta_disconnect.Expected_event_sta_disconnect = 1;
  /* Assign the MAC address to the event structure */
  for (int32_t i = 0; i < 6; i++)
  {
    p_wifi_ctx->evt_sta_disconnect.MAC[i] = mac[i];
  }

  /* Disconnect the station */
  ret = TranslateErrorStatus(W61_WiFi_AP_DisconnectStation(W6X_WiFi_drv_obj, mac));
  if (ret == W6X_STATUS_OK)
  {
    /* Wait for the disconnection to be done */
    eventBits = xEventGroupWaitBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_STA_DISCONNECT, pdTRUE, pdFALSE,
                                    pdMS_TO_TICKS(W6X_WIFI_STA_DISCONNECT_TIMEOUT_MS));
    if ((eventBits & W6X_WIFI_EVENT_FLAG_STA_DISCONNECT) == 0U)
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

W6X_Status_t W6X_WiFi_AP_GetMACAddress(uint8_t mac[6])
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Get the MAC address */
  return TranslateErrorStatus(W61_WiFi_AP_GetMACAddress(W6X_WiFi_drv_obj, mac));
}

/* ============================================================
 * =============== Low-Power specific APIs ====================
 * ============================================================ */
W6X_Status_t W6X_WiFi_SetDTIM(uint32_t dtim_factor)
{
  uint32_t dtim_ap = 0;
  uint32_t current_dtim = dtim_factor;
  uint32_t ps_mode = 0;
  uint32_t try = 30; /* Try up to 3 seconds to get the DTIM from the Soft-AP */
  W61_Status_t ret;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  if ((W6X_WiFi_drv_obj->WifiCtx.StaState != W61_WIFI_STATE_STA_CONNECTED) &&
      (W6X_WiFi_drv_obj->WifiCtx.StaState != W61_WIFI_STATE_STA_GOT_IP))
  {
    WIFI_LOG_WARN("Device is not in the appropriate state to run this command\n");
    return W6X_STATUS_ERROR;
  }

  if (W61_GetPowerMode(W6X_WiFi_drv_obj, &ps_mode) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }

  if (W61_SdkMinVersion(W6X_WiFi_drv_obj, 2, 0, 97) != W61_STATUS_OK)
  {
    /* Get the current DTIM period for Soft-AP */
    do
    {
      vTaskDelay(pdMS_TO_TICKS(100));
      (void)W61_WiFi_GetDTIM_AP(W6X_WiFi_drv_obj, &dtim_ap);
    } while ((try-- > 0U) && (dtim_ap == 0U));

    if (dtim_ap == 0U)
    {
      return W6X_STATUS_ERROR;
    }

    current_dtim = dtim_factor * dtim_ap;
  }

  ret = W61_WiFi_SetDTIM(W6X_WiFi_drv_obj, current_dtim);
  if (ret == W61_STATUS_OK)
  {
    W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Factor = dtim_factor;
    /* Below code is only valid for SDK 2.0.89 */
    W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Interval = current_dtim;
    if (ps_mode == 0U)
    {
      WIFI_LOG_WARN("Device is not in power save mode, DTIM configuration"
                    " will be applied when changing to power save mode.\n");
    }
  }
  return TranslateErrorStatus(ret);
}

W6X_Status_t W6X_WiFi_GetDTIM(uint32_t *dtim_factor, uint32_t *dtim_interval)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  *dtim_factor = W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Factor;

  if ((W6X_WiFi_drv_obj->WifiCtx.StaState != W61_WIFI_STATE_STA_CONNECTED) &&
      (W6X_WiFi_drv_obj->WifiCtx.StaState != W61_WIFI_STATE_STA_GOT_IP))
  {
    *dtim_interval = 0;
    return W6X_STATUS_OK;
  }

  if (W61_SdkMinVersion(W6X_WiFi_drv_obj, 2, 0, 97) == W61_STATUS_OK)
  {
    uint32_t dtim_ap = 0;
    uint32_t try = 30U; /* Try up to 3 seconds to get the DTIM from the Soft-AP */
    W61_Status_t ret;

    do
    {
      vTaskDelay(pdMS_TO_TICKS(100));
      ret = W61_WiFi_GetDTIM_AP(W6X_WiFi_drv_obj, &dtim_ap);
    } while ((try-- > 0U) && (ret == W61_STATUS_ERROR));

    if (dtim_ap == 0U)
    {
      return W6X_STATUS_ERROR;
    }

    /* In the case the dtim_factor is null, the device will wake up every beacon */
    if (*dtim_factor == 0U)
    {
      *dtim_interval = 1U;
    }
    else
    {
      *dtim_interval = dtim_ap * (*dtim_factor);
    }
  }
  else
  {
    *dtim_interval = W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Interval;
  }

  return W6X_STATUS_OK;
}

W6X_Status_t W6X_WiFi_GetDTIM_AP(uint32_t *dtim)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  if ((W6X_WiFi_drv_obj->WifiCtx.StaState != W61_WIFI_STATE_STA_CONNECTED) &&
      (W6X_WiFi_drv_obj->WifiCtx.StaState != W61_WIFI_STATE_STA_GOT_IP))
  {
    *dtim = 0;
    return W6X_STATUS_ERROR;
  }
  /* Get the DTIM */
  return TranslateErrorStatus(W61_WiFi_GetDTIM_AP(W6X_WiFi_drv_obj, dtim));
}

W6X_Status_t W6X_WiFi_TWT_Setup(W6X_WiFi_TWT_Setup_Params_t *twt_params)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  uint32_t is_supported = 0;
  uint32_t ps_mode = 0;
  W61_WiFi_TWT_Status_t twt_status = {0};

  /* Verify TWT is supported by the AP */
  if ((W61_WiFi_TWT_IsSupported(W6X_WiFi_drv_obj, &is_supported) != W61_STATUS_OK) || (is_supported == 0U))
  {
    return W6X_STATUS_ERROR;
  }

  if (W61_SdkMinVersion(W6X_WiFi_drv_obj, 2, 0, 106) != W61_STATUS_OK)
  {
    /* TWT multi flow is not supported in current version */
    if (W61_WiFi_TWT_GetStatus(W6X_WiFi_drv_obj, &twt_status) != W61_STATUS_OK)
    {
      return W6X_STATUS_ERROR;
    }
    if (twt_status.flow_count == 1U)
    {
      WIFI_LOG_ERROR("Only one TWT flow setup is supported\n");
      return W6X_STATUS_ERROR;
    }
  }

  if (W61_GetPowerMode(W6X_WiFi_drv_obj, &ps_mode) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }

  if (ps_mode == 0U)
  {
    WIFI_LOG_WARN("Device is not in the appropriate power save mode\n");
  }

  /* Setup TWT */
  return TranslateErrorStatus(W61_WiFi_TWT_Setup(W6X_WiFi_drv_obj, (W61_WiFi_TWT_Setup_Params_t *)twt_params));
}

W6X_Status_t W6X_WiFi_TWT_GetStatus(W6X_WiFi_TWT_Status_t *twt_status)
{
  uint32_t is_supported = 0;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Verify TWT is supported by the AP */
  if (W61_WiFi_TWT_IsSupported(W6X_WiFi_drv_obj, &is_supported) != W61_STATUS_OK)
  {
    return W6X_STATUS_ERROR;
  }
  if (is_supported == 0U)
  {
    twt_status->is_supported = 0;
    return W6X_STATUS_OK;
  }
  twt_status->is_supported = 1;

  /* Get TWT status */
  return TranslateErrorStatus(W61_WiFi_TWT_GetStatus(W6X_WiFi_drv_obj, (W61_WiFi_TWT_Status_t *)twt_status));
}

W6X_Status_t W6X_WiFi_TWT_Teardown(W6X_WiFi_TWT_Teardown_Params_t *twt_params)
{
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  W61_WiFi_TWT_Status_t twt_status = {0};

  /* Check if a TWT flow is currently active */
  if ((W61_WiFi_TWT_GetStatus(W6X_WiFi_drv_obj, &twt_status) != W61_STATUS_OK) || (twt_status.flow_count == 0U))
  {
    return W6X_STATUS_ERROR;
  }
  /* Teardown TWT */
  return TranslateErrorStatus(W61_WiFi_TWT_Teardown(W6X_WiFi_drv_obj, (W61_WiFi_TWT_Teardown_Params_t *)twt_params));
}

W6X_Status_t W6X_WiFi_GetAntennaDiversity(W6X_WiFi_AntennaInfo_t *antenna_info)
{
  W6X_Status_t ret;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
  NULL_ASSERT(antenna_info, "Antenna info pointer is NULL");

  /* Get the antenna information */
  ret = TranslateErrorStatus(W61_WiFi_GetAntennaEnable(W6X_WiFi_drv_obj,
                                                       (W61_WiFi_AntennaMode_e *)&antenna_info->mode));
  if (ret != W6X_STATUS_OK)
  {
    return ret;
  }
  ret = TranslateErrorStatus(W61_WiFi_GetAntennaUsed(W6X_WiFi_drv_obj, &antenna_info->antenna_id));
  if (ret != W6X_STATUS_OK)
  {
    return ret;
  }

  return W6X_STATUS_OK;
}

W6X_Status_t W6X_WiFi_SetAntennaDiversity(W6X_WiFi_AntennaMode_e mode)
{
  W6X_Status_t ret;
  NULL_ASSERT(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);

  /* Set the antenna configuration */
  ret = TranslateErrorStatus(W61_WiFi_SetAntennaEnable(W6X_WiFi_drv_obj, (W61_WiFi_AntennaMode_e)mode));
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

const char *W6X_WiFi_AP_SecurityToStr(W6X_WiFi_ApSecurityType_e security)
{
  /* Check if the security is unknown */
  if (security > W6X_WIFI_AP_SECURITY_UNKNOWN)
  {
    return "Unknown";
  }
  /* Return the security string */
  return W6X_WiFi_AP_Security_str[security];
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
  NULL_ASSERT_VOID(W6X_WiFi_drv_obj, W6X_WiFi_Uninit_str);
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
      W6X_WiFi_drv_obj->WifiCtx.scan_done_cb(W6X_WiFi_drv_obj->WifiCtx.scan_status,
                                             &W6X_WiFi_drv_obj->WifiCtx.ScanResults);
      break;

    case W61_WIFI_EVT_CONNECTED_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_CONNECTED;
      W6X_WiFi_drv_obj->WifiCtx.StaState = W61_WIFI_STATE_STA_CONNECTED;

      /* Delay added to avoid any AT command to be sent to soon after the CONNECTED event.
       * This could create unexpected behaviors (wrong value returned, command not responsive, ...) */
      vTaskDelay(pdMS_TO_TICKS(100));

      if (p_wifi_ctx->Expected_event_connect == 1U)
      {
        /* If the connected event was expected, set the event bit to release the wait */
        (void)xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_CONNECT);
      }

      if ((W6X_WiFi_drv_obj->NetCtx.Supported == 0U) && (W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_sta_up_fn != NULL))
      {
        (void)W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_sta_up_fn();
      }

      /* Call the application callback to inform that the station is connected */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_CONNECTED_ID, NULL);

      break;

#if (ST67_ARCH == W6X_ARCH_T01)
    case W61_WIFI_EVT_GOT_IP_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_GOT_IP;
      W6X_WiFi_drv_obj->WifiCtx.StaState = W61_WIFI_STATE_STA_GOT_IP;

      /* Call the application callback to inform that the station got an IP */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_GOT_IP_ID, (void *)NULL);

      if (p_wifi_ctx->Expected_event_gotip == 1U)
      {
        /* If the IP address event was expected, set the event bit to release the wait */
        (void)xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_GOT_IP);
        p_wifi_ctx->Expected_event_gotip = 0U;
      }

      break;
#endif /* ST67_ARCH */

    case W61_WIFI_EVT_DISCONNECTED_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_DISCONNECTED;
      W6X_WiFi_drv_obj->WifiCtx.StaState = W61_WIFI_STATE_STA_DISCONNECTED;
      W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Interval = 0;
      W6X_WiFi_drv_obj->LowPowerCfg.WiFi_DTIM_Factor = 1U;

      if (p_wifi_ctx->Expected_event_disconnect == 1U)
      {
        /* If the disconnected event was expected, set the event bit to release the wait */
        (void)xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_DISCONNECT);
        p_wifi_ctx->Expected_event_disconnect = 0U;
      }

      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_DISCONNECTED_ID, NULL);

      if ((W6X_WiFi_drv_obj->NetCtx.Supported == 0U) && (W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_sta_down_fn != NULL))
      {
        (void)W6X_WiFi_drv_obj->Callbacks.Netif_cb.link_sta_down_fn();
      }

      break;

    case W61_WIFI_EVT_CONNECTING_ID:
      /* Set the station state */
      p_wifi_ctx->StaState = W6X_WIFI_STATE_STA_CONNECTING;
      W6X_WiFi_drv_obj->WifiCtx.StaState = W61_WIFI_STATE_STA_CONNECTING;

      /* Call the application callback to inform that the station is connecting */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_CONNECTING_ID, (void *)NULL);
      break;

    case W61_WIFI_EVT_REASON_ID:
      reason_code = *(uint32_t *)event_args;
      if (p_wifi_ctx->Expected_event_connect == 1U)
      {
        /* If the error event was expected, set the event bit to release the wait */
        (void)xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_REASON);
      }

      /* Call the application callback to inform that an error occurred */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_REASON_ID, (void *)&reason_code);

      break;

    default:
      /* Wi-Fi STA events unmanaged */
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
      if ((p_wifi_ctx->evt_sta_disconnect.Expected_event_sta_disconnect == 1U) &&
          (memcmp(p_wifi_ctx->evt_sta_disconnect.MAC,
                  ((W61_WiFi_CbParamData_t *)event_args)->MAC, 6) == 0))
      {
        /* If the disconnected event was expected, set the event bit to release the wait */
        (void)xEventGroupSetBits(p_wifi_ctx->Wifi_event, W6X_WIFI_EVENT_FLAG_STA_DISCONNECT);
        p_wifi_ctx->evt_sta_disconnect.Expected_event_sta_disconnect = 0U;
      }
      /* Call the application callback to inform that a station is disconnected from the Soft-AP */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_STA_DISCONNECTED_ID, event_args);
      break;

    case W61_WIFI_EVT_DIST_STA_IP_ID:
      /* Call the application callback to inform that a station has an IP address */
      p_cb_handler->APP_wifi_cb(W6X_WIFI_EVT_DIST_STA_IP_ID, event_args);
      break;

    default:
      /* Wi-Fi AP events unmanaged */
      break;
  }
}

/** @} */
