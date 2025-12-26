/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    altcp_tls_mbedtls.c
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

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "main_app.h"
#include "altcp_tls_mbedtls.h"
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

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  Structure to hold SSL connection parameters
 */
typedef struct
{
  char *ca_cert;              /*!< Pointer to CA certificate */
  int32_t ca_cert_len;        /*!< Length of CA certificate */
  char *own_cert;             /*!< Pointer to own certificate */
  int32_t own_cert_len;       /*!< Length of own certificate */
  char *private_cert;         /*!< Pointer to private key */
  int32_t private_cert_len;   /*!< Length of private key */
  char **alpn;                /*!< Pointer to ALPN protocols */
  int32_t alpn_num;           /*!< Number of ALPN protocols */

  char *psk;                  /*!< Pointer to PSK */
  int32_t psk_len;            /*!< Length of PSK */
  char *pskhint;              /*!< Pointer to PSK hint */
  int32_t pskhint_len;        /*!< Length of PSK hint */

  char *sni;                  /*!< Pointer to Server Name Indication */
} ssl_conn_param_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private function prototypes -----------------------------------------------*/
#if defined(MBEDTLS_CONFIG_FILE)
/**
  * @brief  Establish an SSL/TLS connection using mbedTLS
  * @param  fd: The file descriptor of the underlying TCP connection
  * @param  param: Pointer to SSL connection parameters
  * @return Pointer to SSL connection handle or NULL if connection fails
  */
