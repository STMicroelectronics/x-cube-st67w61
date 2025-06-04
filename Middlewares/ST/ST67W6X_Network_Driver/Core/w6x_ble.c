/**
  ******************************************************************************
  * @file    w6x_ble.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x BLE API
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
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @defgroup ST67W6X_Private_BLE_Macros ST67W6X BLE Macros
  * @ingroup  ST67W6X_Private_BLE
  * @{
  */
#ifndef BDADDR2STR
/** BD Address buffer to string macros */
#define BDADDR2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif /* BDADDR2STR */

#ifndef BDADDRSTR
/** BD address string format */
#define BDADDRSTR "%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16 ":%02" PRIx16
#endif /* BDADDRSTR */

#ifndef UUID128TOSTR
/** 128-bit UUID buffer to string macros */
#define UUID128TOSTR(a) \
  (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5], (a)[6], (a)[7], \
  (a)[8], (a)[9], (a)[10], (a)[11], (a)[12], (a)[13], (a)[14], (a)[15]
#endif /* UUID128TOSTR */

#ifndef UUID128STR
/** 128-bit UUID string format */
#define UUID128STR \
  "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 \
  "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16 "%02" PRIx16
#endif /* UUID128STR */
/** @} */

/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_BLE_Variables ST67W6X BLE Variables
  * @ingroup  ST67W6X_Private_BLE
  * @{
  */
static W61_Object_t *p_DrvObj = NULL; /*!< Global W61 context pointer */

/** W6X BLE init error string */
static const char W6X_Ble_Uninit_str[] = "W6X BLE module not initialized";

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Private_BLE_Functions ST67W6X BLE Functions
  * @ingroup  ST67W6X_Private_BLE
  * @{
  */

/**
  * @brief  BLE callback function
  * @param  event_id: event ID
  * @param  event_args: event arguments
  */
static void W6X_Ble_cb(W61_event_id_t event_id, void *event_args);

/**
  * @brief  Print Services and Characteristics information
  * @param  ServicesTable: Table of services and Characteristics to display
  */
static void W6X_Ble_PrintServicesAndCharacteristics(W6X_Ble_Service_t ServicesTable[]);

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_BLE_Public_Functions
  * @{
  */
W6X_Status_t W6X_Ble_GetInitMode(W6X_Ble_Mode_e *Mode)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get the BLE initialization mode */
  return TranslateErrorStatus(W61_Ble_GetInitMode(p_DrvObj, (W61_Ble_Mode_e *) Mode));
}

W6X_Status_t W6X_Ble_Init(W6X_Ble_Mode_e mode, uint8_t *p_recv_data, size_t max_len)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_App_Cb_t *p_cb_handler;
  W6X_Ble_Mode_e tmp_mode;

  /* Get the global W61 context pointer */
  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  p_cb_handler = W6X_GetCbHandler(); /* Check that application callback is registered */
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_ble_cb == NULL))
  {
    LogError("Please register the APP callback before initializing the module\n");
    return ret;
  }

  /* Register W61 driver callbacks */
  W61_RegisterULcb(p_DrvObj,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   W6X_Ble_cb);

  /* Initialize BLE Data buffer */
  ret = TranslateErrorStatus(W61_Ble_Init(p_DrvObj, (uint8_t) mode, p_recv_data, (uint32_t)max_len));
  if (W6X_STATUS_OK == ret)
  {
    ret = W6X_Ble_GetInitMode(&tmp_mode); /* Check if the BLE mode is correctly set */
    if ((W6X_STATUS_OK == ret) && (tmp_mode != mode))
    {
      ret = W6X_STATUS_ERROR; /* Error: BLE mode not correctly set */
    }
  }

  if (W6X_Ble_SetDeviceName(W6X_BLE_HOSTNAME) != W6X_STATUS_OK) /* Set device name */
  {
    LogError("Failed to set device name\n");
    ret = W6X_STATUS_ERROR;
  }

  return ret;
}

