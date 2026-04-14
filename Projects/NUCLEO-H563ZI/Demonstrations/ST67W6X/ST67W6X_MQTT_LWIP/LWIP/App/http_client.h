/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    http_client.h
  * @author  ST67 Application Team
  * @brief   Simple HTTP client API
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
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>

#include "lwip/sockets.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported constants --------------------------------------------------------*/
/** Return code: Success */
#define HTTP_CLIENT_SUCCESS              (0)

/** Return code: Generic error */
#define HTTP_CLIENT_ERR                  (-1)

/** Return code: Bad parameters */
#define HTTP_CLIENT_BAD_PARAM            (-2)

#ifndef HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT
/** Default HTTP client TCP socket receive timeout (ms) */
#define HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT       5000
#endif /* HTTP_CLIENT_TCP_SOCK_RECV_TIMEOUT */

#ifndef HTTP_CLIENT_TCP_SOCKET_SIZE
/** Default HTTP client TCP socket receive buffer size (bytes) */
#define HTTP_CLIENT_TCP_SOCKET_SIZE             0x3000
#endif /* HTTP_CLIENT_TCP_SOCKET_SIZE */

/* Different HTTP request type */
#define HTTP_REQ_TYPE_HEAD               1  /*!< HTTP HEAD request */
#define HTTP_REQ_TYPE_GET                2  /*!< HTTP GET request */
#define HTTP_REQ_TYPE_POST               3  /*!< HTTP POST request */
#define HTTP_REQ_TYPE_PUT                4  /*!< HTTP PUT request */

/** Maximum size of the server name (Host header/SNI) */
#define HTTP_SNI_MAX_SIZE   64U

/* Exported types ------------------------------------------------------------*/
/** Define generic categories for HTTP response codes */
typedef enum
{
  CONTINUE = 100,
  SWITCHING_PROTOCOLS = 101,
  OK = 200,
  CREATED = 201,
  ACCEPTED = 202,
  NON_AUTHORITATIVE_INFORMATION = 203,
  NO_CONTENT = 204,
  RESET_CONTENT = 205,
  PARTIAL_CONTENT = 206,
  MULTIPLE_CHOICES = 300,
  MOVED_PERMANENTLY = 301,
  FOUND = 302,
  SEE_OTHER = 303,
  NOT_MODIFIED = 304,
  USE_PROXY = 305,
  TEMPORARY_REDIRECT = 307,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  PAYMENT_REQUIRED = 402,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  NOT_ACCEPTABLE = 406,
  PROXY_AUTHENTICATION_REQUIRED = 407,
  REQUEST_TIMEOUT = 408,
  CONFLICT = 409,
  GONE = 410,
  LENGTH_REQUIRED = 411,
  PRECONDITION_FAILED = 412,
  REQUEST_ENTITY_TOO_LARGE = 413,
  REQUEST_URI_TOO_LARGE = 414,
  UNSUPPORTED_MEDIA_TYPE = 415,
  REQUESTED_RANGE_NOT_SATISFIABLE = 416,
  EXPECTATION_FAILED = 417,
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  BAD_GATEWAY = 502,
  SERVICE_UNAVAILABLE = 503,
  GATEWAY_TIMEOUT = 504,
  HTTP_VERSION_NOT_SUPPORTED = 505
} HTTP_Status_Code_e;

/** Generic categories for HTTP response codes */
typedef enum
{
  HTTP_CATEGORY_UNKNOWN = 0,
  HTTP_CATEGORY_INFORMATIONAL = 1,
  HTTP_CATEGORY_SUCCESS = 2,
  HTTP_CATEGORY_REDIRECTION = 3,
  HTTP_CATEGORY_CLIENT_ERROR = 4,
  HTTP_CATEGORY_SERVER_ERROR = 5
} HTTP_Status_Category_e;

/** POST/PUT content types */
typedef enum
{
  HTTP_CONTENT_TYPE_PLAIN_TEXT = 0,
  HTTP_CONTENT_TYPE_URL_ENCODED,
  HTTP_CONTENT_TYPE_JSON,
  HTTP_CONTENT_TYPE_XML,
  HTTP_CONTENT_TYPE_OCTET_STREAM
} HTTP_Content_Type_e;

/** HTTP Data buffer structure */
typedef struct
{
  uint8_t *data;              /*!< Data buffer */
  int32_t length;             /*!< Data length */
} HTTP_buffer_t;

/** HTTP connection state */
typedef int32_t HTTP_state_t;

/** Certificate structure */
typedef struct
{
  char *name;                                 /*!< Certificate name */
  char *content;                              /*!< Certificate content */
} Certificate_t;

/** HTTP result callback */
typedef void (*HTTP_result_cb_t)(void *arg, HTTP_Status_Code_e httpc_result, uint32_t rx_content_len,
                                 uint32_t srv_res, int32_t err);

/** HTTP headers done callback */
typedef int32_t (*HTTP_headers_done_cb_t)(HTTP_state_t *connection, void *arg, uint8_t *hdr, uint16_t hdr_len,
                                          uint32_t content_len);

/** HTTP data callback */
typedef int32_t (*HTTP_data_cb_t)(void *arg, HTTP_buffer_t *p, int32_t err);

/** HTTP POST/PUT data structure */
typedef struct
{
  HTTP_Content_Type_e type;   /*!< Content type */
  char *data;                /*!< Pointer to data buffer */
} HTTP_Post_Data_t;

/** HTTP connection structure */
typedef struct
{
  uint8_t use_proxy;                         /*!< Use proxy */
  uint16_t proxy_port;                       /*!< Proxy port */
  const ip_addr_t *proxy_addr;               /*!< Proxy address */
  Certificate_t https_certificate;           /*!< HTTPS certificate */
  char *server_name;                         /*!< Server name (Host/SNI) */
  HTTP_headers_done_cb_t headers_done_fn;    /*!< Headers done callback */
  HTTP_result_cb_t result_fn;                /*!< Result callback */
  void *callback_arg;                        /*!< Callback argument */
  HTTP_data_cb_t recv_fn;                    /*!< Receive callback */
  void *recv_fn_arg;                         /*!< Receive callback argument */
  uint32_t timeout;                          /*!< Timeout */
  uint32_t max_response_len;                 /*!< Maximum response length */
} HTTP_connection_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  HTTP Client request based on BSD socket
  * @param  server_addr: Server address
  * @param  port: Server port (443 for HTTPS)
  * @param  uri: URI to request (e.g. "/path/file.bin")
  * @param  req_type: HTTP request type (HTTP_REQ_TYPE_GET/HEAD/POST/PUT)
  * @param  post_data: POST/PUT payload descriptor (NULL if none)
  * @param  post_data_len: Length of the data to post/put (0 if none)
  * @param  result_fn: Callback called when the request completes (optional)
  * @param  callback_arg: Argument passed to result_fn (optional)
  * @param  headers_done_fn: Callback called once headers are received (optional)
  * @param  data_fn: Callback called when body data is received (optional)
  * @param  settings: HTTP connection settings (optional)
  * @return HTTP_CLIENT_SUCCESS if request started, negative error code otherwise
  */
int32_t HTTP_Client_Request(const ip_addr_t *server_addr, uint16_t port,
                            const char *uri, uint8_t req_type,
                            const HTTP_Post_Data_t *post_data, size_t post_data_len,
                            HTTP_result_cb_t result_fn, void *callback_arg,
                            HTTP_headers_done_cb_t headers_done_fn,
                            HTTP_data_cb_t data_fn,
                            const HTTP_connection_t *settings);

/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HTTP_CLIENT_H */
