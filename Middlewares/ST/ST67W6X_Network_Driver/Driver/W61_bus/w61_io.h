/**
  ******************************************************************************
  * @file    w61_io.h
  * @author  GPM Application Team
  * @brief   This file provides the common defines and functions prototypes for
  *          the ST67W611M IO operations.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef W61_IO_H
#define W61_IO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "spi_iface.h"  /* Needed for SPI_XFER_MTU_BYTES */

/* Exported types ------------------------------------------------------------*/
typedef void (*BusIo_SPI_rxd_notify_func_t)(void *arg);

/* Exported constants --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
  * @brief  ST67W611M IO Initialization.
  *         This function initialize the SPI interface to deal with the w61,
  *         then starts asynchronous listening on the RX port.
  * @param  type: Traffic type to bind (must be < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  * @retval forward spisync_init ret
  */
int32_t BusIo_SPI_Init(void);

/**
  * @brief  ST67W611M IO Deinitialization.
  *         This function De-initialize the SPI interface of the ST67W611M module.
  *         When called the w61 commands can't be executed anymore.
  * @retval forward spisync_deinit ret
  */
int32_t BusIo_SPI_DeInit(void);

/**
  * @brief  ST67W611M IO bind interface.
  *         This function bind the SPI interface to deal with the w61,
  *         then starts asynchronous listening on the RX port.
  * @param  type: Traffic type to bind (must be < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  * @retval forward spisync_init ret
  */
int32_t BusIo_SPI_Bind(uint8_t type, int32_t rxq_size, BusIo_SPI_rxd_notify_func_t cb);

/**
  * @brief  Delay
  * @param  Delay in ticks (often 1 ticks is config to 1 ms)
  */
void BusIo_SPI_Delay(uint32_t Delay);

/**
  * @brief  Send commands and raw data to the ST67W611M module over the SPI interface.
  *         This function allows sending bytes to the ST67W611M module, the
  *          data can be either an AT command or raw data to send over
  *          a pre-established WiFi connection.
  * @param  type: Traffic type to use (must be < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  * @param  pBuf: data to send
  * @param  length: the data length
  * @param  Timeout: max time to wait before returning
  * @retval forward spisync_deinit ret
  */
int32_t BusIo_SPI_SendData(uint8_t type, uint8_t *pBuf, uint16_t length, uint32_t Timeout);

/**
  * @brief  Receive data from the ST67W611M module over the SPI interface
  * @param  type: Traffic type to use (must be < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  * @param  pBuf: receive buffer
  * @param  length: max length to be retrieved
  * @param  Timeout: max time to wait before returning
  * @retval size of the buffer actually retrieved (could be smaller the the length param)
  */
int32_t BusIo_SPI_ReceiveData(uint8_t type, uint8_t *pBuf, uint16_t length, uint32_t Timeout);

/**
  * @brief  Receive pointer of data from the ST67W611M module over the SPI interface
  * @param  type: Traffic type to use (must be < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  * @param  buffer: receive spi_buffer
  * @param  data: receive buffer
  * @param  Timeout: max time to wait before returning
  * @retval size of the buffer actually retrieved (could be smaller the the length param)
  */
int32_t BusIo_SPI_ReceivePtr(uint8_t type, void **buffer, uint8_t **data, uint32_t Timeout);

/**
  * @brief  Free spi_buffer containing the data
  * @param  buffer: Pointer to store the spi_buffer
  * @return Operation status
  */
int32_t BusIo_SPI_Free(void *buffer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W61_IO_H */
