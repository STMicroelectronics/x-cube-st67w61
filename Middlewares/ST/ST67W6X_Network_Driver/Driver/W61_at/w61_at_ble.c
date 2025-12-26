/**
  ******************************************************************************
  * @file    w61_at_ble.c
  * @author  GPM Application Team
  * @brief   This file provides code for W61 BLE AT module
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

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_BLE_Types
  * @{
  */

/**
  * @brief  BLE service information structure
  */
typedef struct
{
  int8_t service_index;                       /*!< Service index */
  W61_Ble_Service_t *ServiceInfo;             /*!< Service information */
} W61_Ble_Service_Info_t;

/**
  * @brief  BLE characteristic information structure
  */
typedef struct
{
  int8_t service_index;                       /*!< Service index */
  int8_t char_index;                          /*!< Characteristic index */
  W61_Ble_Characteristic_t *CharacInfo;       /*!< Characteristic information */
} W61_Ble_Charac_Info_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_BLE_Constants
  * @{
  */

/** BLE connection timeout in ms */
#define W61_BLE_CONNECT_TIMEOUT                   6000

/** BLE scan timeout in ms */
#define W61_BLE_SCAN_TIMEOUT                      5000

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_BLE_Functions
  * @{
  */

/**
  * @brief  Callback function to handle BLE get service responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_service);

/**
  * @brief  Callback function to handle BLE get characteristic responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_charac);

/**
  * @brief  Callback function to handle BLE get bonded devices responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_bonded);

/**
  * @brief  Parses BLE event and call related callback
  * @param  hObj: pointer to module handle
  * @param  argc: pointer to argument count
  * @param  argv: pointer to argument values
  */
static void W61_Ble_AT_Event(void *hObj, uint16_t *argc, char **argv);

/**
  * @brief  Parses BLE Data event and call related callback
  * @param  event_id: event ID
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @return 0 on success, negative value on error
  */
static int32_t W61_Ble_Data_Event(uint32_t event_id, struct modem_cmd_handler_data *data, uint16_t len);

/**
  * @brief  Convert UUID string to UUID array
  * @param  hexString: UUID string
  * @param  byteArray: UUID array
  * @param  byteArraySize: UUID array size
  */
static void hexStringToByteArray(const char *hexString, char *byteArray, size_t byteArraySize);

/**
  * @brief  Analyze advertising data
  * @param  ptr: advertising data
  * @param  Peripherals: peripheral structure
  * @param  index: index of the peripheral to fill with adv information
  */
static void W61_Ble_AnalyzeAdvData(char *ptr, W61_Ble_Scan_Result_t *Peripherals, uint32_t index);

/* Functions Definition ------------------------------------------------------*/
W61_Status_t W61_Ble_Init(W61_Object_t *Obj, uint8_t mode, uint8_t *p_recv_data, uint32_t req_len)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the data buffer to copy the received message in application layer */
  Obj->BleCtx.AppBuffRecvData = p_recv_data;
  Obj->BleCtx.AppBuffRecvDataSize = req_len;
  Obj->BleCtx.ScanResults.Detected_Peripheral = NULL;
  Obj->BleCtx.ScanResults.Count = 0; /* Reset the count of detected peripherals */
  Obj->BleCtx.ScanComplete = 0;      /* Initialize the scan complete indicator */

  /* Default De-init BLE mode */
  Obj->BleCtx.NetSettings.Mode = W61_BLE_MODE_DEINIT;

  Obj->Callbacks.Ble_event_cb = W61_Ble_AT_Event; /* Set the event callback function */
  Obj->Callbacks.Ble_event_data_cb = W61_Ble_Data_Event; /* Set the data event callback function */

  /* Set the BLE Server or Client mode
     0: BLE off
     1: Client mode
     2: Server mode
     3: Dual mode */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEINIT=%" PRIu16 "\r\n", mode);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_DeInit(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  /* Free the detected peripherals */
  if (Obj->BleCtx.ScanResults.Detected_Peripheral != NULL)
  {
    vPortFree(Obj->BleCtx.ScanResults.Detected_Peripheral);
    Obj->BleCtx.ScanResults.Detected_Peripheral = NULL;
    Obj->BleCtx.ScanResults.Count = 0;
  }

  /* Remove the data buffer pointer */
  Obj->BleCtx.AppBuffRecvData = NULL;
  Obj->BleCtx.AppBuffRecvDataSize = 0;

  /* Default De-init BLE mode */
  Obj->BleCtx.NetSettings.Mode = W61_BLE_MODE_DEINIT;

  Obj->Callbacks.Ble_event_cb = NULL; /* Reset the event callback function */
  Obj->Callbacks.Ble_event_data_cb = NULL; /* Reset the data event callback function */

  /* Deinit the BLE mode */
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+BLEINIT=0\r\n", W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SetRecvDataPtr(W61_Object_t *Obj, uint8_t *p_recv_data, uint32_t recv_data_buf_size)
{
  W61_NULL_ASSERT(Obj);

  Obj->BleCtx.AppBuffRecvData = p_recv_data;
  Obj->BleCtx.AppBuffRecvDataSize = recv_data_buf_size;

  return W61_STATUS_OK;
}

W61_Status_t W61_Ble_GetInitMode(W61_Object_t *Obj, W61_Ble_Mode_e *Mode)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  uint16_t argc = 0;
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mode);

  strncpy(cmd, "AT+BLEINIT?\r\n", sizeof(cmd));
  /* Query the current BLE mode */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLEINIT:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *Mode = (W61_Ble_Mode_e)atoi(argv[0]);
  Obj->BleCtx.NetSettings.Mode = *Mode;

  return ret;
}

W61_Status_t W61_Ble_SetTxPower(W61_Object_t *Obj, uint32_t power)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the BLE Tx power.
     Can bet set in the range of 0 to 20 dBm. */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLETXPWR=%" PRIu32 "\r\n", power);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetTxPower(W61_Object_t *Obj, uint32_t *power)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(power);

  strncpy(cmd, "AT+BLETXPWR?\r\n", sizeof(cmd));
  /* Query the BLE Tx power */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLETXPWR:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *power = (uint32_t)atoi(argv[0]);

  return ret;
}

W61_Status_t W61_Ble_AdvStart(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  /* Start BLE advertising */
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+BLEADVSTART\r\n", W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_AdvStop(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  /* Stop BLE advertising */
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+BLEADVSTOP\r\n", W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_Disconnect(W61_Object_t *Obj, uint32_t conn_handle)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* End the BLE connection */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEDISCONN=%" PRIu32 "\r\n", conn_handle);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_ExchangeMTU(W61_Object_t *Obj, uint32_t conn_handle)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the MTU to the maximum possible size */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEEXCHANGEMTU=%" PRIu32 "\r\n", conn_handle);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SetBDAddress(W61_Object_t *Obj, const uint8_t *bdaddr)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(bdaddr);

  /* Set the BLE address.
     The address is in the form of "XX:XX:XX:XX:XX:XX" */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+BLEADDR=\"%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 "\"\r\n",
           bdaddr[0], bdaddr[1], bdaddr[2], bdaddr[3], bdaddr[4], bdaddr[5]);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetBDAddress(W61_Object_t *Obj, uint8_t *BdAddr)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(BdAddr);

  strncpy(cmd, "AT+BLEADDR?\r\n", sizeof(cmd));
  /* Query the BLE address */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLEADDR:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  W61_AT_RemoveStrQuotes(argv[0]); /* Remove quotes from the string */
  Parser_StrToMAC(argv[0], BdAddr);

  return ret;
}

W61_Status_t W61_Ble_SetDeviceName(W61_Object_t *Obj, const char *name)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(name);

  /* Set the BLE device name */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLENAME=\"%s\"\r\n", name);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetDeviceName(W61_Object_t *Obj, char *DeviceName)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  uint16_t argc = 0;
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(DeviceName);

  strncpy(cmd, "AT+BLENAME?\r\n", sizeof(cmd));
  /* Query the BLE device name */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLENAME:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  if (strlen(argv[0]) > 26)
  {
    ret = W61_STATUS_ERROR;
  }
  else
  {
    strncpy(DeviceName, argv[0], W61_BLE_DEVICE_NAME_SIZE);
  }

  return ret;
}

