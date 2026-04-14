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
/** SPI message control structure */
struct spi_msg_control
{
  /** Ref SPI_MSG_CTRL_xxx */
  uint8_t type;
  /** Length of the following control data */
  uint8_t len;
  /** Pointer to the control data */
  void *val;
};

/** SPI message structure */
struct spi_msg
{
  union
  {
    /** For spi_write and spi_read */
    struct
    {
      /** Data pointer for spi_write and spi_read */
      void *data;
      /** Data length for spi_write and spi_read */
      uint32_t data_len;
    };
    /** Data buffer for spi_write when type is SPI_MSG_OP_BUFFER */
    struct spi_buffer *buffer;
    /** Data buffer for spi_read when type is SPI_MSG_OP_BUFFER_PTR */
    struct spi_buffer **buffer_ptr;
  };
  /** Pointer to control information */
  struct spi_msg_control *ctrl;
  /** Message flags */
  uint32_t flags;
  /** Operation type: Ref SPI_MSG_OP_xxx */
  unsigned char op_type;
};

/** SPI transfer engine structure */
struct spi_stat
{
  uint64_t tx_pkts;                 /*!< Number of transmitted packets */
  uint64_t tx_bytes;                /*!< Number of transmitted bytes */
  uint64_t rx_pkts;                 /*!< Number of received packets */
  uint64_t rx_bytes;                /*!< Number of received bytes */
  uint64_t rx_drop;                 /*!< Number of dropped received packets */
  uint64_t io_err;                  /*!< Number of I/O errors */
  uint64_t hdr_err;                 /*!< Number of header errors */
  uint64_t wait_txn_timeouts;       /*!< Number of wait transaction timeouts */
  uint64_t wait_msg_xfer_timeouts;  /*!< Number of wait message transfer timeouts */
  uint64_t wait_hdr_ack_timeouts;   /*!< Number of wait header ack timeouts */
  uint64_t mem_err;                 /*!< Number of memory errors */
  uint64_t rx_stall;                /*!< Number of RX stalls */
};

/** SPI buffer structure */
struct spi_buffer
{
  /** Pointer to the data */
  void *data;
  /** Length of the data */
  uint16_t len;
  /** Capacity of the buffer, >= len */
  uint32_t cap;
  /** Buffer flags */
  uint32_t flags;
  /** Control block for private data */
  unsigned char cb[16];
};

/** Callback function type for received data notification */
typedef void (*spi_rxd_notify_func_t)(void *arg);

/** SPI message control traffic type enumeration */
typedef enum
{
  SPI_MSG_CTRL_TRAFFIC_AT_CMD = 0,
  SPI_MSG_CTRL_TRAFFIC_NETWORK_STA,
  SPI_MSG_CTRL_TRAFFIC_NETWORK_AP,
  SPI_MSG_CTRL_TRAFFIC_HCI,
  SPI_MSG_CTRL_TRAFFIC_OT,
  SPI_MSG_CTRL_TRAFFIC_TYPE_MAX,
} spi_msg_ctrl_t;

/* Exported constants --------------------------------------------------------*/
/** SPI message flags */
#define SPI_MSG_F_TRUNCATED            0x1U

/** SPI message control types */
#define SPI_MSG_CTRL_TRAFFIC_TYPE      0x1U

/** Length of traffic type control info */
#define SPI_MSG_CTRL_TRAFFIC_TYPE_LEN  1U

/** SPI message operation type: data copy */
#define SPI_MSG_OP_DATA                0U
/** SPI message operation type: buffer pointer for write */
#define SPI_MSG_OP_BUFFER              1U
/** SPI message operation type: buffer pointer for read */
#define SPI_MSG_OP_BUFFER_PTR          2U

/** Maximum SPI buffer size */
#define SPI_XFER_MTU_BYTES             W61_MAX_SPI_XFER

/* Exported macro ------------------------------------------------------------*/
/**
  * @brief  Initialize a SPI message control structure
  * @param  c:   spi_msg_control structure to initialize
  * @param  t:   Control type
  * @param  l:   Control data length
  * @param  v:   Pointer to control data
  */
