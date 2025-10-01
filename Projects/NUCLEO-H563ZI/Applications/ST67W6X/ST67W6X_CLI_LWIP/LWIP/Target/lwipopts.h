/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lwipopts.h
  * @author  GPM Application Team
  * @brief   This file overrides LwIP stack default configuration
             done in opt.h file.
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

/* Define to prevent recursive inclusion --------------------------------------*/
#ifndef __LWIPOPTS_H
#define __LWIPOPTS_H

/*-----------------------------------------------------------------------------*/
/* Current version of LwIP supported by CubeMx: 2.2.0 -*/
/*-----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* USER CODE BEGIN Options */

/* USER CODE END Options */

#define LWIP_STATS_DISPLAY            1
#define LWIP_MULTICAST_PING           1
#define LWIP_BROADCAST_PING           1

#define LWIP_ASSERT_CORE_LOCKED()
#define LWIP_NOASSERT

#define DNS_MAX_SERVERS               3
#define LWIP_NETIF_HOSTNAME           1
#define TCPIP_MBOX_SIZE               64
#define TCPIP_THREAD_STACKSIZE        4096
#define TCPIP_THREAD_PRIO             28

#define DEFAULT_RAW_RECVMBOX_SIZE     32
#define DEFAULT_UDP_RECVMBOX_SIZE     64
#define DEFAULT_TCP_RECVMBOX_SIZE     64
#define DEFAULT_ACCEPTMBOX_SIZE       32

#define SNTP_SERVER_DNS               1
#define LWIP_NETIF_LOOPBACK           1
#define LWIP_HAVE_LOOPIF              1
#define LWIP_LOOPBACK_MAX_PBUFS       0
#define LWIP_DHCP_DOES_ACD_CHECK      0

#define LWIP_ALTCP_TLS_MBEDTLS        1
#define LWIP_ALTCP                    1
#define LWIP_ALTCP_TLS                1
#define LWIP_CHKSUM_ALGORITHM         3
#define LWIP_TCPIP_CORE_LOCKING       1
#define LWIP_TCPIP_CORE_LOCKING_INPUT 1

#define PBUF_LINK_ENCAPSULATION_HLEN  388

#define CONFIG_MAC_RXQ_DEPTH          12
#define CONFIG_MAC_TXQ_DEPTH          32
#define IP_REASS_MAX_PBUFS            (2 * CONFIG_MAC_RXQ_DEPTH - 2)

#define MEMP_NUM_NETBUF               28
#define MEMP_NUM_ALTCP_PCB            2
#define MEMP_NUM_UDP_PCB              6
#define MEMP_NUM_TCP_PCB              6
#define MEMP_NUM_TCP_PCB_LISTEN       1
#define MEMP_NUM_NETCONN              (MEMP_NUM_TCP_PCB + MEMP_NUM_TCP_PCB_LISTEN)
#define MEMP_NUM_REASSDATA            LWIP_MIN((IP_REASS_MAX_PBUFS), 5)

#define MAC_TXQ_DEPTH                 CONFIG_MAC_TXQ_DEPTH
#define MAC_RXQ_DEPTH                 CONFIG_MAC_RXQ_DEPTH

#define TCP_MSS                       (1500 - 40)
#define TCP_WND                       ((2 * MAC_RXQ_DEPTH) * TCP_MSS)
#define TCP_SND_BUF                   (2 * MAC_TXQ_DEPTH * TCP_MSS)

#define TCP_QUEUE_OOSEQ               1
#define TCP_SND_QUEUELEN              ((2 * TCP_SND_BUF) / TCP_MSS)
#define MEMP_NUM_TCP_SEG              ((4 * TCP_SND_BUF) / TCP_MSS)
#define MEMP_NUM_PBUF                 (TCP_SND_BUF / TCP_MSS)
#define PBUF_POOL_SIZE                0
#define LWIP_WND_SCALE                1
#define TCP_RCV_SCALE                 2
#define TCP_SNDLOWAT                  LWIP_MIN(LWIP_MAX(((TCP_SND_BUF) / 4), (2 * TCP_MSS) + 1), (TCP_SND_BUF)-1)

#define MEM_MIN_TCP                   (2300 + MEMP_NUM_PBUF * (100 + PBUF_LINK_ENCAPSULATION_HLEN))
#define MEM_MIN                       MEM_MIN_TCP
#define MEM_ALIGNMENT                 4

#define MEMP_NUM_SYS_TIMEOUT          (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 8 + 3)

