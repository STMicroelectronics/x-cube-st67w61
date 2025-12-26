/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sntp.c
  * @author  GPM Application Team
  * @brief   This is simple "SNTP" client for the lwIP raw API.
  *          It is a minimal implementation of SNTPv4 as specified in RFC 4330.
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
/* USER CODE END Header */

/*
 * Portions of this file are based on LwIP's http client example application, LwIP version is 2.2.1.
 * Which is licensed under modified BSD-3 Clause license as indicated below.
 * See https://savannah.nongnu.org/projects/lwip/ for more information.
 *
 * Reference source :
 * https://github.com/lwip-tcpip/lwip/blob/master/src/apps/sntp/sntp.c
 *
 */

/*
 * Copyright (c) 2007-2009 Frederic Bernon, Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Frederic Bernon, Simon Goldschmidt
 */

/* Includes ------------------------------------------------------------------*/
#include "sntp.h"

#include "lwip.h"
#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/dhcp.h"

#include <string.h>
#include "logging.h"
#include "shell.h"

#include <FreeRTOS.h>
#include <task.h>
#include "event_groups.h"
#include "stm32h5xx_hal.h"

#if (LWIP_UDP == 0)
#error "SNTP needs UDP to be enabled"
#endif /* LWIP_UDP == 0 */
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
#if defined(HAL_RTC_MODULE_ENABLED)
extern RTC_HandleTypeDef hrtc;
#endif /* HAL_RTC_MODULE_ENABLED */
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  64-bit NTP timestamp, in network byte order
  */
struct sntp_time
{
  uint32_t sec;                   /*!< Seconds */
  uint32_t frac;                  /*!< Fraction */
};

/**
  * @brief  Timestamp to be extracted from the NTP header
  */
struct sntp_timestamp
{
  struct sntp_time xmit;          /*!< Transmit timestamp */
};

#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/bpstruct.h"
#endif /* PACK_STRUCT_USE_INCLUDES */
PACK_STRUCT_BEGIN
/**
  * @brief  SNTP packet format (without optional fields)
  * Timestamps are coded as 64 bits:
  * - signed 32 bits seconds since Feb 07, 2036, 06:28:16 UTC (epoch 1)
  * - unsigned 32 bits seconds fraction (2^32 = 1 second)
  */