void W6X_Ble_DeInit(void)
{
  W61_Ble_DeInit(p_DrvObj); /* Deinitialize BLE */
  p_DrvObj = NULL; /* Reset the global pointer */
}

W6X_Status_t W6X_Ble_SetRecvDataPtr(uint8_t *p_recv_data, uint32_t recv_data_buf_size)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Update the BLE Data buffer */
  return TranslateErrorStatus(W61_Ble_SetRecvDataPtr(p_DrvObj, p_recv_data, recv_data_buf_size));
}

W6X_Status_t W6X_Ble_SetTxPower(uint32_t power)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set BLE Tx Power */
  return TranslateErrorStatus(W61_Ble_SetTxPower(p_DrvObj, power));
}

W6X_Status_t W6X_Ble_GetTxPower(uint32_t *Power)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get BLE Tx Power */
  return TranslateErrorStatus(W61_Ble_GetTxPower(p_DrvObj, Power));
}

W6X_Status_t W6X_Ble_AdvStart(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Start BLE advertising */
  return TranslateErrorStatus(W61_Ble_AdvStart(p_DrvObj));
}

W6X_Status_t W6X_Ble_AdvStop(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Stop BLE advertising */
  return TranslateErrorStatus(W61_Ble_AdvStop(p_DrvObj));
}

W6X_Status_t W6X_Ble_GetBDAddress(uint8_t *BdAddr)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get the BD address */
  return TranslateErrorStatus(W61_Ble_GetBDAddress(p_DrvObj, BdAddr));
}

W6X_Status_t W6X_Ble_Disconnect(uint32_t connection_handle)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Disconnect BLE */
  return TranslateErrorStatus(W61_Ble_Disconnect(p_DrvObj, connection_handle));
}

W6X_Status_t W6X_Ble_ExchangeMTU(uint32_t connection_handle)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Exchange BLE MTU length */
  return TranslateErrorStatus(W61_Ble_ExchangeMTU(p_DrvObj, connection_handle));
}

W6X_Status_t W6X_Ble_SetBdAddress(const uint8_t *bdaddr)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set the BD address */
  return TranslateErrorStatus(W61_Ble_SetBDAddress(p_DrvObj, bdaddr));
}

W6X_Status_t W6X_Ble_SetDeviceName(char *name)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set the host name */
  return TranslateErrorStatus(W61_Ble_SetDeviceName(p_DrvObj, name));
}

W6X_Status_t W6X_Ble_GetDeviceName(char *DeviceName)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get the host name */
  return TranslateErrorStatus(W61_Ble_GetDeviceName(p_DrvObj, DeviceName));
}

W6X_Status_t W6X_Ble_SetAdvData(const char *advdata)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set the advertising data */
  return TranslateErrorStatus(W61_Ble_SetAdvData(p_DrvObj, advdata));
}

W6X_Status_t W6X_Ble_SetScanRespData(const char *scanrespdata)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set the scan response data */
  return TranslateErrorStatus(W61_Ble_SetScanRespData(p_DrvObj, scanrespdata));
}

W6X_Status_t W6X_Ble_SetAdvParam(uint32_t adv_int_min, uint32_t adv_int_max,
                                 W6X_Ble_AdvType_e adv_type, W6X_Ble_AdvChannel_e adv_channel)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set advertising parameters */
  return TranslateErrorStatus(W61_Ble_SetAdvParam(p_DrvObj, adv_int_min, adv_int_max,
                                                  (uint8_t)adv_type, (uint8_t)adv_channel));
}

