/**
  ******************************************************************************
  * @file    wfa_tg.c
  * @author  GPM Application Team
  * @brief   Library functions for traffic generator. They are shared with both TC and DUT agent.
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
  * Portions of this file are based on Wi-FiTestSuite,
  * which is licensed under the ISC license as indicated below.
  * See https://www.wi-fi.org/certification/wi-fi-test-tools for more information.
  *
  * Reference source:
  * https://github.com/Wi-FiTestSuite/Wi-FiTestSuite-Linux-DUT/blob/master/lib/wfa_tg.c
  */

/****************************************************************************
  *
  * Copyright (c) 2016 Wi-Fi Alliance
  *
  * Permission to use, copy, modify, and/or distribute this software for any
  * purpose with or without fee is hereby granted, provided that the above
  * copyright notice and this permission notice appear in all copies.
  *
  * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
  * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
  * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
  * USE OR PERFORMANCE OF THIS SOFTWARE.
  *
  *****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

#include "wfa_tg.h"
#include "w6x_config.h"
#include "w6x_api.h"
#include "common_parser.h"
#include "shell.h"

#include "FreeRTOS.h"
#include "task.h"

#if (WFA_TG_ENABLE == 1)

/* Private defines -----------------------------------------------------------*/
#define TG_MAX_STREAMS            4

/* Traffic generator related constant or threshold values defined here */
/** Maximum UDP packet length */
#define MAX_UDP_LEN               1470
/** Maximum receive buffer length */
#define MAX_RCV_BUF_LEN           MAX_UDP_LEN
/** Maximum name length */
#define TG_DSCP_TABLE_SIZE        16

/* stream state */
/** Stream is inactive */
#define TG_STREAM_INACTIVE        0
/** Stream is active */
#define TG_STREAM_ACTIVE          1

/* Rates and timing */
/** Multicast test rate is fixed at 50 frames/sec */
#define TG_MCAST_FRATE            50
/** G.729 50 pkt per second  = 20 ms interval */
#define TG_G_CODEC_RATE           50
/** Chunk default time in ms */
#define TG_CHUNK_DEFAULT_TIME_MS  1000

/** interval between burst sends - must be divisor of 1000 */
#define TG_CHUNK_TIME_MS          20
/** multicast chunk time in ms */
#define TG_CHUNK_MCAST_TIME_MS    100

/** bursts per second */
#define TG_CHUNK_PER_SEC(x)      (1000 / (x))
/** default burst per second */
#define TG_FRAME_INTERVAL         1000000
/** Rx default timeout in ms */
#define TG_RX_DEFAULT_TIMEOUT_MS  200
/** Rx timeout for transc in ms */
#define TG_RX_TRANSC_TIMEOUT_MS   400
/** Rx timeout for file tx in ms */
#define TG_RX_TIMEOUT_MS          2000

#if 0
/* Limited bit rate generator related constant or threshold values defined here   */
#define TG_SEND_FIX_BITRATE_MAX            25 * 1024 * 1024 /* 25 Mbits per sec per stream */
#endif /* 0 */

/* wmm defs */
#define TG_TOS_VO 0xD0  /*!< 110 1  0000 (6)  AC_VO tos/dscp values */
#define TG_TOS_VI 0xA0  /*!< 101 0  0000 (5)  AC_VI */
#define TG_TOS_BE 0x00  /*!< 000 0  0000 (0)  AC_BE */
#define TG_TOS_BK 0x20  /*!< 001 0  0000 (1)  AC_BK */
#define MAX_NAME  30    /*!< Maximum name length */

/** 255.255.255.255 */
#define INADDR_NONE         ((uint32_t)0xffffffffUL)
/** 127.0.0.1 */
#define INADDR_LOOPBACK     ((uint32_t)0x7f000001UL)
/** 0.0.0.0 */
#define INADDR_ANY          ((uint32_t)0x00000000UL)
/** 255.255.255.255 */
#define INADDR_BROADCAST    ((uint32_t)0xffffffffUL)

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Traffic generator header structure
  */
typedef struct
{
  char hdr[20];                   /*!< Always wfa */
} tg_header_t;

/**
  * @brief  Traffic generator stream structure
  */
typedef struct
{
  int32_t id;                     /*!< Stream ID */
  int32_t last_packet_SN;         /*!< Use for Jitter calculation */
  int32_t fm_interval;            /*!< Frame interval */
  int32_t rx_time_last;           /*!< Use for pkLost             */
  int32_t state;                  /*!< Indicate if the stream being active */
  int32_t socket_fd;              /*!< Socket file descriptor */
  TaskHandle_t thread;            /*!< Thread handle */
  tg_send_done_cb_t finished_cb;  /*!< Send Done Callback function */
  tg_profile_t profile;           /*!< Profile */
  tg_stats_t stats;               /*!< Statistics */
  char name[MAX_NAME];            /*!< Stream name */
} tg_stream_t;

