/**
  ******************************************************************************
  * @file    w61_io.c
  * @author  GPM Application Team
  * @brief   This file provides the IO operations to deal with the STM32W61 module.
  *          It mainly initialize and de-initialize the SPI interface.
  *          Send and receive data over it.
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
#include "stdint.h"
#include "logging.h"
#include "w61_io.h"
#include "FreeRTOS.h"
#include "task.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Initialization flag */
static volatile int32_t initialized = 0;

/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
int32_t BusIo_SPI_Init(void)
{
  int32_t ret;
  if (initialized)
  {
    return -1;
  }

  ret = spi_transaction_init();
  if (ret == 0)
  {
    initialized = 1;
  }
  return ret;
}

int32_t BusIo_SPI_DeInit(void)
{
  initialized = 0;
  return spi_transaction_deinit();
}

int32_t BusIo_SPI_Bind(uint8_t type, int32_t rxq_size, BusIo_SPI_rxd_notify_func_t cb)
{
  int32_t ret = spi_bind(type, rxq_size);
  if (ret != 0)
  {
    return ret;
  }

  if ((cb != NULL) && ((type == SPI_MSG_CTRL_TRAFFIC_NETWORK_STA) || (type == SPI_MSG_CTRL_TRAFFIC_NETWORK_AP)))
  {
    ret = spi_rxd_callback_register((spi_msg_ctrl_t)type, cb, NULL);
  }
  return ret;
}

void BusIo_SPI_Delay(uint32_t Delay)
{
  vTaskDelay(pdMS_TO_TICKS(Delay));
}

int32_t BusIo_SPI_SendData(uint8_t type, uint8_t *pBuf, uint16_t length, uint32_t Timeout)
{
  struct spi_msg_control ctrl;
  struct spi_msg m;

  SPI_MSG_CONTROL_INIT(ctrl, SPI_MSG_CTRL_TRAFFIC_TYPE, SPI_MSG_CTRL_TRAFFIC_TYPE_LEN, &type);
  SPI_MSG_INIT(m, SPI_MSG_OP_DATA, &ctrl, 0);
  m.data = (void *)pBuf;
  m.data_len = length;
  return spi_write(&m, Timeout);
}

int32_t BusIo_SPI_ReceiveData(uint8_t type, uint8_t *pBuf, uint16_t length, uint32_t Timeout)
{
  struct spi_msg_control ctrl;
  struct spi_msg m;

  SPI_MSG_CONTROL_INIT(ctrl, SPI_MSG_CTRL_TRAFFIC_TYPE, SPI_MSG_CTRL_TRAFFIC_TYPE_LEN, &type);
  SPI_MSG_INIT(m, SPI_MSG_OP_DATA, &ctrl, 0);
  m.data = pBuf;
  m.data_len = length;
  return spi_read(&m, Timeout);
}

int32_t BusIo_SPI_ReceivePtr(uint8_t type, void **buffer, uint8_t **data, uint32_t Timeout)
{
  struct spi_msg_control ctrl;
  struct spi_msg m;
  int32_t ret = 0;
  struct spi_buffer *spi_buf = NULL;
  *buffer = NULL;
  *data = NULL;

  SPI_MSG_CONTROL_INIT(ctrl, SPI_MSG_CTRL_TRAFFIC_TYPE, SPI_MSG_CTRL_TRAFFIC_TYPE_LEN, &type);
  SPI_MSG_INIT(m, SPI_MSG_OP_BUFFER_PTR, &ctrl, 0);
  m.buffer_ptr = &spi_buf;

  ret = spi_read(&m, Timeout);

  if ((ret >= 0) && (spi_buf != NULL))
  {
    *buffer = (void *)spi_buf;
    *data = spi_buf->data;
  }

  return ret;
}

int32_t BusIo_SPI_Free(void *buffer)
{
  if (buffer)
  {
    vPortFree(buffer);
  }

  return 0;
}