W61_Status_t W61_Ble_SetAdvData(W61_Object_t *Obj, const char *advdata)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(advdata);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEADVDATA=\"%s\"\r\n", advdata);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SetScanRespData(W61_Object_t *Obj, const char *scanrespdata)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(scanrespdata);

  /* Set the BLE scan response data */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESCANRSPDATA=\"%s\"\r\n", scanrespdata);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SetAdvParam(W61_Object_t *Obj, uint32_t adv_int_min, uint32_t adv_int_max,
                                 uint8_t adv_type, uint8_t adv_channel)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the BLE advertising parameters. The parameters are:
     - <adv_int_min>: Minimum advertising interval. The range of this parameter is [0x0020,0x4000].
                      The actual advertising interval equals this parameter multiplied by 0.625 ms,
                      so the range for the actual minimum interval is [20, 10240] ms.
                      It should be less than or equal to the value of <adv_int_max>.
     - <adv_int_max>: Maximum advertising interval. The range of this parameter is [0x0020,0x4000].
                      The actual advertising interval equals this parameter multiplied by 0.625 ms,
                      so the range for the actual maximum interval is [20, 10240] ms.
                      It should be more than or equal to the value of <adv_int_min>.
     - <adv_type>:    Advertising type.
                      The range of this parameter is [0, 2]. The actual advertising type is:
                      0: Scannable and connectable
                      1: Non-connectable and scannable
                      2: Non-connectable and non-scannable
     - <channel_map>: Channel of advertising.
                      1: Channel 37
                      2: Channel 38
                      4: Channel 39
                      4: All channels */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEADVPARAM=%" PRIu32 ",%" PRIu32 ",%" PRIu16 ",%" PRIu16 "\r\n",
           adv_int_min, adv_int_max, adv_type, adv_channel);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_Scan(W61_Object_t *Obj, uint8_t enable)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  if (Obj->BleCtx.ScanResults.Detected_Peripheral == NULL)
  {
    /* Allocate memory for the detected peripherals */
    Obj->BleCtx.ScanResults.Detected_Peripheral = pvPortMalloc(sizeof(W61_Ble_Device_t) *
                                                               W61_BLE_MAX_DETECTED_PERIPHERAL);
    if (Obj->BleCtx.ScanResults.Detected_Peripheral == NULL)
    {
      return W61_STATUS_ERROR;
    }
  }
  /* Initialize the structure */
  memset(Obj->BleCtx.ScanResults.Detected_Peripheral, 0, sizeof(W61_Ble_Device_t) *
         W61_BLE_MAX_DETECTED_PERIPHERAL);

  /* Check if a previous scan has been executed */
  if (Obj->BleCtx.ScanResults.Count > 0)
  {
    /* If scan result exists, re-initialize counter */
    Obj->BleCtx.ScanResults.Count = 0; /* Reset the count of detected peripherals */
  }

  /* Start or abort BLE scanning */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESCAN=%" PRIu16 "\r\n", enable);
  ret = W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
  if (ret == W61_STATUS_OK && enable == 1)
  {
    /* If the scan is started:
       - Set the scan complete flag to 0
       - Set the scan start time to force the stop after the timeout */
    Obj->BleCtx.startScanTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
    Obj->BleCtx.ScanComplete = 0;
  }

  return ret;
}

W61_Status_t W61_Ble_SetScanParam(W61_Object_t *Obj, uint8_t scan_type, uint8_t own_addr_type,
                                  uint8_t filter_policy, uint32_t scan_interval, uint32_t scan_window)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the BLE scan parameters. The parameters are:
     - <scan_type>:     Scan type.
                        0: passive scan
                        1: active scan
     - <own_addr_type>: Own address type.
                        0: public address
                        1: random address
                        2: public address (resolvable private address)
                        3: random address (resolvable private address)
     - <filter_policy>: Filter policy.
                        0: any scan request or connect request
                        1: all connect request, white list scan request
                        2: all scan request, white list connect request
                        3: white list scan request and connect request
     - <scan_interval>: Scan interval. The range of this parameter is [0x0004,0x4000].
                        The actual scan interval equals this parameter multiplied by 0.625 ms,
                        so the range for the actual minimum interval is [2.5, 10240] ms.
     - <scan_window>:   Scan window. The range of this parameter is [0x0004,0x4000].
                        The actual scan window equals this parameter multiplied by 0.625 ms,
                        so the range for the actual minimum interval is [2.5, 10240] ms */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+BLESCANPARAM=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu32 ",%" PRIu32 "\r\n",
           scan_type, own_addr_type, filter_policy, scan_interval, scan_window);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetScanParam(W61_Object_t *Obj, uint32_t *ScanType, uint32_t *OwnAddrType,
                                  uint32_t *FilterPolicy, uint32_t *ScanInterval, uint32_t *ScanWindow)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ScanType);
  W61_NULL_ASSERT(OwnAddrType);
  W61_NULL_ASSERT(FilterPolicy);
  W61_NULL_ASSERT(ScanInterval);
  W61_NULL_ASSERT(ScanWindow);

  strncpy(cmd, "AT+BLESCANPARAM?\r\n", sizeof(cmd));
  /* Query the BLE scan parameters. The response is in the form of
     +BLESCANPARAM:<ScanType>,<OwnAddrType>,<FilterPolicy>,<ScanInterval>,<ScanWindow> */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLESCANPARAM:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 5)
  {
    return W61_STATUS_ERROR;
  }

  *ScanType = (uint32_t)atoi(argv[0]);
  *OwnAddrType = (uint32_t)atoi(argv[1]);
  *FilterPolicy = (uint32_t)atoi(argv[2]);
  *ScanInterval = (uint32_t)atoi(argv[3]);
  *ScanWindow = (uint32_t)atoi(argv[4]);

  return ret;
}

W61_Status_t W61_Ble_GetAdvParam(W61_Object_t *Obj, uint32_t *AdvIntMin,
                                 uint32_t *AdvIntMax, uint32_t *AdvType, uint32_t *ChannelMap)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(AdvIntMin);
  W61_NULL_ASSERT(AdvIntMax);
  W61_NULL_ASSERT(AdvType);
  W61_NULL_ASSERT(ChannelMap);

  strncpy(cmd, "AT+BLEADVPARAM?\r\n", sizeof(cmd));
  /* Query the BLE advertising parameters */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLEADVPARAM:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 4)
  {
    return W61_STATUS_ERROR;
  }

  /* The response is in the form of
     +BLEADVPARAM:<AdvIntMin>,<AdvIntMax>,<AdvType>,<ChannelMap> */
  *AdvIntMin = (uint32_t)atoi(argv[0]);
  *AdvIntMax = (uint32_t)atoi(argv[1]);
  *AdvType = (uint32_t)atoi(argv[2]);
  *ChannelMap = (uint32_t)atoi(argv[3]);

  return ret;
}

