/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lwip_netif.h
  * @author  GPM Application Team
  * @brief   This file provides code for the configuration of the ST67W6X Network interface over LwIP
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LWIP_NETIF_H
#define __LWIP_NETIF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "w6x_api.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/** Netif task priority */
#define NETIF_TASK_PRIORITY     50

/** Netif task stack size */
#define NETIF_TASK_STACK        2048

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Initializes the network interface
  * @param  net_if_cb: Pointer to the network interface control block
  * @return Operation status
  */
int32_t net_if_init(W6X_Net_if_cb_t *net_if_cb);

/**
  * @brief  Sends a packet from the network interface over SPI
  * @param  net_if: Pointer to the network interface structure
  * @param  p_buf: Pointer to the packet buffer to be sent
  * @return Returns ERR_OK on success or an error code on failure
  */
err_t net_if_output(struct netif *net_if, struct pbuf *p_buf);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LWIP_NETIF_H */