/**
  * @brief  Time structure
  */
typedef struct
{
  long sec;                       /*!< Seconds part of timestamp */
  long usec;                      /*!< Microseconds part of timestamp */
} time_struct_t;

/* Private variables ---------------------------------------------------------*/
/** Traffic generator streams */
static tg_stream_t Streams[TG_MAX_STREAMS];
/** Stream ID counter */
static int32_t StreamIdCounter = 1;
/** Echo server thread handle */
static TaskHandle_t EchoThread = NULL;
/** Echo server go flag */
static int32_t EchoGo = 0;
/** Echo server socket file descriptor */
static int32_t EchoSock = -1;

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Convert integer to buffer. Little Endian to Big Endian
  * @param  val: Integer value
  * @param  buf: Buffer to store the value
  */
static void int2buff(int32_t val, char *buf);

/**
  * @brief  Convert buffer to integer. Big Endian to Little Endian
  * @param  buff: Buffer to read the value
  * @return Integer value
  */
static int32_t buff2int(char *buff);

/**
  * @brief  Get the real time in ms
  * @return Real time in ms
  */
static uint64_t get_real_time(void);

/**
  * @brief  Get the current time
  * @param  pT: Time structure to store the time
  * @return 0 if success, -1 otherwise
  */
static int32_t get_time(time_struct_t *pT);

/**
  * @brief  Get the stream with the specified ID
  * @param  stream_id: Stream ID
  * @return Stream pointer or NULL if not found
  */
static tg_stream_t *get_stream(int32_t stream_id);

/**
  * @brief  Create a UDP socket
  * @param  ipaddr: IP address to bind
  * @param  port: Port number
  * @return Socket file descriptor or TG_FAILURE
  */
static int32_t wfa_tg_create_udp_socket(char *ipaddr, uint16_t port);

/**
  * @brief  Send a packet on the specified stream
  * @param  stream: Stream to send the packet
  * @param  buf: Buffer containing the packet
  * @param  len: Length of the packet
  * @return TG_SUCCESS or TG_FAILURE
  */
static int32_t wfa_tg_send_packet(tg_stream_t *stream, char *buf, int32_t len);

/**
  * @brief  Receive data on the specified stream
  * @param  stream: Stream to receive the data
  * @param  recv_buf: Buffer to store the received data
  * @return Number of bytes received
  */
static int32_t wfa_tg_recv_data(tg_stream_t *stream, char *recv_buf);

/**
  * @brief  Set the send timing based on the profile and rate
  * @param  profile: Profile type
  * @param  rate: Rate in frames per second
  * @param  chunk_time_ms: Chunk time in ms
  * @param  chunk_rate: Chunk rate
  * @param  remainder: Remainder
  */
static void wfa_tg_set_send_timing(int32_t profile, int32_t rate, int32_t *chunk_time_ms, int32_t *chunk_rate,
                                   int32_t *remainder);

/**
  * @brief  RX thread function
  * @param  arg: Stream pointer
  */
static void wfa_rx_thread(void *arg);

/**
  * @brief  TX thread function
  * @param  arg: Stream pointer
  */
static void wfa_tx_thread(void *arg);

/**
  * @brief  Echo server thread function
  * @param  arg: Not used
  */
static void wfa_tg_echo_thread(void *arg);

/**
  * @brief  Create a UDP socket and connect to the specified address
  * @param  mysock: Socket file descriptor
  * @param  daddr: Destination address
  * @param  dport: Destination port
  * @return Socket file descriptor or TG_FAILURE
  */
static int32_t wfa_tg_connect_udp(int32_t mysock, char *daddr, int32_t dport);

/**
  * @brief  Start the echo server on the specified port
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t wfa_tg_start_echo_server_cmd(int32_t argc, char **argv);

/**
  * @brief  Stop the echo server
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t wfa_tg_stop_echo_server_cmd(int32_t argc, char **argv);

/* Functions Definition ------------------------------------------------------*/
int32_t wfa_tg_new_stream(void)
{
  int32_t i;
  for (i = 0; i < TG_MAX_STREAMS; i++)
  {
    if (Streams[i].id == 0)
    {
      Streams[i].id = StreamIdCounter++;
      sprintf(Streams[i].name, "wfa_stream_%" PRIi32 "_thread\n", Streams[i].id);
      return Streams[i].id;
    }
  }
  return TG_ERROR_MAX_STREAMS;
}

