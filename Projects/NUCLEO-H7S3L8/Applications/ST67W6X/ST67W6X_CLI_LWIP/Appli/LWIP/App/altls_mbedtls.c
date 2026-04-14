/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    altls_mbedtls.c
  * @author  ST67 Application Team
  * @brief   Application layered TLS connection API
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

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "main_app.h"
#include "altls_mbedtls.h"
#include "app_config.h"
#include <lwip/errno.h>
#include <netdb.h>

#include "w6x_api.h"
#include "common_parser.h" /* Common Parser functions */
#include "spi_iface.h" /* SPI falling/rising_callback */
#include "logging.h"
#include "shell.h"
#include "logshell_ctrl.h"

#include "FreeRTOS.h"

#ifndef ALTLS_MBEDTLS_VERIFY_INFO_BUF_SIZE
/** Application-chosen buffer size for mbedtls_x509_crt_verify_info().
  * mbedTLS does not expose a public "max" length for this formatted string,
  * because it depends on which flag bits are set and on the prefix.
  */
#define ALTLS_MBEDTLS_VERIFY_INFO_BUF_SIZE 512U
#endif /* ALTLS_MBEDTLS_VERIFY_INFO_BUF_SIZE */

#ifndef ALTLS_MBEDTLS_DEBUG_LEVEL
/** MbedTLS debug trace level */
#define ALTLS_MBEDTLS_DEBUG_LEVEL 2
#endif /* ALTLS_MBEDTLS_DEBUG_LEVEL */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private function prototypes -----------------------------------------------*/
#if defined(MBEDTLS_CONFIG_FILE)
/**
  * @brief  mbedTLS receive data callback function
  * @param  ctx: The SSL connection context
  * @param  buf: Pointer to the data buffer to store received data
  * @param  len: Length of the data to receive
  * @return Number of bytes received or negative value on error
  */
static int ssl_recv(void *ctx, unsigned char *buf, size_t len);

/**
  * @brief  mbedTLS send data callback function
  * @param  ctx: The SSL connection context
  * @param  buf: Pointer to the data buffer containing data to send
  * @param  len: Length of the data to send
  * @return Number of bytes sent or negative value on error
  */
static int ssl_send(void *ctx, const unsigned char *buf, size_t len);
#endif /* MBEDTLS_CONFIG_FILE */

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
#if defined(MBEDTLS_DEBUG_C)
void my_debug(void *ctx, int level,
              const char *file, int line,
              const char *str)
{
  (void)level;
  (void)ctx;
  LogDebug("[MBEDTLS] %s:%04d: (lvl %d) %s", file, line, level, str);
}
#endif /* MBEDTLS_DEBUG_C */