W61_Status_t W61_Ble_SetConnParam(W61_Object_t *Obj, uint32_t conn_handle, uint32_t conn_int_min,
                                  uint32_t conn_int_max, uint32_t latency, uint32_t timeout)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the BLE connection parameters. The parameters are:
     - <conn_handle>:  Connection handle
     - <conn_int_min>: Minimum connection interval. The range of this parameter is [0x0006,0x0C80].
                       The actual connection interval equals this parameter multiplied by 1.25 ms,
                       so the range for the actual minimum interval is [7.5, 4000] ms.
     - <conn_int_max>: Maximum connection interval. The range of this parameter is [0x0006,0x0C80].
                       The actual connection interval equals this parameter multiplied by 1.25 ms,
                       so the range for the actual maximum interval is [7.5, 4000] ms.
     - <latency>:      Slave latency. The range of this parameter is [0x0000,0x01F3].
     - <timeout>:      Supervision timeout. The range of this parameter is [0x000A,0x0C80].
                       The actual supervision timeout equals this parameter multiplied by 10 ms,
                       so the range for the actual supervision timeout is [100, 32000] ms */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+BLECONNPARAM=%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
           conn_handle, conn_int_min, conn_int_max, latency, timeout);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetConnParam(W61_Object_t *Obj, uint32_t *ConnHandle, uint32_t *ConnIntMin,
                                  uint32_t *ConnIntMax, uint32_t *ConnIntCurrent, uint32_t *Latency, uint32_t *Timeout)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnHandle);
  W61_NULL_ASSERT(ConnIntMin);
  W61_NULL_ASSERT(ConnIntMax);
  W61_NULL_ASSERT(ConnIntCurrent);
  W61_NULL_ASSERT(Latency);
  W61_NULL_ASSERT(Timeout);

  strncpy(cmd, "AT+BLECONNPARAM?\r\n", sizeof(cmd));
  /* Query the BLE connection parameters. The response is in the form of
     +BLECONNPARAM:<ConnHandle>,<ConnIntMin>,<ConnIntMax>,<ConnIntCurrent>,<Latency>,<Timeout> */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLECONNPARAM:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 6)
  {
    return W61_STATUS_ERROR;
  }

  *ConnHandle = (uint32_t)atoi(argv[0]);
  *ConnIntMin = (uint32_t)atoi(argv[1]);
  *ConnIntMax = (uint32_t)atoi(argv[2]);
  *ConnIntCurrent = (uint32_t)atoi(argv[3]);
  *Latency = (uint32_t)atoi(argv[4]);
  *Timeout = (uint32_t)atoi(argv[5]);

  return ret;
}

W61_Status_t W61_Ble_GetConn(W61_Object_t *Obj, uint32_t *ConnHandle, uint8_t *RemoteBDAddr)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnHandle);
  W61_NULL_ASSERT(RemoteBDAddr);

  strncpy(cmd, "AT+BLECONN?\r\n", sizeof(cmd));
  /* Query the BLE connection. The response is in the form of
     +BLECONN:<ConnHandle>,<RemoteAddress> */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLECONN:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 2)
  {
    *ConnHandle = 0xFF; /* No connection detected, return invalid connection handle */
  }
  else
  {
    *ConnHandle = (uint32_t)atoi(argv[0]);
    W61_AT_RemoveStrQuotes((char *)argv[1]); /* Remove quotes from the string */
    Parser_StrToMAC(argv[1], RemoteBDAddr);
  }
  return ret;
}

W61_Status_t W61_Ble_Connect(W61_Object_t *Obj, uint32_t conn_handle, uint8_t *RemoteBDAddr)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(RemoteBDAddr);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+BLECONN=%" PRIu32 ",\"%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16
           "\"\r\n",
           conn_handle, RemoteBDAddr[0], RemoteBDAddr[1], RemoteBDAddr[2],
           RemoteBDAddr[3], RemoteBDAddr[4], RemoteBDAddr[5]);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SetDataLength(W61_Object_t *Obj, uint32_t conn_handle, uint32_t tx_bytes, uint32_t tx_trans_time)
{
  W61_NULL_ASSERT(Obj);
  char cmd[W61_CMDRSP_STRING_SIZE];

  /* Set the BLE data length. The parameters are:
     - <conn_handle>:     Connection handle
     - <tx_bytes>:        Maximum number of bytes to be sent in a single packet
                          The range is [27, 251] bytes.
     - <tx_time>:         Maximum transmission time in microseconds
                          The range is [0, 2120] microseconds. */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEDATALEN=%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
           conn_handle, tx_bytes, tx_trans_time);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

/* GATT Server APIs */
W61_Status_t W61_Ble_CreateService(W61_Object_t *Obj, uint8_t service_index, const char *service_uuid,
                                   uint8_t uuid_type)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Create a GATT service. The parameters are:
     - <service_index>:  Service index
     - <service_uuid>:   Service UUID. The UUID is in the form of
                         "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" or "XXXX"
     - <uuid_type>:      UUID type.
                         0: 16-bit UUID
                         2: 128-bit UUID */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTSSRVCRE=%" PRIu16 ",\"%s\",1,%" PRIu16 "\r\n",
           service_index, service_uuid, uuid_type);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_DeleteService(W61_Object_t *Obj, uint8_t service_index)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Delete a GATT service. The parameters are:
     - <service_index>:  Service index */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTSSRVDEL=%" PRIu16 "\r\n", service_index);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetService(W61_Object_t *Obj, W61_Ble_Service_t *ServiceInfo, int8_t service_index)
{
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ServiceInfo);
  W61_Status_t ret;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;
  W61_Ble_Service_Info_t service_info =
  {
    .service_index = service_index,
    .ServiceInfo = ServiceInfo,
  };
  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+BLEGATTSSRV:", on_cmd_service, 4U, ","),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = &service_info;
  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+BLEGATTSSRV?\r\n",
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_Ble_CreateCharacteristic(W61_Object_t *Obj, uint8_t service_index, uint8_t char_index,
                                          const char *char_uuid, uint8_t uuid_type, uint8_t char_property,
                                          uint8_t char_permission)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Create a GATT characteristic. The parameters are:
     - <service_index>:   Service index
     - <char_index>:      Characteristic index
     - <char_uuid>:       Characteristic UUID. The UUID is in the form of
                          "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" or "XXXX"
     - <uuid_type>:       UUID type.
                          0: 16-bit UUID
                          2: 128-bit UUID
     - <char_property>:   Characteristic property.
                          0x02: read
                          0x04: write without response
                          0x08: write with response
                          0x10: notify
                          0x20: indicate
     - <char_permission>: Characteristic permission.
                          1: read permission
                          2: write permission */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+BLEGATTSCHARCRE=%" PRIu16 ",%" PRIu16 ",\"%s\",%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
           service_index, char_index, char_uuid, char_property, char_permission, uuid_type);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetCharacteristic(W61_Object_t *Obj, W61_Ble_Characteristic_t *CharacInfo, int8_t service_index,
                                       int8_t char_index)
{
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(CharacInfo);
  W61_Status_t ret;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;
  W61_Ble_Charac_Info_t charac_info =
  {
    .service_index = service_index,
    .char_index = char_index,
    .CharacInfo = CharacInfo,
  };

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+BLEGATTSCHAR:", on_cmd_charac, 6U, ","),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = &charac_info;
  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+BLEGATTSCHAR?\r\n",
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

W61_Status_t W61_Ble_RegisterCharacteristics(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  /* Register the GATT services and characteristics */
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+BLEGATTSREGISTER=1\r\n", W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_ServerSendNotification(W61_Object_t *Obj, uint8_t conn_handle, uint8_t service_index,
                                            uint8_t char_index, uint8_t *pdata, uint32_t req_len, uint32_t *SentLen,
                                            uint32_t Timeout)
{
  W61_Status_t ret;
  int32_t ret_len;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (req_len > SPI_XFER_MTU_BYTES)
  {
    req_len = SPI_XFER_MTU_BYTES;
  }

  *SentLen = req_len;

  /* Timeout should let the time to NCP to return SEND:ERROR message */
  if (Timeout < W61_BLE_TIMEOUT)
  {
    Timeout = W61_BLE_TIMEOUT;
  }
  /* The command AT+BLEGATTSNTFY is used to send a notification to the client.
     The parameters are:
     - <service_index>:  Service index
     - <char_index>:     Characteristic index
     - <req_len>:        Length of the data to be sent.
                         Maximum data length is 244.
     - <conn_index>:     BLE Connection index */
  ret_len = snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTSNTFY=%" PRIu16 ",%" PRIu16 ",%" PRIu32,
                     service_index, char_index, req_len);

  if (W61_SdkMinVersion(Obj, 2, 0, 97) == W61_STATUS_OK)
  {
    snprintf(&cmd[ret_len], W61_CMDRSP_STRING_SIZE - ret_len, ",%" PRIu16 "\r\n", conn_handle);
  }
  else
  {
    snprintf(&cmd[ret_len], W61_CMDRSP_STRING_SIZE - ret_len, "\r\n");
  }
  ret = W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, pdata, req_len, Timeout, true);

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

