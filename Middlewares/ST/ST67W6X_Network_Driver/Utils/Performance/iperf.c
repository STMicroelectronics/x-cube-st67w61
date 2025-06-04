/**
  ******************************************************************************
  * @file    iperf.c
  * @author  GPM Application Team
  * @brief   Iperf implementation
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
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "iperf.h"
#include "errno.h"
#include "w6x_api.h"

/* Private typedef -----------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Iperf_Types
  * @{
  */

/**
  * @brief  Iperf configuration structure definition
  */
typedef struct
{
  iperf_cfg_t cfg;        /*!< Iperf configuration */
  bool finish;            /*!< Iperf execution done status */
  uint64_t actual_len;    /*!< Actual length of data */
  uint64_t tot_len;       /*!< Total length of data */
  uint32_t buffer_len;    /*!< Buffer length */
  uint8_t *buffer;        /*!< Buffer */
  uint32_t sockfd;        /*!< Socket file descriptor */
  uint32_t ps_mode;       /*!< Low power mode */
  uint32_t dtim;          /*!< DTIM */
} iperf_ctrl_t;

/**
  * @brief  Iperf time structure definition
  */
typedef struct
{
  long sec;               /*!< Seconds part of timestamp */
  long usec;              /*!< Microseconds part of timestamp */
} iperf_time_struct_t;

/**
  * @brief  Iperf client header structure definition
  */
typedef struct
{
  int32_t flags;          /*!< Flags */
  int32_t numThreads;     /*!< Number of threads */
  int32_t mPort;          /*!< Port */
  int32_t bufferlen;      /*!< Buffer length */
  int32_t mWindowSize;    /*!< Window size */
  int32_t mAmount;        /*!< Amount */
  int32_t mRate;          /*!< Rate */
  int32_t mUDPRateUnits;  /*!< UDP rate units */
  int32_t mRealtime;      /*!< Realtime */
} iperf_client_hdr_t;

/**
  * @brief  Iperf server header structure definition
  */
typedef struct
{
  int32_t flags;          /*!< Flags */
  int32_t total_len1;     /*!< Total length MSB part */
  int32_t total_len2;     /*!< Total length LSB part */
  int32_t stop_sec;       /*!< Stop seconds */
  int32_t stop_usec;      /*!< Stop microseconds */
  int32_t error_cnt;      /*!< Error count */
  int32_t outorder_cnt;   /*!< Out of order count */
  int32_t datagrams;      /*!< Number of datagrams including errors */
  int32_t jitter1;        /*!< Jitter seconds part */
  int32_t jitter2;        /*!< Jitter microseconds part */
} iperf_server_hdr_t;

/**
  * @brief  Iperf transfer information structure definition
  */
typedef struct
{
  void *reserved_delay;   /*!< Reserved for delay */
  int32_t transferID;     /*!< Transfer ID */
  int32_t groupID;        /*!< Group ID */
  int32_t cntError;       /*!< Error count */
  int32_t cntOutofOrder;  /*!< Out of order count */
  int32_t cntDatagrams;   /*!< Number of datagrams */
  uint64_t TotalLen;      /*!< Total length */
  int32_t jitter;          /*!< Jitter */
  int32_t startTime;       /*!< Start time */
  int32_t endTime;         /*!< End time */
  char   mFormat;         /*!< Format */
  unsigned char mTTL;     /*!< Time to live */
  char   mUDP;            /*!< UDP */
  char   free;            /*!< Free */
} Transfer_Info_t;

/**
  * @brief  Iperf UDP datagram structure definition
  */
