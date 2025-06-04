/**
  ******************************************************************************
  * @file    w6x_http.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x HTTP API and a simple HTTP client API
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
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "w6x_types.h"
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */
#include "common_parser.h" /* Common Parser functions */
#include "w6x_default_config.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_HTTP_Types ST67W6X HTTP Types
  * @ingroup  ST67W6X_Private_HTTP
  * @{
  */

/**
  * @brief  HTTP request task context structure
  */
struct http_request_task_obj
{
  int32_t sock;                         /*!< Socket used for the HTTP request */
  char *uri;                            /*!< URI to request */
  ip_addr_t server;                     /*!< Server IP address */
  char method[6];                       /*!< HTTP method to use */
  W6X_HTTP_Post_Data_t post_data;       /*!< Data to post */
  size_t post_data_len;                 /*!< Length of the data to post */
  W6X_HTTP_connection_t settings;       /*!< HTTP connection settings */
};

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_HTTP_Constants ST67W6X HTTP Constants
  * @ingroup  ST67W6X_Private_HTTP
  * @{
  */

#ifndef W6X_HTTP_CLIENT_THREAD_STACK_SIZE
/** HTTP Client thread stack size */
#define W6X_HTTP_CLIENT_THREAD_STACK_SIZE   2048
#endif /* W6X_HTTP_CLIENT_THREAD_STACK_SIZE */

#ifndef W6X_HTTP_CLIENT_THREAD_PRIO
/** HTTP Client thread priority */
#define W6X_HTTP_CLIENT_THREAD_PRIO         30
#endif /* W6X_HTTP_CLIENT_THREAD_PRIO */

/** Maximum size of accepted HTTP response buffer */
#define W6X_HTTP_HEAD_MAX_RESP_BUFFER_SIZE  2048U

/** HTTP Client custom version string */
#define W6X_HTTP_CUSTOM_VERSION_STRING      "1.0"

/** HTTP client agent string */
#define W6X_HTTP_CLIENT_AGENT               "w6x/" W6X_HTTP_CUSTOM_VERSION_STRING

/** @} */

/* Private macros ------------------------------------------------------------*/
/** @defgroup ST67W6X_Private_HTTP_Macros ST67W6X HTTP Macros
  * @ingroup  ST67W6X_Private_HTTP
  * @{
  */

/** HEAD request with host */
#define HTTPC_REQ_HEAD_11_HOST                                                         \
  "HEAD %s HTTP/1.1\r\n"  /* URI */                                                    \
  "User-Agent: %s\r\n"    /* User-Agent */                                             \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"          /* server name */                                            \
  "Connection: Close\r\n" /* we don't support persistent connections, yet */           \
  "\r\n"

/** HEAD request with host format */
#define HTTPC_REQ_HEAD_11_HOST_FORMAT(uri, srv_name) \
  HTTPC_REQ_HEAD_11_HOST, uri, W6X_HTTP_CLIENT_AGENT, srv_name

/** HEAD request basic */
#define HTTPC_REQ_HEAD_11                                                              \
  "HEAD %s HTTP/1.1\r\n"  /* URI */                                                    \
  "User-Agent: %s\r\n"    /* User-Agent */                                             \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"          /* server name */                                            \
  "Connection: Close\r\n" /* we don't support persistent connections, yet */           \
  "\r\n"

/** HEAD request basic format */
#define HTTPC_REQ_HEAD_11_FORMAT(uri, srv_name) \
  HTTPC_REQ_HEAD_11, uri, W6X_HTTP_CLIENT_AGENT, srv_name

/** GET request basic */
#define HTTPC_REQ_10                                                                   \
  "GET %s HTTP/1.0\r\n"   /* URI */                                                    \
  "User-Agent: %s\r\n"    /* User-Agent */                                             \
  "Accept: */*\r\n"                                                                    \
  "Connection: Close\r\n" /* we don't support persistent connections, yet */           \
  "\r\n"

/** GET request basic format */
#define HTTPC_REQ_10_FORMAT(uri) HTTPC_REQ_10, uri, W6X_HTTP_CLIENT_AGENT

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
  "Host: %s\r\n"          /* server name */                                            \
  "Connection: Close\r\n" /* we don't support persistent connections, yet */           \
  "\r\n"

/** GET request with host format */
#define HTTPC_REQ_11_HOST_FORMAT(uri, srv_name) \
  HTTPC_REQ_11_HOST, uri, W6X_HTTP_CLIENT_AGENT, srv_name

/** GET request with proxy */
#define HTTPC_REQ_11_PROXY                                                             \
  "GET http://%s%s HTTP/1.1\r\n" /* HOST, URI */                                       \
  "User-Agent: %s\r\n"           /* User-Agent */                                      \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"                 /* server name */                                     \
  "Connection: Close\r\n"        /* we don't support persistent connections, yet */    \
  "\r\n"

/** GET request with proxy format */
#define HTTPC_REQ_11_PROXY_FORMAT(host, uri, srv_name) \
  HTTPC_REQ_11_PROXY, host, uri, W6X_HTTP_CLIENT_AGENT, srv_name

/** GET request with proxy (non-default server port) */
#define HTTPC_REQ_11_PROXY_PORT                                                        \
  "GET http://%s:%d%s HTTP/1.1\r\n" /* HOST, host-port, URI */                         \
  "User-Agent: %s\r\n"              /* User-Agent */                                   \
  "Accept: */*\r\n"                                                                    \
  "Host: %s\r\n"                    /* server name */                                  \
  "Connection: Close\r\n"           /* we don't support persistent connections, yet */ \
  "\r\n"

/** GET request with proxy (non-default server port) format */
#define HTTPC_REQ_11_PROXY_PORT_FORMAT(host, host_port, uri, srv_name) \
  HTTPC_REQ_11_PROXY_PORT, host, host_port, uri, W6X_HTTP_CLIENT_AGENT, srv_name

/** POST/PUT request basic format */
#define HTTPC_REQ_POST_PUT_11                                                          \
  "%s %s HTTP/1.1\r\n"                                                                 \
  "User-Agent: %s\r\n"              /* User-Agent */                                   \
  "Accept: */*\r\n"                                                                    \
  "Accept-Encoding: deflate, gzip\r\n"                                                 \
  "Host: %s\r\n"                                                                       \
  "Content-Type: %s\r\n"                                                               \
  "Content-Length: %d\r\n"                                                             \
  "Connection: keep-alive"          /* we don't support persistent connections, yet */ \
  "\r\n\r\n%s"

/** POST request with host */
#define HTTPC_REQ_POST_11_FORMAT(uri, srv_name, content_type, length, data) \
  HTTPC_REQ_POST_PUT_11, "POST", uri, W6X_HTTP_CLIENT_AGENT, srv_name, content_type, length, data

/** PUT request with host */
#define HTTPC_REQ_PUT_11_FORMAT(uri, srv_name, content_type, length, data) \
  HTTPC_REQ_POST_PUT_11, "PUT", uri, W6X_HTTP_CLIENT_AGENT, srv_name, content_type, length, data

/** @} */

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Private_HTTP_Functions ST67W6X HTTP Functions
  * @ingroup  ST67W6X_Private_HTTP
  * @{
  */

/**
  * @brief  HTTP client task
  * @param  arg: Task argument
  */
static void W6X_HTTP_Client_task(void *arg);

/**
  * @brief  Parse the response header content length value
  * @param  buffer: buffer containing the http response
  * @param  content_length: pointer to the content length value
  * @return W6X_STATUS_OK if success, W6X_STATUS_ERROR otherwise
  */
static W6X_Status_t W6X_HTTP_Parse_Content_Length(const uint8_t *buffer, uint32_t *content_length);

/**
  * @brief  Parse http header response status
  * @param  buffer: buffer containing the http response
  * @param  http_version: http version
  * @param  http_status: http status code
  * @return W6X_STATUS_OK if success, W6X_STATUS_ERROR otherwise
  */
static W6X_Status_t W6X_HTTP_Parse_Response_Status(const uint8_t *buffer, uint16_t *http_version,
                                                   W6X_HTTP_Status_Code_e *http_status);

/**
  * @brief  Function to get the category of a status code
  * @param  status: status code defined by W6X_HTTP_Status_Code_e
  * @param  category: pointer to the category of the status code
  * @return category of the status code, list of possible value are defined by W6X_HTTP_Status_Category_e
  */
static W6X_Status_t W6X_HTTP_Get_Status_Category(W6X_HTTP_Status_Code_e status, W6X_HTTP_Status_Category_e *category);

/**
  * @brief  Wait for for full header response to be received \
  *         Ending of an header response is CRLF CRLF \
  *         buffer should be big enough to receive the whole response header
  * @param  sock: socket to use
  * @param  buffer: buffer to store the response
  * @param  body_start_offset: buffer containing the start of the HTTP request body if any.
  *         If none is expected or yet received, body_start points to the end of the HTTP header in buffer.
  * @return W6X_STATUS_OK if header was received successfully, W6X_STATUS_ERROR otherwise
  */
static W6X_Status_t W6X_HTTP_Client_Wait_For_Header(int32_t sock, W6X_HTTP_buffer_t *buffer,
                                                    uint32_t *body_start_offset);

/** @} */

/* Functions Definition ------------------------------------------------------*/
W6X_Status_t W6X_HTTP_Client_Request(const ip_addr_t *server_addr, uint16_t port, const char *uri, const char *method,
                                     const void *post_data, size_t post_data_len, W6X_HTTP_result_cb_t result_fn,
                                     void *callback_arg, W6X_HTTP_headers_done_cb_t headers_done_fn,
                                     W6X_HTTP_data_cb_t data_fn, const W6X_HTTP_connection_t *settings)
{
  int32_t sock;
  struct sockaddr_in addr_t = {0};
  int32_t optval = W6X_HTTP_CLIENT_TCP_SOCKET_SIZE;
  int32_t timeout = W6X_HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT;
  uint8_t sec_tag_list[] = { 0 };
  char alpn_list[16] = "http1.1,http1.2";
  struct http_request_task_obj *Obj = NULL;
  /* Create a TCP socket  if the port is not 443 which is used for https*/
  if (port != 443)
  {
    sock = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
      LogError("Socket creation failed\n");
      return W6X_STATUS_ERROR;
    }
  }
  else
  {
    sock = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
    if (sock < 0)
    {
      LogError("Socket creation failed\n");
      return W6X_STATUS_ERROR;
    }

    /** Use a tag that has the value of the socket id to be able to run multiple
      * without waiting for the previous to end */
    if ((settings->https_certificate != NULL) && (settings->https_certificate_len > 0))
    {
      sec_tag_list[0] = sock;
      if (W6X_Net_Tls_Credential_Add(sec_tag_list[0], W6X_NET_TLS_CREDENTIAL_CA_CERTIFICATE,
                                     (void *)settings->https_certificate, settings->https_certificate_len) != 0)
      {
        LogError("Socket creation failed\n");
        W6X_Net_Close(sock);
        return W6X_STATUS_ERROR;
      }

      if (W6X_Net_Setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list, sizeof(sec_tag_list)) != 0)
      {
        LogError("Set socket option failed\n");
        goto _err;
      }
    }

    LogDebug("Socket option set\n");

    if (W6X_Net_Setsockopt(sock, SOL_TLS, TLS_ALPN_LIST, alpn_list, 2) != 0)
    {
      LogError("Set ALPN failed\n");
      goto _err;
    }

    LogDebug("ALPN set\n");

    if (W6X_Net_Setsockopt(sock, SOL_TLS, TLS_HOSTNAME, settings->server_name, strlen(settings->server_name)) != 0)
    {
      LogError("Set SNI failed\n");
      goto _err;
    }

    LogDebug("SNI set\n");
  }

  LogDebug("Socket creation done\n");
  /* Configure receive data timeout for the TCP socket */
  if (0 != W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)))
  {
    LogError("Socket set timeout option failed\n");
    goto _err;
  }

  /* Configure the socket receive buffer size */
  if (0 != W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval)))
  {
    LogError("Socket set receive buff option failed\n");
    goto _err;
  }

  /* supports only IPv4 without DNS */
  addr_t.sin_family = AF_INET;
  addr_t.sin_port = PP_HTONS(port);
  addr_t.sin_addr_t.s_addr = server_addr->u_addr.ip4;
  if (0 != W6X_Net_Connect(sock, (struct sockaddr *)&addr_t, sizeof(addr_t)))
  {
    LogError("Socket connection failed\n");
    goto _err;
  }

  LogDebug("Socket %" PRIi32 " connected\n", sock);
  Obj = pvPortMalloc(sizeof(struct http_request_task_obj));
  if (Obj == NULL)
  {
    LogError("Callback structure allocation failed\n");
    goto _err;
  }
  memset(Obj, 0, sizeof(struct http_request_task_obj));
  Obj->sock = sock;
  Obj->post_data_len = post_data_len;

  if ((post_data != NULL) && (post_data_len > 0))
  {
    Obj->post_data.data = pvPortMalloc(post_data_len + 1);
    if (Obj->post_data.data == NULL)
    {
      vPortFree(Obj);
      LogError("Post data allocation failed\n");
      goto _err;
    }
    memcpy(Obj->post_data.data, ((W6X_HTTP_Post_Data_t *)post_data)->data, Obj->post_data_len);
    ((char *) Obj->post_data.data)[Obj->post_data_len] = 0;
    Obj->post_data.type = ((W6X_HTTP_Post_Data_t *)post_data)->type;
  }

  Obj->uri = pvPortMalloc(strlen(uri) + 1);
  if (Obj->uri == NULL)
  {
    vPortFree(Obj->post_data.data);
    vPortFree(Obj);
    LogError("Callback structure allocation failed\n");
    goto _err;
  }
  strncpy(Obj->uri, uri, strlen(uri));
  Obj->uri[strlen(uri)] = 0;
  strncpy(Obj->method, method, 5);
  memcpy(&Obj->server, server_addr, sizeof(ip_addr_t));
  memcpy(&Obj->settings, settings, sizeof(W6X_HTTP_connection_t));

  /* Since callbacks info is duplicated, if the one we will use is not set, try using the other */
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
  if (pdPASS == xTaskCreate(W6X_HTTP_Client_task, "HTTP task", W6X_HTTP_CLIENT_THREAD_STACK_SIZE >> 2,
                            Obj, W6X_HTTP_CLIENT_THREAD_PRIO, NULL))
  {
    return W6X_STATUS_OK;
  }

  /*Failed to start the task, need to free buffer and structures */
  vPortFree(Obj->post_data.data);
  vPortFree(Obj->uri);
  vPortFree(Obj);
