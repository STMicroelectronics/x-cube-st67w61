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

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

/* For redefinitions purposes*/
#include "w61_driver_config.h"

#include "spi_iface.h"
#include "spi_port.h"

#define SPI_HEADER_MAGIC_CODE 0x55AA

#ifndef SPI_THREAD_STACK_SIZE
/** SPI thread stack size */
#define SPI_THREAD_STACK_SIZE 768
#endif /* SPI_THREAD_STACK_SIZE */

#ifndef SPI_THREAD_PRIO
/** SPI thread priority */
#define SPI_THREAD_PRIO       53
#endif /* SPI_THREAD_PRIO */

struct spi_header
{
  uint16_t magic;
  uint16_t len;
  uint8_t version : 2;
  /* Peer RX is stalled. */
  uint8_t rx_stall : 1;
  uint8_t flags : 5;
  uint8_t type;
  uint16_t rsvd;
} __attribute__((packed));

#define SPI_HEADER_INIT(h, _type, _len) do {                                     \
                                             (h)->magic = SPI_HEADER_MAGIC_CODE; \
                                             (h)->type = _type;                  \
                                             (h)->version = 0;                   \
                                             (h)->rx_stall = 0;                  \
                                             (h)->flags = 0;                     \
                                             (h)->len = _len;                    \
                                             (h)->rsvd = 0;                      \
                                           } while (0)

/* SPI transfer state. */
enum
{
  SPI_XFER_STATE_IDLE,
  SPI_XFER_STATE_FIRST_PART,
  SPI_XFER_STATE_SECOND_PART,
  SPI_XFER_STATE_TXN_DONE,
};

enum
{
  SPI_XFER_F_SKIP_FIRST_TXN_WAIT = 0x1,
};

#define SPI_STAT_INC(stat, mb, val) (stat)->mb += (val)

#define SPI_TXQ_LEN 8
#define SPI_RXQ_LEN 8

/**
  * SPI transfer engine structure
  *
  * This structure manages the state and resources for SPI communication,
  * including support for multiple receive queues bound to different message types.
  */
struct spi_xfer_engine
{
  /* Transfer task handle */
  TaskHandle_t task;
  /* Stop flag for clean shutdown */
  int32_t stop;
  /* Initialization flag */
  int32_t initialized;
  /* Event flags for synchronization */
  EventGroupHandle_t event;
  /* Current transfer state */
  int32_t state;
  /* Slave RX is stalled */
  int32_t rx_stall;
  /* Single transmit queue */
  QueueHandle_t txq;
  /* Array of receive queues by type */
  QueueHandle_t rxq[SPI_MSG_CTRL_TRAFFIC_TYPE_MAX];
  /* Bitmap of bound traffic types */
  uint8_t rxq_bound;
  /* Current transmit buffer */
  struct spi_buffer *txbuf;
  /* Transfer statistics */
  struct spi_stat stat;
};

static struct spi_xfer_engine xfer_engine = {0};
#define SPI_BUF_ALIGN_MASK  (0x3)

/* SPI transfer trace points. */
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

static void spi_on_transaction_complete(void);

static inline void spi_buffer_set_traffic_type(struct spi_buffer *buf, uint8_t type)
{
  buf->cb[0] = type;
}

static inline uint8_t spi_buffer_get_traffic_type(struct spi_buffer *buf)
{
  return buf->cb[0];
}

/* For prepending data like header. */
static inline void *spi_buffer_push(struct spi_buffer *buf, uint64_t size)
{
  char *p, *start, *data;

  if (!buf)
  {
    return NULL;
  }

  if (!size)
  {
    return buf->data;
  }

  p = (char *)buf;
  data = buf->data;
  start = p + sizeof(struct spi_buffer);
  if (data - size < start)
  {
    spi_err("Illegal try of spi buffer push!\n");
    return NULL;
  }

  buf->data = data - size;
  buf->len += size;
  return buf->data;
}