typedef struct
{
  int32_t id;             /*!< Datagram ID */
  uint32_t tv_sec;        /*!< Seconds part of timestamp */
  uint32_t tv_usec;       /*!< Microseconds part of timestamp */
  int32_t id2;            /*!< Datagram ID 2 */
} UDP_datagram_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Iperf_Constants
  * @{
  */

#define iperf_err_t           int32_t     /*!< Iperf error type definition */

#define IPERF_OK              0           /*!< iperf_err_t value indicating success (no error) */

#define IPERF_FAIL            -1          /*!< Generic iperf_err_t code indicating failure */

#define RECV_DUAL_BUF_LEN     (4 * 1024)  /*!< Receive buffer length for dual test */

#define LAST_SOCKET_TIMEOUT   1000        /*!< Last socket timeout */

#define TCP_RX_SOCKET_TIMEOUT 1000        /*!< TCP receive socket timeout */

#define PERCENT_MULTIPLIER    1000        /*!< Percentage multiplier for summary */

#define HEADER_VERSION1       0x80000000  /*!< Header version 1 mask */

#define RUN_NOW               0x00000001  /*!< Run now mask */

#define JITTER_RX             0           /*!< Enable jitter calculation */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Iperf_Variables ST67W6X Utility Performance Iperf Variables
  * @ingroup  ST67W6X_Utilities_Performance_Iperf
  * @{
  */

/** Iperf is running status */
static bool s_iperf_is_running = false;

/** Iperf control structure */
static iperf_ctrl_t s_iperf_ctrl;

/** Iperf report task handle */
static TaskHandle_t report_task_handle = NULL;

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Iperf_Functions ST67W6X Utility Performance Iperf Functions
  * @ingroup  ST67W6X_Utilities_Performance_Iperf
  * @{
  */

/**
  * @brief  Get current time
  * @param  pT: Pointer to time structure
  */
static void iperf_timer_get_time(iperf_time_struct_t *pT);

/**
  * @brief  Show socket error reason
  * @param  str: Error string
  * @param  sockfd: Socket file descriptor
  * @return Error code
  */
static int32_t iperf_show_socket_error_reason(const char *str, int32_t sockfd);

/**
  * @brief  Iperf report task
  * @param  arg: Task argument
  */
static void iperf_report_task(void *arg);

/**
  * @brief  Start iperf report
  * @return Iperf error code
  */
static iperf_err_t iperf_start_report(void);

/**
  * @brief  Execute the iperf server
  * @param  recv_socket: Receive socket
  * @param  listen_addr_t: Listen address
  * @param  type: Transfer type
  */
static void socket_recv(int32_t recv_socket, struct sockaddr_storage listen_addr_t, uint8_t type);

/**
  * @brief  Execute the iperf client
  * @param  send_socket: Send socket
  * @param  dest_addr_t: Destination address
  * @param  type: Transfer type
  * @param  bw_lim: Bandwidth limit
  */
static void socket_send(int32_t send_socket, struct sockaddr_storage dest_addr_t, uint8_t type, int32_t bw_lim);

/**
  * @brief  Run a Iperf TCP server
  * @return Iperf error code
  */
static iperf_err_t iperf_run_tcp_server(void);

/**
  * @brief  Run a Iperf TCP client
  * @return Iperf error code
  */
static iperf_err_t iperf_run_tcp_client(void);

/**
  * @brief  Run a Iperf UDP server
  * @return Iperf error code
  */
static iperf_err_t iperf_run_udp_server(void);

/**
  * @brief  Run a Iperf UDP client
  * @return Iperf error code
  */
static iperf_err_t iperf_run_udp_client(void);

/**
  * @brief  Iperf process task
  * @param  arg: Task argument
  */
void iperf_task_traffic(void *arg);

#if (IPERF_DUAL_MODE == 1)
/**
  * @brief  Execute the iperf server for dual test
  * @param  recv_socket: Receive socket
  * @param  listen_addr_t: Listen address
  * @param  type: Transfer type
  */
static void socket_recv_dual(int32_t recv_socket, struct sockaddr_storage listen_addr_t, uint8_t type);

/**
  * @brief  Send dual header
  * @param  sock: Socket file descriptor
  * @param  addr: Socket address
  * @param  socklen: Socket length
  */
static void send_dual_header(int32_t sock, struct sockaddr *addr, socklen_t socklen);

/**
  * @brief  Iperf TCP dual test task
  * @param  pvParameters: Task parameters
  */
static void iperf_tcp_dual_server_task(void *pvParameters);
#endif /* IPERF_DUAL_MODE */

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Iperf_Functions
  * @{
  */

int32_t iperf_start(iperf_cfg_t *cfg)
{
#if (IPERF_ENABLE == 1)
  if (!cfg)
  {
    return IPERF_FAIL;
  }

  if (s_iperf_is_running)
  {
    LogDebug("iperf is running\n");
    return IPERF_FAIL;
  }

  memset(&s_iperf_ctrl, 0, sizeof(s_iperf_ctrl));
  memcpy(&s_iperf_ctrl.cfg, cfg, sizeof(*cfg));
  s_iperf_is_running = true;
  s_iperf_ctrl.finish = false;

  /* Calculate the buffer length depending on the configuration */
  if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP))
  {
    s_iperf_ctrl.buffer_len = (s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_UDP_TX_LEN : s_iperf_ctrl.cfg.len_buf);
  }
  else if (((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP)))
  {
    s_iperf_ctrl.buffer_len = (s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_UDP_RX_LEN : s_iperf_ctrl.cfg.len_buf);
  }
  else if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP))
  {
    s_iperf_ctrl.buffer_len = (s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_TCP_TX_LEN : s_iperf_ctrl.cfg.len_buf);
  }
  else
  {
    s_iperf_ctrl.buffer_len = (s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_TCP_RX_LEN : s_iperf_ctrl.cfg.len_buf);
  }

  /* Allocate the buffer */
  s_iperf_ctrl.buffer = (uint8_t *)IPERF_MALLOC(s_iperf_ctrl.buffer_len);
  if (!s_iperf_ctrl.buffer)
  {
    LogError("[iperf] create buffer: not enough memory\n");
    return IPERF_FAIL;
  }

  memset(s_iperf_ctrl.buffer, 0, s_iperf_ctrl.buffer_len);

  if (pdPASS != xTaskCreate(iperf_task_traffic, IPERF_TRAFFIC_TASK_NAME, IPERF_TRAFFIC_TASK_STACK >> 2,
                            NULL, s_iperf_ctrl.cfg.traffic_task_priority, NULL))
  {
    LogError("[iperf] create task %s failed\n", IPERF_TRAFFIC_TASK_NAME);
    if (s_iperf_ctrl.buffer != NULL)
    {
      IPERF_FREE(s_iperf_ctrl.buffer);
      s_iperf_ctrl.buffer = NULL;
    }
    return IPERF_FAIL;
  }

  return IPERF_OK;
#else
  return IPERF_FAIL; /* Iperf is disabled */
#endif /* IPERF_ENABLE */
}

int32_t iperf_stop(void)
{
#if (IPERF_ENABLE == 1)
  if (s_iperf_is_running)
  {
    LogInfo("iperf aborting ...\n");
    s_iperf_ctrl.finish = true;
  }

  return IPERF_OK;
#else
  return IPERF_FAIL; /* Iperf is disabled */
#endif /* IPERF_ENABLE */
}

/* Private Functions Definition ----------------------------------------------*/
static void iperf_timer_get_time(iperf_time_struct_t *pT)
{
  uint32_t system_tick;
  system_tick = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();

  pT->sec = system_tick / configTICK_RATE_HZ;
  pT->usec = (system_tick * portTICK_PERIOD_MS) * 1000 - (pT->sec * 1000000);
}

static int32_t iperf_show_socket_error_reason(const char *str, int32_t sockfd)
{
  int32_t err = errno;
  if (err != 0)
  {
    LogInfo("%s error, error code: %" PRIi32 ", reason: %s\n", str, err, strerror(err));
  }

  return err;
}