int32_t wfa_tg_config(int32_t stream_id, tg_profile_t *profile)
{
  if (profile == NULL)
  {
    return TG_ERROR_CONFIG;
  }

  tg_stream_t *stream = get_stream(stream_id);
  if (stream == NULL)
  {
    return TG_ERROR_INVALID_ID;
  }

  stream->profile = *profile;

  return TG_SUCCESS;
}

void wfa_tg_reset(void)
{
  int32_t i;
  for (i = 0; i < TG_MAX_STREAMS; i++)
  {
    if (Streams[i].socket_fd >= 0)
    {
      (void) W6X_Net_Close(Streams[i].socket_fd);
      Streams[i].socket_fd = -1;
    }
  }

  vTaskDelay(500);
  for (i = 0; i < TG_MAX_STREAMS; i++)
  {
    /* Sanity check */
    if (Streams[i].thread != NULL)
    {
      LogDebug("%s: killing stream %" PRIi32 " thread\n", __func__, Streams[i].id);
      vTaskDelete(Streams[i].thread);
      Streams[i].thread = NULL;
    }
    memset(&Streams[i], 0, sizeof(tg_stream_t));
    Streams[i].socket_fd = -1;
  }
}

int32_t wfa_tg_recv_start(int32_t stream_id)
{
  tg_stream_t *stream = get_stream(stream_id);
  if (stream == NULL)
  {
    return TG_ERROR_INVALID_ID;
  }

  tg_profile_t *profile = &stream->profile;
  if (profile == NULL || profile->direction != TG_DIRECT_RECV)
  {
    return TG_ERROR_CONFIG;
  }

  /* calculate the frame interval which is used to derive its jitter */
  if (profile->rate != 0 && profile->rate < 5000)
  {
    stream->fm_interval = TG_FRAME_INTERVAL / profile->rate; /* in ms */
  }
  else
  {
    stream->fm_interval = 0;
  }

  memset(&stream->stats, 0, sizeof(tg_stats_t));

  int32_t sock;
  int32_t timeout_ms = TG_RX_DEFAULT_TIMEOUT_MS;
  if (profile->profile == TG_PROF_IPTV || profile->profile == TG_PROF_FILE_TX || profile->profile == TG_PROF_MCAST)
  {
    sock = wfa_tg_create_udp_socket(profile->dipaddr, profile->dport);
    if (sock < 0)
    {
      return TG_FAILURE;
    }

    if (profile->profile == TG_PROF_MCAST)
    {
#if 0
      int32_t so = wfa_tg_join_mcast(sock, profile->dipaddr);
      if (so < 0)
      {
        LogError("%s: Join the multicast group failed\n", __func__);
        (void) W6X_Net_Close(sock);
        return TG_FAILURE;
      }
#else
      return TG_FAILURE;
#endif /* Not supported */
    }
  }
  else if (profile->profile == TG_PROF_TRANSC || profile->profile == TG_PROF_CALI_RTD)
  {
    sock = wfa_tg_create_udp_socket(profile->sipaddr, profile->sport);
    if (sock < 0)
    {
      return TG_FAILURE;
    }
    timeout_ms = TG_RX_TRANSC_TIMEOUT_MS;
  }
  else
  {
    return TG_FAILURE;
  }

  stream->socket_fd = sock;

  /* set timeout for blocking receive */
  int32_t to = timeout_ms; /* ms or us ? */
  int32_t len = sizeof(to);
  if (W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &to, len) != 0)
  {
    return TG_FAILURE;
  }

  /* mark the stream active */
  stream->state = TG_STREAM_ACTIVE;
  xTaskCreate(wfa_rx_thread, stream->name, (MAX_RCV_BUF_LEN + 2000) >> 2,
              stream, 24, &stream->thread);

  return TG_SUCCESS;
}

int32_t wfa_tg_recv_stop(int32_t stream_id)
{
  tg_stream_t *stream = get_stream(stream_id);
  if (stream == NULL)
  {
    return TG_ERROR_INVALID_ID;
  }

  tg_profile_t *profile = &stream->profile;
  if (profile == NULL || profile->direction != TG_DIRECT_RECV)
  {
    return TG_ERROR_CONFIG;
  }

  if (stream->thread)
  {
    if (stream->socket_fd >= 0)
    {
      (void) W6X_Net_Close(stream->socket_fd);
      stream->socket_fd = -1;
    }
    vTaskDelay(100);

    /* Wait for RX thread */
    while (stream->thread != NULL)
    {
      LogDebug("%s: ...\n", __func__);
      vTaskDelay(100);
    }
  }

  /* This should be cleaned up by RX thread already */
  if (stream->socket_fd >= 0)
  {
    LogError("%s: socket force closed\n", __func__);
    (void) W6X_Net_Close(stream->socket_fd);
    stream->socket_fd = -1;
  }

  stream->state = TG_STREAM_INACTIVE;

  return TG_SUCCESS;
}