W6X_Status_t W6X_Ble_GetAdvParam(uint32_t *AdvIntMin, uint32_t *AdvIntMax,
                                 W6X_Ble_AdvType_e *AdvType, W6X_Ble_AdvChannel_e *ChannelMap)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint32_t AdvType_tmp;
  uint32_t ChannelMap_tmp;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get advertising parameters */
  ret = TranslateErrorStatus(W61_Ble_GetAdvParam(p_DrvObj, AdvIntMin, AdvIntMax, &AdvType_tmp, &ChannelMap_tmp));

  if (ret == W6X_STATUS_OK)
  {
    /* Return the advertising parameters */
    *AdvType = (W6X_Ble_AdvType_e)AdvType_tmp;
    *ChannelMap = (W6X_Ble_AdvChannel_e)ChannelMap_tmp;
  }
  return ret;
}

W6X_Status_t W6X_Ble_SetConnParam(uint32_t conn_handle, uint32_t conn_int_min,
                                  uint32_t conn_int_max, uint32_t latency, uint32_t timeout)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);
  /* Check the connection interval range */
  if ((conn_int_min < 0x6) || (conn_int_min > 0xC80) || (conn_int_max < 0x6) || (conn_int_max > 0xC80) ||
      (conn_int_max < conn_int_min))
  {
    LogError("Invalid connection interval values\n");
    return ret;
  }
  /* Check the latency */
  if (latency > 0x1F3) /* 499 */
  {
    LogError("Invalid Latency value\n");
    return ret;
  }
  /* Check the timeout */
  if ((timeout < 0xA) || (timeout > 0xC80)) /* 10ms to 32s */
  {
    LogError("Invalid timeout value\n");
    return ret;
  }

  /* Set Connection parameters */
  return TranslateErrorStatus(W61_Ble_SetConnParam(p_DrvObj, conn_handle, conn_int_min, conn_int_max,
                                                   latency, timeout));
}

W6X_Status_t W6X_Ble_GetConnParam(uint32_t *ConnHandle, uint32_t *ConnIntMin,
                                  uint32_t *ConnIntMax, uint32_t *ConnIntCurrent, uint32_t *Latency, uint32_t *Timeout)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get Connection parameters */
  return TranslateErrorStatus(W61_Ble_GetConnParam(p_DrvObj, ConnHandle, ConnIntMin, ConnIntMax,
                                                   ConnIntCurrent, Latency, Timeout));
}

W6X_Status_t W6X_Ble_GetConn(uint32_t *ConnHandle, uint8_t *RemoteBDAddr)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get Connection information */
  return TranslateErrorStatus(W61_Ble_GetConn(p_DrvObj, ConnHandle, RemoteBDAddr));
}

W6X_Status_t W6X_Ble_Connect(uint32_t conn_handle, uint8_t *RemoteBDAddr)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Connect to a remote device */
  return TranslateErrorStatus(W61_Ble_Connect(p_DrvObj, conn_handle, RemoteBDAddr));
}

W6X_Status_t W6X_Ble_StartScan(W6X_Ble_Scan_Result_cb_t cb)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);
  NULL_ASSERT(cb, "Invalid callback");

  /* Set the scan callback */
  p_DrvObj->BleCtx.scan_done_cb = (W61_Ble_Scan_Result_cb_t)cb;

  /* Start the scan */
  return TranslateErrorStatus(W61_Ble_Scan(p_DrvObj, 1));
}

void W6X_Ble_Print_Scan(W6X_Ble_Scan_Result_t *Scan_results)
{
  NULL_ASSERT_VOID(p_DrvObj, W6X_Ble_Uninit_str);

  /* Print the scan results */
  for (uint32_t count = 0; count < Scan_results->Count; count++)
  {
    /* Print the mandatory fields from the scan results */
    LogInfo("Scanned device: Addr : " BDADDRSTR ", RSSI : %" PRIi16 "  %s\n",
            BDADDR2STR(Scan_results->Detected_Peripheral[count].BDAddr),
            Scan_results->Detected_Peripheral[count].RSSI, Scan_results->Detected_Peripheral[count].DeviceName);
    vTaskDelay(15); /* Wait few ms to avoid logging buffer overflow */
  }
}

W6X_Status_t W6X_Ble_StopScan()
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Stop the scan */
  return TranslateErrorStatus(W61_Ble_Scan(p_DrvObj, 0));
}