static void iperf_report_task(void *arg)
{
  uint32_t time = s_iperf_ctrl.cfg.time;
  uint32_t interval_sec = s_iperf_ctrl.cfg.interval;
  uint32_t start_time = 0;
  uint32_t elapsed_time = 0;
  volatile int32_t average = 0;
  int32_t actual_bandwidth = 0;
  volatile int32_t actual_transfer = 0;

  vTaskDelay(1000);
  LogInfo("[ ID] Interval       Transfer        Bandwidth\n");

  /* Report the bandwidth every interval seconds */
  uint32_t count = 0;
  while (!s_iperf_ctrl.finish)
  {
    if ((count % 10) == 0)
    {
      elapsed_time ++;
      count = 0;
      if ((!s_iperf_ctrl.finish) && (interval_sec != 0) && (elapsed_time % interval_sec == 0))
      {
        /* The following values used for display are by 10 or 100 depending on the desired precision
           so that the decimal part can be displayed without using float */
        /* Calculate the actual bytes transferred */
        actual_transfer = (s_iperf_ctrl.actual_len  * 100) / (1024 * 1024);
        /* Calculate the actual bandwidth
           Formula is: (Bytes * 8 * 10 / (1000 * 1000)) / interval duration*/
        actual_bandwidth = (s_iperf_ctrl.actual_len * 8 / 100000) / interval_sec;
        /* Calculate the average bandwidth from the start */
        average = ((average * (elapsed_time - 1) / elapsed_time) + (actual_bandwidth / elapsed_time));

        LogInfo("[%3" PRIu32 "] %" PRIu32 ".0-%" PRIu32 ".0 sec  %2" PRIu32 ".%02" PRIu32 " MBytes    %2"
                PRIu32 ".%" PRIu32 " Mbits/sec\n",
                s_iperf_ctrl.sockfd, start_time, start_time + interval_sec,
                (int32_t)actual_transfer / 100, actual_transfer % 100,
                (int32_t)actual_bandwidth / 10, actual_bandwidth % 10);
        start_time += interval_sec; /* Update the start time for the next interval */
        s_iperf_ctrl.actual_len = 0; /* Reset the actual length for the next interval */
      }
      if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (elapsed_time >= time)) /* Check if the time is over */
      {
        s_iperf_ctrl.finish = true;
        break;
      }
    }
    vTaskDelay(100);
    count++;
  }

  if (((count > 0) || (elapsed_time > 0)) &&
      (((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (elapsed_time >= time)) ||
       ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP))))
  {
    /* The following values used for display are by 10 or 100 depending on the desired precision
       so that the decimal part can be displayed without using float */
    /* Convert the total bytes transferred to MBytes */
    actual_transfer = (s_iperf_ctrl.tot_len * 100) / (1024 * 1024);

    /* Calculate the average bandwidth from the start
       Formula is: (Total bytes * 8 * 10) / (1000 * 1000) / ((10 * seconds + count) / 10) */
    average = ((s_iperf_ctrl.tot_len * 8 / 10000)) / (10 * elapsed_time + count);

    LogInfo("[%3" PRIu32 "]  0.0-%" PRIu32 ".%" PRIu32 " sec  %2" PRIu32 ".%02" PRIu32
            " MBytes    %2" PRIu32 ".%" PRIu32 " Mbits/sec\n",
            s_iperf_ctrl.sockfd, elapsed_time + count / 10, count % 10,
            (int32_t)actual_transfer / 100, actual_transfer % 100,
            (int32_t)(average / 10), average % 10);
  }

  ulTaskNotifyTake(pdTRUE, 500);
  vTaskDelete(NULL);
}

static iperf_err_t iperf_start_report(void)
{
  if (pdPASS != xTaskCreate(iperf_report_task, IPERF_REPORT_TASK_NAME, IPERF_REPORT_TASK_STACK >> 2,
                            NULL, s_iperf_ctrl.cfg.traffic_task_priority, &report_task_handle))
  {
    LogError("[iperf] create task %s failed\n", IPERF_REPORT_TASK_NAME);
    return IPERF_FAIL;
  }

  return IPERF_OK;
}

