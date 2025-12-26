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
#include "w6x_types.h"     /* W6X_ARCH_** */
#if (ST67_ARCH == W6X_ARCH_T02)
#include "lwip.h"
#include "lwip/errno.h"
#include "lwip/sockets.h"
#include "lwip/udp.h"
#else
#include "errno.h"
#endif /* ST67_ARCH */
#include "w6x_api.h"

#if (ST67_ARCH == W6X_ARCH_T02)
#if (IPERF_V6 == 1) && (LWIP_IPV6 == 0)
#error "IPv6 support must be enabled for IPERF_V6"
#endif /* IPERF_V6 && LWIP_IPV6 */
#else
#if (IPERF_V6 == 1) && (W6X_NET_IPV6_ENABLE == 0)
#error "IPv6 support must be enabled for IPERF_V6"
#endif /* IPERF_V6 && W6X_NET_IPV6 */
#endif /* ST67_ARCH */

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

#define PERCENT_MULTIPLIER    100         /*!< Percentage multiplier for summary */

#define HEADER_VERSION1       0x80000000  /*!< Header version 1 mask */

#define RUN_NOW               0x00000001  /*!< Run now mask */

/** UDP packet size refers to the UDP payload size.
  * By default iperf2 has a UDP payload size of 1470 bytes in IPv4 and 1450 bytes in IPv6 for the client
  * Because of the current iperf implementation, UDPv6 packet should be 1450 */
#define UDP_PACKET_SIZE       1470        /*!< UDP default packet size */

#define UDPV6_PACKET_SIZE     1450        /*!< UDP default IPv6 packet size */

#define MIN_BUF_SIZE          512         /*!< Minimum buffer size */

#define JITTER_RX             0           /*!< Enable jitter calculation */

/** @} */

/* Private macros ------------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Iperf_Macros
  * @{
  */

/** Macro to get the maximum of two values */
#define MAX( a, b )    ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

/** @} */

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
  * @param  listen_addr: Listen address
  * @param  type: Transfer type
  */
static void socket_recv(int32_t recv_socket, struct sockaddr_storage listen_addr, uint8_t type);

/**
  * @brief  Execute the iperf client
  * @param  send_socket: Send socket
  * @param  dest_addr: Destination address
  * @param  type: Transfer type
  * @param  bw_lim: Bandwidth limit
  */
static void socket_send(int32_t send_socket, struct sockaddr_storage dest_addr, uint8_t type, int32_t bw_lim);

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
  * @param  listen_addr: Listen address
  * @param  type: Transfer type
  */
static void socket_recv_dual(int32_t recv_socket, struct sockaddr_storage listen_addr, uint8_t type);

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

  /* Start the iperf test with the provided configuration */
  if (!cfg)
  {
    /* Configuration pointer is null, cannot start */
    return IPERF_FAIL;
  }

  if (s_iperf_is_running)
  {
    /* Only one iperf instance can run at a time */
    LogDebug("iperf is running\n");
    return IPERF_FAIL;
  }

  /* Initialize control structure and copy configuration */
  memset(&s_iperf_ctrl, 0, sizeof(s_iperf_ctrl));
  memcpy(&s_iperf_ctrl.cfg, cfg, sizeof(*cfg));

  s_iperf_is_running = true;
  s_iperf_ctrl.finish = false;
  /* Mark iperf as running and not finished */

  /* Calculate the buffer length depending on the configuration */
  if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP))
  {
    /* UDP client: use default or configured buffer length for TX */
    s_iperf_ctrl.buffer_len = (s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_UDP_TX_LEN : s_iperf_ctrl.cfg.len_buf);
  }
  else if (((s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP)))
  {
    /* UDP server: use default or configured buffer length for RX */
    s_iperf_ctrl.buffer_len = (s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_UDP_RX_LEN : s_iperf_ctrl.cfg.len_buf);
  }
  else if ((s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT) && (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP))
  {
    /* TCP client: use default or configured buffer length for TX, with max check */
    if ((s_iperf_ctrl.cfg.len_buf == 0) || (s_iperf_ctrl.cfg.len_buf > IPERF_TCP_TX_LEN))
    {
      s_iperf_ctrl.buffer_len = IPERF_TCP_TX_LEN;
    }
    else
    {
      s_iperf_ctrl.buffer_len = s_iperf_ctrl.cfg.len_buf;
    }
  }
  else
  {
    /* TCP server: use default or configured buffer length for RX, with max check */
    if ((s_iperf_ctrl.cfg.len_buf == 0) || (s_iperf_ctrl.cfg.len_buf > IPERF_TCP_RX_LEN))
    {
      s_iperf_ctrl.buffer_len = IPERF_TCP_RX_LEN;
    }
    else
    {
      s_iperf_ctrl.buffer_len = s_iperf_ctrl.cfg.len_buf;
    }
  }

  /* Allocate the buffer for data transmission/reception */
  uint32_t mem_size = MAX(s_iperf_ctrl.buffer_len, MIN_BUF_SIZE);
  s_iperf_ctrl.buffer = (uint8_t *)IPERF_MALLOC(mem_size);
  if (!s_iperf_ctrl.buffer)
  {
    /* Memory allocation failed */
    LogError("[iperf] create buffer: not enough memory\n");
    return IPERF_FAIL;
  }

  /* Clear the buffer before use */
  memset(s_iperf_ctrl.buffer, 0, s_iperf_ctrl.buffer_len);

  /* Create the iperf traffic task (FreeRTOS) */
  if (pdPASS != xTaskCreate(iperf_task_traffic, IPERF_TRAFFIC_TASK_NAME, IPERF_TRAFFIC_TASK_STACK >> 2,
                            NULL, s_iperf_ctrl.cfg.traffic_task_priority, NULL))
  {
    /* Task creation failed, clean up and return error */
    LogError("[iperf] create task %s failed\n", IPERF_TRAFFIC_TASK_NAME);
    if (s_iperf_ctrl.buffer != NULL)
    {
      IPERF_FREE(s_iperf_ctrl.buffer);
      s_iperf_ctrl.buffer = NULL;
    }
    return IPERF_FAIL;
  }

  /* Successfully started iperf test */
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
    /* Set finish flag to abort the running iperf test */
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
  pT->usec = (system_tick % configTICK_RATE_HZ) * 1000;
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
        actual_transfer = (s_iperf_ctrl.actual_len * 100) / (1024 * 1024);
        /* Calculate the actual bandwidth
           Formula is: (Bytes * 8 * 10 / (1000 * 1000)) / interval duration */
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

