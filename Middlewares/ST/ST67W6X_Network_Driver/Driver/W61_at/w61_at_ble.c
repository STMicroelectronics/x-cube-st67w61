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
#include "w61_at_rx_parser.h"
#include "w61_io.h" /* SPI_XFER_MTU_BYTES */
#include "common_parser.h" /* Common Parser functions */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_BLE_Constants
  * @{
  */

#define W61_BLE_EVT_CONNECTED_KEYWORD             "+BLE:CONNECTED"        /*!< Connected event keyword */
#define W61_BLE_EVT_DISCONNECTED_KEYWORD          "+BLE:DISCONNECTED"     /*!< Disconnected event keyword */
#define W61_BLE_EVT_CONNECTION_PARAM_KEYWORD      "+BLE:CONNPARAM"        /*!< Connection parameter event keyword */
#define W61_BLE_EVT_READ_KEYWORD                  "+BLE:GATTREAD"         /*!< GATT Read event keyword */
#define W61_BLE_EVT_WRITE_KEYWORD                 "+BLE:GATTWRITE"        /*!< GATT Write event keyword */
#define W61_BLE_EVT_SERVICE_FOUND_KEYWORD         "+BLE:SRV"              /*!< Service found event keyword */
#define W61_BLE_EVT_CHAR_FOUND_KEYWORD            "+BLE:SRVCHAR"          /*!< Characteristic found event keyword */
#define W61_BLE_EVT_INDICATION_STATUS_KEYWORD     "+BLE:INDICATION"       /*!< Indication status event keyword */
#define W61_BLE_EVT_NOTIFICATION_STATUS_KEYWORD   "+BLE:NOTIFICATION"     /*!< Notification status event keyword */
#define W61_BLE_EVT_NOTIFICATION_DATA_KEYWORD     "+BLE:NOTIDATA"         /*!< Notification data event keyword */
#define W61_BLE_EVT_MTU_SIZE_KEYWORD              "+BLE:MTUSIZE"          /*!< MTU size event keyword */
#define W61_BLE_EVT_PAIRING_FAILED_KEYWORD        "+BLE:PAIRINGFAILED"    /*!< Pairing failed event keyword */
#define W61_BLE_EVT_PAIRING_COMPLETED_KEYWORD     "+BLE:PAIRINGCOMPLETED" /*!< Pairing completed event keyword */
#define W61_BLE_EVT_PAIRING_CONFIRM_KEYWORD       "+BLE:PAIRINGCONFIRM"   /*!< Pairing confirm event keyword */
#define W61_BLE_EVT_PASSKEY_ENTRY_KEYWORD         "+BLE:PASSKEYENTRY"     /*!< Passkey entry event keyword */
#define W61_BLE_EVT_PASSKEY_DISPLAY_KEYWORD       "+BLE:PASSKEYDISPLAY"   /*!< Passkey display event keyword */
#define W61_BLE_EVT_PASSKEY_CONFIRM_KEYWORD       "+BLE:PASSKEYCONFIRM"   /*!< Passkey confirm event keyword */
#define W61_BLE_EVT_SCAN_KEYWORD                  "+BLE:SCAN"             /*!< BLE scan event keyword */

#define W61_BLE_CONNECT_TIMEOUT                   6000                    /*!< BLE connection timeout in ms */
#define W61_BLE_SCAN_TIMEOUT                      5000                    /*!< BLE scan timeout in ms */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** Variable to manage BLE Scan */
uint8_t ScanComplete = 0;

/** Timer for BLE Scan */
TickType_t startScanTime;

/** sscanf parsing error string */
static const char W61_Parsing_Error_str[] = "Parsing of the result failed";

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_BLE_Functions
  * @{
  */

/**
  * @brief  Parses BLE event and call related callback.
  * @param  hObj: pointer to module handle
  * @param  p_evt: pointer to event buffer
  * @param  evt_len: event length
  */
static void W61_Ble_AT_Event(void *hObj, const uint8_t *p_evt, int32_t evt_len);

/**
  * @brief  Convert UUID string to UUID array
  * @param  hexString: UUID string
  * @param  byteArray: UUID array
  * @param  byteArraySize: UUID array size
  */
static void hexStringToByteArray(const char *hexString, char *byteArray, size_t byteArraySize);

/**
  * @brief  Parse peripheral data
  * @param  pdata: peripheral data
  * @param  len: data length
  * @param  Peripherals: peripheral structure
  */
static void W61_Ble_AT_ParsePeripheral(char *pdata, int32_t len, W61_Ble_Scan_Result_t *Peripherals);

/**
  * @brief  Parse service data
  * @param  pdata: service data
  * @param  len: data length
  * @param  Service: service structure
  */
static void W61_Ble_AT_ParseService(char *pdata, int32_t len, W61_Ble_Service_t *Service);

/**
  * @brief  Parse service characteristic data
  * @param  pdata: service characteristic data
  * @param  len: data length
  * @param  Service: service structure
  */
static void W61_Ble_AT_ParseServiceCharac(char *pdata, int32_t len, W61_Ble_Service_t *Service);

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
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  /* Set the data buffer to copy the received message in application layer */
  Obj->BleCtx.AppBuffRecvData = p_recv_data;
  Obj->BleCtx.AppBuffRecvDataSize = req_len;

  /* Allocate memory for the detected peripherals */
  Obj->BleCtx.ScanResults.Detected_Peripheral = pvPortMalloc(sizeof(W61_Ble_Scanned_Device_t) *
                                                             W61_BLE_MAX_DETECTED_PERIPHERAL);
  if (Obj->BleCtx.ScanResults.Detected_Peripheral == NULL)
  {
    return W61_STATUS_ERROR;
  }
  memset(Obj->BleCtx.ScanResults.Detected_Peripheral, 0, sizeof(W61_Ble_Scanned_Device_t)*
         W61_BLE_MAX_DETECTED_PERIPHERAL);
  Obj->BleCtx.ScanResults.Count = 0; /* Reset the count of detected peripherals */

  Obj->Ble_event_cb = W61_Ble_AT_Event; /* Set the event callback function */

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the BLE Server or Client mode
       0: BLE off
       1: Client mode
       2: Server mode */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEINIT=%" PRIu16 "\r\n", mode);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_DeInit(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
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

  Obj->Ble_event_cb = NULL; /* Reset the event callback function */

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Deinit the BLE mode */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEINIT=0\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_SetRecvDataPtr(W61_Object_t *Obj, uint8_t *p_recv_data, uint32_t recv_data_buf_size)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  /* Careful: The application shall change the pointer only on callback, DATA_ID or READ_ID events,
     i.e. only when the W61_ATD_RxPooling_task is running and it has completed previous data copy */
  Obj->BleCtx.AppBuffRecvData = p_recv_data;
  Obj->BleCtx.AppBuffRecvDataSize = recv_data_buf_size;

  return W61_STATUS_OK;
}

W61_Status_t W61_Ble_GetInitMode(W61_Object_t *Obj, W61_Ble_Mode_e *Mode)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t tmp = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Mode);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the current BLE mode */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEINIT?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((const char *)Obj->CmdResp, "+BLEINIT:%" SCNu32 "\r\n", &tmp) != 1)
      {
        LogError("%s\n", W61_Parsing_Error_str);
        ret = W61_STATUS_ERROR;
      }
      else
      {
        *Mode = (W61_Ble_Mode_e)tmp;
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

W61_Status_t W61_Ble_SetTxPower(W61_Object_t *Obj, uint32_t power)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the BLE Tx power.
       Can bet set in the range of 0 to 20 dBm. */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLETXPWR=%" PRIu32 "\r\n", power);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetTxPower(W61_Object_t *Obj, uint32_t *power)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(power);
  int32_t tmp = 0;

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE Tx power */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLETXPWR?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (strncmp((char *)Obj->CmdResp, "+BLETXPWR:", sizeof("+BLETXPWR:") - 1) == 0)
      {
        Parser_StrToInt((char *)Obj->CmdResp + strlen("+BLETXPWR:"), NULL, &tmp);
        *power = (uint32_t)tmp;
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

W61_Status_t W61_Ble_AdvStart(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Start BLE advertising */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEADVSTART\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_AdvStop(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Stop BLE advertising */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEADVSTOP\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetConnection(W61_Object_t *Obj, uint32_t *ConnectionHandle, uint8_t address[6])
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t recv_len;
  int32_t cmp;
  char remote_address[19];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnectionHandle);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE connection handle */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLECONN?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
      if (recv_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }
      /* Get the response. The response is in the form of
         +BLECONN:<ConnectionHandle>,<RemoteAddress>
         Multiples connection can be established in client mode, so the response can be received multiple times. */
      while ((recv_len > 0) && (strncmp((char *)Obj->CmdResp, "+BLECONN:", strlen("+BLECONN:")) == 0))
      {
        Obj->CmdResp[recv_len] = 0;
        cmp = sscanf((char *)Obj->CmdResp, "+BLECONN:%" SCNu32 ",\"%18[^\"]\"",
                     ConnectionHandle, remote_address);

        /* Parse the remote address and connection handle */
        if (cmp == 2)
        {
          Parser_StrToMAC(remote_address, address);
        }
        else if (cmp == 0)
        {
          *ConnectionHandle = 0;
        }

        /* Get the next response if available */
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
      }

      if (recv_len > 0)
      {
        /* Check if the last response is OK or ERROR */
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
}

W61_Status_t W61_Ble_Disconnect(W61_Object_t *Obj, uint32_t connection_handle)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* End the BLE connection */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEDISCONN=%" PRIu32 "\r\n",
             connection_handle);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_ExchangeMTU(W61_Object_t *Obj, uint32_t connection_handle)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the MTU to the maximum possible size */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEEXCHANGEMTU=%" PRIu32 "\r\n",
             connection_handle);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_SetBDAddress(W61_Object_t *Obj, const uint8_t *bdaddr)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(bdaddr);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the BLE address.
       The address is in the form of "XX:XX:XX:XX:XX:XX" */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEADDR=\"%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 "\"\r\n",
             bdaddr[0], bdaddr[1], bdaddr[2], bdaddr[3], bdaddr[4], bdaddr[5]);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetBDAddress(W61_Object_t *Obj, uint8_t *BdAddr)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char *ptr;
  char *token;
  int32_t recv_len;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(BdAddr);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE address */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEADDR?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
      if (recv_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
      }
      else
      {
        /* Get the response. The response is in the form of
           +BLEADDR:<BLE_public_addr> */
        ptr = strstr((char *)(Obj->CmdResp), "+BLEADDR:");
        if (ptr != NULL)
        {
          ptr += sizeof("+BLEADDR:");
          token = strstr(ptr, "\r");
          if (token == NULL)
          {
            goto _err;
          }
          *(--token) = 0;
          Parser_StrToMAC(ptr, BdAddr); /* Convert the string to MAC address */

          /* Check if the last response is OK or ERROR */
          recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, Obj->NcpTimeout);
          if (recv_len > 0)
          {
            ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
          }
          else
          {
            ret = W61_STATUS_TIMEOUT;
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

_err:
  W61_ATunlock(Obj);
  return W61_STATUS_ERROR;
}

W61_Status_t W61_Ble_SetDeviceName(W61_Object_t *Obj, const char *name)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(name);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the BLE device name */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLENAME=\"%s\"\r\n", name);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetDeviceName(W61_Object_t *Obj, char *DeviceName)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(DeviceName);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE device name */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLENAME?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+BLENAME:%26[^\r\n]", DeviceName) != 1)
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

