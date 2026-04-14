/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    http_client.c
  * @author  ST67 Application Team
  * @brief   This file provides code fora simple HTTP client API
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

/*
 * Portions of this file are based on LwIP's http client example application, LwIP version is 2.2.0.
 * Which is licensed under modified BSD-3 Clause license as indicated below.
 * See https://savannah.nongnu.org/projects/lwip/ for more information.
 *
 * Reference source :
 * https://github.com/lwip-tcpip/lwip/blob/master/src/apps/http/http_client.c
 *
 */

/*
 * Copyright (c) 2018 Simon Goldschmidt <goldsimon@gmx.de>
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_client.h"
#include "altls_mbedtls.h"
#include "w6x_default_config.h"
#include "logging.h"

#include "FreeRTOS.h"
#include "task.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  HTTP request task context structure
  */
struct http_request_task_obj
{
  int32_t sock;                         /*!< Socket used for the HTTP request */
  uint16_t port;                        /*!< Destination port (LWIP_IANA_PORT_HTTPS => HTTPS) */
  char *uri;                            /*!< URI to request */
  ip_addr_t server;                     /*!< Server IP address */
  uint8_t req_type;                     /*!< HTTP request type (HTTP_REQ_TYPE_*) */
  HTTP_Post_Data_t post_data;           /*!< Data to post */
  size_t post_data_len;                 /*!< Length of the data to post */
  HTTP_connection_t settings;           /*!< HTTP connection settings */
};

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#ifndef LWIP_IANA_PORT_HTTPS
/** Port number for TLS over HTTP according to IANA */
#define LWIP_IANA_PORT_HTTPS 443U
#endif /* LWIP_IANA_PORT_HTTPS */

#ifndef HTTP_CLIENT_THREAD_STACK_SIZE
/** HTTP Client thread stack size */
#define HTTP_CLIENT_THREAD_STACK_SIZE           2048U
#endif /* HTTP_CLIENT_THREAD_STACK_SIZE */

#ifndef HTTP_CLIENT_THREAD_PRIO
/** HTTP Client thread priority */
#define HTTP_CLIENT_THREAD_PRIO                 30
#endif /* HTTP_CLIENT_THREAD_PRIO */

#ifndef HTTP_CLIENT_DATA_RECV_SIZE
/** HTTP data maximum requested data size */
#define HTTP_CLIENT_DATA_RECV_SIZE              1024U
#endif /* HTTP_CLIENT_DATA_RECV_SIZE */

/** Maximum size of data to be received in a single HTTP transaction */
#if (HTTP_CLIENT_DATA_RECV_SIZE < 4096U)
#define HTTP_CLIENT_MAX_DATA_SEND_BUFFER_SIZE   4096U
#else
#define HTTP_CLIENT_MAX_DATA_SEND_BUFFER_SIZE   HTTP_CLIENT_DATA_RECV_SIZE
#endif /* HTTP_CLIENT_MAX_DATA_SEND_BUFFER_SIZE */

/** Maximum size of accepted HTTP response buffer */
#define HTTP_CLIENT_HEAD_MAX_RESP_BUFFER_SIZE   2048U

/** HTTP Client custom version string */
#define HTTP_CLIENT_CUSTOM_VERSION_STRING       "1.0"

/** HTTP client agent string */
#define HTTP_CLIENT_AGENT                       "stm32/" HTTP_CLIENT_CUSTOM_VERSION_STRING

/** Value to configure the SSL context in client mode */
#define HTTPS_CLIENT_MODE                        1U

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/** HEAD request with host */
#define HTTPC_REQ_HEAD_11_HOST                                                         \
  "HEAD %s HTTP/1.1\r\n"  /* URI */                                                    \
  "User-Agent: %s\r\n"    /* User-Agent */                                             \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"          /* Server name */                                            \
  "Connection: close\r\n" /* Don't yet support persistent connections */               \
  "\r\n"

/** HEAD request with host format */
#define HTTPC_REQ_HEAD_11_HOST_FORMAT(uri, srv_name) \
  HTTPC_REQ_HEAD_11_HOST, uri, HTTP_CLIENT_AGENT, srv_name

/** HEAD request basic */
#define HTTPC_REQ_HEAD_11                                                              \
  "HEAD %s HTTP/1.1\r\n"  /* URI */                                                    \
  "User-Agent: %s\r\n"    /* User-Agent */                                             \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"          /* Server name */                                            \
  "Connection: close\r\n" /* Don't yet support persistent connections */               \
  "\r\n"

/** HEAD request basic format */
#define HTTPC_REQ_HEAD_11_FORMAT(uri, srv_name) \
  HTTPC_REQ_HEAD_11, uri, HTTP_CLIENT_AGENT, srv_name

/** GET request basic */
#define HTTPC_REQ_10                                                                   \
  "GET %s HTTP/1.0\r\n"   /* URI */                                                    \
  "User-Agent: %s\r\n"    /* User-Agent */                                             \
  "Accept: */*\r\n"                                                                    \
  "Connection: close\r\n" /* Don't yet support persistent connections */               \
  "\r\n"