_err:
  W6X_Net_Tls_Credential_Delete(sock, W6X_NET_TLS_CREDENTIAL_CA_CERTIFICATE);
  W6X_Net_Close(sock);
  return W6X_STATUS_ERROR;
}

/* Private Functions Definition ----------------------------------------------*/
/** @addtogroup ST67W6X_Private_HTTP_Functions
  * @{
  */

static void W6X_HTTP_Client_task(void *arg)
{
  struct http_request_task_obj *Obj = (struct http_request_task_obj *) arg;
  uint32_t content_length = 0;
  int32_t req_len = 0;
  int32_t req_len2 = 0;
  int32_t total_recv_data = 0;
  uint8_t *req_buffer = NULL;
  int32_t count_done;
  int32_t error = -1;
  W6X_HTTP_buffer_t *buff_head;
  W6X_HTTP_Status_Code_e http_status = HTTP_VERSION_NOT_SUPPORTED;
  W6X_HTTP_Status_Category_e http_category = HTTP_CATEGORY_UNKNOWN;
  char *content_type = NULL;
  uint16_t http_version = 0;
  uint32_t offset = 0;
  int8_t method;
  char host_name[INET_ADDRSTRLEN] = {0};
  (void)W6X_Net_Inet_ntop(AF_INET, (void *) &Obj->server.u_addr.ip4, host_name, INET_ADDRSTRLEN);

  switch (Obj->post_data.type)
  {
    case W6X_HTTP_CONTENT_TYPE_PLAIN_TEXT:
      content_type = "text/plain";
      break;
    case W6X_HTTP_CONTENT_TYPE_URL_ENCODED:
      content_type = "application/x-www-form-urlencoded";
      break;
    case W6X_HTTP_CONTENT_TYPE_JSON:
      content_type = "application/json";
      break;
    case W6X_HTTP_CONTENT_TYPE_XML:
      content_type = "application/xml";
      break;
    case W6X_HTTP_CONTENT_TYPE_OCTET_STREAM:
      content_type = "application/octet-stream";
      break;
    default:
      content_type = "text/plain";
  }

  if (strncmp(Obj->method, "HEAD", 4) == 0)
  {
    method = W6X_HTTP_REQ_TYPE_HEAD;
    /* Get the length of the HTTP request */
    req_len = strlen(HTTPC_REQ_HEAD_11) + strlen(Obj->uri) + strlen(W6X_HTTP_CLIENT_AGENT) + strlen(host_name);
    /* Allocate dynamically the HTTP request based on previous result */
    req_buffer = pvPortMalloc(req_len);
    if (req_buffer == NULL)
    {
      goto _err;
    }
    /* Prepare send request */
    req_len2 = snprintf((char *)req_buffer, req_len, HTTPC_REQ_HEAD_11_FORMAT(Obj->uri, host_name));
  }
  else if (strncmp(Obj->method, "GET", 3) == 0)
  {
    method = W6X_HTTP_REQ_TYPE_GET;
    /* Get the length of the HTTP request */
    req_len = strlen(HTTPC_REQ_11_HOST) + strlen(Obj->uri) + strlen(W6X_HTTP_CLIENT_AGENT) + strlen(host_name);
    /* Allocate dynamically the HTTP request based on previous result */
    req_buffer = pvPortMalloc(req_len);
    if (req_buffer == NULL)
    {
      goto _err;
    }
    /* Prepare send request */
    req_len2 = snprintf((char *)req_buffer, req_len, HTTPC_REQ_11_HOST_FORMAT(Obj->uri, host_name));
  }
  else if (strncmp(Obj->method, "PUT", 3) == 0)
  {
    method = W6X_HTTP_REQ_TYPE_PUT;
    /* Get the length of the HTTP request */
    req_len = strlen(HTTPC_REQ_POST_PUT_11) + strlen(Obj->uri) + strlen(W6X_HTTP_CLIENT_AGENT)
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
  }
  else if (strncmp(Obj->method, "POST", 4) == 0)
  {
    method = W6X_HTTP_REQ_TYPE_POST;
    /* Get the length of the HTTP request */
    req_len = strlen(HTTPC_REQ_POST_PUT_11) + strlen(Obj->uri) + strlen(W6X_HTTP_CLIENT_AGENT)
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
  }
  else
  {
    LogError("Unknown Request type\n");
    goto _err;
  }

  if ((req_len2 <= 0) || (req_len2 > req_len))
  {
    vPortFree(req_buffer);
    goto _err;
  }

  /* Send the HTTP request to the HTTP server via the TCP socket */
  count_done = W6X_Net_Send(Obj->sock, req_buffer, req_len2, 0);
  if (count_done < 0)
  {
    LogError("Failed to send data to tcp server (%" PRIi32 "), try again\n", count_done);
    vPortFree(req_buffer);
    goto _err;
  }

  LogDebug("HTTP request send data success (%" PRIi32 ")\n", count_done);
  vPortFree(req_buffer);
  vTaskDelay(100);
  buff_head = pvPortMalloc(sizeof(W6X_HTTP_buffer_t));
  if (buff_head == NULL)
  {
    goto _err;
  }

  memset(buff_head, 0x00, sizeof(W6X_HTTP_buffer_t));
  /* Wait to receive an answer to the HTTP request we sent */
  if (W6X_STATUS_OK != W6X_HTTP_Client_Wait_For_Header(Obj->sock, buff_head, &offset))
  {
    LogError("Get HTTP header failed\n");
    vPortFree(buff_head);
    goto _err;
  }

  /* Parse the HTTP response status */
  if (W6X_HTTP_Parse_Response_Status(&buff_head->data[0], &http_version, &http_status) != W6X_STATUS_OK)
  {
    LogError("Parse of HTTP response status failed\n");
    vPortFree(buff_head);
    goto _err;
  }

  /* Get the returned status category and check if it is a success, we only support 200 OK currently */
  (void)W6X_HTTP_Get_Status_Category(http_status, &http_category);

  /* Parse the content length field of the HTTP response to know how much body content to expect,
     if we don't expect a body we set the content length value to 0 */
  if (method == W6X_HTTP_REQ_TYPE_GET)
  {
    if (W6X_HTTP_Parse_Content_Length(&buff_head->data[0], &content_length) != W6X_STATUS_OK)
    {
      LogDebug("Content Length tag not found in header\n");
    }
  }

  if (Obj->settings.headers_done_fn)
  {
    (Obj->settings.headers_done_fn)(NULL, Obj->settings.callback_arg, &buff_head->data[0],
                                    offset, content_length);
  }

  if (Obj->settings.result_fn)
  {
    (Obj->settings.result_fn)(Obj->settings.callback_arg, http_status, content_length, 0, 0);
  }

  /* If an error occurred, No need to read data, but status code must be returned */
  if (http_category >= HTTP_CATEGORY_CLIENT_ERROR)
  {
    LogError("W6X_HTTP_Client_Get_Status_Category failed\n");
    vPortFree(buff_head);
    goto _err;
  }

  if (offset > 0)
  {
    buff_head->length -= offset;
    for (int32_t i = 0; i < buff_head->length; i++)
    {
      buff_head->data[i] = buff_head->data[i + offset];
    }
    buff_head->data[buff_head->length] = 0;
  }

  while (buff_head->length > 0)
  {
    total_recv_data += buff_head->length;
    /* Pass Data to callback if not NULL */
    if ((Obj->settings.recv_fn) && ((Obj->settings.recv_fn)(Obj->settings.recv_fn_arg, buff_head, 0) < 0))
    {
      LogError("User function for received data processing returned an error");
      break;
    }
    if ((content_length > 0) && (total_recv_data >= content_length))
    {
      break;
    }
    memset(buff_head, 0, sizeof(W6X_HTTP_buffer_t));
    buff_head->length = W6X_Net_Recv(Obj->sock, &buff_head->data[0], W6X_HTTP_HEAD_MAX_RESP_BUFFER_SIZE, 0);
  }

  vPortFree(buff_head);
  error = 0;

_err:
  if (error != 0)
  {
    if (Obj->settings.result_fn)
    {
      (Obj->settings.result_fn)(Obj->settings.callback_arg, http_status, 0, 0, -1);
    }
    if ((Obj->settings.recv_fn))
    {
      (Obj->settings.recv_fn)(Obj->settings.callback_arg, NULL, -1);
    }
  }
  W6X_Net_Tls_Credential_Delete(Obj->sock, W6X_NET_TLS_CREDENTIAL_CA_CERTIFICATE);
  W6X_Net_Close(Obj->sock);
  vPortFree(Obj->post_data.data);
  vPortFree(Obj->uri);
  vPortFree(Obj);
  vTaskDelete(NULL);
}