struct sntp_msg
{
  PACK_STRUCT_FLD_8(uint8_t  li_vn_mode);               /*!< Leap Indicator, Version and Mode */
  PACK_STRUCT_FLD_8(uint8_t  stratum);                  /*!< Stratum level of the local clock */
  PACK_STRUCT_FLD_8(uint8_t  poll);                     /*!< Maximum interval between successive messages */
  PACK_STRUCT_FLD_8(uint8_t  precision);                /*!< Precision of the local clock */
  PACK_STRUCT_FIELD(uint32_t root_delay);               /*!< Total round trip delay time */
  PACK_STRUCT_FIELD(uint32_t root_dispersion);          /*!< Max error aloud from primary clock source */
  PACK_STRUCT_FIELD(uint32_t reference_identifier);     /*!< Reference clock identifier */
  PACK_STRUCT_FIELD(uint32_t reference_timestamp[2]);   /*!< Reference time-stamp seconds */
  PACK_STRUCT_FIELD(uint32_t originate_timestamp[2]);   /*!< Originate time-stamp seconds */
  PACK_STRUCT_FIELD(uint32_t receive_timestamp[2]);     /*!< Receive time-stamp seconds */
  PACK_STRUCT_FIELD(uint32_t transmit_timestamp[2]);    /*!< Transmit time-stamp seconds */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/epstruct.h"
#endif /* PACK_STRUCT_USE_INCLUDES */

/**
  * @brief  Names/Addresses of servers
  */
struct sntp_server
{
  const char *name;               /*!< DNS name of the server */
  ip_addr_t addr;                 /*!< IP address of the server */
  uint8_t reachability;           /*!< Reachability shift register as described in RFC 5905 */
};

/**
  * @brief  SNTP client context structure definition
  */
typedef struct
{
  struct udp_pcb *sntp_pcb;       /*!< UDP protocol control block */
  struct sntp_server sntp_server; /*!< SNTP server information */
  int16_t timezone;               /*!< Timezone in hours */
  date_time_t dt;                 /*!< Date and time information */
} sntp_context_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#if SNTP_UPDATE_DELAY < 15000
#error "SNTPv4 RFC 4330 enforces a minimum update time of 15 seconds!"
#endif /* SNTP_UPDATE_DELAY */

/** SNTP message length */
#define SNTP_MSG_LEN                48

/** SNTP message offsets and modes */
#define SNTP_OFFSET_LI_VN_MODE      0

/** Leap Indicator: no warning */
#define SNTP_LI_NO_WARNING          (0x00 << 6)

/** Version Number: NTP Version 4 */
#define SNTP_VERSION                (4<<3)

/** Mode mask */
#define SNTP_MODE_MASK              0x07

/** Mode: Client */
#define SNTP_MODE_CLIENT            0x03

/** Mode: Server */
#define SNTP_MODE_SERVER            0x04

/** SNTP error code for Kiss-o'-Death */
#define SNTP_ERR_KOD                1

/** Stratum offset */
#define SNTP_OFFSET_STRATUM         1

/** Stratum: Kiss-o'-Death */
#define SNTP_STRATUM_KOD            0x00

/** Offset of the transmit timestamp in the SNTP message */
#define SNTP_OFFSET_TRANSMIT_TIME   40

/** Start year for Unix time */
#define SNTP_UNIX_START_YEAR        1970

/** Number of days in a year */
#define SNTP_DAYS_IN_YEAR           365

/** Number of seconds between 1900 (NTP time) and 1970 (Unix time) */
#define DIFF_SEC_1970_1900          ((uint32_t)2208988800L)

/** Start offset of the timestamps to extract from the SNTP packet */
#define SNTP_OFFSET_TIMESTAMPS      (SNTP_OFFSET_TRANSMIT_TIME + 8 - sizeof(struct sntp_timestamp))

/** Event bit for SNTP time received */
#define SNTP_TIME_RECEIVED          (1 << 0)

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/** Convert NTP timestamp fraction to microseconds */
#define SNTP_FRAC_TO_US(f)          ((uint32_t)(((uint64_t)(f) * 1000000UL) >> 32))

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** SNTP client context */
static sntp_context_t *sntp_ctx = NULL;

/** LWIP event group handle */
EventGroupHandle_t lwip_event_group = NULL;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Convert time_t timestamp to date_time_t structure
  * @param timestamp pointer to the time_t timestamp
  * @param dt pointer to the date_time_t structure to be filled
  * @retval int32_t 0 on success, -1 on failure
  */
static int32_t sntp_time_format(time_t *timestamp, date_time_t *dt);

/**
  * @brief  SNTP processing of received timestamp
  * @param timestamp pointer to the NTP timestamp structure
  */
static void sntp_process(const struct sntp_timestamp *timestamp);

/**
  * @brief  Initialize request struct to be sent to server
  * @param req pointer to the sntp_msg structure
  */
static void sntp_initialize_request(struct sntp_msg *req);

/**
  * @brief  Retry: send a new request (and increase retry timeout)
  * @param arg is unused (only necessary to conform to sys_timeout)
  */
static void sntp_retry(void *arg);

/**
  * @brief  Receive callback for the NTP server UDP response
  * @param  arg: user argument
  * @param  pcb: pointer to the udp_pcb structure
  * @param  p: pointer to the received pbuf
  * @param  addr: pointer to the source IP address
  * @param  port: source port number
  */
static void sntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, uint16_t port);

/**
  * @brief Send an sntp request to a server
  * @param server_addr resolved IP address of the SNTP server
  */
static void sntp_send_request(const ip_addr_t *server_addr);

