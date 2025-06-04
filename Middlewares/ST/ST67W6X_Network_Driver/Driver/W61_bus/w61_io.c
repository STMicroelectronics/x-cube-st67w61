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
#include "logging.h"
#include "w61_io.h"
#include "FreeRTOS.h"
#include "task.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/
int32_t BusIo_SPI_Init(void)
{
  if (spi_transaction_init())
  {
    return -1;
  }

  return spi_bind(SPI_MSG_CTRL_TRAFFIC_AT_CMD, 16);
}

int32_t BusIo_SPI_DeInit(void)
{
  return spi_transaction_deinit();
}

void BusIo_SPI_Delay(uint32_t Delay)
{
  vTaskDelay(pdMS_TO_TICKS(Delay));
}

int32_t BusIo_SPI_Send(uint8_t *pBuf, uint16_t length, uint32_t Timeout)
{
  struct spi_msg_control ctrl;
  uint8_t traffic = SPI_MSG_CTRL_TRAFFIC_AT_CMD;
  struct spi_msg m;

  SPI_MSG_CONTROL_INIT(ctrl, SPI_MSG_CTRL_TRAFFIC_TYPE,
                       SPI_MSG_CTRL_TRAFFIC_TYPE_LEN, &traffic);
  SPI_MSG_INIT(m, SPI_MSG_OP_DATA, &ctrl, 0);
  m.data = (void *)pBuf;
  m.data_len = length;
  return spi_write(&m, Timeout);
}

int32_t BusIo_SPI_Receive(uint8_t *pBuf, uint16_t length, uint32_t Timeout)
{
  uint8_t traffic_type = SPI_MSG_CTRL_TRAFFIC_AT_CMD;
  struct spi_msg_control ctrl;
  struct spi_msg m;

  SPI_MSG_CONTROL_INIT(ctrl, SPI_MSG_CTRL_TRAFFIC_TYPE,
                       SPI_MSG_CTRL_TRAFFIC_TYPE_LEN, &traffic_type);
  SPI_MSG_INIT(m, SPI_MSG_OP_DATA, &ctrl, 0);
  m.data = pBuf;
  m.data_len = length;
  return spi_read(&m, Timeout);
}