W6X_Status_t W6X_HTTP_Parse_Content_Length(const uint8_t *buffer, uint32_t *content_length)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint8_t *header_end;
  uint8_t *content_len_hdr;
  uint8_t *content_len_line_end;
  int32_t len = 0;
  uint8_t content_len_num[16];
  size_t content_len_num_len;
  NULL_ASSERT(buffer, W6X_Obj_Null_str);

  header_end = (uint8_t *)strstr((char *)buffer, "\r\n\r\n");
  if (header_end == NULL)
  {
    goto _err;
  }

  /* Look for the Content-Length header field */
  content_len_hdr = (uint8_t *)strstr((const char *)buffer, "Content-Length: ");
  if (content_len_hdr == NULL)
  {
    goto _err;
  }

  /* Parse content len value */
  content_len_line_end = (uint8_t *)strstr((const char *)content_len_hdr, "\r\n");;
  if (content_len_line_end == NULL)
  {
    goto _err;
  }

  content_len_num_len = (size_t)(content_len_line_end - content_len_hdr - 16);
  memset(content_len_num, 0, sizeof(content_len_num));

  if (strncpy((char *)content_len_num, (char *)(content_len_hdr + 16), content_len_num_len) != NULL)
  {
    Parser_StrToInt((char *)content_len_num, NULL, &len);
    if (len >= 0)
    {
      *content_length = (uint32_t)len;
      ret = W6X_STATUS_OK;
    }
  }