W61_Status_t W61_Ble_SetAdvData(W61_Object_t *Obj, const char *advdata)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(advdata);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the BLE advertising data */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEADVDATA=\"%s\"\r\n", advdata);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_SetScanRespData(W61_Object_t *Obj, const char *scanrespdata)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(scanrespdata);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the BLE scan response data */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESCANRSPDATA=\"%s\"\r\n", scanrespdata);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_SetAdvParam(W61_Object_t *Obj, uint32_t adv_int_min, uint32_t adv_int_max,
                                 uint8_t adv_type, uint8_t adv_channel)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
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
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEADVPARAM=%" PRIu32 ",%" PRIu32 ",%" PRIu16 ",%" PRIu16 "\r\n",
             adv_int_min, adv_int_max, adv_type, adv_channel);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_Scan(W61_Object_t *Obj, uint8_t enable)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  /* Check if a previous scan has been executed */
  if (Obj->BleCtx.ScanResults.Count > 0)
  {
    /* If scan result exists, re-initialize the structure and counter */
    vPortFree(Obj->BleCtx.ScanResults.Detected_Peripheral);
    Obj->BleCtx.ScanResults.Detected_Peripheral = pvPortMalloc(sizeof(W61_Ble_Scanned_Device_t) *
                                                               W61_BLE_MAX_DETECTED_PERIPHERAL);
    if (Obj->BleCtx.ScanResults.Detected_Peripheral == NULL)
    {
      return W61_STATUS_ERROR;
    }
    memset(Obj->BleCtx.ScanResults.Detected_Peripheral, 0, sizeof(W61_Ble_Scanned_Device_t)*
           W61_BLE_MAX_DETECTED_PERIPHERAL);
    Obj->BleCtx.ScanResults.Count = 0; /* Reset the count of detected peripherals */
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Start or abort BLE scanning */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESCAN=%" PRIu16 "\r\n", enable);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK && enable == 1)
    {
      /* If the scan is started:
         - Set the scan complete flag to 0
         - Set the scan start time to force the stop after the timeout */
      startScanTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
      ScanComplete = 0;
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_Ble_SetScanParam(W61_Object_t *Obj, uint8_t scan_type, uint8_t own_addr_type,
                                  uint8_t filter_policy, uint32_t scan_interval, uint32_t scan_window)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
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
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLESCANPARAM=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu32 ",%" PRIu32 "\r\n",
             scan_type, own_addr_type, filter_policy, scan_interval, scan_window);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetScanParam(W61_Object_t *Obj, uint32_t *ScanType, uint32_t *OwnAddrType,
                                  uint32_t *FilterPolicy, uint32_t *ScanInterval, uint32_t *ScanWindow)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ScanType);
  W61_NULL_ASSERT(OwnAddrType);
  W61_NULL_ASSERT(FilterPolicy);
  W61_NULL_ASSERT(ScanInterval);
  W61_NULL_ASSERT(ScanWindow);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE scan parameters. The response is in the form of
       +BLESCANPARAM:<ScanType>,<OwnAddrType>,<FilterPolicy>,<ScanInterval>,<ScanWindow> */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESCANPARAM?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp,
                 "+BLESCANPARAM:%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                 ScanType, OwnAddrType, FilterPolicy, ScanInterval, ScanWindow) != 5)
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