W61_Status_t W61_Ble_ServerSendIndication(W61_Object_t *Obj, uint8_t conn_handle, uint8_t service_index,
                                          uint8_t char_index, uint8_t *pdata, uint32_t req_len, uint32_t *SentLen,
                                          uint32_t Timeout)
{
  W61_Status_t ret;
  int32_t ret_len;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (req_len > SPI_XFER_MTU_BYTES)
  {
    req_len = SPI_XFER_MTU_BYTES;
  }

  *SentLen = req_len;

  /* Timeout should let the time to NCP to return SEND:ERROR message */
  if (Timeout < W61_BLE_TIMEOUT)
  {
    Timeout = W61_BLE_TIMEOUT;
  }
  /* The command AT+BLEGATTSIND is used to send an indication to the client.
      The parameters are:
      - <service_index>:  Service index
      - <char_index>:     Characteristic index
      - <req_len>:        Length of the data to be sent.
                          Maximum data length is 244.
      - <conn_index>:     BLE Connection index */
  ret_len = snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTSIND=%" PRIu16 ",%" PRIu16 ",%" PRIu32 "\r\n",
                     service_index, char_index, req_len);

  if (W61_SdkMinVersion(Obj, 2, 0, 97) == W61_STATUS_OK)
  {
    snprintf(&cmd[ret_len], W61_CMDRSP_STRING_SIZE - ret_len, ",%" PRIu16 "\r\n", conn_handle);
  }
  else
  {
    snprintf(&cmd[ret_len], W61_CMDRSP_STRING_SIZE - ret_len, "\r\n");
  }
  ret = W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, pdata, req_len, Timeout, false);

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

W61_Status_t W61_Ble_ServerSetReadData(W61_Object_t *Obj, uint8_t service_index, uint8_t char_index, uint8_t *pdata,
                                       uint32_t req_len, uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (req_len > SPI_XFER_MTU_BYTES)
  {
    req_len = SPI_XFER_MTU_BYTES;
  }
  if (req_len > Obj->BleCtx.AppBuffRecvDataSize)
  {
    req_len = Obj->BleCtx.AppBuffRecvDataSize;
  }
  *SentLen = req_len;

  /* Timeout should let the time to NCP to return SEND:ERROR message */
  if (Timeout < W61_BLE_TIMEOUT)
  {
    Timeout = W61_BLE_TIMEOUT;
  }
  /* The command AT+BLEGATTSRD is used to send a read response to the client.
      The parameters are:
      - <service_index>:  Service index
      - <char_index>:     Characteristic index
      - <req_len>:        Length of the data to be sent.
                          Maximum data length is 244. */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTSRD=%" PRIu16 ",%" PRIu16 ",%" PRIu32 "\r\n",
           service_index, char_index, req_len);
  ret = W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, pdata, req_len, Timeout, false);

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

/* GATT Client APIs */
W61_Status_t W61_Ble_RemoteServiceDiscovery(W61_Object_t *Obj, uint8_t conn_handle)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Discover the GATT services. The parameters are:
     - <conn_handle>:        Connection handle
     The available services will be returned as W61_BLE_EVT_SERVICE_FOUND_ID events */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTCSRVDIS=%" PRIu16 "\r\n", conn_handle);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_RemoteCharDiscovery(W61_Object_t *Obj, uint8_t conn_handle, uint8_t service_index)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Discover the GATT characteristics. The parameters are:
     - <conn_handle>:        Connection handle
     - <service_index>:      Service index
     The available characteristics will be returned as W61_BLE_EVT_CHAR_FOUND_ID events */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTCCHARDIS=%" PRIu16 ",%" PRIu16 "\r\n", conn_handle, service_index);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_ClientWriteData(W61_Object_t *Obj, uint8_t conn_handle, uint8_t service_index, uint8_t char_index,
                                     uint8_t *pdata, uint32_t req_len, uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (req_len > SPI_XFER_MTU_BYTES)
  {
    req_len = SPI_XFER_MTU_BYTES;
  }

  *SentLen = req_len;

  /* Timeout should let the time to NCP to return SEND:ERROR message */
  if (Timeout < W61_BLE_TIMEOUT)
  {
    Timeout = W61_BLE_TIMEOUT;
  }
  /* Send a write request to the server characteristic.
      The parameters are:
      - <conn_handle>:     Connection handle
      - <service_index>:   Service index
      - <char_index>:      Characteristic index
      - <req_len>:         Length of the data to be sent.
                          Maximum data length is 244.
      The response will be returned as W61_BLE_EVT_WRITE_ID event */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTCWR=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu32 "\r\n",
           conn_handle, service_index, char_index, req_len);
  ret = W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, pdata, req_len, Timeout, false);

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

W61_Status_t W61_Ble_ClientReadData(W61_Object_t *Obj, uint8_t conn_handle, uint8_t service_index, uint8_t char_index)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Send a read request to the server characteristic.
     The parameters are:
     - <conn_handle>:     Connection handle
     - <service_index>:   Service index
     - <char_index>:      Characteristic index
     The response will be returned as W61_BLE_EVT_READ_ID event */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTCRD=%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
           conn_handle, service_index, char_index);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_ClientSubscribeChar(W61_Object_t *Obj, uint8_t conn_handle, uint8_t char_value_handle,
                                         uint8_t char_prop)
{
  uint8_t char_desc_handle = char_value_handle + 1;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Subscribe to a characteristic. The parameters are:
     - <conn_handle>:        Connection handle
     - <char_desc_handle>:   Characteristic descriptor handle
     - <char_value_handle>:  Characteristic value handle
     - <char_prop>:          Characteristic property */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTCSUBSCRIBE=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
           conn_handle, char_desc_handle, char_value_handle, char_prop);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_ClientUnsubscribeChar(W61_Object_t *Obj, uint8_t conn_handle, uint8_t char_value_handle)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Unsubscribe from a characteristic. The parameters are:
     - <conn_handle>:        Connection handle
     - <char_value_handle>:  Characteristic value handle */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLEGATTCUNSUBSCRIBE=%" PRIu16 ",%" PRIu16 "\r\n",
           conn_handle, char_value_handle);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

/* Security APIs */
W61_Status_t W61_Ble_SetSecurityParam(W61_Object_t *Obj, uint8_t security_parameter)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the security management related I/O capabilities. The parameters are:
     - <security_parameter>:  Security parameter
                              0: Display only
                              1: Display with Yes/No-buttons
                              2: Keyboard only
                              3: No input and no output
                              4: Display with keyboard */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESECPARAM=%" PRIu16 "\r\n", security_parameter);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_GetSecurityParam(W61_Object_t *Obj, uint32_t *SecurityParameter)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(SecurityParameter);

  strncpy(cmd, "AT+BLESECPARAM?\r\n", sizeof(cmd));
  /* Query the security management related I/O capabilities. The response is in the form of
     +BLESECPARAM:<SecurityParameter> */
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+BLESECPARAM:", &argc, argv, W61_BLE_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 1)
  {
    return W61_STATUS_ERROR;
  }

  *SecurityParameter = (uint32_t)atoi(argv[0]);

  return ret;
}