/* For removing leading data like header. */
static inline void *spi_buffer_pull(struct spi_buffer *buf, uint64_t size)
{
  char *p, *end, *data;

  if (!buf)
  {
    return NULL;
  }

  if (!size)
  {
    return buf->data;
  }

  p = (char *)buf;
  data = buf->data;
  end = p + sizeof(struct spi_buffer) + buf->cap;
  if (data + size > end)
  {
    spi_err("Illegal try of spi buffer pull\n");
    return NULL;
  }

  buf->data = data + size;
  buf->len -= size;
  return buf->data;
}

struct spi_buffer *spi_buffer_alloc(uint32_t size, uint32_t reserve)
{
  uint32_t cap;
  uint32_t extra;
  struct spi_buffer *buf;
  int32_t desc_size = sizeof(struct spi_buffer);

  cap = (size + reserve + SPI_BUF_ALIGN_MASK) & ~SPI_BUF_ALIGN_MASK;
  buf = pvPortMalloc(cap + desc_size);
  if (!buf)
  {
    return NULL;
  }

  buf->flags = 0;
  buf->len = size;
  buf->cap = cap;
  buf->data = (char *)buf + desc_size + reserve;
  memset(buf->cb, 0, sizeof(buf->cb));
  /* Fill in the debug pattern. */
  extra = cap - size - reserve;
  while (extra)
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

  if (engine->txbuf)
  {
    return engine->txbuf;
  }

  ret = xQueueReceive(engine->txq, &buf, 0);
  if (ret != pdTRUE)
  {
    buf = NULL;
  }

  engine->txbuf = buf;
  if (buf)
  {
    SPI_STAT_INC(&engine->stat, tx_pkts, 1);
    SPI_STAT_INC(&engine->stat, tx_bytes, buf->len);
  }
  return buf;
}

/* return 1 if the header is valid. */
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

int32_t spi_wait_event(void *evnt_ctx, uint32_t event, int32_t timeout_ms)
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
  if (bits & event)
  {
    return 1;
  }

  return 0;
}

static int32_t inline spi_clear_event(struct spi_xfer_engine *engine, uint32_t event)
{
  xEventGroupClearBits(engine->event, event);
  return 0;
}