W61_Status_t W61_Ble_GetAdvParam(W61_Object_t *Obj, uint32_t *AdvIntMin,
                                 uint32_t *AdvIntMax, uint32_t *AdvType, uint32_t *ChannelMap)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(AdvIntMin);
  W61_NULL_ASSERT(AdvIntMax);
  W61_NULL_ASSERT(AdvType);
  W61_NULL_ASSERT(ChannelMap);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE advertising parameters */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEADVPARAM?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp,
                 "+BLEADVPARAM:%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                 AdvIntMin, AdvIntMax, AdvType, ChannelMap) != 4)
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

W61_Status_t W61_Ble_SetConnParam(W61_Object_t *Obj, uint32_t conn_handle, uint32_t conn_int_min,
                                  uint32_t conn_int_max, uint32_t latency, uint32_t timeout)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
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
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLECONNPARAM=%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
             conn_handle, conn_int_min, conn_int_max, latency, timeout);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetConnParam(W61_Object_t *Obj, uint32_t *ConnHandle, uint32_t *ConnIntMin,
                                  uint32_t *ConnIntMax, uint32_t *ConnIntCurrent, uint32_t *Latency, uint32_t *Timeout)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnHandle);
  W61_NULL_ASSERT(ConnIntMin);
  W61_NULL_ASSERT(ConnIntMax);
  W61_NULL_ASSERT(ConnIntCurrent);
  W61_NULL_ASSERT(Latency);
  W61_NULL_ASSERT(Timeout);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE connection parameters. The response is in the form of
       +BLECONNPARAM:<ConnHandle>,<ConnIntMin>,<ConnIntMax>,<ConnIntCurrent>,<Latency>,<Timeout> */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLECONNPARAM?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp,
                 "+BLECONNPARAM:%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                 ConnHandle, ConnIntMin, ConnIntMax, ConnIntCurrent, Latency, Timeout) != 6)
      {
        LogError("%s\n", W61_Parsing_Error_str);
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

W61_Status_t W61_Ble_GetConn(W61_Object_t *Obj, uint32_t *ConnHandle, uint8_t *RemoteBDAddr)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char tmp_bdaddr[18];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ConnHandle);
  W61_NULL_ASSERT(RemoteBDAddr);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the BLE connection. The response is in the form of
       +BLECONN:<ConnHandle>,<RemoteAddress> */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLECONN?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+BLECONN:%" SCNu32 ",\"%17[^\"]\"\r\n", ConnHandle, tmp_bdaddr) != 2)
      {
        LogError("%s\n", W61_Parsing_Error_str);
        ret = W61_STATUS_ERROR;
      }
      else
      {
        Parser_StrToMAC(tmp_bdaddr, RemoteBDAddr);
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

W61_Status_t W61_Ble_Connect(W61_Object_t *Obj, uint32_t conn_handle, uint8_t *RemoteBDAddr)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char status_buf[sizeof(AT_ERROR_STRING) - 1] = {'\0'};
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(RemoteBDAddr);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT + W61_BLE_CONNECT_TIMEOUT))
  {
    /* Connect to a remote BLE device. The parameters are:
       - <conn_handle>:  Connection handle
       - <RemoteBDAddr>: Remote device address. The address is in the form of "XX:XX:XX:XX:XX:XX" */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLECONN=%" PRIu32 ",\"%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16
             "\"\r\n",
             conn_handle, RemoteBDAddr[0], RemoteBDAddr[1], RemoteBDAddr[2],
             RemoteBDAddr[3], RemoteBDAddr[4], RemoteBDAddr[5]);
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      /* W61_ATD_Recv Timeout is increased to manage connection processing time */
      if (W61_ATD_Recv(Obj, (uint8_t *)status_buf, W61_ATD_RSP_SIZE, W61_BLE_CONNECT_TIMEOUT) != 0)
      {
        ret = W61_AT_ParseOkErr(status_buf);
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
}

W61_Status_t W61_Ble_SetDataLength(W61_Object_t *Obj, uint32_t conn_handle, uint32_t tx_bytes, uint32_t tx_trans_time)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the BLE data length. The parameters are:
       - <conn_handle>:     Connection handle
       - <tx_bytes>:        Maximum number of bytes to be sent in a single packet
                            The range is [27, 251] bytes.
       - <tx_time>:         Maximum transmission time in microseconds
                            The range is [0, 2120] microseconds. */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEDATALEN=%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
             conn_handle, tx_bytes, tx_trans_time);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