W6X_Status_t W6X_Ble_SetScanParam(W6X_Ble_ScanType_e scan_type, W6X_Ble_AddrType_e own_addr_type,
                                  W6X_Ble_FilterPolicy_e filter_policy, uint32_t scan_interval, uint32_t scan_window)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set scan parameters */
  return TranslateErrorStatus(W61_Ble_SetScanParam(p_DrvObj, (uint8_t) scan_type, (uint8_t) own_addr_type,
                                                   (uint8_t) filter_policy, scan_interval, scan_window));
}

W6X_Status_t W6X_Ble_GetScanParam(W6X_Ble_ScanType_e *ScanType, W6X_Ble_AddrType_e *AddrType,
                                  W6X_Ble_FilterPolicy_e *ScanFilter, uint32_t *ScanInterval, uint32_t *ScanWindow)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint32_t ScanType_tmp = 0;
  uint32_t AddrType_tmp = 0;
  uint32_t ScanFilter_tmp = 0;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get scan parameters */
  ret = TranslateErrorStatus(W61_Ble_GetScanParam(p_DrvObj, &ScanType_tmp, &AddrType_tmp, &ScanFilter_tmp, ScanInterval,
                                                  ScanWindow));
  if (ret == W6X_STATUS_OK)
  {
    /* Return the scan parameters */
    *ScanType = (W6X_Ble_ScanType_e)ScanType_tmp;
    *AddrType = (W6X_Ble_AddrType_e)AddrType_tmp;
    *ScanFilter = (W6X_Ble_FilterPolicy_e)ScanFilter_tmp;
  }
  return ret;
}

W6X_Status_t W6X_Ble_SetDataLength(uint32_t conn_handle, uint32_t tx_bytes, uint32_t tx_trans_time)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set the data length */
  return TranslateErrorStatus(W61_Ble_SetDataLength(p_DrvObj, conn_handle, tx_bytes, tx_trans_time));
}

/* GATT Server APIs */
W6X_Status_t W6X_Ble_CreateService(uint8_t service_index, const char *service_uuid, uint8_t uuid_type)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Create a service */
  return TranslateErrorStatus(W61_Ble_CreateService(p_DrvObj, service_index, service_uuid, uuid_type));
}

W6X_Status_t W6X_Ble_DeleteService(uint8_t service_index)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Delete a service */
  return TranslateErrorStatus(W61_Ble_DeleteService(p_DrvObj, service_index));
}

W6X_Status_t W6X_Ble_CreateCharacteristic(uint8_t service_index, uint8_t char_index, const char *char_uuid,
                                          uint8_t uuid_type, uint8_t char_property, uint8_t char_permission)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Create a characteristic */
  return TranslateErrorStatus(W61_Ble_CreateCharacteristic(p_DrvObj, service_index, char_index, char_uuid,
                                                           uuid_type, char_property, char_permission));
}

W6X_Status_t W6X_Ble_GetServicesAndCharacteristics(W6X_Ble_Service_t ServicesTable[])
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get services */
  for (int32_t srv_idx = 0; srv_idx < W6X_BLE_MAX_SERVICE_NBR; srv_idx++)
  {
    /* Get the service information */
    ret = TranslateErrorStatus(W61_Ble_GetService(p_DrvObj,
                                                  (W61_Ble_Service_t *)(&ServicesTable[srv_idx]),
                                                  srv_idx));
    if ((ret != W6X_STATUS_OK) || (ServicesTable[srv_idx].service_idx == (W6X_BLE_MAX_SERVICE_NBR + 1)))
    {
      /* No more services */
      break;
    }
    /* Get characteristics for each service */
    for (int32_t char_idx = 0; char_idx < W6X_BLE_MAX_CHAR_NBR; char_idx++)
    {
      ret = TranslateErrorStatus(
              W61_Ble_GetCharacteristic(p_DrvObj,
                                        (W61_Ble_Characteristic_t *)(&ServicesTable[srv_idx].charac[char_idx]),
                                        srv_idx, char_idx));
      if ((ret != W6X_STATUS_OK) || (ServicesTable[srv_idx].charac[char_idx].char_idx == (W6X_BLE_MAX_CHAR_NBR + 1)))
      {
        /* No more characteristics for this service */
        break;
      }
    }
  }

  if (ret == W6X_STATUS_OK)
  {
    W6X_Ble_PrintServicesAndCharacteristics(ServicesTable);
  }
  return ret;
}

