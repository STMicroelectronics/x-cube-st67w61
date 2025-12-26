/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lwip_netif.c
  * @author  GPM Application Team
  * @brief   This file provides initialization code for ST67W6X Network interface over LwIP
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "lwip.h"
#include "lwip_netif.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Structure to handle pbuf with driver buffer pointer
 */
typedef struct
{
  struct pbuf_custom pb;        /*!< pbuf_custom structure */
  void *buffer;                 /*!< Pointer to the driver buffer */
} netif_pbuf_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/** Bit indicating that STA interface has data ready to be processed */
#define NET_IF_STA_RX_RDY       (1 << 0)

/** Bit indicating that AP interface has data ready to be processed */
#define NET_IF_AP_RX_RDY        (1 << 1)

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
#ifndef CONTAINER_OF
/** Return the pointer to the structure containing the specified member */
#define CONTAINER_OF(ptr, type, field)                  \
  ({                                                    \
    ((type *)(((char *)(ptr)) - offsetof(type, field)));\
  })
#endif /* CONTAINER_OF */

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static TaskHandle_t netif_task_handle = NULL; /*!< Netif task handle */

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Custom malloc function for pbuf with driver buffer ptr
  * @param  buffer Pointer of the driver buffer
  * @return Pointer to the allocated pbuf_custom structure or NULL if allocation fails
  */
static struct pbuf_custom *netif_pbuf_alloc(void *buffer);

/**
  * @brief  Custom free function for pbuf with driver buffer ptr
  * @param  p Pointer to the pbuf structure to be freed
  */
static void netif_pbuf_free(struct pbuf *p);

/**
  * @brief  Callback function to handle RX notification from STA interface
  * @param  arg: Pointer to the argument (not used)
  */
static void netif_sta_notify_callback(void *arg);

/**
  * @brief  Callback function to handle RX notification from AP interface
  * @param  arg: Pointer to the argument (not used)
  */
static void netif_ap_notify_callback(void *arg);

/**
  * @brief  Process received data from the specified link interface
  * @param  link_id: Link identifier (0 for STA, 1 for AP)
  * @return Number of bytes processed or negative value on error
  */
static int32_t netif_rx_process(uint32_t link_id);

/** @brief  Netif task function
  * @param  arg: Pointer to the task argument (not used)
  * @note   This function runs in a FreeRTOS task and processes incoming network data
  */
static void netif_task(void *arg);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
int32_t net_if_init(W6X_Net_if_cb_t *net_if_cb)
{
  if ((netif_task_handle != NULL) || (net_if_cb == NULL))
  {
    return -1;
  }

  net_if_cb->rxd_sta_notify_fn = netif_sta_notify_callback;
  net_if_cb->rxd_ap_notify_fn = netif_ap_notify_callback;

  if (W6X_Netif_Init(net_if_cb))
  {
    LogError("Failed to initialize ST67W6X Net if component\n");
    return -1;
  }

  if (pdPASS != xTaskCreate(netif_task, "netif", NETIF_TASK_STACK >> 2,
                            NULL, NETIF_TASK_PRIORITY, &netif_task_handle))
  {
    LogError("xTaskCreate failed to create netif task\n");
    return -1;
  }
  vTaskDelay(100);
  return 0;
}