/**
  * @brief  DNS found callback when using DNS names as server address
  * @param  hostname: pointer to the hostname
  * @param  ipaddr: pointer to the resolved IP address
  * @param  arg: user argument
  */
static void sntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

/**
  * @brief  Send out an sntp request
  * @param arg is unused (only necessary to conform to sys_timeout)
  */
static void sntp_request(void *arg);

#if (SHELL_ENABLE == 1)
/**
  * @brief SNTP shell command to get the current time from the SNTP server
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t sntp_shell_gettime(int32_t argc, char **argv);
#endif /* SHELL_ENABLE */

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
int32_t sntp_init(int16_t timezone)
{
  if (sntp_ctx == NULL)
  {
    sntp_ctx = pvPortMalloc(sizeof(sntp_context_t));
    if (sntp_ctx == NULL)
    {
      LogError("%s: SNTP context allocation failed\n", __func__);
      return -1;
    }
    sntp_ctx->sntp_server.name = SNTP_SERVER_ADDRESS;
    sntp_ctx->timezone = timezone;

    sntp_ctx->sntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (sntp_ctx->sntp_pcb != NULL)
    {
      udp_recv(sntp_ctx->sntp_pcb, sntp_recv, NULL);
      sys_timeout(SNTP_STARTUP_DELAY, sntp_request, NULL);
    }
    else
    {
      vPortFree(sntp_ctx);
      sntp_ctx = NULL;
      LogError("%s: Failed to allocate udp pcb for sntp client\n", __func__);
      return -1;
    }
  }
  return 0;
}

void sntp_stop(void)
{
  if (sntp_ctx != NULL)
  {
    sys_untimeout(sntp_request, NULL);
    sys_untimeout(sntp_retry, NULL);
    udp_remove(sntp_ctx->sntp_pcb);
    vPortFree(sntp_ctx);
    sntp_ctx = NULL;
  }
}

int32_t sntp_gettime(date_time_t *dt)
{
  if (sntp_ctx == NULL || dt == NULL)
  {
    return -1;
  }
  if (sntp_ctx->dt.timestamp == 0)
  {
    return -1;
  }

#if defined(HAL_RTC_MODULE_ENABLED)
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};
  /* Get the current date and time */
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

  dt->seconds = time.Seconds;
  dt->minutes = time.Minutes;
  dt->hours   = time.Hours;
  dt->day     = date.Date;
  dt->month   = date.Month;
  dt->year    = date.Year + 2000;
  dt->day_of_week = date.WeekDay;
#else
  /* Compute the elapsed time from the last server response */
  uint32_t elapsed_time = (sys_now() / 1000) - sntp_ctx->dt.current_time;
  time_t current_timestamp = sntp_ctx->dt.timestamp + elapsed_time;

  /* Update date_time_t structure */
  sntp_time_format(&current_timestamp, &sntp_ctx->dt);
  sntp_ctx->dt.current_time = sys_now() / 1000;

  /* copy date/time info */
  memcpy(dt, &sntp_ctx->dt, sizeof(date_time_t));
#endif /* HAL_RTC_MODULE_ENABLED */

  return 0;
}