/** GET request basic format */
#define HTTPC_REQ_10_FORMAT(uri) HTTPC_REQ_10, uri, HTTP_CLIENT_AGENT

/** GET request basic simple */
#define HTTPC_REQ_10_SIMPLE                                                            \
  "GET %s HTTP/1.0\r\n" /* URI */                                                      \
  "\r\n"

/** GET request basic simple format */
#define HTTPC_REQ_10_SIMPLE_FORMAT(uri) HTTPC_REQ_10_SIMPLE, uri

/** GET request with host */
#define HTTPC_REQ_11_HOST                                                              \
  "GET %s HTTP/1.1\r\n"   /* URI */                                                    \
  "User-Agent: %s\r\n"    /* User-Agent */                                             \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"          /* Server name */                                            \
  "Connection: close\r\n" /* Don't yet support persistent connections */               \
  "\r\n"

/** GET request with host format */
#define HTTPC_REQ_11_HOST_FORMAT(uri, srv_name) \
  HTTPC_REQ_11_HOST, uri, HTTP_CLIENT_AGENT, srv_name

/** GET request with proxy */
#define HTTPC_REQ_11_PROXY                                                             \
  "GET http://%s%s HTTP/1.1\r\n" /* HOST, URI */                                       \
  "User-Agent: %s\r\n"           /* User-Agent */                                      \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"                 /* Server name */                                     \
  "Connection: close\r\n"        /* Don't yet support persistent connections */        \
  "\r\n"

/** GET request with proxy format */
#define HTTPC_REQ_11_PROXY_FORMAT(host, uri, srv_name) \
  HTTPC_REQ_11_PROXY, host, uri, HTTP_CLIENT_AGENT, srv_name

/** GET request with proxy (non-default server port) */
#define HTTPC_REQ_11_PROXY_PORT                                                        \
  "GET http://%s:%d%s HTTP/1.1\r\n" /* HOST, host-port, URI */                         \
  "User-Agent: %s\r\n"              /* User-Agent */                                   \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"                    /* Server name */                                  \
  "Connection: close\r\n"           /* Don't yet support persistent connections */     \
  "\r\n"

/** GET request with proxy (non-default server port) format */
#define HTTPC_REQ_11_PROXY_PORT_FORMAT(host, host_port, uri, srv_name) \
  HTTPC_REQ_11_PROXY_PORT, host, host_port, uri, HTTP_CLIENT_AGENT, srv_name

/** POST/PUT request basic format */
#define HTTPC_REQ_POST_PUT_11                                                          \
  "%s %s HTTP/1.1\r\n"                                                                 \
  "User-Agent: %s\r\n"              /* User-Agent */                                   \
  "Accept: */*\r\n"                                                                    \
  "Accept-Encoding: deflate, gzip\r\n"                                                 \
  "Host: %s\r\n"                                                                       \
  "Content-Type: %s\r\n"                                                               \
  "Content-Length: %d\r\n"                                                             \
  "Connection: close\r\n"           /* Don't yet support persistent connections */     \
  "\r\n%s"

/** POST request with host */
#define HTTPC_REQ_POST_11_FORMAT(uri, srv_name, content_type, length, data) \
  HTTPC_REQ_POST_PUT_11, "POST", uri, HTTP_CLIENT_AGENT, srv_name, content_type, length, data

/** PUT request with host */
#define HTTPC_REQ_PUT_11_FORMAT(uri, srv_name, content_type, length, data) \
  HTTPC_REQ_POST_PUT_11, "PUT", uri, HTTP_CLIENT_AGENT, srv_name, content_type, length, data

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined(MBEDTLS_CONFIG_FILE)
/** Supported HTTP versions (ALPN list) */
static const char *alpn_list[] = {"http/1.1", NULL};

/** SSL connection context */
static ssl_conn_t *conn_param = NULL;

/** SSL configuration context */
static ssl_config_ctx_t *ssl_param = NULL;
#endif /* MBEDTLS_CONFIG_FILE */

#if LWIP_SO_RCVBUF
/** HTTP client TCP socket size */
static const int32_t http_optval = HTTP_CLIENT_TCP_SOCKET_SIZE;
#endif /* LWIP_SO_RCVBUF */

#if LWIP_SO_RCVTIMEO
/** HTTP client TCP socket data receive timeout */
static const int32_t http_timeout = HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT;
#endif /* LWIP_SO_RCVTIMEO */

