/**
  ******************************************************************************
  * @file    spi_iface.c
  * @brief   SPI bus interface implementation
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

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

/* For redefinitions purposes */
#include "w61_default_config.h"

#include "spi_iface.h"
#include "spi_port.h"

/** SPI header magic code */
#define SPI_HEADER_MAGIC_CODE           0x55AAU

#ifndef SPI_THREAD_STACK_SIZE
/** SPI thread stack size */
#define SPI_THREAD_STACK_SIZE           768U
#endif /* SPI_THREAD_STACK_SIZE */

#ifndef SPI_THREAD_PRIO
/** SPI thread priority */
#define SPI_THREAD_PRIO                 53
#endif /* SPI_THREAD_PRIO */

#ifndef SPI_TXQ_LEN
/** SPI Tx Queue length */
#define SPI_TXQ_LEN                     8U
#endif /* SPI_TXQ_LEN */

#ifndef SPI_RXQ_LEN
/** SPI Rx Queue length */
#define SPI_RXQ_LEN                     8U
#endif /* SPI_RXQ_LEN */

/** Maximum digits for 64-bit integer string representation */
#define STR64BIT_DIGIT                  (20 + 1)

/** SPI message header structure */
struct spi_header
{
  /** Magic number for header validation */
  uint16_t magic;
  /** Length of the payload */
  uint16_t len;
  /** Header version */
  uint8_t version : 2;
  /** Peer RX is stalled */
  uint8_t rx_stall : 1;
  /** Header flags */
  uint8_t flags : 5;
  /** Message type */
  uint8_t type;
  /** Reserved for future use */
  uint16_t rsvd;
} __attribute__((packed));

/** Initialize SPI header */
#define SPI_HEADER_INIT(h, _type, _len) do {                                     \
                                             (h)->magic = SPI_HEADER_MAGIC_CODE; \
                                             (h)->type = _type;                  \
                                             (h)->version = 0;                   \
                                             (h)->rx_stall = 0;                  \
                                             (h)->flags = 0;                     \
                                             (h)->len = _len;                    \
                                             (h)->rsvd = 0;                      \
                                           } while (false)

/** SPI transfer state */
enum
{
  SPI_XFER_STATE_IDLE,
  SPI_XFER_STATE_FIRST_PART,
  SPI_XFER_STATE_SECOND_PART,
  SPI_XFER_STATE_TXN_DONE,
};

/** SPI transfer flags */
#define SPI_XFER_F_SKIP_FIRST_TXN_WAIT  0x1U

/** Increment SPI statistics field */
#define SPI_STAT_INC(stat, mb, val) (stat)->mb += (val)

/**
  * SPI transfer engine structure
  * This structure manages the state and resources for SPI communication,
  * including support for multiple receive queues bound to different message types.
  */
struct spi_xfer_engine
{
  /** Transfer task handle */
  TaskHandle_t task;
  /** Stop flag for clean shutdown */
  int32_t stop;
  /** Initialization flag */
  int32_t initialized;
  /** Event flags for synchronization */
  EventGroupHandle_t event;
  /** Current transfer state */
  int32_t state;
  /** Slave RX is stalled */
  int32_t rx_stall;
  /** Single transmit queue */
  QueueHandle_t txq;
  /** Array of receive queues by type */
  QueueHandle_t rxq[SPI_MSG_CTRL_TRAFFIC_TYPE_MAX];
  /** Bitmap of bound traffic types */
  uint8_t rxq_bound;
  /** Current transmit buffer */
  struct spi_buffer *txbuf;
  /** Transfer statistics */
  struct spi_stat stat;
  /** Received data callbacks for each traffic type */
  spi_rxd_notify_func_t cb[SPI_MSG_CTRL_TRAFFIC_TYPE_MAX];
  /** Callback arguments for each traffic type */
  void *cb_arg[SPI_MSG_CTRL_TRAFFIC_TYPE_MAX];
};

/** SPI transfer engine instance */
static struct spi_xfer_engine xfer_engine = {0};

/** SPI buffer alignment mask */
#define SPI_BUF_ALIGN_MASK              (0x0003U)

/** SPI transfer trace points */
enum
{
  SPI_TP_NONE = 0,
  SPI_TP_WRITE = 1,
  SPI_TP_ASSERT_CS = 2,
  SPI_TP_SLAVE_TXN_RDY = 3,
  SPI_TP_FIRST_TXN_START = 4,
  SPI_TP_FIRST_TXN_END = 5,
  SPI_TP_SECOND_TXN_START = 6,
  SPI_TP_SECOND_TXN_END = 7,
  SPI_TP_WAIT_HDR_ACK_START = 8,
  SPI_TP_HDR_ACKED = 9,
  SPI_TP_WAIT_HDR_ACK_END = 10,
  SPI_TP_DEASSERT_CS = 11,
  SPI_TP_READ = 12,
  SPI_TP_NUM = 12,
};

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Set the traffic type in the SPI buffer
  * @param  buf: Pointer to the SPI buffer
  * @param  type: Traffic type to set
  */
static inline void spi_buffer_set_traffic_type(struct spi_buffer *buf, uint8_t type);

/**
  * @brief  Get the traffic type from the SPI buffer
  * @param  buf: Pointer to the SPI buffer
  * @return Traffic type
  */
static inline uint8_t spi_buffer_get_traffic_type(struct spi_buffer *buf);

/**
  * @brief  Push data to the front of the SPI buffer
  * @param  buf: Pointer to the SPI buffer
  * @param  size: Size of the data to push
  * @return Pointer to the start of the pushed data
  */
static inline void *spi_buffer_push(struct spi_buffer *buf, uint32_t size);

/**
  * @brief  Pull data from the front of the SPI buffer
  * @param  buf: Pointer to the SPI buffer
  * @param  size: Size of the data to pull
  * @return Pointer to the start of the remaining data
  */
static inline void *spi_buffer_pull(struct spi_buffer *buf, uint32_t size);