tg_stats_t wfa_tg_get_stats(int32_t stream_id)
{
  tg_stream_t *stream = get_stream(stream_id);
  if (stream == NULL)
  {
    LogError("%s: Invalid stream id %" PRIi32 "\n", __func__, stream_id);
    return (tg_stats_t) {0};
  }
  return stream->stats;
}

int32_t wfa_tg_send_start(int32_t stream_id, tg_send_done_cb_t done_cb)
{
  tg_stream_t *stream = get_stream(stream_id);
  if (stream == NULL)
  {
    return TG_FAILURE;
  }

  tg_profile_t *profile = &stream->profile;

  profile = &stream->profile;
  if (profile == NULL || profile->direction != TG_DIRECT_SEND)
  {
    return TG_ERROR_CONFIG;
  }

  if (profile->duration == 0 && profile->maxcnt == 0)
  {
    return TG_ERROR_CONFIG;
  }

  memset(&stream->stats, 0, sizeof(tg_stats_t));

  int32_t sock = wfa_tg_create_udp_socket(profile->sipaddr, profile->sport);
  if (sock < 0)
  {
    return TG_FAILURE;
  }
  sock = wfa_tg_connect_udp(sock, profile->dipaddr, profile->dport);
  if (sock < 0)
  {
    return TG_FAILURE;
  }
  stream->socket_fd = sock;

#if 0
  /*
   * Set packet/socket priority TOS field
   */
  wfa_tg_set_tos(sock, profile->traffic_class);

  if ((profile->profile == TG_PROF_MCAST) && (wfa_tg_set_mcast_send(sock) != 0))
  {
    LogError("%s: Failed to set MULTICAST_TTL\n", __func__);
    return TG_ERROR_CONFIG;
  }
#else
  if (profile->profile == TG_PROF_MCAST)
  {
    return TG_ERROR_CONFIG;
  }
#endif /* Not supported */

  stream->finished_cb = done_cb;
  stream->state = TG_STREAM_ACTIVE;

  xTaskCreate(wfa_tx_thread, stream->name, (MAX_UDP_LEN + 2000) >> 2,
              stream, 24, &stream->thread);
  return TG_SUCCESS;
}

int32_t wfa_tg_start_echo_server(uint16_t port)
{
  if (EchoThread || EchoSock >= 0)
  {
    return TG_ERROR_BUSY;
  }

  EchoSock = wfa_tg_create_udp_socket(NULL, port);
  if (EchoSock < 0)
  {
    return TG_FAILURE;
  }

  /* set timeout for blocking receive */
  int32_t to = TG_RX_TIMEOUT_MS;
  int32_t len = sizeof(to);
  if (W6X_Net_Setsockopt(EchoSock, SOL_SOCKET, SO_RCVTIMEO, &to, len) != 0)
  {
    LogError("%s: setsockopt failed\n", __func__);
    return TG_FAILURE;
  }

  EchoGo = 1;
  xTaskCreate(wfa_tg_echo_thread, "wfa_tg_echo_thread", (MAX_RCV_BUF_LEN + 2000) >> 2,
              NULL, 24, &EchoThread);
  return TG_SUCCESS;
}

int32_t wfa_tg_stop_echo_server(void)
{
  if (EchoThread)
  {
    EchoGo = 0;
    vTaskDelay(100);

    /* Wait for the Echo thread */
    while (EchoThread != NULL)
    {
      LogDebug("%s: ...\n", __func__);
      vTaskDelay(100);
    }
  }

  /* This should be cleaned up by the Echo thread already */
  if (EchoSock >= 0)
  {
    LogError("%s: socket force closed\n", __func__);
    (void) W6X_Net_Shutdown(EchoSock, 1);
    EchoSock = -1;
  }

  return TG_SUCCESS;
}

/* Private Functions Definition ----------------------------------------------*/
static void int2buff(int32_t val, char *buf)
{
  char *le = (char *)&val;

  buf[0] = le[3];
  buf[1] = le[2];
  buf[2] = le[1];
  buf[3] = le[0];
}

static int32_t buff2int(char *buff)
{
  int32_t val;
  char *strval = (char *)&val;

  strval[0] = buff[3];
  strval[1] = buff[2];
  strval[2] = buff[1];
  strval[3] = buff[0];

  return val;
}