/** HTTP content type strings */
static const char *http_content_type_str[] =
{
  "text/plain",                         /* HTTP_CONTENT_TYPE_PLAIN_TEXT */
  "application/x-www-form-urlencoded",  /* HTTP_CONTENT_TYPE_URL_ENCODED */
  "application/json",                   /* HTTP_CONTENT_TYPE_JSON */
  "application/xml",                    /* HTTP_CONTENT_TYPE_XML */
  "application/octet-stream"            /* HTTP_CONTENT_TYPE_OCTET_STREAM */
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  HTTP client task
  * @param  arg: Task argument
  */
static void HTTP_Client_task(void *arg);

/**
  * @brief  Parse the response header content length value
  * @param  buffer: buffer containing the http response
  * @param  content_length: pointer to the content length value
  * @return HTTP_CLIENT_SUCCESS if success, HTTP_CLIENT_ERR otherwise
  */
static int32_t HTTP_Parse_Content_Length(const uint8_t *buffer, uint32_t *content_length);

/**
  * @brief  Parse http header response status
  * @param  buffer: buffer containing the http response
  * @param  http_version: http version
  * @param  http_status: http status code
  * @return HTTP_CLIENT_SUCCESS if success, HTTP_CLIENT_ERR otherwise
  */
static int32_t HTTP_Parse_Response_Status(const uint8_t *buffer, uint16_t *http_version,
                                          HTTP_Status_Code_e *http_status);

/**
  * @brief  Function to get the category of a status code
  * @param  status: status code defined by HTTP_Status_Code_e
  * @param  category: pointer to the category of the status code
  * @return category of the status code, list of possible value are defined by HTTP_Status_Category_e
  */
static int32_t HTTP_Get_Status_Category(HTTP_Status_Code_e status, HTTP_Status_Category_e *category);

/**
  * @brief  Wait for for full header response to be received \
  *         Ending of an header response is CRLF CRLF \
  *         buffer should be big enough to receive the whole response header
  * @param  sock: socket to use
  * @param  buffer: buffer to store the response
  * @param  port: destination port (LWIP_IANA_PORT_HTTPS => HTTPS)
  * @param  body_start_offset: buffer containing the start of the HTTP request body if any.
  *         If none is expected or yet received, body_start points to the end of the HTTP header in buffer.
  * @return HTTP_CLIENT_SUCCESS if header was received successfully, HTTP_CLIENT_ERR otherwise
  */
static int32_t HTTP_Client_Wait_For_Header(int32_t sock, HTTP_buffer_t *buffer,
                                           uint16_t port,
                                           uint32_t *body_start_offset);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
int32_t HTTP_Client_Request(const ip_addr_t *server_addr, uint16_t port,
                            const char *uri, uint8_t req_type,
                            const HTTP_Post_Data_t *post_data, size_t post_data_len,
                            HTTP_result_cb_t result_fn, void *callback_arg,
                            HTTP_headers_done_cb_t headers_done_fn,
                            HTTP_data_cb_t data_fn,
                            const HTTP_connection_t *settings)
{
  int32_t sock = -1;
  struct sockaddr_in addr = {0};
#if (W6X_NET_IPV6_ENABLE == 1)
  struct sockaddr_in6 addr6 = {0};
#endif /* W6X_NET_IPV6_ENABLE */
#if defined(MBEDTLS_CONFIG_FILE)
  ssl_conn_param_t params = {0};
#endif /* MBEDTLS_CONFIG_FILE */
  struct http_request_task_obj *Obj = NULL;
  int family = AF_INET;

#if (W6X_NET_IPV6_ENABLE == 1)
  if (server_addr->type == W6X_NET_IPV6)
  {
    family = AF_INET6;
  }
#endif /* W6X_NET_IPV6_ENABLE */
  /* Create a TCP socket  if the port is not LWIP_IANA_PORT_HTTPS which is used for https */
  if (port != LWIP_IANA_PORT_HTTPS)
  {
    sock = (int32_t)socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
      LogError("Socket creation failed\n");
      return HTTP_CLIENT_ERR;
    }
  }
  else
  {
#if defined(MBEDTLS_CONFIG_FILE)
    /* Set no certificate for this example.
     * As a client, a CA could be configured here using ca_cert and ca_cert_len.*/
    params.own_cert = NULL;
    params.own_cert_len = 0;
    params.private_cert = NULL;
    params.private_cert_len = 0;
    params.ca_cert = NULL;
    params.ca_cert_len = 0;
    /* Sets the ALPNs supported */
    params.alpn = alpn_list;
    params.alpn_num = 1;
    /* Configure the SSL context */
    ssl_param = ssl_configure(&params, HTTPS_CLIENT_MODE);
    if (ssl_param == NULL)
    {
      LogError("Invalid configuration of SSL\n");
      goto _err;
    }
#endif /* MBEDTLS_CONFIG_FILE */
    /* HTTPS over mbedTLS: underlying transport is a plain TCP socket */
    sock = (int32_t)socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
      LogError("Socket creation failed\n");
      return HTTP_CLIENT_ERR;
    }
  }

  LogDebug("Socket creation done\n");
#if LWIP_SO_RCVTIMEO
  /* Set the receive timeout value of the TCP socket. Timeval structure is used or not depending of the availability  */
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  {
    const int32_t so_timeout = timeout;
    if (0 != setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &http_timeout, sizeof(http_timeout)))
    {
      LogError("Socket set timeout option failed\n");
      goto _err;
    }
  }