static void socket_recv(int32_t recv_socket, struct sockaddr_storage listen_addr_t, uint8_t type)
{
  bool iperf_recv_running = false;
  uint8_t timeout_count = 2;
  int32_t want_recv = 0;
  int32_t actual_recv = 0;
  int32_t datagramID;
  iperf_time_struct_t lastPacketTime;
  iperf_time_struct_t firstPacketTime;
#if JITTER_RX
  iperf_time_struct_t packetTime;
  iperf_time_struct_t sentTime;
  long lastTransit = 0;
#endif /* JITTER_RX */
  int32_t lastPacketID = 0;
  Transfer_Info_t stats;
#if IPERF_V6
  socklen_t socklen = (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6) ? \
                      sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
#else
  socklen_t socklen = sizeof(struct sockaddr_in);
#endif /* IPERF_V6 */
  const char *error_log = (type == IPERF_TRANS_TYPE_TCP) ? "tcp server recv" : "udp server recv";

  s_iperf_ctrl.sockfd = recv_socket;
  want_recv = s_iperf_ctrl.buffer_len;

  memset(&stats, 0, sizeof(Transfer_Info_t));

  while (!s_iperf_ctrl.finish) /* Start the iperf traffic task in server mode */
  {
    /* Receive data */
    if (type == IPERF_TRANS_TYPE_UDP)
    {
      actual_recv = W6X_Net_Recvfrom(recv_socket, s_iperf_ctrl.buffer, want_recv, 0,
                                     (struct sockaddr *)&listen_addr_t, &socklen);
    }
    else
    {
      actual_recv = W6X_Net_Recv(recv_socket, s_iperf_ctrl.buffer, want_recv, 0);
    }

    if (actual_recv < 0) /* Check if the receive is failed */
    {
      if (!s_iperf_ctrl.finish)
      {
        iperf_show_socket_error_reason(error_log, recv_socket);
        s_iperf_ctrl.finish = true; /* Stop the iperf traffic task */
        break;
      }
    }
    else if (!iperf_recv_running && (actual_recv == 0) && (timeout_count > 0)) /* Wait for the first packet */
    {
      timeout_count--;
      if (timeout_count == 0) /* No data received after 10s (2*5s of RecvTimeout). */
      {
        s_iperf_ctrl.finish = true; /* Stop the iperf traffic task */
        break;
      }
    }
    else if (s_iperf_ctrl.finish)
    {
      break;
    }

    if (s_iperf_ctrl.finish)
    {
      break;
    }
    else if (actual_recv > 0) /* Data received */
    {
      if (!iperf_recv_running) /* Start the iperf report task on first packet received */
      {
        iperf_timer_get_time(&firstPacketTime);
        iperf_start_report();
        iperf_recv_running = true;
      }

      /* Update the actual length and total length */
      s_iperf_ctrl.actual_len += actual_recv;
      s_iperf_ctrl.tot_len += actual_recv;

      /* Check if the total length is reached */
      if (s_iperf_ctrl.cfg.num_bytes > 0 && s_iperf_ctrl.tot_len > s_iperf_ctrl.cfg.num_bytes)
      {
        break;
      }

      if (type == IPERF_TRANS_TYPE_UDP)
      {
        stats.cntDatagrams++;

        /* Get the datagram ID from the received packet */
        datagramID = PP_HTONL(((UDP_datagram_t *) s_iperf_ctrl.buffer)->id);

        if (datagramID >= 0) /* If the datagram ID is negative, it means the end of the test */
        {
#if JITTER_RX
          long deltaTransit;
          sentTime.sec = PP_HTONL(((UDP_datagram_t *) s_iperf_ctrl.buffer)->tv_sec);
          sentTime.usec = PP_HTONL(((UDP_datagram_t *) s_iperf_ctrl.buffer)->tv_usec);

          /* Update received amount and time */
          iperf_timer_get_time(&packetTime);

          /* From RFC 1889, Real Time Protocol (RTP)
           * J = J + ( | D(i-1,i) | - J ) / 16 */
          long transit = (long)((sentTime.sec - packetTime.sec) * 1000000 + (sentTime.usec - packetTime.usec));
          if (lastTransit != 0)
          {
            if (lastTransit < transit)
            {
              deltaTransit = transit - lastTransit;
            }
            else
            {
              deltaTransit = lastTransit - transit;
            }

            stats.jitter += (((float)deltaTransit / 1e6) - stats.jitter) / (16.0);
          }
          lastTransit = transit;
#endif /* JITTER_RX */

          /* Packet loss occurred if the datagram numbers aren't sequential */
          if (datagramID != lastPacketID + 1)
          {
            if (datagramID < lastPacketID + 1)
            {
              stats.cntOutofOrder++;
            }
            else
            {
              stats.cntError += datagramID - lastPacketID - 1;
            }
          }
          /* Never decrease datagramID (e.g. if we get an out-of-order packet) */
          if (datagramID > lastPacketID)
          {
            lastPacketID = datagramID;
          }
        }
        else if (s_iperf_ctrl.finish == false)
        {
          s_iperf_ctrl.finish = true;

          /* End of test. Prepare to send the report packet */
          if (actual_recv > (int)(sizeof(UDP_datagram_t) + sizeof(iperf_server_hdr_t)))
          {
            UDP_datagram_t *UDP_Hdr;
            iperf_server_hdr_t *server_hdr;
            iperf_timer_get_time(&lastPacketTime);

            UDP_Hdr = (UDP_datagram_t *) s_iperf_ctrl.buffer;
            server_hdr = (iperf_server_hdr_t *)(UDP_Hdr + 1);
            server_hdr->flags        = PP_HTONL(0x80000000);
            server_hdr->total_len1   = PP_HTONL((long)(s_iperf_ctrl.tot_len >> 32));
            server_hdr->total_len2   = PP_HTONL((long)(s_iperf_ctrl.tot_len & 0xFFFFFFFF));
            stats.endTime = (lastPacketTime.sec - firstPacketTime.sec) * 10 + \
                            (lastPacketTime.usec - firstPacketTime.usec) / 100000;
            server_hdr->stop_sec     = PP_HTONL((long)(lastPacketTime.sec - firstPacketTime.sec));
            server_hdr->stop_usec    = PP_HTONL((long)(lastPacketTime.usec - firstPacketTime.usec));
            server_hdr->error_cnt    = PP_HTONL(stats.cntError);
            server_hdr->outorder_cnt = PP_HTONL(stats.cntOutofOrder);
            server_hdr->datagrams    = PP_HTONL(stats.cntDatagrams + stats.cntError);
            server_hdr->jitter1      = PP_HTONL((long) stats.jitter);
            server_hdr->jitter2      = PP_HTONL((long)((stats.jitter - (long)stats.jitter) * 1e6));

            /* The following values used for display are by 10 or 100 depending on the desired precision
               so that the decimal part can be displayed without using float */
            int32_t total_transfer = (s_iperf_ctrl.tot_len) * 10 / (1024 * 1024);
            /* Calculate the average bandwidth from the start */
            int32_t average = (total_transfer * 8) * 10 / stats.endTime;
            /* Calculate the error percentage to be printed */
            int32_t err_percentage = PERCENT_MULTIPLIER * stats.cntError / (stats.cntDatagrams + stats.cntError);

            LogInfo("[%3" PRIu32 "]  0.0-%" PRIu32 ".%" PRIu32 " sec  %2" PRIu32 ".%02" PRIu32
                    " MBytes    %2" PRIu32 ".%" PRIu32 " Mbits/sec\n",
                    s_iperf_ctrl.sockfd, (int32_t)(stats.endTime / 10),
                    stats.endTime % 10, (int32_t)(total_transfer / 10),
                    total_transfer % 10, (int32_t)(average / 10), average % 10);

#if JITTER_RX
            LogInfo("[ ID]  Jitter        Lost/Total Datagrams\n");
            LogInfo("[%3d] %6.3f ms      %4d/%5d (%.2g%%)\n", s_iperf_ctrl.sockfd,
                    stats.jitter * 1000.0, stats.cntError, (stats.cntDatagrams + stats.cntError),
                    (100.0 * stats.cntError) / (stats.cntDatagrams + stats.cntError));
#else
            LogInfo("[ ID]                Lost/Total Datagrams\n");
            LogInfo("[%3" PRIu32 "]                %4" PRIi32 "/%5" PRIi32 " (%2" PRIu32 ".%" PRIu32 "%%)\n",
                    s_iperf_ctrl.sockfd, stats.cntError, (stats.cntDatagrams + stats.cntError),
                    (int32_t)(err_percentage / 10), err_percentage % 10);
#endif /* JITTER_RX */
          }
          /* Send the report packet */
          W6X_Net_Sendto(recv_socket, s_iperf_ctrl.buffer, want_recv, 0, (struct sockaddr *)&listen_addr_t, socklen);

          /* Limits to to 1 sec */
          int32_t to = LAST_SOCKET_TIMEOUT;
          int32_t len = sizeof(to);

          (void)W6X_Net_Setsockopt(recv_socket, SOL_SOCKET, SO_RCVTIMEO, &to, len);

          /* Wait for the last packet to flush remaining data */
          (void)W6X_Net_Recvfrom(recv_socket, s_iperf_ctrl.buffer, want_recv, 0,
                                 (struct sockaddr *)&listen_addr_t, &socklen);
        }
      }
    }
  }

  if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP))
  {
    vTaskDelay(1000); /* Wait for the last report trace to be printed */
  }

  s_iperf_ctrl.finish = true;
}