W6X_Status_t W6X_Ble_RegisterCharacteristics(void)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Register characteristics */
  return TranslateErrorStatus(W61_Ble_RegisterCharacteristics(p_DrvObj));
}

W6X_Status_t W6X_Ble_ServerSendNotification(uint8_t service_index, uint8_t char_index, void *pdata, uint32_t req_len,
                                            uint32_t *sent_data_len, uint32_t timeout)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Send a notification */
  return TranslateErrorStatus(W61_Ble_ServerSendNotification(p_DrvObj, service_index, char_index, (uint8_t *)pdata,
                                                             req_len, sent_data_len, timeout));
}

W6X_Status_t W6X_Ble_ServerSendIndication(uint8_t service_index, uint8_t char_index, void *pdata, uint32_t req_len,
                                          uint32_t *sent_data_len, uint32_t timeout)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Send an indication */
  return TranslateErrorStatus(W61_Ble_ServerSendIndication(p_DrvObj, service_index, char_index, (uint8_t *)pdata,
                                                           req_len, sent_data_len, timeout));
}

W6X_Status_t W6X_Ble_ServerSetReadData(uint8_t service_index, uint8_t char_index, void *pdata, uint32_t req_len,
                                       uint32_t *sent_data_len, uint32_t timeout)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set the data to read */
  return TranslateErrorStatus(W61_Ble_ServerSetReadData(p_DrvObj, service_index, char_index, (uint8_t *)pdata,
                                                        req_len, sent_data_len, timeout));
}

/* GATT Client APIs */
W6X_Status_t W6X_Ble_RemoteServiceDiscovery(uint8_t connection_handle)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Discover services of a remote device */
  return TranslateErrorStatus(W61_Ble_RemoteServiceDiscovery(p_DrvObj, connection_handle));
}

W6X_Status_t W6X_Ble_RemoteCharDiscovery(uint8_t connection_handle, uint8_t service_index)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Discover characteristics of a remote device */
  return TranslateErrorStatus(W61_Ble_RemoteCharDiscovery(p_DrvObj, connection_handle, service_index));
}

W6X_Status_t W6X_Ble_ClientWriteData(uint8_t conn_handle, uint8_t service_index, uint8_t char_index,
                                     void *pdata, uint32_t req_len, uint32_t *sent_data_len, uint32_t timeout)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Write data to a characteristic */
  return TranslateErrorStatus(W61_Ble_ClientWriteData(p_DrvObj, conn_handle, service_index, char_index,
                                                      (uint8_t *)pdata, req_len, sent_data_len, timeout));
}

W6X_Status_t W6X_Ble_ClientReadData(uint8_t conn_handle, uint8_t service_index, uint8_t char_index)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Read data from a characteristic */
  return TranslateErrorStatus(W61_Ble_ClientReadData(p_DrvObj, conn_handle, service_index, char_index));
}

W6X_Status_t W6X_Ble_ClientSubscribeChar(uint8_t conn_handle, uint8_t char_value_handle, uint8_t char_prop)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Subscribe to a characteristic */
  return TranslateErrorStatus(W61_Ble_ClientSubscribeChar(p_DrvObj, conn_handle, char_value_handle, char_prop));
}

W6X_Status_t W6X_Ble_ClientUnsubscribeChar(uint8_t conn_handle, uint8_t char_value_handle)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Unsubscribe from a characteristic */
  return TranslateErrorStatus(W61_Ble_ClientUnsubscribeChar(p_DrvObj, conn_handle, char_value_handle));
}