#else
  {
    struct timeval so_timeout;
    so_timeout.tv_sec = http_timeout / 1000;
    so_timeout.tv_usec = (http_timeout % 1000) * 1000;
    if (0 != setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &so_timeout, sizeof(so_timeout)))
    {
      LogError("Socket set timeout option failed\n");
      goto _err;
    }
  }
#endif /* LWIP_SO_SNDRCVTIMEO_NONSTANDARD */
#endif  /* LWIP_SO_RCVTIMEO */

#if LWIP_SO_RCVBUF
  if (0 != setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &http_optval, sizeof(http_optval)))
  {
    LogError("Socket set receive buff option failed\n");
    goto _err;
  }
#endif /* LWIP_SO_RCVBUF */

  /* Supports IPv4 and IPv6 without DNS */
#if (W6X_NET_IPV6_ENABLE == 1)
  if (family == AF_INET6)
  {
    addr6.sin6_family = family;
    addr6.sin6_port = PP_HTONS(port);
    (void)memcpy(addr6.sin6_addr.un.u32_addr, server_addr->u_addr.ip6.addr, sizeof(addr6.sin6_addr.un.u32_addr));
    if (0 != connect(sock, (struct sockaddr *)&addr6, sizeof(addr6)))
    {
      LogError("Socket connection failed\n");
      goto _err;
    }
  }
  else
#endif /* W6X_NET_IPV6_ENABLE */
  {
    addr.sin_family = family;
    addr.sin_port = PP_HTONS(port);
    addr.sin_addr.s_addr = server_addr->u_addr.ip4.addr;
    if (0 != connect(sock, (struct sockaddr *)&addr, sizeof(addr)))
    {
      LogError("Socket connection failed\n");
      goto _err;
    }
  }

  LogDebug("Socket %" PRIi32 " connected\n", sock);
  Obj = pvPortMalloc(sizeof(struct http_request_task_obj));
  if (Obj == NULL)
  {
    LogError("Callback structure allocation failed\n");
    goto _err;
  }
  (void)memset(Obj, 0, sizeof(struct http_request_task_obj));
  Obj->sock = sock;
  Obj->port = port;
  Obj->post_data_len = post_data_len;

  if ((post_data != NULL) && (post_data->data != NULL) && (post_data_len > 0U))
  {
    Obj->post_data.data = pvPortMalloc(post_data_len + 1U);
    if (Obj->post_data.data == NULL)
    {
      vPortFree(Obj);
      LogError("Post data allocation failed\n");
      goto _err;
    }
    (void)memcpy(Obj->post_data.data, post_data->data, Obj->post_data_len);
    Obj->post_data.data[Obj->post_data_len] = '\0';
    Obj->post_data.type = post_data->type;
  }

  Obj->uri = pvPortMalloc(strlen(uri) + 1U);
  if (Obj->uri == NULL)
  {
    vPortFree(Obj->post_data.data);
    vPortFree(Obj);
    LogError("Callback structure allocation failed\n");
    goto _err;
  }
  (void)strncpy(Obj->uri, uri, strlen(uri));
  Obj->uri[strlen(uri)] = '\0';
  Obj->req_type = req_type;
  (void)memcpy(&Obj->server, server_addr, sizeof(ip_addr_t));
  (void)memcpy(&Obj->settings, settings, sizeof(HTTP_connection_t));

  if (settings->server_name == NULL)
  {
    Obj->settings.server_name = NULL;
  }
  else
  {
    Obj->settings.server_name = pvPortMalloc(strlen(settings->server_name) + 1U);
    if (Obj->settings.server_name == NULL)
    {
      vPortFree(Obj->uri);
      vPortFree(Obj->post_data.data);
      vPortFree(Obj);
      LogError("Callback structure allocation failed\n");
      goto _err;
    }
    (void)strncpy(Obj->settings.server_name, settings->server_name, strlen(settings->server_name));
    Obj->settings.server_name[strlen(settings->server_name)] = '\0';
  }

#if defined(MBEDTLS_CONFIG_FILE)
  if (port == LWIP_IANA_PORT_HTTPS)
  {
    conn_param = ssl_secure_connection(Obj->sock, ssl_param, 1, Obj->settings.server_name);
    if (conn_param == NULL)
    {
      goto _err;
    }
  }