static void socket_send(int32_t send_socket, struct sockaddr_storage dest_addr_t, uint8_t type, int32_t bw_lim)
{
  UDP_datagram_t *hdr;
  int32_t pkt_cnt = 0;
  int32_t want_send = 0;
  int32_t delay_target = -1;
  int32_t delay = 0;
  int64_t adjust = 0;
  iperf_time_struct_t lastPacketTime;
  iperf_time_struct_t time = {0};
  /* int32_t err = 0; */
#if IPERF_V6
  const socklen_t socklen = (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6) ? \
                            sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
#else
  const socklen_t socklen = sizeof(struct sockaddr_in);
#endif /* IPERF_V6 */
  const char *error_log = (type == IPERF_TRANS_TYPE_TCP) ? "tcp client send" : "udp client send";

  s_iperf_ctrl.sockfd = send_socket;
  hdr = (UDP_datagram_t *)s_iperf_ctrl.buffer;
  want_send = s_iperf_ctrl.buffer_len;
  iperf_start_report();

  if (bw_lim > 0)
  {
    delay_target = want_send * 8 / bw_lim;
  }

#if (IPERF_DUAL_MODE == 1)
  if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP)
      && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_DUAL))
  {
    send_dual_header(send_socket, (struct sockaddr *)&dest_addr_t, socklen);
  }
#endif /* IPERF_DUAL_MODE */

  iperf_timer_get_time(&lastPacketTime); /* Get the current time */

  while (!s_iperf_ctrl.finish) /* Start the iperf traffic task in client mode */
  {
    if (delay_target > 0) /* Calculate the delay between each packet if the bandwidth limit is set */
    {
      iperf_timer_get_time(&time);
      adjust = delay_target + (int64_t)(lastPacketTime.sec - time.sec) * 1000000 + (lastPacketTime.usec - time.usec);
      lastPacketTime = time;
      /* If the delay is positive, it means we are ahead of schedule and we need to delay the loop */
      if (adjust > 0  ||  delay > 0)
      {
        delay += adjust;
      }
    }
    int32_t subpacket_count = 0;
    /* Make sure the datagram header will fit within the remaining space */
    hdr = (UDP_datagram_t *)(s_iperf_ctrl.buffer);
    hdr->tv_sec = PP_HTONL(time.sec);
    hdr->tv_usec = PP_HTONL(time.usec);
    while ((subpacket_count * 1470 + sizeof(UDP_datagram_t)) < want_send)
    {
      hdr = (UDP_datagram_t *)(s_iperf_ctrl.buffer + subpacket_count * 1470);
      hdr->id = PP_HTONL(pkt_cnt); /* Datagrams need to be sequentially numbered */
      hdr->tv_sec = ((UDP_datagram_t *)(s_iperf_ctrl.buffer))->tv_sec;
      hdr->tv_usec = ((UDP_datagram_t *)(s_iperf_ctrl.buffer))->tv_usec;
      if (pkt_cnt >= INT32_MAX) /* Wrap the sequence number */
      {
        pkt_cnt = 0;
      }
      else
      {
        pkt_cnt++;
      }
      subpacket_count++;
    }
    uint32_t buffer_len = want_send;
    long currLen = 0;

    while (buffer_len) /* Send data */
    {
      if (type == IPERF_TRANS_TYPE_UDP)
      {
        currLen = W6X_Net_Sendto(send_socket, s_iperf_ctrl.buffer, buffer_len, 0,
                                 (struct sockaddr *)&dest_addr_t, socklen);
      }
      else
      {
        currLen = W6X_Net_Send(send_socket, s_iperf_ctrl.buffer, buffer_len, 0);
      }

      if (currLen >= 0) /* Check if the send is failed */
      {
        buffer_len -= currLen;
      }
      else
      {
        break;
      }
    }

    if (currLen < 0) /* Send is in error. Stop the iperf traffic task */
    {
      if (type == IPERF_TRANS_TYPE_UDP)
      {
        /* err = errno; */
        /* ENOMEM is expected under heavy load => do not print it */
        /* if (err != ENOMEM) */
        {
          iperf_show_socket_error_reason(error_log, send_socket);
        }
      }
      else if (type == IPERF_TRANS_TYPE_TCP)
      {
        iperf_show_socket_error_reason(error_log, send_socket);
        break;
      }
    }

    s_iperf_ctrl.actual_len += want_send;
    s_iperf_ctrl.tot_len += want_send;

    /* Check if the total length is reached */
    if (s_iperf_ctrl.cfg.num_bytes > 0 && s_iperf_ctrl.tot_len >= s_iperf_ctrl.cfg.num_bytes)
    {
      break;
    }

    /* The send delay may be negative, it indicates we are trying to catch up and hence to not delay the loop at all. */
    if (delay > 2000)
    {
      if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
      {
        vTaskDelay((delay / 1000) / portTICK_PERIOD_MS); /* Parameter should be in system ticks */
      }
    }
  }

  if (type == IPERF_TRANS_TYPE_UDP)
  {
    hdr->id = (int32_t)PP_HTONL(-pkt_cnt);
    hdr->tv_sec = PP_HTONL(time.sec);
    hdr->tv_usec = PP_HTONL(time.usec);

    int32_t count = 0;
    int32_t rc;

    /* Limits to to 1 sec */
    int32_t to = LAST_SOCKET_TIMEOUT;
    int32_t len = sizeof(to);

    (void)W6X_Net_Setsockopt(send_socket, SOL_SOCKET, SO_RCVTIMEO, &to, len);

    while (count < 2) /* Try to receive the report packet from the server */
    {
      count++;
      /* Write data */
      (void)W6X_Net_Sendto(send_socket, s_iperf_ctrl.buffer, want_send, 0, (struct sockaddr *)&dest_addr_t, socklen);

      socklen_t rxsocklen = socklen;
      rc = W6X_Net_Recvfrom(send_socket, s_iperf_ctrl.buffer, want_send, 0,
                            (struct sockaddr *)&dest_addr_t, &rxsocklen);
      if (rc > 0)
      {
        /* Check if the receive packet is the report packet */
        if (rc >= (int)(sizeof(UDP_datagram_t) + sizeof(iperf_server_hdr_t)))
        {
          while (!s_iperf_ctrl.finish); /* Wait the end of report task */

          /* Process and print the report packet */
          iperf_server_hdr_t *server_hdr = (iperf_server_hdr_t *)&s_iperf_ctrl.buffer[sizeof(UDP_datagram_t)];

          LogInfo("[ ID]  Jitter        Lost/Total Datagrams\n");
          LogInfo("[%3" PRIi32 "] %" PRIu32 ".%03" PRIu32 " ms    %4" PRIu32 "/%5" PRIu32 " (%3" PRIu32 "%%)\n",
                  send_socket,
                  (uint32_t)PP_HTONL(server_hdr->jitter1) * 1000, (uint32_t)PP_HTONL(server_hdr->jitter2) * 1000,
                  (uint32_t)PP_HTONL(server_hdr->error_cnt), (uint32_t)PP_HTONL(server_hdr->datagrams),
                  (uint32_t)((100 * PP_HTONL(server_hdr->error_cnt)) / PP_HTONL(server_hdr->datagrams)));
        }
        return;
      }
      else if (rc < 0)
      {
        break;
      }
    }
  }
}

