/**
  ******************************************************************************
  * @file    w6x_ble_shell.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x BLE Shell Commands
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
#include "event_groups.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_BLE_Constants ST67W6X BLE Constants
  * @ingroup  ST67W6X_Private_BLE
  * @{
  */

#define EVENT_FLAG_SCAN_DONE  (1<<1)  /*!< Scan done event bitmask */

#define SCAN_TIMEOUT          10000   /*!< Delay before to declare the scan in failure */

#ifndef BDADDR2STR
/** BD Address buffer to string macros */
#define BDADDR2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif /* BDADDR2STR */

#ifndef BDADDRSTR
/** BD address string format */
#define BDADDRSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif /* BDADDRSTR */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @addtogroup ST67W6X_Private_BLE_Variables
  * @{
  */
uint8_t a_SHELL_AvailableData[247] = {0}; /*!< Available data buffer */

/** Event bitmask flag used for asynchronous execution */
static EventGroupHandle_t scan_event = NULL;

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Private_BLE_Functions
  * @{
  */
/**
  * @brief  BLE Scan callback function
  * @param  entry: scan result entry
  */
void W6X_Shell_Ble_Scan_cb(W6X_Ble_Scan_Result_t *entry);

/**
  * @brief  BLE Scan print function
  * @param  Scan_results: pointer to scan results
  */
void W6X_Shell_Ble_Print_Scan(W6X_Ble_Scan_Result_t *Scan_results);

/**
  * @brief  BLE Init shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_Init(int32_t argc, char **argv);

/**
  * @brief BLE Start advertising shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_AdvStart(int32_t argc, char **argv);

/**
  * @brief BLE Stop advertising shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_AdvStop(int32_t argc, char **argv);

/**
  * @brief  BLE scan shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ScanStart(int32_t argc, char **argv);

/**
  * @brief  BLE stop scan shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ScanStop(int32_t argc, char **argv);

/**
  * @brief  Get/set BLE connection function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_Connect(int32_t argc, char **argv);

/**
  * @brief  BLE disconnection function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_Disconnect(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
void W6X_Shell_Ble_Print_Scan(W6X_Ble_Scan_Result_t *Scan_results)
{
  /* Print the scan results */
  for (uint32_t count = 0; count < Scan_results->Count; count++)
  {
    /* Print the mandatory fields from the scan results */
    SHELL_PRINTF("Scanned device: Addr : " BDADDRSTR ", RSSI : %ld  %s\r\n",
                 BDADDR2STR(Scan_results->Detected_Peripheral[count].BDAddr),
                 Scan_results->Detected_Peripheral[count].RSSI, Scan_results->Detected_Peripheral[count].DeviceName);
    vTaskDelay(15); /* Wait few ms to avoid logging buffer overflow */
  }
}

void W6X_Shell_Ble_Scan_cb(W6X_Ble_Scan_Result_t *entry)
{
  LogInfo(" Cb informed APP that BLE SCAN DONE.");
  if (entry->Count == 0)
  {
    LogInfo("No scan results");
  }
  else
  {
    W6X_Shell_Ble_Print_Scan(entry);
    /* Set the scan done event */
    xEventGroupSetBits(scan_event, EVENT_FLAG_SCAN_DONE);
  }
}

int32_t W6X_Shell_Ble_Init(int32_t argc, char **argv)
{
  int32_t mode = 0;
  int32_t ret = W6X_STATUS_ERROR;

  if (argc == 1)
  {
    /* Get the auto connect current state */
    if (W6X_Ble_GetInitMode((W6X_Ble_Mode_e *) &mode) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get BLE Init Mode: %d\r\n", mode);
    }
    else
    {
      SHELL_PRINTF("Get BLE Init Mode failed\r\n");
      return W6X_STATUS_ERROR;
    }
  }
  else if (argc == 2)
  {
    Parser_StrToInt(argv[1], NULL, &mode);
    /* BLE Init */
    if (W6X_Ble_Init((W6X_Ble_Mode_e) mode, a_SHELL_AvailableData,
                     sizeof(a_SHELL_AvailableData) - 1) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Init: Mode = %d\r\n", mode);
      ret = W6X_STATUS_OK;
    }
    else
    {
      SHELL_PRINTF("BLE Init failed\r\n");
    }
  }
  else
  {
    SHELL_PRINTF("BLE Init failed: Mode must be 1 or 2\r\n");
  }
  return ret;
}

/** Shell command to Initialize BLE  */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Init, ble_init, ble_init [ 1: client mode; 2:server mode ]);

int32_t W6X_Shell_Ble_AdvStart(int32_t argc, char **argv)
{
  int32_t ret = W6X_STATUS_ERROR;
  if (argc != 1)
  {
    SHELL_E("Too many arguments\r\n");
    return ret;
  }
  ret = W6X_Ble_AdvStart();
  return ret;
}