/* GATT Server APIs */
W61_Status_t W61_Ble_CreateService(W61_Object_t *Obj, uint8_t service_index, const char *service_uuid,
                                   uint8_t uuid_type)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Create a GATT service. The parameters are:
       - <service_index>:  Service index
       - <service_uuid>:   Service UUID. The UUID is in the form of
                           "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" or "XXXX"
       - <uuid_type>:      UUID type.
                           0: 16-bit UUID
                           2: 128-bit UUID */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTSSRVCRE=%" PRIu16
             ",\"%s\",1,%" PRIu16 "\r\n", service_index, service_uuid, uuid_type);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_DeleteService(W61_Object_t *Obj, uint8_t service_index)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Delete a GATT service. The parameters are:
       - <service_index>:  Service index */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTSSRVDEL=%" PRIu16 "\r\n", service_index);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetService(W61_Object_t *Obj, W61_Ble_Service_t *ServiceInfo, int8_t service_index)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char service_uuid_tmp[33] = {0};
  int32_t cmp;
  int32_t resp_len;
  uint32_t tmp_service_idx = W61_BLE_MAX_SERVICE_NBR + 1;
  uint32_t tmp_service_type = 0;
  uint32_t tmp_uuid_type = 0;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Discover the GATT services. The parameters are:
       - <service_index>:  Service index
       The response is in the form of
       +BLEGATTSSRV:<ServiceIndex>,<ServiceUUID>,<ServiceType>,<UUIDType>
       Multiple services can be returned in the response. */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTSSRV?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      resp_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
      if (resp_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }
      while (resp_len > 0)
      {
        cmp = strncmp((char *) Obj->CmdResp, "+BLEGATTSSRV:", sizeof("+BLEGATTSSRV:") - 1);
        if (cmp == 0)
        {
          memset(service_uuid_tmp, 0x0, sizeof(service_uuid_tmp));
          if (sscanf((char *)Obj->CmdResp,
                     "+BLEGATTSSRV:%" SCNu32 ",\"%32[^\"]\",%" SCNu32 ",%" SCNu32 "\r\n",
                     &tmp_service_idx, service_uuid_tmp, &tmp_service_type, &tmp_uuid_type) != 4)
          {
            LogError("%s\n", W61_Parsing_Error_str);
            ret = W61_STATUS_ERROR;
          }
          else if (tmp_service_idx < W61_BLE_MAX_SERVICE_NBR)
          {
            if (tmp_service_idx == service_index)
            {
              ServiceInfo->service_idx = tmp_service_idx;
              ServiceInfo->service_type = tmp_service_type;
              ServiceInfo->uuid_type = tmp_uuid_type;
              memset(ServiceInfo->service_uuid, 0, sizeof(ServiceInfo->service_uuid));
              hexStringToByteArray(service_uuid_tmp, ServiceInfo->service_uuid, 16);
            }
            resp_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
          }
        }
        else if (W61_AT_ParseOkErr((char *)Obj->CmdResp) == W61_STATUS_OK)
        {
          resp_len = 0;
          ret = W61_STATUS_OK;
        }
        else
        {
          resp_len = 0;
          ret = W61_STATUS_ERROR;  /* Unexpected answer */
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

W61_Status_t W61_Ble_CreateCharacteristic(W61_Object_t *Obj, uint8_t service_index, uint8_t char_index,
                                          const char *char_uuid, uint8_t uuid_type, uint8_t char_property,
                                          uint8_t char_permission)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
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
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEGATTSCHARCRE=%" PRIu16 ",%" PRIu16 ",\"%s\",%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
             service_index, char_index, char_uuid, char_property, char_permission, uuid_type);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_GetCharacteristic(W61_Object_t *Obj, W61_Ble_Characteristic_t *CharacInfo, int8_t service_index,
                                       int8_t char_index)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  char charac_uuid_tmp[33] = {0};
  int32_t cmp;
  int32_t resp_len;
  uint32_t tmp_srv_idx = W61_BLE_MAX_SERVICE_NBR + 1;
  uint32_t tmp_char_idx = W61_BLE_MAX_CHAR_NBR + 1;
  uint32_t tmp_char_prop = 0;
  uint32_t tmp_char_perm = 0;
  uint32_t tmp_uuid_type = 0;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Discover the GATT characteristics. The parameters are:
       - <service_index>:  Service index
       - <char_index>:     Characteristic index
       The response is in the form of
       +BLEGATTSCHAR:<ServiceIndex>,<CharIndex>,<CharUUID>,<CharProperty>,<CharPermission>,<UUIDType>
       Multiple characteristics can be returned in the response. */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTSCHAR?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      resp_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
      if (resp_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }
      while (resp_len > 0)
      {
        cmp = strncmp((char *) Obj->CmdResp, "+BLEGATTSCHAR:", sizeof("+BLEGATTSCHAR:") - 1);
        if (cmp == 0)
        {
          memset(charac_uuid_tmp, 0x0, sizeof(charac_uuid_tmp));
          if (sscanf((char *)Obj->CmdResp,
                     "+BLEGATTSCHAR:%" SCNu32 ",%" SCNu32 ",\"%32[^\"]\",%" SCNu32 ",%" SCNu32 ",%" SCNu32 "\r\n",
                     &tmp_srv_idx, &tmp_char_idx, charac_uuid_tmp, &tmp_char_prop, &tmp_char_perm, &tmp_uuid_type) != 6)
          {
            LogError("%s\n", W61_Parsing_Error_str);
            ret = W61_STATUS_ERROR;
          }
          else if ((tmp_srv_idx < W61_BLE_MAX_SERVICE_NBR) && (tmp_char_idx < W61_BLE_MAX_CHAR_NBR))
          {
            if ((tmp_srv_idx == service_index) && (tmp_char_idx == char_index))
            {
              CharacInfo->char_idx = tmp_char_idx;
              memset(CharacInfo->char_uuid, 0, sizeof(CharacInfo->char_uuid));
              hexStringToByteArray(charac_uuid_tmp, CharacInfo->char_uuid, 16);
              CharacInfo->char_property = tmp_char_prop;
              CharacInfo->char_permission = tmp_char_perm;
              CharacInfo->uuid_type = tmp_uuid_type;
            }
            resp_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
          }
        }
        else if (W61_AT_ParseOkErr((char *)Obj->CmdResp) == W61_STATUS_OK)
        {
          resp_len = 0;
          ret = W61_STATUS_OK;
        }
        else
        {
          resp_len = 0;
          ret = W61_STATUS_ERROR;  /* Unexpected answer */
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

W61_Status_t W61_Ble_RegisterCharacteristics(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Register the GATT services and characteristics */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTSREGISTER=1\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_Ble_ServerSendNotification(W61_Object_t *Obj, uint8_t service_index, uint8_t char_index,
                                            uint8_t *pdata, uint32_t req_len, uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (req_len > SPI_XFER_MTU_BYTES)
  {
    req_len = SPI_XFER_MTU_BYTES;
  }

  *SentLen = req_len;

  if (W61_ATlock(Obj, Timeout))
  {
    /* W61_AT_RequestSendData timeout should let the time to NCP to return SEND:ERROR message */
    if (Timeout < W61_BLE_TIMEOUT)
    {
      Timeout = W61_BLE_TIMEOUT;
    }
    /* The command AT+BLEGATTSNTFY is used to send a notification to the client.
       The parameters are:
       - <service_index>:  Service index
       - <char_index>:     Characteristic index
       - <req_len>:        Length of the data to be sent.
                           Maximum data length is 244. */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEGATTSNTFY=%" PRIu16 ",%" PRIu16 ",%" PRIu32 "\r\n",
             service_index, char_index, req_len);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, Timeout);
    if (ret == W61_STATUS_OK)
    {
      /* Send the data to the client */
      ret = W61_AT_RequestSendData(Obj, pdata, req_len, Timeout);
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

W61_Status_t W61_Ble_ServerSendIndication(W61_Object_t *Obj, uint8_t service_index, uint8_t char_index,
                                          uint8_t *pdata, uint32_t req_len, uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (req_len > SPI_XFER_MTU_BYTES)
  {
    req_len = SPI_XFER_MTU_BYTES;
  }

  *SentLen = req_len;

  if (W61_ATlock(Obj, Timeout))
  {
    /* W61_AT_RequestSendData timeout should let the time to NCP to return SEND:ERROR message */
    if (Timeout < W61_BLE_TIMEOUT)
    {
      Timeout = W61_BLE_TIMEOUT;
    }
    /* The command AT+BLEGATTSIND is used to send an indication to the client.
       The parameters are:
       - <service_index>:  Service index
       - <char_index>:     Characteristic index
       - <req_len>:        Length of the data to be sent.
                           Maximum data length is 244. */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEGATTSIND=%" PRIu16 ",%" PRIu16 ",%" PRIu32 "\r\n",
             service_index, char_index, req_len);
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Timeout) > 0)
    {
      /* Send the data to the client */
      ret = W61_AT_RequestSendData(Obj, pdata, req_len, Timeout);
    }
    else
    {
      ret = W61_STATUS_ERROR;
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

W61_Status_t W61_Ble_ServerSetReadData(W61_Object_t *Obj, uint8_t service_index, uint8_t char_index, uint8_t *pdata,
                                       uint32_t req_len, uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret = W61_STATUS_ERROR;
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

  if (W61_ATlock(Obj, Timeout))
  {
    /* W61_AT_RequestSendData timeout should let the time to NCP to return SEND:ERROR message */
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
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEGATTSRD=%" PRIu16 ",%" PRIu16 ",%" PRIu32 "\r\n",
             service_index, char_index, req_len);
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      ret = W61_AT_RequestSendData(Obj, pdata, req_len, Timeout);
    }
    else
    {
      ret = W61_STATUS_ERROR;
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

/* GATT Client APIs */
W61_Status_t W61_Ble_RemoteServiceDiscovery(W61_Object_t *Obj, uint8_t connection_handle)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Discover the GATT services. The parameters are:
       - <connection_handle>:  Connection handle
       The available services will be returned as W61_BLE_EVT_SERVICE_FOUND_ID events */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTCSRVDIS=%" PRIu16 "\r\n",
             connection_handle);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  return ret;
}

W61_Status_t W61_Ble_RemoteCharDiscovery(W61_Object_t *Obj, uint8_t connection_handle, uint8_t service_index)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Discover the GATT characteristics. The parameters are:
       - <connection_handle>:  Connection handle
       - <service_index>:      Service index
       The available characteristics will be returned as W61_BLE_EVT_CHAR_FOUND_ID events */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTCCHARDIS=%" PRIu16 ",%" PRIu16 "\r\n",
             connection_handle, service_index);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  return ret;
}

W61_Status_t W61_Ble_ClientWriteData(W61_Object_t *Obj, uint8_t conn_handle, uint8_t service_index, uint8_t char_index,
                                     uint8_t *pdata, uint32_t req_len, uint32_t *SentLen, uint32_t Timeout)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(pdata);
  W61_NULL_ASSERT(SentLen);

  if (req_len > SPI_XFER_MTU_BYTES)
  {
    req_len = SPI_XFER_MTU_BYTES;
  }

  *SentLen = req_len;

  if (W61_ATlock(Obj, Timeout))
  {
    /* W61_AT_RequestSendData timeout should let the time to NCP to return SEND:ERROR message */
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
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEGATTCWR=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu32 "\r\n",
             conn_handle, service_index, char_index, req_len);
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      /* Send the data to the server */
      ret = W61_AT_RequestSendData(Obj, pdata, req_len, Timeout);
    }
    else
    {
      ret = W61_STATUS_ERROR;
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  if (ret != W61_STATUS_OK)
  {
    *SentLen = 0;
  }
  return ret;
}

W61_Status_t W61_Ble_ClientReadData(W61_Object_t *Obj, uint8_t conn_handle, uint8_t service_index, uint8_t char_index)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Send a read request to the server characteristic.
       The parameters are:
       - <conn_handle>:     Connection handle
       - <service_index>:   Service index
       - <char_index>:      Characteristic index
       The response will be returned as W61_BLE_EVT_READ_ID event */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTCRD=%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
             conn_handle, service_index, char_index);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_Ble_ClientSubscribeChar(W61_Object_t *Obj, uint8_t conn_handle, uint8_t char_value_handle,
                                         uint8_t char_prop)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t char_desc_handle = char_value_handle + 1;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Subscribe to a characteristic. The parameters are:
       - <conn_handle>:        Connection handle
       - <char_desc_handle>:   Characteristic descriptor handle
       - <char_value_handle>:  Characteristic value handle
       - <char_prop>:          Characteristic property */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLEGATTCSUBSCRIBE=%" PRIu16 ",%" PRIu16 ",%" PRIu16 ",%" PRIu16 "\r\n",
             conn_handle, char_desc_handle, char_value_handle, char_prop);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_Ble_ClientUnsubscribeChar(W61_Object_t *Obj, uint8_t conn_handle, uint8_t char_value_handle)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Unsubscribe from a characteristic. The parameters are:
       - <conn_handle>:        Connection handle
       - <char_value_handle>:  Characteristic value handle */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLEGATTCUNSUBSCRIBE=%" PRIu16 ",%" PRIu16 "\r\n",
             conn_handle, char_value_handle);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