static uint64_t get_real_time(void)
{
  return xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
}

static int32_t get_time(time_struct_t *pT)
{
  uint32_t system_tick = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();

  pT->sec = system_tick / configTICK_RATE_HZ;
  pT->usec = (system_tick * portTICK_PERIOD_MS) * 1000 - (pT->sec * 1000000);

  return 0;
}

static tg_stream_t *get_stream(int32_t stream_id)
{
  if (stream_id <= 0)
  {
    return NULL;
  }
  int32_t i;
  for (i = 0; i < TG_MAX_STREAMS; i++)
  {
    if (Streams[i].id == stream_id)
    {
      return &Streams[i];
    }
  }
  return NULL;
}

static int32_t wfa_tg_create_udp_socket(char *ipaddr, uint16_t port)
{
  int32_t udpsock;             /* socket to create */
  struct sockaddr_in servAddr_t; /* Local address */

  udpsock = W6X_Net_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (udpsock < 0)
  {
    LogError("%s: socket() failed\n", __func__);
    return TG_FAILURE;
  }

  memset(&servAddr_t, 0, sizeof(servAddr_t));
  servAddr_t.sin_family = AF_INET;
  servAddr_t.sin_addr_t.s_addr = PP_HTONL(INADDR_ANY);
  servAddr_t.sin_port = PP_HTONS(port);

  if (W6X_Net_Bind(udpsock, (struct sockaddr *)&servAddr_t, sizeof(servAddr_t)) != 0)
  {
    (void) W6X_Net_Close(udpsock);
    return TG_FAILURE;
  }

  return udpsock;
}

#if 0
static int32_t wfa_tg_join_mcast(int32_t sockfd, char *mcastgroup)
{
  struct ip_mreq mcreq_t;
  int32_t so;

  mcreq_t.imr_multiaddr.s_addr = ATON_R(mcastgroup);
  mcreq_t.imr_interface.s_addr = PP_HTONL(INADDR_ANY);
  so = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mcreq_t, sizeof(mcreq_t));

  return so;
}
#endif /* Not supported */

static int32_t wfa_tg_send_packet(tg_stream_t *stream, char *buf, int32_t len)
{
  tg_profile_t *profile = &stream->profile;

  if (stream->socket_fd < 0)
  {
    return TG_FAILURE;
  }

  struct sockaddr_in to_addr_t = {0};
  to_addr_t.sin_family = AF_INET;
  if (profile->direction == TG_DIRECT_SEND)
  {
    to_addr_t.sin_addr_t.s_addr = ATON_R(profile->dipaddr);
    to_addr_t.sin_port = PP_HTONS(profile->dport);
  }
  else if (profile->direction == TG_DIRECT_RECV)
  {
    to_addr_t.sin_addr_t.s_addr = ATON_R(profile->sipaddr);
    to_addr_t.sin_port = PP_HTONS(profile->sport);
  }

  /* fill in the packet_n */
  int2buff(stream->stats.tx_frames + 1, &((tg_header_t *)buf)->hdr[8]);

  /*
   * Fill the timestamp to the header.
   */
  time_struct_t stime;
  get_time(&stime);
  int2buff(stime.sec, &((tg_header_t *)buf)->hdr[12]);
  int2buff(stime.usec, &((tg_header_t *)buf)->hdr[16]);

  int32_t bytes_sent = W6X_Net_Sendto(stream->socket_fd, buf, len, 0, (struct sockaddr *)&to_addr_t, sizeof(to_addr_t));

  if (bytes_sent != -1)
  {
    stream->stats.tx_frames++;
    stream->stats.tx_bytes += bytes_sent;

    return TG_SUCCESS;
  }
  else
  {
    LogDebug("%s: Packet send error\n", __func__);
    return -1;
  }
}

static int32_t wfa_tg_recv_data(tg_stream_t *stream, char *recv_buf)
{
  if (stream == NULL)
  {
    return TG_FAILURE;
  }

  uint32_t bytes_received = W6X_Net_Recv(stream->socket_fd, recv_buf, MAX_RCV_BUF_LEN, 0);
  if (bytes_received != -1)
  {
    stream->stats.rx_frames++;
    stream->stats.rx_bytes += bytes_received;

    /*
     *  Get the lost packet count
     */
    int32_t lost_pkts = buff2int(&((tg_header_t *)recv_buf)->hdr[8]) - 1 - stream->last_packet_SN;
    stream->stats.lost_packets += lost_pkts;
    stream->last_packet_SN = buff2int(&((tg_header_t *)recv_buf)->hdr[8]);
  }

  return bytes_received;
}