W61_Status_t W61_Ble_SecurityStart(W61_Object_t *Obj, uint8_t conn_handle, uint8_t security_level)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Start the security management. The parameters are:
     - <conn_handle>:     Connection handle
     - <security_level>:  Security level
                          0: Only for BR/EDR special cases
                          1: No encryption and no authentication
                          2: Encryption and no authentication (no MITM)
                          3: Encryption and authentication (MITM)
                          4: Authenticated Secure Connections and 128-bit key */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESECSTART=%" PRIu16 ",%" PRIu16 "\r\n", conn_handle, security_level);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SecurityPassKeyConfirm(W61_Object_t *Obj, uint8_t conn_handle)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Request the passkey confirm. The parameters are:
     - <conn_handle>:     Connection handle
     The response will be returned as W61_BLE_EVT_PASSKEY_CONFIRM_ID event */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESECPASSKEYCONFIRM=%" PRIu16 "\r\n", conn_handle);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SecurityPairingConfirm(W61_Object_t *Obj, uint8_t conn_handle)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Request the pairing confirm. The parameters are:
     - <conn_handle>:     Connection handle
     The response will be returned as W61_BLE_EVT_PAIRING_CONFIRM_ID event */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESECPAIRINGCONFIRM=%" PRIu16 "\r\n", conn_handle);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SecuritySetPassKey(W61_Object_t *Obj, uint8_t conn_handle, uint32_t passkey)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Set the passkey. The parameters are:
     - <conn_handle>:     Connection handle
     - <passkey>:         Passkey
     The response will be returned as W61_BLE_EVT_PASSKEY_CONFIRM_ID event */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESECPASSKEY=%" PRIu16 ",%06" PRIu32 "\r\n", conn_handle, passkey);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SecurityPairingCancel(W61_Object_t *Obj, uint8_t conn_handle)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Cancel the pairing. The parameters are:
     - <conn_handle>:     Connection handle */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+BLESECCANNEL=%" PRIu16 "\r\n", conn_handle);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SecurityUnpair(W61_Object_t *Obj, uint8_t *RemoteBDAddr, uint32_t remote_addr_type)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  /* Unpair a remote device. The parameters are:
     - <RemoteBDAddr>:     Remote device BD address
     - <remote_addr_type>: Remote device address type
                           0: Public address
                           1: Random address */
  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+BLESECUNPAIR=\"%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16
           "\",%" PRIu32 "\r\n",
           RemoteBDAddr[0], RemoteBDAddr[1], RemoteBDAddr[2],
           RemoteBDAddr[3], RemoteBDAddr[4], RemoteBDAddr[5],
           remote_addr_type);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_BLE_TIMEOUT);
}

W61_Status_t W61_Ble_SecurityGetBondedDeviceList(W61_Object_t *Obj,
                                                 W61_Ble_Bonded_Devices_Result_t *RemoteBondedDevices)
{
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(RemoteBondedDevices);
  W61_Status_t ret;
  struct modem *mdm = (struct modem *) &Obj->Modem;
  struct modem_cmd_handler_data *data = (struct modem_cmd_handler_data *)mdm->modem_cmd_handler.cmd_handler_data;

  struct modem_cmd handlers[] =
  {
    MODEM_CMD_ARGS_MAX("+BLESECGETLTKLIST: BONDADDR ", on_cmd_bonded, 1U, 10U, "()"),
  };

  if (data == NULL)
  {
    return W61_STATUS_ERROR;
  }
  (void)xSemaphoreTake(data->sem_tx_lock, portMAX_DELAY);

  mdm->rx_data = RemoteBondedDevices;
  RemoteBondedDevices->Count = 0; /* Initialize the count of bonded devices */

  ret = W61_Status(modem_cmd_send_ext(&mdm->iface,
                                      &mdm->modem_cmd_handler,
                                      handlers,
                                      ARRAY_SIZE(handlers),
                                      (const uint8_t *)"AT+BLESECGETLTKLIST?\r\n",
                                      mdm->sem_response,
                                      W61_NCP_TIMEOUT,
                                      MODEM_NO_TX_LOCK));

  (void)xSemaphoreGive(data->sem_tx_lock);
  return ret;
}

/* Private Functions Definition ----------------------------------------------*/
MODEM_CMD_DEFINE(on_cmd_service)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Ble_Service_Info_t *service_info = (W61_Ble_Service_Info_t *)mdm->rx_data;

  if (argc >= 4)
  {
    uint32_t service_idx = (uint32_t)atoi((char *)argv[0]);
    if ((service_idx < W61_BLE_MAX_SERVICE_NBR) && (service_idx == service_info->service_index))
    {
      /* If the service index matches, fill the ServiceInfo structure */
      service_info->ServiceInfo->service_idx = service_idx;
      W61_AT_RemoveStrQuotes((char *)argv[1]); /* Remove quotes from the string */
      hexStringToByteArray((char *)argv[1], service_info->ServiceInfo->service_uuid, W61_BLE_MAX_UUID_SIZE);
      service_info->ServiceInfo->service_type = (uint8_t)atoi((char *)argv[2]);
      service_info->ServiceInfo->uuid_type = (uint8_t)atoi((char *)argv[3]);
    }
  }

  return 0;
}

MODEM_CMD_DEFINE(on_cmd_charac)
{
  struct modem *mdm = (struct modem *) data->user_data;
  W61_Ble_Charac_Info_t *charac_info = (W61_Ble_Charac_Info_t *)mdm->rx_data;

  if (argc >= 6)
  {
    uint32_t service_idx = (uint32_t)atoi((char *)argv[0]);
    uint32_t charac_idx = (uint32_t)atoi((char *)argv[1]);
    if ((service_idx < W61_BLE_MAX_SERVICE_NBR) && (service_idx == charac_info->service_index) &&
        (charac_idx < W61_BLE_MAX_CHAR_NBR) && (charac_idx == charac_info->char_index))
    {
      /* If the service and characteristic index match, fill the CharacInfo structure */
      charac_info->CharacInfo->char_idx = (uint8_t)charac_idx;
      W61_AT_RemoveStrQuotes((char *)argv[2]); /* Remove quotes from the string */
      hexStringToByteArray((char *)argv[2], charac_info->CharacInfo->char_uuid, W61_BLE_MAX_UUID_SIZE);
      charac_info->CharacInfo->char_property = (uint32_t)atoi((char *)argv[3]);
      charac_info->CharacInfo->char_permission = (uint32_t)atoi((char *)argv[4]);
      charac_info->CharacInfo->uuid_type = (uint32_t)atoi((char *)argv[5]);
    }
  }

  return 0;
}

MODEM_CMD_DEFINE(on_cmd_bonded)
{
  struct modem *mdm = (struct modem *) data->user_data;
  char tmp_hex_table[38] = {0};
  W61_Ble_Bonded_Devices_Result_t *bonded_devices = (W61_Ble_Bonded_Devices_Result_t *)mdm->rx_data;

  if ((argc >= 3) && (bonded_devices->Count < W61_BLE_MAX_BONDED_DEVICES))
  {
    uint8_t count = bonded_devices->Count++;
    /* If the count is within the maximum bonded devices limit, fill the Bonded_device structure */
    argv[0][W61_BLE_BD_ADDR_STRING_SIZE] = 0; /* Skip last space */
    Parser_StrToMAC((char *)argv[0], bonded_devices->Bonded_device[count].BDAddr);

    /* Get address type */
    if (strcmp((char *)argv[1], "public") == 0)
    {
      bonded_devices->Bonded_device[count].bd_addr_type = W61_BLE_PUBLIC_ADDR;
    }
    else if (strcmp((char *)argv[1], "random") == 0)
    {
      bonded_devices->Bonded_device[count].bd_addr_type = W61_BLE_RANDOM_ADDR;
    }
    else if (strcmp((char *)argv[1], "public-id") == 0)
    {
      bonded_devices->Bonded_device[count].bd_addr_type = W61_BLE_RPA_PUBLIC_ADDR;
    }
    else if (strcmp((char *)argv[1], "random-id") == 0)
    {
      bonded_devices->Bonded_device[count].bd_addr_type = W61_BLE_RPA_RANDOM_ADDR;
    }
    else
    {
      bonded_devices->Bonded_device[count].bd_addr_type = 0xff;
    }

    /* Get LTK */
    memcpy(tmp_hex_table, (void *)argv[2], 38);
    for (int32_t i = 0; i < 32; i++)
    {
      /* Remove " LTK:" */
      bonded_devices->Bonded_device[count].LongTermKey[i] = (uint8_t)tmp_hex_table[i + 5];
    }
  }

  return 0;
}