static int32_t spi_txrx(struct spi_xfer_engine *engine, void *tx_buf, void *rx_buf, uint16_t len)
{
  int32_t status;

  if (!engine || !tx_buf || !rx_buf)
  {
    return -1;
  }

  if (!len)
  {
    return 0;
  }

  if (len <= SPI_DMA_XFER_SIZE_THRESHOLD)
  {
    status = spi_port_transfer(tx_buf, rx_buf, len, SPI_WAIT_POLL_XFER_TIMEOUT_MS);
    if (status < 0)
    {
      spi_err("spi txrx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1);
      return -2;
    }
  }
  else
  {
    status = spi_port_transfer_dma(tx_buf, rx_buf, len);
    if (status < 0)
    {
      spi_err("spi txrx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1);
      return -2;
    }
    /* Wait for spi transaction completion. */
    if (!spi_wait_event(engine, SPI_EVT_HW_XFER_DONE, SPI_WAIT_MSG_XFER_TIMEOUT_MS))
    {
      spi_err("spi txrx transaction timeouted\n");
      SPI_STAT_INC(&engine->stat, wait_msg_xfer_timeouts, 1);
      return -3;
    }
  }

  return 0;
}

static int32_t spi_rx(struct spi_xfer_engine *engine, void *rx_buf, uint16_t len)
{
  int32_t status;

  if (!engine || !rx_buf)
  {
    return -1;
  }

  if (!len)
  {
    return 0;
  }

  if (len <= SPI_DMA_XFER_SIZE_THRESHOLD)
  {
    status = spi_port_transfer(NULL, rx_buf, len, SPI_WAIT_POLL_XFER_TIMEOUT_MS);
    if (status < 0)
    {
      spi_err("spi rx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1);
      return -2;
    }
  }
  else
  {
    status = spi_port_transfer_dma(NULL, rx_buf, len);
    if (status < 0)
    {
      spi_err("spi rx failed, %" PRIi32 "\n", status);
      SPI_STAT_INC(&engine->stat, io_err, 1);
      return -2;
    }
    /* Wait for spi transaction completion. */
    if (!spi_wait_event(engine, SPI_EVT_HW_XFER_DONE, SPI_WAIT_MSG_XFER_TIMEOUT_MS))
    {
      spi_err("spi rx transaction timeouted\n");
      SPI_STAT_INC(&engine->stat, wait_msg_xfer_timeouts, 1);
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
  struct spi_header mh_t;
  struct spi_header *pmh;
  struct spi_header *psh;
  struct spi_buffer *rxbuf = NULL;
  int32_t rx_restore = 0;

  spi_trace(SPI_TP_NONE, "wait_txn_rdy %" PRIi32 "\n", wait_txn_rdy);
  engine->state = SPI_XFER_STATE_FIRST_PART;
  if (wait_txn_rdy &&
      !spi_wait_event(engine, SPI_EVT_TXN_RDY, SPI_WAIT_TXN_TIMEOUT_MS))
  {
    SPI_STAT_INC(&engine->stat, wait_txn_timeouts, 1);
    spi_err("waiting for spi txn ready timeouted\n");
    return -1;
  }

  /* Re-initialize events in case of pending ones. */
  spi_clear_event(engine, SPI_EVT_HW_XFER_DONE | SPI_EVT_HDR_ACKED);

  /* Yes, this allocation will be wasted if there is no data from slave. */
  rxbuf = spi_buffer_alloc(SPI_XFER_MTU_BYTES + sizeof(struct spi_header), 0);
  if (!rxbuf)
  {
    spi_err("No mem for rxbuf\n");
    SPI_STAT_INC(&engine->stat, mem_err, 1);
    return -1;
  }

  spi_trace(SPI_TP_FIRST_TXN_START, "start the first transaction\n");
  if (txbuf)
  {
    uint16_t msglen;
    uint8_t type = spi_buffer_get_traffic_type(txbuf);

    if (!engine->rx_stall)
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
      SPI_HEADER_INIT(&mh_t, 0, 0);
      txp = &mh_t;
      xfer_size = sizeof(struct spi_header);
    }
  }
  else
  {
    /* Initialize master header. */
    SPI_HEADER_INIT(&mh_t, 0, 0);
    txp = &mh_t;
    xfer_size = sizeof(struct spi_header);
  }

  if (spi_txrx(engine, txp, rxbuf->data, xfer_size))
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
  if (!ret)
  {
    spi_err("Invalid spi header from peer, magic 0x%" PRIx16 ", len (%" PRIu16 ", 0x%" PRIx16 "), version %" PRIu16
            ", type %" PRIx16 "flags 0x%" PRIx16 ", rsvd 0x%" PRIx16 "\n",
            psh->magic, psh->len, psh->len, psh->version, psh->type, psh->flags, psh->rsvd);
    SPI_STAT_INC(&engine->stat, hdr_err, 1);
    goto out;
  }

  if (!engine->rx_stall)
  {
    if (psh->rx_stall)
    {
      SPI_STAT_INC(&engine->stat, rx_stall, 1);
      spi_trace(SPI_TP_NONE, "Slave RX stalled\n");
    }
  }
  else if (!psh->rx_stall)
  {
    rx_restore = 1;
  }
  engine->rx_stall = psh->rx_stall;

  /* Receive the remaining data from slave if any. */
  if (psh->len + sizeof(struct spi_header) > xfer_size)
  {
    uint8_t *rxp = (uint8_t *)rxbuf->data;
    uint16_t remain = psh->len + sizeof(struct spi_header) - xfer_size;

    spi_trace(SPI_TP_SECOND_TXN_START, "Receiving remaining data\n");
    engine->state = SPI_XFER_STATE_SECOND_PART;
    remain = (remain + SPI_BUF_ALIGN_MASK) & ~SPI_BUF_ALIGN_MASK;
    rxp += xfer_size;
    if (spi_rx(engine, rxp, remain))
    {
      spi_err("Failed to receive the remaining bytes\n");
      goto out;
    }
    spi_trace(SPI_TP_SECOND_TXN_END, "Remaining transfer completed\n");
  }

  /* free txbuf after the message transaction. */
  if (txbuf)
  {
    if (!engine->rx_stall && !rx_restore)
    {
      engine->txbuf = NULL;
      spi_buffer_free(txbuf);
    }
  }

  if (psh->len)
  {
    SPI_STAT_INC(&engine->stat, rx_pkts, 1);
    SPI_STAT_INC(&engine->stat, rx_bytes, psh->len);
    spi_buffer_pull(rxbuf, sizeof(struct spi_header));
    /* rx buffer length fix-up */
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
    if (msg_type < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX &&
        (engine->rxq_bound & (1 << msg_type)))
    {
      if (psh->len < 256)
      {
        struct spi_buffer *rxbuf_resized = spi_buffer_alloc(psh->len, 0);
        if (!rxbuf_resized)
        {
          spi_err("No mem for rxbuf\n");
          SPI_STAT_INC(&engine->stat, mem_err, 1);
        }
        else
        {
          rxbuf_resized->flags = rxbuf->flags;
          memcpy(rxbuf_resized->data, rxbuf->data, psh->len);
          spi_buffer_free(rxbuf);
          rxbuf = rxbuf_resized;
        }
      }

      ret = xQueueSend(engine->rxq[msg_type], &rxbuf, portMAX_DELAY);
      if (ret != pdTRUE)
      {
        spi_trace(SPI_TP_NONE, "failed to send to type %d rxq, msg discarded\r\n", msg_type);
        spi_buffer_free(rxbuf);
        SPI_STAT_INC(&engine->stat, rx_drop, 1);
      }
    }
    else
    {
      /* No queue bound for this type, discard message */
      spi_trace(SPI_TP_NONE, "No queue bound for type %d, msg discarded\r\n", msg_type);
      spi_buffer_free(rxbuf);
      SPI_STAT_INC(&engine->stat, rx_drop, 1);
    }
  }
  else if (rxbuf)
  {
    spi_buffer_free(rxbuf);
  }

  /* Wait until slave acknowledged header. */
  spi_trace(SPI_TP_WAIT_HDR_ACK_START, "waiting for header ack\n");
  while (!spi_wait_event(engine, SPI_EVT_HDR_ACKED, SPI_WAIT_HDR_ACK_TIMEOUT_MS))
  {
    if (!spi_port_is_ready())
    {
      spi_dbg("Since the slave txn/data pin is already low, did we miss that interrupt?\n");
      break;
    }

    spi_dbg("wait header ack timeouted\n");
    SPI_STAT_INC(&engine->stat, wait_hdr_ack_timeouts, 1);
  }
  spi_trace(SPI_TP_WAIT_HDR_ACK_END, "Got header ack\n");

  return 0;

out:
  if (rxbuf)
  {
    spi_buffer_free(rxbuf);
  }
  return err;
}

static int32_t spi_do_xfer(struct spi_xfer_engine *engine, int32_t flags)
{
  struct spi_buffer *txbuf;
  int32_t rx_pending;
  int32_t wait_txn_rdy;

  while (1)
  {
    /* Get txbuf */
    txbuf = spi_get_txbuf(engine);

    /* Read data ready GPIO level */
    rx_pending = spi_port_is_ready();

    spi_trace(SPI_TP_NONE, "txbuf %" PRIuPTR ", rx_pending %" PRIi32 "\n", txbuf, rx_pending);
    /* Check if txbuf is valid or data is ready for transfer */
    if (txbuf || rx_pending == 1)
    {
      /* Assert chip select */
      spi_trace(SPI_TP_ASSERT_CS, "Assert CS\n");
      spi_port_set_cs(1);

      /* Transfer one packet */
      if (flags & SPI_XFER_F_SKIP_FIRST_TXN_WAIT)
      {
        flags &= ~SPI_XFER_F_SKIP_FIRST_TXN_WAIT;
        wait_txn_rdy = 0;
      }
      else
      {
        wait_txn_rdy = 1;
      }

      spi_xfer_one(engine, txbuf, wait_txn_rdy);

      /* De-assert chip select */
      spi_trace(SPI_TP_DEASSERT_CS, "Deassert CS\n");
      spi_port_set_cs(0);
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

static void spi_show_stat(struct spi_stat *stat)
{
  spi_dbg("tx %" PRIu64 " pkts, %" PRIu64 " bytes, rx %" PRIu64 " pkts, %" PRIu64 " bytes, drop %" PRIu64
          " pkts, mem_err %" PRIu64 ", stall %" PRIu64 "\n",
          stat->tx_pkts, stat->tx_bytes, stat->rx_pkts, stat->rx_bytes, stat->rx_drop, stat->mem_err,
          stat->rx_stall);
  spi_dbg("IO error %" PRIu64 ", header error %" PRIu64 ", wait_txn_timeout %" PRIu64
          ", wait_msg_xfer_timeouts %" PRIu64 ", wait_hdr_ack_timeout %" PRIu64 "\n",
          stat->io_err, stat->hdr_err, stat->wait_txn_timeouts, stat->wait_msg_xfer_timeouts,
          stat->wait_hdr_ack_timeouts);
}

static void spi_xfer_engine_task(void *arg)
{
  EventBits_t bits;
  struct spi_xfer_engine *engine = arg;

  /* Wait for events and process them. */
  while (!engine->stop)
  {
    engine->state = SPI_XFER_STATE_IDLE;
    bits = SPI_EVT_TXN_PENDING | SPI_EVT_TXN_RDY;
    bits = xEventGroupWaitBits(engine->event, bits, pdTRUE, pdFALSE,
                               portMAX_DELAY);
    spi_trace(SPI_TP_NONE, "Got event bits %" PRIx32 "\n", bits);
    if (bits & SPI_EVT_TXN_RDY)
    {
      spi_do_xfer(engine, SPI_XFER_F_SKIP_FIRST_TXN_WAIT);
    }
    else if (bits & SPI_EVT_TXN_PENDING)
    {
      spi_do_xfer(engine, 0);
    }
  }
}

int32_t spi_transaction_init(void)
{
  int32_t ret;

  /* Initialize rxq array to NULL and rxq_bound to 0 */
  memset(xfer_engine.rxq, 0, sizeof(xfer_engine.rxq));
  xfer_engine.rxq_bound = 0;

  /* Create event group for SPI transaction */
  xfer_engine.event = xEventGroupCreate();
  if (!xfer_engine.event)
  {
    spi_err("Failed to create event group for SPI\n");
    ret = -1;
    goto error;
  }

  /* Create TX queue */
  xfer_engine.txq = xQueueCreate(SPI_TXQ_LEN, sizeof(void *));
  if (!xfer_engine.txq)
  {
    spi_err("Failed to create txq\n");
    ret = -1;
    goto error;
  }

  spi_port_init(spi_on_transaction_complete);

  /* Create SPI transfer engine task */
  xfer_engine.stop = 0;
  xTaskCreate(spi_xfer_engine_task, "spi_xfer_engine", SPI_THREAD_STACK_SIZE >> 2,
              &xfer_engine, SPI_THREAD_PRIO, &xfer_engine.task);

  if (!xfer_engine.task)
  {
    spi_err("Failed to create spi xfer engine task\n");
    ret = -1;
    goto error;
  }

  /* Check the state of the slave data ready pin */
  if (spi_port_is_ready() == 1)
  {
    /* Set the TXN event if data is ready */
    xEventGroupSetBits(xfer_engine.event, SPI_EVT_TXN_RDY);
  }

  xfer_engine.rx_stall = 0;
  xfer_engine.initialized = 1;
  spi_log("SPI transaction initialized\n");
  return 0;

error:
  /* Clean up resources in case of error */
  if (xfer_engine.txq)
  {
    vQueueDelete(xfer_engine.txq);
    xfer_engine.txq = NULL;
  }

  if (xfer_engine.event)
  {
    vEventGroupDelete(xfer_engine.event);
    xfer_engine.event = NULL;
  }

  return ret;
}

int32_t spi_transaction_deinit(void)
{
  /* Clean up resources in case of error */
  if (xfer_engine.txq)
  {
    vQueueDelete(xfer_engine.txq);
  }

  if (xfer_engine.event)
  {
    vEventGroupDelete(xfer_engine.event);
  }

  for (int32_t i = 0; i < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX; i++)
  {
    if (xfer_engine.rxq[i])
    {
      vQueueDelete(xfer_engine.rxq[i]);
    }
  }

  /* clear xfer_engine context*/
  memset(&xfer_engine, 0, sizeof(struct spi_xfer_engine));

  spi_port_deinit();

  return 0;
}

/**
  * Read data from the SPI interface
  *
  * This function handles both raw data and buffer pointer operations based on the
  * message operation type (op_type) field.
  *
  * @param msg         Message structure for receiving data
  * @param timeout_ms  Timeout for queue operations in milliseconds (-1 for infinite)
  * @return            Number of bytes read or negative error code
  */
int32_t spi_read(struct spi_msg *msg, int32_t timeout_ms)
{
  BaseType_t ret;
  TickType_t ticks;
  struct spi_buffer *buf = NULL;
  uint8_t traffic_type = SPI_MSG_CTRL_TRAFFIC_AT_CMD; /* Default to AT commands */
  QueueHandle_t targetQ;

  /* Verify message is valid and operation type is supported */
  if (!msg || (msg->op_type != SPI_MSG_OP_DATA && msg->op_type != SPI_MSG_OP_BUFFER_PTR))
  {
    return -1;
  }

  /* Verify appropriate fields based on operation type */
  if (msg->op_type == SPI_MSG_OP_DATA)
  {
    if (!msg->data || !msg->data_len)
    {
      return -1;
    }
  }
  else if (msg->op_type == SPI_MSG_OP_BUFFER_PTR)
  {
    if (!msg->buffer_ptr)
    {
      return -1;
    }
  }

  /* Verify context is initialized */
  if (!xfer_engine.initialized)
  {
    spi_err("SPI transaction is NOT initialized!\n");
    return -2;
  }

  /* Process control information if present */
  if (msg->ctrl)
  {
    /* Check for traffic type control information */
    if (msg->ctrl->type == SPI_MSG_CTRL_TRAFFIC_TYPE)
    {
      /* Validate control data length */
      if (msg->ctrl->len == SPI_MSG_CTRL_TRAFFIC_TYPE_LEN && msg->ctrl->val)
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
  if (traffic_type >= SPI_MSG_CTRL_TRAFFIC_TYPE_MAX ||
      !(xfer_engine.rxq_bound & (1 << traffic_type)))
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
    ticks = pdMS_TO_TICKS(timeout_ms);

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
      memcpy(msg->data, buf->data, msg->data_len);
      spi_buffer_free(buf);
      return msg->data_len;

    case SPI_MSG_OP_BUFFER_PTR:
      /* Receive buffer pointer directly into msg->buffer_ptr */
      ret = xQueueReceive(targetQ, msg->buffer_ptr, ticks);
      if (ret != pdTRUE)
      {
        return -4;
      }

      return (*(msg->buffer_ptr))->len;
  }

  /* Should never reach here, but just in case */
  return -1;
}

/**
  * Write data to the SPI interface
  *
  * This function handles both raw data and pre-allocated buffers based on the
  * message operation type (op_type) field.
  *
  * @param msg         Message containing data or buffer to send
  * @param timeout_ms  Timeout for queue operations in milliseconds (-1 for infinite)
  * @return            Number of bytes written or negative error code
  */
int32_t spi_write(struct spi_msg *msg, int32_t timeout_ms)
{
  BaseType_t ret;
  struct spi_buffer *buf;
  TickType_t ticks;
  uint8_t traffic_type = SPI_MSG_CTRL_TRAFFIC_AT_CMD;
  int32_t data_len = 0;

  /* Verify message is valid and operation type is supported */
  if (!msg || (msg->op_type != SPI_MSG_OP_DATA && msg->op_type != SPI_MSG_OP_BUFFER))
  {
    return -1;
  }

  /* Verify context is initialized */
  if (!xfer_engine.initialized)
  {
    spi_err("SPI transaction is NOT initialized!\r\n");
    return -2;
  }

  /* Process control information if present */
  if (msg->ctrl)
  {
    /* Check for traffic type control information */
    if (msg->ctrl->type == SPI_MSG_CTRL_TRAFFIC_TYPE)
    {
      /* Validate control data length */
      if (msg->ctrl->len == SPI_MSG_CTRL_TRAFFIC_TYPE_LEN && msg->ctrl->val)
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

  /* Process based on operation type */
  switch (msg->op_type)
  {
    case SPI_MSG_OP_DATA:
      /* Verify data operation has valid data pointer and length */
      if (!msg->data || !msg->data_len)
      {
        return -1;
      }

      /* Allocate buffer for data operation */
      spi_trace(SPI_TP_WRITE, "spi_write data mode\r\n");
      buf = spi_buffer_alloc(msg->data_len, sizeof(struct spi_header));
      if (!buf)
      {
        return -3;
      }

      data_len = msg->data_len;
      memcpy(buf->data, msg->data, msg->data_len);
      break;

    case SPI_MSG_OP_BUFFER:
      /* Verify buffer operation has valid buffer */
      if (!msg->buffer)
      {
        return -1;
      }

      /* Use the provided pre-allocated buffer */
      spi_trace(SPI_TP_WRITE, "spi_write buffer mode\r\n");
      buf = msg->buffer;
      data_len = buf->len;
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
    ticks = pdMS_TO_TICKS(timeout_ms);

  /* Send buffer to transmission queue */
  ret = xQueueSend(xfer_engine.txq, &buf, ticks);
  if (ret != pdTRUE)
  {
    /* Free buffer on queue send failure if we allocated it */
    if (msg->op_type == SPI_MSG_OP_DATA)
    {
      spi_buffer_free(buf);
    }
    return -5;
  }

  /* Notify transfer engine about pending data */
  xEventGroupSetBits(xfer_engine.event, SPI_EVT_TXN_PENDING);

  /* Return number of bytes queued for transmission */
  return data_len;
}

/**
  * Bind a specific traffic type to a dedicated receive queue
  *
  * @param type      Traffic type to bind (must be < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  * @param rxq_size  Size of receive queue to create (0 for default)
  * @return          0 on success, negative value on error
  *
  * @note This function should be called during initialization before
  * any SPI transfers start, as it's not fully thread-safe. Multiple
  * concurrent binds or binding while transfers are in progress could
  * lead to race conditions.
  */
int32_t spi_bind(unsigned char type, int32_t rxq_size)
{
  /* Use default queue size if requested size is invalid */
  if (rxq_size <= 0)
  {
    rxq_size = SPI_RXQ_LEN;
  }

  if (!xfer_engine.initialized)
  {
    spi_err("SPI transaction is NOT initialized!\r\n");
    return -1;
  }

  if (type >= SPI_MSG_CTRL_TRAFFIC_TYPE_MAX)
  {
    spi_err("Invalid traffic type: %d, max allowed: %d\r\n",
            type, SPI_MSG_CTRL_TRAFFIC_TYPE_MAX - 1);
    return -2;
  }

  /* Check if this type is already bound */
  if (xfer_engine.rxq_bound & (1 << type))
  {
    spi_err("Traffic type %d is already bound\r\n", type);
    return -3;
  }

  /* Create a new queue for this type with specified size */
  xfer_engine.rxq[type] = xQueueCreate(rxq_size, sizeof(void *));
  if (!xfer_engine.rxq[type])
  {
    spi_err("Failed to create queue for type %d\r\n", type);
    return -4;
  }

  /* Mark this type as bound */
  xfer_engine.rxq_bound |= (1 << type);
  return 0;
}

int32_t spi_get_stats(struct spi_stat *stat)
{
  if (!stat)
  {
    return -1;
  }

  *stat = xfer_engine.stat;
  return 0;
}

int32_t spi_on_txn_data_ready(void)
{
  if (xfer_engine.event != NULL)
  {
    spi_trace(SPI_TP_SLAVE_TXN_RDY, "slave txn/data ready\n");
    if (xPortIsInsideInterrupt())
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xEventGroupSetBitsFromISR(xfer_engine.event, SPI_EVT_TXN_RDY, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      xEventGroupSetBits(xfer_engine.event, SPI_EVT_TXN_RDY);
    }
  }
  return 0;
}

int32_t spi_on_header_ack(void)
{
  if (xfer_engine.event != NULL)
  {
    spi_trace(SPI_TP_HDR_ACKED, "slave header ack\n");
    if (xPortIsInsideInterrupt())
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xEventGroupSetBitsFromISR(xfer_engine.event, SPI_EVT_HDR_ACKED, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      xEventGroupSetBits(xfer_engine.event, SPI_EVT_HDR_ACKED);
    }
  }
  return 0;
}

static void spi_on_transaction_complete(void)
{
  if (xfer_engine.event != NULL)
  {
    spi_trace(SPI_TP_NONE, "hw txn done\n");
    if (xPortIsInsideInterrupt())
    {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xEventGroupSetBitsFromISR(xfer_engine.event, SPI_EVT_HW_XFER_DONE, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
      xEventGroupSetBits(xfer_engine.event, SPI_EVT_HW_XFER_DONE);
    }
  }
}

#if (SPI_PORT_DEBUG_ENABLE == 1)
static const char *spi_state_str[] =
{
  [SPI_XFER_STATE_IDLE] = "Idle",
  [SPI_XFER_STATE_FIRST_PART] = "First Part Transaction",
  [SPI_XFER_STATE_SECOND_PART] = "Second Part Transaction",
  [SPI_XFER_STATE_TXN_DONE] = "Transfer Complete",
};
#endif /* SPI_PORT_DEBUG_ENABLE */

void spi_dump(void)
{
  EventBits_t bits;
  char pending_events[64] = {0};
  uint32_t pos = 0;

  if (!xfer_engine.initialized)
  {
    spi_err("spi transaction is NOT initialized!\n");
    return;
  }

  spi_dbg("Master transfer state %s\n", spi_state_str[xfer_engine.state]);

  bits = xEventGroupGetBits(xfer_engine.event);
  spi_dbg("SPI pending events %lu ", bits);
  if (bits & SPI_EVT_TXN_RDY)
  {
    pos = snprintf(pending_events, sizeof(pending_events), ", TXN Ready");
  }
  if (bits & SPI_EVT_TXN_PENDING)
  {
    pos += snprintf(pending_events + pos, sizeof(pending_events) - pos, ", TXN Pending");
  }
  if (bits & SPI_EVT_HW_XFER_DONE)
  {
    snprintf(pending_events + pos, sizeof(pending_events) - pos, ", Hardware Xfer done");
  }
  spi_dbg("SPI pending events %" PRIu32 " %s\n", bits, pending_events);

  spi_dbg("Slave data ready pin %s\n", spi_port_is_ready() == 1 ? "High" : "Low");

  spi_show_stat(&xfer_engine.stat);

  spi_dbg("Number of queue items, TX %" PRIi32 "\n",
          uxQueueMessagesWaiting(xfer_engine.txq));

  for (int32_t i = 0; i < SPI_MSG_CTRL_TRAFFIC_TYPE_MAX; i++)
  {
    if (xfer_engine.rxq_bound & (1 << i))
    {
      spi_dbg("  RX type %d: %lu items\r\n", i,
              uxQueueMessagesWaiting(xfer_engine.rxq[i]));
    }
  }
}