err_t net_if_output(struct netif *net_if, struct pbuf *p_buf)
{
  err_t status = ERR_OK;
  int32_t ret = 0;
  struct pbuf *q;
  uint32_t link_id = 0;

  for (link_id = NETIF_STA; link_id < NETIF_MAX; link_id++)
  {
    if (net_if == netif_get_interface(link_id))
    {
      break;
    }
  }
  if (link_id >= NETIF_MAX)
  {
    return ERR_OK;
  }

  link_id = (link_id == NETIF_STA) ? W6X_NET_IF_STA : W6X_NET_IF_AP;

  for (q = p_buf; q != NULL; q = q->next)
  {
    ret = W6X_Netif_output(link_id, q->payload, q->len);
    if (ret < 0)
    {
      LogError("%s: spi_write ERROR : %d\n", __func__, ret);
      switch (ret)
      {
        case -1:
          status = ERR_BUF;
          break;
        case -2:
          status = ERR_VAL;
          break;
        case -3:
          status = ERR_MEM;
          break;
        case -4:
          status = ERR_BUF;
          break;
        case -5:
          status = ERR_INPROGRESS;
          break;
        default:
          status = ERR_VAL;
      }
    }
  }

  return status;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static struct pbuf_custom *netif_pbuf_alloc(void *buffer)
{
  /* Allocate new pbuf custom element */
  netif_pbuf_t *netif_pbuf = pvPortMalloc(sizeof(netif_pbuf_t));
  if (netif_pbuf == NULL)
  {
    return NULL;
  }
  memset(netif_pbuf, 0, sizeof(netif_pbuf_t));
  /* Register the free callback to clean buffer when process of payload is completed */
  netif_pbuf->pb.custom_free_function = netif_pbuf_free;

  /* Store the buffer pointer to free when process of payload is completed */
  netif_pbuf->buffer = buffer;

  return &netif_pbuf->pb;
}

static void netif_pbuf_free(struct pbuf *pb)
{
  if (pb)
  {
    /* Retrieve the original pointer based on child field pbuf */
    struct pbuf_custom *pbufc = CONTAINER_OF(pb, struct pbuf_custom, pbuf);
    netif_pbuf_t *netif_pbuf = CONTAINER_OF(pbufc, netif_pbuf_t, pb);

    /* Free the driver buffer and netif_pbuf */
    W6X_Netif_free(netif_pbuf->buffer);
    vPortFree(netif_pbuf);
  }
}

static void netif_sta_notify_callback(void *arg)
{
  xTaskNotify(netif_task_handle, NET_IF_STA_RX_RDY, eSetBits);
}

static void netif_ap_notify_callback(void *arg)
{
  xTaskNotify(netif_task_handle, NET_IF_AP_RX_RDY, eSetBits);
}

static int32_t netif_rx_process(uint32_t link_id)
{
  int32_t ret = 0;
  struct pbuf *pb;
  void *buffer = NULL;
  uint8_t *payload = NULL;
  struct netif *netif = NULL;

  netif = netif_get_interface(link_id);
  if (netif == NULL)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
    return -1;
  }

  /* Wait to receive the next data from W6X */
  ret = W6X_Netif_input(link_id, &buffer, &payload);
  if (ret <= 0)
  {
    if (buffer)
    {
      W6X_Netif_free(buffer); /* Free the buffer even if in error */
    }
    return ret;
  }

  /* Allocate new pbuf custom element */
  struct pbuf_custom *pbuf_custom = netif_pbuf_alloc(buffer);
  if (pbuf_custom == NULL)
  {
    LogError("Memory allocation failure\n");
    W6X_Netif_free(buffer);
    vTaskDelay(pdMS_TO_TICKS(100));
    return -1;
  }

  /* Setup the pbuf_custom structure and return the subfield pbuf */
  pb = pbuf_alloced_custom(PBUF_RAW, ret, PBUF_REF, pbuf_custom, payload, ret);
  if (pb == NULL)
  {
    LogError("Memory allocation failure\n");
    W6X_Netif_free(buffer);
    vTaskDelay(pdMS_TO_TICKS(100));
    return -1;
  }

  /* Call the upper layer callback */
  if (netif->input(pb, netif))
  {
    LogError("Input ERROR\n");
    W6X_Netif_free(buffer);
    netif_pbuf_free(pb);
    return -1;
  }

  return ret;
}

static void netif_task(void *arg)
{
  int32_t ret = 0;
  uint32_t netif_event = 0;

  while (1)
  {
    xTaskNotifyWait(0, NET_IF_STA_RX_RDY | NET_IF_AP_RX_RDY, &netif_event, portMAX_DELAY);
    do
    {
      if (netif_event & NET_IF_STA_RX_RDY)
      {
        ret = netif_rx_process(NETIF_STA);
        if (ret <= 0)
        {
          netif_event &= ~NET_IF_STA_RX_RDY;
        }
      }

      if (netif_event & NET_IF_AP_RX_RDY)
      {
        ret = netif_rx_process(NETIF_AP);
        if (ret <= 0)
        {
          netif_event &= ~NET_IF_AP_RX_RDY;
        }
      }
    } while (netif_event);
  }
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