void sntp_setserver(const ip_addr_t *server)
{
  if (sntp_ctx == NULL)
  {
    return;
  }
  if (server != NULL)
  {
    sntp_ctx->sntp_server.addr = (*server);
  }
  else
  {
    ip_addr_set_zero(&sntp_ctx->sntp_server.addr);
  }
  sntp_ctx->sntp_server.name = NULL;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static int32_t sntp_time_format(time_t *timestamp, date_time_t *dt)
{
  /* locale gmtime (UTC) for embedded platforms without a full libc
   * Based on the algorithm for converting Unix timestamp -> UTC date
   * Does not handle negative years or dates before 1970 */
  static const uint16_t days_in_month[12] =
  {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };
  static const char *wday_name[] =
  {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static const char *mon_name[] =
  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  uint64_t t = (uint64_t)(*timestamp);
  dt->seconds = (uint32_t)(t % 60);
  t /= 60;
  dt->minutes = (uint32_t)(t % 60);
  t /= 60;
  dt->hours = (uint32_t)(t % 24);
  t /= 24;
  dt->day = (uint32_t)t;

  /* 1970-01-01 was a Thursday (4) */
  dt->day_of_week = (dt->day + 4) % 7;

  /* Calculate year */
  dt->year = SNTP_UNIX_START_YEAR;
  while (1)
  {
    uint32_t days_in_year = SNTP_DAYS_IN_YEAR;
    if ((dt->year % 4 == 0 && dt->year % 100 != 0) || (dt->year % 400 == 0))
    {
      days_in_year = SNTP_DAYS_IN_YEAR + 1;
    }
    if (dt->day < days_in_year) { break; }
    dt->day -= days_in_year;
    dt->year++;
  }

  /* Calculate month */
  dt->month = 0;
  for (uint32_t y = 0; y < 12; y++)
  {
    uint32_t dim = days_in_month[y];
    if (y == 1 && ((dt->year % 4 == 0 && dt->year % 100 != 0) || (dt->year % 400 == 0)))
    {
      dim = 29;
    }
    if (dt->day < dim) { break; }
    dt->day -= dim;
    dt->month++;
  }
  dt->day = dt->day + 1;
  dt->month = dt->month + 1;
  dt->timestamp = *timestamp;

  snprintf(dt->raw, sizeof(dt->raw),
           "%s %s %2" PRIu32 " %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32 " %04" PRIu32,
           wday_name[dt->day_of_week], mon_name[dt->month - 1],
           dt->day, dt->hours, dt->minutes,
           dt->seconds, dt->year);
  return 0;
}

static void sntp_process(const struct sntp_timestamp *timestamp)
{
  int32_t ret;
  int32_t sec = (int32_t)lwip_ntohl(timestamp->xmit.sec);

  if (sntp_ctx == NULL)
  {
    return;
  }

  sec += sntp_ctx->timezone * 3600;
  sntp_ctx->dt.timestamp = (uint32_t)((uint32_t)sec - DIFF_SEC_1970_1900);
  /* Get the time of the system */
  sntp_ctx->dt.current_time = sys_now() / 1000;
  ret = sntp_time_format(&sntp_ctx->dt.timestamp, &sntp_ctx->dt);

  if (ret != 0)
  {
    return;
  }

#if defined(HAL_RTC_MODULE_ENABLED)
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  /* Set the Time in RTC IP */
  time.Hours = sntp_ctx->dt.hours;
  time.Minutes = sntp_ctx->dt.minutes;
  time.Seconds = sntp_ctx->dt.seconds;
  time.TimeFormat = RTC_HOURFORMAT12_AM;
  time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  time.StoreOperation = RTC_STOREOPERATION_RESET;

  if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
  {
    LogError("Process Time failed\n");
    return;
  }

  /* Set the Date in RTC IP */
  date.Year = sntp_ctx->dt.year - 2000;    /* Convert the year to 2-digits format */
  date.Month = sntp_ctx->dt.month;
  date.Date = sntp_ctx->dt.day;
  date.WeekDay = sntp_ctx->dt.day_of_week;

  if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
  {
    LogError("Process Time failed\n");
    return;
  }
#endif /* HAL_RTC_MODULE_ENABLED */

  if (lwip_event_group != NULL)
  {
    xEventGroupSetBits(lwip_event_group, SNTP_TIME_RECEIVED);
  }
}

static void sntp_initialize_request(struct sntp_msg *req)
{
  memset(req, 0, SNTP_MSG_LEN);
  req->li_vn_mode = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;
}

static void sntp_retry(void *arg)
{
  /* set up a timer to send a retry and increase the retry delay */
  sys_untimeout(sntp_request, NULL);
  sys_timeout(SNTP_RETRY_TIMEOUT, sntp_request, NULL);
}

static void sntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, uint16_t port)
{
  struct sntp_timestamp timestamp = {0};
  uint8_t mode;
  uint8_t stratum;
  err_t err = ERR_ARG;
  if (sntp_ctx == NULL)
  {
    pbuf_free(p);
    return;
  }
  {
    /* process the response */
    if (p->tot_len == SNTP_MSG_LEN)
    {
      mode = pbuf_get_at(p, SNTP_OFFSET_LI_VN_MODE) & SNTP_MODE_MASK;
      /* if this is a SNTP response... */
      if (mode == SNTP_MODE_SERVER)
      {
        stratum = pbuf_get_at(p, SNTP_OFFSET_STRATUM);

        if (stratum == SNTP_STRATUM_KOD)
        {
          /* Kiss-of-death packet. Increase UPDATE_DELAY. */
          err = SNTP_ERR_KOD;
        }
        else
        {
          pbuf_copy_partial(p, &timestamp, sizeof(timestamp), SNTP_OFFSET_TIMESTAMPS);
          {
            /* correct answer */
            err = ERR_OK;
          }
        }
      }
      else
      {
        LogWarn("sntp_recv: Invalid mode in response: %"U16_F"\n", (uint16_t)mode);
        /* wait for correct response */
        err = ERR_TIMEOUT;
      }
    }
    else
    {
      LogWarn("sntp_recv: Invalid packet length: %"U16_F"\n", p->tot_len);
    }
  }

  pbuf_free(p);

  if (err == ERR_OK)
  {
    /* correct packet received: process it it */
    sntp_process(&timestamp);

    /* indicate that server responded */
    sntp_ctx->sntp_server.reachability |= 1;
    /* Set up timeout for next request (only if poll response was received) */
    sys_untimeout(sntp_retry, NULL);
    sys_untimeout(sntp_request, NULL);
    sys_timeout(SNTP_UPDATE_DELAY, sntp_request, NULL);
  }
  else if (err == SNTP_ERR_KOD)
  {
    /* KOD errors are only processed in case of an explicit poll response */
    /* Kiss-of-death packet. Increase UPDATE_DELAY. */
    sntp_retry(NULL);
  }
  else
  {
    /* ignore any broken packet, poll mode: retry after timeout to avoid flooding */
  }
}

static void sntp_send_request(const ip_addr_t *server_addr)
{
  struct pbuf *p;
  if (sntp_ctx == NULL)
  {
    return;
  }
  if (server_addr == NULL)
  {
    LogError("%s: NULL server address\n", __func__);
    return;
  }

  p = pbuf_alloc(PBUF_TRANSPORT, SNTP_MSG_LEN, PBUF_RAM);
  if (p != NULL)
  {
    struct sntp_msg *sntpmsg = (struct sntp_msg *)p->payload;
    /* initialize request message */
    sntp_initialize_request(sntpmsg);
    /* send request */
    udp_sendto(sntp_ctx->sntp_pcb, p, server_addr, SNTP_PORT);
    /* free the pbuf after sending it */
    pbuf_free(p);
    /* indicate new packet has been sent */
    sntp_ctx->sntp_server.reachability <<= 1;
    /* set up receive timeout: retry on timeout */
    sys_untimeout(sntp_retry, NULL);
    sys_timeout((uint32_t)SNTP_RECV_TIMEOUT, sntp_retry, NULL);
  }
  else
  {
    LogWarn("%s: Out of memory, trying again in %"U32_F" ms\n", __func__, (uint32_t)SNTP_RETRY_TIMEOUT);
    /* out of memory: set up a timer to send a retry */
    sys_untimeout(sntp_request, NULL);
    sys_timeout((uint32_t)SNTP_RETRY_TIMEOUT, sntp_request, NULL);
  }
}

static void sntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
  if (sntp_ctx == NULL)
  {
    return;
  }
  if (ipaddr != NULL)
  {
    /* Address resolved, send request */
    sntp_ctx->sntp_server.addr = *ipaddr;
    sntp_send_request(ipaddr);
  }
  else
  {
    /* DNS resolving failed */
    LogWarn("%s: Failed to resolve server address resolved\n", __func__);

    if (!netif_is_link_up(netif_get_interface(NETIF_STA)))
    {
      LogWarn("%s: Network interface is not up. Time process stopped\n", __func__);
      sntp_stop();
    }
    else
    {
      sntp_retry(NULL);
    }
  }
}

