/**
  ******************************************************************************
  * @file    w6x_netif.c
  * @author  ST67 Application Team
  * @brief   This file provides code for W6x Net interface API
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
#include "w6x_types.h"     /* W6X_ARCH_** */
#if (ST67_ARCH == W6X_ARCH_T02)
#include <string.h>
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */
#include "common_parser.h" /* Common Parser functions */
#include "event_groups.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_Netif_Variables ST67W6X Network Interface Variables
  * @ingroup  ST67W6X_Private_Netif
  * @{
  */

static W61_Object_t *W6X_Netif_drv_obj = NULL;         /*!< Global W61 context pointer */

/** @} */

/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_Netif_Public_Functions
  * @{
  */

W6X_Status_t W6X_Netif_Init(W6X_Net_if_cb_t *net_if_cb)
{
  /* Get the global W61 context pointer */
  W6X_Netif_drv_obj = W61_ObjGet();
  NULL_ASSERT(W6X_Netif_drv_obj, W6X_Obj_Null_str);

  if (W6X_Netif_drv_obj->NetCtx.Supported != 0U)
  {
    NET_LOG_ERROR("Netif module not supported\n");
    return W6X_STATUS_ERROR;
  }

  if (BusIo_SPI_Bind(SPI_MSG_CTRL_TRAFFIC_NETWORK_STA, (int32_t)W6X_NETIF_STA_RXQ_DEPTH,
                     net_if_cb->rxd_sta_notify_fn) != 0)
  {
    SYS_LOG_ERROR("Bind Network Traffic Station failed\n");
    return W6X_STATUS_ERROR;
  }

  if (BusIo_SPI_Bind(SPI_MSG_CTRL_TRAFFIC_NETWORK_AP, (int32_t)W6X_NETIF_AP_RXQ_DEPTH,
                     net_if_cb->rxd_ap_notify_fn) != 0)
  {
    SYS_LOG_ERROR("Bind Network Traffic AP failed\n");
    return W6X_STATUS_ERROR;
  }

  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_sta_up_fn = net_if_cb->link_sta_up_fn;
  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_sta_down_fn = net_if_cb->link_sta_down_fn;
  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_ap_up_fn = net_if_cb->link_ap_up_fn;
  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_ap_down_fn = net_if_cb->link_ap_down_fn;

  return W6X_STATUS_OK;
}

void W6X_Netif_DeInit(void)
{
  if (W6X_Netif_drv_obj == NULL)
  {
    return; /* Nothing to do */
  }
  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_sta_up_fn = NULL;
  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_sta_down_fn = NULL;
  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_ap_up_fn = NULL;
  W6X_Netif_drv_obj->Callbacks.Netif_cb.link_ap_down_fn = NULL;
}

int32_t W6X_Netif_output(uint32_t link_id, uint8_t *buf, uint32_t len)
{
  uint8_t type;

  if (link_id == W6X_NET_IF_STA)
  {
    type = SPI_MSG_CTRL_TRAFFIC_NETWORK_STA;
  }
  else if (link_id == W6X_NET_IF_AP)
  {
    type = SPI_MSG_CTRL_TRAFFIC_NETWORK_AP;
  }
  else
  {
    return -1;
  }

  return BusIo_SPI_SendData(type, buf, len, pdMS_TO_TICKS(10000));
}

int32_t W6X_Netif_input(uint32_t link_id, void **buffer, uint8_t **data)
{
  uint8_t type;

  if (link_id == W6X_NET_IF_STA)
  {
    type = SPI_MSG_CTRL_TRAFFIC_NETWORK_STA;
  }
  else if (link_id == W6X_NET_IF_AP)
  {
    type = SPI_MSG_CTRL_TRAFFIC_NETWORK_AP;
  }
  else
  {
    return -1;
  }

  return BusIo_SPI_ReceivePtr(type, buffer, data, 0);
}

int32_t W6X_Netif_free(void *buffer)
{
  return BusIo_SPI_Free(buffer);
}

/** @} */

#endif /* ST67_ARCH */