#define SPI_MSG_CONTROL_INIT(c, t, l, v) do {                                    \
                                              struct spi_msg_control *_c = &(c); \
                                              _c->type = t;                      \
                                              _c->len = l;                       \
                                              _c->val = v;                       \
                                            } while (false)

/**
  * @brief  Initialize a SPI message structure
  * @param  m:   spi_msg structure to initialize
  * @param  t:   Operation type
  * @param  c:   Pointer to control information
  * @param  f:   Message flags
  */
#define SPI_MSG_INIT(m, t, c, f) do {                            \
                                      struct spi_msg *_m = &(m); \
                                      _m->op_type = t;           \
                                      _m->ctrl = c;              \
                                      _m->flags = f;             \
                                    } while (false)

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Initialize SPI transaction context
  * @return 0 on success, negative error code on failure
  */
int32_t spi_transaction_init(void);

/**
  * @brief  Deinitialize SPI transaction context
  * @return 0 on success, negative error code on failure
  */
int32_t spi_transaction_deinit(void);

/**
  * @brief  Allocate a SPI buffer
  * @param  size: Size of the buffer to allocate
  * @return Pointer to the allocated buffer, or NULL on failure
  */
struct spi_buffer *spi_tx_buffer_alloc(uint32_t size);

/**
  * @brief  Free a SPI buffer
  * @param  buf: Pointer to the buffer to free
  */
void spi_buffer_free(struct spi_buffer *buf);

/**
  * @brief  Get data pointer from SPI buffer
  * @param  buf: Pointer to the SPI buffer
  * @return Pointer to the data
  */
static inline void *spi_buffer_data(struct spi_buffer *buf)
{
  return buf->data;
}

/**
  * @brief  Get data length from SPI buffer
  * @param  buf: Pointer to the SPI buffer
  * @return Length of the data
  */
static inline uint32_t spi_buffer_len(struct spi_buffer *buf)
{
  return buf->len;
}

/**
  * @brief  Bind a specific traffic type to a dedicated receive queue
  * @param  type:     Traffic type to bind (must be < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  * @param  rxq_size: Size of receive queue to create (0 for default)
  * @return           0 on success, negative value on error
  *
  * @note This function should be called during initialization before
  * any SPI transfers start, as it's not fully thread-safe. Multiple
  * concurrent binds or binding while transfers are in progress could
  * lead to race conditions.
  */
int32_t spi_bind(unsigned char type, int32_t rxq_size);

/**
  * @brief  Read data from the SPI interface
  *         This function handles both raw data and buffer pointer operations based on the
  *         message operation type (op_type) field.
  *
  * @param  msg:        Message structure for receiving data
  * @param  timeout_ms: Timeout for queue operations in milliseconds (-1 for infinite)
  * @return             Number of bytes read or negative error code
  */
int32_t spi_read(struct spi_msg *msg, int32_t timeout_ms);

/**
  * @brief  Write data to the SPI interface
  *         This function handles both raw data and pre-allocated buffers based on the
  *         message operation type (op_type) field.
  *
  * @param  msg:        Message containing data or buffer to send
  * @param  timeout_ms: Timeout for queue operations in milliseconds (-1 for infinite)
  * @return             Number of bytes written or negative error code
  */
int32_t spi_write(struct spi_msg *msg, int32_t timeout_ms);

/**
  * @brief  Register a callback for received data of a specific traffic type
  * @param  type: Traffic type
  * @param  cb: Callback function
  * @param  arg: Argument to pass to the callback
  * @return 0 on success, negative error code on failure
  */
int32_t spi_rxd_callback_register(spi_msg_ctrl_t type, spi_rxd_notify_func_t cb, void *arg);

/**
  * @brief  Handle SPI transaction/data ready event
  * @return 0 on success, negative error code on failure
  */
int32_t spi_on_txn_data_ready(void);

/**
  * @brief  Handle SPI header acknowledgment event
  * @return 0 on success, negative error code on failure
  */
int32_t spi_on_header_ack(void);

/**
  * @brief  Dump SPI transaction state and statistics
  */
void spi_dump(void);

/**
  * @brief  Get SPI transaction statistics
  * @param  stat: Pointer to the statistics structure to fill
  * @return 0 on success, negative error code on failure
  */
int32_t spi_get_stats(struct spi_stat *stat);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SPI_IFACE_H */