/**
  * @brief  Allocate a SPI buffer
  * @param  size: Size of the buffer to allocate
  * @param  reserve: Reserved size for internal use
  * @return Pointer to the allocated buffer, or NULL on failure
  */
static struct spi_buffer *spi_buffer_alloc(uint32_t size, uint32_t reserve);

/**
  * @brief  Get transmit buffer from the SPI transfer engine
  * @param  engine: Pointer to the SPI transfer engine
  * @return Pointer to the transmit buffer
  */
static struct spi_buffer *spi_get_txbuf(struct spi_xfer_engine *engine);

/**
  * @brief  Validate the SPI header
  * @param  hdr: Pointer to the SPI header
  * @return 1 if valid, 0 if invalid
  */
static int32_t inline spi_header_validate(struct spi_header *hdr);

/**
  * @brief  Wait for a specific event in the SPI transfer engine
  * @param  evnt_ctx: Event context
  * @param  event: Event to wait for
  * @param  timeout_ms: Timeout in milliseconds
  * @return 0 on success, negative error code on failure
  */
static int32_t spi_wait_event(void *evnt_ctx, uint32_t event, int32_t timeout_ms);

/**
  * @brief  Clear a specific event in the SPI transfer engine
  * @param  engine: Pointer to the SPI transfer engine
  * @param  event: Event to clear
  * @return 0 on success, negative error code on failure
  */
static int32_t inline spi_clear_event(struct spi_xfer_engine *engine, uint32_t event);

/**
  * @brief  Perform a full-duplex SPI transfer
  * @param  engine: Pointer to the SPI transfer engine
  * @param  tx_buf: Pointer to the transmit buffer
  * @param  rx_buf: Pointer to the receive buffer
  * @param  len: Length of the transfer
  * @return 0 on success, negative error code on failure
  */
static int32_t spi_txrx(struct spi_xfer_engine *engine, void *tx_buf, void *rx_buf, uint16_t len);

/**
  * @brief  Perform a SPI transmit-only operation
  * @param  engine: Pointer to the SPI transfer engine
  * @param  rx_buf: Pointer to the receive buffer
  * @param  len: Length of the transmit
  * @return 0 on success, negative error code on failure
  */
static int32_t spi_rx(struct spi_xfer_engine *engine, void *rx_buf, uint16_t len);

/**
  * @brief  Perform a SPI transaction with the given buffer
  * @param  engine: Pointer to the SPI transfer engine
  * @param  txbuf: Pointer to the transmit buffer
  * @param  wait_txn_rdy: Flag to indicate whether to wait for transaction ready
  * @return 0 on success, negative error code on failure
  */
static int32_t spi_xfer_one(struct spi_xfer_engine *engine, struct spi_buffer *txbuf,
                            int32_t wait_txn_rdy);

/**
  * @brief  Perform SPI transfers based on the engine state and flags
  * @param  engine: Pointer to the SPI transfer engine
  * @param  flags: Transfer flags
  * @return 0 on success, negative error code on failure
  */
static int32_t spi_do_xfer(struct spi_xfer_engine *engine, uint32_t flags);

/**
  * @brief  SPI transfer engine task
  * @param  arg: Task argument
  */
static void spi_xfer_engine_task(void *arg);

/**
  * @brief  SPI transaction complete callback
  */
static void spi_on_transaction_complete(void);

/**
  * @brief  Convert a 64-bit number to string
  * @param  out: Output string buffer
  * @param  out_strlen: Length of the output buffer
  * @param  num: 64-bit number to convert
  */
static void num2string64(char *out, int32_t out_strlen, const uint64_t num);

/**
  * @brief  Display SPI transaction statistics
  * @param  stat: Pointer to the statistics structure
  */
static void spi_show_stat(struct spi_stat *stat);

/* Functions Definition ------------------------------------------------------*/
static inline void spi_buffer_set_traffic_type(struct spi_buffer *buf, uint8_t type)
{
  buf->cb[0] = type;
}

static inline uint8_t spi_buffer_get_traffic_type(struct spi_buffer *buf)
{
  return buf->cb[0];
}

/* For prepending data like header. */
static inline void *spi_buffer_push(struct spi_buffer *buf, uint32_t size)
{
  char *p, *start, *data;

  if (buf == NULL)
  {
    return NULL;
  }

  if (size == 0U)
  {
    return buf->data;
  }

  p = (char *)buf;
  data = buf->data;
  start = p + sizeof(struct spi_buffer);
  if ((data - size) < start)
  {
    spi_err("Illegal try of spi buffer push!\n");
    return NULL;
  }

  buf->data = data - size;
  buf->len += size;
  return buf->data;
}

/* For removing leading data like header. */
static inline void *spi_buffer_pull(struct spi_buffer *buf, uint32_t size)
{
  char *p, *end, *data;

  if (buf == NULL)
  {
    return NULL;
  }

  if (size == 0U)
  {
    return buf->data;
  }

  p = (char *)buf;
  data = buf->data;
  end = p + sizeof(struct spi_buffer) + buf->cap;
  if ((data + size) > end)
  {
    spi_err("Illegal try of spi buffer pull\n");
    return NULL;
  }

  buf->data = data + size;
  buf->len -= size;
  return buf->data;
}

static struct spi_buffer *spi_buffer_alloc(uint32_t size, uint32_t reserve)
{
  uint32_t cap;
  uint32_t extra;
  struct spi_buffer *buf;
  uint32_t desc_size = sizeof(struct spi_buffer);

  cap = (size + reserve + SPI_BUF_ALIGN_MASK) & ~SPI_BUF_ALIGN_MASK;
  buf = pvPortMalloc(cap + desc_size);
  if (buf == NULL)
  {
    return NULL;
  }