static iperf_err_t iperf_run_tcp_server(void)
{
  int32_t listen_socket = -1;
  int32_t client_socket = -1;
  /* int32_t opt = 1; */
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_in remote_addr_t;
  int32_t timeout = 0;
  socklen_t addr_len = sizeof(struct sockaddr);
  struct sockaddr_storage listen_addr_t = { 0 };
#if IPERF_V6
  struct sockaddr_in6 listen_addr6_t = { 0 };
#endif /* IPERF_V6 */
  struct sockaddr_in listen_addr4_t = { 0 };

#if IPERF_V6
  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
#else
  if (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

#if IPERF_V6
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {
    /* The TCP server listen at the address "::", which means all addresses can be listened to. */
    inet6_aton("::", &listen_addr6_t.sin6_addr);
    listen_addr6_t.sin6_family = AF_INET6;
    listen_addr6_t.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.sport);

    listen_socket = W6X_Net_Socket(AF_INET6, SOCK_STREAM, IPPROTO_IPV6);
    if (listen_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    (void)W6X_Net_Setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    (void)W6X_Net_Setsockopt(listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    err = W6X_Net_Bind(listen_socket, (struct sockaddr *)&listen_addr6_t, sizeof(listen_addr6_t));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 ", IPPROTO: %" PRIu16 "\n", errno, AF_INET6);
      goto exit;
    }

    err = W6X_Net_Listen(listen_socket, 1);
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Error occurred during listen: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    timeout = IPERF_SOCKET_RX_TIMEOUT * 1000;
    (void)W6X_Net_Setsockopt(listen_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memcpy(&listen_addr_t, &listen_addr6_t, sizeof(listen_addr6_t));
  }
  else
#endif /* IPERF_V6 */
  {
    if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
    {
      listen_addr4_t.sin_family = AF_INET;
      listen_addr4_t.sin_port = PP_HTONS(s_iperf_ctrl.cfg.sport);
      listen_addr4_t.sin_addr_t.s_addr = s_iperf_ctrl.cfg.source_ip4;

      listen_socket = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (listen_socket < 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
        goto exit;
      }

      /* W6X_Net_Setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); */
      err = W6X_Net_Bind(listen_socket, (struct sockaddr *)&listen_addr4_t, sizeof(listen_addr4_t));
      if (err != 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Socket unable to bind: errno %" PRIi32 ", IPPROTO: %" PRIu16 "\n", (int32_t)errno, AF_INET);
        goto exit;
      }

      err = W6X_Net_Listen(listen_socket, 5);
      if (err != 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Error occurred during listen: errno %" PRIi32 "\n", (int32_t)errno);
        goto exit;
      }
      memcpy(&listen_addr_t, &listen_addr4_t, sizeof(listen_addr4_t));
    }
  }

  client_socket = W6X_Net_Accept(listen_socket, (struct sockaddr *)&remote_addr_t, &addr_len);
  if (client_socket < 0)
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Unable to accept connection: errno %" PRIi32 "\n", (int32_t)errno);
    goto exit;
  }

  timeout = TCP_RX_SOCKET_TIMEOUT;
  (void)W6X_Net_Setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  socket_recv(client_socket, listen_addr_t, IPERF_TRANS_TYPE_TCP);
exit:
  if (client_socket != -1)
  {
    W6X_Net_Close(client_socket);
  }

  if (listen_socket != -1)
  {
    W6X_Net_Shutdown(listen_socket, 1);
    /* W6X_Net_Close(listen_socket); */
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

static iperf_err_t iperf_run_tcp_client(void)
{
  int32_t client_socket = -1;
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_storage dest_addr_t = { 0 };
#if IPERF_V6
  struct sockaddr_in6 dest_addr6_t = { 0 };
#endif /* IPERF_V6 */
  struct sockaddr_in dest_addr4_t = { 0 };
  /* int32_t opt = s_iperf_ctrl.cfg.tos; */

#if IPERF_V6
  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
#else
  if (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

#if IPERF_DUAL_MODE
  if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP)
      && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_DUAL))
  {
    xTaskCreate(iperf_tcp_dual_server_task, "dual_rx", IPERF_TRAFFIC_TASK_STACK >> 2,
                NULL, s_iperf_ctrl.cfg.traffic_task_priority, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
#endif /* IPERF_DUAL_MODE */

#if IPERF_V6
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {
    client_socket = W6X_Net_Socket(AF_INET6, SOCK_STREAM, IPPROTO_IPV6);
    if (client_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    (void)W6X_Net_Setsockopt(client_socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt));

    inet6_aton(s_iperf_ctrl.cfg.destination_ip6, &dest_addr6_t.sin6_addr);
    dest_addr6_t.sin6_family = AF_INET6;
    dest_addr6_t.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.dport);

    err = W6X_Net_Connect(client_socket, (struct sockaddr *)&dest_addr6_t, sizeof(struct sockaddr_in6));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to connect: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    memcpy(&dest_addr_t, &dest_addr6_t, sizeof(dest_addr6_t));
  }
  else
#endif /* IPERF_V6 */
  {
    if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
    {
      client_socket = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (client_socket < 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
        goto exit;
      }
      /* W6X_Net_Setsockopt(client_socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt)); */

      dest_addr4_t.sin_family = AF_INET;
      dest_addr4_t.sin_port = PP_HTONS(s_iperf_ctrl.cfg.dport);
      dest_addr4_t.sin_addr_t.s_addr = s_iperf_ctrl.cfg.destination_ip4;
      err = W6X_Net_Connect(client_socket, (struct sockaddr *)&dest_addr4_t, sizeof(struct sockaddr_in));
      if (err != 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Socket unable to connect: errno %" PRIi32 "\n", (int32_t)errno);
        goto exit;
      }
      memcpy(&dest_addr_t, &dest_addr4_t, sizeof(dest_addr4_t));
    }
  }

  socket_send(client_socket, dest_addr_t, IPERF_TRANS_TYPE_TCP, s_iperf_ctrl.cfg.bw_lim);
exit:
  if (client_socket != -1)
  {
    /* W6X_Net_Shutdown(client_socket, 0); */
    W6X_Net_Close(client_socket);
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

static iperf_err_t iperf_run_udp_server(void)
{
  int32_t listen_socket = -1;
  /* int32_t opt = 1; */
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
  int32_t timeout = 0;
  struct sockaddr_storage listen_addr_t = { 0 };
#if IPERF_V6
  struct sockaddr_in6 listen_addr6_t = { 0 };
#endif /* IPERF_V6 */
  struct sockaddr_in listen_addr4_t = { 0 };

#if IPERF_V6
  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
#else
  if (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

#if IPERF_V6
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {
    /* The UDP server listen at the address "::", which means all addresses can be listened to. */
    inet6_aton("::", &listen_addr6_t.sin6_addr);
    listen_addr6_t.sin6_family = AF_INET6;
    listen_addr6_t.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.sport);

    listen_socket = W6X_Net_Socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (listen_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    (void)W6X_Net_Setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    err = W6X_Net_Bind(listen_socket, (struct sockaddr *)&listen_addr6_t, sizeof(struct sockaddr_in6));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    memcpy(&listen_addr_t, &listen_addr6_t, sizeof(listen_addr6_t));
  }
  else
#endif /* IPERF_V6 */
  {
    if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
    {
      listen_addr4_t.sin_family = AF_INET;
      listen_addr4_t.sin_port = PP_HTONS(s_iperf_ctrl.cfg.sport);
      listen_addr4_t.sin_addr_t.s_addr = s_iperf_ctrl.cfg.source_ip4;

      listen_socket = W6X_Net_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (listen_socket < 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
        goto exit;
      }

      /* W6X_Net_Setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); */
      timeout = IPERF_SOCKET_RX_TIMEOUT * 1000;
      (void)W6X_Net_Setsockopt(listen_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

      err = W6X_Net_Bind(listen_socket, (struct sockaddr *)&listen_addr4_t, sizeof(struct sockaddr_in));
      if (err != 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Socket unable to bind: errno %" PRIi32 "\n", (int32_t)errno);
        goto exit;
      }
      memcpy(&listen_addr_t, &listen_addr4_t, sizeof(listen_addr4_t));
    }
  }

  socket_recv(listen_socket, listen_addr_t, IPERF_TRANS_TYPE_UDP);
exit:
  if (listen_socket != -1)
  {
    W6X_Net_Shutdown(listen_socket, 1);
    /* W6X_Net_Close(listen_socket); */
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

static iperf_err_t iperf_run_udp_client(void)
{
  int32_t client_socket = -1;
  /* int32_t opt = 1; */
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_storage dest_addr_t = { 0 };
#if IPERF_V6
  struct sockaddr_in6 dest_addr6_t = { 0 };
#endif /* IPERF_V6 */
  struct sockaddr_in dest_addr4_t = { 0 };
#if IPERF_V6
  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
#else
  if (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

#if IPERF_V6
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {
    inet6_aton(s_iperf_ctrl.cfg.destination_ip6, &dest_addr6_t.sin6_addr);
    dest_addr6_t.sin6_family = AF_INET6;
    dest_addr6_t.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.dport);

    client_socket = W6X_Net_Socket(AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);
    if (client_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    (void)W6X_Net_Setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    opt = s_iperf_ctrl.cfg.tos;
    (void)W6X_Net_Setsockopt(client_socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt));
    memcpy(&dest_addr_t, &dest_addr6_t, sizeof(dest_addr6_t));
  }
  else
#endif /* IPERF_V6 */
  {
    if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
    {
      dest_addr4_t.sin_family = AF_INET;
      dest_addr4_t.sin_port = PP_HTONS(s_iperf_ctrl.cfg.dport);
      dest_addr4_t.sin_addr_t.s_addr = s_iperf_ctrl.cfg.destination_ip4;

      client_socket = W6X_Net_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (client_socket < 0)
      {
        ret = IPERF_FAIL;
        LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
        goto exit;
      }

      /*
      W6X_Net_Setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      opt = s_iperf_ctrl.cfg.tos;
      W6X_Net_Setsockopt(client_socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt));
      */
      memcpy(&dest_addr_t, &dest_addr4_t, sizeof(dest_addr4_t));
    }
  }

  socket_send(client_socket, dest_addr_t, IPERF_TRANS_TYPE_UDP, s_iperf_ctrl.cfg.bw_lim);
exit:
  if (client_socket != -1)
  {
    /* W6X_Net_Shutdown(client_socket, 0); */
    W6X_Net_Close(client_socket);
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

void iperf_task_traffic(void *arg)
{
  (void)s_iperf_is_running;

  /* Save low power config */
  if ((W6X_WiFi_GetDTIM(&s_iperf_ctrl.dtim) != W6X_STATUS_OK) ||
      (W6X_GetPowerMode(&s_iperf_ctrl.ps_mode) != W6X_STATUS_OK))
  {
    goto _err1;
  }

  /* Disable low power */
  if ((W6X_SetPowerMode(0) != W6X_STATUS_OK) || (W6X_WiFi_SetDTIM(0) != W6X_STATUS_OK))
  {
    goto _err2;
  }

  LogInfo("------------------------------------------------------------\n");
  if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP))
  {
    LogInfo("Server listening on UDP port %" PRIu16 "\n",
            s_iperf_ctrl.cfg.sport);
  }
  else if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP))
  {
    LogInfo("Server listening on TCP port %" PRIu16 "\n",
            s_iperf_ctrl.cfg.sport);
  }
  else if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP))
  {
    char ipaddr[INET_ADDRSTRLEN + 1] = {0};
    /*char *ip_ptr = */(void)W6X_Net_Inet_ntop(AF_INET, (void *) & (s_iperf_ctrl.cfg.destination_ip4), ipaddr,
                                               INET_ADDRSTRLEN + 1);
    LogInfo("Client connecting to %s, UDP port %" PRIu16 "\n", ipaddr, s_iperf_ctrl.cfg.dport);
    LogInfo("Sending %" PRIu16 " bytes datagrams\n",
            s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_UDP_TX_LEN : s_iperf_ctrl.cfg.len_buf);
  }
  else if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP))
  {
    char ipaddr[INET_ADDRSTRLEN + 1] = {0};
    /*char *ip_ptr = */(void)W6X_Net_Inet_ntop(AF_INET, (void *) & (s_iperf_ctrl.cfg.destination_ip4), ipaddr,
                                               INET_ADDRSTRLEN + 1);
    LogInfo("Client connecting to %s, TCP port %" PRIu16 "\n", ipaddr, s_iperf_ctrl.cfg.dport);
  }
  LogInfo("------------------------------------------------------------\n");

  if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP))
  {
    iperf_run_udp_client();
  }
  else if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP))
  {
    iperf_run_udp_server();
  }
  else if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP))
  {
    iperf_run_tcp_client();
  }
  else
  {
    iperf_run_tcp_server();
  }

  if (s_iperf_ctrl.buffer) /* Free the buffer at the end of the task */
  {
    IPERF_FREE(s_iperf_ctrl.buffer);
    s_iperf_ctrl.buffer = NULL;
  }