/* Security APIs */
W6X_Status_t W6X_Ble_SetSecurityParam(W6X_Ble_SecurityParameter_e security_parameter)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set security parameters */
  return TranslateErrorStatus(W61_Ble_SetSecurityParam(p_DrvObj, (uint8_t) security_parameter));
}

W6X_Status_t W6X_Ble_GetSecurityParam(W6X_Ble_SecurityParameter_e *SecurityParameter)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint32_t SecurityParam_tmp;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get security parameters */
  ret = TranslateErrorStatus(W61_Ble_GetSecurityParam(p_DrvObj, &SecurityParam_tmp));
  if (ret == W6X_STATUS_OK)
  {
    *SecurityParameter = (W6X_Ble_SecurityParameter_e) SecurityParam_tmp;
  }
  return ret;
}

W6X_Status_t W6X_Ble_SecurityStart(uint8_t conn_handle, uint8_t security_level)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Start BLE security */
  return TranslateErrorStatus(W61_Ble_SecurityStart(p_DrvObj, conn_handle, security_level));
}

W6X_Status_t W6X_Ble_SecurityPassKeyConfirm(uint8_t conn_handle)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Confirm the passkey */
  return TranslateErrorStatus(W61_Ble_SecurityPassKeyConfirm(p_DrvObj, conn_handle));
}

W6X_Status_t W6X_Ble_SecurityPairingConfirm(uint8_t conn_handle)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Confirm the pairing */
  return TranslateErrorStatus(W61_Ble_SecurityPairingConfirm(p_DrvObj, conn_handle));
}

W6X_Status_t W6X_Ble_SecuritySetPassKey(uint8_t conn_handle, uint32_t passkey)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Set the passkey */
  return TranslateErrorStatus(W61_Ble_SecuritySetPassKey(p_DrvObj, conn_handle, passkey));
}

W6X_Status_t W6X_Ble_SecurityPairingCancel(uint8_t conn_handle)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Cancel the pairing */
  return TranslateErrorStatus(W61_Ble_SecurityPairingCancel(p_DrvObj, conn_handle));
}

W6X_Status_t W6X_Ble_SecurityUnpair(uint8_t *RemoteBDAddr, W6X_Ble_AddrType_e remote_addr_type)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Unpair a device */
  return TranslateErrorStatus(W61_Ble_SecurityUnpair(p_DrvObj, RemoteBDAddr, (uint32_t) remote_addr_type));
}

W6X_Status_t W6X_Ble_SecurityGetBondedDeviceList(W6X_Ble_Bonded_Devices_Result_t *RemoteBondedDevices)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Ble_Uninit_str);

  /* Get the list of bonded devices */
  return TranslateErrorStatus(W61_Ble_SecurityGetBondedDeviceList(
                                p_DrvObj, (W61_Ble_Bonded_Devices_Result_t *)RemoteBondedDevices));
}
/** @} */

/* Private Functions Definition ----------------------------------------------*/
/** @addtogroup ST67W6X_Private_BLE_Functions
  * @{
  */