/* Security APIs */
W61_Status_t W61_Ble_SetSecurityParam(W61_Object_t *Obj, uint8_t security_parameter)
{
  W61_Status_t ret = W61_STATUS_ERROR;

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the security management related I/O capabilities. The parameters are:
       - <security_parameter>:  Security parameter
                                0: Display only
                                1: Display with Yes/No-buttons
                                2: Keyboard only
                                3: No input and no output
                                4: Display with keyboard */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESECPARAM=%" PRIu16 "\r\n",
             security_parameter);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_Ble_GetSecurityParam(W61_Object_t *Obj, uint32_t *SecurityParameter)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(SecurityParameter);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the security management related I/O capabilities. The response is in the form of
       +BLESECPARAM:<SecurityParameter> */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESECPARAM?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_BLE_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      if (sscanf((char *)Obj->CmdResp, "+BLESECPARAM:%" SCNu32 "\r\n", SecurityParameter) != 1)
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
  return ret;
}

W61_Status_t W61_Ble_SecurityStart(W61_Object_t *Obj, uint8_t conn_handle, uint8_t security_level)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Start the security management. The parameters are:
       - <conn_handle>:     Connection handle
       - <security_level>:  Security level
                            0: Only for BR/EDR special cases
                            1: No encryption and no authentication
                            2: Encryption and no authentication (no MITM)
                            3: Encryption and authentication (MITM)
                            4: Authenticated Secure Connections and 128-bit key */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESECSTART=%" PRIu16 ",%" PRIu16 "\r\n",
             conn_handle, security_level);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_Ble_SecurityPassKeyConfirm(W61_Object_t *Obj, uint8_t conn_handle)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Request the passkey confirm. The parameters are:
       - <conn_handle>:     Connection handle
       The response will be returned as W61_BLE_EVT_PASSKEY_CONFIRM_ID event */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESECPASSKEYCONFIRM=%" PRIu16 "\r\n",
             conn_handle);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_Ble_SecurityPairingConfirm(W61_Object_t *Obj, uint8_t conn_handle)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Request the pairing confirm. The parameters are:
       - <conn_handle>:     Connection handle
       The response will be returned as W61_BLE_EVT_PAIRING_CONFIRM_ID event */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESECPAIRINGCONFIRM=%" PRIu16 "\r\n",
             conn_handle);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_Ble_SecuritySetPassKey(W61_Object_t *Obj, uint8_t conn_handle, uint32_t passkey)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Set the passkey. The parameters are:
       - <conn_handle>:     Connection handle
       - <passkey>:         Passkey
       The response will be returned as W61_BLE_EVT_PASSKEY_CONFIRM_ID event */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLESECPASSKEY=%" PRIu16 ",%06" PRIu32 "\r\n",
             conn_handle, passkey);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_Ble_SecurityPairingCancel(W61_Object_t *Obj, uint8_t conn_handle)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Cancel the pairing. The parameters are:
       - <conn_handle>:     Connection handle */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESECCANNEL=%" PRIu16 "\r\n",
             conn_handle);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_Ble_SecurityUnpair(W61_Object_t *Obj, uint8_t *RemoteBDAddr, uint32_t remote_addr_type)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Unpair a remote device. The parameters are:
       - <RemoteBDAddr>:     Remote device BD address
       - <remote_addr_type>: Remote device address type
                             0: Public address
                             1: Random address */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+BLESECUNPAIR=\"%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16 ":%02" PRIX16
             "\",%" PRIu32 "\r\n",
             RemoteBDAddr[0], RemoteBDAddr[1], RemoteBDAddr[2],
             RemoteBDAddr[3], RemoteBDAddr[4], RemoteBDAddr[5],
             remote_addr_type);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_BLE_TIMEOUT);
    W61_ATunlock(Obj);
  }

  return ret;
}