#ifdef LWIP_HEAP_SIZE
#define MEM_SIZE                      LWIP_HEAP_SIZE
#else
#if MEM_MIN > 8192
#define MEM_SIZE                      (MEM_MIN)
#else
#define MEM_SIZE                      8192
#endif /* MEM_SIZE */
#endif /* LWIP_HEAP_SIZE */

#define LWIP_IPV6                     1
#if (LWIP_IPV6 == 1)
#define LWIP_IPV6_DHCP6               0
#define LWIP_IPV6_MLD                 1
#define LWIP_IPV6_ND                  1
#define MEMP_NUM_MLD6_GROUP           ((1 + 1) + 4)
#define LWIP_IPV6_FORWARD             1
#define LWIP_IPV6_ROUTE_TABLE_SUPPORT 1

#ifndef IPV6_FRAG_COPYHEADER
#if defined(__x86_64__)
#define IPV6_FRAG_COPYHEADER          1
#else
#define IPV6_FRAG_COPYHEADER          0
#endif /* __x86_64__ */
#endif /* IPV6_FRAG_COPYHEADER */
#endif /* LWIP_IPV6 */

/**
  * Debug printing
  * By default enable debug printing for debug build, but set level to off
  * This allows user to change any desired debug level to on.
 */
/* #define LWIP_DEBUG */

#ifdef LWIP_DEBUG

#define MEMP_OVERFLOW_CHECK           (1)
#define MEMP_SANITY_CHECK             (1)

#define MEM_DEBUG                     LWIP_DBG_OFF
#define MEMP_DEBUG                    LWIP_DBG_OFF
#define PBUF_DEBUG                    LWIP_DBG_OFF
#define API_LIB_DEBUG                 LWIP_DBG_OFF
#define API_MSG_DEBUG                 LWIP_DBG_OFF
#define TCPIP_DEBUG                   LWIP_DBG_OFF
#define NETIF_DEBUG                   LWIP_DBG_OFF
#define SOCKETS_DEBUG                 LWIP_DBG_OFF
#define DEMO_DEBUG                    LWIP_DBG_OFF
#define IP_DEBUG                      LWIP_DBG_OFF
#define IP6_DEBUG                     LWIP_DBG_OFF
#define IP_REASS_DEBUG                LWIP_DBG_OFF
#define RAW_DEBUG                     LWIP_DBG_OFF
#define ICMP_DEBUG                    LWIP_DBG_OFF
#define UDP_DEBUG                     LWIP_DBG_OFF
#define TCP_DEBUG                     LWIP_DBG_OFF
#define TCP_INPUT_DEBUG               LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG              LWIP_DBG_OFF
#define TCP_RTO_DEBUG                 LWIP_DBG_OFF
#define TCP_CWND_DEBUG                LWIP_DBG_OFF
#define TCP_WND_DEBUG                 LWIP_DBG_OFF
#define TCP_FR_DEBUG                  LWIP_DBG_OFF
#define TCP_QLEN_DEBUG                LWIP_DBG_OFF
#define TCP_RST_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                     LWIP_DBG_OFF
#define ETHARP_DEBUG                  LWIP_DBG_OFF

#endif /* LWIP_DEBUG */

#define LWIP_RAW                      1
#define LWIP_MULTICAST_TX_OPTIONS     1

#define LWIP_ERRNO_STDINCLUDE         1
#define LWIP_SOCKET_SET_ERRNO         1

#define LWIP_DHCP                     1
#define LWIP_DNS                      1
#define LWIP_SO_RCVTIMEO              1
#define LWIP_SO_SNDTIMEO              1
#define SO_REUSE                      1
#define LWIP_TCP_KEEPALIVE            1

#ifdef CONFIG_LWIP_LP
#define TCP_TIMER_PRECISE_NEEDED      1
#define DHCP_TIMER_PRECISE_NEEDED     1
#define ARP_TIMER_PRECISE_NEEDED      1
#define IP4_FRAG_TIMER_PRECISE_NEEDED 1
#define DNS_TIMER_PRECISE_NEEDED      1

#define LWIP_IGMP                     0
#else
#define LWIP_IGMP                     1
#endif /* CONFIG_LWIP_LP */

#define LWIP_NETIF_STATUS_CALLBACK    1
#define LWIP_NETIF_API                1

#define ETHARP_SUPPORT_STATIC_ENTRIES 1

#define LWIP_SUPPORT_CUSTOM_PBUF      1
#define LWIP_NETIF_TX_SINGLE_PBUF     1
#define LWIP_RAND()                   ((u32_t)rand())

/* USER CODE BEGIN Options_end */

/* USER CODE END Options_end */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__LWIPOPTS_H */