  buf->flags = 0;
  buf->len = size;
  buf->cap = cap;
  buf->data = (char *)buf + desc_size + reserve;
  (void)memset(buf->cb, 0, sizeof(buf->cb));
  /* Fill in the debug pattern. */
  extra = cap - size - reserve;
  while (extra > 0U)
  {
    uint8_t *p = (uint8_t *)buf + desc_size;

    p[cap - extra] = 0x88;
    extra--;
  }
  return buf;
}

struct spi_buffer *spi_tx_buffer_alloc(uint32_t size)
{
  return spi_buffer_alloc(size, sizeof(struct spi_header));
}

void spi_buffer_free(struct spi_buffer *buf)
{
  if (buf)
  {
    vPortFree(buf);
  }
}

/* Get tx buffer from in non-blocking mode. */
static struct spi_buffer *spi_get_txbuf(struct spi_xfer_engine *engine)
{
  struct spi_buffer *buf = NULL;
  BaseType_t ret;

  if (engine->txbuf != NULL)
  {
    return engine->txbuf;
  }

  ret = xQueueReceive(engine->txq, &buf, 0);
  if (ret != pdTRUE)
  {
    buf = NULL;
  }

  engine->txbuf = buf;
  if (buf != NULL)
  {
    SPI_STAT_INC(&engine->stat, tx_pkts, 1U);
    SPI_STAT_INC(&engine->stat, tx_bytes, buf->len);
  }
  return buf;
}

/* Return 1 if the header is valid. */
static int32_t inline spi_header_validate(struct spi_header *hdr)
{
  if (hdr->magic != SPI_HEADER_MAGIC_CODE)
  {
    spi_err("Invalid magic 0x%" PRIx16 "\n", hdr->magic);
    return 0;
  }

  if (hdr->len > SPI_XFER_MTU_BYTES)
  {
    spi_err("invalid len %" PRIu16 "\n", hdr->len);
    return 0;
  }
  return 1;
}

static int32_t spi_wait_event(void *evnt_ctx, uint32_t event, int32_t timeout_ms)
{
  EventBits_t bits;
  TickType_t ticks;
  struct spi_xfer_engine *engine = (struct spi_xfer_engine *)evnt_ctx;

  if (timeout_ms >= 0)
  {
    ticks = pdMS_TO_TICKS(timeout_ms);
  }
  else
  {
    ticks = portMAX_DELAY;
  }

  bits = xEventGroupWaitBits(engine->event, event, pdTRUE, pdFALSE, ticks);
  if ((bits & event) != 0U)
  {
    return 1;
  }

  return 0;
}

static int32_t inline spi_clear_event(struct spi_xfer_engine *engine, uint32_t event)
{
  (void)xEventGroupClearBits(engine->event, event);
  return 0;
}