static void sntp_request(void *arg)
{
  ip_addr_t sntp_server_address = IPADDR_ANY_TYPE_INIT;
  err_t err;
  if (sntp_ctx == NULL)
  {
    return;
  }

  /* initialize SNTP server address */
  if (sntp_ctx->sntp_server.name)
  {
    /* always resolve the name and rely on dns-internal caching & timeout */
    ip_addr_set_zero(&sntp_ctx->sntp_server.addr);
    err = dns_gethostbyname(sntp_ctx->sntp_server.name, &sntp_server_address,
                            sntp_dns_found, NULL);
    if (err == ERR_INPROGRESS)
    {
      /* DNS request sent, wait for sntp_dns_found being called */
      return;
    }
    else if (err == ERR_OK)
    {
      sntp_ctx->sntp_server.addr = sntp_server_address;
    }
  }
  else
  {
    sntp_server_address = sntp_ctx->sntp_server.addr;
    err = (ip_addr_isany_val(sntp_server_address)) ? ERR_ARG : ERR_OK;
  }

  if (err == ERR_OK)
  {
    sntp_send_request(&sntp_server_address);
  }
  else
  {
    /* address conversion failed */
    LogWarn("%s: Invalid server address\n", __func__);
    if (!netif_is_link_up(netif_get_interface(NETIF_STA)))
    {
      LogWarn("%s: Network interface is not up. Time process stopped\n", __func__);
      sntp_stop();
    }
    else
    {
      sys_untimeout(sntp_retry, NULL);
      sys_timeout((uint32_t)SNTP_RETRY_TIMEOUT, sntp_retry, NULL);
    }
  }
}