#endif /* MBEDTLS_CONFIG_FILE */

  /* Since callbacks info is duplicated, if the one is not set, try using the other */
  if (Obj->settings.headers_done_fn == NULL)
  {
    Obj->settings.headers_done_fn = headers_done_fn;
  }
  if (Obj->settings.result_fn == NULL)
  {
    Obj->settings.result_fn = result_fn;
  }
  if (Obj->settings.callback_arg == NULL)
  {
    Obj->settings.callback_arg = callback_arg;
  }
  if (Obj->settings.recv_fn == NULL)
  {
    Obj->settings.recv_fn = data_fn;
  }

  if ((Obj->settings.headers_done_fn == NULL) &&
      (Obj->settings.result_fn == NULL) &&
      (Obj->settings.recv_fn == NULL))
  {
    LogWarn("No callback has been registered to the HTTP Client task, any error would not be reported\n");
  }
  /*Run HTTP get and close socket in separate thread */
  if (pdPASS == xTaskCreate(HTTP_Client_task, "HTTP task", HTTP_CLIENT_THREAD_STACK_SIZE >> 2U,
                            Obj, HTTP_CLIENT_THREAD_PRIO, NULL))
  {
    return HTTP_CLIENT_SUCCESS;
  }
#if defined(MBEDTLS_CONFIG_FILE)

  /*Failed to start the task, need to free buffer and structures */
  if (port == LWIP_IANA_PORT_HTTPS)
  {
    (void)mbedtls_ssl_close_notify(&conn_param->ssl);
    (void)ssl_destroy(NULL, conn_param);
  }
#endif /* MBEDTLS_CONFIG_FILE */

  if (Obj->settings.server_name != NULL)
  {
    vPortFree(Obj->settings.server_name);
  }
  vPortFree(Obj->post_data.data);
  vPortFree(Obj->uri);
  vPortFree(Obj);
_err:
#if defined(MBEDTLS_CONFIG_FILE)
  if (port == LWIP_IANA_PORT_HTTPS)
  {
    (void)ssl_destroy(ssl_param, NULL);
  }
  else
