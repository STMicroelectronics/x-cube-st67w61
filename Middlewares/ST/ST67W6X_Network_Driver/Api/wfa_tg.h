/**
  ******************************************************************************
  * @file    wfa_tg.h
  * @author  GPM Application Team
  * @brief   Definitions used in Traffic Generator Modules
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
  * https://github.com/Wi-FiTestSuite/Wi-FiTestSuite-Linux-DUT/blob/master/inc/wfa_tg.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef WFA_TG_H
#define WFA_TG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "w6x_config.h"

/* Exported constants --------------------------------------------------------*/
#ifndef WFA_TG_ENABLE
/** Enable the Traffic Generator module */
#define WFA_TG_ENABLE             0
#endif /* WFA_TG_ENABLE */

/* Error codes */
#define TG_SUCCESS                0     /*!< Success */
#define TG_FAILURE                -1    /*!< Failure */
#define TG_ERROR_CONFIG           -2    /*!< Configuration error */
#define TG_ERROR_NOT_SUPPORTED    -3    /*!< Not supported */
#define TG_ERROR_BUSY             -4    /*!< Busy */
#define TG_ERROR_INVALID_ID       -5    /*!< Invalid ID */
#define TG_ERROR_MAX_STREAMS      -6    /*!< Maximum streams reached */

#define IPV4_ADDRESS_STRING_LEN   16    /*!< IPv4 address string length */
#define IPV6_ADDRESS_STRING_LEN   40    /*!< IPv6 address string length */

/* Profile Types */
#define TG_PROF_FILE_TX           1     /*!< Profile Type: File transfer */
#define TG_PROF_MCAST             2     /*!< Profile Type: Multicast */
#define TG_PROF_IPTV              3     /*!< Profile Type: IPTV */
#define TG_PROF_TRANSC            4     /*!< Profile Type: Transparent */
#define TG_PROF_CALI_RTD          5     /*!< Profile Type: Calibration RTD */
#define TG_PROF_UAPSD             6     /*!< Profile Type: UAPSD */
#define TG_PROF_LAST              7     /*!< Profile Type: Last */

/* Traffic Directions */
#define TG_DIRECT_SEND            1     /*!< Send */
#define TG_DIRECT_RECV            2     /*!< Receive */

#define TG_WMM_AC_UP0             12    /*!< User Priority 0 */
#define TG_WMM_AC_UP1             13    /*!< User Priority 1 */
#define TG_WMM_AC_UP2             14    /*!< User Priority 2 */
#define TG_WMM_AC_UP3             15    /*!< User Priority 3 */
#define TG_WMM_AC_UP4             16    /*!< User Priority 4 */
#define TG_WMM_AC_UP5             17    /*!< User Priority 5 */
#define TG_WMM_AC_UP6             18    /*!< User Priority 6 */
#define TG_WMM_AC_UP7             19    /*!< User Priority 7 */

#define TG_WMM_AC_BE              1     /*!< Best Effort */
#define TG_WMM_AC_BK              2     /*!< Background  */
#define TG_WMM_AC_VI              3     /*!< Video       */
#define TG_WMM_AC_VO              4     /*!< Voice       */
#define TG_WMM_AC_UAPSD           5     /*!< UAPSD       */

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Traffic generator statistics structure
  */
typedef struct
{
  uint32_t tx_frames;               /*!< Tx frames count */
  uint32_t rx_frames;               /*!< Rx frames count */
  uint64_t tx_bytes;                /*!< Tx bytes count */
  uint64_t rx_bytes;                /*!< Rx bytes count */
#if 0
  uint32_t out_of_sequence_frames;  /*!< Out of sequence frames count */
#endif /* 0 */
  uint32_t lost_packets;            /*!< Lost packets count */
#if 0
  uint64_t jitter;                  /*!< Jitter */
#endif /* 0 */
} tg_stats_t;

/**
  * @brief  Traffic generator profile structure
  */
typedef struct
{
  int32_t profile;                        /*!< Profile id */
  int32_t direction;                      /*!< Traffic direction */
  char dipaddr[IPV4_ADDRESS_STRING_LEN];  /*!< Destination/remote ip address */
  int32_t dport;                          /*!< Destination/remote port */
  char sipaddr[IPV4_ADDRESS_STRING_LEN];  /*!< Source/local ip address */
  int32_t sport;                          /*!< Source/local port */
  int32_t rate;                           /*!< Rate in kbps */
  int32_t duration;                       /*!< Duration in seconds */
  int32_t pksize;                         /*!< Packet size */
  int16_t traffic_class;                  /*!< VO, VI, BK, BE */
  int32_t startdelay;                     /*!< Start delay in seconds */
  int32_t maxcnt;                         /*!< Maximum count */
  int32_t hti;                            /*!< Header template index */
} tg_profile_t;

/** Callback invoked when the send operation is finished */
typedef void (*tg_send_done_cb_t)(int32_t stream_id, tg_stats_t stats);

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Create a new stream
  * @return Stream ID
  */
int32_t wfa_tg_new_stream(void);

/**
  * @brief  Configure the selected stream with the provided profile
  * @param  stream_id: Stream ID
  * @param  profile: Profile to set
  * @return TG_SUCCESS or TG_ERROR_CONFIG
  */
int32_t wfa_tg_config(int32_t stream_id, tg_profile_t *profile);

/**
  * @brief  Reset the Traffic Generator module, clearing all stream info
  */
void wfa_tg_reset(void);

/**
  * @brief  Start receiving traffic on the specified stream according to the set profile
  * @param  stream_id: Stream ID
  * @return TG_SUCCESS or TG_FAILURE
  */
int32_t wfa_tg_recv_start(int32_t stream_id);

/**
  * @brief  Stop receiving traffic on the specified stream
  * @param  stream_id: Stream ID
  * @return TG_SUCCESS or TG_FAILURE
  */
int32_t wfa_tg_recv_stop(int32_t stream_id);

/**
  * @brief  Start sending traffic on the specified stream according to the set profile
  * @param  stream_id: Stream ID
  * @param  done_cb: Callback invoked when the send operation is finished
  * @return TG_SUCCESS or TG_FAILURE
  */
int32_t wfa_tg_send_start(int32_t stream_id, tg_send_done_cb_t done_cb);

/**
  * @brief  Get statistics from the current session. Call this after wfa_tg_recv_stop or wfa_tg_send is finished
  * @param  stream_id: Stream ID
  * @return Statistics
  */
tg_stats_t wfa_tg_get_stats(int32_t stream_id);

/**
  * @brief  Start the active echo server
  * @param  port: Port number
  * @return TG_SUCCESS or TG_FAILURE
  */
int32_t wfa_tg_start_echo_server(uint16_t port);

/**
  * @brief  Stop the active echo server
  * @return TG_SUCCESS or TG_FAILURE
  */
int32_t wfa_tg_stop_echo_server(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WFA_TG_H */