/** Shell command to Start BLE advertising */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_AdvStart, ble_adv_start, ble_adv_start);

int32_t W6X_Shell_Ble_AdvStop(int32_t argc, char **argv)
{
  int32_t ret = W6X_STATUS_ERROR;
  if (argc != 1)
  {
    SHELL_E("Too many arguments\r\n");
    return ret;
  }
  ret = W6X_Ble_AdvStop();
  return ret;
}

/** Shell command to Stop BLE advertising */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_AdvStop, ble_adv_stop, ble_adv_stop);

int32_t W6X_Shell_Ble_ScanStart(int32_t argc, char **argv)
{
  int32_t ret = W6X_STATUS_ERROR;
  /* Initialize the scan event */
  if (scan_event == NULL)
  {
    scan_event = xEventGroupCreate();
  }

  if (argc != 1)
  {
    SHELL_E("Too many arguments\r\n");
    return ret;
  }

  /* Start the BLE scan */
  if (W6X_STATUS_OK == W6X_Ble_StartScan(W6X_Shell_Ble_Scan_cb))
  {
    /* Wait for the scan to be done */
    if ((int32_t)xEventGroupWaitBits(scan_event, EVENT_FLAG_SCAN_DONE, pdTRUE, pdFALSE,
                                     SCAN_TIMEOUT / portTICK_PERIOD_MS) != EVENT_FLAG_SCAN_DONE)
    {
      /* Scan timeout */
      SHELL_PRINTF("No device found\r\n");
    }
    SHELL_PRINTF("Stop Scan\r\n");
    ret = W6X_Ble_StopScan();
  }
  return ret;
}

/** Shell command to scan for BLE devices */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ScanStart, ble_start_scan, ble_start_scan);

int32_t W6X_Shell_Ble_ScanStop(int32_t argc, char **argv)
{
  int32_t ret = W6X_STATUS_ERROR;

  if (scan_event == NULL)
  {
    scan_event = xEventGroupCreate();
  }

  if (argc != 1)
  {
    SHELL_E("Too many arguments\r\n");
    return ret;
  }

  /* Stop the BLE scan */
  ret = W6X_Ble_StopScan();
  return ret;
}

/** Shell command to stop BLE scan */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ScanStop, ble_stop_scan, ble_stop_scan);

int32_t W6X_Shell_Ble_Connect(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;
  int32_t tmp = 0;
  uint8_t ble_bd_addr[6] = {0};

  if (argc > 3)
  {
    SHELL_E("Too many arguments\r\n");
    return W6X_STATUS_ERROR;
  }

  else if (argc == 1)
  {
    /* Get the connection information */
    if (W6X_Ble_GetConn(&conn_handle, ble_bd_addr) == W6X_STATUS_OK)
    {
      /* Display the connection information */
      SHELL_PRINTF("Connection handle : %d\r\n", conn_handle);
      SHELL_PRINTF("BD Addr : " MACSTR "\r\n", MAC2STR(ble_bd_addr));
    }
    else
    {
      SHELL_E("Get BLE Connection error\r\n");
      return W6X_STATUS_ERROR;
    }
  }
  else if (argc == 3)
  {
    Parser_StrToInt(argv[1], NULL, &tmp);
    Parser_StrToMAC(argv[2], ble_bd_addr);
    conn_handle = tmp;

    /* Establish BLE Connection */
    if (W6X_Ble_Connect((uint32_t)conn_handle, ble_bd_addr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Connection success\r\n");
    }
    else
    {
      SHELL_E("BLE Connection failed\r\n");
      return W6X_STATUS_ERROR;
    }
  }
  else
  {
    SHELL_E("Missing argument\r\n");
    return W6X_STATUS_ERROR;
  }
  return W6X_STATUS_OK;
}

/** Shell command to connect to a BLE device */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Connect, ble_connect, ble_connect [ conn handle: 0 or 1 ] [ BD addr ]);

int32_t W6X_Shell_Ble_Disconnect(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;
  int32_t tmp = 0;

  if (argc > 2)
  {
    SHELL_E("Too many arguments\r\n");
    return W6X_STATUS_ERROR;
  }

  else if (argc == 2)
  {
    Parser_StrToInt(argv[1], NULL, &tmp);
    conn_handle = tmp;

    /* Disconnect */
    if (W6X_Ble_Disconnect(conn_handle) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Disconnection success\r\n");
    }
    else
    {
      SHELL_E("BLE Disconnection failed\r\n");
      return W6X_STATUS_ERROR;
    }
  }
  else
  {
    SHELL_E("Missing argument\r\n");
    return W6X_STATUS_ERROR;
  }
  return W6X_STATUS_OK;
}

/** Shell command to disconnect from remote BLE device */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Disconnect, ble_disconnect, ble_disconnect [ conn handle: 0 or 1 ]);

/** @} */
