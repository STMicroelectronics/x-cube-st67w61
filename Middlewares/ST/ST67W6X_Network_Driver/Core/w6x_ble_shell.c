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
/** BD Address string format */
#define BDADDRSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif /* BDADDRSTR */

#define MAX_BLE_TX_POWER  20  /*!< Maximum value of BLE TX power */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @addtogroup ST67W6X_Private_BLE_Variables
  * @{
  */

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
  * @brief  BLE Deinit shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_DeInit(int32_t argc, char **argv);

/**
  * @brief  BLE Start/Stop advertising shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_Adv(int32_t argc, char **argv);

/**
  * @brief  BLE Start/Stop scan shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_Scan(int32_t argc, char **argv);

/**
  * @brief  BLE Get/set connection function
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

/**
  * @brief  BLE set/get TX power
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_TxPower(int32_t argc, char **argv);

/**
  * @brief  BLE set/get BD Address function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_BdAddress(int32_t argc, char **argv);

/**
  * @brief  BLE set/get Device name function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_DeviceName(int32_t argc, char **argv);

/**
  * @brief  BLE set Advertising data function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_AdvData(int32_t argc, char **argv);

/**
  * @brief  BLE set/get Advertising parameters function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_AdvParam(int32_t argc, char **argv);

/**
  * @brief  BLE set/get Scan parameters function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ScanParam(int32_t argc, char **argv);

/**
  * @brief  BLE set Scan response data function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ScanRespData(int32_t argc, char **argv);

/**
  * @brief  BLE set/get Connection parameters function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ConnParam(int32_t argc, char **argv);

/**
  * @brief  BLE get Connection information function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_GetConn(int32_t argc, char **argv);

/**
  * @brief  BLE initiate MTU exchange function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ExchangeMTU(int32_t argc, char **argv);

/**
  * @brief  BLE set Data Length function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_DataLength(int32_t argc, char **argv);

/**
  * @brief  BLE service creation function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_CreateService(int32_t argc, char **argv);

/**
  * @brief  BLE service deletion function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_DeleteService(int32_t argc, char **argv);

/**
  * @brief  BLE characteristic creation function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_CreateCharacteristic(int32_t argc, char **argv);

/**
  * @brief  BLE get services and characteristic registered function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_GetServicesAndCharacteristics(int32_t argc, char **argv);

/**
  * @brief  BLE services and characteristics register function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_RegisterServiceAndCharac(int32_t argc, char **argv);

/**
  * @brief  BLE services discovery function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_RemoteServiceDiscovery(int32_t argc, char **argv);

/**
  * @brief  BLE characteristics discovery function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_RemoteCharDiscovery(int32_t argc, char **argv);

/**
  * @brief  BLE send notification function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ServerSendNotification(int32_t argc, char **argv);

/**
  * @brief  BLE send indication function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ServerSendIndication(int32_t argc, char **argv);

/**
  * @brief  BLE set read data function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ServerSetReadData(int32_t argc, char **argv);

/**
  * @brief  BLE client write data function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ClientWriteData(int32_t argc, char **argv);

/**
  * @brief  BLE client read data function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ClientReadData(int32_t argc, char **argv);

/**
  * @brief  BLE client subscribe to characteristic function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ClientSubscribeChar(int32_t argc, char **argv);

/**
  * @brief  BLE client unsubscribe to characteristic function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_ClientUnsubscribeChar(int32_t argc, char **argv);

/**
  * @brief  BLE set/get security parameter function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecurityParam(int32_t argc, char **argv);

/**
  * @brief  BLE security start function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecurityStart(int32_t argc, char **argv);

/**
  * @brief  BLE security passkey confirm function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecurityPassKeyConfirm(int32_t argc, char **argv);

/**
  * @brief  BLE security pairing confirm function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecurityPairingConfirm(int32_t argc, char **argv);

/**
  * @brief  BLE security pairing cancel function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecurityPairingCancel(int32_t argc, char **argv);

/**
  * @brief  BLE security unpair function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecurityUnpair(int32_t argc, char **argv);

/**
  * @brief  BLE get bonded devices list function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecurityGetBondedDeviceList(int32_t argc, char **argv);

/**
  * @brief  BLE set security passkey function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval 0 in case of success, -1 otherwise
  */
int32_t W6X_Shell_Ble_SecuritySetPassKey(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
void W6X_Shell_Ble_Print_Scan(W6X_Ble_Scan_Result_t *Scan_results)
{
  if ((Scan_results == NULL) || (Scan_results->Detected_Peripheral == NULL))
  {
    return;
  }

  /* Print the scan results */
  for (uint32_t count = 0; count < Scan_results->Count; count++)
  {
    /* Print the mandatory fields from the scan results */
    SHELL_PRINTF("[" BDADDRSTR "] RSSI: %4" PRIi16 "| Name: %s\n",
                 BDADDR2STR(Scan_results->Detected_Peripheral[count].BDAddr),
                 Scan_results->Detected_Peripheral[count].RSSI, Scan_results->Detected_Peripheral[count].DeviceName);
    vTaskDelay(15); /* Wait few ms to avoid logging buffer overflow */
  }
}

void W6X_Shell_Ble_Scan_cb(W6X_Ble_Scan_Result_t *entry)
{
  if (scan_event != NULL)
  {
    SHELL_PRINTF(" Cb informed APP that BLE SCAN DONE\n");
    if (entry->Count == 0)
    {
      SHELL_PRINTF("No scan results\n");
    }
    else
    {
      W6X_Shell_Ble_Print_Scan(entry);
      /* Set the scan done event */
      xEventGroupSetBits(scan_event, EVENT_FLAG_SCAN_DONE);
    }
  }
}

int32_t W6X_Shell_Ble_Init(int32_t argc, char **argv)
{
  int32_t mode = 0;

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get the auto connect current state */
    if (W6X_Ble_GetInitMode((W6X_Ble_Mode_e *) &mode) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get BLE Init Mode: %" PRIu32 "\n", mode);
    }
    else
    {
      SHELL_E("Get BLE Init Mode Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */
  if (argc == 2)
  {
    /* BLE Init */
    mode = atoi(argv[1]);
    if (W6X_Ble_Init((W6X_Ble_Mode_e) mode, NULL, 0) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Init: Mode = %" PRIu32 "\n", mode);
    }
    else
    {
      SHELL_E("BLE Init Failure\n");
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
/** Shell command to Initialize BLE */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Init, ble_init, ble_init [ 1: client mode; 2: server mode; 3:dual mode ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_DeInit(int32_t argc, char **argv)
{
  /* BLE deinitialize */
  W6X_Ble_DeInit();
  SHELL_PRINTF("BLE DeInit\n");
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to deinitialize BLE */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_DeInit, ble_deinit, ble_deinit);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_Adv(int32_t argc, char **argv)
{
  if (argc > 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if ((argc == 2) && (strcmp(argv[1], "-a") == 0))
  {
    /* BLE Stop advertising */
    if (W6X_Ble_AdvStop() == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Advertising stop OK\n");
    }
    else
    {
      SHELL_E("BLE Advertising stop Failure\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    /* BLE Start advertising */
    if (W6X_Ble_AdvStart() == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Advertising start OK\n");
    }
    else
    {
      SHELL_E("BLE Advertising start Failure\n");
      return SHELL_STATUS_ERROR;
    }
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to Start/Stop BLE advertising */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Adv, ble_adv, ble_adv [ -a abort adv ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_Scan(int32_t argc, char **argv)
{
  if (argc > 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if ((argc == 2) && (strcmp(argv[1], "-a") == 0))
  {
    /* Stop the BLE scan */
    if (W6X_Ble_StopScan() == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Stop Scan OK\n");
    }
    else
    {
      SHELL_E("BLE Stop Scan Failure\n");
      return SHELL_STATUS_ERROR;
    }
  }
  else
  {
    /* Initialize the scan event */
    if (scan_event == NULL)
    {
      scan_event = xEventGroupCreate();
    }
    /* Start the BLE scan */
    if (W6X_Ble_StartScan(W6X_Shell_Ble_Scan_cb) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Start Scan OK\n");
      /* Wait for the scan to be done */
      if ((int32_t)xEventGroupWaitBits(scan_event, EVENT_FLAG_SCAN_DONE, pdTRUE, pdFALSE,
                                       pdMS_TO_TICKS(SCAN_TIMEOUT)) != EVENT_FLAG_SCAN_DONE)
      {
        /* Scan timeout */
        SHELL_PRINTF("No device found\n");
      }
      SHELL_PRINTF("Stop Scan\n");
      W6X_Ble_StopScan();
    }
    else
    {
      SHELL_E("BLE Start Scan Failure\n");
      return SHELL_STATUS_ERROR;
    }
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to Start/Stop scan for BLE devices */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Scan, ble_scan, ble_scan [ -a abort scan ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_Connect(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;
  uint8_t ble_bd_addr[6] = {0};

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get the connection information */
    if (W6X_Ble_GetConn(&conn_handle, ble_bd_addr) == W6X_STATUS_OK)
    {
      if (conn_handle == 0xFF)
      {
        SHELL_PRINTF("No BLE Connection established\n");
        return SHELL_STATUS_OK;
      }
      /* Display the connection information */
      SHELL_PRINTF("Get BLE Connection OK\n");
      SHELL_PRINTF("Connection handle : %" PRIu32 "\n", conn_handle);
      SHELL_PRINTF("BD Addr : " MACSTR "\n", MAC2STR(ble_bd_addr));
    }
    else
    {
      SHELL_E("Get BLE Connection Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 3)
  {
    conn_handle = (uint32_t)atoi(argv[1]);
    if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    Parser_StrToMAC(argv[2], ble_bd_addr);

    /* Establish BLE Connection */
    if (W6X_Ble_Connect(conn_handle, ble_bd_addr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Connection OK\n");
    }
    else
    {
      SHELL_E("BLE Connection Failure\n");
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
/** Shell command to connect to a BLE device */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Connect, ble_connect, ble_connect < Conn Handle [0; 1] > < BD Addr >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_Disconnect(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  else
  {
    conn_handle = (uint32_t)atoi(argv[1]);
    if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Disconnect */
    if (W6X_Ble_Disconnect(conn_handle) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BLE Disconnection OK\n");
    }
    else
    {
      SHELL_E("BLE Disconnection Failure\n");
      return SHELL_STATUS_ERROR;
    }
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to disconnect from remote BLE device */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_Disconnect, ble_disconnect, ble_disconnect < Conn Handle [0; 1] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_TxPower(int32_t argc, char **argv)
{
  uint32_t tx_power = 0;

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get TX Power */
    if (W6X_Ble_GetTxPower(&tx_power) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get TxPower OK: %" PRIu32 "\n", tx_power);
    }
    else
    {
      SHELL_E("Get TxPower Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 2)
  {
    tx_power = (uint32_t)atoi(argv[1]);
    if (tx_power > MAX_BLE_TX_POWER)
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Set TX power */
    if (W6X_Ble_SetTxPower(tx_power) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Tx Power set OK: %" PRIu32 "\n", tx_power);
    }
    else
    {
      SHELL_E("Set Tx Power Failure\n");
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
/** Shell command to set/get the BLE TX Power */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_TxPower, ble_tx_power, ble_tx_power [ Tx Power [0; 20] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_BdAddress(int32_t argc, char **argv)
{
  uint8_t bdaddr[6] = {0};

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get BD Address */
    if (W6X_Ble_GetBDAddress(bdaddr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   bdaddr[0], bdaddr[1], bdaddr[2], bdaddr[3], bdaddr[4], bdaddr[5]);
    }
    else
    {
      SHELL_E("Get BD_ADDR Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 2)
  {
    /* Parse the Bluetooth Device Address */
    if (strlen(argv[1]) != 17)
    {
      SHELL_E("Invalid BD_ADDR length. Expected 12 hexadecimal characters\n");
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    Parser_StrToMAC(argv[1], bdaddr);

    /* Set BD Address */
    if (W6X_Ble_SetBdAddress(bdaddr) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Set BD_ADDR OK\n");
    }
    else
    {
      SHELL_E("Set BD_ADDR Failure\n");
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
/** Shell command to set/get the BLE BD Address */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_BdAddress, ble_bd_addr, ble_bd_addr [ BD Addr ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_DeviceName(int32_t argc, char **argv)
{
  char device_name[W6X_BLE_DEVICE_NAME_SIZE + 1] = {0};

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get BLE device name */
    if (W6X_Ble_GetDeviceName(device_name) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get Device Name OK: %s\n", device_name);
    }
    else
    {
      SHELL_E("Get Device Name Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 2)
  {
    if (strlen(argv[1]) > W6X_BLE_DEVICE_NAME_SIZE)
    {
      SHELL_E("Device name is too long. Maximum length is %" PRIu32 " characters\n", W6X_BLE_DEVICE_NAME_SIZE);
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    strncpy(device_name, argv[1], W6X_BLE_DEVICE_NAME_SIZE);
    device_name[W6X_BLE_DEVICE_NAME_SIZE] = '\0'; /* Ensure null termination */

    /* Set Device Name */
    if (W6X_Ble_SetDeviceName(device_name) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Set Device Name OK: %s\n", device_name);
    }
    else
    {
      SHELL_E("Set Device Name Failure\n");
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
/** Shell command to set/get the Bluetooth Device Name */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_DeviceName, ble_device_name, ble_device_name [ Device Name ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_AdvData(int32_t argc, char **argv)
{
  char adv_data[(W6X_BLE_MAX_ADV_DATA_LENGTH * 2) + 1] = {0};

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (strlen(argv[1]) > (W6X_BLE_MAX_ADV_DATA_LENGTH * 2))
  {
    SHELL_E("Advertising data is too long. Maximum length is %" PRIu32 " characters\n",
            (W6X_BLE_MAX_ADV_DATA_LENGTH * 2));
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  strncpy(adv_data, argv[1], W6X_BLE_MAX_ADV_DATA_LENGTH * 2);
  adv_data[W6X_BLE_MAX_ADV_DATA_LENGTH * 2] = '\0';

  /* Set Advertising data */
  if (W6X_Ble_SetAdvData(adv_data) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Set Advertising Data OK\n");
  }
  else
  {
    SHELL_E("Set Advertising Data Failure\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to set the BLE advertising data */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_AdvData, ble_adv_data, ble_adv_data < Advertising Data >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_AdvParam(int32_t argc, char **argv)
{
  uint32_t adv_int_min = 0;
  uint32_t adv_int_max = 0;
  W6X_Ble_AdvType_e adv_type;
  W6X_Ble_AdvChannel_e adv_channel;

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get BLE Advertising parameters */
    if (W6X_Ble_GetAdvParam(&adv_int_min, &adv_int_max, &adv_type, &adv_channel) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get Advertising Parameters OK:\n");
      SHELL_PRINTF("  AdvIntMin: %" PRIu32 "\n", adv_int_min);
      SHELL_PRINTF("  AdvIntMax: %" PRIu32 "\n", adv_int_max);
      SHELL_PRINTF("  AdvType: %" PRIu32 "\n", adv_type);
      SHELL_PRINTF("  AdvChannel: %" PRIu32 "\n", adv_channel);
    }
    else
    {
      SHELL_E("Get Advertising Parameters Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 5)
  {
    /* Parse the minimum advertising interval */
    adv_int_min = (uint32_t)atoi(argv[1]);
    if ((adv_int_min < 0x0020) || (adv_int_min > 0x4000))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the maximum advertising interval */
    adv_int_max = (uint32_t)atoi(argv[2]);
    if ((adv_int_max < 0x0020) || (adv_int_max > 0x4000) || (adv_int_max < adv_int_min))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the advertising type */
    adv_type = (W6X_Ble_AdvType_e)atoi(argv[3]);
    if ((adv_type != W6X_BLE_ADV_TYPE_IND) && (adv_type != W6X_BLE_ADV_TYPE_SCAN_IND) &&
        (adv_type != W6X_BLE_ADV_TYPE_NONCONN_IND))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the advertising channel */
    adv_channel = (W6X_Ble_AdvChannel_e)atoi(argv[4]);
    if ((adv_channel != W6X_BLE_ADV_CHANNEL_37) && (adv_channel != W6X_BLE_ADV_CHANNEL_38) &&
        (adv_channel != W6X_BLE_ADV_CHANNEL_39) && (adv_channel != W6X_BLE_ADV_CHANNEL_ALL))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Set Advertising parameters */
    if (W6X_Ble_SetAdvParam(adv_int_min, adv_int_max, adv_type, adv_channel) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Set Advertising Parameters OK:\n");
      SHELL_PRINTF("  AdvIntMin: %" PRIu32 "\n", adv_int_min);
      SHELL_PRINTF("  AdvIntMax: %" PRIu32 "\n", adv_int_max);
      SHELL_PRINTF("  AdvType: %" PRIu32 "\n", adv_type);
      SHELL_PRINTF("  AdvChannel: %" PRIu32 "\n", adv_channel);
    }
    else
    {
      SHELL_E("Set Advertising Parameters Failure\n");
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
/** Shell command to set/get the BLE advertising parameters */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_AdvParam, ble_adv_param,
                       ble_adv_param [ AdvIntMin [32; 16384] ] [ AdvIntMax [32; 16384] ] [ Adv Type [0; 2] ]
                       [Adv Channel [1: chan 37; 2: chan 38; 3: chan 39; 7: all] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ScanParam(int32_t argc, char **argv)
{
  W6X_Ble_ScanType_e scan_type;
  W6X_Ble_AddrType_e addr_type;
  W6X_Ble_FilterPolicy_e filter_policy;
  uint32_t scan_interval = 0;
  uint32_t scan_window = 0;

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get BLE Scan parameters */
    if (W6X_Ble_GetScanParam(&scan_type, &addr_type, &filter_policy, &scan_interval, &scan_window) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get Scan Parameters OK:\n");
      SHELL_PRINTF("  ScanType: %" PRIu32 "\n", scan_type);
      SHELL_PRINTF("  AddrType: %" PRIu32 "\n", addr_type);
      SHELL_PRINTF("  ScanFilter: %" PRIu32 "\n", filter_policy);
      SHELL_PRINTF("  ScanInterval: %" PRIu32 " (x0.625ms = %.3f ms)\n", scan_interval, scan_interval * 0.625);
      SHELL_PRINTF("  ScanWindow: %" PRIu32 " (x0.625ms = %.3f ms)\n", scan_window, scan_window * 0.625);
    }
    else
    {
      SHELL_E("Get Scan Parameters Failure \n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 6)
  {
    /* Parse the scan type */
    scan_type = (W6X_Ble_ScanType_e)atoi(argv[1]);
    if ((scan_type != W6X_BLE_SCAN_PASSIVE) && (scan_type != W6X_BLE_SCAN_ACTIVE))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the own address type */
    addr_type = (W6X_Ble_AddrType_e)atoi(argv[2]);
    if ((addr_type != W6X_BLE_PUBLIC_ADDR) && (addr_type != W6X_BLE_RANDOM_ADDR) &&
        (addr_type != W6X_BLE_RPA_PUBLIC_ADDR) && (addr_type != W6X_BLE_RPA_RANDOM_ADDR))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the filter policy */
    filter_policy = (W6X_Ble_FilterPolicy_e)atoi(argv[3]);
    if ((filter_policy != W6X_BLE_SCAN_FILTER_ALLOW_ALL) &&
        (filter_policy != W6X_BLE_SCAN_FILTER_ALLOW_ONLY_WLST) &&
        (filter_policy != W6X_BLE_SCAN_FILTER_ALLOW_UND_RPA_DIR) &&
        (filter_policy != W6X_BLE_SCAN_FILTER_ALLOW_WLIST_PRA_DIR))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the scan interval */
    scan_interval = (uint32_t)atoi(argv[4]);
    if ((scan_interval < 4) || (scan_interval > 16384))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the scan window */
    scan_window = (uint32_t)atoi(argv[5]);
    if ((scan_window < 4) || (scan_window > 16384) || (scan_window > scan_interval))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Set Scan parameters */
    if (W6X_Ble_SetScanParam(scan_type, addr_type, filter_policy, scan_interval, scan_window) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Set Scan Parameters OK:\n");
      SHELL_PRINTF("  ScanType: %" PRIu32 "\n", scan_type);
      SHELL_PRINTF("  AddrType: %" PRIu32 "\n", addr_type);
      SHELL_PRINTF("  FilterPolicy: %" PRIu32 "\n", filter_policy);
      SHELL_PRINTF("  ScanInterval: %" PRIu32 "\n", scan_interval);
      SHELL_PRINTF("  ScanWindow: %" PRIu32 "\n", scan_window);
    }
    else
    {
      SHELL_E("Set Scan Parameters Failure\n");
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
/** Shell command to set/get the BLE scan parameters */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ScanParam, ble_scan_param,
                       ble_scan_param [ Scan Type [0; 1] ] [ OwnAddr Type [0; 3] ] [ Filter Policy [0; 3] ]
                       [ Scan Interval [4; 16384] ] [ Scan Window [4; 16384] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ScanRespData(int32_t argc, char **argv)
{
  char scan_resp_data[(W6X_BLE_MAX_SCAN_RESP_DATA_LENGTH * 2) + 1] = {0};

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if (strlen(argv[1]) > (W6X_BLE_MAX_SCAN_RESP_DATA_LENGTH * 2))
  {
    SHELL_E("Scan response data is too long. Maximum length is %" PRIu32 " characters\n",
            (W6X_BLE_MAX_SCAN_RESP_DATA_LENGTH * 2));
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  strncpy(scan_resp_data, argv[1], (W6X_BLE_MAX_SCAN_RESP_DATA_LENGTH * 2));
  scan_resp_data[(W6X_BLE_MAX_SCAN_RESP_DATA_LENGTH * 2)] = '\0';

  /* Set Scan Response data */
  if (W6X_Ble_SetScanRespData(scan_resp_data) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Set Scan Response Data OK: %s\n", scan_resp_data);
  }
  else
  {
    SHELL_E("Set Scan Response Data Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to set the BLE scan response data */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ScanRespData, ble_scanrespdata,
                       ble_scanrespdata < Scan Response Data >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ConnParam(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;
  uint32_t conn_int_min = 0;
  uint32_t conn_int_max = 0;
  uint32_t latency = 0;
  uint32_t timeout = 0;

#if (SHELL_CMD_LEVEL >= 1)
  uint32_t conn_int_current = 0;
  if (argc == 2)
  {
    /* Parse the connection handle */
    conn_handle = (uint32_t)atoi(argv[1]);
    if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Get connection parameters */
    if (W6X_Ble_GetConnParam(&conn_handle, &conn_int_min, &conn_int_max, &conn_int_current, &latency,
                             &timeout) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get Connection Parameters OK:\n");
      SHELL_PRINTF("  ConnHandle: %" PRIu32 "\n", conn_handle);
      SHELL_PRINTF("  ConnIntMin: %" PRIu32 " (x1.25ms = %.2f ms)\n", conn_int_min, conn_int_min * 1.25);
      SHELL_PRINTF("  ConnIntMax: %" PRIu32 " (x1.25ms = %.2f ms)\n", conn_int_max, conn_int_max * 1.25);
      SHELL_PRINTF("  ConnIntCurrent: %" PRIu32 " (x1.25ms = %.2f ms)\n", conn_int_current, conn_int_current * 1.25);
      SHELL_PRINTF("  Latency: %" PRIu32 "\n", latency);
      SHELL_PRINTF("  Timeout: %" PRIu32 " (x10ms = %.2f ms)\n", timeout, timeout * 10.0);
    }
    else
    {
      SHELL_E("Get Connection Parameters Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 6)
  {
    /* Parse the connection handle */
    conn_handle = (uint32_t)atoi(argv[1]);
    if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the minimum connection interval */
    conn_int_min = (uint32_t)atoi(argv[2]);
    if ((conn_int_min < 0x6) || (conn_int_min > 0xC80))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the maximum connection interval */
    conn_int_max = (uint32_t)atoi(argv[3]);
    if ((conn_int_max < 0x6) || (conn_int_max > 0xC80) || (conn_int_max < conn_int_min))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the connection latency */
    latency = (uint32_t)atoi(argv[4]);
    if (latency > 0x1F3)
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Parse the supervision timeout */
    timeout = (uint32_t)atoi(argv[5]);
    if ((timeout < 0xA) || (timeout > 0xC80))
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Set Connection parameters */
    if (W6X_Ble_SetConnParam(conn_handle, conn_int_min, conn_int_max, latency, timeout) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Set Connection Parameters OK:\n");
      SHELL_PRINTF("  ConnHandle: %" PRIu32 "\n", conn_handle);
      SHELL_PRINTF("  ConnIntMin: %" PRIu32 " (x1.25ms = %.2f ms)\n", conn_int_min, conn_int_min * 1.25);
      SHELL_PRINTF("  ConnIntMax: %" PRIu32 " (x1.25ms = %.2f ms)\n", conn_int_max, conn_int_max * 1.25);
      SHELL_PRINTF("  Latency: %" PRIu32 "\n", latency);
      SHELL_PRINTF("  Timeout: %" PRIu32 " (x10ms = %.2f ms)\n", timeout, timeout * 10.0);
    }
    else
    {
      SHELL_E("Set Connection Parameters Failure\n");
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
/** Shell command to set/get the BLE connection parameters */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ConnParam, ble_conn_param,
                       ble_conn_param [ Conn Handle [0; 1] ] [ ConnIntMin [6; 3200] ] [ConnIntMax [6; 3200] ]
                       [ Latency [0; 499] ] [ Timeout [10; 3200] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_GetConn(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;
  uint8_t remote_bdaddr[6] = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get connection information */
  if (W6X_Ble_GetConn(&conn_handle, remote_bdaddr) == W6X_STATUS_OK)
  {
    if (conn_handle == 0xFF)
    {
      SHELL_PRINTF("No BLE Connection established\n");
      return SHELL_STATUS_OK;
    }
    SHELL_PRINTF("Get Connection Information OK:\n");
    SHELL_PRINTF("  ConnHandle: %" PRIu32 "\n", conn_handle);
    SHELL_PRINTF("  Remote BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 remote_bdaddr[0], remote_bdaddr[1], remote_bdaddr[2],
                 remote_bdaddr[3], remote_bdaddr[4], remote_bdaddr[5]);
  }
  else
  {
    SHELL_E("Get Connection Information Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get the BLE connection information */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_GetConn, ble_get_conn, ble_get_conn);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ExchangeMTU(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Exchange MTU */
  if (W6X_Ble_ExchangeMTU(conn_handle) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Exchange MTU OK: %" PRIu32 "\n", conn_handle);
  }
  else
  {
    SHELL_E("Exchange MTU Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to initiate BLE MTU exchange */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ExchangeMTU, ble_exchange_mtu, ble_exchange_mtu < Conn Handle [0; 1] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_DataLength(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;
  uint32_t tx_bytes = 0;
  uint32_t tx_trans_time = 0;

  if (argc != 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the data packet length */
  tx_bytes = (uint32_t)atoi(argv[2]);
  if ((tx_bytes < 27) || (tx_bytes > 251))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the data packet transition time */
  tx_trans_time = (uint32_t)atoi(argv[3]);

  /* Set Data Length */
  if (W6X_Ble_SetDataLength(conn_handle, tx_bytes, tx_trans_time) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Set Data Length OK:\n");
    SHELL_PRINTF("  Conn Handle: %" PRIu32 "\n", conn_handle);
    SHELL_PRINTF("  TxBytes: %" PRIu32 "\n", tx_bytes);
    SHELL_PRINTF("  TxTransTime: %" PRIu32 "\n", tx_trans_time);
  }
  else
  {
    SHELL_E("Set Data Length Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to set the BLE data length parameters */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_DataLength, ble_data_length,
                       ble_data_length < Conn Handle [0; 1] > < TxBytes [27; 251] > < TxTransTime >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_CreateService(int32_t argc, char **argv)
{
  uint8_t service_index = 0;
  uint8_t uuid_type = 0;
  char service_uuid[((W6X_BLE_MAX_UUID_SIZE - 1) * 2) + 1] = {0};

  if (argc != 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[1]);
  if (service_index > (W6X_BLE_MAX_CREATED_SERVICE_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service UUID argument */
  if (strlen(argv[2]) > ((W6X_BLE_MAX_UUID_SIZE - 1) * 2))
  {
    SHELL_E("UUID is too long\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Copy the service UUID */
  strncpy(service_uuid, argv[2], ((W6X_BLE_MAX_UUID_SIZE - 1) * 2));
  service_uuid[((W6X_BLE_MAX_UUID_SIZE - 1) * 2)] = 0; /* Ensure end of UUID detection */

  /* Parse the UUID type argument */
  uuid_type = (uint8_t)atoi(argv[3]);

  /* Connect to the AP */
  if (W6X_Ble_CreateService(service_index, service_uuid, uuid_type) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Create BLE Service OK\n");
  }
  else
  {
    SHELL_E("Create BLE Service Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to create BLE services */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_CreateService, ble_srv_create,
                       ble_srv_create < Service Index [0; 2] > < UUID > < UUID type >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_DeleteService(int32_t argc, char **argv)
{
  uint8_t service_index = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[1]);
  if (service_index > (W6X_BLE_MAX_CREATED_SERVICE_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Delete Service */
  if (W6X_Ble_DeleteService(service_index) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Delete BLE Service OK: Service deleted %" PRIu32 "\n", service_index);
  }
  else
  {
    SHELL_E("Delete BLE Service Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to delete a BLE service */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_DeleteService, ble_srv_delete, ble_srv_delete < Service Index [0; 2] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_CreateCharacteristic(int32_t argc, char **argv)
{
  uint8_t service_index = 0;
  uint8_t char_index = 0;
  uint8_t uuid_type = 0;
  char char_uuid[((W6X_BLE_MAX_UUID_SIZE - 1) * 2) + 1] = {0};
  uint8_t char_property = 0;
  uint8_t char_permission = 0;

  if (argc != 7)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[1]);
  if (service_index > (W6X_BLE_MAX_CREATED_SERVICE_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic index argument */
  char_index = (uint8_t)atoi(argv[2]);
  if (char_index > (W6X_BLE_MAX_CHAR_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic UUID argument */
  if (strlen(argv[3]) > ((W6X_BLE_MAX_UUID_SIZE - 1) * 2))
  {
    SHELL_E("UUID is too long\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Copy the service UUID */
  strncpy(char_uuid, argv[3], ((W6X_BLE_MAX_UUID_SIZE - 1) * 2));
  char_uuid[((W6X_BLE_MAX_UUID_SIZE - 1) * 2)] = 0; /* Ensure end of UUID detection */

  /* Parse the UUID type argument */
  uuid_type = (uint8_t)atoi(argv[4]);

  /* Parse the characteristic property argument */
  char_property = (uint8_t)atoi(argv[5]);

  /* Parse the characteristic permission argument */
  char_permission = (uint8_t)atoi(argv[6]);

  /* Connect to the AP */
  if (W6X_Ble_CreateCharacteristic(service_index, char_index, char_uuid, uuid_type, char_property,
                                   char_permission) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Create BLE Characteristic OK\n");
  }
  else
  {
    SHELL_E("Create BLE Characteristic Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to create BLE characteristic */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_CreateCharacteristic, ble_char_create,
                       ble_char_create < Service Index [0; 2] > < Charac Index : [0; 4] > < UUID > < UUID type >
                       < Charac Property > < Charac Permission: read 1; write 2 >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_GetServicesAndCharacteristics(int32_t argc, char **argv)
{
  W6X_Ble_Service_t services_table[W6X_BLE_MAX_SERVICE_NBR] = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get BLE Services and Characteristics */
  if (W6X_Ble_GetServicesAndCharacteristics(services_table) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Get Services and Characteristics OK\n");
  }
  else
  {
    SHELL_E("Get Services and Characteristics Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get BLE services and characteristics */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_GetServicesAndCharacteristics, ble_srv_list, ble_srv_list);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_RegisterServiceAndCharac(int32_t argc, char **argv)
{
  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Register Services and Characteristics */
  if (W6X_Ble_RegisterCharacteristics() == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Register Services and Characteristics OK\n");
  }
  else
  {
    SHELL_E("Register Services and Characteristics Failure\n");
    return SHELL_STATUS_ERROR;
  }
  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to register BLE Services and Characteristics */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_RegisterServiceAndCharac, ble_srv_reg, ble_srv_reg);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_RemoteServiceDiscovery(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Remote Services discovery */
  if (W6X_Ble_RemoteServiceDiscovery((uint8_t)conn_handle) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Remote Service Discovery OK: Connection Handle %" PRIu32 "\n", conn_handle);
  }
  else
  {
    SHELL_E("Remote Service Discovery Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to initiate remote service discovery */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_RemoteServiceDiscovery, ble_rem_srv_list,
                       ble_rem_srv_list < Conn Handle [0; 1] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_RemoteCharDiscovery(int32_t argc, char **argv)
{
  uint32_t conn_handle = 0;
  uint8_t service_index = 0;

  if (argc != 3)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[2]);
  if (service_index > W6X_BLE_MAX_SERVICE_NBR)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Remote Characteristics discovery */
  if (W6X_Ble_RemoteCharDiscovery((uint8_t)conn_handle, service_index) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Remote Characteristic Discovery OK: Connection Handle %" PRIu32 " and Service Index %" PRIu32 "\n",
                 conn_handle, service_index);
  }
  else
  {
    SHELL_E("Remote Characteristic Discovery Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to initiate remote characteristic discovery */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_RemoteCharDiscovery, ble_rem_char_list,
                       ble_rem_char_list < Conn Handle [0; 1] > < Service Index [1; 4] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ServerSendNotification(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint8_t service_index = 0;
  uint8_t char_index = 0;
  uint8_t data_buffer[W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH] = {0};
  uint32_t req_len = 0;
  uint32_t sent_data_len = 0;
  uint32_t timeout = 0;

  if (argc != 6)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle argument */
  conn_handle = (uint8_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[2]);
  if (service_index > (W6X_BLE_MAX_CREATED_SERVICE_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic index argument */
  char_index = (uint8_t)atoi(argv[3]);
  if (char_index > (W6X_BLE_MAX_CHAR_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the timeout */
  timeout = (uint32_t)atoi(argv[4]);

  /* Parse the data hex format */
  req_len = strlen(argv[5]) / 2; /* Each byte is represented by 2 hex characters */
  if ((req_len == 0) || (req_len > W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH))
  {
    SHELL_E("Data length must be between 1 and %" PRIu32 " bytes\n", W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH);
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  for (uint32_t i = 0; i < req_len; i++)
  {
    char byte_str[3] = {argv[5][i * 2], argv[5][i * 2 + 1], '\0'};
    int32_t hex_value = Parser_StrToHex(byte_str, NULL);
    if (hex_value < 0)
    {
      SHELL_E("Invalid hex data format: %s\n", byte_str);
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    data_buffer[i] = (uint8_t)hex_value;
  }

  /* Send Notification */
  if (W6X_Ble_ServerNotify(conn_handle, service_index, char_index, data_buffer, req_len, &sent_data_len,
                           timeout) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Send Notification OK:\n");
    SHELL_PRINTF("  Service Index: %" PRIu32 "\n", service_index);
    SHELL_PRINTF("  Char Index: %" PRIu32 "\n", char_index);
    SHELL_PRINTF("  Data: %s\n", data_buffer);
    SHELL_PRINTF("  Requested Length: %" PRIu32 "\n", req_len);
    SHELL_PRINTF("  Sent Length: %" PRIu32 "\n", sent_data_len);
    SHELL_PRINTF("  Timeout: %" PRIu32 " ms\n", timeout);
  }
  else
  {
    SHELL_E("Send Notification Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to send a BLE server notification */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ServerSendNotification, ble_send_notif,
                       ble_send_notif < Conn Handle [0; 1] > < Service Index [0; 1] > < Char Index [0; 4] >
                       < Timeout > < Data >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ServerSendIndication(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint8_t service_index = 0;
  uint8_t char_index = 0;
  uint8_t data_buffer[W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH] = {0};
  uint32_t req_len = 0;
  uint32_t sent_data_len = 0;
  uint32_t timeout = 0;

  if (argc != 6)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle argument */
  conn_handle = (uint8_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[2]);
  if (service_index > (W6X_BLE_MAX_CREATED_SERVICE_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic index argument */
  char_index = (uint8_t)atoi(argv[3]);
  if (char_index > (W6X_BLE_MAX_CHAR_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the timeout */
  timeout = (uint32_t)atoi(argv[4]);

  /* Parse the data hex format */
  req_len = strlen(argv[5]) / 2; /* Each byte is represented by 2 hex characters */
  if ((req_len == 0) || (req_len > W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH))
  {
    SHELL_E("Data length must be between 1 and %" PRIu32 " bytes\n", W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH);
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  for (uint32_t i = 0; i < req_len; i++)
  {
    char byte_str[3] = {argv[5][i * 2], argv[5][i * 2 + 1], '\0'};
    int32_t hex_value = Parser_StrToHex(byte_str, NULL);
    if (hex_value < 0)
    {
      SHELL_E("Invalid hex data format: %s\n", byte_str);
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    data_buffer[i] = (uint8_t)hex_value;
  }

  /* Send Indication */
  if (W6X_Ble_ServerIndicate(conn_handle, service_index, char_index, data_buffer, req_len, &sent_data_len,
                             timeout) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Send Indication OK:\n");
    SHELL_PRINTF("  Service Index: %" PRIu32 "\n", service_index);
    SHELL_PRINTF("  Char Index: %" PRIu32 "\n", char_index);
    SHELL_PRINTF("  Data: %s\n", data_buffer);
    SHELL_PRINTF("  Requested Length: %" PRIu32 "\n", req_len);
    SHELL_PRINTF("  Sent Length: %" PRIu32 "\n", sent_data_len);
    SHELL_PRINTF("  Timeout: %" PRIu32 " ms\n", timeout);
  }
  else
  {
    SHELL_E("Send Indication Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to send a BLE server indication */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ServerSendIndication, ble_send_indication,
                       ble_send_indication < Conn Handle [0; 1] > < Service Index [0; 1] > < Char Index [0; 4] >
                       < Timeout > < Data >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ServerSetReadData(int32_t argc, char **argv)
{
  uint8_t service_index = 0;
  uint8_t char_index = 0;
  uint8_t data_buffer[W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH] = {0};
  uint32_t req_len = 0;
  uint32_t sent_data_len = 0;
  uint32_t timeout = 0;

  if (argc != 5)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[1]);
  if (service_index > (W6X_BLE_MAX_CREATED_SERVICE_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic index argument */
  char_index = (uint8_t)atoi(argv[2]);
  if (char_index > (W6X_BLE_MAX_CHAR_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the timeout */
  timeout = (uint32_t)atoi(argv[3]);

  /* Parse the data hex format */
  req_len = strlen(argv[4]) / 2; /* Each byte is represented by 2 hex characters */
  if ((req_len == 0) || (req_len > W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH))
  {
    SHELL_E("Data length must be between 1 and %" PRIu32 " bytes\n", W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH);
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  for (uint32_t i = 0; i < req_len; i++)
  {
    char byte_str[3] = {argv[4][i * 2], argv[4][i * 2 + 1], '\0'};
    int32_t hex_value = Parser_StrToHex(byte_str, NULL);
    if (hex_value < 0)
    {
      SHELL_E("Invalid hex data format: %s\n", byte_str);
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    data_buffer[i] = (uint8_t)hex_value;
  }

  /* Set Read Data */
  if (W6X_Ble_ServerSetReadData(service_index, char_index, data_buffer, req_len, &sent_data_len,
                                timeout) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Set Read Data OK:\n");
    SHELL_PRINTF("  Service Index: %" PRIu32 "\n", service_index);
    SHELL_PRINTF("  Char Index: %" PRIu32 "\n", char_index);
    SHELL_PRINTF("  Data: %s\n", data_buffer);
    SHELL_PRINTF("  Requested Length: %" PRIu32 "\n", req_len);
    SHELL_PRINTF("  Sent Length: %" PRIu32 "\n", sent_data_len);
    SHELL_PRINTF("  Timeout: %" PRIu32 " ms\n", timeout);
  }
  else
  {
    SHELL_E("Set Read Data Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to set the BLE server read data */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ServerSetReadData, ble_read_data,
                       ble_read_data < Service Index [0; 1] > < Char Index [0; 4] > < Timeout > < Data >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ClientWriteData(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint8_t service_index = 0;
  uint8_t char_index = 0;
  uint8_t data_buffer[W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH] = {0};
  uint32_t req_len = 0;
  uint32_t sent_data_len = 0;
  uint32_t timeout = 0;

  if (argc != 6)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[2]);
  if (service_index > W6X_BLE_MAX_SERVICE_NBR)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic index argument */
  char_index = (uint8_t)atoi(argv[3]);
  if (char_index > W6X_BLE_MAX_CHAR_NBR)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the timeout */
  timeout = (uint32_t)atoi(argv[4]);

  /* Parse the data hex format */
  req_len = strlen(argv[5]) / 2; /* Each byte is represented by 2 hex characters */
  if ((req_len == 0) || (req_len > W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH))
  {
    SHELL_E("Data length must be between 1 and %" PRIu32 " bytes\n", W6X_BLE_MAX_NOTIF_IND_DATA_LENGTH);
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  for (uint32_t i = 0; i < req_len; i++)
  {
    char byte_str[3] = {argv[5][i * 2], argv[5][i * 2 + 1], '\0'};
    int32_t hex_value = Parser_StrToHex(byte_str, NULL);
    if (hex_value < 0)
    {
      SHELL_E("Invalid hex data format: %s\n", byte_str);
      return SHELL_STATUS_UNKNOWN_ARGS;
    }
    data_buffer[i] = (uint8_t)hex_value;
  }
  /* Write Data */
  if (W6X_Ble_ClientWriteData(conn_handle, service_index, char_index, data_buffer, req_len, &sent_data_len,
                              timeout) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Client Write Data OK:\n");
    SHELL_PRINTF("  ConnHandle: %" PRIu32 "\n", conn_handle);
    SHELL_PRINTF("  Service Index: %" PRIu32 "\n", service_index);
    SHELL_PRINTF("  Char Index: %" PRIu32 "\n", char_index);
    SHELL_PRINTF("  Data: %s\n", data_buffer);
    SHELL_PRINTF("  Requested Length: %" PRIu32 "\n", req_len);
    SHELL_PRINTF("  Sent Length: %" PRIu32 "\n", sent_data_len);
    SHELL_PRINTF("  Timeout: %" PRIu32 " ms\n", timeout);
  }
  else
  {
    SHELL_E("Client Write Data Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to write data to a BLE characteristic */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ClientWriteData, ble_client_write_data,
                       ble_client_write_data < Conn Handle [0; 1] > < Service Index [1; 5] > < Char Index [1; 5] >
                       < Timeout > < Data >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ClientReadData(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint8_t service_index = 0;
  uint8_t char_index = 0;

  if (argc != 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the service index argument */
  service_index = (uint8_t)atoi(argv[2]);
  if (service_index > W6X_BLE_MAX_SERVICE_NBR)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic index argument */
  char_index = (uint8_t)atoi(argv[3]);
  if (char_index > W6X_BLE_MAX_CHAR_NBR)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Read Data */
  if (W6X_Ble_ClientReadData(conn_handle, service_index, char_index) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Client Read Data OK: Conn Handle %" PRIu32 ", Service Index %" PRIu32 ", Char Index %" PRIu32 "\n",
                 conn_handle, service_index, char_index);
  }
  else
  {
    SHELL_E("Client Read Data Failure\n", conn_handle, service_index, char_index);
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to read data from a BLE characteristic */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ClientReadData, ble_client_read_data,
                       ble_client_read_data < Conn Handle [0; 1] > < Service Index [1; 5] > < Char Index [1; 5] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ClientSubscribeChar(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint8_t char_value_handle = 0;
  uint8_t char_prop = 0;

  if (argc != 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic value handle */
  char_value_handle = (uint8_t)atoi(argv[2]);

  /* Parse the characteristic properties */
  char_prop = (uint8_t)atoi(argv[3]);
  if ((char_prop < 1) || (char_prop > 2))
  {
    SHELL_E("Invalid CharProp. Expected a value between 1 (notification) and 2 (indication)\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Subscribe to characteristic */
  if (W6X_Ble_ClientSubscribeChar(conn_handle, char_value_handle, char_prop) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Client Subscribe Charac OK:\n");
    SHELL_PRINTF("  Conn Handle: %" PRIu32 "\n", conn_handle);
    SHELL_PRINTF("  CharValue Handle: %" PRIu32 "\n", char_value_handle);
    SHELL_PRINTF("  Char Prop: %" PRIu32 "\n", char_prop);
  }
  else
  {
    SHELL_E("Client Subscribe Charac Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 0)
/** Shell command to subscribe to a BLE characteristic */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ClientSubscribeChar, ble_client_subscribe_char,
                       ble_client_subscribe_char < Conn Handle [0; 1] > < CharValue Handle > < Char Prop [1; 2] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_ClientUnsubscribeChar(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint8_t char_value_handle = 0;

  if (argc != 3)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the characteristic value handle */
  char_value_handle = (uint8_t)atoi(argv[2]);

  /* Unsubscribe to characteristic */
  if (W6X_Ble_ClientUnsubscribeChar(conn_handle, char_value_handle) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Client Unsubscribe Charac OK:\n");
    SHELL_PRINTF("  Conn Handle: %" PRIu32 "\n", conn_handle);
    SHELL_PRINTF("  CharValue Handle: %" PRIu32 "\n", char_value_handle);
  }
  else
  {
    SHELL_E("Client Unsubscribe Charac Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to subscribe to a BLE characteristic */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_ClientUnsubscribeChar, ble_client_unsubscribe_char,
                       ble_client_unsubscribe_char < Conn Handle [0; 1] > < CharValue Handle >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecurityParam(int32_t argc, char **argv)
{
  W6X_Ble_SecurityParameter_e security_parameter;

#if (SHELL_CMD_LEVEL >= 1)
  if (argc == 1)
  {
    /* Get Security parameter */
    if (W6X_Ble_GetSecurityParam(&security_parameter) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Get Security Parameter OK:\n");
      SHELL_PRINTF("  Security Parameter: %" PRIu32 "\n", security_parameter);
    }
    else
    {
      SHELL_E("Get Security Parameter Failure\n");
      return SHELL_STATUS_ERROR;
    }
    return SHELL_STATUS_OK;
  }
#endif /* SHELL_CMD_LEVEL */

  if (argc == 2)
  {
    /* Parse the security parameter */
    security_parameter = (W6X_Ble_SecurityParameter_e)atoi(argv[1]);
    if (security_parameter > W6X_BLE_SEC_IO_KEYBOARD_DISPLAY)
    {
      SHELL_E("Invalid SecurityParameter. Expected a value between 0 and %" PRIu32 "\n",
              W6X_BLE_SEC_IO_KEYBOARD_DISPLAY);
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    /* Set Security parameter */
    if (W6X_Ble_SetSecurityParam(security_parameter) == W6X_STATUS_OK)
    {
      SHELL_PRINTF("Set Security Parameter OK:\n");
      SHELL_PRINTF("  Security Parameter: %" PRIu32 "\n", security_parameter);
    }
    else
    {
      SHELL_E("Set Security Parameter Failure\n");
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
/** Shell command to set the BLE security parameter */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecurityParam, ble_sec_param,
                       ble_sec_param [ Security Parameter [0; 4] ]);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecurityStart(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint8_t security_level = 0;

  if (argc != 3)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the security level */
  security_level = (uint8_t)atoi(argv[2]);
  if (security_level > 4)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Start BLE Security */
  if (W6X_Ble_SecurityStart(conn_handle, security_level) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("BLE Start Security OK:\n");
    SHELL_PRINTF("  Conn Handle:  %" PRIu32 "\n", conn_handle);
    SHELL_PRINTF("  Security Level:  %" PRIu32 "\n", security_level);
  }
  else
  {
    SHELL_E("BLE Start Security Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to start BLE security */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecurityStart, ble_sec_start,
                       ble_sec_start < Conn Handle [0; 1] > < Security Level [1; 4] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecuritySetPassKey(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;
  uint32_t passkey = 0;

  if (argc != 3)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the passkey */
  passkey = (uint32_t)atoi(argv[2]);
  if (passkey > 999999)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Set security passkey */
  if (W6X_Ble_SecuritySetPassKey(conn_handle, passkey) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Security Set PassKey OK:\n");
    SHELL_PRINTF("  Conn Handle: %u\n", conn_handle);
    SHELL_PRINTF("  PassKey: %06u\n", passkey);
  }
  else
  {
    SHELL_E("Security Set PassKey Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to set the BLE security passkey */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecuritySetPassKey, ble_sec_set_passkey,
                       ble_sec_set_passkey < Conn Handle [0; 1] > < PassKey [0; 999999] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecurityPassKeyConfirm(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Security passkey confirm */
  if (W6X_Ble_SecurityPassKeyConfirm(conn_handle) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("BLE Security PassKey Confirm OK: Conn Handle %" PRIu32 "\n", conn_handle);
  }
  else
  {
    SHELL_E("BLE Security PassKey Confirm Failure\n", conn_handle);
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to confirm the BLE security passkey */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecurityPassKeyConfirm, ble_sec_passkey_confirm,
                       ble_sec_passkey_confirm < Conn Handle [0; 1] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecurityPairingConfirm(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Security pairing confirm */
  if (W6X_Ble_SecurityPairingConfirm(conn_handle) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("BLE Security Pairing Confirm OK: Conn Handle %" PRIu32 "\n", conn_handle);
  }
  else
  {
    SHELL_E("BLE Security Pairing Confirm Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to confirm the BLE security pairing */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecurityPairingConfirm, ble_sec_pairing_confirm,
                       ble_sec_pairing_confirm < Conn Handle [0; 1] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecurityPairingCancel(int32_t argc, char **argv)
{
  uint8_t conn_handle = 0;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the connection handle */
  conn_handle = (uint32_t)atoi(argv[1]);
  if (conn_handle > (W6X_BLE_MAX_CONN_NBR - 1))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Security pairing cancel */
  if (W6X_Ble_SecurityPairingCancel(conn_handle) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("BLE Security Pairing Cancel OK: Conn Handle %" PRIu32 "\n", conn_handle);
  }
  else
  {
    SHELL_E("BLE Security Pairing Cancel Failure\n", conn_handle);
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to cancel the BLE security pairing */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecurityPairingCancel, ble_sec_pairing_cancel,
                       ble_sec_pairing_cancel < Conn Handle [0; 1] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecurityUnpair(int32_t argc, char **argv)
{
  uint8_t remote_bdaddr[6] = {0};
  W6X_Ble_AddrType_e remote_addr_type;

  if (argc != 3)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Parse the remote Bluetooth Device Address */
  if (strlen(argv[1]) != 17)
  {
    SHELL_E("Invalid Remote BD Addr length. Expected 12 hexadecimal characters\n");
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  Parser_StrToMAC(argv[1], remote_bdaddr);

  /* Parse the remote address type */
  remote_addr_type = (W6X_Ble_AddrType_e)atoi(argv[2]);
  if (remote_addr_type > W6X_BLE_RPA_RANDOM_ADDR)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Unpair from remote BLE device */
  if (W6X_Ble_SecurityUnpair(remote_bdaddr, remote_addr_type) == W6X_STATUS_OK)
  {
    SHELL_PRINTF("Unpair Device OK:\n");
    SHELL_PRINTF("  Remote BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 remote_bdaddr[0], remote_bdaddr[1], remote_bdaddr[2],
                 remote_bdaddr[3], remote_bdaddr[4], remote_bdaddr[5]);
    SHELL_PRINTF("  Addr Type: %" PRIu32 "\n", remote_addr_type);
  }
  else
  {
    SHELL_E("Unpair Device Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to unpair a BLE device */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecurityUnpair, ble_sec_unpair,
                       ble_sec_unpair < Remote BD Addr > < Addr Type [0; 3] >);
#endif /* SHELL_CMD_LEVEL */

int32_t W6X_Shell_Ble_SecurityGetBondedDeviceList(int32_t argc, char **argv)
{
  W6X_Ble_Bonded_Devices_Result_t bonded_devices = {0};

  if (argc != 1)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Call the W6X_Ble_SecurityGetBondedDeviceList function */
  if (W6X_Ble_SecurityGetBondedDeviceList(&bonded_devices) == W6X_STATUS_OK)
  {
    /* Print the list of bonded devices */
    if (bonded_devices.Count != 0)
    {
      SHELL_PRINTF("Get Bonded Device List:\n");
      for (uint8_t i = 0; i < bonded_devices.Count; i++)
      {
        SHELL_PRINTF("  Device %" PRIu32 ":\n", i + 1);
        SHELL_PRINTF("    BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                     bonded_devices.Bonded_device[i].BDAddr[0], bonded_devices.Bonded_device[i].BDAddr[1],
                     bonded_devices.Bonded_device[i].BDAddr[2], bonded_devices.Bonded_device[i].BDAddr[3],
                     bonded_devices.Bonded_device[i].BDAddr[4], bonded_devices.Bonded_device[i].BDAddr[5]);
        SHELL_PRINTF("    BD Addr type: %" PRIu32 "\n", bonded_devices.Bonded_device[i].bd_addr_type);
        SHELL_PRINTF("    LTK: %s\n", bonded_devices.Bonded_device[i].LongTermKey);
      }
    }
    else
    {
      SHELL_PRINTF("No Bonded Device found\n");
    }
  }
  else
  {
    SHELL_E("Get Bonded Devices list Failure\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

#if (SHELL_CMD_LEVEL >= 1)
/** Shell command to get the list of bonded BLE devices */
SHELL_CMD_EXPORT_ALIAS(W6X_Shell_Ble_SecurityGetBondedDeviceList, ble_bonded_device_list,
                       ble_bonded_device_list);
#endif /* SHELL_CMD_LEVEL */

/** @} */
