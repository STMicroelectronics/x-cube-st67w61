/**
  ******************************************************************************
  * @file    spi_port.h
  * @brief   SPI bus interface porting layer
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

/**
  * This file is based on QCC74xSDK provided by Qualcomm.
  * See https://git.codelinaro.org/clo/qcc7xx/QCCSDK-QCC74x for more information.
  *
  * Reference source: examples/stm32_spi_host/QCC743_SPI_HOST/Core/Src/spi_iface.c
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SPI_PORT_H
#define SPI_PORT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdint.h>
#include "logging.h"

/* Exported types ------------------------------------------------------------*/
typedef void (*spi_transaction_complete_t)(void);

/* Exported constants --------------------------------------------------------*/
/* Transfer with size greater than this value should use DMA. */
#define SPI_DMA_XFER_SIZE_THRESHOLD     8

#define SPI_WAIT_TXN_TIMEOUT_MS         2000
#define SPI_WAIT_MSG_XFER_TIMEOUT_MS    500
#define SPI_WAIT_HDR_ACK_TIMEOUT_MS     100
#define SPI_WAIT_POLL_XFER_TIMEOUT_MS   100

#ifndef SPI_PORT_LOG_ENABLE
#define SPI_PORT_LOG_ENABLE             0
#endif /* SPI_PORT_LOG_ENABLE */

#ifndef SPI_PORT_DEBUG_ENABLE
#define SPI_PORT_DEBUG_ENABLE           0
#endif /* SPI_PORT_DEBUG_ENABLE */

#ifndef SPI_PORT_ERROR_ENABLE
#define SPI_PORT_ERROR_ENABLE           0
#endif /* SPI_PORT_ERROR_ENABLE */

#ifndef SPI_PORT_TRACE_ENABLE
#define SPI_PORT_TRACE_ENABLE           0
#endif /* SPI_PORT_TRACE_ENABLE */

enum
{
  SPI_EVT_TXN_PENDING = 0x1,
  SPI_EVT_TXN_RDY = 0x2,
  SPI_EVT_HDR_ACKED = 0x4,
  SPI_EVT_HW_XFER_DONE = 0x8,
};

#ifndef SHORT_FILE
#define SHORT_FILE \
  (strchr(__FILE__, '\\') \
   ? ((strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)) \
   : ((strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)))
#endif /* SHORT_FILE */

/* Exported macro ------------------------------------------------------------*/
#ifdef __ICCARM__
#define memcpy spi_port_memcpy
#endif /* __ICCARM__ */

#if (SPI_PORT_LOG_ENABLE == 1)
#define spi_log(M, ...) do { LogDebug("[%" PRIu32 "][%s +%" PRIu32 "] " M "",\
                             (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                             SHORT_FILE, __LINE__,\
                             ##__VA_ARGS__);\
                           } while(0)
#else
#define spi_log(...)
#endif /* SPI_PORT_LOG_ENABLE */

#if (SPI_PORT_DEBUG_ENABLE == 1)
#define spi_dbg(M, ...) do { LogDebug("[%" PRIu32 "][%s +%" PRIu32 "] " M "",\
                             (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                             SHORT_FILE, __LINE__,\
                             ##__VA_ARGS__);\
                           } while(0)
#else
#define spi_dbg(...)
#endif /* SPI_PORT_DEBUG_ENABLE */

#if (SPI_PORT_ERROR_ENABLE == 1)
#define spi_err(M, ...) do { LogError("[%" PRIu32 "][%s +%" PRIu32 "] " M "",\
                             (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                             SHORT_FILE, __LINE__,\
                             ##__VA_ARGS__);\
                           } while(0)
#else
#define spi_err(...)
#endif /* SPI_PORT_LOG_ENABLE */

#if (SPI_PORT_TRACE_ENABLE == 1)
#define spi_trace(e, m, ...) do {                          \
                                  if (e)                   \
                                  spi_port_itm(e);         \
                                  spi_log(m, __VA_ARGS__); \
                                } while (0)
#else
#define spi_trace(...)
#endif /* SPI_PORT_TRACE_ENABLE */

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Initialize SPI port
  * @param  transaction_complete_cb: Callback function to be called when a DMA transaction is completed
  * @retval 0 if successful, -1 otherwise
  */
int32_t spi_port_init(spi_transaction_complete_t transaction_complete_cb);

/**
  * @brief  De-Initialize SPI port
  * @retval 0 if successful, -1 otherwise
  */
int32_t spi_port_deinit(void);

/**
  * @brief  Execute SPI transaction in blocking mode (polling) with timeout. Can be TxRX or Rx only
  * @retval 0 if successful, -1 otherwise
  */
int32_t spi_port_transfer(void *tx_buf, void *rx_buf, uint16_t len, uint32_t timeout);

/**
  * @brief  Execute SPI transaction in non-blocking mode (interrupt). Can be TxRX or Rx only
  * @retval 0 if successful, -1 otherwise
  */
int32_t spi_port_transfer_dma(void *tx_buf, void *rx_buf, uint16_t len);

/**
  * @brief  Check if NCP requires to send a new data packet
  * @retval 1 if ready, 0 otherwise
  */
int32_t spi_port_is_ready(void);

/**
  * @brief  Enable or disable the SPI Chip Select (CS) line
  * @param  state: 1 to set the CS line high, 0 to set it low
  * @retval 0 if successful, -1 otherwise
  */
int32_t spi_port_set_cs(int32_t state);

/**
  * @brief  Set an ITM event
  * @retval 1 if high, 0 if low
  */
uint32_t spi_port_itm(uint32_t ch);

#ifdef __ICCARM__
/**
  * @brief  Copy memory from source to destination
  * @retval Destination address
  */
void *spi_port_memcpy(void *dest, const void *src, unsigned int len);
#endif /* __ICCARM__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SPI_PORT_H */
