/**
  ******************************************************************************
  * @file    w6x_wifi_shell.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x WiFi Shell Commands
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
#include <string.h>
#include "w6x_api.h"
#include "shell.h"
#include "logging.h"
#include "common_parser.h" /* Common Parser functions */
#include "FreeRTOS.h"
#include "event_groups.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W6X_Private_WiFi_Constants
  * @{
  */

#define EVENT_FLAG_SCAN_DONE  (1<<1)  /*!< Scan done event bitmask */

#define SCAN_TIMEOUT_MS       10000   /*!< Delay before to declare the scan in failure */

#define CONNECT_MAX_INTERVAL  7200 /*!< Maximum reconnection interval in seconds */

#define CONNECT_MAX_ATTEMPTS  1000 /*!< Maximum reconnection attempts */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @addtogroup ST67W6X_Private_WiFi_Variables
  * @{
  */

/** Event bitmask flag used for asynchronous execution */
static EventGroupHandle_t scan_event = NULL;

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Private_WiFi_Functions
  * @{
  */

/**
  * @brief  Wi-Fi Scan callback function
  * @param  status: scan status
  * @param  entry: scan result entry
  */
void W6X_Shell_WiFi_Scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *entry);

/**
  * @brief  Wi-Fi scan shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Scan(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi connect shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Connect(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi disconnect shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Disconnect(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi auto-connect mode shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_AutoConnect(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get/set country code shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Country_Code(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get STA state shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Station_State(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get STA MAC shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Station_MAC(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi start Soft-AP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_AP_Start(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi stop Soft-AP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_AP_Stop(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi list connected stations shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_AP_List_Stations(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi disconnect station shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_AP_Disconnect_Station(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get Soft-AP MAC shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_AP_MAC(int32_t argc, char **argv);

/**
  * @brief  Set/Get DTIM shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_DTIM(int32_t argc, char **argv);

/**
  * @brief  Setup TWT shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_TWT_Setup(int32_t argc, char **argv);

/**
  * @brief  Get TWT status shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_TWT_GetStatus(int32_t argc, char **argv);

/**
  * @brief  Set TWT shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_TWT_Set(int32_t argc, char **argv);

/**
  * @brief  Teardown TWT shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_TWT_Teardown(int32_t argc, char **argv);

/**
  * @brief  Wi-FiAntenna diversity shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Antenna(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
void W6X_Shell_WiFi_Scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *entry)
{
  if ((entry == NULL) || (entry->AP == NULL))
  {
    return;
  }

  if (scan_event != NULL)
  {
    SHELL_PRINTF("Scan results :\n");

    /* Print the scan results */
    for (uint32_t count = 0; count < entry->Count; count++)
    {
      SHELL_PRINTF("[" MACSTR "] Channel: %2" PRIu16 " | %13.13s | %4s | RSSI: %4" PRIi16 " | SSID: %32.32s\n",
                   MAC2STR(entry->AP[count].MAC), entry->AP[count].Channel,
                   W6X_WiFi_SecurityToStr(entry->AP[count].Security),
                   W6X_WiFi_ProtocolToStr(entry->AP[count].Protocol),
                   entry->AP[count].RSSI, entry->AP[count].SSID);
    }

    /* Set the scan done event */
    xEventGroupSetBits(scan_event, EVENT_FLAG_SCAN_DONE);
  }
}