_err:
  return ret;
}

W6X_Status_t W6X_HTTP_Parse_Response_Status(const uint8_t *buffer, uint16_t *http_version,
                                            W6X_HTTP_Status_Code_e *http_status)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint8_t *header_end;
  uint8_t *space1;
  NULL_ASSERT(buffer, W6X_Obj_Null_str);

  /* Look for the end of the header */
  header_end = (uint8_t *)strstr((char *)buffer, "\r\n\r\n");
  if (header_end == NULL)
  {
    goto _err;
  }

  header_end += 4; /* Move past the "\r\n\r\n" */
  space1 = (uint8_t *)strstr((char *)buffer, " ");
  if (space1 == NULL)
  {
    goto _err;
  }

  if ((strncmp((char *)buffer, "HTTP/", 5) == 0) && (buffer[6] == '.'))
  {
    uint8_t status_num[10];
    size_t status_num_len;
    /* parse http version */
    uint16_t version = buffer[5] - '0';
    version <<= 8;
    version |= buffer[7] - '0';
    int32_t status = 0;
    *http_version = version;
    /* parse HTTP status number */
    const uint8_t *space2 = (uint8_t *) strstr((char *)space1 + 1, " ");

    if (space2 != NULL)
    {
      status_num_len = space2 - space1 - 1;
    }
    else
    {
      status_num_len = header_end - space1 - 1;
    }

    memset(status_num, 0, sizeof(status_num));
    strncpy((char *)status_num, (char *)(space1 + 1), status_num_len);
    Parser_StrToInt((char *)status_num, NULL, &status);

    if ((status > 0) && (status <= 0xFFFF))
    {
      *http_status = (W6X_HTTP_Status_Code_e)status;
      ret = W6X_STATUS_OK;
    }
  }

