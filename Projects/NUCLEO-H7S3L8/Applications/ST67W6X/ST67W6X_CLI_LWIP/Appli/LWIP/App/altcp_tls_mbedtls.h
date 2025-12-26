/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    altcp_tls_mbedtls.h
  * @author  GPM Application Team
  * @brief   Application layered TCP/TLS connection API
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef ALTCP_TLS_MBEDTLS_H
#define ALTCP_TLS_MBEDTLS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "lwip.h"

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#endif /* MBEDTLS_CONFIG_FILE */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/** Failed to open a socket. */
#define MBEDTLS_ERR_NET_SOCKET_FAILED                     -0x0042
/** The connection to the given server / port failed. */
#define MBEDTLS_ERR_NET_CONNECT_FAILED                    -0x0044
/** Binding of the socket failed. */
#define MBEDTLS_ERR_NET_BIND_FAILED                       -0x0046
/** Could not listen on the socket. */
#define MBEDTLS_ERR_NET_LISTEN_FAILED                     -0x0048
/** Could not accept the incoming connection. */
#define MBEDTLS_ERR_NET_ACCEPT_FAILED                     -0x004A
/** Reading information from the socket failed. */
#define MBEDTLS_ERR_NET_RECV_FAILED                       -0x004C
/** Sending information through the socket failed. */
#define MBEDTLS_ERR_NET_SEND_FAILED                       -0x004E
/** Connection was reset by peer. */
#define MBEDTLS_ERR_NET_CONN_RESET                        -0x0050
/** Failed to get an IP address for the given hostname. */
#define MBEDTLS_ERR_NET_UNKNOWN_HOST                      -0x0052
/** Buffer is too small to hold the data. */
#define MBEDTLS_ERR_NET_BUFFER_TOO_SMALL                  -0x0043
/** The context is invalid, eg because it was free()ed. */
#define MBEDTLS_ERR_NET_INVALID_CONTEXT                   -0x0045
/** Polling the net context failed. */
#define MBEDTLS_ERR_NET_POLL_FAILED                       -0x0047
/** Input invalid. */
#define MBEDTLS_ERR_NET_BAD_INPUT_DATA                    -0x0049

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  Structure of SSL connection and mbedTLS parameters
  */
typedef struct
{
  int32_t ssl_inited;                     /*!< SSL initialized flag */
  int32_t fd;                             /*!< Socket handle of the underlying TCP connection */
#if defined(MBEDTLS_CONFIG_FILE)
  mbedtls_x509_crt ca_cert;               /*!< CA certificate */
  mbedtls_x509_crt owncert;               /*!< Own certificate */
  mbedtls_ssl_config conf;                /*!< SSL configuration */
  mbedtls_ssl_context ssl;                /*!< SSL context */
  mbedtls_pk_context pkey;                /*!< Private key context */
  mbedtls_entropy_context entropy;        /*!< Entropy context */
  mbedtls_ctr_drbg_context ctr_drbg;      /*!< CTR-DRBG context */
#endif /* MBEDTLS_CONFIG_FILE */
} ssl_param_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  Establish a TCP connection to the specified IP address and port
  * @param  ip: Pointer to the IP address structure
  * @param  port: Port number
  * @return Socket file descriptor or negative value on error
  */
int32_t tcp_client_connect(ip4_addr_t *ip, int32_t port);

/**
  * @brief  Establish an SSL/TLS connection to the specified IP address and port
  * @param  ip: Pointer to the IP address structure
  * @param  port: Port number
  * @param  sni: Server Name Indication (SNI) string
  * @param  cert: Pointer to own certificate
  * @param  cert_len: Length of own certificate
  * @param  priv_key: Pointer to private key
  * @param  priv_key_len: Length of private key
  * @param  ca_cert: Pointer to CA certificate
  * @param  ca_cert_len: Length of CA certificate
  * @param  alpn: Array of ALPN protocol strings
  * @param  alpn_num: Number of ALPN protocols
  * @param  ssl_param: Pointer to store the SSL connection handle
  * @return Socket file descriptor or negative value on error
  */
int32_t ssl_client_connect(ip4_addr_t *ip, int32_t port, char *sni,
                           char *cert, uint32_t cert_len, char *priv_key, uint32_t priv_key_len,
                           char *ca_cert, uint32_t ca_cert_len,
                           char *alpn[6], int32_t alpn_num,
                           ssl_param_t **ssl_param);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ALTCP_TLS_MBEDTLS_H */