_err2:
  /* Restore low power config */
  (void)W6X_SetPowerMode(s_iperf_ctrl.ps_mode);
  (void)W6X_WiFi_SetDTIM(s_iperf_ctrl.dtim);

_err1:
  s_iperf_is_running = false;
  LogInfo("iperf exit\n");
  if (report_task_handle)
  {
    xTaskNotifyGive(report_task_handle);
    report_task_handle = NULL;
  }

  vTaskDelete(NULL);
}

#if (IPERF_DUAL_MODE == 1)
static void socket_recv_dual(int32_t recv_socket, struct sockaddr_storage listen_addr_t, uint8_t type)
{
  uint8_t *buffer;
  int32_t want_recv = 0;
  int32_t actual_recv = 0;
  socklen_t socklen = sizeof(struct sockaddr_in);

  buffer = IPERF_MALLOC(RECV_DUAL_BUF_LEN);
  want_recv = RECV_DUAL_BUF_LEN;
  if (!buffer)
  {
    return;
  }
  while (1)
  {
    actual_recv = W6X_Net_Recvfrom(recv_socket, buffer, want_recv, 0, (struct sockaddr *)&listen_addr_t, &socklen);
    if (actual_recv <= 0)
    {
      break;
    }
  }
  IPERF_FREE(buffer);
}