static void W61_Ble_AT_Event(void *hObj, uint16_t *argc, char **argv)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  W61_Ble_CbParamData_t cb_param_ble_data = {0};
  uint8_t conn_handle = 0;
  uint8_t conn_role = 0xff;
  char tmp_hex_table[33] = {0};

  uint32_t j = 0;

  if ((Obj == NULL) || (Obj->ulcbs.UL_ble_cb == NULL) || (*argc < 1))
  {
    return;
  }

  if (strcmp(argv[0], "CONNPARAM") == 0)
  {
    conn_handle = (uint8_t)atoi(argv[1]);
    cb_param_ble_data.remote_ble_device.conn_handle = conn_handle;
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_CONNECTION_PARAM_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "CONNECTED") == 0) && (*argc >= 3))
  {
    /* Parse the connection handle */
    conn_handle = (uint8_t)atoi(argv[1]);

    /* Parse the BD address */
    W61_AT_RemoveStrQuotes(argv[2]);
    Parser_StrToMAC(argv[2], Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].BDAddr);

    /* Check the BD address validity */
    if (Parser_CheckValidAddress(Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].BDAddr, 6) != 0)
    {
      return;
    }

    if (*argc == 4)
    {
      /* Parse the connection mode */
      conn_role = (uint8_t)atoi(argv[3]);
    }

    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].IsConnected = 1;
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].conn_handle = conn_handle;
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].conn_role = conn_role;
    Obj->BleCtx.NetSettings.DeviceConnectedNb++;

    cb_param_ble_data.remote_ble_device = Obj->BleCtx.NetSettings.RemoteDevice[conn_handle];

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_CONNECTED_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "DISCONNECTED") == 0) && (*argc >= 3))
  {
    conn_handle = (uint8_t)atoi(argv[1]);

    if (*argc == 4)
    {
      conn_role = (uint8_t)atoi(argv[3]);
    }
    else
    {
      conn_role = 0xff; /* Default value */
    }

    /* Keep connection handle and connection role for upper layer event management */
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].conn_handle = conn_handle;
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].conn_role = conn_role;
    /* Re-initialize all other attributes */
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].IsConnected = 0;
    memset(Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].BDAddr, 0x0, W61_BLE_BD_ADDR_SIZE);
    memset(Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].DeviceName, 0x0, W61_BLE_DEVICE_NAME_SIZE);
    memset(Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].ManufacturerData, 0x0, W61_BLE_MANUF_DATA_SIZE);
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].bd_addr_type = 0;
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].RSSI = 0;
    Obj->BleCtx.NetSettings.DeviceConnectedNb--;
    cb_param_ble_data.remote_ble_device = Obj->BleCtx.NetSettings.RemoteDevice[conn_handle];
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_DISCONNECTED_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "INDICATION") == 0) && (*argc >= 3))
  {
    uint32_t event_type;
    uint32_t arg_1;

    /* SDK v2.0.84 and lower */
    event_type = (uint32_t)atoi(argv[1]);
    arg_1 = (uint32_t)atoi(argv[2]);

    if ((*argc == 3) && (event_type == 2))
    {
      if (arg_1 == 0)
      {
        /* Indication Complete event */
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_INDICATION_ACK_ID, NULL);
      }
      else
      {
        /* Indication Not Complete event */
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_INDICATION_NACK_ID, NULL);
      }
    }
    else if (*argc == 4)
    {
      cb_param_ble_data.service_idx = arg_1;
      cb_param_ble_data.charac_idx = (uint8_t)atoi(argv[3]);
      cb_param_ble_data.indication_status[arg_1] = event_type;

      if (event_type == 0)
      {
        /* Indication Disable event */
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_INDICATION_STATUS_DISABLED_ID, &cb_param_ble_data);
      }
      else if (event_type == 1)
      {
        /* Indication Enable event */
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_INDICATION_STATUS_ENABLED_ID, &cb_param_ble_data);
      }
    }
    return;
  }

  if ((strcmp(argv[0], "NOTIFICATION") == 0) && (*argc >= 4))
  {
    cb_param_ble_data.service_idx = (uint32_t)atoi(argv[2]);
    cb_param_ble_data.notification_status[cb_param_ble_data.service_idx] = (uint8_t)atoi(argv[1]);
    cb_param_ble_data.charac_idx = (uint8_t)atoi(argv[3]);

    if (cb_param_ble_data.notification_status[cb_param_ble_data.service_idx])
    {
      /* Notification Enabled event */
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_NOTIFICATION_STATUS_ENABLED_ID, &cb_param_ble_data);
    }
    else
    {
      /* Notification Disabled event */
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_NOTIFICATION_STATUS_DISABLED_ID, &cb_param_ble_data);
    }
    return;
  }

  if (strcmp(argv[0], "SCAN") == 0)
  {
    if (Obj->BleCtx.ScanResults.Detected_Peripheral == NULL)
    {
      return;
    }
    TickType_t currentTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();

    if ((Obj->BleCtx.ScanResults.Count < W61_BLE_MAX_DETECTED_PERIPHERAL) && (Obj->BleCtx.ScanComplete == 0) &&
        ((currentTime - Obj->BleCtx.startScanTime) <= ((TickType_t) pdMS_TO_TICKS(W61_BLE_SCAN_TIMEOUT))))
    {
      /* Parse the BD address */
      uint8_t mac_ui8[W61_BLE_BD_ADDR_SIZE] = {0};
      W61_AT_RemoveStrQuotes(argv[1]);
      Parser_StrToMAC(argv[1], mac_ui8);

      /* Check the BD address validity */
      if (Parser_CheckValidAddress(mac_ui8, 6) != 0)
      {
        return;
      }

      /* RSSI */
      int32_t rssi = (int32_t)atoi(argv[2]);

      /* Adv data */
      char adv_data[100] = {0};
      if (strlen(argv[3]) > 0)
      {
        strncpy(adv_data, argv[3], sizeof(adv_data) - 1);
      }

      /* Scan response data */
      char scan_rsp_data[100] = {0};
      if (strlen(argv[4]) > 0)
      {
        strncpy(scan_rsp_data, argv[4], sizeof(scan_rsp_data) - 1);
      }

      /* BD address type */
      int32_t bd_addr_type = atoi(argv[5]);

      uint32_t index = 0;
      for (; (index < Obj->BleCtx.ScanResults.Count) && (index < W61_BLE_MAX_DETECTED_PERIPHERAL); index++)
      {
        if (memcmp(mac_ui8, Obj->BleCtx.ScanResults.Detected_Peripheral[index].BDAddr, W61_BLE_BD_ADDR_SIZE) == 0)
        {
          break;
        }
      }
      if (index < W61_BLE_MAX_DETECTED_PERIPHERAL)
      {
        /* Update data and RSSI */
        Obj->BleCtx.ScanResults.Detected_Peripheral[index].RSSI = rssi;
        W61_Ble_AnalyzeAdvData(adv_data, &Obj->BleCtx.ScanResults, index);
        W61_Ble_AnalyzeAdvData(scan_rsp_data, &Obj->BleCtx.ScanResults, index);

        if (index == Obj->BleCtx.ScanResults.Count)
        {
          /* New peripheral detected, fill BD address and address type */
          memcpy(Obj->BleCtx.ScanResults.Detected_Peripheral[index].BDAddr, mac_ui8, W61_BLE_BD_ADDR_SIZE);
          Obj->BleCtx.ScanResults.Detected_Peripheral[index].bd_addr_type = bd_addr_type;
          Obj->BleCtx.ScanResults.Count++;
        }
      }
    }
    else
    {
      if ((Obj->ulcbs.UL_ble_cb != NULL) && (Obj->BleCtx.ScanComplete == 0))
      {
        Obj->BleCtx.ScanComplete = 1;
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_SCAN_DONE_ID, NULL);
      }
    }
    return;
  }

  if (strcmp(argv[0], "SRVCHAR") == 0)
  {
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)atoi(argv[1]);

    /* Get service */
    cb_param_ble_data.Service.service_idx = (uint8_t)atoi(argv[2]);
    cb_param_ble_data.Service.charac.char_idx = (uint8_t)atoi(argv[3]);

    for (int32_t i = 0; (i < 32); i++)
    {
      if (argv[4][j] == 0x00) /* End of UUID */
      {
        break;
      }
      else
      {
        if (argv[4][j] == '-')
        {
          j++;
        }
        tmp_hex_table[i] = argv[4][j];
        j++;
      }
    }
    hexStringToByteArray(tmp_hex_table, cb_param_ble_data.Service.charac.char_uuid,
                         W61_BLE_MAX_UUID_SIZE);
    cb_param_ble_data.Service.charac.uuid_type = W61_BLE_UUID_TYPE_16;
    /* Check if 128 bit UUID */
    for (int32_t i = 2; i < W61_BLE_MAX_UUID_SIZE; i++)
    {
      if (cb_param_ble_data.Service.charac.char_uuid[i] != 0x00)
      {
        cb_param_ble_data.Service.charac.uuid_type = W61_BLE_UUID_TYPE_128;
        break;
      }
    }
    cb_param_ble_data.Service.charac.char_property = (uint8_t)atoi(argv[5]);
    cb_param_ble_data.Service.charac.char_handle = (uint16_t)atoi(argv[6]);
    cb_param_ble_data.Service.charac.char_value_handle = (uint16_t)atoi(argv[7]);

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_CHAR_FOUND_ID, &cb_param_ble_data);
    return;
  }

  if (strcmp(argv[0], "SRV") == 0)
  {
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)atoi(argv[1]);

    /* Get service */
    cb_param_ble_data.Service.service_idx = (uint8_t)atoi(argv[2]);
    for (int32_t i = 0; (i < 32); i++)
    {
      if (argv[3][j] == 0x00) /* End of UUID */
      {
        break;
      }
      else
      {
        if (argv[3][j] == '-')
        {
          j++;
        }
        tmp_hex_table[i] = argv[3][j];
        j++;
      }
    }
    hexStringToByteArray(tmp_hex_table, cb_param_ble_data.Service.service_uuid,
                         W61_BLE_MAX_UUID_SIZE);
    cb_param_ble_data.Service.uuid_type = W61_BLE_UUID_TYPE_16;
    /* Check if 128 bit UUID */
    for (int32_t i = 2; i < W61_BLE_MAX_UUID_SIZE; i++)
    {
      if (cb_param_ble_data.Service.service_uuid[i] != 0x00)
      {
        cb_param_ble_data.Service.uuid_type = W61_BLE_UUID_TYPE_128;
        break;
      }
    }
    cb_param_ble_data.Service.service_type = (uint8_t)atoi(argv[4]);

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_SERVICE_FOUND_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "MTUSIZE") == 0) && (*argc >= 3))
  {
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)atoi(argv[1]);
    cb_param_ble_data.mtu_size = (uint16_t)atoi(argv[2]);

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_MTU_SIZE_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "PASSKEYCONFIRM") == 0) && (*argc >= 10))
  {
    /* Parse the BD address */
    for (int32_t i = 0; i < W61_BLE_BD_ADDR_SIZE; i++)
    {
      cb_param_ble_data.remote_ble_device.BDAddr[i] = (uint8_t)strtoul(argv[i + 1], NULL, 16);
    }

    /* Check the BD address validity */
    if (Parser_CheckValidAddress(cb_param_ble_data.remote_ble_device.BDAddr, 6) != 0)
    {
      return;
    }

    /* Identify connection handle */
    cb_param_ble_data.remote_ble_device.conn_handle = 0xff;
    for (int32_t i = 0; i < W61_BLE_MAX_CONN_NBR; i++)
    {
      if (memcmp(cb_param_ble_data.remote_ble_device.BDAddr,
                 Obj->BleCtx.NetSettings.RemoteDevice[i].BDAddr,
                 W61_BLE_BD_ADDR_SIZE) == 0)
      {
        cb_param_ble_data.remote_ble_device.conn_handle = i;
        break;
      }
    }

    /* Get address type */
    if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RANDOM_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_RANDOM_ADDR;
    }
    else
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = 0xff;
    }

    cb_param_ble_data.PassKey = (uint32_t)atoi(argv[9]);
    if (cb_param_ble_data.PassKey != 0)
    {
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PASSKEY_CONFIRM_ID, &cb_param_ble_data);
    }
    return;
  }

  if ((strcmp(argv[0], "PASSKEYENTRY") == 0) && (*argc >= 8))
  {
    /* Parse the BD address */
    for (int32_t i = 0; i < W61_BLE_BD_ADDR_SIZE; i++)
    {
      cb_param_ble_data.remote_ble_device.BDAddr[i] = (uint8_t)strtoul(argv[i + 1], NULL, 16);
    }

    /* Check the BD address validity */
    if (Parser_CheckValidAddress(cb_param_ble_data.remote_ble_device.BDAddr, 6) != 0)
    {
      return;
    }

    /* Identify connection handle */
    cb_param_ble_data.remote_ble_device.conn_handle = 0xff;
    for (int32_t i = 0; i < W61_BLE_MAX_CONN_NBR; i++)
    {
      if (memcmp(cb_param_ble_data.remote_ble_device.BDAddr,
                 Obj->BleCtx.NetSettings.RemoteDevice[i].BDAddr,
                 W61_BLE_BD_ADDR_SIZE) == 0)
      {
        cb_param_ble_data.remote_ble_device.conn_handle = i;
        break;
      }
    }

    /* Get address type */
    if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RANDOM_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_RANDOM_ADDR;
    }
    else
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = 0xff;
    }

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PASSKEY_ENTRY_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "PASSKEYDISPLAY") == 0) && (*argc >= 2))
  {
    cb_param_ble_data.PassKey = (uint32_t)atoi(argv[1]);
    if (cb_param_ble_data.PassKey != 0)
    {
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PASSKEY_DISPLAY_ID, &cb_param_ble_data);
    }
    return;
  }

  if ((strcmp(argv[0], "PAIRINGCOMPLETED") == 0) && (*argc >= 10))
  {
    /* Parse the BD address */
    for (int32_t i = 0; i < W61_BLE_BD_ADDR_SIZE; i++)
    {
      cb_param_ble_data.remote_ble_device.BDAddr[i] = (uint8_t)strtoul(argv[i + 2], NULL, 16);
    }

    /* Check the BD address validity */
    if (Parser_CheckValidAddress(cb_param_ble_data.remote_ble_device.BDAddr, 6) != 0)
    {
      return;
    }

    /* Identify connection handle */
    cb_param_ble_data.remote_ble_device.conn_handle = 0xff;
    for (int32_t i = 0; i < W61_BLE_MAX_CONN_NBR; i++)
    {
      if (memcmp(cb_param_ble_data.remote_ble_device.BDAddr,
                 Obj->BleCtx.NetSettings.RemoteDevice[i].BDAddr,
                 W61_BLE_BD_ADDR_SIZE) == 0)
      {
        cb_param_ble_data.remote_ble_device.conn_handle = i;
        break;
      }
    }

    /* Get address type */
    if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 2], "public") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 2], "random") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RANDOM_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 2], "public-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 2], "random-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_RANDOM_ADDR;
    }
    else
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = 0xff;
    }

    /* Get LTK */
    memcpy(tmp_hex_table, (void *)argv[W61_BLE_BD_ADDR_SIZE + 4], 33);
    for (int32_t i = 0; i < 32; i++)
    {
      /* Remove space before LTK */
      cb_param_ble_data.LongTermKey[i] = (uint8_t)tmp_hex_table[i + 1];
    }

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PAIRING_COMPLETED_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "PAIRINGFAILED") == 0)  && (*argc >= 8))
  {
    /* Parse the BD address */
    for (int32_t i = 0; i < W61_BLE_BD_ADDR_SIZE; i++)
    {
      cb_param_ble_data.remote_ble_device.BDAddr[i] = (uint8_t)strtoul(argv[i + 1], NULL, 16);
    }

    /* Check the BD address validity */
    if (Parser_CheckValidAddress(cb_param_ble_data.remote_ble_device.BDAddr, 6) != 0)
    {
      return;
    }

    /* Identify connection handle */
    cb_param_ble_data.remote_ble_device.conn_handle = 0xff;
    for (int32_t i = 0; i < W61_BLE_MAX_CONN_NBR; i++)
    {
      if (memcmp(cb_param_ble_data.remote_ble_device.BDAddr,
                 Obj->BleCtx.NetSettings.RemoteDevice[i].BDAddr,
                 W61_BLE_BD_ADDR_SIZE) == 0)
      {
        cb_param_ble_data.remote_ble_device.conn_handle = i;
        break;
      }
    }

    /* Get address type */
    if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RANDOM_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_RANDOM_ADDR;
    }
    else
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = 0xff;
    }

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PAIRING_FAILED_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "PAIRINGCONFIRM") == 0) && (*argc >= 8))
  {
    /* Parse the BD address */
    for (int32_t i = 0; i < W61_BLE_BD_ADDR_SIZE; i++)
    {
      cb_param_ble_data.remote_ble_device.BDAddr[i] = (uint8_t)strtoul(argv[i + 1], NULL, 16);
    }

    /* Check the BD address validity */
    if (Parser_CheckValidAddress(cb_param_ble_data.remote_ble_device.BDAddr, 6) != 0)
    {
      return;
    }

    /* Identify connection handle */
    cb_param_ble_data.remote_ble_device.conn_handle = 0xff;
    for (int32_t i = 0; i < W61_BLE_MAX_CONN_NBR; i++)
    {
      if (memcmp(cb_param_ble_data.remote_ble_device.BDAddr,
                 Obj->BleCtx.NetSettings.RemoteDevice[i].BDAddr,
                 W61_BLE_BD_ADDR_SIZE) == 0)
      {
        cb_param_ble_data.remote_ble_device.conn_handle = i;
        break;
      }
    }

    /* Get address type */
    if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RANDOM_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_RANDOM_ADDR;
    }
    else
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = 0xff;
    }

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PAIRING_CONFIRM_ID, &cb_param_ble_data);
    return;
  }

  if ((strcmp(argv[0], "PAIRCANNELED") == 0) && (*argc >= 8))
  {
    /* Parse the BD address */
    for (int32_t i = 0; i < W61_BLE_BD_ADDR_SIZE; i++)
    {
      cb_param_ble_data.remote_ble_device.BDAddr[i] = (uint8_t)strtoul(argv[i + 1], NULL, 16);
    }

    /* Check the BD address validity */
    if (Parser_CheckValidAddress(cb_param_ble_data.remote_ble_device.BDAddr, 6) != 0)
    {
      return;
    }

    /* Identify connection handle */
    cb_param_ble_data.remote_ble_device.conn_handle = 0xff;
    for (int32_t i = 0; i < W61_BLE_MAX_CONN_NBR; i++)
    {
      if (memcmp(cb_param_ble_data.remote_ble_device.BDAddr,
                 Obj->BleCtx.NetSettings.RemoteDevice[i].BDAddr,
                 W61_BLE_BD_ADDR_SIZE) == 0)
      {
        cb_param_ble_data.remote_ble_device.conn_handle = i;
        break;
      }
    }

    /* Get address type */
    if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RANDOM_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "public-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_PUBLIC_ADDR;
    }
    else if (strcmp(argv[W61_BLE_BD_ADDR_SIZE + 1], "random-id") == 0)
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = W61_BLE_RPA_RANDOM_ADDR;
    }
    else
    {
      cb_param_ble_data.remote_ble_device.bd_addr_type = 0xff;
    }

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PAIRING_CANCELED_ID, &cb_param_ble_data);
    return;
  }
}

