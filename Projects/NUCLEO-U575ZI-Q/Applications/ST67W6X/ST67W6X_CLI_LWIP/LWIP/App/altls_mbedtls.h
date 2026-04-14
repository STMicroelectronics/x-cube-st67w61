/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    altls_mbedtls.h
  * @author  ST67 Application Team
  * @brief   Application layered TCP/TLS connection API
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
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
#include "mbedtls/x509.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
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
  int32_t fd;                             /*!< Socket handle of the underlying TCP connection */
#if defined(MBEDTLS_CONFIG_FILE)
  mbedtls_ssl_context ssl;                /*!< SSL context */
#endif /* MBEDTLS_CONFIG_FILE */
} ssl_conn_t;

/**
  * @brief  Structure to hold SSL configuration parameters and contexts
 */
typedef struct
{
  int32_t ssl_inited;                     /*!< SSL initialized flag */
  int32_t authmode;                       /*!< Cached authmode (MBEDTLS_SSL_VERIFY_*) */
#if defined(MBEDTLS_CONFIG_FILE)
  mbedtls_x509_crt ca_cert;               /*!< CA certificate */
  mbedtls_x509_crt owncert;               /*!< Own certificate */
  mbedtls_ssl_config conf;                /*!< SSL configuration */
  mbedtls_pk_context pkey;                /*!< Private key context */
  mbedtls_entropy_context entropy;        /*!< Entropy context */
  mbedtls_ctr_drbg_context ctr_drbg;      /*!< CTR-DRBG context */
#endif /* MBEDTLS_CONFIG_FILE */
} ssl_config_ctx_t;

/**
  * @brief  Structure to hold SSL connection parameters
 */
typedef struct
{
  const char *ca_cert;        /*!< Pointer to CA certificate */
  int32_t ca_cert_len;        /*!< Length of CA certificate */
  const char *own_cert;       /*!< Pointer to own certificate */
  int32_t own_cert_len;       /*!< Length of own certificate */
  const char *private_cert;   /*!< Pointer to private key */
  int32_t private_cert_len;   /*!< Length of private key */
  const char **alpn;          /*!< Pointer to ALPN protocols */
  int32_t alpn_num;           /*!< Number of ALPN protocols */

  const char *psk;            /*!< Pointer to PSK */
  int32_t psk_len;            /*!< Length of PSK */
  const char *pskhint;        /*!< Pointer to PSK hint */
  int32_t pskhint_len;        /*!< Length of PSK hint */
} ssl_conn_param_t;
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported functions --------------------------------------------------------*/
#if defined(MBEDTLS_DEBUG_C)
void my_debug(void *ctx, int level,
              const char *file, int line,
              const char *str);
#endif /* MBEDTLS_DEBUG_C */

/**
  * @brief  Configure SSL context
  * @param  param: Descriptor containing all configuration params
  * @param  is_client: Integer to differentiate client and server connection
  * @return SSL configuration structure
  */
void *ssl_configure(ssl_conn_param_t *param, int32_t is_client);

/**
  * @brief  Establish an SSL/TLS connection to the specified socket
  * @param  sock: Socket file descriptor
  * @param  ssl_conf: SSL configuration structure
  * @param  is_client: Integer to differentiate client and server connection
  * @param  sni: Server Name Indication (SNI) string
  * @return SSL connection structure
  */
ssl_conn_t *ssl_secure_connection(int32_t sock, ssl_config_ctx_t *ssl_conf, int32_t is_client, char *sni);

/**
  * @brief  Destroy SSL context and free resources
  * @param  tls_config: TLS context structure
  * @param  tls_conn: TLS connection structure
  */
void ssl_destroy(ssl_config_ctx_t *tls_config, ssl_conn_t *tls_conn);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ALTCP_TLS_MBEDTLS_H */