W61_Status_t W61_Ble_SecurityGetBondedDeviceList(W61_Object_t *Obj,
                                                 W61_Ble_Bonded_Devices_Result_t *RemoteBondedDevices)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t count;  /* Number of bonded devices */
  char tmp_bondaddr[18];
  int32_t recv_len;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(RemoteBondedDevices);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    /* Query the bonded devices. The parameters are:
       - <bonded_device_list>:  List of bonded devices
       The response is in the form of
       +BLESECGETLTKLIST: BONDADDR <bonded_device_list>
       Multiple bonded devices can be returned in the response. */
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+BLESECGETLTKLIST?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
      if (recv_len <= 0)
      {
        ret = W61_STATUS_TIMEOUT;
        W61_ATunlock(Obj);
        return ret;
      }
      count = 0;
      while ((recv_len > 0) &&
             (strncmp((char *)Obj->CmdResp, "+BLESECGETLTKLIST: ", sizeof("+BLESECGETLTKLIST:") - 1) == 0) &&
             (count < W61_BLE_MAX_BONDED_DEVICES))
      {
        if (sscanf((char *)Obj->CmdResp, "+BLESECGETLTKLIST: BONDADDR %17[^ ]", tmp_bondaddr) == 1)
        {
          Parser_StrToMAC(tmp_bondaddr, RemoteBondedDevices->Bonded_device[count].BDAddr);
          count++;
        }
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_BLE_TIMEOUT);
      }
      if (recv_len <= 0)
      {
        /* OK/ERROR message not received */
        ret = W61_STATUS_TIMEOUT;
      }
      else
      {
        ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
        RemoteBondedDevices->Count = count;
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

/* Private Functions Definition ----------------------------------------------*/
static void W61_Ble_AT_Event(void *hObj, const uint8_t *rxbuf, int32_t rxbuf_len)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  char *ptr = (char *)rxbuf;
  char *token;
  W61_Ble_CbParamData_t cb_param_ble_data = {0};
  uint8_t conn_handle;
  int32_t tmp = 0;

  (void)rxbuf_len;

  if ((Obj == NULL) || (Obj->ulcbs.UL_ble_cb == NULL))
  {
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_CONNECTION_PARAM_KEYWORD, sizeof(W61_BLE_EVT_CONNECTION_PARAM_KEYWORD) - 1) == 0)
  {
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_CONNECTION_PARAM_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_CONNECTED_KEYWORD, sizeof(W61_BLE_EVT_CONNECTED_KEYWORD) - 1) == 0)
  {
    ptr += sizeof(W61_BLE_EVT_CONNECTED_KEYWORD);
    Parser_StrToInt(ptr, NULL, &tmp);
    conn_handle = (uint8_t)tmp;
    cb_param_ble_data.remote_ble_device.conn_handle = conn_handle;
    ptr += sizeof(conn_handle) + sizeof(",");
    token = strstr(ptr, "\r");
    if (token != NULL)
    {
      *(--token) = 0;
      Parser_StrToMAC(ptr, Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].BDAddr);
    }
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].IsConnected = 1;
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].conn_handle = conn_handle;
    Obj->BleCtx.NetSettings.DeviceConnectedNb++;

    cb_param_ble_data.remote_ble_device = Obj->BleCtx.NetSettings.RemoteDevice[conn_handle];

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_CONNECTED_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_DISCONNECTED_KEYWORD, sizeof(W61_BLE_EVT_DISCONNECTED_KEYWORD) - 1) == 0)
  {
    ptr += sizeof(W61_BLE_EVT_DISCONNECTED_KEYWORD);
    Parser_StrToInt(ptr, NULL, &tmp);
    conn_handle = (uint8_t)tmp;
    cb_param_ble_data.remote_ble_device.conn_handle = conn_handle;
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].IsConnected = 0;
    Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].conn_handle = 0xf;
    for (int32_t i = 0; i < W61_BLE_BD_ADDR_SIZE; i++)
    {
      Obj->BleCtx.NetSettings.RemoteDevice[conn_handle].BDAddr[i] = 0x0;
    }
    Obj->BleCtx.NetSettings.DeviceConnectedNb--;
    cb_param_ble_data.remote_ble_device = Obj->BleCtx.NetSettings.RemoteDevice[conn_handle];
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_DISCONNECTED_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_INDICATION_STATUS_KEYWORD, sizeof(W61_BLE_EVT_INDICATION_STATUS_KEYWORD) - 1) == 0)
  {
    uint32_t event_type = 0xFF;
    uint32_t arg_1 = 0xFF;
    uint32_t arg_2 = 0xFF;
    int32_t sscanf_result = 0;
    ptr += sizeof(W61_BLE_EVT_INDICATION_STATUS_KEYWORD);
    sscanf_result = sscanf(ptr, "%" SCNu32 ",%" SCNu32 ",%" SCNu32, &event_type, &arg_1, &arg_2);

    if ((sscanf_result == 2) && (event_type == 2))
    {
      if (arg_1 == 0)
      {
        /* Indication Complete event */
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_INDICATION_ACK_ID, &cb_param_ble_data);
      }
      else
      {
        /* Indication Not Complete event */
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_INDICATION_NACK_ID, &cb_param_ble_data);
      }
    }
    else if (sscanf_result == 3)
    {
      cb_param_ble_data.service_idx = arg_1;
      cb_param_ble_data.charac_idx = arg_2;
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

  if (strncmp(ptr, W61_BLE_EVT_NOTIFICATION_STATUS_KEYWORD, sizeof(W61_BLE_EVT_NOTIFICATION_STATUS_KEYWORD) - 1) == 0)
  {
    uint32_t status = 0;
    uint32_t service_idx = 0;
    uint32_t charac_idx = 0;
    ptr += sizeof(W61_BLE_EVT_NOTIFICATION_STATUS_KEYWORD);
    if (sscanf(ptr, "%" SCNu32 ",%" SCNu32 ",%" SCNu32, &status, &service_idx, &charac_idx) != 3)
    {
      return;
    }
    cb_param_ble_data.service_idx = service_idx;
    cb_param_ble_data.charac_idx = charac_idx;
    cb_param_ble_data.notification_status[service_idx] = status;

    if (status)
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

  if (strncmp(ptr, W61_BLE_EVT_NOTIFICATION_DATA_KEYWORD, sizeof(W61_BLE_EVT_NOTIFICATION_DATA_KEYWORD) - 1) == 0)
  {
    /* Process data */
    char *str_token;
    ptr += sizeof(W61_BLE_EVT_NOTIFICATION_DATA_KEYWORD);
    str_token = strtok(ptr, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.remote_ble_device.conn_handle = (uint32_t)tmp;
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.available_data_length = (uint32_t)tmp;
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_NOTIFICATION_DATA_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_WRITE_KEYWORD, sizeof(W61_BLE_EVT_WRITE_KEYWORD) - 1) == 0)
  {
    /* Process data */
    char *str_token;
    ptr += sizeof(W61_BLE_EVT_WRITE_KEYWORD);
    str_token = strtok(ptr, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)tmp;
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.service_idx = (uint8_t)tmp;
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.charac_idx = (uint8_t)tmp;
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.available_data_length = (uint32_t)tmp;
    if (cb_param_ble_data.available_data_length != 0)
    {
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_WRITE_ID, &cb_param_ble_data);
    }
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_READ_KEYWORD, sizeof(W61_BLE_EVT_READ_KEYWORD) - 1) == 0)
  {
    /* Process data */
    char *str_token;
    ptr += sizeof(W61_BLE_EVT_READ_KEYWORD);
    str_token = strtok(ptr, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)tmp;
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.service_idx = (uint8_t)tmp;
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.charac_idx = (uint8_t)tmp;
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.available_data_length = (uint32_t)tmp;
    if (cb_param_ble_data.available_data_length != 0)
    {
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_READ_ID, &cb_param_ble_data);
    }
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_SCAN_KEYWORD, sizeof(W61_BLE_EVT_SCAN_KEYWORD) - 1) == 0)
  {
    TickType_t currentTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
    if ((Obj->BleCtx.ScanResults.Count < W61_BLE_MAX_DETECTED_PERIPHERAL) && (ScanComplete == 0) && \
        (currentTime - startScanTime) <= ((TickType_t) pdMS_TO_TICKS(W61_BLE_SCAN_TIMEOUT)))
    {
      W61_Ble_AT_ParsePeripheral((char *)rxbuf, rxbuf_len, &(Obj->BleCtx.ScanResults));
    }
    else
    {
      if ((Obj->ulcbs.UL_ble_cb != NULL) && (ScanComplete == 0))
      {
        ScanComplete = 1;
        Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_SCAN_DONE_ID, NULL);
      }
    }
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_CHAR_FOUND_KEYWORD, sizeof(W61_BLE_EVT_CHAR_FOUND_KEYWORD) - 1) == 0)
  {
    /* Get connection handle */
    ptr += sizeof(W61_BLE_EVT_CHAR_FOUND_KEYWORD);
    Parser_StrToInt(ptr, NULL, &tmp);
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)tmp;
    /* Get service */
    W61_Ble_AT_ParseServiceCharac((char *)rxbuf, rxbuf_len, &cb_param_ble_data.remote_ble_device.Service);
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_CHAR_FOUND_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_SERVICE_FOUND_KEYWORD, sizeof(W61_BLE_EVT_SERVICE_FOUND_KEYWORD) - 1) == 0)
  {
    /* Get connection handle */
    ptr += sizeof(W61_BLE_EVT_SERVICE_FOUND_KEYWORD);
    Parser_StrToInt(ptr, NULL, &tmp);
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)tmp;

    /* Get service */
    W61_Ble_AT_ParseService((char *)rxbuf, rxbuf_len, &cb_param_ble_data.remote_ble_device.Service);
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_SERVICE_FOUND_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_MTU_SIZE_KEYWORD, sizeof(W61_BLE_EVT_MTU_SIZE_KEYWORD) - 1) == 0)
  {
    /* Get connection handle */
    char *str_token;
    ptr += sizeof(W61_BLE_EVT_MTU_SIZE_KEYWORD);
    str_token = strtok(ptr, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.remote_ble_device.conn_handle = (uint8_t)tmp;

    /* Get MTU size */
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.mtu_size = (uint16_t)tmp;

    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_MTU_SIZE_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_PASSKEY_CONFIRM_KEYWORD, sizeof(W61_BLE_EVT_PASSKEY_CONFIRM_KEYWORD) - 1) == 0)
  {
    /* Process data */
    char *str_token;
    ptr += sizeof(W61_BLE_EVT_PASSKEY_CONFIRM_KEYWORD);
    str_token = strstr(ptr, "PASSKEY:");
    if (str_token == NULL)
    {
      return;
    }
    str_token = str_token + sizeof("PASSKEY:") - 1;
    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_ble_data.PassKey = (uint32_t)tmp;
    if (cb_param_ble_data.PassKey != 0)
    {
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PASSKEY_CONFIRM_ID, &cb_param_ble_data);
    }
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_PASSKEY_ENTRY_KEYWORD, sizeof(W61_BLE_EVT_PASSKEY_ENTRY_KEYWORD) - 1) == 0)
  {
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PASSKEY_ENTRY_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_PASSKEY_DISPLAY_KEYWORD, sizeof(W61_BLE_EVT_PASSKEY_DISPLAY_KEYWORD) - 1) == 0)
  {
    /* Process Passkey */
    ptr += sizeof(W61_BLE_EVT_PASSKEY_DISPLAY_KEYWORD);
    Parser_StrToInt(ptr, NULL, &tmp);
    cb_param_ble_data.PassKey = (uint32_t)tmp;
    if (cb_param_ble_data.PassKey != 0)
    {
      Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PASSKEY_DISPLAY_ID, &cb_param_ble_data);
    }
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_PAIRING_COMPLETED_KEYWORD, sizeof(W61_BLE_EVT_PAIRING_COMPLETED_KEYWORD) - 1) == 0)
  {
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PAIRING_COMPLETED_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_PAIRING_FAILED_KEYWORD, sizeof(W61_BLE_EVT_PAIRING_FAILED_KEYWORD) - 1) == 0)
  {
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PAIRING_FAILED_ID, &cb_param_ble_data);
    return;
  }

  if (strncmp(ptr, W61_BLE_EVT_PAIRING_CONFIRM_KEYWORD, sizeof(W61_BLE_EVT_PAIRING_CONFIRM_KEYWORD) - 1) == 0)
  {
    Obj->ulcbs.UL_ble_cb(W61_BLE_EVT_PAIRING_CONFIRM_ID, &cb_param_ble_data);
    return;
  }
}