static void W6X_Ble_cb(W61_event_id_t event_id, void *event_args)
{
  W61_Ble_CbParamData_t *p_param_ble_data = (W61_Ble_CbParamData_t *) event_args;
  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();

  /* Check if the application callback is registered */
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_ble_cb == NULL))
  {
    LogError("Please register the APP callback before initializing the module\n");
    return;
  }

  switch (event_id) /* Check the event ID and call the appropriate callback */
  {
    case W61_BLE_EVT_CONNECTED_ID:
      LogDebug("BLE connected\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_CONNECTED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_CONNECTION_PARAM_ID:
      LogDebug("BLE connection param update\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_CONNECTION_PARAM_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_DISCONNECTED_ID:
      LogDebug("BLE disconnected\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_DISCONNECTED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_INDICATION_STATUS_ENABLED_ID:
      LogDebug("BLE indication enabled\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_INDICATION_STATUS_ENABLED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_INDICATION_STATUS_DISABLED_ID:
      LogDebug("BLE indication disabled\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_INDICATION_STATUS_DISABLED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_INDICATION_ACK_ID:
      LogDebug("BLE indication complete\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_INDICATION_ACK_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_INDICATION_NACK_ID:
      LogDebug("BLE indication not complete\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_INDICATION_NACK_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_NOTIFICATION_STATUS_ENABLED_ID:
      LogDebug("BLE notification enabled\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_NOTIFICATION_STATUS_ENABLED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_NOTIFICATION_STATUS_DISABLED_ID:
      LogDebug("BLE notification disabled\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_NOTIFICATION_STATUS_DISABLED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_NOTIFICATION_DATA_ID:
      LogDebug("BLE notification data\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_NOTIFICATION_DATA_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_WRITE_ID:
      LogDebug("BLE write\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_WRITE_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_READ_ID:
      LogDebug("BLE read\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_READ_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_SCAN_DONE_ID:
      LogDebug("BLE scan done\n"); /* Call the scan done callback */
      p_DrvObj->BleCtx.scan_done_cb(&p_DrvObj->BleCtx.ScanResults);
      break;

    case W61_BLE_EVT_SERVICE_FOUND_ID:
      LogDebug("BLE service discovered\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_SERVICE_FOUND_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_CHAR_FOUND_ID:
      LogDebug("BLE characteristic discovered\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_CHAR_FOUND_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_MTU_SIZE_ID:
      LogDebug("BLE MTU exchange\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_MTU_SIZE_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_PAIRING_COMPLETED_ID:
      LogDebug("BLE pairing completed\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_PAIRING_COMPLETED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_PAIRING_FAILED_ID:
      LogDebug("BLE pairing failed\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_PAIRING_FAILED_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_PAIRING_CONFIRM_ID:
      LogDebug("BLE pairing confirm\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_PAIRING_CONFIRM_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_PASSKEY_CONFIRM_ID:
      LogDebug("BLE passkey confirm\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_PASSKEY_CONFIRM_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_PASSKEY_ENTRY_ID:
      LogDebug("BLE passkey entry\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_PASSKEY_ENTRY_ID, (void *) p_param_ble_data);
      break;

    case W61_BLE_EVT_PASSKEY_DISPLAY_ID:
      LogDebug("BLE passkey display\n");
      p_cb_handler->APP_ble_cb(W6X_BLE_EVT_PASSKEY_DISPLAY_ID, (void *) p_param_ble_data);
      break;

    default:
      break;
  }
}

static void W6X_Ble_PrintServicesAndCharacteristics(W6X_Ble_Service_t ServicesTable[])
{
  for (int32_t i = 0; i < W6X_BLE_MAX_SERVICE_NBR; i++) /* Loop through the services */
  {
    if (i == ServicesTable[i].service_idx) /* Check if the service is valid */
    {
      /* Print the service information */
      LogInfo("Service: %" PRIu16 ", UUID: " UUID128STR "\n", ServicesTable[i].service_idx,
              UUID128TOSTR(ServicesTable[i].service_uuid));

      for (int32_t k = 0; k < W6X_BLE_MAX_CHAR_NBR; k++) /* Loop through the characteristics */
      {
        if (k == ServicesTable[i].charac[k].char_idx) /* Check if the characteristic is valid */
        {
          /* Print the characteristics of the service */
          LogInfo("Characteristic: %" PRIu16 ", UUID: " UUID128STR ", Property: %" PRIu16 "\n",
                  ServicesTable[i].charac[k].char_idx,
                  UUID128TOSTR(ServicesTable[i].charac[k].char_uuid),
                  ServicesTable[i].charac[k].char_property);
        }
      }
      LogInfo("\n");
    }
  }
}

/** @} */