void *ssl_configure(ssl_conn_param_t *param, int32_t is_client)
{
#if defined(MBEDTLS_CONFIG_FILE)
  int32_t ret;
  const char *pers = (is_client != 0) ? "ssl_client" : "ssl_server";

  /* Create the ssl param instance */
  ssl_config_ctx_t *ssl_conf = pvPortMalloc(sizeof(ssl_config_ctx_t));
  if (NULL == ssl_conf)
  {
    LogError("[MBEDTLS] ssl configure: malloc(%d) fail\r\n", sizeof(ssl_config_ctx_t));
    return NULL;
  }

  (void)memset(ssl_conf, 0, sizeof(ssl_config_ctx_t));
  ssl_conf->ssl_inited = 1;

  /* Init TLS context for a given session. */
  mbedtls_ssl_config_init(&ssl_conf->conf);

#if defined(MBEDTLS_DEBUG_C)
  mbedtls_ssl_conf_dbg(&ssl_conf->conf, my_debug, NULL);
  mbedtls_debug_set_threshold(ALTLS_MBEDTLS_DEBUG_LEVEL);
#endif /* MBEDTLS_DEBUG_C */

  /* Initialize certificates and keys */
  if ((param->ca_cert != NULL) && (param->ca_cert_len > 0))
  {
    mbedtls_x509_crt_init(&ssl_conf->ca_cert);
  }
  if ((param->own_cert != NULL) && (param->own_cert_len > 0))
  {
    mbedtls_x509_crt_init(&ssl_conf->owncert);
  }
  if ((param->private_cert != NULL) && (param->private_cert_len > 0))
  {
    mbedtls_pk_init(&ssl_conf->pkey);
  }

  /*Init entropy (randomness source) and random number generation context.*/
  mbedtls_entropy_init(&ssl_conf->entropy);
  mbedtls_ctr_drbg_init(&ssl_conf->ctr_drbg);

  /* Seed the random number generator */
  ret = mbedtls_ctr_drbg_seed(&ssl_conf->ctr_drbg, mbedtls_entropy_func,
                              &ssl_conf->entropy, (const unsigned char *)pers, strlen(pers));
  if (ret != 0)
  {
    LogError("[MBEDTLS] ssl configure: ctr_drbg_seed failed: 0x%x\r\n", -ret);
    goto ERROR;
  }

  /* Set the RNG (Random Number Generator) callback function */
  mbedtls_ssl_conf_rng(&ssl_conf->conf, mbedtls_ctr_drbg_random, &ssl_conf->ctr_drbg);

  /* Parse CA (Certificate Authority) certificate */
  if ((param->ca_cert != NULL) && (param->ca_cert_len > 0))
  {
    LogInfo("[MBEDTLS] Loading the CA root certificate ... \r\n");
    ret = mbedtls_x509_crt_parse(&ssl_conf->ca_cert, (const unsigned char *)param->ca_cert,
                                 (size_t)param->ca_cert_len);
    if (ret < 0)
    {
      LogError("[MBEDTLS] ssl configure: root parse failed: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }

  /* Parse own certificate */
  if ((param->own_cert != NULL) && (param->own_cert_len > 0))
  {
    LogInfo("[MBEDTLS] Loading the server certificate ... \r\n");
    ret = mbedtls_x509_crt_parse(&ssl_conf->owncert, (const unsigned char *)param->own_cert,
                                 (size_t)param->own_cert_len);
    if (ret < 0)
    {
      LogError("[MBEDTLS] ssl configure: x509 parse failed: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }

  /* Parse private key */
  if ((param->private_cert != NULL) && (param->private_cert_len > 0))
  {
    LogInfo("[MBEDTLS] Loading the server private key ... \r\n");
    ret = mbedtls_pk_parse_key(&ssl_conf->pkey,
                               (const unsigned char *)param->private_cert,
                               param->private_cert_len,
                               NULL, 0,
                               mbedtls_ctr_drbg_random, &ssl_conf->ctr_drbg);
    if (ret != 0)
    {
      LogError("[MBEDTLS] ssl configure: x509 parse failed: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }

  /* Setup stuff */
  LogInfo("[MBEDTLS] Setting up the SSL/TLS structure ... \r\n");
  if (is_client == 1)
  {
    ret = mbedtls_ssl_config_defaults(&ssl_conf->conf, MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    /* Set maximum fragment length to 4096 bytes */
    (void)mbedtls_ssl_conf_max_frag_len(&ssl_conf->conf, MBEDTLS_SSL_MAX_FRAG_LEN_4096);
#endif /* MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

    /* Configure ALPN protocols */
    if (param->alpn_num && param->alpn)
    {
      (void)mbedtls_ssl_conf_alpn_protocols(&ssl_conf->conf, (const char **)param->alpn);
    }

    /* Configure PSK */
    if ((param->psk_len > 0) && (param->psk != NULL) && (param->pskhint_len > 0) && (param->pskhint != NULL))
    {
      (void)mbedtls_ssl_conf_psk(&ssl_conf->conf, (const unsigned char *)param->psk, param->psk_len,
                                 (const unsigned char *)param->pskhint, param->pskhint_len);
    }
  }
  else
  {
    ret = mbedtls_ssl_config_defaults(&ssl_conf->conf, MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
  }
  if (ret != 0)
  {
    LogError("[MBEDTLS] ssl configure: set ssl config failed: 0x%x\r\n", -ret);
    goto ERROR;
  }

  /* Configure own certificate and private key */
  if ((param->own_cert != NULL) && (param->own_cert_len > 0) &&
      (param->private_cert != NULL) && (param->private_cert_len > 0))
  {
    ret = mbedtls_ssl_conf_own_cert(&ssl_conf->conf, &ssl_conf->owncert, &ssl_conf->pkey);
    if (ret != 0)
    {
      LogError("[MBEDTLS] ssl configure: mbedtls_ssl_conf_own_cert failed: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }
  if (is_client == 1)
  {
    /* In client mode, set the verification mode to OPTIONAL.
     * It is preferred to REQUIRE the verification using a CA to avoid impersonation of the server/peer.
     * This is for demonstration purposes. */
    mbedtls_ssl_conf_authmode(&ssl_conf->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    ssl_conf->authmode = MBEDTLS_SSL_VERIFY_OPTIONAL;
    if ((param->ca_cert != NULL) && (param->ca_cert_len > 0))
    {
      mbedtls_ssl_conf_ca_chain(&ssl_conf->conf, &ssl_conf->ca_cert, NULL);
    }
  }
  else
  {
    /* In server mode, set the list of trusted CAs for client certificate verification */
    if ((param->ca_cert != NULL) && (param->ca_cert_len > 0))
    {
      /* Configure SSL to require verification of the peer certificate */
      mbedtls_ssl_conf_authmode(&ssl_conf->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
      ssl_conf->authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
      mbedtls_ssl_conf_ca_chain(&ssl_conf->conf, &ssl_conf->ca_cert, NULL);
    }
    else
    {
      mbedtls_ssl_conf_authmode(&ssl_conf->conf, MBEDTLS_SSL_VERIFY_NONE);
      ssl_conf->authmode = MBEDTLS_SSL_VERIFY_NONE;
    }
  }

  LogInfo("[MBEDTLS] ssl configure ok\r\n");
  return (void *)ssl_conf;

ERROR:
  if ((param->ca_cert != NULL) && (param->ca_cert_len > 0))
  {
    mbedtls_x509_crt_free(&ssl_conf->ca_cert);
  }
  if ((param->own_cert != NULL) && (param->own_cert_len > 0))
  {
    mbedtls_x509_crt_free(&ssl_conf->owncert);
  }
  if ((param->private_cert != NULL) && (param->private_cert_len > 0))
  {
    mbedtls_pk_free(&ssl_conf->pkey);
  }

  mbedtls_ctr_drbg_free(&ssl_conf->ctr_drbg);
  mbedtls_entropy_free(&ssl_conf->entropy);
  mbedtls_ssl_config_free(&ssl_conf->conf);
  vPortFree(ssl_conf);
#endif /* MBEDTLS_CONFIG_FILE */
  return NULL;
}

ssl_conn_t *ssl_secure_connection(int32_t sock, ssl_config_ctx_t *ssl_conf, int32_t is_client, char *sni)
{
#if defined(MBEDTLS_CONFIG_FILE)
  int32_t ret;

  ssl_conn_t *ssl_conn = pvPortMalloc(sizeof(ssl_conn_t));
  if (ssl_conn == NULL)
  {
    LogError("[MBEDTLS] ssl malloc failed: altcp_ssl_accept ");
    return NULL;
  }

  (void)memset(ssl_conn, 0, sizeof(ssl_conn_t));
  ssl_conn->fd = sock;
  mbedtls_ssl_init(&ssl_conn->ssl);
  ret = mbedtls_ssl_setup(&ssl_conn->ssl, &ssl_conf->conf);
  if (ret != 0)
  {
    LogError("[MBEDTLS] ssl accept: mbedtls_ssl_setup returned: 0x%x\r\n", -ret);
    goto _err;
  }

  if (is_client == 1)
  {
    /* Set hostname for the SNI (Server Name Indication) TLS extension.
     * The SNI is set if present and is not a literal IPv4 or IPv6 address. (see RFC 6066) */
    if (sni != NULL)
    {
      ip_addr_t tmp_ip;
      if (ipaddr_aton(sni, &tmp_ip) == 0)
      {
        (void)mbedtls_ssl_set_hostname(&ssl_conn->ssl, sni);
      }
      else
      {
        LogWarn("[MBEDTLS] IP Address cannot be used to configure SNI\r\n");
      }
    }
  }
  else
  {
    UNUSED(sni);
  }
  /* Set the underlying BIO callbacks */
  mbedtls_ssl_set_bio(&ssl_conn->ssl, (void *)&ssl_conn->fd, ssl_send, ssl_recv, NULL);

  /* Perform the SSL/TLS handshake */
  LogInfo("[MBEDTLS] Performing the SSL/TLS handshake ... \r\n");
  while (true)
  {
    ret = mbedtls_ssl_handshake(&ssl_conn->ssl);
    if (ret == 0)
    {
      break;
    }
    if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
    {
      LogError("[MBEDTLS] ssl accept: mbedtls_ssl_handshake returned: 0x%x\r\n", -ret);
      /* If the error is related to the certificate verification and that in client mode,
       * break in order to verify the result before going into error. */
      if ((is_client == 1) && ((ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) ||
                               (ret == MBEDTLS_ERR_SSL_BAD_CERTIFICATE)))
      {
        break;
      }
      else
      {
        goto _err;
      }
    }
  }

  /* When in client mode, the CA certificate configured will be used to verify the certificates given by the server,
   * if requested by the client */
  if (is_client == 1)
  {
    /* Verify the server certificate */
    LogInfo("[MBEDTLS] ...... Verifying peer X.509 certificate ... \r\n");
    uint32_t verify_flags = mbedtls_ssl_get_verify_result(&ssl_conn->ssl);
    if (verify_flags != 0U)
    {
      char verify_info[ALTLS_MBEDTLS_VERIFY_INFO_BUF_SIZE];
      (void)memset(verify_info, 0, sizeof(verify_info));
      int32_t verify_info_len = (int32_t)mbedtls_x509_crt_verify_info(verify_info, sizeof(verify_info),
                                                                      "  ! ", verify_flags);
      if ((verify_info_len < 0) || (verify_info_len >= (int32_t)sizeof(verify_info)))
      {
        LogWarn("[MBEDTLS] verify reason(s) truncated, consider increasing "
                "ALTLS_MBEDTLS_VERIFY_INFO_BUF_SIZE (need ~%" PRIi32 ")\r\n",
                verify_info_len);
      }

      if (ssl_conf->authmode == MBEDTLS_SSL_VERIFY_REQUIRED)
      {
        LogError("[MBEDTLS] ssl connect: peer certificate verification failed (flags=0x%08" PRIx32 ")\r\n",
                 verify_flags);
        LogError("[MBEDTLS] verify reason(s):\r\n%s\r\n", verify_info);
        goto _err;
      }

      /* When OPTIONAL mode is configured, only display a warning (NONE verification will not set any flag). */
      LogWarn("[MBEDTLS] ssl connect: peer certificate verification failed (flags=0x%08" PRIx32
              ") - continuing (verification is set to OPTIONAL)\r\n",
              verify_flags);
      LogWarn("[MBEDTLS] verify reason(s):\r\n%s\r\n", verify_info);
    }
  }

  return ssl_conn;

_err:
  (void)lwip_close(sock);
  mbedtls_ssl_free(&ssl_conn->ssl);
  vPortFree(ssl_conn);
#endif /* MBEDTLS_CONFIG_FILE */
  return NULL;
}

void ssl_destroy(ssl_config_ctx_t *tls_config, ssl_conn_t *tls_conn)
{
#if defined(MBEDTLS_CONFIG_FILE)

  /* Free SSL connection  resources */
  if (tls_conn != NULL)
  {
    mbedtls_ssl_free(&tls_conn->ssl);
    vPortFree(tls_conn);
  }
  /* Free SSL configuration resources */
  if (tls_config != NULL)
  {
    mbedtls_ssl_config_free(&tls_config->conf);
    mbedtls_x509_crt_free(&tls_config->ca_cert);
    mbedtls_x509_crt_free(&tls_config->owncert);
    mbedtls_pk_free(&tls_config->pkey);
    mbedtls_ctr_drbg_free(&tls_config->ctr_drbg);
    mbedtls_entropy_free(&tls_config->entropy);
    vPortFree(tls_config);
  }
#endif /* MBEDTLS_CONFIG_FILE */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
#if defined(MBEDTLS_CONFIG_FILE)
static int32_t net_would_block(void *ctx)
{
  /* USER CODE BEGIN net_would_block_1 */

  /* USER CODE END net_would_block_1 */
  int32_t ret = 0;
  int32_t err = errno;
  int32_t fd = *(int32_t *)ctx;

  /* Never return 'WOULD BLOCK' on a non-blocking socket */
  if ((fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK)
  {
    errno = err;
    return ret;
  }

  switch (err)
  {
#if defined EAGAIN
    case EAGAIN:
      ret = 1;
      break;
#endif /* EAGAIN */
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
      ret = 1;
      break;
#endif /* EWOULDBLOCK */
    default:
      /* Undefined case */
      break;
  }
  /* USER CODE BEGIN net_would_block_2 */

  /* USER CODE END net_would_block_2 */
  return ret;

  /* USER CODE BEGIN net_would_block_End */

  /* USER CODE END net_would_block_End */
}

static int ssl_recv(void *ctx, unsigned char *buf, size_t len)
{
  /* USER CODE BEGIN ssl_recv_1 */

  /* USER CODE END ssl_recv_1 */
  int32_t ret;
  int32_t fd = *(int32_t *)ctx;

  if (fd < 0)
  {
    LogError("[MBEDTLS] invalid socket fd\r\n");
    return (MBEDTLS_ERR_NET_INVALID_CONTEXT);
  }

  ret = (int32_t)read(fd, buf, len);
  if (ret < 0)
  {
    if (errno == EPIPE || errno == ECONNRESET)
    {
      LogError("[MBEDTLS] net reset - errno: %d\r\n", errno);
      return (MBEDTLS_ERR_NET_CONN_RESET);
    }

    if (errno == EINTR)
    {
      return (MBEDTLS_ERR_SSL_WANT_READ);
    }

    if (net_would_block(ctx) != 0)
    {
      return (MBEDTLS_ERR_SSL_WANT_READ);
    }
    LogError("[MBEDTLS] ssl recv failed - errno: %d\r\n", errno);
    return (MBEDTLS_ERR_NET_RECV_FAILED);
  }

  /* USER CODE BEGIN ssl_recv_2 */

  /* USER CODE END ssl_recv_2 */
  return ret;

  /* USER CODE BEGIN ssl_recv_End */

  /* USER CODE END ssl_recv_End */
}

static int ssl_send(void *ctx, const unsigned char *buf, size_t len)
{
  /* USER CODE BEGIN ssl_send_1 */

  /* USER CODE END ssl_send_1 */
  int32_t ret;
  int32_t fd = *(int32_t *)ctx;

  if (fd < 0)
  {
    return (MBEDTLS_ERR_NET_INVALID_CONTEXT);
  }

  ret = (int32_t)send(fd, buf, len, 0);
  if (ret < 0)
  {
    if ((errno == EPIPE) || (errno == ECONNRESET))
    {
      LogError("[MBEDTLS] net reset - errno: %d\r\n", errno);
      return MBEDTLS_ERR_NET_CONN_RESET;
    }

    if (errno == EINTR)
    {
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    if (net_would_block(ctx) != 0)
    {
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    LogError("[MBEDTLS] ssl send failed - errno: %d\r\n", errno);
    return (MBEDTLS_ERR_NET_SEND_FAILED);
  }
  /* USER CODE BEGIN ssl_send_2 */

  /* USER CODE END ssl_send_2 */
  return ret;

  /* USER CODE BEGIN ssl_send_End */

  /* USER CODE END ssl_send_End */
}
#endif /* MBEDTLS_CONFIG_FILE */