int32_t W6X_Shell_WiFi_Scan(int32_t argc, char **argv)
{
  W6X_WiFi_Scan_Opts_t Opts = {0};
  int32_t current_arg = 1;

  if (argc > 11)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Initialize the scan event */
  if (scan_event == NULL)
  {
    scan_event = xEventGroupCreate();
  }

  while (current_arg < argc)
  {
    /* Passive scan mode argument */
    if ((strncmp(argv[current_arg], "-p", 2) == 0) && strlen(argv[current_arg]) == 2)
    {
      /* Set the passive scan mode */
      Opts.Scan_type = W6X_WIFI_SCAN_PASSIVE;
    }
    /* SSID filter argument */
    else if ((strncmp(argv[current_arg], "-s", 2) == 0) && strlen(argv[current_arg]) == 2)
    {
      current_arg++;
      /* Check the SSID length */
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if (strlen(argv[current_arg]) > W6X_WIFI_MAX_SSID_SIZE)
      {
        return SHELL_STATUS_ERROR;
      }
      /* Copy the SSID */
      strncpy((char *)Opts.SSID, argv[current_arg], sizeof(Opts.SSID) - 1);
    }
    /* BSSID filter argument */
    else if ((strncmp(argv[current_arg], "-b", 2) == 0) && strlen(argv[current_arg]) == 2)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }

      /* Parse the BSSID filter argument */
      Parser_StrToMAC(argv[current_arg], Opts.MAC);

      /* Check the BSSID validity */
      if (Parser_CheckValidAddress(Opts.MAC, 6) != 0)
      {
        SHELL_E("Invalid BSSID address\n");
        return SHELL_STATUS_ERROR;
      }
    }
    /* Channel filter argument */
    else if ((strncmp(argv[current_arg], "-c", 2) == 0) && (strlen(argv[current_arg]) == 2))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the channel filter argument */
      Opts.Channel = (uint8_t)atoi(argv[current_arg]);
      /* Check the channel validity */
      if ((Opts.Channel < 1) || (Opts.Channel > 13))
      {
        return SHELL_STATUS_ERROR;
      }
    }
    /* Max number of beacon received argument */
    else if ((strncmp(argv[current_arg], "-n", 2) == 0) && strlen(argv[current_arg]) == 2)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the Max number of APs argument */
      Opts.MaxCnt = (uint8_t)atoi(argv[current_arg]);
      if ((Opts.MaxCnt < 1) || (Opts.MaxCnt > W61_WIFI_MAX_DETECTED_AP))
      {
        return SHELL_STATUS_ERROR;
      }
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    current_arg++;
  }

  /* Start the scan */
  if (W6X_STATUS_OK != W6X_WiFi_Scan(&Opts, W6X_Shell_WiFi_Scan_cb))
  {
    SHELL_E("Scan Failed\n");
    return SHELL_STATUS_ERROR;
  }
  /* Wait for the scan to be done */
  if ((int32_t)xEventGroupWaitBits(scan_event, EVENT_FLAG_SCAN_DONE, pdTRUE, pdFALSE,
                                   pdMS_TO_TICKS(SCAN_TIMEOUT_MS)) != EVENT_FLAG_SCAN_DONE)
  {
    /* Scan timeout */
    SHELL_E("Scan Failed\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to scan for WiFi networks */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Scan, wifi_scan,
                       wifi_scan [ -p ] [ -s SSID ] [ -b BSSID ] [ -c channel [1; 13] ] [ -n max_count [1; 50] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_Connect(int32_t argc, char **argv)
{
  W6X_WiFi_Connect_Opts_t connect_opts = {0};
  int32_t current_arg = 1;

  if (argc < 2) /* SSID is mandatory */
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Search if wps argument is present */
  for (int32_t i = 1; i < argc; i++)
  {
    if (strncmp(argv[i], "-wps", sizeof("-wps") - 1) == 0)
    {
      /* Connect to the AP using WPS */
      connect_opts.WPS = 1;
      if (W6X_WiFi_Connect(&connect_opts) == W6X_STATUS_OK)
      {
        SHELL_PRINTF("Connection success\n");
        return SHELL_STATUS_OK;
      }
      return SHELL_STATUS_ERROR;
    }
  }

  /* Parse the SSID argument */
  if (strlen(argv[current_arg]) > W6X_WIFI_MAX_SSID_SIZE)
  {
    SHELL_E("SSID is too long\n");
    return SHELL_STATUS_ERROR;
  }

  /* Copy the SSID */
  strncpy((char *)connect_opts.SSID, argv[current_arg], sizeof(connect_opts.SSID) - 1);
  current_arg++;

  /* Parse the Password argument if present */
  if (argc > 2 && (strncmp(argv[current_arg], "-", 1) != 0))
  {
    /* Check the password length */
    if (strlen(argv[current_arg]) > W6X_WIFI_MAX_PASSWORD_SIZE)
    {
      SHELL_E("Password is too long\n");
      return SHELL_STATUS_ERROR;
    }
    strncpy((char *)connect_opts.Password, argv[current_arg], sizeof(connect_opts.Password) - 1);
    current_arg++;
  }

  while (current_arg < argc)
  {
    /* Parse the BSSID argument */
    if (strncmp(argv[current_arg], "-b", sizeof("-b") - 1) == 0)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }

      /* Parse the BSSID */
      Parser_StrToMAC(argv[current_arg], connect_opts.MAC);

      /* Check the BSSID validity */
      if (Parser_CheckValidAddress(connect_opts.MAC, 6) != 0)
      {
        SHELL_E("Invalid BSSID address\n");
        return SHELL_STATUS_ERROR;
      }
    }

    /* Parse the reconnection interval argument */
    else if (strncmp(argv[current_arg], "-i", sizeof("-i") - 1) == 0)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }

      /* Check the reconnection interval validity */
      connect_opts.Reconnection_interval = (uint16_t)atoi(argv[current_arg]);
      if (connect_opts.Reconnection_interval > CONNECT_MAX_INTERVAL)
      {
        SHELL_E("Interval between two reconnection is out of range : [0;7200]\n");
        return SHELL_STATUS_ERROR;
      }
    }
    /* Parse the reconnection attempts argument */
    else if (strncmp(argv[current_arg], "-n", sizeof("-n") - 1) == 0)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }

      /* Check the reconnection attempts validity */
      connect_opts.Reconnection_nb_attempts = (uint16_t)atoi(argv[current_arg]);
      if (connect_opts.Reconnection_nb_attempts > CONNECT_MAX_ATTEMPTS)
      {
        SHELL_E("Number of reconnection attempts is out of range : [0;1000]\n");
        return SHELL_STATUS_ERROR;
      }
    }
    /* Parse the reconnection attempts argument */
    else if (strncmp(argv[current_arg], "-wep", sizeof("-wep") - 1) == 0)
    {
      current_arg++;

      connect_opts.WEP = 1;
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    current_arg++;
  }

  /* Connect to the AP */
  if (W6X_WiFi_Connect(&connect_opts) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Connection success\n");
  }
  else
  {
    SHELL_E("Connection error\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to connect to a WiFi network */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Connect, wifi_sta_connect,
                       wifi_sta_connect < SSID > [ Password ] [ -b BSSID ]
                       [ -i interval [0; 7200] ] [ -n nb_attempts [0; 1000] ] [ -wps ] [ -wep ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_Disconnect(int32_t argc, char **argv)
{
  uint32_t restore = 0;

  if (argc > 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if ((argc == 2) && (strncmp(argv[1], "-r", 2) == 0))
  {
    /* Restore the connection information to prevent autoconnect */
    restore = 1;
  }

  /* Disconnect from the AP */
  if (W6X_WiFi_Disconnect(restore) != W6X_STATUS_OK)
  {
    SHELL_E("Disconnection error\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to disconnect from a WiFi network */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Disconnect, wifi_sta_disconnect, wifi_sta_disconnect [ -r ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_AutoConnect(int32_t argc, char **argv)
{
  uint32_t state = 0;

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the auto connect current state */
  if (W6X_WiFi_GetAutoConnect(&state) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Auto connect state : %" PRIu32 "\n", state);
  }
  else
  {
    SHELL_PRINTF("Auto connect get failed\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get autoconnect status */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AutoConnect, wifi_auto_connect, wifi_auto_connect);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_Station_MAC(int32_t argc, char **argv)
{
  uint8_t mac_addr[6] = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the STA MAC address */
  if (W6X_WiFi_Station_GetMACAddress(mac_addr) == W6X_STATUS_OK)
  {
    /* Display the MAC address */
    SHELL_PRINTF("STA MAC : " MACSTR "\n", MAC2STR(mac_addr));
    return SHELL_STATUS_OK;
  }
  else
  {
    SHELL_E("Get STA MAC error\n");
    return SHELL_STATUS_ERROR;
  }
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get the STA MAC address */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Station_MAC, wifi_sta_mac, wifi_sta_mac);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_Station_State(int32_t argc, char **argv)
{
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_DISCONNECTED;
  W6X_WiFi_Connect_t ConnectData = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the STA state */
  if (W6X_WiFi_Station_GetState(&state, &ConnectData) == W6X_STATUS_OK)
  {
    /* Display the STA state */
    SHELL_PRINTF("STA state : %s\n", W6X_WiFi_StateToStr(state));

    /* Display the connection information if connected */
    if (((uint32_t)state == W6X_WIFI_STATE_STA_CONNECTED) || ((uint32_t)state == W6X_WIFI_STATE_STA_GOT_IP))
    {
      SHELL_PRINTF("Connected to following Access Point :\n");
      SHELL_PRINTF("[" MACSTR "] Channel: %" PRIu32 " | RSSI: %" PRIi32 " | SSID: %s\n",
                   MAC2STR(ConnectData.MAC),
                   ConnectData.Channel,
                   ConnectData.Rssi,
                   ConnectData.SSID);
    }

    return SHELL_STATUS_OK;
  }
  else
  {
    SHELL_E("Get STA state error\n");
    return SHELL_STATUS_ERROR;
  }
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to get the STA state */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Station_State, wifi_sta_state, wifi_sta_state);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_Country_Code(int32_t argc, char **argv)
{
  uint32_t policy = 0;
  char countryString[3] = {0}; /* Alpha-2 code: 2 characters + null terminator */
  int32_t len = 0;

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get the country code information */
    if (W6X_WiFi_GetCountryCode(&policy, countryString) == W6X_STATUS_OK)
    {
      /* Display the country code information */
      SHELL_PRINTF("Country policy : %" PRIu32 "\nCountry code : %s\n",
                   policy, countryString);
    }
    else
    {
      SHELL_E("Get Country code configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 3)
  {
    /* Get the country policy mode */
    policy = (uint32_t)atoi(argv[1]);
    if (!((policy == 0) || (policy == 1)))
    {
      SHELL_E("First parameter should be 0 to disable, or 1 to enable Country policy\n");
      return SHELL_STATUS_ERROR;
    }

    /* Get the country code */
    len = strlen(argv[2]);

    /* Check the country code length */
    if ((len < 0) || (len > 2))
    {
      SHELL_E("Second parameter length is invalid\n");
      return SHELL_STATUS_ERROR;
    }
    memcpy(countryString, argv[2], len);

    if (W6X_WiFi_SetCountryCode(&policy, countryString) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Country code configuration succeed\n");
    }
    else
    {
      SHELL_E("Country code configuration failed\n");
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
/** Shell command to get/set the country code */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Country_Code, wifi_country_code,
                       wifi_country_code [ 0:AP aligned country code; 1:User country code ]
                       [ Country code [CN; JP; US; EU; 00] ]);
#endif /* SHELL_CMD_LEVEL */

/** Shell command to start a WiFi Soft-AP */
int32_t W6X_Shell_WiFi_AP_Start(int32_t argc, char **argv)
{
  W6X_WiFi_ApConfig_t ap_config = {0};
  int32_t current_arg = 1;
  int32_t temp_protocol = 0;
  ap_config.MaxConnections = W6X_WIFI_MAX_CONNECTED_STATIONS;
  ap_config.Hidden = 0;
  ap_config.Channel = 1; /* Default option when not defined */
  ap_config.Protocol = W6X_WIFI_PROTOCOL_11AX; /* Default option when not defined */

  if (argc > 13)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    if (W6X_WiFi_AP_GetConfig(&ap_config) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("AP SSID :     %s\n", ap_config.SSID);
      SHELL_PRINTF("AP Channel :  %" PRIu32 "\n", ap_config.Channel);
      SHELL_PRINTF("AP Security : %s\n", W6X_WiFi_SecurityToStr(ap_config.Security));
      SHELL_PRINTF("AP Hidden :   %" PRIu32 "\n", ap_config.Hidden);
      SHELL_PRINTF("AP Protocol : %s\n", W6X_WiFi_ProtocolToStr(ap_config.Protocol));
      return SHELL_STATUS_OK;
    }
    else
    {
      SHELL_E("Get Soft-AP configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
#endif /* SHELL_CMD_LEVEL */

  while (current_arg < argc)
  {
    /* Parse the SSID argument */
    if (strncmp(argv[current_arg], "-s", 2) == 0)
    {
      current_arg++;
      /* Check the SSID length */
      if (current_arg == argc)
      {
        SHELL_E("SSID invalid\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if (strlen(argv[current_arg]) > W6X_WIFI_MAX_SSID_SIZE)
      {
        SHELL_E("SSID invalid\n");
        return SHELL_STATUS_ERROR;
      }
      /* Copy the SSID */
      strncpy((char *)ap_config.SSID, argv[current_arg], sizeof(ap_config.SSID) - 1);
    }
    /* Parse the Password argument */
    else if (strncmp(argv[current_arg], "-p", 2) == 0)
    {
      current_arg++;
      /* Check the password length */
      if (current_arg == argc)
      {
        SHELL_E("Password invalid\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if (strlen(argv[current_arg]) > W6X_WIFI_MAX_PASSWORD_SIZE)
      {
        SHELL_E("Password invalid\n");
        return SHELL_STATUS_ERROR;
      }
      /* Copy the password */
      strncpy((char *)ap_config.Password, argv[current_arg], sizeof(ap_config.Password) - 1);
    }
    /* Parse the Channel argument */
    else if (strncmp(argv[current_arg], "-c", 2) == 0)
    {
      current_arg++;
      if (current_arg == argc)
      {
        SHELL_E("Channel value invalid\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the channel */
      ap_config.Channel = (uint32_t)atoi(argv[current_arg]);
      if ((ap_config.Channel < 1) || (ap_config.Channel > 13))
      {
        SHELL_E("Channel value out of range : [1;13]\n");
        return SHELL_STATUS_ERROR;
      }
    }
    /* Parse the Security argument */
    else if (strncmp(argv[current_arg], "-e", 2) == 0)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the security */
      ap_config.Security = (W6X_WiFi_ApSecurityType_e)atoi(argv[current_arg]);
      if ((ap_config.Security > 4) || (ap_config.Security == 1))
      {
        SHELL_E("Security not supported in Soft-AP mode, range [0;4], WEP (=1) not supported\n");
        return SHELL_STATUS_ERROR;
      }
    }
    /* Parse the Hidden argument */
    else if (strncmp(argv[current_arg], "-h", 2) == 0)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the hidden value */
      ap_config.Hidden = (uint32_t)atoi(argv[current_arg]);
      if (ap_config.Hidden > 1)
      {
        SHELL_E("Hidden value out of range [0;1]\n");
        return SHELL_STATUS_ERROR;
      }
    }
    /* Parse the protocol argument */
    else if (strncmp(argv[current_arg], "-m", 2) == 0)
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Parse the protocol */
      temp_protocol = atoi(argv[current_arg]);
      if (temp_protocol > W6X_WIFI_PROTOCOL_11AX)
      {
        SHELL_E("Protocol not supported in Soft-AP mode, range [1: 802.11b, 2: 802.11g, 3: 802.11n, 4: 802.11ax]\n");
        return SHELL_STATUS_ERROR;
      }
      ap_config.Protocol = (W6X_WiFi_Protocol_e)temp_protocol;
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    current_arg++;
  }

  if (ap_config.SSID[0] == '\0')
  {
    SHELL_E("SSID cannot be null\n");
    return SHELL_STATUS_ERROR;
  }
  if (W6X_WiFi_AP_Start(&ap_config) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Soft-AP started successfully\n");
  }
  else
  {
    SHELL_E("Soft-AP start failed\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AP_Start, wifi_ap_start,
                       wifi_ap_start [ -s SSID ] [ -p Password ]
                       [ -c channel [1; 13] ] [ -e security [0:Open; 2:WPA; 3:WPA2; 4:WPA3] ] [ -h hidden [0; 1] ]
                       [ -m protocol [1: 802 11 b; 2: 802 11 g; 3: 802 11 n; 4: 802 11 ax] ]);
#endif /* SHELL_CMD_LEVEL */

/** Shell command to stop a WiFi Soft-AP */
int32_t W6X_Shell_WiFi_AP_Stop(int32_t argc, char **argv)
{
  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_WiFi_AP_Stop() == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Soft-AP stopped successfully\n");
  }
  else
  {
    SHELL_E("Soft-AP stop failed\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AP_Stop, wifi_ap_stop, wifi_ap_stop);
#endif /* SHELL_CMD_LEVEL */

#if (ST67_ARCH == W6X_ARCH_T01)
/** Shell command to list stations connected to the Soft-AP */
int32_t W6X_Shell_WiFi_AP_List_Stations(int32_t argc, char **argv)
{
  W6X_WiFi_Connected_Sta_t connected_sta = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_WiFi_AP_ListConnectedStations(&connected_sta) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Connected stations :\n");
    for (int32_t i = 0; i < connected_sta.Count; i++)
    {
      SHELL_PRINTF("MAC : " MACSTR " | IP : " IPSTR "\n",
                   MAC2STR(connected_sta.STA[i].MAC),
                   IP2STR(connected_sta.STA[i].IP));
    }
    return SHELL_STATUS_OK;
  }
  return SHELL_STATUS_ERROR;
}

#if (SHELL_CMD_LEVEL >= 0)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AP_List_Stations, wifi_ap_list_sta, wifi_ap_list_sta);
#endif /* SHELL_CMD_LEVEL */
#endif /* ST67_ARCH_T01 */

/** Shell command to disconnect a station connected to the Soft-AP */
int32_t W6X_Shell_WiFi_AP_Disconnect_Station(int32_t argc, char **argv)
{
  uint8_t mac_addr[6] = {0};

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the MAC address */
  Parser_StrToMAC(argv[1], mac_addr);

  /* Check the MAC address validity */
  if (Parser_CheckValidAddress(mac_addr, 6) != 0)
  {
    SHELL_E("Invalid MAC address\n");
    return SHELL_STATUS_ERROR;
  }

  if (W6X_WiFi_AP_DisconnectStation(mac_addr) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Station disconnected successfully\n");
  }
  else
  {
    SHELL_E("Disconnect station failed\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AP_Disconnect_Station, wifi_ap_disconnect_sta, wifi_ap_disconnect_sta < MAC >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_AP_MAC(int32_t argc, char **argv)
{
  uint8_t mac_addr[6] = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  else
  {
    /* Get the Soft-AP MAC address */
    if (W6X_WiFi_AP_GetMACAddress(mac_addr) == W6X_STATUS_OK)
    {
      /* Display the MAC address */
      SHELL_PRINTF("Soft-AP MAC : " MACSTR "\n", MAC2STR(mac_addr));
      return SHELL_STATUS_OK;
    }
    else
    {
      SHELL_E("Get Soft-AP MAC error\n");
      return SHELL_STATUS_ERROR;
    }
  }
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get the STA MAC address */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AP_MAC, wifi_ap_mac, wifi_ap_mac);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_DTIM(int32_t argc, char **argv)
{
  int32_t value = 0;

#if (SHELL_CMD_LEVEL >= 0)
  if (argc == 1)
  {
    uint32_t dtim_factor = 0;
    uint32_t dtim_interval = 0;
    uint32_t dtim_ap = 0;
    if (W6X_WiFi_GetDTIM(&dtim_factor, &dtim_interval) != W6X_STATUS_OK)
    {
      SHELL_E("Get DTIM error\n");
      return SHELL_STATUS_ERROR;
    }

    if (dtim_interval != 0)
    {
      if (W6X_WiFi_GetDTIM_AP(&dtim_ap) != W6X_STATUS_OK)
      {
        SHELL_E("Get AP DTIM error\n");
        return SHELL_STATUS_ERROR;
      }
    }

    SHELL_PRINTF("STA DTIM factor: %" PRIu32 "\n", dtim_factor);
    SHELL_PRINTF("AP  DTIM factor: %" PRIu32 "\n", dtim_ap);
    SHELL_PRINTF("Listen interval: %" PRIu32 " x beacon interval ms\n", dtim_interval);
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 2)
  {
    value = atoi(argv[1]);

    SHELL_PRINTF("STA DTIM factor: %" PRIi32 "\n", value);

    if (W6X_WiFi_SetDTIM((uint32_t)(value)) != W6X_STATUS_OK)
    {
      SHELL_E("Could not set dtim\n");
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
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_DTIM, wifi_dtim, wifi_dtim < value [greater than 0] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_TWT_Setup(int32_t argc, char **argv)
{
  W6X_WiFi_TWT_Setup_Params_t twt_params;

  if (argc != 6)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Setup type */
  twt_params.setup_type = (uint8_t)atoi(argv[1]);

  /* Flow type */
  twt_params.flow_type = (uint8_t)atoi(argv[2]);

  /* Wake interval exponent */
  twt_params.wake_int_exp = (uint32_t)atoi(argv[3]);

  /* Minimum wake duration */
  twt_params.min_twt_wake_dur = (uint32_t)atoi(argv[4]);

  /* Wake interval mantissa */
  twt_params.wake_int_mantissa = (uint32_t)atoi(argv[5]);

  if (W6X_WiFi_TWT_Setup(&twt_params) != W6X_STATUS_OK)
  {
    SHELL_E("Could not setup TWT\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_TWT_Setup, wifi_twt_setup,
                       wifi_twt_setup < setup_type(0: request; 1: suggest; 2: demand) >
                       < flow_type(0: announced; 1: unannounced) >
                       < wake_int_exp > < min_wake_duration > < wake_int_mantissa >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_TWT_GetStatus(int32_t argc, char **argv)
{
  W6X_WiFi_TWT_Status_t twt_status = {0};

  if (W6X_WiFi_TWT_GetStatus(&twt_status) != W6X_STATUS_OK)
  {
    SHELL_E("Could not get TWT status\n");
    return SHELL_STATUS_ERROR;
  }

  /* Print if TWT is not supported by AP */
  if (twt_status.is_supported == 0)
  {
    SHELL_E("TWT not supported\n");
    return SHELL_STATUS_ERROR;
  }

  /* Display the TWT status */
  SHELL_PRINTF("TWT flows active: %" PRIu32 "\n", twt_status.flow_count);
  if (twt_status.flow_count != 0)
  {
    for (uint32_t i = 0; i < W6X_WIFI_MAX_TWT_FLOWS; i++)
    {
      if (twt_status.flow[i].wake_int_mantissa != 0)
      {
        SHELL_PRINTF("ID: %" PRIu32 " Type %" PRIu16 " Wake Interval exponent: %" PRIu32
                     " Minimum Wake duration: %" PRIu32 " Wake Interval mantissa: %" PRIu32 "\n",
                     i, twt_status.flow[i].flow_type, twt_status.flow[i].wake_int_exp,
                     twt_status.flow[i].min_twt_wake_dur, twt_status.flow[i].wake_int_mantissa);
      }
    }
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_TWT_GetStatus, wifi_twt_status, wifi_twt_status);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_TWT_Teardown(int32_t argc, char **argv)
{
#if 0 /* TWT multi flow is not supported in current version */
  int32_t value = 0;
  W6X_WiFi_TWT_Teardown_Params_t twt_params = {0};

  if ((argc == 1) || (argc > 3))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Teardown type */
  value = atoi(argv[1]);
  if ((value < 0) || (value > 1))
  {
    SHELL_E("TWT teardown type should be between [0; 1]\n");
    return SHELL_STATUS_ERROR;
  }

  twt_params.all_twt = (uint8_t)value;
  twt_params.id = 0;

  if (argc == 3)
  {
    value = atoi(argv[2]);
    if ((value < 0) || (value > 10))
    {
      SHELL_E("In case of single flow teardown, flow id should be between [0; 10]\n");
      return SHELL_STATUS_ERROR;
    }

    twt_params.id = (uint8_t)value;
  }
#endif /* TWT multi flow is not supported in current version */

  W6X_WiFi_TWT_Teardown_Params_t twt_params = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Teardown TWT */
  if (W6X_WiFi_TWT_Teardown(&twt_params) != W6X_STATUS_OK)
  {
    SHELL_E("Could not teardown TWT\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
#if 0 /* TWT multi flow is not supported in current version */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_TWT_Teardown, wifi_twt_teardown,
                       wifi_twt_teardown < all_twt [0: a single flow; 1: all flows] >
                       [ flow_id(in case all_twt param equals 0) [0 ; 10]]);
#else
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_TWT_Teardown, wifi_twt_teardown, wifi_twt_teardown);
#endif /* TWT multi flow is not supported in current version */
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_WiFi_Antenna(int32_t argc, char **argv)
{
#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    W6X_WiFi_AntennaInfo_t antenna_info;
    if (W6X_WiFi_GetAntennaDiversity(&antenna_info) != W6X_STATUS_OK)
    {
      SHELL_E("Unable to get antenna info\n");
      return SHELL_STATUS_ERROR;
    }
    SHELL_PRINTF("Antenna info:\n");
    SHELL_PRINTF("  Mode       :  %s\n", W6X_WiFi_AntDivToStr(antenna_info.mode));
    SHELL_PRINTF("  Antenna ID :  %" PRIu32 "\n", antenna_info.antenna_id);
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 2)
  {
    W6X_WiFi_AntennaMode_e mode = (W6X_WiFi_AntennaMode_e)atoi(argv[1]);
    if (W6X_WiFi_SetAntennaDiversity(mode) != W6X_STATUS_OK)
    {
      SHELL_E("Unable to set antenna diversity\n");
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
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Antenna, wifi_antenna,
                       wifi_antenna [ mode [0: disabled; 1: static; 2: dynamic ] ]);
#endif /* SHELL_CMD_LEVEL */

/** @} */