_err:
  return ret;
}

W6X_Status_t W6X_HTTP_Get_Status_Category(W6X_HTTP_Status_Code_e status, W6X_HTTP_Status_Category_e *category)
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
  return W6X_STATUS_OK;
}

W6X_Status_t W6X_HTTP_Client_Wait_For_Header(int32_t sock, W6X_HTTP_buffer_t *buffer, uint32_t *body_start_offset)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint8_t header_received = 0;
  int32_t data_received = 0;
  uint32_t total_data_received = 0;
  uint8_t *header_end;
  NULL_ASSERT(buffer, W6X_Obj_Null_str);
  /* Read the headers */
  do
  {
    data_received = W6X_Net_Recv(sock, &buffer->data[total_data_received],
                                 W6X_HTTP_HEAD_MAX_RESP_BUFFER_SIZE - total_data_received, 0);
    if (data_received < 0)
    {
      LogError("Receive failed with %" PRIi32 "\n", data_received);
      goto _err;
    }

    total_data_received += data_received;

    /* Look for the end of the headers */
    header_end = (uint8_t *) strstr((char *)buffer, "\r\n\r\n");
    if (header_end != NULL)
    {
      header_received = 1;
      header_end += 4U; /** + 4U to go past the "\r\n\r\n" termination */

      /** Copy body content to the provided buffer */
      *body_start_offset = (uint32_t)(header_end - &buffer->data[0]);
      break;
    }
  } while (!header_received && (total_data_received < W6X_HTTP_HEAD_MAX_RESP_BUFFER_SIZE));
  buffer->length = total_data_received;
  if (!header_received)
  {
    LogError("Failed to receive HTTP headers\n");
    goto _err;
  }

  ret = W6X_STATUS_OK;

_err:
  return ret;
}

/** @} */