static int32_t spi_txrx(struct spi_xfer_engine *engine, void *tx_buf, void *rx_buf, uint16_t len)
{
  int32_t status;

  if ((engine == NULL) || (tx_buf == NULL) || (rx_buf == NULL))
  {
    return -1;
  }

  if (len == 0U)
  {
    return 0;
  }

  if (len <= SPI_DMA_XFER_SIZE_THRESHOLD)
  {
    status = spi_port_transfer(tx_buf, rx_buf, len, SPI_WAIT_POLL_XFER_TIMEOUT_MS);
    if (status < 0)
    {
      spi_err("spi txrx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1U);
      return -2;
    }
  }
  else
  {
    status = spi_port_transfer_dma(tx_buf, rx_buf, len);
    if (status < 0)
    {
      spi_err("spi txrx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1U);
      return -2;
    }
    /* Wait for spi transaction completion. */
    if (spi_wait_event(engine, SPI_EVT_HW_XFER_DONE, SPI_WAIT_MSG_XFER_TIMEOUT_MS) == 0)
    {
      spi_err("spi txrx transaction timeouted\n");
      SPI_STAT_INC(&engine->stat, wait_msg_xfer_timeouts, 1U);
      return -3;
    }
  }

  return 0;
}

static int32_t spi_rx(struct spi_xfer_engine *engine, void *rx_buf, uint16_t len)
{
  int32_t status;

  if ((engine == NULL) || (rx_buf == NULL))
  {
    return -1;
  }

  if (len == 0U)
  {
    return 0;
  }

  if (len <= SPI_DMA_XFER_SIZE_THRESHOLD)
  {
    status = spi_port_transfer(NULL, rx_buf, len, SPI_WAIT_POLL_XFER_TIMEOUT_MS);
    if (status < 0)
    {
      spi_err("spi rx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1U);
      return -2;
    }
  }
  else
  {
    status = spi_port_transfer_dma(NULL, rx_buf, len);
    if (status < 0)
    {
      spi_err("spi rx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1U);
      return -2;
    }
    /* Wait for spi transaction completion. */
    if (spi_wait_event(engine, SPI_EVT_HW_XFER_DONE, SPI_WAIT_MSG_XFER_TIMEOUT_MS) == 0)
    {
      spi_err("spi rx transaction timeouted\n");
      SPI_STAT_INC(&engine->stat, wait_msg_xfer_timeouts, 1U);
      return -3;
    }
  }

  return 0;
}

static int32_t spi_xfer_one(struct spi_xfer_engine *engine, struct spi_buffer *txbuf,
                            int32_t wait_txn_rdy)
{
  void *txp;
  int32_t ret;
  int32_t err = -1;
  uint16_t xfer_size;
  struct spi_header mh;
  struct spi_header *pmh;
  struct spi_header *psh;
  struct spi_buffer *rxbuf = NULL;
  int32_t rx_restore = 0;

  spi_trace(SPI_TP_NONE, "wait_txn_rdy %" PRIi32 "\n", wait_txn_rdy);
  engine->state = SPI_XFER_STATE_FIRST_PART;
  if ((wait_txn_rdy != 0) &&
      (spi_wait_event(engine, SPI_EVT_TXN_RDY, SPI_WAIT_TXN_TIMEOUT_MS) == 0))
  {
    SPI_STAT_INC(&engine->stat, wait_txn_timeouts, 1U);
    spi_err("waiting for spi txn ready timeouted\n");
    return -1;
  }

  /* Re-initialize events in case of pending ones. */
  (void)spi_clear_event(engine, SPI_EVT_HW_XFER_DONE | SPI_EVT_HDR_ACKED);

  /* Yes, this allocation will be wasted if there is no data from slave. */
  rxbuf = spi_buffer_alloc(SPI_XFER_MTU_BYTES + sizeof(struct spi_header), 0);
  if (rxbuf == NULL)
  {
    spi_err("No mem for rxbuf\n");
    SPI_STAT_INC(&engine->stat, mem_err, 1U);
    return -1;
  }

  spi_trace(SPI_TP_FIRST_TXN_START, "start the first transaction\n");
  if (txbuf != NULL)
  {
    uint16_t msglen;
    uint8_t type = spi_buffer_get_traffic_type(txbuf);

    if (engine->rx_stall == 0)
    {
      pmh = txbuf->data;

      msglen = txbuf->len - sizeof(struct spi_header);

      /* Initialize master header. */
      SPI_HEADER_INIT(pmh, type, msglen);
      txp = txbuf->data;
      xfer_size = (txbuf->len + SPI_BUF_ALIGN_MASK) & ~SPI_BUF_ALIGN_MASK;
    }
    else
    {
      SPI_HEADER_INIT(&mh, 0, 0);
      txp = &mh;
      xfer_size = sizeof(struct spi_header);
    }
  }
  else
  {
    /* Initialize master header. */
    SPI_HEADER_INIT(&mh, 0, 0);
    txp = &mh;
    xfer_size = sizeof(struct spi_header);
  }

  if (spi_txrx(engine, txp, rxbuf->data, xfer_size) != 0)
  {
    spi_err("Failed to do the first transaction\n");
    goto out;
  }

  psh = rxbuf->data;
  spi_trace(SPI_TP_FIRST_TXN_END,
            "slave spi header, magic 0x%" PRIx16 ", len (%" PRIu16 ", 0x%" PRIx16 "), version %" PRIu16
            ", type %" PRIx16 ", flags 0x%" PRIx16 ", rsvd 0x%" PRIx16 "\n",
            psh->magic, psh->len, psh->len, psh->version, psh->type, psh->flags, psh->rsvd);
  ret = spi_header_validate(psh);
  if (ret == 0)
  {
    spi_err("Invalid spi header from peer, magic 0x%" PRIx16 ", len (%" PRIu16 ", 0x%" PRIx16 "), version %" PRIu16
            ", type %" PRIx16 "flags 0x%" PRIx16 ", rsvd 0x%" PRIx16 "\n",
            psh->magic, psh->len, psh->len, psh->version, psh->type, psh->flags, psh->rsvd);
    SPI_STAT_INC(&engine->stat, hdr_err, 1U);
    goto out;
  }

  if (engine->rx_stall == 0)
  {
    if (psh->rx_stall != 0U)
    {
      SPI_STAT_INC(&engine->stat, rx_stall, 1U);
      spi_trace(SPI_TP_NONE, "Slave RX stalled\n");
    }
  }
  else if (psh->rx_stall == 0U)
  {
    rx_restore = 1;
  }
  else
  {
    /* Normal case */
  }
  engine->rx_stall = psh->rx_stall;

  /* Receive the remaining data from slave if any. */
  if ((psh->len + sizeof(struct spi_header)) > xfer_size)
  {
    uint8_t *rxp = (uint8_t *)rxbuf->data;
    uint16_t remain = psh->len + sizeof(struct spi_header) - xfer_size;

    spi_trace(SPI_TP_SECOND_TXN_START, "Receiving remaining data\n");
    engine->state = SPI_XFER_STATE_SECOND_PART;
    remain = (remain + SPI_BUF_ALIGN_MASK) & ~SPI_BUF_ALIGN_MASK;
    rxp += xfer_size;
    if (spi_rx(engine, rxp, remain) != 0)
    {
      spi_err("Failed to receive the remaining bytes\n");
      goto out;
    }
    spi_trace(SPI_TP_SECOND_TXN_END, "Remaining transfer completed\n");
  }

  /* Free txbuf after the message transaction. */
  if (txbuf != NULL)
  {
    if ((engine->rx_stall == 0) && (rx_restore == 0))
    {
      engine->txbuf = NULL;
      spi_buffer_free(txbuf);
    }
  }

  if (psh->len > 0U)
  {
    SPI_STAT_INC(&engine->stat, rx_pkts, 1U);
    SPI_STAT_INC(&engine->stat, rx_bytes, psh->len);
    (void)spi_buffer_pull(rxbuf, sizeof(struct spi_header));
    /* Rx buffer length fix-up */
    rxbuf->len = psh->len;
    /* Get message type from header and store in buffer's control block */
    uint8_t msg_type = psh->type;
    spi_buffer_set_traffic_type(rxbuf, msg_type);

    /*
     * Note: Thread safety consideration required here.
     * If a queue is unbound by another thread while we're accessing it,
     * or if multiple transfers are accessing rxq_bound bitmap concurrently,
     * race conditions could occur. Consider using appropriate synchronization.
     */

    /* Select appropriate queue based on type */
    if ((msg_type < (uint8_t)SPI_MSG_CTRL_TRAFFIC_TYPE_MAX) &&
        ((engine->rxq_bound & (1U << msg_type)) != 0U))
    {
      if (psh->len < 256U)
      {
        struct spi_buffer *rxbuf_resized = spi_buffer_alloc(psh->len, 0);
        if (rxbuf_resized == NULL)
        {
          spi_err("No mem for rxbuf\n");
          SPI_STAT_INC(&engine->stat, mem_err, 1U);
        }
        else
        {
          rxbuf_resized->flags = rxbuf->flags;
          (void)spi_port_memcpy(rxbuf_resized->data, rxbuf->data, psh->len);
          spi_buffer_free(rxbuf);
          rxbuf = rxbuf_resized;
        }
      }

      ret = xQueueSend(engine->rxq[msg_type], &rxbuf, portMAX_DELAY);
      if (ret != pdTRUE)
      {
        spi_trace(SPI_TP_NONE, "failed to send to type %d rxq, msg discarded\r\n", msg_type);
        spi_buffer_free(rxbuf);
        SPI_STAT_INC(&engine->stat, rx_drop, 1U);
      }
      else
      {
        if (engine->cb[msg_type] != NULL)
        {
          engine->cb[msg_type](engine->cb_arg[msg_type]);
        }
      }
    }
    else
    {
      /* No queue bound for this type, discard message */
      spi_trace(SPI_TP_NONE, "No queue bound for type %d, msg discarded\r\n", msg_type);
      spi_buffer_free(rxbuf);
      SPI_STAT_INC(&engine->stat, rx_drop, 1U);
    }
  }
  else if (rxbuf != NULL)
  {
    spi_buffer_free(rxbuf);
  }
  else
  {
    /* No rx bytes to read */
  }

  /* Wait until slave acknowledged header. */
  spi_trace(SPI_TP_WAIT_HDR_ACK_START, "waiting for header ack\n");
  while (spi_wait_event(engine, SPI_EVT_HDR_ACKED, SPI_WAIT_HDR_ACK_TIMEOUT_MS) == 0)
  {
    if (spi_port_is_ready() == 0)
    {
      break;
    }

    SPI_STAT_INC(&engine->stat, wait_hdr_ack_timeouts, 1U);
  }
  spi_trace(SPI_TP_WAIT_HDR_ACK_END, "Got header ack\n");

  return 0;

out:
  spi_buffer_free(rxbuf);
  return err;
}

static int32_t spi_do_xfer(struct spi_xfer_engine *engine, uint32_t flags)
{
  struct spi_buffer *txbuf;
  int32_t rx_pending;
  int32_t wait_txn_rdy;

  while (true)
  {
    /* Get txbuf */
    txbuf = spi_get_txbuf(engine);

    /* Read data ready GPIO level */
    rx_pending = spi_port_is_ready();

    spi_trace(SPI_TP_NONE, "txbuf %" PRIuPTR ", rx_pending %" PRIi32 "\n", txbuf, rx_pending);
    /* Check if txbuf is valid or data is ready for transfer */
    if ((txbuf != NULL) || (rx_pending == 1))
    {
      /* Assert chip select */
      spi_trace(SPI_TP_ASSERT_CS, "Assert CS\n");
      (void)spi_port_set_cs(1);

      /* Transfer one packet */
      if ((flags & SPI_XFER_F_SKIP_FIRST_TXN_WAIT) != 0U)
      {
        flags &= ~SPI_XFER_F_SKIP_FIRST_TXN_WAIT;
        wait_txn_rdy = 0;
      }
      else
      {
        wait_txn_rdy = 1;
      }

      (void)spi_xfer_one(engine, txbuf, wait_txn_rdy);

      /* De-assert chip select */
      spi_trace(SPI_TP_DEASSERT_CS, "Deassert CS\n");
      (void)spi_port_set_cs(0);
      engine->state = SPI_XFER_STATE_TXN_DONE;
    }
    else
    {
      /* Neither txbuf nor rx_pending require processing, exit loop */
      break;
    }
  }

  return 0;
}

static void spi_xfer_engine_task(void *arg)
{
  EventBits_t bits;
  struct spi_xfer_engine *engine = arg;

  /* Wait for events and process them. */
  while (engine->stop == 0)
  {
    engine->state = SPI_XFER_STATE_IDLE;
    bits = SPI_EVT_TXN_PENDING | SPI_EVT_TXN_RDY;
    bits = xEventGroupWaitBits(engine->event, bits, pdTRUE, pdFALSE,
                               portMAX_DELAY);
    spi_trace(SPI_TP_NONE, "Got event bits %" PRIx32 "\n", bits);
    if ((bits & SPI_EVT_TXN_RDY) != 0U)
    {
      (void)spi_do_xfer(engine, SPI_XFER_F_SKIP_FIRST_TXN_WAIT);
    }
    else if ((bits & SPI_EVT_TXN_PENDING) != 0U)
    {
      (void)spi_do_xfer(engine, 0U);
    }
    else
    {
      /* Event not managed */
    }
  }
}

int32_t spi_transaction_init(void)
{
  int32_t ret;

  /* Initialize rxq array to NULL and rxq_bound to 0 */
  (void)memset(xfer_engine.rxq, 0, sizeof(xfer_engine.rxq));
  xfer_engine.rxq_bound = 0;

  /* Create event group for SPI transaction */
  xfer_engine.event = xEventGroupCreate();
  if (xfer_engine.event == NULL)
  {
    spi_err("Failed to create event group for SPI\n");
    ret = -1;
    goto error;
  }

  /* Create TX queue */
  xfer_engine.txq = xQueueCreate(SPI_TXQ_LEN, sizeof(void *));
  if (xfer_engine.txq == NULL)
  {
    spi_err("Failed to create txq\n");
    ret = -1;
    goto error;
  }

  if (spi_port_init(spi_on_transaction_complete) != 0)
  {
    ret = -1;
    goto error;
  }

  /* Create SPI transfer engine task */
  xfer_engine.stop = 0;
  (void)xTaskCreate(spi_xfer_engine_task, "spi_xfer_engine", SPI_THREAD_STACK_SIZE >> 2U,
                    &xfer_engine, SPI_THREAD_PRIO, &xfer_engine.task);

  if (xfer_engine.task == NULL)
  {
    spi_err("Failed to create spi xfer engine task\n");
    ret = -1;
    goto error;
  }

  /* Check the state of the slave data ready pin */
  if (spi_port_is_ready() == 1)
  {
    /* Set the TXN event if data is ready */
    (void)xEventGroupSetBits(xfer_engine.event, SPI_EVT_TXN_RDY);
  }

  xfer_engine.rx_stall = 0;
  xfer_engine.initialized = 1;
  return 0;

error:
  /* Clean up resources in case of error */
  if (xfer_engine.txq != NULL)
  {
    vQueueDelete(xfer_engine.txq);
    xfer_engine.txq = NULL;
  }

  if (xfer_engine.event != NULL)
  {
    vEventGroupDelete(xfer_engine.event);
    xfer_engine.event = NULL;
  }

  return ret;
}

int32_t spi_transaction_deinit(void)
{
  if (xfer_engine.task != NULL)
  {
    vTaskDelete(xfer_engine.task);
    xfer_engine.task = NULL;
  }

  /* Clean up resources in case of error */
  if (xfer_engine.txq != NULL)
  {
    vQueueDelete(xfer_engine.txq);
  }

  if (xfer_engine.event != NULL)
  {
    vEventGroupDelete(xfer_engine.event);
  }

  for (int32_t i = 0; i < (int32_t)SPI_MSG_CTRL_TRAFFIC_TYPE_MAX; i++)
  {
    if (xfer_engine.rxq[i] != NULL)
    {
      vQueueDelete(xfer_engine.rxq[i]);
    }
  }

  /* Clear xfer_engine context */
  (void)memset(&xfer_engine, 0, sizeof(struct spi_xfer_engine));

  (void)spi_port_deinit();

  return 0;
}

int32_t spi_read(struct spi_msg *msg, int32_t timeout_ms)
{
  BaseType_t ret;
  TickType_t ticks;
  struct spi_buffer *buf = NULL;
  uint8_t traffic_type = SPI_MSG_CTRL_TRAFFIC_AT_CMD; /* Default to AT commands */
  QueueHandle_t targetQ;

  /* Verify message is valid and operation type is supported */
  if ((msg == NULL) || ((msg->op_type != SPI_MSG_OP_DATA) && (msg->op_type != SPI_MSG_OP_BUFFER_PTR)))
  {
    return -1;
  }

  /* Verify appropriate fields based on operation type */
  if (msg->op_type == SPI_MSG_OP_DATA)
  {
    if ((msg->data == NULL) || (msg->data_len == 0U))
    {
      return -1;
    }
  }
  else if (msg->op_type == SPI_MSG_OP_BUFFER_PTR)
  {
    if (msg->buffer_ptr == NULL)
    {
      return -1;
    }
  }
  else
  {
  }

  /* Verify context is initialized */
  if (xfer_engine.initialized == 0)
  {
    spi_err("SPI transaction is NOT initialized!\n");
    return -2;
  }

  /* Process control information if present */
  if (msg->ctrl != NULL)
  {
    /* Check for traffic type control information */
    if (msg->ctrl->type == SPI_MSG_CTRL_TRAFFIC_TYPE)
    {
      /* Validate control data length */
      if ((msg->ctrl->len == SPI_MSG_CTRL_TRAFFIC_TYPE_LEN) && (msg->ctrl->val != NULL))
      {
        /* Extract traffic type */
        traffic_type = *((uint8_t *)msg->ctrl->val);
      }
      else
      {
        spi_err("Invalid traffic type control info\r\n");
      }
    }
  }

  /* Verify traffic type is valid and bound */
  if ((traffic_type >= (uint8_t)SPI_MSG_CTRL_TRAFFIC_TYPE_MAX) ||
      ((xfer_engine.rxq_bound & (1U << traffic_type)) == 0U))
  {
    spi_err("Traffic type %d not bound, operation not allowed\r\n", traffic_type);
    return -3;
  }

  /* Get the target queue for this traffic type */
  targetQ = xfer_engine.rxq[traffic_type];
  spi_trace(SPI_TP_READ, "spi_read type %d\r\n", traffic_type);

  /* Set up timeout value */
  if (timeout_ms < 0)
  {
    ticks = portMAX_DELAY;
  }
  else
  {
    ticks = pdMS_TO_TICKS(timeout_ms);
  }

  /* Process based on operation type */
  switch (msg->op_type)
  {
    case SPI_MSG_OP_DATA:
      /* Receive buffer from queue */
      ret = xQueueReceive(targetQ, &buf, ticks);
      if (ret != pdTRUE)
      {
        return -4;
      }

      /* Copy data with truncation handling */
      if (msg->data_len >= buf->len)
      {
        msg->data_len = buf->len;
        msg->flags &= ~SPI_MSG_F_TRUNCATED;
      }
      else
      {
        msg->flags |= SPI_MSG_F_TRUNCATED;
      }

      /* Copy data and free buffer */
      (void)spi_port_memcpy(msg->data, buf->data, msg->data_len);
      spi_buffer_free(buf);
      return msg->data_len;
      break;

    case SPI_MSG_OP_BUFFER_PTR:
      /* Receive buffer pointer directly into msg->buffer_ptr */
      ret = xQueueReceive(targetQ, msg->buffer_ptr, ticks);
      if (ret != pdTRUE)
      {
        return -4;
      }

      return (*(msg->buffer_ptr))->len;
      break;

    default:
      /* Types unmanaged in read mode */
      break;
  }

  /* Should never reach here, but just in case */
  return -1;
}

int32_t spi_write(struct spi_msg *msg, int32_t timeout_ms)
{
  BaseType_t ret;
  struct spi_buffer *buf;
  TickType_t ticks;
  uint8_t traffic_type = SPI_MSG_CTRL_TRAFFIC_AT_CMD;
  uint32_t data_len = 0;

  /* Verify message is valid and operation type is supported */
  if ((msg == NULL) || ((msg->op_type != SPI_MSG_OP_DATA) && (msg->op_type != SPI_MSG_OP_BUFFER)))
  {
    return -1;
  }

  /* Verify context is initialized */
  if (xfer_engine.initialized == 0)
  {
    spi_err("SPI transaction is NOT initialized!\r\n");
    return -2;
  }

  /* Process control information if present */
  if (msg->ctrl != NULL)
  {
    /* Check for traffic type control information */
    if (msg->ctrl->type == SPI_MSG_CTRL_TRAFFIC_TYPE)
    {
      /* Validate control data length */
      if ((msg->ctrl->len == SPI_MSG_CTRL_TRAFFIC_TYPE_LEN) && (msg->ctrl->val != NULL))
      {
        /* Extract traffic type */
        traffic_type = *((uint8_t *)msg->ctrl->val);
      }
      else
      {
        spi_err("Invalid traffic type control info\r\n");
      }
    }
    /* Additional control types can be handled here in the future */
  }

  /* Verify data length does not exceed maximum transfer unit */
  if (msg->data_len > SPI_XFER_MTU_BYTES)
  {
    spi_err("Invalid len %" PRIu16 "\n", msg->data_len);
    return -3;
  }

  /* Process based on operation type */
  switch (msg->op_type)
  {
    case SPI_MSG_OP_DATA:
      /* Verify data operation has valid data pointer and length */
      if ((msg->data == NULL) || (msg->data_len == 0U))
      {
        return -1;
      }

      /* Allocate buffer for data operation */
      spi_trace(SPI_TP_WRITE, "spi_write data mode\r\n");
      buf = spi_buffer_alloc(msg->data_len, sizeof(struct spi_header));
      if (buf == NULL)
      {
        return -3;
      }

      data_len = msg->data_len;
      (void)spi_port_memcpy(buf->data, msg->data, msg->data_len);
      break;

    case SPI_MSG_OP_BUFFER:
      /* Verify buffer operation has valid buffer */
      if (msg->buffer == NULL)
      {
        return -1;
      }

      /* Use the provided pre-allocated buffer */
      spi_trace(SPI_TP_WRITE, "spi_write buffer mode\r\n");
      buf = msg->buffer;
      data_len = buf->len;
      break;

    default:
      /* Types unmanaged in write mode */
      break;
  }

  /* Set traffic type for the buffer */
  spi_buffer_set_traffic_type(buf, traffic_type);

  /* Push header space */
  if (spi_buffer_push(buf, sizeof(struct spi_header)) == NULL)
  {
    spi_err("Can't push SPI buffer header\r\n");
    if (msg->op_type == SPI_MSG_OP_DATA)
    {
      spi_buffer_free(buf);
    }
    return -4;
  }

  /* Prepare timeout value */
  if (timeout_ms < 0)
  {
    ticks = portMAX_DELAY;
  }
  else
  {
    ticks = pdMS_TO_TICKS(timeout_ms);
  }

  /* Send buffer to transmission queue */
  ret = xQueueSend(xfer_engine.txq, &buf, ticks);
  if (ret != pdTRUE)
  {
    /* Free buffer on queue send failure if it is allocated */
    if (msg->op_type == SPI_MSG_OP_DATA)
    {
      spi_buffer_free(buf);
    }
    return -5;
  }

  /* Notify transfer engine about pending data */
  (void)xEventGroupSetBits(xfer_engine.event, SPI_EVT_TXN_PENDING);

  /* Return number of bytes queued for transmission */
  return (int32_t)data_len;
}

int32_t spi_bind(unsigned char type, int32_t rxq_size)
{
  /* Use default queue size if requested size is invalid */
  if (rxq_size <= 0)
  {
    rxq_size = SPI_RXQ_LEN;
  }

  if (xfer_engine.initialized == 0)
  {
    spi_err("SPI transaction is NOT initialized!\r\n");
    return -1;
  }

  if (type >= SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  {
    spi_err("Invalid traffic type: %d, max allowed: %d\r\n",
            type, (uint32_t)SPI_MSG_CTRL_TRAFFIC_TYPE_MAX - 1U);
    return -2;
  }

  /* Check if this type is already bound */
  if ((xfer_engine.rxq_bound & (1U << (uint8_t)type)) != 0U)
  {
    spi_err("Traffic type %d is already bound\r\n", type);
    return -3;
  }

  /* Create a new queue for this type with specified size */
  xfer_engine.rxq[type] = xQueueCreate((uint32_t)rxq_size, sizeof(void *));
  if (xfer_engine.rxq[type] == NULL)
  {
    spi_err("Failed to create queue for type %d\r\n", type);
    return -4;
  }

  /* Mark this type as bound */
  xfer_engine.rxq_bound |= (1U << (uint8_t)type);
  return 0;
}

int32_t spi_get_stats(struct spi_stat *stat)
{
  if (stat == NULL)
  {
    return -1;
  }

  *stat = xfer_engine.stat;
  return 0;
}

int32_t spi_rxd_callback_register(spi_msg_ctrl_t type, spi_rxd_notify_func_t cb, void *arg)
{
  if (type >= SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  {
    return -1;
  }
  xfer_engine.cb[type] = cb;
  xfer_engine.cb_arg[type] = arg;
  return 0;
}

int32_t spi_on_txn_data_ready(void)
{
  if (xfer_engine.event != NULL)
  {
    spi_trace(SPI_TP_SLAVE_TXN_RDY, "slave txn/data ready\n");
#if ((tskKERNEL_VERSION_MAJOR > 10) || ((tskKERNEL_VERSION_MAJOR == 10) && (tskKERNEL_VERSION_MINOR >= 6)))
    if (xPortIsInsideInterrupt())
#else
    if (__get_IPSR() != 0U)
#endif /* tskKERNEL_VERSION_MAJOR */
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      (void)xEventGroupSetBitsFromISR(xfer_engine.event, SPI_EVT_TXN_RDY, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      (void)xEventGroupSetBits(xfer_engine.event, SPI_EVT_TXN_RDY);
    }
  }
  return 0;
}

int32_t spi_on_header_ack(void)
{
  if (xfer_engine.event != NULL)
  {
    spi_trace(SPI_TP_HDR_ACKED, "slave header ack\n");
#if ((tskKERNEL_VERSION_MAJOR > 10) || ((tskKERNEL_VERSION_MAJOR == 10) && (tskKERNEL_VERSION_MINOR >= 6)))
    if (xPortIsInsideInterrupt())
#else
    if (__get_IPSR() != 0U)
#endif /* tskKERNEL_VERSION_MAJOR */
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      (void)xEventGroupSetBitsFromISR(xfer_engine.event, SPI_EVT_HDR_ACKED, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      (void)xEventGroupSetBits(xfer_engine.event, SPI_EVT_HDR_ACKED);
    }
  }
  return 0;
}

static void spi_on_transaction_complete(void)
{
  if (xfer_engine.event != NULL)
  {
    spi_trace(SPI_TP_NONE, "hw txn done\n");
#if ((tskKERNEL_VERSION_MAJOR > 10) || ((tskKERNEL_VERSION_MAJOR == 10) && (tskKERNEL_VERSION_MINOR >= 6)))
    if (xPortIsInsideInterrupt())
#else
    if (__get_IPSR() != 0U)
#endif /* tskKERNEL_VERSION_MAJOR */
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      (void)xEventGroupSetBitsFromISR(xfer_engine.event, SPI_EVT_HW_XFER_DONE, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      (void)xEventGroupSetBits(xfer_engine.event, SPI_EVT_HW_XFER_DONE);
    }
  }
}

/** SPI transfer state strings */
static const char *spi_state_str[4] =
{
  [SPI_XFER_STATE_IDLE] = "Idle",
  [SPI_XFER_STATE_FIRST_PART] = "First Part Transaction",
  [SPI_XFER_STATE_SECOND_PART] = "Second Part Transaction",
  [SPI_XFER_STATE_TXN_DONE] = "Transfer Complete",
};

static void num2string64(char *out, int32_t out_strlen, const uint64_t num)
{
  uint64_t temp = num;
  int32_t count = 0;

  /* Count number of digit */
  while ((temp != 0U) && (count < out_strlen))
  {
    temp = temp / 10U;
    count++;
  }

  /* Write end of string char */
  out[count--] = '\0';

  temp = num;
  while (temp != 0U)
  {
    uint8_t digit = (uint8_t)((temp % 10U) + '0');
    out[count--] = (char)digit;
    temp = temp / 10U;
  }
}

static void spi_show_stat(struct spi_stat *stat)
{
  char count_ui64[STR64BIT_DIGIT];
  num2string64(count_ui64, STR64BIT_DIGIT, stat->tx_pkts);
  LogInfo("TX                    %-10s pkts\n", count_ui64);
  num2string64(count_ui64, STR64BIT_DIGIT, stat->tx_bytes);
  LogInfo("TX                    %-10s bytes\n", count_ui64);

  num2string64(count_ui64, STR64BIT_DIGIT, stat->rx_pkts);
  LogInfo("RX                    %-10s pkts\n", count_ui64);
  num2string64(count_ui64, STR64BIT_DIGIT, stat->rx_bytes);
  LogInfo("RX                    %-10s bytes\n", count_ui64);

  num2string64(count_ui64, STR64BIT_DIGIT, stat->rx_drop);
  LogInfo("drop                  %s\n", count_ui64);
  num2string64(count_ui64, STR64BIT_DIGIT, stat->mem_err);
  LogInfo("mem_err               %s\n", count_ui64);
  num2string64(count_ui64, STR64BIT_DIGIT, stat->rx_stall);
  LogInfo("stall                 %s\n", count_ui64);

  num2string64(count_ui64, STR64BIT_DIGIT, stat->io_err);
  LogInfo("IO error              %s\n", count_ui64);
  num2string64(count_ui64, STR64BIT_DIGIT, stat->hdr_err);
  LogInfo("header error          %s\n", count_ui64);
  num2string64(count_ui64, STR64BIT_DIGIT, stat->wait_txn_timeouts);
  LogInfo("wait_txn_timeout      %s\n", count_ui64);
  num2string64(count_ui64, STR64BIT_DIGIT, stat->wait_hdr_ack_timeouts);
  LogInfo("wait_hdr_ack_timeout  %s\n", count_ui64);
}

void spi_dump(void)
{
  EventBits_t bits;
  char pending_events[64] = {0};
  uint32_t pos = 0;

  if (xfer_engine.initialized == 0)
  {
    spi_err("spi transaction is NOT initialized!\n");
    return;
  }

  LogInfo("Master transfer state %s\n", spi_state_str[xfer_engine.state]);

  bits = xEventGroupGetBits(xfer_engine.event);
  if ((bits & SPI_EVT_TXN_RDY) != 0U)
  {
    pos = (uint32_t)snprintf(pending_events, sizeof(pending_events), ", TXN Ready");
  }
  if ((bits & SPI_EVT_TXN_PENDING) != 0U)
  {
    pos += (uint32_t)snprintf(pending_events + pos, sizeof(pending_events) - pos, ", TXN Pending");
  }
  if ((bits & SPI_EVT_HW_XFER_DONE) != 0U)
  {
    (void)snprintf(pending_events + pos, sizeof(pending_events) - pos, ", Hardware Xfer done");
  }
  LogInfo("SPI pending events    %" PRIu32 " %s\n", bits, pending_events);

  LogInfo("Slave data ready pin  %s\n", (spi_port_is_ready() == 1) ? "High" : "Low");

  spi_show_stat(&xfer_engine.stat);
  LogInfo("TX queue items        %" PRIu32 "\n", uxQueueMessagesWaiting(xfer_engine.txq));

  for (uint32_t i = 0; i < (uint32_t)SPI_MSG_CTRL_TRAFFIC_TYPE_MAX; i++)
  {
    if ((xfer_engine.rxq_bound & (1U << i)) != 0U)
    {
      LogInfo("RX queue[%" PRIu32 "] items     %" PRIu32 "\n", i,
              uxQueueMessagesWaiting(xfer_engine.rxq[i]));
    }
  }
}