#if 0
static int32_t wfa_tg_set_tos(int32_t sockfd, int32_t tgUserPriority)
{
  int32_t tosval;

  socklen_t size = sizeof(tosval);

  if (sockfd < 0)
  {
    LogDebug("%s: socket closed\n", __func__);
    return TG_FAILURE;
  }

  if (W6X_Net_Getsockopt(sockfd, IPPROTO_IP, IP_TOS, &tosval, &size) != 0)
  {
    LogError("%s: Failed to set IP_TOS\n", __func__);
    return -1;
  }
  switch (tgUserPriority)
  {
    case TG_WMM_AC_BK: /* user priority "1" */
      /*Change this value to the ported device*/
      tosval = TG_TOS_BK;
      break;

    case TG_WMM_AC_VI: /* user priority "5" */
      /*Change this value to the ported device*/
      tosval = TG_TOS_VI;
      break;

    case TG_WMM_AC_UAPSD:
      tosval = 0x88;
      break;

    case TG_WMM_AC_VO: /* user priority "6" */
      /*Change this value to the ported device*/
      tosval = TG_TOS_VO;
      break;

    case TG_WMM_AC_BE: /* user priority "0" */
      tosval = TG_TOS_BE;
      break;

    /* For WMM-AC Program User Priority Definitions */
    case TG_WMM_AC_UP0:
      tosval = 0x00;
      break;

    case TG_WMM_AC_UP1:
      tosval = 0x20;
      break;

    case TG_WMM_AC_UP2:
      tosval = 0x40;
      break;

    case TG_WMM_AC_UP3:
      tosval = 0x60;
      break;

    case TG_WMM_AC_UP4:
      tosval = 0x80;
      break;
    case TG_WMM_AC_UP5:
      tosval = 0xa0;
      break;

    case TG_WMM_AC_UP6:
      tosval = 0xc0;
      break;

    case TG_WMM_AC_UP7:
      tosval = 0xe0;
      break;

    default:
      tosval = 0x00;
      break;
  }

  if (W6X_Net_Setsockopt(sockfd, IPPROTO_IP, IP_TOS, &tosval, sizeof(tosval)) != 0)
  {
    LogError("%s: Failed to set IP_TOS\n", __func__);
  }

  return (tosval == 0xE0) ? 0xD8 : tosval;
}

static int32_t wfa_tg_set_mcast_send(int32_t sockfd)
{
  unsigned char ttlval = 1;

  return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttlval, sizeof(ttlval));
}
#endif /* Not supported */

static void wfa_tg_set_send_timing(int32_t profile, int32_t rate, int32_t *chunk_time_ms, int32_t *chunk_rate,
                                   int32_t *remainder)
{
  if (rate == 0)
  {
    return;
  }

  *chunk_time_ms = TG_CHUNK_TIME_MS;
  switch (profile)
  {
    case TG_PROF_MCAST:
      if (rate < 500 && rate >= 50)
      {
        *chunk_time_ms = TG_CHUNK_MCAST_TIME_MS;
      }
      break;
    case TG_PROF_IPTV:
    case TG_PROF_FILE_TX:
      if (rate > 0 && rate <= 50) /* typically for voice */
      {
        *chunk_time_ms = TG_CHUNK_PER_SEC(rate);
      }
      break;
    default:
      *chunk_time_ms = TG_CHUNK_TIME_MS;
      break;
  }
  int32_t chunks_per_sec = TG_CHUNK_PER_SEC(*chunk_time_ms);
  *chunk_rate = rate / chunks_per_sec;
  *remainder = rate % chunks_per_sec;
}

/* Thread arg is pointer to the stream */
static void wfa_rx_thread(void *arg)
{
  tg_stream_t *stream = arg;
  tg_profile_t *profile = &stream->profile;
  char recv_buf[MAX_RCV_BUF_LEN + 1];

  if (profile == NULL || profile->direction != TG_DIRECT_RECV)
  {
    LogError("%s: Invalid profile\n", __func__);
    vTaskDelete(NULL);
    return;
  }

  LogInfo("%s: Start stream ID %" PRIi32 "\n", __func__, stream->id);

  int32_t nbytes = 0;
  const bool transaction = (profile->profile == TG_PROF_TRANSC || profile->profile == TG_PROF_CALI_RTD);
  while ((stream->socket_fd >= 0 || nbytes > 0))
  {
    nbytes = wfa_tg_recv_data(stream, (char *)recv_buf);
    if (transaction && nbytes > 0)
    {
      int32_t err = wfa_tg_send_packet(stream, recv_buf, nbytes);
      if (err != 0)
      {
        LogDebug("%s: transaction send error - skip packet\n", __func__);
      }
    }
  }

  if (stream->socket_fd >= 0)
  {
    (void) W6X_Net_Close(stream->socket_fd);
    stream->socket_fd = -1;
  }

  stream->thread = NULL;
  vTaskDelete(NULL);
  return;
}