static void socket_recv(int32_t recv_socket, struct sockaddr_storage listen_addr, uint8_t type)
{
  /* Receive data from a socket and collect statistics for iperf */
  bool iperf_recv_running = false;
  uint8_t timeout_count = 2;
  int32_t want_recv = 0;
  int32_t actual_recv = 0;
  int32_t datagramID;
#if (IPERF_V6 == 1)
  uint32_t udp_payload_size = (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6) ? \
                              UDPV6_PACKET_SIZE : UDP_PACKET_SIZE;
#else
  uint32_t udp_payload_size = UDP_PACKET_SIZE;
#endif /* IPERF_V6 & LWIP_IPV6 */
  iperf_time_struct_t lastPacketTime;
  iperf_time_struct_t firstPacketTime;
#if JITTER_RX
  iperf_time_struct_t packetTime;
  iperf_time_struct_t sentTime;
  long lastTransit = 0;
#endif /* JITTER_RX */
  int32_t lastPacketID = 0;
  Transfer_Info_t stats;
#if (IPERF_V6 == 1)
  socklen_t socklen = (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6) ? \
                      sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
#else
  socklen_t socklen = sizeof(struct sockaddr_in);
#endif /* IPERF_V6 & LWIP_IPV6 */
  const char *error_log = (type == IPERF_TRANS_TYPE_TCP) ? "tcp server recv" : "udp server recv";

  s_iperf_ctrl.sockfd = recv_socket;
  want_recv = s_iperf_ctrl.buffer_len;

  memset(&stats, 0, sizeof(Transfer_Info_t));

  uint32_t leftover_data = 0;
  while (!s_iperf_ctrl.finish) /* Start the iperf traffic task in server mode */
  {
    /* Receive data */
    if (type == IPERF_TRANS_TYPE_UDP)
    {
      actual_recv = NET_RECVFROM(recv_socket, s_iperf_ctrl.buffer + leftover_data, want_recv - leftover_data, 0,
                                 (struct sockaddr *)&listen_addr, &socklen);
    }
    else
    {
      actual_recv = NET_RECV(recv_socket, s_iperf_ctrl.buffer, want_recv, 0);
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
      if ((s_iperf_ctrl.cfg.num_bytes > 0) && (s_iperf_ctrl.tot_len > s_iperf_ctrl.cfg.num_bytes))
      {
        break;
      }

      if (type == IPERF_TRANS_TYPE_UDP)
      {
        uint32_t datagrams_counter = 0;
        while (actual_recv + leftover_data >= udp_payload_size * (datagrams_counter + 1))
        {
          if (s_iperf_ctrl.buffer == NULL)
          {
            s_iperf_ctrl.finish = true;
            break;
          }
          stats.cntDatagrams++;

          /* Get the datagram ID from the received packet */
          datagramID = PP_HTONL(((UDP_datagram_t *)(s_iperf_ctrl.buffer + (udp_payload_size * datagrams_counter)))->id);

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
            /* Never decrease datagramID (e.g. if an out-of-order packet is received) */
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
              if (s_iperf_ctrl.buffer == NULL)
              {
                break;
              }
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
              if (lastPacketTime.usec >= firstPacketTime.usec)
              {
                server_hdr->stop_sec     = PP_HTONL((long)(lastPacketTime.sec - firstPacketTime.sec));
                server_hdr->stop_usec    = PP_HTONL((long)(lastPacketTime.usec - firstPacketTime.usec));
              }
              else
              {
                server_hdr->stop_sec     = PP_HTONL((long)(lastPacketTime.sec - 1 - firstPacketTime.sec));
                server_hdr->stop_usec    = PP_HTONL((long)(1000000 + lastPacketTime.usec - firstPacketTime.usec));
              }
              server_hdr->error_cnt    = PP_HTONL(stats.cntError);
              server_hdr->outorder_cnt = PP_HTONL(stats.cntOutofOrder);
              server_hdr->datagrams    = PP_HTONL(stats.cntDatagrams + stats.cntError);
              server_hdr->jitter1      = PP_HTONL((long) stats.jitter);
              server_hdr->jitter2      = PP_HTONL((long)((stats.jitter - (long)stats.jitter) * 1e6));

              /* The following values used for display are by 10 or 100 depending on the desired precision
                 so that the decimal part can be displayed without using float */
              int32_t total_transfer = (s_iperf_ctrl.tot_len) * 10 / (1024 * 1024);
              /* Calculate the average bandwidth from the start */
              int32_t average = (s_iperf_ctrl.tot_len * 8) / 10000 / stats.endTime;
              /* Calculate the error percentage to be printed */
              int32_t err_percentage = PERCENT_MULTIPLIER * 10 * stats.cntError / (stats.cntDatagrams + stats.cntError);

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
            NET_SENDTO(recv_socket, s_iperf_ctrl.buffer, want_recv, 0, (struct sockaddr *)&listen_addr, socklen);

            /* Limits to to 1 sec */

            int32_t to = LAST_SOCKET_TIMEOUT;
            int32_t len = sizeof(to);

            (void)NET_SETSOCKOPT(recv_socket, SOL_SOCKET, SO_RCVTIMEO, &to, len);

            /* Wait for the last packet to flush remaining data */
            (void)NET_RECVFROM(recv_socket, s_iperf_ctrl.buffer, want_recv, 0,
                               (struct sockaddr *)&listen_addr, &socklen);
          }
          datagrams_counter++;
        }
        if (s_iperf_ctrl.buffer == NULL)
        {
          s_iperf_ctrl.finish = true;
          break;
        }
        uint32_t data_start = udp_payload_size * datagrams_counter;
        leftover_data = actual_recv + leftover_data - data_start;
        for (uint32_t i = 0; i < leftover_data; i++)
        {
          s_iperf_ctrl.buffer[i] = s_iperf_ctrl.buffer[data_start + i];
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

static void socket_send(int32_t send_socket, struct sockaddr_storage dest_addr, uint8_t type, int32_t bw_lim)
{
  /* Send data to a socket according to iperf configuration */
  UDP_datagram_t *hdr;
  int32_t pkt_cnt = 0;
  int32_t want_send = 0;
  int32_t delay_target = -1;
  int32_t delay_us = 0;
  int64_t adjust = 0;
  iperf_time_struct_t lastPacketTime;
  iperf_time_struct_t time = {0};
#if (IPERF_V6 == 1)
  uint32_t udp_payload_size = (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6) ? \
                              UDPV6_PACKET_SIZE : UDP_PACKET_SIZE;
#else
  uint32_t udp_payload_size = UDP_PACKET_SIZE;
#endif /* IPERF_V6 & LWIP_IPV6 */
#if (IPERF_V6 == 1)
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
    send_dual_header(send_socket, (struct sockaddr *)&dest_addr, socklen);
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
      /* If the delay is positive,
         it means that the received packet is ahead of schedule and the loop needs to be delayed */
      if ((adjust > 0) || (delay_us > 0))
      {
        delay_us += adjust;
      }
    }
    int32_t subpacket_count = 0;
    if (s_iperf_ctrl.buffer == NULL)
    {
      LogError("Failed to point to the datagram header\n");
      s_iperf_ctrl.finish = true;
      break;
    }
    /* Make sure the datagram header will fit within the remaining space */
    hdr = (UDP_datagram_t *)s_iperf_ctrl.buffer;
    hdr->tv_sec = PP_HTONL(time.sec);
    hdr->tv_usec = PP_HTONL(time.usec);
    while ((subpacket_count * udp_payload_size + sizeof(UDP_datagram_t)) < want_send)
    {
      hdr = (UDP_datagram_t *)(s_iperf_ctrl.buffer + subpacket_count * udp_payload_size);
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
        currLen = NET_SENDTO(send_socket, s_iperf_ctrl.buffer, buffer_len, 0,
                             (struct sockaddr *)&dest_addr, socklen);
      }
      else
      {
        currLen = NET_SEND(send_socket, s_iperf_ctrl.buffer, buffer_len, 0);
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
        iperf_show_socket_error_reason(error_log, send_socket);
#if (ST67_ARCH == W6X_ARCH_T02)
        s_iperf_ctrl.finish = true; /* Stop the iperf traffic task */
        break;
#endif /* ST67_ARCH */
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

    /* The send delay may be negative, it indicates that the received packet is trying
     * to catch up and hence to not delay the loop at all. */
    if (delay_us > 2000)
    {
      if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
      {
        vTaskDelay(pdMS_TO_TICKS(delay_us / 1000)); /* Parameter should be in system ticks */
      }
    }
  }

  if (type == IPERF_TRANS_TYPE_UDP)
  {
    if (s_iperf_ctrl.buffer == NULL)
    {
      LogError("Failed to point to the datagram header\n");
      s_iperf_ctrl.finish = true;
      return;
    }
    hdr = (UDP_datagram_t *)s_iperf_ctrl.buffer;
    hdr->id = (int32_t)PP_HTONL(-pkt_cnt);
    hdr->tv_sec = PP_HTONL(time.sec);
    hdr->tv_usec = PP_HTONL(time.usec);

    int32_t count = 0;
    int32_t rc;

    /* Limits to to 1 sec */
    int32_t to = LAST_SOCKET_TIMEOUT;
    int32_t len = sizeof(to);

    (void)NET_SETSOCKOPT(send_socket, SOL_SOCKET, SO_RCVTIMEO, &to, len);

    while (count < 2) /* Try to receive the report packet from the server */
    {
      count++;
      /* Write data */
      (void)NET_SENDTO(send_socket, s_iperf_ctrl.buffer, want_send, 0, (struct sockaddr *)&dest_addr, socklen);
      struct sockaddr_storage source_addr;
      socklen_t rxsocklen = socklen;
      rc = NET_RECVFROM(send_socket, s_iperf_ctrl.buffer, want_send, 0,
                        (struct sockaddr *)&source_addr, &rxsocklen);
      if (rc > 0)
      {
        /* Check if the receive packet is the report packet */
        if (rc >= (int)(sizeof(UDP_datagram_t) + sizeof(iperf_server_hdr_t)))
        {
          while (!s_iperf_ctrl.finish); /* Wait the end of report task */

          /* Process and print the report packet */
          iperf_server_hdr_t *server_hdr = (iperf_server_hdr_t *)&s_iperf_ctrl.buffer[sizeof(UDP_datagram_t)];
          int32_t datagram_count = PP_HTONL(server_hdr->datagrams);
          int32_t error_count = PP_HTONL(server_hdr->error_cnt);
          int32_t percent_error = 0;
          if (datagram_count > 0)
          {
            percent_error = PERCENT_MULTIPLIER * error_count / datagram_count;
          }

          LogInfo("[ ID]  Jitter        Lost/Total Datagrams\n");
          LogInfo("[%3" PRIi32 "] %" PRIu32 ".%03" PRIu32 " ms    %4" PRIu32 "/%5" PRIu32 " (%3" PRIu32 "%%)\n",
                  send_socket,
                  (uint32_t)PP_HTONL(server_hdr->jitter1) * 1000, (uint32_t)PP_HTONL(server_hdr->jitter2) * 1000,
                  (uint32_t)error_count, (uint32_t)datagram_count, percent_error);
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
  /* Run the iperf TCP server, sending data to the client to measure throughput */
  int32_t listen_socket = -1;
  int32_t client_socket = -1;
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_in remote_addr;
  int32_t timeout = 0;
  socklen_t addr_len = sizeof(struct sockaddr);
  struct sockaddr_storage listen_addr = { 0 };
  struct sockaddr_in listen_addr4 = { 0 };

  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

#if (IPERF_V6 == 1)
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {
#if (ST67_ARCH == W6X_ARCH_T02)
    int32_t opt = 1;
#endif /* ST67_ARCH */
    struct sockaddr_in6 listen_addr6 = { 0 };

    /* The TCP server listens on "::" (all interfaces). */
#if (ST67_ARCH == W6X_ARCH_T01)
    (void)NET_INET6_ATON("::", &listen_addr6.sin6_addr);
#elif (ST67_ARCH == W6X_ARCH_T02)
    inet6_aton("::", &listen_addr6.sin6_addr);
#endif /* ST67_ARCH */
    listen_addr6.sin6_family = AF_INET6;
    listen_addr6.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.sport);
#if (ST67_ARCH == W6X_ARCH_T01)
    listen_socket = NET_SOCKET(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#elif (ST67_ARCH == W6X_ARCH_T02)
    listen_socket = NET_SOCKET(AF_INET6, SOCK_STREAM, IPPROTO_IPV6);
#endif  /* ST67_ARCH */
    if (listen_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
#if (ST67_ARCH == W6X_ARCH_T02)
    (void)NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    (void)NET_SETSOCKOPT(listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif  /* ST67_ARCH */

    err = NET_BIND(listen_socket, (struct sockaddr *)&listen_addr6, sizeof(listen_addr6));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 ", IPPROTO: %" PRIu16 "\n", errno, AF_INET6);
      goto exit;
    }

    err = NET_LISTEN(listen_socket, 1);
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Error occurred during listen: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    timeout = IPERF_SOCKET_RX_TIMEOUT * 1000;
    (void)NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memcpy(&listen_addr, &listen_addr6, sizeof(listen_addr6));
  }
  else if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    listen_addr4.sin_family = AF_INET;
    listen_addr4.sin_port = PP_HTONS(s_iperf_ctrl.cfg.sport);
    listen_addr4.sin_addr.s_addr = s_iperf_ctrl.cfg.source_ip4;

    listen_socket = NET_SOCKET(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    /* NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); */
    err = NET_BIND(listen_socket, (struct sockaddr *)&listen_addr4, sizeof(listen_addr4));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 ", IPPROTO: %" PRIu16 "\n", (int32_t)errno, AF_INET);
      goto exit;
    }

    err = NET_LISTEN(listen_socket, 5);
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Error occurred during listen: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    memcpy(&listen_addr, &listen_addr4, sizeof(listen_addr4));
  }

  client_socket = NET_ACCEPT(listen_socket, (struct sockaddr *)&remote_addr, &addr_len);
  if (client_socket < 0)
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Unable to accept connection: errno %" PRIi32 "\n", (int32_t)errno);
    goto exit;
  }

  timeout = TCP_RX_SOCKET_TIMEOUT;
  (void)NET_SETSOCKOPT(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  socket_recv(client_socket, listen_addr, IPERF_TRANS_TYPE_TCP);
exit:
  if (client_socket != -1)
  {
    NET_CLOSE(client_socket);
  }

  if (listen_socket != -1)
  {
    NET_SHUTDOWN(listen_socket, 1);
    NET_CLOSE(listen_socket);
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

static iperf_err_t iperf_run_tcp_client(void)
{
  /* Run the iperf TCP client test, sending data to the server and measuring throughput */
  int32_t client_socket = -1;
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_storage dest_addr = { 0 };
#if IPERF_V6
  struct sockaddr_in6 dest_addr6 = { 0 };
#endif /* IPERF_V6 */
  struct sockaddr_in dest_addr4 = { 0 };

  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
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

#if (IPERF_V6 == 1)
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {

#if (ST67_ARCH == W6X_ARCH_T02)
    int32_t opt = s_iperf_ctrl.cfg.tos;
    struct netif *netif = netif_get_interface(NETIF_STA);
#endif /* ST67_ARCH */

#if (ST67_ARCH == W6X_ARCH_T01)
    client_socket = NET_SOCKET(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#elif (ST67_ARCH == W6X_ARCH_T02)
    client_socket = NET_SOCKET(AF_INET6, SOCK_STREAM, IPPROTO_IPV6);
#endif  /* ST67_ARCH */
    if (client_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
#if (ST67_ARCH == W6X_ARCH_T02)
    ip6_addr_t const *src_ip6 = netif_ip6_addr(netif, 0);
    if ((src_ip6 == NULL) || ip6_addr_isany(src_ip6))
    {
      LogError("No valid IPv6 address assigned to default interface\n");
      goto exit;
    }

    struct sockaddr_in6 src_addr = {0};
    src_addr.sin6_family = AF_INET6;
    memcpy(&src_addr.sin6_addr, src_ip6, sizeof(src_addr.sin6_addr));
    src_addr.sin6_port = 0;

    if (NET_BIND(client_socket, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0)
    {
      LogError("Failed to bind socket to source address: %d\n", errno);
      NET_CLOSE(client_socket);
      goto exit;
    }

    (void)NET_SETSOCKOPT(client_socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt));
#endif  /* ST67_ARCH */

    memcpy(&dest_addr6.sin6_addr, s_iperf_ctrl.cfg.destination_ip6, sizeof(dest_addr6.sin6_addr));
    dest_addr6.sin6_family = AF_INET6;
    dest_addr6.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.dport);

    err = NET_CONNECT(client_socket, (struct sockaddr *)&dest_addr6, sizeof(struct sockaddr_in6));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to connect: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    memcpy(&dest_addr, &dest_addr6, sizeof(dest_addr6));
  }
  else if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    client_socket = NET_SOCKET(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    /* NET_SETSOCKOPT(client_socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt)); */

    dest_addr4.sin_family = AF_INET;
    dest_addr4.sin_port = PP_HTONS(s_iperf_ctrl.cfg.dport);
    dest_addr4.sin_addr.s_addr = s_iperf_ctrl.cfg.destination_ip4;
    err = NET_CONNECT(client_socket, (struct sockaddr *)&dest_addr4, sizeof(struct sockaddr_in));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to connect: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    memcpy(&dest_addr, &dest_addr4, sizeof(dest_addr4));
  }

  socket_send(client_socket, dest_addr, IPERF_TRANS_TYPE_TCP, s_iperf_ctrl.cfg.bw_lim);
exit:
  if (client_socket != -1)
  {
    NET_SHUTDOWN(client_socket, 0);
    NET_CLOSE(client_socket);
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

static iperf_err_t iperf_run_udp_server(void)
{
  /* Run the iperf UDP server test, receiving datagrams and collecting statistics */
  int32_t listen_socket = -1;
#if (ST67_ARCH == W6X_ARCH_T02)
  int32_t opt = 1;
#endif /* ST67_ARCH */
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
#if (ST67_ARCH == W6X_ARCH_T01)
  int32_t timeout = IPERF_SOCKET_RX_TIMEOUT * 1000;
#elif (ST67_ARCH == W6X_ARCH_T02)
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int32_t timeout = IPERF_SOCKET_RX_TIMEOUT * 1000;
#else
  struct timeval timeout;
  timeout.tv_sec = IPERF_SOCKET_RX_TIMEOUT;
  timeout.tv_usec = 0;
#endif /* LWIP_SO_SNDRCVTIMEO_NONSTANDARD */
#endif /* ST67_ARCH */
  struct sockaddr_storage listen_addr = { 0 };
  struct sockaddr_in listen_addr4 = { 0 };

  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

#if (IPERF_V6 == 1)
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {
    struct sockaddr_in6 listen_addr6 = { 0 };

    /* The UDP server listens on "::" (all interfaces). */
#if (ST67_ARCH == W6X_ARCH_T01)
    (void)NET_INET6_ATON("::", &listen_addr6.sin6_addr);
#elif (ST67_ARCH == W6X_ARCH_T02)
    inet6_aton("::", &listen_addr6.sin6_addr);
#endif /* ST67_ARCH */
    listen_addr6.sin6_family = AF_INET6;
    listen_addr6.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.sport);

    listen_socket = NET_SOCKET(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (listen_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

#if (ST67_ARCH == W6X_ARCH_T01)
    (void)NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#elif (ST67_ARCH == W6X_ARCH_T02)
    (void)NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif  /* ST67_ARCH */

    err = NET_BIND(listen_socket, (struct sockaddr *)&listen_addr6, sizeof(struct sockaddr_in6));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    memcpy(&listen_addr, &listen_addr6, sizeof(listen_addr6));
  }
  else if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    listen_addr4.sin_family = AF_INET;
    listen_addr4.sin_port = PP_HTONS(s_iperf_ctrl.cfg.sport);
    listen_addr4.sin_addr.s_addr = s_iperf_ctrl.cfg.source_ip4;

    listen_socket = NET_SOCKET(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listen_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

#if (ST67_ARCH == W6X_ARCH_T01)
    (void)NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#elif (ST67_ARCH == W6X_ARCH_T02)
    (void)NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif /* ST67_ARCH */

    err = NET_BIND(listen_socket, (struct sockaddr *)&listen_addr4, sizeof(struct sockaddr_in));
    if (err != 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    memcpy(&listen_addr, &listen_addr4, sizeof(listen_addr4));
  }

#if (ST67_ARCH == W6X_ARCH_T02)
  (void)NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#endif /* ST67_ARCH */

  socket_recv(listen_socket, listen_addr, IPERF_TRANS_TYPE_UDP);
exit:
  if (listen_socket != -1)
  {
    NET_SHUTDOWN(listen_socket, 1);
    NET_CLOSE(listen_socket);
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

static iperf_err_t iperf_run_udp_client(void)
{
  /* Run the iperf UDP client test, sending datagrams to the server and measuring performance */
  int32_t client_socket = -1;
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_storage dest_addr = { 0 };
#if IPERF_V6
  struct sockaddr_in6 dest_addr6 = { 0 };
#endif /* IPERF_V6 */
  struct sockaddr_in dest_addr4 = { 0 };

  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
  {
    ret = IPERF_FAIL;
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

#if (IPERF_V6 == 1)
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
  {
#if (ST67_ARCH == W6X_ARCH_T02)
    int32_t opt = 1;
    struct netif *netif = netif_get_interface(NETIF_STA);
#endif  /* ST67_ARCH */
    client_socket = NET_SOCKET(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

#if (ST67_ARCH == W6X_ARCH_T02)
    ip6_addr_t const *src_ip6 = netif_ip6_addr(netif, 0);
    if ((src_ip6 == NULL) || ip6_addr_isany(src_ip6))
    {
      LogError("No valid IPv6 address assigned to default interface\n");
      goto exit;
    }

    struct sockaddr_in6 src_addr = {0};
    src_addr.sin6_family = AF_INET6;
    memcpy(&src_addr.sin6_addr, src_ip6, sizeof(src_addr.sin6_addr));
    src_addr.sin6_port = 0;

    if (NET_BIND(client_socket, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0)
    {
      LogError("Failed to bind socket to source address: %d\n", errno);
      NET_CLOSE(client_socket);
      goto exit;
    }
    (void)NET_SETSOCKOPT(client_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif  /* ST67_ARCH */

#if 0
    opt = s_iperf_ctrl.cfg.tos;
    (void)NET_SETSOCKOPT(client_socket, IPPROTO_IP, IP_TOS, &opt, sizeof(opt));
#endif /* 0 */

#if (ST67_ARCH == W6X_ARCH_T02)
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int32_t timeout = 10 * 1000;
#else
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
#endif /* LWIP_SO_SNDRCVTIMEO_NONSTANDARD */
    /* Set timeout */
    (void)NET_SETSOCKOPT(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif /* ST67_ARCH */

    /* NET_SETSOCKOPT(client_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)); */

    memcpy(&dest_addr6.sin6_addr, s_iperf_ctrl.cfg.destination_ip6, sizeof(dest_addr6.sin6_addr));
    dest_addr6.sin6_family = AF_INET6;
    dest_addr6.sin6_port = PP_HTONS(s_iperf_ctrl.cfg.dport);

    memcpy(&dest_addr, &dest_addr6, sizeof(dest_addr6));

    if (dest_addr.ss_family == AF_INET6)
    {
      char addr_str[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&dest_addr;
      if (NET_INET_NTOP(AF_INET6, &addr_in6->sin6_addr, addr_str, sizeof(addr_str)) == NULL)
      {
        LogError("[iperf] inet_ntop error\n");
        return 1;
      }
      LogInfo("\n\r IPv6 address: %s   Port: %d\n\r", addr_str, PP_NTOHS(addr_in6->sin6_port));
    }
    else
    {
      LogError("[iperf] Address is not of type AF_INET6.\n");
    }
  }
  else if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
#endif /* IPERF_V6 */
  {
    dest_addr4.sin_family = AF_INET;
    dest_addr4.sin_port = PP_HTONS(s_iperf_ctrl.cfg.dport);
    dest_addr4.sin_addr.s_addr = s_iperf_ctrl.cfg.destination_ip4;

    client_socket = NET_SOCKET(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket < 0)
    {
      ret = IPERF_FAIL;
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    memcpy(&dest_addr, &dest_addr4, sizeof(dest_addr4));
  }

  socket_send(client_socket, dest_addr, IPERF_TRANS_TYPE_UDP, s_iperf_ctrl.cfg.bw_lim);
exit:
  if (client_socket != -1)
  {
    /* NET_SHUTDOWN(client_socket, 0); */
    NET_CLOSE(client_socket);
  }
  s_iperf_ctrl.finish = true;
  return ret;
}

void iperf_task_traffic(void *arg)
{
  (void)s_iperf_is_running;

  /* Save and disable low power config */
  if ((W6X_GetPowerMode(&s_iperf_ctrl.ps_mode) != W6X_STATUS_OK) || (W6X_SetPowerMode(0) != W6X_STATUS_OK))
  {
    goto _err1;
  }

  LogInfo("------------------------------------------------------------\n");
  if (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP)
  {
    LogInfo("[UDP]");
  }
  else if (s_iperf_ctrl.cfg.flag & IPERF_FLAG_TCP)
  {
    LogInfo("[TCP]");
  }

  if (s_iperf_ctrl.cfg.flag & IPERF_FLAG_SERVER)
  {
    LogInfo(" Server listening on port %" PRIu16 "\n", s_iperf_ctrl.cfg.sport);
  }
  else if (s_iperf_ctrl.cfg.flag & IPERF_FLAG_CLIENT)
  {
    char ipaddr[INET6_ADDRSTRLEN] = {0};
    if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
    {
      (void)NET_INET_NTOP(AF_INET, (void *) & (s_iperf_ctrl.cfg.destination_ip4), ipaddr, INET_ADDRSTRLEN);
    }
    else if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV6)
    {
      (void)NET_INET_NTOP(AF_INET6, (void *) & (s_iperf_ctrl.cfg.destination_ip6), ipaddr, INET6_ADDRSTRLEN);
    }
    LogInfo(" Client connecting to %s, port %" PRIu16 "\n", ipaddr, s_iperf_ctrl.cfg.dport);
    if (s_iperf_ctrl.cfg.flag & IPERF_FLAG_UDP)
    {
      LogInfo("Sending %" PRIu16 " bytes datagrams\n",
              s_iperf_ctrl.cfg.len_buf == 0 ? IPERF_UDP_TX_LEN : s_iperf_ctrl.cfg.len_buf);
    }
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

  /* Restore low power config */
  (void)W6X_SetPowerMode(s_iperf_ctrl.ps_mode);

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
static void socket_recv_dual(int32_t recv_socket, struct sockaddr_storage listen_addr, uint8_t type)
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
    actual_recv = NET_RECVFROM(recv_socket, buffer, want_recv, 0, (struct sockaddr *)&listen_addr, &socklen);
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

  NET_SENDTO(sock, &hdr, sizeof(hdr), 0, addr, socklen);
}

static void iperf_tcp_dual_server_task(void *pvParameters)
{
  int32_t listen_socket = -1;
  int32_t client_socket = -1;
  int32_t err = 0;
  iperf_err_t ret = IPERF_OK;
  struct sockaddr_in remote_addr;
  int32_t timeout = { 0 };
  socklen_t addr_len = sizeof(struct sockaddr);
  struct sockaddr_storage listen_addr = { 0 };
  struct sockaddr_in listen_addr4 = { 0 };

  if ((s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV6) && (s_iperf_ctrl.cfg.type != IPERF_IP_TYPE_IPV4))
  {
    LogError("[iperf] Invalid AF types\n");
    goto exit;
  }

  (void)ret;
  if (s_iperf_ctrl.cfg.type == IPERF_IP_TYPE_IPV4)
  {
    listen_addr4.sin_family = AF_INET;
    listen_addr4.sin_port = PP_HTONS(s_iperf_ctrl.cfg.sport);
    listen_addr4.sin_addr.s_addr = s_iperf_ctrl.cfg.source_ip4;

    listen_socket = NET_SOCKET(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket < 0)
    {
      LogError("[iperf] Unable to create socket: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }

    /* NET_SETSOCKOPT(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); */

    err = NET_BIND(listen_socket, (struct sockaddr *)&listen_addr4, sizeof(listen_addr4));
    if (err != 0)
    {
      LogError("[iperf] Socket unable to bind: errno %" PRIi32 ", IPPROTO: %" PRIu16 "\n", errno, AF_INET);
      goto exit;
    }

    err = NET_LISTEN(listen_socket, 5);
    if (err != 0)
    {
      LogError("[iperf] Error occurred during listen: errno %" PRIi32 "\n", (int32_t)errno);
      goto exit;
    }
    memcpy(&listen_addr, &listen_addr4, sizeof(listen_addr4));
  }

  client_socket = NET_ACCEPT(listen_socket, (struct sockaddr *)&remote_addr, &addr_len);
  if (client_socket < 0)
  {
    LogError("[iperf] Unable to accept connection: errno %" PRIi32 "\n", (int32_t)errno);
    goto exit;
  }

  timeout = IPERF_SOCKET_RX_TIMEOUT * 1000;
  (void)NET_SETSOCKOPT(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  socket_recv_dual(client_socket, listen_addr, IPERF_TRANS_TYPE_TCP);
exit:
  if (client_socket != -1)
  {
    NET_CLOSE(client_socket);
  }

  if (listen_socket != -1)
  {
    NET_SHUTDOWN(listen_socket, 0);
    NET_CLOSE(listen_socket);
  }

  vTaskDelete(NULL);
}
#endif /* IPERF_DUAL_MODE */

/** @} */