static void hexStringToByteArray(const char *hexString, char *byteArray, size_t byteArraySize)
{
  size_t hexStringLength = strlen(hexString);
  for (size_t i = 0; i < hexStringLength / 2 && i < byteArraySize; i++)
  {
    uint32_t byte;
    if (sscanf(hexString + 2 * i, "%02" SCNx32, &byte) != 1)
    {
      return;
    }
    byteArray[i] = byte;
  }
}

static void W61_Ble_AT_ParsePeripheral(char *pdata, int32_t len, W61_Ble_Scan_Result_t *ScanResults)
{
  int32_t rssi = 0;
  int32_t bd_addr_type = -1;
  char mac[18] = {0};
  uint8_t mac_ui8[W61_BLE_BD_ADDR_SIZE] = {0};
  char adv_data[100] = {0};
  char scan_rsp_data[100] = {0};

  int32_t ret = sscanf(pdata, "+BLE:SCAN:\"%[^\"]\",%" SCNd32 ",%[^,],%[^,],%" SCNd32,
                       mac, &rssi, adv_data, scan_rsp_data, &bd_addr_type);
  if (ret < 5)
  {
    /* Handle the case where scan_rsp_data is empty */
    ret = sscanf(pdata, "+BLE:SCAN:\"%[^\"]\",%" SCNd32 ",%[^,],,%" SCNd32,
                 mac, &rssi, adv_data, &bd_addr_type);
    if (ret == 4)
    {
      /* Set scan_rsp_data to an empty string */
      scan_rsp_data[0] = '\0';
    }
    /* Handle the case where adv_data is empty */
    else if (ret == 2)
    {
      ret = sscanf(pdata, "+BLE:SCAN:\"%[^\"]\",%" SCNd32 ",,,%" SCNd32, mac, &rssi, &bd_addr_type);
      if (ret == 3)
      {
        /* Set adv_data and scan_rsp_data to an empty string */
        adv_data[0] = '\0';
        scan_rsp_data[0] = '\0';
      }
      else
      {
        /* Handle the case where adv_data is empty and scan_rsp_data filled*/
        ret = sscanf(pdata, "+BLE:SCAN:\"%[^\"]\",%" SCNd32 ",,%[^,],%" SCNd32,
                     mac, &rssi, scan_rsp_data, &bd_addr_type);
        if (ret == 4)
        {
          /* Set adv_data to an empty string */
          adv_data[0] = '\0';
        }
      }
    }
  }

  if (ret == 5 || ret == 4 || ret == 3)
  {
    uint32_t index = 0;
    Parser_StrToMAC(mac, mac_ui8);

    for (; index < ScanResults->Count; index++)
    {
      if (memcmp(mac_ui8, ScanResults->Detected_Peripheral[index].BDAddr, W61_BLE_BD_ADDR_SIZE) == 0)
      {
        break;
      }
    }

    if (index < W61_BLE_MAX_DETECTED_PERIPHERAL)
    {
      /* Update data and RSSI */
      ScanResults->Detected_Peripheral[index].RSSI = rssi;
      W61_Ble_AnalyzeAdvData(adv_data, ScanResults, index);
      W61_Ble_AnalyzeAdvData(scan_rsp_data, ScanResults, index);

      if (index == ScanResults->Count)
      {
        /* New peripheral detected, fill BD address and address type */
        Parser_StrToMAC(mac, ScanResults->Detected_Peripheral[index].BDAddr);
        ScanResults->Detected_Peripheral[index].bd_addr_type = bd_addr_type;
        ScanResults->Count++;
      }
    }
  }
}

