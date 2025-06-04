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
  * @brief  Wi-Fi station mode shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_STA_Mode(int32_t argc, char **argv);

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
  * @brief  Wi-Fi get/set hostname shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Hostname(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get/set STA IP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_STA_IP(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get/set STA Gateway shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_STA_DNS(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get STA MAC shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_STA_MAC(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get STA state shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_STA_State(int32_t argc, char **argv);

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
  * @brief  Wi-Fi start Soft-AP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_StartAP(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi stop Soft-AP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_StopAP(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi Soft-AP mode shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_AP_Mode(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi list connected stations shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_List_Stations(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi disconnect station shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Disconnect_Station(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get Soft-AP IP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_Get_AP_IP(int32_t argc, char **argv);

/**
  * @brief  Wi-Fi get/set DHCP shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_DHCP_Config(int32_t argc, char **argv);

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
  * @retval 0
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
int32_t W6X_Shell_WiFi_SetupTWT(int32_t argc, char **argv);

/**
  * @brief  Set TWT shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_SetTWT(int32_t argc, char **argv);

/**
  * @brief  Teardown TWT shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t W6X_Shell_WiFi_TeardownTWT(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
void W6X_Shell_WiFi_Scan_cb(int32_t status, W6X_WiFi_Scan_Result_t *entry)
{
  if (scan_event != NULL)
  {
    SHELL_PRINTF("Scan results :\n");

    /* Print the scan results */
    for (uint32_t count = 0; count < entry->Count; count++)
    {
      SHELL_PRINTF("[" MACSTR "] Channel: %2" PRIu16 " | %13.13s | RSSI: %4" PRIi16 " | SSID: %32.32s\n",
                   MAC2STR(entry->AP[count].MAC), entry->AP[count].Channel,
                   W6X_WiFi_SecurityToStr(entry->AP[count].Security),
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
  int32_t tmp = 0;

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
      uint32_t temp[6];

      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }

      /* Parse the BSSID filter argument */
      if (sscanf(argv[current_arg], "%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32,
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]) != 6)
      {
        return SHELL_STATUS_ERROR;
      }

      /* Check the MAC address validity */
      for (int32_t i = 0; i < 6; i++)
      {
        if (temp[i] > 0xFF)
        {
          return SHELL_STATUS_ERROR;
        }
        Opts.MAC[i] = (uint8_t)temp[i];
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
      if (Parser_StrToInt(argv[current_arg], NULL, &tmp) == 0)
      {
        SHELL_E("Invalid channel value\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      /* Check the channel validity */
      if ((tmp < 1) || (tmp > 13))
      {
        return SHELL_STATUS_ERROR;
      }

      Opts.Channel = (uint8_t)tmp;
      tmp = 0;

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
      if (Parser_StrToInt(argv[current_arg], NULL, &tmp) == 0)
      {
        SHELL_E("Invalid max number of APs value\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if ((tmp < 1) || (tmp > W61_WIFI_MAX_DETECTED_AP))
      {
        return SHELL_STATUS_ERROR;
      }

      Opts.MaxCnt = tmp;
      tmp = 0;
    }
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    current_arg++;
  }

  /* Start the scan */
  if (W6X_STATUS_OK == W6X_WiFi_Scan(&Opts, W6X_Shell_WiFi_Scan_cb))
  {
    /* Wait for the scan to be done */
    if ((int32_t)xEventGroupWaitBits(scan_event, EVENT_FLAG_SCAN_DONE, pdTRUE, pdFALSE,
                                     pdMS_TO_TICKS(SCAN_TIMEOUT_MS)) != EVENT_FLAG_SCAN_DONE)
    {
      /* Scan timeout */
      SHELL_E("Scan Failed\n");
    }
  }
  return SHELL_STATUS_OK;
}

/** Shell command to scan for WiFi networks */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Scan, wifi_scan,
                       wifi_scan [ -p ] [ -s SSID ] [ -b BSSID ] [ -c channel [1; 13] ] [ -n max_count [1; 50] ]);

int32_t W6X_Shell_WiFi_Connect(int32_t argc, char **argv)
{
  W6X_WiFi_Connect_Opts_t connect_opts = {0};
  int32_t current_arg = 1;
  int32_t tmp = 0;

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
      uint32_t temp[6];
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }

      /* Parse the MAC address */
      if (sscanf(argv[current_arg], "%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32,
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]) != 6)
      {
        SHELL_E("MAC address is not valid.\n");
        return SHELL_STATUS_ERROR;
      }

      /* Check the MAC address validity */
      for (int32_t i = 0; i < 6; i++)
      {
        if (temp[i] > 0xFF)
        {
          SHELL_E("MAC address is not valid.\n");
          return SHELL_STATUS_ERROR;
        }
        connect_opts.MAC[i] = (uint8_t)temp[i];
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
      if ((Parser_StrToInt(argv[current_arg], NULL, &tmp) == 0) || (tmp < 0) || (tmp > CONNECT_MAX_INTERVAL))
      {
        SHELL_E("Interval between two reconnection is out of range : [0;7200]\n");
        return SHELL_STATUS_ERROR;
      }

      connect_opts.Reconnection_interval = (uint16_t)tmp;
      tmp = 0;
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
      if ((Parser_StrToInt(argv[current_arg], NULL, &tmp) == 0) || (tmp < 0) || (tmp > CONNECT_MAX_ATTEMPTS))
      {
        SHELL_E("Number of reconnection attempts is out of range : [0;1000]\n");
        return SHELL_STATUS_ERROR;
      }

      connect_opts.Reconnection_nb_attempts = (uint16_t)tmp;
      tmp = 0;
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

/** Shell command to connect to a WiFi network */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Connect, wifi_sta_connect,
                       wifi_sta_connect < SSID > [ Password ] [ -b BSSID ]
                       [ -i interval [0; 7200] ] [ -n nb_attempts [0; 1000] ] [ -wps ] [ -wep ]);

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

/** Shell command to disconnect from a WiFi network */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Disconnect, wifi_sta_disconnect, wifi_sta_disconnect [ -r ]);

int32_t W6X_Shell_WiFi_STA_Mode(int32_t argc, char **argv)
{
  W6X_WiFi_Mode_t mode = {0};
  int32_t tmp = 0;

  if (argc > 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (argc == 1)
  {
    if (W6X_WiFi_GetStaMode(&mode) == W6X_STATUS_OK) /* Get the STA mode */
    {
      SHELL_PRINTF("STA mode : %" PRIu16 " (B:%" PRIu16 " G:%" PRIu16 " N:%" PRIu16 " AX:%" PRIu16 ")\n",
                   mode.byte, mode.bit.b_mode, mode.bit.g_mode, mode.bit.n_mode, mode.bit.ax_mode);
    }
    else
    {
      SHELL_PRINTF("Get STA mode failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else if (argc == 2)
  {
    /* Check the STA mode argument */
    if (Parser_StrToInt(argv[1], NULL, &tmp) == 0)
    {
      SHELL_E("Invalid STA mode\n");
      return SHELL_STATUS_ERROR;
    }
    mode.byte = (uint8_t)tmp;
    if (W6X_WiFi_SetStaMode(&mode) == W6X_STATUS_OK) /* Set the STA mode */
    {
      SHELL_PRINTF("STA mode set to : %" PRIu16 "\n", mode.byte);
    }
    else
    {
      SHELL_PRINTF("Set STA mode failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  return SHELL_STATUS_OK;
}

/** Shell command to get/set the STA mode */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_STA_Mode, wifi_sta_mode, wifi_sta_mode [ mode ]);

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

/** Shell command to enable/disable autoconnect feature */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AutoConnect, wifi_auto_connect, wifi_auto_connect);

int32_t W6X_Shell_WiFi_Hostname(int32_t argc, char **argv)
{
  uint8_t hostname[34] = {0};

  if (argc == 1)
  {
    /* Get the host name */
    if (W6X_WiFi_GetHostname(hostname) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Host name : %s\n", hostname);
    }
    else
    {
      SHELL_PRINTF("Get host name failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else if (argc == 2)
  {
    /* Check the host name length */
    if (strlen(argv[1]) > 33)
    {
      SHELL_E("Host name maximum length is 32\n");
      return SHELL_STATUS_ERROR;
    }

    /* Set the host name */
    strncpy((char *)hostname, argv[1], 33);
    if (W6X_WiFi_SetHostname(hostname) == W6X_STATUS_OK)
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

/** Shell command to get/set the hostname */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Hostname, wifi_hostname, wifi_hostname [ hostname ]);

int32_t W6X_Shell_WiFi_STA_IP(int32_t argc, char **argv)
{
  uint8_t ip_addr[4] = {0};
  uint8_t gateway_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};

  if (argc > 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (argc == 1)
  {
    /* Get the STA IP configuration */
    if (W6X_WiFi_GetStaIpAddress(ip_addr, gateway_addr, netmask_addr) == W6X_STATUS_OK)
    {
      /* Display the IP configuration */
      SHELL_PRINTF("STA IP : " IPSTR "\n", IP2STR(ip_addr));
      SHELL_PRINTF("GW IP : " IPSTR "\n", IP2STR(gateway_addr));
      SHELL_PRINTF("NETMASK IP : " IPSTR "\n", IP2STR(netmask_addr));
    }
    else
    {
      SHELL_E("Get STA IP error\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    /* Set the STA IP configuration in IP, Gateway, Netmask fixed order. Gateway and Netmask are optional */
    for (int32_t i = 1;  i < argc; i++)
    {
      switch (i)
      {
        case 1:
          /* Parse the IP address */
          Parser_StrToIP(argv[i], ip_addr);
          if (Parser_CheckIP(ip_addr) != 0)
          {
            SHELL_E("IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 2:
          /* Parse the Gateway address */
          Parser_StrToIP(argv[i], gateway_addr);
          if (Parser_CheckIP(gateway_addr) != 0)
          {
            SHELL_E("Gateway IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 3:
          /* Parse the Netmask address */
          Parser_StrToIP(argv[i], netmask_addr);
          if (Parser_CheckIP(netmask_addr) != 0)
          {
            SHELL_E("Netmask IP address invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
      }
    }

    /* Set the IP configuration */
    if (W6X_WiFi_SetStaIpAddress(ip_addr, gateway_addr, netmask_addr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("STA IP configuration set successfully\n");
    }
    else
    {
      SHELL_E("Set STA IP configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  return SHELL_STATUS_OK;
}

/** Shell command to get/set the STA IP configuration */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_STA_IP, wifi_sta_ip, wifi_sta_ip [ IP addr ] [ Gateway addr ] [ Netmask addr ]);

int32_t W6X_Shell_WiFi_STA_DNS(int32_t argc, char **argv)
{
  uint32_t dns_enable = 0;
  uint8_t dns1_addr[4] = {0};
  uint8_t dns2_addr[4] = {0};
  uint8_t dns3_addr[4] = {0};
  int32_t tmp = 0;

  if (argc > 5)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (argc == 1)
  {
    /* Get the STA DNS configuration */
    if (W6X_WiFi_GetDnsAddress(&dns_enable, dns1_addr, dns2_addr, dns3_addr) == W6X_STATUS_OK)
    {
      /* Display the DNS configuration */
      SHELL_PRINTF("DNS state : %" PRIu32 "\n", dns_enable);
      SHELL_PRINTF("DNS1 IP : " IPSTR "\n", IP2STR(dns1_addr));
      SHELL_PRINTF("DNS2 IP : " IPSTR "\n", IP2STR(dns2_addr));
      SHELL_PRINTF("DNS3 IP : " IPSTR "\n", IP2STR(dns3_addr));
      return SHELL_STATUS_OK;
    }
    else
    {
      SHELL_E("Get STA DNS configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    /* Set the STA DNS configuration in enable, DNS1, DNS2, DNS3 fixed order. DNS2 and DNS3 are optional */
    for (int32_t i = 1;  i < argc; i++)
    {
      switch (i)
      {
        case 1:
          /* Parse the DNS enable */
          if ((Parser_StrToInt(argv[i], NULL, &tmp) == 0) || (tmp < 0) || (tmp > 1))
          {
            SHELL_E("Invalid DNS enable value\n");
            return SHELL_STATUS_ERROR;
          }
          dns_enable = (uint32_t)tmp;
          break;
        case 2:
          /* Parse the DNS1 address */
          Parser_StrToIP(argv[i], dns1_addr);
          if (Parser_CheckIP(dns1_addr) != 0)
          {
            SHELL_E("DNS IP 1 invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 3:
          /* Parse the DNS2 address */
          Parser_StrToIP(argv[i], dns2_addr);
          if (Parser_CheckIP(dns2_addr) != 0)
          {
            SHELL_E("DNS IP 2 invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
        case 4:
          /* Parse the DNS3 address */
          Parser_StrToIP(argv[i], dns3_addr);
          if (Parser_CheckIP(dns3_addr) != 0)
          {
            SHELL_E("DNS IP 3 invalid\n");
            return SHELL_STATUS_ERROR;
          }
          break;
      }
    }

    /* Set the DNS configuration */
    if (W6X_WiFi_SetDnsAddress(&dns_enable, dns1_addr, dns2_addr, dns3_addr) == W6X_STATUS_OK)
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
}

/** Shell command to get/set the STA DNS configuration */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_STA_DNS, wifi_sta_dns,
                       wifi_sta_dns [ 0:default IPs; 1: manual IPs ] [ DNS1 addr ] [ DNS2 addr ] [ DNS3 addr ]);

int32_t W6X_Shell_WiFi_STA_MAC(int32_t argc, char **argv)
{
  uint8_t mac_addr[6] = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the STA MAC address */
  if (W6X_WiFi_GetStaMacAddress(mac_addr) == W6X_STATUS_OK)
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

/** Shell command to get the STA MAC address */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_STA_MAC, wifi_sta_mac, wifi_sta_mac);

int32_t W6X_Shell_WiFi_STA_State(int32_t argc, char **argv)
{
  W6X_WiFi_StaStateType_e state = W6X_WIFI_STATE_STA_DISCONNECTED;
  W6X_WiFi_Connect_t ConnectData = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the STA state */
  if (W6X_WiFi_GetStaState(&state, &ConnectData) == W6X_STATUS_OK)
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

/** Shell command to get the STA state */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_STA_State, wifi_sta_state, wifi_sta_state);

int32_t W6X_Shell_WiFi_Country_Code(int32_t argc, char **argv)
{
  uint32_t policy = 0;
  int32_t tmp = 0;
  char countryString[3] = {0}; /* Alpha-2 code: 2 characters + null terminator */
  int32_t len = 0;

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
  }
  else if (argc == 3)
  {
    /* Get the country policy mode */
    if (Parser_StrToInt(argv[1], NULL, &tmp) == 0)
    {
      SHELL_E("First parameter should be 0 to disable, or 1 to enable Country policy\n");
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    if (!((tmp == 0) || (tmp == 1)))
    {
      SHELL_E("First parameter should be 0 to disable, or 1 to enable Country policy\n");
      return SHELL_STATUS_ERROR;
    }
    policy = (uint32_t)tmp;

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

/** Shell command to get/set the country code */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Country_Code, wifi_country_code,
                       wifi_country_code [ 0:AP aligned country code; 1:User country code ]
                       [ Country code [CN; JP; US; EU; 00] ]);

/** Shell command to start a WiFi Soft-AP */
int32_t W6X_Shell_WiFi_StartAP(int32_t argc, char **argv)
{
  W6X_WiFi_ApConfig_t ap_config = {0};
  int32_t current_arg = 1;
  int32_t tmp = 0;
  ap_config.MaxConnections = W6X_WIFI_MAX_CONNECTED_STATIONS;
  ap_config.Hidden = 0;
  ap_config.Channel = 1; /* default option when not defined */

  if (argc > 11)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (argc == 1)
  {
    if (W6X_WiFi_GetApConfig(&ap_config) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("AP SSID :     %s\n", ap_config.SSID);
      SHELL_PRINTF("AP Channel:   %" PRIu32 "\n", ap_config.Channel);
      SHELL_PRINTF("AP Security : %" PRIu32 "\n", ap_config.Security);
      return SHELL_STATUS_OK;
    }
    else
    {
      SHELL_E("Get Soft-AP configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }

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
      if (Parser_StrToInt(argv[current_arg], NULL, &tmp) == 0)
      {
        SHELL_E("Channel value out of range : [1;13]\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if ((tmp < 1) || (tmp > 13))
      {
        SHELL_E("Channel value out of range : [1;13]\n");
        return SHELL_STATUS_ERROR;
      }

      ap_config.Channel = (uint32_t)tmp;
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
      if (Parser_StrToInt(argv[current_arg], NULL, &tmp) == 0)
      {
        SHELL_E("Security not supported in Soft-AP mode, range [0;4], WEP (=1) not supported\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if ((tmp < 0) || (tmp > 4) || (tmp == 1))
      {
        SHELL_E("Security not supported in Soft-AP mode, range [0;4], WEP (=1) not supported\n");
        return SHELL_STATUS_ERROR;
      }

      ap_config.Security = (W6X_WiFi_ApSecurityType_e)tmp;
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
      if (Parser_StrToInt(argv[current_arg], NULL, &tmp) == 0)
      {
        SHELL_E("Hidden value out of range [0;1]\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if ((tmp < 0) || (tmp > 1))
      {
        SHELL_E("Hidden value out of range [0;1]\n");
        return SHELL_STATUS_ERROR;
      }

      ap_config.Hidden = (uint32_t)tmp;
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
  if (W6X_WiFi_StartAp(&ap_config) == W6X_STATUS_OK)
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

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_StartAP, wifi_ap_start,
                       wifi_ap_start [ -s SSID ] [ -p Password ]
                       [ -c channel [1; 13] ] [ -e security [0:Open; 2:WPA; 3:WPA2; 4:WPA3] ] [ -h hidden [0; 1] ]);

/** Shell command to stop a WiFi Soft-AP */
int32_t W6X_Shell_WiFi_StopAP(int32_t argc, char **argv)
{
  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_WiFi_StopAp() == W6X_STATUS_OK)
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

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_StopAP, wifi_ap_stop, wifi_ap_stop);

int32_t W6X_Shell_WiFi_AP_Mode(int32_t argc, char **argv)
{
  W6X_WiFi_Mode_t mode = {0};
  int32_t tmp = 0;

  if (argc > 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (argc == 1)
  {
    if (W6X_WiFi_GetApMode(&mode) == W6X_STATUS_OK) /* Get the Soft-AP mode */
    {
      SHELL_PRINTF("Soft-AP mode : %" PRIu16 " (B:%" PRIu16 " G:%" PRIu16 " N:%" PRIu16 " AX:%" PRIu16 ")\n",
                   mode.byte, mode.bit.b_mode, mode.bit.g_mode, mode.bit.n_mode, mode.bit.ax_mode);
    }
    else
    {
      SHELL_PRINTF("Get Soft-AP mode failed\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else if (argc == 2)
  {
    /* Parse the Soft-AP mode */
    if (Parser_StrToInt(argv[1], NULL, &tmp) == 0)
    {
      SHELL_E("Invalid Soft-AP mode\n");
      return SHELL_STATUS_ERROR;
    }
    mode.byte = (uint8_t)tmp;

    if (W6X_WiFi_SetApMode(&mode) == W6X_STATUS_OK) /* Set the Soft-AP mode */
    {
      SHELL_PRINTF("Soft-AP mode set to : %" PRIu16 "\n", mode.byte);
    }
    else
    {
      SHELL_PRINTF("Set Soft-AP mode failed\n");
      return SHELL_STATUS_ERROR;
    }
  }

  return SHELL_STATUS_OK;
}

/** Shell command to get/set the STA mode */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AP_Mode, wifi_ap_mode, wifi_ap_mode [ mode ]);

/** Shell command to list stations connected to the Soft-AP */
int32_t W6X_Shell_WiFi_List_Stations(int32_t argc, char **argv)
{
  W6X_WiFi_Connected_Sta_t connected_sta = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (W6X_WiFi_ListConnectedSta(&connected_sta) == W6X_STATUS_OK)
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

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_List_Stations, wifi_ap_list_sta, wifi_ap_list_sta);

/** Shell command to disconnect a station connected to the Soft-AP */
int32_t W6X_Shell_WiFi_Disconnect_Station(int32_t argc, char **argv)
{
  uint32_t temp[6] = {0};
  uint8_t mac_addr[6] = {0};

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the MAC address */
  if (sscanf(argv[1], "%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32 ":%02" SCNx32,
             &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]) != 6)
  {
    SHELL_E("MAC address is not valid.\n");
    return SHELL_STATUS_ERROR;
  }
  /* Check the MAC address validity */
  for (int32_t i = 0; i < 6; i++)
  {
    if (temp[i] > 0xFF)
    {
      SHELL_E("MAC address is not valid.\n");
      return SHELL_STATUS_ERROR;
    }
    mac_addr[i] = (uint8_t)temp[i];
  }
  if (W6X_WiFi_DisconnectSta(mac_addr) == W6X_STATUS_OK)
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

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Disconnect_Station, wifi_ap_disconnect_sta, wifi_ap_disconnect_sta < MAC >);

/** Shell command to get the Soft-AP IP configuration */
int32_t W6X_Shell_WiFi_Get_AP_IP(int32_t argc, char **argv)
{
  uint8_t ip_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the Soft-AP IP configuration */
  if (W6X_WiFi_GetApIpAddress(ip_addr, netmask_addr) == W6X_STATUS_OK)
  {
    /* Display the IP configuration */
    SHELL_PRINTF("Soft-AP IP : " IPSTR "\n", IP2STR(ip_addr));
    SHELL_PRINTF("Netmask IP : " IPSTR "\n", IP2STR(netmask_addr));
  }
  else
  {
    SHELL_E("Get Soft-AP IP error\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_Get_AP_IP, wifi_ap_ip, wifi_ap_ip);

int32_t W6X_Shell_WiFi_DHCP_Config(int32_t argc, char **argv)
{
  uint8_t start_ip[4] = {0};
  uint8_t end_ip[4] = {0};
  uint32_t lease_time = 0;
  uint32_t operate = 0;
  int32_t tmp = 0;
  W6X_WiFi_DhcpType_e state = W6X_WIFI_DHCP_DISABLED;

  if ((argc == 2) || (argc > 4))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (argc == 1)
  {
    /* Get the DHCP server configuration */
    if (W6X_WiFi_GetDhcp(&state, &lease_time, start_ip, end_ip) == W6X_STATUS_OK)
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
  }
  else
  {
    /* Get the DHCP client requested mode */
    if (Parser_StrToInt(argv[1], NULL, &tmp) == 0)
    {
      SHELL_E("First parameter should be 0 to disable, or 1 to enable DHCP client\n");
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    if ((tmp < 0) || (tmp > 1))
    {
      SHELL_E("First parameter should be 0 to disable, or 1 to enable DHCP client\n");
      return SHELL_STATUS_ERROR;
    }
    /** Global configuration of the DHCP */
    operate = tmp;

    /* Get the DHCP client requested mask: 0b01 for STA only, 0b10 for Soft-AP only, 0b11 for STA + Soft-AP */
    if (Parser_StrToInt(argv[2], NULL, &tmp) == 0)
    {
      SHELL_E("Second parameter should be 1 to configure STA only, 2 to Soft-AP only, 3 for STA + Soft-AP. "
              "0 won't configure any\n");
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    if (!((tmp == W6X_WIFI_DHCP_STA_ENABLED) ||
          (tmp == W6X_WIFI_DHCP_AP_ENABLED) || (tmp == W6X_WIFI_DHCP_STA_AP_ENABLED)))
    {
      SHELL_E("Second parameter should be 1 to configure STA only, 2 to Soft-AP only, 3 for STA + Soft-AP. "
              "0 won't configure any\n");
      return SHELL_STATUS_ERROR;
    }
    state = (W6X_WiFi_DhcpType_e)tmp;

    if (argc == 4)
    {
      /* DHCP Server configuration */
      /* Parse the lease time */
      if (Parser_StrToInt(argv[3], NULL, &tmp) == 0)
      {
        SHELL_E("Lease time out of range [1;2880]\n");
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      if ((tmp < 1) || (tmp > 2880))
      {
        SHELL_E("Lease time out of range [1;2880]\n");
        return SHELL_STATUS_ERROR;
      }

      lease_time = (uint32_t)tmp;
    }

    if (W6X_WiFi_SetDhcp(&state, &operate, lease_time) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("DHCP configuration succeed\n");
    }
    else
    {
      SHELL_E("DHCP configuration failed\n");
      return SHELL_STATUS_ERROR;
    }
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_DHCP_Config, wifi_dhcp,
                       wifi_dhcp [ 0:DHCP disabled; 1:DHCP enabled ]
                       [ 1:STA only; 2:AP only; 3:STA + AP ] [ lease_time [1; 2880] ]);

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
    if (W6X_WiFi_GetApMacAddress(mac_addr) == W6X_STATUS_OK)
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

/** Shell command to get the STA MAC address */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_AP_MAC, wifi_ap_mac, wifi_ap_mac);

int32_t W6X_Shell_WiFi_DTIM(int32_t argc, char **argv)
{
  uint32_t dtim = 0;
  int32_t value = 0;

  if (argc == 1)
  {
    if (W6X_WiFi_GetDTIM(&dtim) != W6X_STATUS_OK)
    {
      SHELL_E("Get DTIM error\n");
      return SHELL_STATUS_ERROR;
    }
    SHELL_PRINTF("DTIM is %" PRIu32 "\n", dtim);
  }
  else if (argc == 2)
  {
    if (Parser_StrToInt(argv[1], NULL, &value) == 0)
    {
      SHELL_E("DTIM value should be between 0 and 10\n");
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    if (value < 0 || value > 10)
    {
      SHELL_E("DTIM value should be between 0 and 10\n");
      return SHELL_STATUS_ERROR;
    }

    if (W6X_WiFi_SetDTIM((uint32_t)value) != W6X_STATUS_OK)
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

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_DTIM, dtim, dtim < value [0; 10] >);

int32_t W6X_Shell_WiFi_SetupTWT(int32_t argc, char **argv)
{
  int32_t value = 0;
  W6X_WiFi_TWT_Setup_Params_t twt_params;

  if (argc != 6)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Setup type */
  if (Parser_StrToInt(argv[1], NULL, &value) == 0)
  {
    SHELL_E("TWT setup type should be between [0; 2]\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  if ((value < 0) || (value > 2))
  {
    SHELL_E("TWT setup type should be between [0; 2]\n");
    return SHELL_STATUS_ERROR;
  }
  twt_params.setup_type = (uint8_t)value;

  /* Flow type */
  if (Parser_StrToInt(argv[2], NULL, &value) == 0)
  {
    SHELL_E("TWT flow type should be between [0; 1]\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  if ((value < 0) || (value > 1))
  {
    SHELL_E("TWT flow type should be between [0; 1]\n");
    return SHELL_STATUS_ERROR;
  }
  twt_params.flow_type = (uint8_t)value;

  /* Wake interval exponent */
  if (Parser_StrToInt(argv[3], NULL, &value) == 0)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  twt_params.wake_int_exp = (uint8_t)value;

  /* Minimum wake duration */
  if (Parser_StrToInt(argv[4], NULL, &value) == 0)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  twt_params.min_twt_wake_dur = (uint8_t)value;

  /* Wake interval mantissa */
  if (Parser_StrToInt(argv[5], NULL, &value) == 0)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  twt_params.wake_int_mantissa = (uint16_t)value;

  if (W6X_WiFi_SetupTWT(&twt_params) != W6X_STATUS_OK)
  {
    SHELL_E("Could not setup TWT\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_SetupTWT, wifi_twt_setup,
                       wifi_twt_setup < setup_type(0: request; 1: suggest; 2: demand) >
                       < flow_type(0: announced; 1: unannounced) >
                       < wake_int_exp > < min_wake_duration > < wake_int_mantissa >);

int32_t W6X_Shell_WiFi_SetTWT(int32_t argc, char **argv)
{
  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Set TWT */
  if (W6X_WiFi_SetTWT() != W6X_STATUS_OK)
  {
    SHELL_E("Could not set TWT\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_SetTWT, wifi_twt_set, wifi_twt_set);

int32_t W6X_Shell_WiFi_TeardownTWT(int32_t argc, char **argv)
{
  int32_t value = 0;
  W6X_WiFi_TWT_Teardown_Params_t twt_params = {0};

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Teardown type */
  if (Parser_StrToInt(argv[1], NULL, &value) == 0)
  {
    SHELL_E("TWT teardown type should be between [0; 2]\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  if ((value < 0) || (value > 2))
  {
    SHELL_E("TWT teardown type should be between [0; 2]\n");
    return SHELL_STATUS_ERROR;
  }
  if (value == 2)
  {
    twt_params.all_twt = 1;
  }
  else
  {
    twt_params.id = (uint8_t)value;
  }

  /* Teardown TWT */
  if (W6X_WiFi_TeardownTWT(&twt_params) != W6X_STATUS_OK)
  {
    SHELL_E("Could not teardown TWT\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(W6X_Shell_WiFi_TeardownTWT, wifi_twt_teardown,
                       wifi_twt_teardown < 0: announced; 1: unannounced >; 2: all >);

/** @} */