static void wfa_tx_thread(void *arg)
{
  tg_stream_t *stream = (tg_stream_t *)arg;
  tg_profile_t *profile = &stream->profile;
  char packet_buf[MAX_UDP_LEN + 1];

  if (profile == NULL || profile->direction != TG_DIRECT_SEND)
  {
    LogError("%s: Invalid profile\n", __func__);
    vTaskDelete(NULL);
    return;
  }

  /* if delay is too long, it must be something wrong */
  if (profile->startdelay > 0 && profile->startdelay < 100)
  {
    vTaskDelay(profile->startdelay * 1000);
  }

  LogInfo("%s: Start stream ID %" PRIi32 "\n", __func__, stream->id);

  /* If RATE is 0 which means to send as much as possible, the frame size set to max UDP length */
  int32_t packet_len = 0;
  if (profile->rate == 0)
  {
    if (profile->hti == 0 && profile->pksize)
    {
      packet_len = profile->pksize;
    }
    else
    {
      packet_len = MAX_UDP_LEN;
    }
  }
  else
  {
    packet_len = profile->pksize;
  }

  if (profile->profile == TG_PROF_TRANSC)
  {
    int32_t to = 50; /* ms or us ? */
    int32_t len = sizeof(to);
    if (W6X_Net_Setsockopt(stream->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &to, len) != 0)
    {
      vTaskDelete(NULL);
      return;
    }
  }

  /* fill in the header */
  strncpy(packet_buf, "1345678", sizeof(tg_header_t));

  int32_t chunk_time_ms = TG_CHUNK_DEFAULT_TIME_MS;
  int32_t chunk_rate = 0;
  int32_t chunk_rate_remainder = 0;
  wfa_tg_set_send_timing(profile->profile, profile->rate, &chunk_time_ms, &chunk_rate, &chunk_rate_remainder);
  int32_t chunks_per_sec = TG_CHUNK_PER_SEC(chunk_time_ms);

  int32_t packet_n = 0;
  uint64_t start_time_ms = get_real_time();
  uint64_t current_time_ms = 0;
  uint64_t target_time_ms = start_time_ms;
  uint32_t rate_error_accumulator = 0;
  int32_t counter = 0; /* sent packet counter inside chunk */

  uint64_t end_time_ms = start_time_ms;
  /* 100 seconds max duration */
  end_time_ms += (profile->duration <= 0 || profile->duration > 100) ? 100000 : profile->duration * 1000;

  /* Handle the unlimited rate case */
  if (profile->rate == 0)
  {
    target_time_ms = end_time_ms;
    counter = INT32_MAX;
  }

  /* If max packet count is set, check this condition first */
  while (profile->maxcnt <= 0 || packet_n < profile->maxcnt)
  {
    current_time_ms = get_real_time();

    if (counter <= 0 || current_time_ms >= target_time_ms)
    {
      /* Finished sending current chunk, or timeout */

      if (target_time_ms >= end_time_ms)
      {
        /* Finished sending the final chunk */
        break;
      }
      if (current_time_ms >= target_time_ms && target_time_ms != start_time_ms)
      {
        LogDebug("%s: cannot keep up with the bitrate\n", __func__);
      }

      /* Set counter to number of packets to send in this chunk */
      counter = chunk_rate;

      /* Handle rate error accumulation */
      rate_error_accumulator += chunk_rate_remainder;
      if (rate_error_accumulator > 0)
      {
        /* Send extra packet if needed */
        rate_error_accumulator -= chunks_per_sec;
        counter++;
      }

      if (current_time_ms < target_time_ms)
      {
        /* Sleep until beginning of new chunk */
        vTaskDelay(target_time_ms - current_time_ms);
      }

      /* Set the beginning time of the next chunk */
      target_time_ms += chunk_time_ms;
    }

    int32_t err = wfa_tg_send_packet(stream, packet_buf, packet_len);
    if (err != 0)
    {
#if 0
      switch (err)
      {
        case EAGAIN:
        case ENOBUFS:
          LogDebug("%s: send error - try again\n", __func__);
          vTaskDelay(1);
          break;
        default:
          LogDebug("%s: send error - skip packet\n", __func__);
          counter--;
          packet_n++;
          break;
      }
#endif /* Not supported */
      LogDebug("%s: send error - skip packet\n", __func__);
      counter--;
      packet_n++;
      continue;
    }

    counter--;
    packet_n++;

    if (profile->profile == TG_PROF_TRANSC || profile->profile == TG_PROF_CALI_RTD)
    {
      int32_t nbytes = wfa_tg_recv_data(stream, (char *)packet_buf);
      if (nbytes <= 0)
      {
        LogError("%s: recv timeout\n", __func__);
      }
    }
  }

  stream->state = TG_STREAM_INACTIVE;

  if (stream->socket_fd >= 0)
  {
    (void) W6X_Net_Close(stream->socket_fd);
    stream->socket_fd = -1;
  }

  if (stream->finished_cb)
  {
    stream->finished_cb(stream->id, stream->stats);
    stream->finished_cb = NULL;
  }

  stream->thread = NULL;
  vTaskDelete(NULL);
  return;
}