static void W61_Ble_AT_ParseService(char *pdata, int32_t len, W61_Ble_Service_t *Service)
{
  uint8_t num = 0;
  char buf[101] = {0};
  char tmp_uuid[33] = {0};
  char *ptr;
  uint32_t j = 0;
  int32_t tmp = 0;

  memcpy(buf, pdata, len);

  /* Parsing the string separated by , */
  ptr = strtok(buf, ",");
  /* Looping while the ptr reach the length of the data received or there is no new token (,) to parse (end of string)*/
  while ((ptr != NULL) && (buf + len - 4 > ptr) && (num < 4))
  {
    switch (num++)
    {
      case 0:
        /* Connection handle */
        ptr = strtok(NULL, ",");
        break;
      case 1:
        /* Service Index */
        Parser_StrToInt(ptr, NULL, &tmp);
        Service->service_idx = (uint8_t)tmp;
        ptr = strtok(NULL, ",");
        break;
      case 2:
        /* UUID */
        for (int32_t i = 0; (i < 32); i++)
        {
          if (ptr[j] == 0x00) /* end of UUID */
          {
            ptr[i] = 0x00;
          }
          else
          {
            if (ptr[j] == '-')
            {
              j++;
            }
            tmp_uuid[i] = ptr[j];
            j++;
          }
        }

        if (j == 4) /* Short UUID */
        {
          Service->uuid_type = W61_BLE_UUID_TYPE_16;
        }
        else /* Long UUID */
        {
          Service->uuid_type = W61_BLE_UUID_TYPE_128;
        }
        memset(Service->service_uuid, 0, sizeof(Service->service_uuid));
        hexStringToByteArray(tmp_uuid, Service->service_uuid, W61_BLE_MAX_UUID_SIZE);
        ptr = strtok(NULL, ",");
        break;

      case 3:
        /* Service type */
        Parser_StrToInt(ptr, NULL, &tmp);
        Service->service_type = (uint8_t)tmp;
        break;
    }
  }
}

static void W61_Ble_AT_ParseServiceCharac(char *pdata, int32_t len, W61_Ble_Service_t *Service)
{
  uint8_t num = 0;
  char buf[101] = {0};
  char tmp_uuid[33] = {0};
  char *ptr;
  uint32_t j = 0;
  int32_t tmp = 0;

  memcpy(buf, pdata, len);

  /* Parsing the string separated by , */
  ptr = strtok(buf, ",");
  /* Looping while the ptr reach the length of the data received or there is no new token (,) to parse (end of string)*/
  while ((ptr != NULL) && (buf + len - 3 > ptr) && (num < 7))
  {
    switch (num++)
    {
      case 0:
        /* Connection handle */
        ptr = strtok(NULL, ",");
        break;
      case 1:
        /* Service Index */
        Parser_StrToInt(ptr, NULL, &tmp);
        Service->service_idx = (uint8_t)tmp;
        ptr = strtok(NULL, ",");
        break;
      case 2:
        /* Characteristic Index */
        Parser_StrToInt(ptr, NULL, &tmp);
        Service->charac.char_idx = (uint8_t)tmp;
        ptr = strtok(NULL, ",");
        break;
      case 3:
        /* UUID */
        for (int32_t i = 0; (i < 32); i++)
        {
          if (ptr[j] == 0x00) /* end of UUID */
          {
            break;
          }
          else
          {
            if (ptr[j] == '-')
            {
              j++;
            }
            tmp_uuid[i] = ptr[j];
            j++;
          }
        }

        if (j == 4) /* Short UUID */
        {
          Service->charac.uuid_type = W61_BLE_UUID_TYPE_16;
        }
        else /* Long UUID */
        {
          Service->charac.uuid_type = W61_BLE_UUID_TYPE_128;
        }
        memset(Service->charac.char_uuid, 0, sizeof(Service->charac.char_uuid));
        hexStringToByteArray(tmp_uuid, Service->charac.char_uuid, W61_BLE_MAX_UUID_SIZE);
        ptr = strtok(NULL, ",");
        break;

      case 4:
        /* Characteristic property */
        ptr += 2; /* sizeof("0x") - 1 */
        Service->charac.char_property = Parser_StrToHex(ptr, NULL);
        ptr = strtok(NULL, ",");
        break;
      case 5:
        /* Characteristic handle */
        Parser_StrToInt(ptr, NULL, &tmp);
        Service->charac.char_handle = (uint16_t)tmp;
        ptr = strtok(NULL, ",");
        break;
      case 6:
        /* Characteristic value handle */
        Parser_StrToInt(ptr, NULL, &tmp);
        Service->charac.char_value_handle = (uint16_t)tmp;
        break;
    }
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
    end_of_data_packet = *(p_adv_data + 2);

    /* First byte is data length */
    if (sscanf(p_adv_data, "%2" SCNx32, &adv_data_size) != 1)
    {
      return;
    }

    while (end_of_data_packet != 0)
    {
      if ((sscanf(p_adv_data + 2 * i, "%2" SCNx32, &adv_data_size) != 1) ||
          (sscanf(p_adv_data  + 2 * i + 2, "%2" SCNx32, &adv_data_flag) != 1))
      {
        return;
      }

      switch (adv_data_flag)
      {
        case W61_BLE_AD_TYPE_FLAGS:
          break;
        case W61_BLE_AD_TYPE_MANUFACTURER_SPECIFIC_DATA:
        {
          hexStringToByteArray(p_adv_data + 2 * i + 4,
                               (char *) Peripherals->Detected_Peripheral[index].ManufacturerData,
                               adv_data_size - 1);
          break;
        }
        case W61_BLE_AD_TYPE_COMPLETE_LOCAL_NAME:
        {
          hexStringToByteArray(p_adv_data + 2 * i + 4,
                               (char *) Peripherals->Detected_Peripheral[index].DeviceName,
                               adv_data_size - 1);
          break;
        }
        default:
          break;
      } /* end of switch */

      i = i + adv_data_size + 1;

      /* check enf of decoded data packet */
      end_of_data_packet = *(p_adv_data + 2 * i);
    }
  }
}

/** @} */