static int32_t W61_Ble_Data_Event(uint32_t event_id, struct modem_cmd_handler_data *data, uint16_t len)
{
  struct modem *mdm = (struct modem *)data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);
  W61_Ble_CbParamData_t cb_param_ble_data = {0};
  uint8_t *ptr;
  uint8_t *endptr;
  uint32_t data_len;
  uint16_t rx_data_len;

  if ((data->rx_buf == NULL) || (data->rx_buf_len == 0) || (len == 0) ||
      (len >= data->rx_buf_len) || (Obj->BleCtx.AppBuffRecvData == NULL))
  {
    return -EINVAL;
  }
  ptr = &data->rx_buf[len];
  data->rx_buf[data->rx_buf_len] = 0;

  /* Connection handle */
  cb_param_ble_data.remote_ble_device.conn_handle = strtol((char *)ptr, (char **)&endptr, 10);
  if ((endptr == ptr) || (*endptr != ','))
  {
    return -EINVAL;
  }
  endptr++; /* Skip the comma */

  if ((event_id == W61_BLE_EVT_READ_ID) || (event_id == W61_BLE_EVT_WRITE_ID))
  {
    /* Service index */
    cb_param_ble_data.service_idx = strtol((char *)endptr, (char **)&endptr, 10);
    if ((endptr == ptr) || (*endptr != ','))
    {
      return -EINVAL;
    }
    endptr++; /* Skip the comma */

    /* Characteristic index */
    cb_param_ble_data.charac_idx = strtol((char *)endptr, (char **)&endptr, 10);
    if ((endptr == ptr) || (*endptr != ','))
    {
      return -EINVAL;
    }
    endptr++; /* Skip the comma */
  }
  else if (event_id == W61_BLE_EVT_NOTIFICATION_DATA_ID)
  {
    /* Characteristic value handle */
    cb_param_ble_data.charac_value_handle = strtol((char *)endptr, (char **)&endptr, 10);
    if ((endptr == ptr) || (*endptr != ','))
    {
      return -EINVAL;
    }
    endptr++; /* Skip the comma */
  }

  /* Data length */
  data_len = strtol((char *)endptr, (char **)&endptr, 10);
  if (endptr == ptr)
  {
    return -EINVAL;
  }

  /* If the endptr is not pointing to a comma and data is empty, return immediately */
  if ((*endptr != ',') || (data_len == 0))
  {
    return (endptr - data->rx_buf);
  }
  endptr++; /* Skip the comma */

  /* Check if the buffer is large enough to hold the data */
  if (data_len > Obj->BleCtx.AppBuffRecvDataSize)
  {
    return -ENOMEM;
  }

  rx_data_len = endptr - data->rx_buf + data_len;
  if (data->rx_buf_len >= rx_data_len)
  {
    /* Copy the data */
    memcpy(Obj->BleCtx.AppBuffRecvData, endptr, data_len);
    cb_param_ble_data.available_data_length = data_len;
    if (Obj->ulcbs.UL_ble_cb != NULL)
    {
      Obj->ulcbs.UL_ble_cb(event_id, &cb_param_ble_data);
    }
    return rx_data_len;
  }
  else
  {
    return -EAGAIN;
  }
}

