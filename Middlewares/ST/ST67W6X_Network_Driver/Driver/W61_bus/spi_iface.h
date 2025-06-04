/**
  ******************************************************************************
  * @file    spi_iface.h
  * @brief   SPI bus interface definition
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
  * Reference source: examples/stm32_spi_host/QCC743_SPI_HOST/Core/Inc/spi.h
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SPI_IFACE_H
#define SPI_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
struct spi_msg_control
{
  /* Ref SPI_MSG_CTRL_xxx */
  uint8_t type;
  /* Length of the following control data. */
  uint8_t len;
  void *val;
};

struct spi_msg
{
  union
  {
    /* For spi_write and spi_read. */
    struct
    {
      void *data;
      uint32_t data_len;
    };
    /* For spi_write_buffer. */
    struct spi_buffer *buffer;
    /* For spi_read_buffer */
    struct spi_buffer **buffer_ptr;
  };
  struct spi_msg_control *ctrl;
  uint32_t flags;
  unsigned char op_type;
};

struct spi_stat
{
  uint64_t tx_pkts;
  uint64_t tx_bytes;
  uint64_t rx_pkts;
  uint64_t rx_bytes;
  uint64_t rx_drop;
  uint64_t io_err;
  uint64_t hdr_err;
  uint64_t wait_txn_timeouts;
  uint64_t wait_msg_xfer_timeouts;
  uint64_t wait_hdr_ack_timeouts;
  uint64_t mem_err;
  uint64_t rx_stall;
};

struct spi_buffer
{
  void *data;
  /* Length of the data. */
  uint32_t len;
  /* Capacity of the buffer, >= len. */
  uint32_t cap;
  uint32_t flags;
  /* Control block for private data. */
  unsigned char cb[16];
};

/* Exported constants --------------------------------------------------------*/
#define SPI_MSG_F_TRUNCATED 0x1

#define SPI_MSG_CTRL_TRAFFIC_TYPE   0x1
#define SPI_MSG_CTRL_TRAFFIC_TYPE_LEN 1
#define SPI_MSG_CTRL_TRAFFIC_AT_CMD   0
#define SPI_MSG_CTRL_TRAFFIC_NETWORK  1
#define SPI_MSG_CTRL_TRAFFIC_TYPE_MAX   2

#define SPI_MSG_OP_DATA        0
#define SPI_MSG_OP_BUFFER      1
#define SPI_MSG_OP_BUFFER_PTR  2

/** Maximum SPI buffer size */
#define SPI_XFER_MTU_BYTES        (6 * 1024)

/* Exported macro ------------------------------------------------------------*/
#define SPI_MSG_CONTROL_INIT(c, t, l, v) do {                                    \
                                              struct spi_msg_control *_c = &(c); \
                                              _c->type = t;                      \
                                              _c->len = l;                       \
                                              _c->val = v;                       \
                                            } while (0)

#define SPI_MSG_INIT(m, t, c, f) do {                            \
                                      struct spi_msg *_m = &(m); \
                                      _m->op_type = t;           \
                                      _m->ctrl = c;              \
                                      _m->flags = f;             \
                                    } while (0)

/* Exported functions ------------------------------------------------------- */
int32_t spi_transaction_init(void);

int32_t spi_transaction_deinit(void);

struct spi_buffer *spi_buffer_alloc(uint32_t size, uint32_t reserve);

struct spi_buffer *spi_tx_buffer_alloc(uint32_t size);

void spi_buffer_free(struct spi_buffer *buf);

static inline void *spi_buffer_data(struct spi_buffer *buf)
{
  return buf->data;
}

static inline uint32_t spi_buffer_len(struct spi_buffer *buf)
{
  return buf->len;
}

int32_t spi_bind(unsigned char type, int32_t rxq_size);

int32_t spi_read(struct spi_msg *msg, int32_t timeout_ms);

int32_t spi_write(struct spi_msg *msg, int32_t timeout_ms);

int32_t spi_wait_event(void *evnt_ctx, uint32_t event, int32_t timeout_ms);

void spi_show_throuput_enable(int32_t en);

int32_t spi_on_transaction_ready(void);

int32_t spi_on_txn_data_ready(void);

int32_t spi_on_header_ack(void);

void spi_dump(void);

int32_t spi_get_stats(struct spi_stat *stat);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SPI_IFACE_H */