#endif /* MBEDTLS_CONFIG_FILE */
  {
    (void)close(sock);
  }
  return HTTP_CLIENT_ERR;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void HTTP_Client_task(void *arg)
{
  struct http_request_task_obj *Obj = (struct http_request_task_obj *) arg;
  uint32_t content_length = 0;
  int32_t req_len = 0;
  int32_t req_len2 = 0;
  int32_t total_recv_data = 0;
  uint8_t *req_buffer = NULL;
  int32_t bytes_sent;
  ssize_t recv_len;
  int32_t error = -1;
  HTTP_buffer_t http_buffer = {0};
  HTTP_Status_Code_e http_status = HTTP_VERSION_NOT_SUPPORTED;
  HTTP_Status_Category_e http_category = HTTP_CATEGORY_UNKNOWN;
  char *content_type = NULL;
  uint16_t http_version = 0;
  uint32_t offset = 0;
  /* SNI max size is used for hostname since it is the same information used
   * in both instances and SNI is limited to HTTP_SNI_MAX_SIZE bytes */
  char host_name[HTTP_SNI_MAX_SIZE + 1] = {0};

  if ((Obj == NULL) || (Obj->sock < 0) || (Obj->uri == NULL))
  {
    LogError("Invalid input parameters\n");
    return;
  }

  if ((Obj->settings.server_name != NULL) && (strlen(Obj->settings.server_name) <= HTTP_SNI_MAX_SIZE))
  {
    (void)strncpy(host_name, Obj->settings.server_name, sizeof(host_name) - 1U);
  }
  else
  {
#if (W6X_NET_IPV6_ENABLE == 1)
    if (Obj->server.type == W6X_NET_IPV6)
    {
      (void)inet_ntop(AF_INET6, (void *) &Obj->server.u_addr.ip6, host_name, INET6_ADDRSTRLEN);
    }
    else
#endif /* W6X_NET_IPV6_ENABLE */
    {
      (void)inet_ntop(AF_INET, (void *) &Obj->server.u_addr.ip4, host_name, INET_ADDRSTRLEN);
    }
  }

  if (Obj->post_data.type > HTTP_CONTENT_TYPE_OCTET_STREAM)
  {
    content_type = (char *)http_content_type_str[HTTP_CONTENT_TYPE_PLAIN_TEXT];
  }
  else
  {
    content_type = (char *)http_content_type_str[Obj->post_data.type];
  }

  switch (Obj->req_type)
  {
    case HTTP_REQ_TYPE_HEAD:
      /* Get the length of the HTTP request */
      req_len = strlen(HTTPC_REQ_HEAD_11) + strlen(Obj->uri) + strlen(HTTP_CLIENT_AGENT) + strlen(host_name);
      /* Allocate dynamically the HTTP request based on previous result */
      req_buffer = pvPortMalloc(req_len);
      if (req_buffer == NULL)
      {
        goto _err;
      }
      /* Prepare send request */
      req_len2 = snprintf((char *)req_buffer, req_len, HTTPC_REQ_HEAD_11_FORMAT(Obj->uri, host_name));
      break;

    case HTTP_REQ_TYPE_GET:
      /* Get the length of the HTTP request */
      req_len = strlen(HTTPC_REQ_11_HOST) + strlen(Obj->uri) + strlen(HTTP_CLIENT_AGENT) + strlen(host_name);
      /* Allocate dynamically the HTTP request based on previous result */
      req_buffer = pvPortMalloc(req_len);
      if (req_buffer == NULL)
      {
        goto _err;
      }
      /* Prepare send request */
      req_len2 = snprintf((char *)req_buffer, req_len, HTTPC_REQ_11_HOST_FORMAT(Obj->uri, host_name));
      break;

    case HTTP_REQ_TYPE_PUT:
      /* Get the length of the HTTP request */
      req_len = strlen(HTTPC_REQ_POST_PUT_11) + strlen(Obj->uri) + strlen(HTTP_CLIENT_AGENT)
                + strlen(host_name) + Obj->post_data_len + strlen(content_type);
      /* Allocate dynamically the HTTP request based on previous result */
      req_buffer = pvPortMalloc(req_len);
      if (req_buffer == NULL)
      {
        goto _err;
      }
      /* Prepare send request */
      req_len2 = snprintf((char *)req_buffer, req_len,
                          HTTPC_REQ_PUT_11_FORMAT(Obj->uri, host_name, content_type,
                                                  Obj->post_data_len, (char *)Obj->post_data.data));
      break;

    case HTTP_REQ_TYPE_POST:
      /* Get the length of the HTTP request */
      req_len = strlen(HTTPC_REQ_POST_PUT_11) + strlen(Obj->uri) + strlen(HTTP_CLIENT_AGENT)
                + strlen(host_name) + Obj->post_data_len + strlen(content_type);
      /* Allocate dynamically the HTTP request based on previous result */
      req_buffer = pvPortMalloc(req_len);
      if (req_buffer == NULL)
      {
        goto _err;
      }
      /* Prepare send request */
      req_len2 = snprintf((char *)req_buffer, req_len,
                          HTTPC_REQ_POST_11_FORMAT(Obj->uri, host_name, content_type,
                                                   Obj->post_data_len, (char *)Obj->post_data.data));
      break;

    default:
      LogError("Unknown Request type\n");
      goto _err;
      break;
  }

  if ((req_len2 <= 0) || (req_len2 > req_len))
  {
    vPortFree(req_buffer);
    goto _err;
  }

  LogDebug("Sending HTTP request, request size: (%" PRIi32 ")\n", req_len2);

  /* The variable is only used for a pointer copy temporarily, limited to this scope*/
  {
    /* Save the pointer of the request buffer for any free later on.
     * Original request buffer pointer will be incremented possibly */
    uint8_t *data = req_buffer;
    do
    {
      /* Send the HTTP request to the HTTP server via the TCP socket */
#if defined(MBEDTLS_CONFIG_FILE)
      if (Obj->port == LWIP_IANA_PORT_HTTPS)
      {
        /* When doing HTTPS, mbedtls API is used (mbedtls manages the TCP socket) */
        bytes_sent = (int32_t)mbedtls_ssl_write(&conn_param->ssl, data, (size_t)req_len2);
      }
      else
#endif /* MBEDTLS_CONFIG_FILE */
      {
        bytes_sent = (int32_t)send(Obj->sock, data, (size_t)req_len2, 0);
      }

      if (bytes_sent < 0)
      {
        LogError("Failed to send data to tcp server (%" PRIi32 "), try again\n", bytes_sent);
        vPortFree(req_buffer);
        goto _err;
      }
      req_len2 -= bytes_sent;
      data += bytes_sent;
    } while (req_len2 > 0);

    vPortFree(req_buffer);
  }
  vTaskDelay(pdMS_TO_TICKS(100));
  http_buffer.data = pvPortMalloc(HTTP_CLIENT_MAX_DATA_SEND_BUFFER_SIZE + 1U);
  if (http_buffer.data == NULL)
  {
    goto _err;
  }

  (void)memset(http_buffer.data, 0x00, HTTP_CLIENT_MAX_DATA_SEND_BUFFER_SIZE + 1U);
  /* Wait to receive an answer to the HTTP request sent */
  if (HTTP_CLIENT_SUCCESS != HTTP_Client_Wait_For_Header(Obj->sock, &http_buffer, Obj->port, &offset))
  {
    LogError("Get HTTP header failed\n");
    goto _err2;
  }

  /* Parse the HTTP response status */
  if (HTTP_Parse_Response_Status(http_buffer.data, &http_version, &http_status) != HTTP_CLIENT_SUCCESS)
  {
    LogError("Parse of HTTP response status failed\n");
    goto _err2;
  }

  /* Get the returned status category and check if it is a success. Only support 200 OK currently */
  (void)HTTP_Get_Status_Category(http_status, &http_category);

  /* Parse the content length field of the HTTP response to know how much body content to expect.
     If a body is not expected, set the content length value to 0 */
  if (Obj->req_type == HTTP_REQ_TYPE_GET)
  {
    if (HTTP_Parse_Content_Length(http_buffer.data, &content_length) != HTTP_CLIENT_SUCCESS)
    {
      LogDebug("Content Length tag not found in header\n");
    }
  }

  if (Obj->settings.headers_done_fn != NULL)
  {
    (void)Obj->settings.headers_done_fn(NULL, Obj->settings.callback_arg, http_buffer.data,
                                        offset, content_length);
  }

  if (Obj->settings.result_fn != NULL)
  {
    Obj->settings.result_fn(Obj->settings.callback_arg, http_status, content_length, 0, 0);
  }

  /* If an error occurred, No need to read data, but status code must be returned */
  if (http_category >= HTTP_CATEGORY_CLIENT_ERROR)
  {
    LogError("HTTP_Get_Status_Category failed\n");
    goto _err2;
  }

  if (offset > 0U)
  {
    http_buffer.length -= offset;
    (void)memcpy(http_buffer.data, &http_buffer.data[offset], http_buffer.length);
    http_buffer.data[http_buffer.length] = 0;
  }

  if (http_buffer.length == 0U)
  {
#if defined(MBEDTLS_CONFIG_FILE)
    if (Obj->port == LWIP_IANA_PORT_HTTPS)
    {
      /* When doing HTTPS, mbedtls API is used (mbedtls manages the TCP socket) */
      recv_len = (int32_t)mbedtls_ssl_read(&conn_param->ssl, http_buffer.data, HTTP_CLIENT_DATA_RECV_SIZE);
    }
    else
#endif /* MBEDTLS_CONFIG_FILE */
    {
      recv_len = (int32_t)recv(Obj->sock, http_buffer.data, HTTP_CLIENT_DATA_RECV_SIZE, 0);
    }

    if (recv_len < 0)
    {
      LogError("Net recv failed: %" PRIi32 "\n", recv_len);
      goto _err2;
    }
    http_buffer.length = (size_t)recv_len;
  }

  while (http_buffer.length > 0U)
  {
    total_recv_data += http_buffer.length;
    /* Pass Data to callback if not NULL */
    if ((Obj->settings.recv_fn != NULL) && (Obj->settings.recv_fn(Obj->settings.recv_fn_arg, &http_buffer, 0) < 0))
    {
      LogError("User function for received data processing returned an error");
      break;
    }
    if ((content_length > 0U) && (total_recv_data >= content_length))
    {
      break;
    }
    if (content_length == 0U)
    {
      if (strncmp((char *)&http_buffer.data[http_buffer.length - 4U], "\r\n\r\n", 4) == 0)
      {
        /* The full request has been received leaving the reading loop */
        break;
      }
    }
    (void)memset(http_buffer.data, 0, HTTP_CLIENT_DATA_RECV_SIZE);
#if defined(MBEDTLS_CONFIG_FILE)
    if (Obj->port == LWIP_IANA_PORT_HTTPS)
    {
      /* When doing HTTPS, mbedtls API is used (mbedtls manages the TCP socket) */
      recv_len = (int32_t)mbedtls_ssl_read(&conn_param->ssl, http_buffer.data, HTTP_CLIENT_DATA_RECV_SIZE);
    }
    else
#endif /* MBEDTLS_CONFIG_FILE */
    {
      recv_len = (int32_t)recv(Obj->sock, http_buffer.data, HTTP_CLIENT_DATA_RECV_SIZE, 0);
    }

    if (recv_len < 0)
    {
      LogError("Net recv failed: %" PRIi32 "\n", recv_len);
      goto _err2;
    }
    http_buffer.length = (size_t)recv_len;
  }

  error = 0; /* Success */

_err2:
  vPortFree(http_buffer.data);

_err:
  if (error != 0)
  {
    if (Obj->settings.result_fn != NULL)
    {
      Obj->settings.result_fn(Obj->settings.callback_arg, http_status, 0, 0, -1);
    }
    if (Obj->settings.recv_fn != NULL)
    {
      (void)Obj->settings.recv_fn(Obj->settings.callback_arg, NULL, -1);
    }
  }
#if defined(MBEDTLS_CONFIG_FILE)
  if (Obj->port == LWIP_IANA_PORT_HTTPS)
  {
    /* Notice the peer of socket closure and clean contexts */
    (void)mbedtls_ssl_close_notify(&conn_param->ssl);
    (void)ssl_destroy(ssl_param, conn_param);
  }
  else
#endif /* MBEDTLS_CONFIG_FILE */
  {
    (void)close(Obj->sock);
  }
  if (Obj->settings.server_name != NULL)
  {
    vPortFree(Obj->settings.server_name);
  }
  vPortFree(Obj->post_data.data);
  vPortFree(Obj->uri);
  vPortFree(Obj);
  vTaskDelete(NULL);
}