static void *mbedtls_ssl_connect(int32_t fd, const ssl_conn_param_t *param);

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
int32_t tcp_client_connect(ip4_addr_t *ip, int32_t port)
{
  /* USER CODE BEGIN tcp_client_connect_1 */

  /* USER CODE END tcp_client_connect_1 */
  int32_t fd;
  int32_t res;
  struct sockaddr_in addr;

  LogInfo("tcp client connect %s:%d\r\n", ip4addr_ntoa(ip), port);

  if ((fd = lwip_socket(AF_INET, SOCK_STREAM, 0))  < 0)
  {
    LogError("socket create failed\r\n");
    return -2;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_len = sizeof(addr);
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip4_addr_get_u32(ip);

  LogInfo("tcp_client_connect fd:%d\r\n", fd);

  int32_t on = 1;
  res = lwip_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (res != 0)
  {
    LogError("setsockopt failed, res:%d\r\n", res);
  }

  res = lwip_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (res < 0)
  {
    LogError("connect failed, res:%d\r\n", res);
    close(fd);
  }

  /* make non-blocking */
  if (fd != -1)
  {
    int32_t iMode = 1;
    ioctlsocket(fd, FIONBIO, &iMode);
  }
  /* USER CODE BEGIN tcp_client_connect_2 */

  /* USER CODE END tcp_client_connect_2 */

  return fd;
  /* USER CODE BEGIN tcp_client_connect_End */

  /* USER CODE END tcp_client_connect_End */
}

int32_t ssl_client_connect(ip4_addr_t *ip, int32_t port, char *sni,
                           char *cert, uint32_t cert_len, char *priv_key, uint32_t priv_key_len,
                           char *ca_cert, uint32_t ca_cert_len,
                           char *alpn[6], int32_t alpn_num,
                           ssl_param_t **ssl_param)
{
  /* USER CODE BEGIN ssl_client_connect_1 */

  /* USER CODE END ssl_client_connect_1 */
  int32_t fd;
#if defined(MBEDTLS_CONFIG_FILE)
  ssl_conn_param_t param = {0};
#endif /* MBEDTLS_CONFIG_FILE */

  fd = tcp_client_connect(ip, port);
  if (fd < 0)
  {
    LogError("tcp_client_connect fd:%d\r\n", fd);
    return -1;
  }

#if defined(MBEDTLS_CONFIG_FILE)
  /* make blocking */
  int32_t iMode = 0;
  ioctlsocket(fd, FIONBIO, &iMode);

  if (ca_cert)
  {
    param.ca_cert = ca_cert;
    param.ca_cert_len = ca_cert_len;
  }

  if (cert)
  {
    param.own_cert = cert;
    param.own_cert_len = cert_len;
  }

  if (priv_key)
  {
    param.private_cert = priv_key;
    param.private_cert_len = priv_key_len;
  }

  if (alpn)
  {
    param.alpn = (char **)alpn;
    param.alpn_num = alpn_num;
  }

  param.sni = sni;

  *ssl_param = mbedtls_ssl_connect(fd, &param);

  if (*ssl_param == NULL)
  {
    LogError("mbedtls_ssl_connect handle NULL, fd:%d\r\n", fd);
    close(fd);
    fd = -1;
  }

  /* make non-blocking */
  if (fd != -1)
  {
    int32_t iMode = 1;
    ioctlsocket(fd, FIONBIO, &iMode);
  }
#else
  LogWarn("mbedtls not defined. SSL connection is not possible\n");
#endif /* MBEDTLS_CONFIG_FILE */

  /* USER CODE BEGIN ssl_client_connect_2 */

  /* USER CODE END ssl_client_connect_2 */

  return fd;
  /* USER CODE BEGIN ssl_client_connect_End */

  /* USER CODE END ssl_client_connect_End */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
#if defined(MBEDTLS_CONFIG_FILE)
static void *mbedtls_ssl_connect(int32_t fd, const ssl_conn_param_t *param)
{
  /* USER CODE BEGIN mbedtls_ssl_connect_1 */

  /* USER CODE END mbedtls_ssl_connect_1 */
  int32_t ret;
  uint32_t result;
  ssl_param_t *ssl_param = NULL;
  const char *pers = "ssl_client";

#if defined(MBEDTLS_DEBUG_C)
  mbedtls_debug_set_threshold(MBEDTLS_DEBUG_LEVEL);
#endif /* MBEDTLS_DEBUG_C */

  ssl_param = (ssl_param_t *)pvPortMalloc(sizeof(ssl_param_t));
  if (NULL == ssl_param)
  {
    LogError("[MBEDTLS] ssl connect: malloc(%d) fail\r\n", sizeof(ssl_param_t));
    return NULL;
  }

  memset(ssl_param, 0, sizeof(ssl_param_t));
  ssl_param->ssl_inited = 1;
  /* Initialize the connection */
  ssl_param->fd = fd;

  /* Initialize SSL configuration and context */
  mbedtls_ssl_config_init(&ssl_param->conf);
  mbedtls_ssl_init(&ssl_param->ssl);

  /* Initialize certificates and keys */
  if (param->ca_cert && (param->ca_cert_len > 0))
  {
    mbedtls_x509_crt_init(&ssl_param->ca_cert);
  }
  if (param->own_cert && (param->own_cert_len > 0))
  {
    mbedtls_x509_crt_init(&ssl_param->owncert);
  }
  if (param->private_cert && (param->private_cert_len > 0))
  {
    mbedtls_pk_init(&ssl_param->pkey);
  }

  /* Seed the random number generator */
  mbedtls_entropy_init(&ssl_param->entropy);
  mbedtls_ctr_drbg_init(&ssl_param->ctr_drbg);
  ret = mbedtls_ctr_drbg_seed(&ssl_param->ctr_drbg, mbedtls_entropy_func,
                              &ssl_param->entropy, (const unsigned char *)pers, strlen(pers));
  if (ret != 0)
  {
    LogError("[MBEDTLS] ssl connect: ctr_drbg_seed failed: 0x%x\r\n", -ret);
    goto ERROR;
  }

  /* Set the RNG callback function */
  mbedtls_ssl_conf_rng(&ssl_param->conf, mbedtls_ctr_drbg_random, &ssl_param->ctr_drbg);

  /* Parse CA certificate */
  LogInfo("[MBEDTLS] Loading the CA root certificate ... \r\n");
  if (param->ca_cert && (param->ca_cert_len > 0))
  {
    LogInfo("[MBEDTLS] Loading the rsa\r\n");
    ret = mbedtls_x509_crt_parse(&ssl_param->ca_cert, (unsigned char *)param->ca_cert, (size_t)param->ca_cert_len);
    if (ret < 0)
    {
      LogError("[MBEDTLS] ssl connect: root parse failed: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }

  /* Parse own certificate */
  LogInfo("[MBEDTLS] Loading the client certificate ... \r\n");
  if (param->own_cert && (param->own_cert_len > 0))
  {
    ret = mbedtls_x509_crt_parse(&ssl_param->owncert, (unsigned char *)param->own_cert, (size_t)param->own_cert_len);
    if (ret < 0)
    {
      LogError("[MBEDTLS] ssl connect: x509 parse failed: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }

  /* Parse private key */
  LogInfo("[MBEDTLS] Loading the client private key ... \r\n");
  if (param->private_cert && (param->private_cert_len > 0))
  {
    ret = mbedtls_pk_parse_key(&ssl_param->pkey,
                               (unsigned char *)param->private_cert,
                               param->private_cert_len,
                               NULL, 0,
                               mbedtls_ctr_drbg_random, &ssl_param->ctr_drbg);
    if (ret != 0)
    {
      LogError("[MBEDTLS] ssl connect: x509 parse failed: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }

  /* Setup stuff */
  LogInfo("[MBEDTLS] Setting up the SSL/TLS structure ... \r\n");
  ret = mbedtls_ssl_config_defaults(&ssl_param->conf, MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0)
  {
    LogError("[MBEDTLS] ssl connect: set ssl config failed: 0x%x\r\n", -ret);
    goto ERROR;
  }

  if (param->ca_cert && (param->ca_cert_len > 0))
  {
    /* Configure SSL to require verification of the peer certificate */
    mbedtls_ssl_conf_authmode(&ssl_param->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&ssl_param->conf, &ssl_param->ca_cert, NULL);
  }
  else
  {
    mbedtls_ssl_conf_authmode(&ssl_param->conf, MBEDTLS_SSL_VERIFY_NONE);
  }

  /* Configure own certificate and private key */
  if (param->own_cert && (param->own_cert_len > 0) && param->private_cert && (param->private_cert_len > 0))
  {
    mbedtls_ssl_conf_own_cert(&ssl_param->conf, &ssl_param->owncert, &ssl_param->pkey);
  }

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
  /* Set maximum fragment length to 4096 bytes */
  mbedtls_ssl_conf_max_frag_len(&ssl_param->conf, MBEDTLS_SSL_MAX_FRAG_LEN_4096);
#endif /* MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

  /* Configure ALPN protocols */
  if (param->alpn_num && param->alpn)
  {
    mbedtls_ssl_conf_alpn_protocols(&ssl_param->conf, (const char **)param->alpn);
  }

  /* Configure PSK */
  if (param->psk_len && param->psk && param->pskhint_len && param->pskhint)
  {
    mbedtls_ssl_conf_psk(&ssl_param->conf, (const unsigned char *)param->psk, param->psk_len,
                         (const unsigned char *)param->pskhint, param->pskhint_len);
  }

  /* Setup SSL context */
  ret = mbedtls_ssl_setup(&ssl_param->ssl, &ssl_param->conf);
  if (ret != 0)
  {
    LogError("[MBEDTLS] ssl connect: mbedtls_ssl_setup returned: 0x%x\r\n", -ret);
    goto ERROR;
  }

  /* Set hostname for SNI extension */
  if (param->sni)
  {
    mbedtls_ssl_set_hostname(&ssl_param->ssl, param->sni);
  }

  /* Set the underlying BIO callbacks */
  mbedtls_ssl_set_bio(&ssl_param->ssl, (void *)&ssl_param->fd, ssl_send, ssl_recv, NULL);

  /* Perform the SSL/TLS handshake */
  LogInfo("[MBEDTLS] Performing the SSL/TLS handshake ... \r\n");
  while ((ret = mbedtls_ssl_handshake(&ssl_param->ssl)) != 0)
  {
    if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
    {
      LogError("[MBEDTLS] ssl connect: mbedtls_ssl_handshake returned: 0x%x\r\n", -ret);
      goto ERROR;
    }
  }

  /* Verify the server certificate */
  LogInfo("[MBEDTLS] ...... Verifying peer X.509 certificate ... \r\n");
  result = mbedtls_ssl_get_verify_result(&ssl_param->ssl);
  if (result != 0)
  {
    LogError("[MBEDTLS] ssl connect: verify result not confirmed: 0x%x\r\n", -ret);
    goto ERROR;
  }

  /* USER CODE BEGIN mbedtls_ssl_connect_2 */

  /* USER CODE END mbedtls_ssl_connect_2 */
  LogInfo("[MBEDTLS] ssl connect ok\r\n");
  return (void *)ssl_param;

ERROR:
  if (ssl_param != NULL)
  {
    if (param->ca_cert && (param->ca_cert_len > 0))
    {
      mbedtls_x509_crt_free(&ssl_param->ca_cert);
    }
    if (param->own_cert && (param->own_cert_len > 0))
    {
      mbedtls_x509_crt_free(&ssl_param->owncert);
    }
    if (param->private_cert && (param->private_cert_len > 0))
    {
      mbedtls_pk_free(&ssl_param->pkey);
    }
    mbedtls_ssl_free(&ssl_param->ssl);
    mbedtls_ssl_config_free(&ssl_param->conf);

    vPortFree(ssl_param);
    ssl_param = NULL;
  }
  return NULL;
  /* USER CODE BEGIN mbedtls_ssl_connect_End */

  /* USER CODE END mbedtls_ssl_connect_End */
}

static int32_t net_would_block(void *ctx)
{
  /* USER CODE BEGIN net_would_block_1 */

  /* USER CODE END net_would_block_1 */
  int32_t err = errno;
  int32_t fd = *(int32_t *)ctx;

  /* Never return 'WOULD BLOCK' on a non-blocking socket */
  if ((fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK)
  {
    errno = err;
    return 0;
  }

  switch (errno = err)
  {
#if defined EAGAIN
    case EAGAIN:
#endif /* EAGAIN */
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif /* EWOULDBLOCK */
      return 1;
  }
  /* USER CODE BEGIN net_would_block_2 */

  /* USER CODE END net_would_block_2 */
  return 0;

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