static void send_dual_header(int32_t sock, struct sockaddr *addr, socklen_t socklen)
{
  iperf_client_hdr_t hdr = {0};
  iperf_cfg_t *cfg = &s_iperf_ctrl.cfg;
  uint32_t source_port = cfg->sport;

  hdr.flags = PP_HTONL(HEADER_VERSION1 | RUN_NOW);
  hdr.numThreads = PP_HTONL(1);
  hdr.mPort = PP_HTONL(source_port);
  hdr.mAmount = PP_HTONL(-(cfg->time * 100));

  W6X_Net_Sendto(sock, &hdr, sizeof(hdr), 0, addr, socklen);
}

static void iperf_tcp_dual_server_task(void *pvParameters)
{
  int32_t listen_socket = -1;
  int32_t client_socket = -1;
  /* int32_t opt = 1; */
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_in remote_addr_t;
  int32_t timeout = { 0 };
  socklen_t addr_len = sizeof(struct sockaddr);
  struct sockaddr_storage listen_addr_t = { 0 };
  struct sockaddr_in listen_addr4_t = { 0 };

#if IPERF_V6
  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
#else
  if (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

  (void)ret;
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
  {
    listen_addr4_t.sin_family = AF_INET;
    listen_addr4_t.sin_port = PP_HTONS(s_iperf_ctrl.cfg.sport);
    listen_addr4_t.sin_addr_t.s_addr = s_iperf_ctrl.cfg.source_ip4;

    listen_socket = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket < 0)
    {
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    /* W6X_Net_Setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); */

    err = W6X_Net_Bind(listen_socket, (struct sockaddr *)&listen_addr4_t, sizeof(listen_addr4_t));
    if (err != 0)
    {
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 ", IPPROTO: %" PRIu16 "\n", errno, AF_INET);
      goto exit;
    }

    err = W6X_Net_Listen(listen_socket, 5);
    if (err != 0)
    {
      LogError("[iperf] Error occurred during listen: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    memcpy(&listen_addr_t, &listen_addr4_t, sizeof(listen_addr4_t));
  }

  client_socket = W6X_Net_Accept(listen_socket, (struct sockaddr *)&remote_addr_t, &addr_len);
  if (client_socket < 0)
  {
    LogError("[iperf] Unable to accept connection: errno %" PRIi32 "\n", (int32_t)errno);
    goto exit;
  }

  timeout = IPERF_SOCKET_RX_TIMEOUT * 1000;
  (void)W6X_Net_Setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  socket_recv_dual(client_socket, listen_addr_t, IPERF_TRANS_TYPE_TCP);
exit:
  if (client_socket != -1)
  {
    W6X_Net_Close(client_socket);
  }

  if (listen_socket != -1)
  {
    W6X_Net_Shutdown(listen_socket, 0);
    W6X_Net_Close(listen_socket);
  }

  vTaskDelete(NULL);
}
#endif /* IPERF_DUAL_MODE */

/** @} */