static int32_t HTTP_Parse_Content_Length(const uint8_t *buffer, uint32_t *content_length)
{
  int32_t ret = HTTP_CLIENT_ERR;
  uint8_t *content_len_hdr;
  uint32_t rx_data_len;
  uint8_t *endptr;

  /* Look for the Content-Length header field */
  content_len_hdr = (uint8_t *)strstr((const char *)buffer, "Content-Length: ");
  if (content_len_hdr == NULL)
  {
    goto _err;
  }
  content_len_hdr += 16U; /* Move past "Content-Length: " */

  /* Parse content len value */
  rx_data_len = strtol((char *)content_len_hdr, (char **)&endptr, 10);
  if ((endptr == content_len_hdr) || (*endptr != '\r'))
  {
    goto _err;
  }
  *content_length = rx_data_len;
  ret = HTTP_CLIENT_SUCCESS;

_err:
  return ret;
}

static int32_t HTTP_Parse_Response_Status(const uint8_t *buffer, uint16_t *http_version,
                                          HTTP_Status_Code_e *http_status)
{
  int32_t ret = HTTP_CLIENT_ERR;
  uint8_t *space1;

  /* Look for the end of the header */
  if ((uint8_t *)strstr((char *)buffer, "\r\n\r\n") == NULL)
  {
    goto _err;
  }

  space1 = (uint8_t *)strstr((char *)buffer, " ");
  if (space1 == NULL)
  {
    goto _err;
  }
  space1++; /* Move past the first space */

  if ((strncmp((char *)buffer, "HTTP/", 5) == 0) && (buffer[6] == '.'))
  {
    uint8_t *endptr;
    uint32_t rx_data_len;
    uint32_t status;
    /* Parse http version */
    uint16_t version = buffer[5] - '0';
    version <<= 8;
    version |= buffer[7] - '0';
    *http_version = version;

    /* Parse HTTP status number */
    rx_data_len = strtol((char *)space1, (char **)&endptr, 10);
    if ((endptr == space1) || (*endptr != ' '))
    {
      goto _err;
    }
    status = rx_data_len;
    if ((status > 0U) && (status <= 0xFFFFU))
    {
      *http_status = (HTTP_Status_Code_e)status;
      ret = HTTP_CLIENT_SUCCESS;
    }
  }

_err:
  return ret;
}