#if (SHELL_ENABLE == 1)
int32_t sntp_shell_gettime(int32_t argc, char **argv)
{
  date_time_t dt = {0};
  int16_t timezone = 0;
  EventBits_t eventBits;

  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  /* Get the current timezone */
  timezone = (int16_t)atoi(argv[1]);

  /* Check UTC timezone format */
  if ((timezone < -12) || (timezone > 14))
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }

  if ((sntp_ctx == NULL) || (sntp_ctx->timezone != timezone))
  {
    if (sntp_ctx == NULL)
    {
      sntp_init(timezone);
    }
    else if (sntp_ctx->timezone != timezone)
    {
      sntp_stop();
      sntp_init(timezone);
    }
    if (sntp_ctx == NULL)
    {
      goto _err;
    }
    LogInfo("SNTP: Getting time from server (can take up to %d ms)...\n", SNTP_STARTUP_DELAY);
    /* Create the event group to wait for SNTP response */
    lwip_event_group = xEventGroupCreate();
    /* Wait to receive the first response */
    eventBits = xEventGroupWaitBits(lwip_event_group, SNTP_TIME_RECEIVED,
                                    pdFALSE, pdTRUE, pdMS_TO_TICKS(SNTP_STARTUP_DELAY + 2000));
    vEventGroupDelete(lwip_event_group);
    lwip_event_group = NULL;

    if ((eventBits & SNTP_TIME_RECEIVED) == 0)
    {
      goto _err;
    }
  }

  /* Get the time */
  if (sntp_gettime(&dt) == 0)
  {
    SHELL_PRINTF("Time: %s\n", dt.raw);
    return SHELL_STATUS_OK;
  }

_err:
  SHELL_E("Time Failure\n");
  return SHELL_STATUS_ERROR;
}

/** Shell command to get the time from SNTP server */
SHELL_CMD_EXPORT_ALIAS(sntp_shell_gettime, time, time < timezone : UTC format : range [-12; 14] >);
#endif /* SHELL_ENABLE */

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