static void hexStringToByteArray(const char *hexString, char *byteArray, size_t byteArraySize)
{
  size_t hexStringLength = strlen(hexString);
  for (size_t i = 0; i < hexStringLength / 2 && i < byteArraySize; i++)
  {
    byteArray[i] = (Parser_Hex2Num(hexString[2 * i]) << 4) + Parser_Hex2Num(hexString[2 * i + 1]);
  }
}

static void W61_Ble_AnalyzeAdvData(char *ptr, W61_Ble_Scan_Result_t *Peripherals, uint32_t index)
{
  uint32_t adv_data_size;
  uint32_t adv_data_flag;
  uint32_t end_of_data_packet;
  char *p_adv_data;
  uint16_t i = 0;
  if (ptr != NULL)
  {
    p_adv_data = (char *)ptr;
    end_of_data_packet = p_adv_data[2];

    while (end_of_data_packet != 0)
    {
      adv_data_size = (Parser_Hex2Num(p_adv_data[2 * i]) << 4) + Parser_Hex2Num(p_adv_data[2 * i + 1]);
      adv_data_flag = (Parser_Hex2Num(p_adv_data[2 * i + 2]) << 4) + Parser_Hex2Num(p_adv_data[2 * i + 3]);

      switch (adv_data_flag)
      {
        case W61_BLE_AD_TYPE_FLAGS:
          break;
        case W61_BLE_AD_TYPE_MANUFACTURER_SPECIFIC_DATA:
        {
          if (Peripherals->Detected_Peripheral != NULL)
          {
            hexStringToByteArray(p_adv_data + 2 * i + 4,
                                 (char *) Peripherals->Detected_Peripheral[index].ManufacturerData,
                                 adv_data_size - 1);
          }
          break;
        }
        case W61_BLE_AD_TYPE_COMPLETE_LOCAL_NAME:
        {
          if (Peripherals->Detected_Peripheral != NULL)
          {
            hexStringToByteArray(p_adv_data + 2 * i + 4,
                                 (char *) Peripherals->Detected_Peripheral[index].DeviceName,
                                 adv_data_size - 1);
          }
          break;
        }
        default:
          break;
      } /* End of switch */

      i = i + adv_data_size + 1;

      /* Check end of decoded data packet */
      end_of_data_packet = *(p_adv_data + 2 * i);
    }
  }
}

/** @} */