static void wfa_tg_echo_thread(void *arg)
{
  struct sockaddr_storage from_addr_t;
  char buffer[MAX_RCV_BUF_LEN] = {0};
  socklen_t from_addr_len;
  int32_t len;
  uint32_t dtim = 0;
  uint32_t ps_mode = 0;

  LogInfo("%s: thread start\n", __func__);

  /* Save low power config */
  if ((W6X_WiFi_GetDTIM(&dtim) != W6X_STATUS_OK) || (W6X_GetPowerMode(&ps_mode) != W6X_STATUS_OK))
  {
    goto _err1;
  }

  /* Disable low power */
  if ((W6X_SetPowerMode(0) != W6X_STATUS_OK) || (W6X_WiFi_SetDTIM(0) != W6X_STATUS_OK))
  {
    goto _err2;
  }

  while (EchoGo && EchoSock >= 0)
  {
    from_addr_len = sizeof(from_addr_t);
    len = W6X_Net_Recvfrom(EchoSock, buffer, MAX_RCV_BUF_LEN, 0, (struct sockaddr *)&from_addr_t, &from_addr_len);
    if (len <= 0)
    {
      LogInfo("%s: no data received\n", __func__);
      continue;
    }
    LogInfo("%s: received length = %" PRIi32 "\n", __func__, len);

    len = W6X_Net_Sendto(EchoSock, (const char *)buffer, len, 0, (struct sockaddr *)&from_addr_t, from_addr_len);
    LogInfo("%s: sent length = %" PRIi32 "\n", __func__, len);
  }

  LogInfo("%s: thread exit\n", __func__);

  if (EchoSock >= 0)
  {
    (void) W6X_Net_Shutdown(EchoSock, 1);
  }
  EchoSock = -1;

_err2:
  /* Restore low power config */
  (void)W6X_SetPowerMode(ps_mode);
  (void)W6X_WiFi_SetDTIM(dtim);

_err1:
  EchoThread = NULL;
  vTaskDelete(NULL);
  return;
}

static int32_t wfa_tg_connect_udp(int32_t mysock, char *daddr, int32_t dport)
{
  struct sockaddr_in peerAddr_t;

  memset(&peerAddr_t, 0, sizeof(peerAddr_t));
  peerAddr_t.sin_family = AF_INET;
  memcpy(peerAddr_t.sin_addr_t.s4_addr, daddr, 4);
  peerAddr_t.sin_port = PP_HTONS(dport);

  if (W6X_Net_Connect(mysock, (struct sockaddr *)&peerAddr_t, sizeof(peerAddr_t)) != 0)
  {
    return -1;
  }
  return mysock;
}

int32_t wfa_tg_start_echo_server_cmd(int32_t argc, char **argv)
{
  int32_t ret;
  int32_t tmp = 0;
  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  Parser_StrToInt(argv[1], NULL, &tmp);
  uint16_t port = (uint16_t)tmp;

  ret = wfa_tg_start_echo_server(port);
  if (ret != TG_SUCCESS)
  {
    LogError("Cannot start echo server %" PRIi32 "\n", ret);
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(wfa_tg_start_echo_server_cmd, echostart,
                       echostart < port >. WFA - Starts the UDP echo server on the specified port.);

int32_t wfa_tg_stop_echo_server_cmd(int32_t argc, char **argv)
{
  if (wfa_tg_stop_echo_server() != TG_SUCCESS)
  {
    LogError("Cannot stop echo server\n");
    return SHELL_STATUS_ERROR;
  }

  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(wfa_tg_stop_echo_server_cmd, echostop,
                       echostop. WFA - Stops the UDP echo server.);

#endif /* WFA_TG_ENABLE */