static int32_t HTTP_Get_Status_Category(HTTP_Status_Code_e status, HTTP_Status_Category_e *category)
{
  switch (status / 100)
  {
    case 1:
      *category = HTTP_CATEGORY_INFORMATIONAL;
      break;
    case 2:
      *category = HTTP_CATEGORY_SUCCESS;
      break;
    case 3:
      *category = HTTP_CATEGORY_REDIRECTION;
      break;
    case 4:
      *category = HTTP_CATEGORY_CLIENT_ERROR;
      break;
    case 5:
      *category = HTTP_CATEGORY_SERVER_ERROR;
      break;
    default:
      *category = HTTP_CATEGORY_UNKNOWN;
      break;
  }
  return HTTP_CLIENT_SUCCESS;
}

static int32_t HTTP_Client_Wait_For_Header(int32_t sock, HTTP_buffer_t *buffer,
                                           uint16_t port,
                                           uint32_t *body_start_offset)
{
  int32_t ret = HTTP_CLIENT_ERR;
  uint8_t header_received = 0;
  int32_t data_received = 0;
  uint32_t total_data_received = 0;
  uint8_t *header_end;

  if (buffer->data == NULL)
  {
    goto _err;
  }

  /* Read the headers */
  do
  {
#if defined(MBEDTLS_CONFIG_FILE)
    if (port == LWIP_IANA_PORT_HTTPS)
    {
      /* When doing HTTPS, mbedtls API is used (mbedtls manages the TCP socket) */
      data_received = (int32_t)mbedtls_ssl_read(&conn_param->ssl,
                                                &buffer->data[total_data_received],
                                                HTTP_CLIENT_HEAD_MAX_RESP_BUFFER_SIZE - total_data_received);
    }
    else
#endif /* MBEDTLS_CONFIG_FILE */
    {
      data_received = (int32_t)recv(sock,
                                    &buffer->data[total_data_received],
                                    HTTP_CLIENT_HEAD_MAX_RESP_BUFFER_SIZE - total_data_received, 0);
    }

    if (data_received < 0)
    {
      LogError("Receive failed with %" PRIi32 "\n", data_received);
      goto _err;
    }

    total_data_received += data_received;

    /* Look for the end of the headers */
    header_end = (uint8_t *) strstr((char *)buffer->data, "\r\n\r\n");
    if (header_end != NULL)
    {
      header_received = 1U;
      header_end += 4U; /* + 4U to go past the "\r\n\r\n" termination */

      /* Copy body content to the provided buffer */
      *body_start_offset = (uint32_t)(header_end - &buffer->data[0]);
      break;
    }
  } while (total_data_received < HTTP_CLIENT_HEAD_MAX_RESP_BUFFER_SIZE);
  buffer->length = total_data_received;

  if (header_received == 0U)
  {
    LogError("Failed to receive HTTP headers\n");
    goto _err;
  }

  ret = HTTP_CLIENT_SUCCESS;

_err:
  return ret;
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
