/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    httpserver.c
  * @author  ST67 Application Team
  * @brief   Http server application.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include "httpserver.h"
#include "main.h"

#include "w6x_api.h"
#include "common_parser.h" /* Common Parser functions */

/* Please read the following Wiki on how to create the html_pages.h binary:
https://wiki.st.com/stm32mcu/wiki/Connectivity:Wi-Fi_ST67W6X_HTTP_Server_Application */
#include "html_pages.h"

#include "FreeRTOS.h"
#include "event_groups.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief  HTTP server response type
  */
typedef enum
{
  FAVICON_SVG,
  ST_LOGO_SVG,
  INDEX_HTML,
  LED_GREEN_STATE,
  LED_RED_STATE,
  BUTTON_STATE,
  ERROR_404_HTML,
  UNKNOWN_RESPONSE
} HttpServer_response_e;

/**
  * @brief  Structure to handle GPIO pins for HTTP server application
  */
typedef struct
{
  GPIO_PinState btn_state;        /*!< user button state */
  GPIO_PinState led_green_state;  /*!< green led state */
  GPIO_PinState led_red_state;    /*!< red led state */
} PinInfos_t;

/**
  * @brief  HTTP server response structure
  */
typedef struct
{
  HttpServer_response_e response_type;  /*!< Type of response */
  const char *request;                  /*!< Request string */
  const char *response;                 /*!< Response string */
  size_t resp_len;                      /*!< Response string length */
} HttpServer_response_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/** Priority of the user pins polling task */
#define PIN_POLLING_THREAD_PRIO        30

/** Stack size of the user pins polling task */
#define PIN_POLLING_TASK_STACK_SIZE    512U

/** Priority of the web server child task */
#define WEBSERVER_CHILD_THREAD_PRIO    29

/** Stack size of the web server child task */
#define HTTP_CHILD_TASK_STACK_SIZE     2048U

/** HTTP server port */
#define HTTP_PORT                      80

/** Socket timeout in ms */
#define SOCKET_TIMEOUT_MS              1000

/** Event flag for pin status update */
#define EVENT_FLAG_PIN                 (1UL << 1U)

/** Timeout for the pin status update in ms */
#define PIN_TIMEOUT_MS                 9000

/** Buffer size for the child task */
#define HTTP_CHILD_TASK_BUFFER_SIZE    1024U

/** Maximum bytes to send in one step */
#define MAX_BYTES_TO_SEND              4096U

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Button and Leds states */
static volatile PinInfos_t pins_info =
{
  .btn_state = GPIO_PIN_RESET,
  .led_green_state = GPIO_PIN_RESET,
  .led_red_state = GPIO_PIN_RESET
};

/** Event group handle for pin status update */
EventGroupHandle_t pin_handle;

/** Flag to indicate if the button state has changed */
extern uint8_t button_changed;

/** Response with OK content */
static const char response_ok_html[] =
{
  "HTTP/1.1 200 OK\r\n"
  "Server: U5\r\n"
  "Access-Control-Allow-Origin: * \r\n"
  "Cache-Control: no-cache\r\n"
  "Keep-Alive: timeout=2, max=2\r\n"
  "Connection: close\r\n"
  "Content-Type: text/html; charset=utf-8\r\n"
  "Content-Length: 2\r\n\r\n"
  "OK"
};

/** Response content depending on the request */
static const HttpServer_response_t http_server_responses[] =
{
  {INDEX_HTML,      "GET / ",                                     response_index_html,  sizeof(response_index_html)},
  {FAVICON_SVG,     "GET /favicon.ico",                           response_favicon_svg, sizeof(response_favicon_svg)},
  {ST_LOGO_SVG,     "GET /ST_logo_2020_white_no_tagline_rgb.svg", response_st_logo_svg, sizeof(response_st_logo_svg)},
  {LED_GREEN_STATE, "GET /LedGreen",                              response_ok_html,     sizeof(response_ok_html)},
  {LED_RED_STATE,   "GET /LedRed",                                response_ok_html,     sizeof(response_ok_html)},
  {BUTTON_STATE,    "GET /pins_status",                           NULL,                 0U},
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Web server child task
  * @param  arg: pointer on argument(not used here)
  */
static void http_server_serve_task(void *arg);

/**
  * @brief  Pins polling task
  * @param  arg: pointer on argument(not used here)
  */
static void pin_verification_task(void *arg);

/**
  * @brief  Write HTML data to client
  * @param  client: client information
  * @param  buffer: HTML data to send
  * @param  buffer_size: size of the data to send
  * @retval 0 if success, 1 otherwise
  */
static int32_t http_server_write(int32_t client, const char *buffer, size_t buffer_size);

/**
  * @brief  Close client connection
  * @param  client: client information
  * @retval 0
  */
static int32_t close_client(int32_t client);

/**
  * @brief  Process the HTTP response
  * @param  client: client information
  * @param  recv_buffer: received request data
  */
static void http_process_response(int32_t client, char *recv_buffer);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void http_server_socket(void *arg)
{
  (void)arg;
  const int32_t domain = AF_INET;
  int32_t sock = -1;
  struct sockaddr_in s_addr_in_t = { 0 };
  int32_t fct_start = 9;
  uint16_t port = HTTP_PORT;
  uint8_t ip_address[4] = {0};
  uint8_t netmask_addr[4] = {0};
  int32_t timeout = (int32_t)pdMS_TO_TICKS(SOCKET_TIMEOUT_MS);

  /* USER CODE BEGIN http_server_socket_1 */

  /* USER CODE END http_server_socket_1 */

  /* Get the soft-AP current IP address */
  if (W6X_Net_AP_GetIPAddress(ip_address, netmask_addr) != W6X_STATUS_OK)
  {
    LogError("Get soft-AP IP failed\n");
    goto _err;
  }

  LogInfo("Soft-AP IP address : " IPSTR "\n", IP2STR(ip_address));
  s_addr_in_t.sin_addr.s_addr = ATON(ip_address);
  s_addr_in_t.sin_port = PP_HTONS(port);
  s_addr_in_t.sin_family = AF_INET;

  /* Create a new TCP server socket with port HTTP_PORT */
  sock = W6X_Net_Socket(domain, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
  {
    LogInfo("Could not create the socket.\n");
    goto _err;
  }

  /* Set the socket Receive timeout option */
  if (W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout)) != 0)
  {
    LogError("Could not set the socket options.\n");
    goto _err;
  }

  /* Set the socket Send timeout option */
  if (W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout)) != 0)
  {
    LogError("Could not set the socket options.\n");
    goto _err;
  }

  /* Set the socket Send timeout option */
  if (W6X_Net_Setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&timeout, sizeof(timeout)) != 0)
  {
    LogError("Could not set the socket options.\n");
    goto _err;
  }

  /* Bind the socket to the server address */
  if (W6X_Net_Bind(sock, (struct sockaddr *)&s_addr_in_t, sizeof(s_addr_in_t)) != 0)
  {
    LogInfo("\n LwIP Bind Fail\n");
    goto _err;
  }

  LogInfo("\n LwIP Bind Pass\n");

  /* Listen for incoming connections (TCP listen backlog = 5). */
  if (W6X_Net_Listen(sock, 5) != 0)
  {
    LogInfo("\n LwIP Listen Fail\n");
    goto _err;
  }

  LogInfo("\n LwIP Listen Pass\n");

  /* USER CODE BEGIN http_server_socket_2 */

  /* USER CODE END http_server_socket_2 */

  /* Creation of a thread to check if the pin value of the button or the LEDs has change */
  pin_handle = xEventGroupCreate();
  if (pdPASS != xTaskCreate((TaskFunction_t)pin_verification_task, "UserPinsPolling",
                            PIN_POLLING_TASK_STACK_SIZE >> 2U,
                            &fct_start, PIN_POLLING_THREAD_PRIO, NULL))
  {
    LogInfo("User pins task creation failed\n");
    goto _err;
  }

  while (true)
  {
    struct sockaddr remotehost_t;
    uint32_t remotehost_size = sizeof(remotehost_t);

    /* Wait for an incoming client connection */
    int32_t newconn = W6X_Net_Accept(sock, (struct sockaddr *)&remotehost_t, (socklen_t *)&remotehost_size);
    if (newconn < 0)
    {
      vTaskDelay(pdMS_TO_TICKS(200));
      LogInfo("\n Failed to accept new client requests.\n");
    }
    else
    {
      /* Create a temporary thread to process the incoming HTTP request */
      char thread_name[14];
      const size_t thread_name_len = sizeof(thread_name);
      (void)snprintf(thread_name, thread_name_len, "HTTP_%08" PRIX32, newconn);
      thread_name[thread_name_len - 1U] = '\0';

      LogDebug("\n Creation of temporary thread to process an incoming HTTP request : %" PRIi32 "\n", newconn);
      if (pdPASS != xTaskCreate((TaskFunction_t)http_server_serve_task, thread_name,
                                HTTP_CHILD_TASK_STACK_SIZE >> 2U,
                                &newconn, WEBSERVER_CHILD_THREAD_PRIO, NULL))
      {
        LogInfo("%s task creation failed\n", thread_name);
      }
      /* Delay added to avoid that too many requests are processed in parallel */
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

  /* USER CODE BEGIN http_server_socket_last */

  /* USER CODE END http_server_socket_last */

_err:
  /* Error case */
  if (sock >= 0)
  {
    if (W6X_Net_Shutdown(sock, 1) != 0)
    {
      LogError("Failed to close server socket\n");
    }
  }
  return;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static void http_server_serve_task(void *arg)
{
  int32_t client = *((int32_t *)arg);
  int32_t bytes_received;
  int32_t recv_total_len = 0;
  bool request_complete = false;
  /* Allocate considering the biggest buffer the client can send */
  ssize_t recv_buffer_len = HTTP_CHILD_TASK_BUFFER_SIZE;
  char *recv_buffer = (char *)pvPortMalloc(recv_buffer_len + 1);
  if (recv_buffer == NULL)
  {
    LogError("Unable to allocate recv buffer\n");
    goto _err;
  }

  /* USER CODE BEGIN http_server_serve_task_1 */

  /* USER CODE END http_server_serve_task_1 */

  /* Read in the request */
  recv_buffer[0] = '\0';
  do
  {
    bytes_received = W6X_Net_Recv(client, (uint8_t *)&recv_buffer[recv_total_len], recv_buffer_len, 0);
    if (bytes_received <= 0) /* No data received or error */
    {
      break;
    }

    recv_total_len += bytes_received;

    /* Verify if we have receive the whole request or if we need to do another receive */
    if (strncmp(&recv_buffer[recv_total_len - 4], "\r\n\r\n", 4) == 0)
    {
      /* The full request has been received leaving the reading loop */
      request_complete = true;
      break;
    }

    recv_buffer_len -= bytes_received;
  } while (recv_buffer_len > 0);

  if (!request_complete)
  {
    if (recv_total_len == 0)
    {
      LogError("No data read on the socket, closing the socket\n");
    }
    else
    {
      LogError("Data received does not contain the whole request, receive buffer is too small, closing the socket\n");
    }
    goto _close;
  }

  LogDebug("\n %" PRIi32 " >>> %" PRIi32 " <<<\n", client, recv_total_len);

  /* Count can be negative. */
  recv_buffer[recv_total_len] = '\0';
  LogDebug("\n %" PRIi32 " >>> %s <<<\n", client, recv_buffer);

  /* USER CODE BEGIN http_server_serve_task_2 */

  /* USER CODE END http_server_serve_task_2 */

  /* Process the response */
  http_process_response(client, recv_buffer);

  /* USER CODE BEGIN http_server_serve_task_last */

  /* USER CODE END http_server_serve_task_last */

_close:
  vPortFree(recv_buffer);
_err:
  (void)close_client(client);
  vTaskDelete(NULL);
}

static void pin_verification_task(void *arg)
{
  UNUSED(arg);
  GPIO_PinState Btn;
  GPIO_PinState LedGreen;
  GPIO_PinState LedRed;

  /* USER CODE BEGIN pin_verification_task_1 */

  /* USER CODE END pin_verification_task_1 */

  while (true)
  {
    while (true) /* Wait for the button or the LEDs to change */
    {
      vTaskDelay(pdMS_TO_TICKS(50));
      Btn = (GPIO_PinState)button_changed;
      LedGreen = HAL_GPIO_ReadPin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
      LedRed = HAL_GPIO_ReadPin(LED_RED_GPIO_Port, LED_RED_Pin);

      if (Btn != pins_info.btn_state)
      {
        break;
      }
      if (LedGreen != pins_info.led_green_state)
      {
        break;
      }
      if (LedRed != pins_info.led_red_state)
      {
        break;
      }
    }

    /* Update the button and the LEDs states */
    pins_info.btn_state = Btn;
    pins_info.led_green_state = LedGreen;
    pins_info.led_red_state = LedRed;

    /* Notify the HTTP server task */
    (void)xEventGroupSetBits(pin_handle, EVENT_FLAG_PIN);
  }
}

static int32_t http_server_write(int32_t client, const char *buffer, size_t buffer_size)
{
  int32_t bytes_sent = 0;

  /* USER CODE BEGIN http_server_write_1 */

  /* USER CODE END http_server_write_1 */

  LogDebug("[%" PRIi32 "] *****> %s<*****\n", client, buffer);

  do /* Send the data. Can be done in multiple steps. */
  {
    bytes_sent = W6X_Net_Send(client, (void *)buffer, buffer_size, 0);
    if (bytes_sent < 0)
    {
      LogError("[%" PRIi32 "] *****> SEND ERROR <*****\n", client);
      return 1;
    }
    buffer_size -= bytes_sent;
    buffer += bytes_sent;
  } while (buffer_size > 0);

  /* USER CODE BEGIN http_server_write_last */

  /* USER CODE END http_server_write_last */

  return 0;
}

static int32_t close_client(int32_t client)
{
  /* USER CODE BEGIN close_client_1 */

  /* USER CODE END close_client_1 */

  /* Close the connection */
  (void)W6X_Net_Close(client);
  LogDebug("!!! Closed connection <%" PRIi32 "> !!!\n", client);

  /* USER CODE BEGIN close_client_last */

  /* USER CODE END close_client_last */

  return 0;
}

static void http_process_response(int32_t client, char *recv_buffer)
{
  HttpServer_response_e response = UNKNOWN_RESPONSE;
  const char *response_data = response_error_404_html; /* Request not recognized, return 404 error */
  size_t resp_len = sizeof(response_error_404_html);
  char response_template[250] =
  {
    "HTTP/1.1 200 OK\r\n"
    "Server: U5\r\n"
    "Access-Control-Allow-Origin: * \r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: close\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
  };

  /* USER CODE BEGIN http_process_response_1 */

  /* USER CODE END http_process_response_1 */

  /* Check the request to determine the response */
  for (uint32_t i = 0; i < (sizeof(http_server_responses) / sizeof(http_server_responses[0])); i++)
  {
    if (strncmp(recv_buffer, http_server_responses[i].request, strlen(http_server_responses[i].request)) == 0)
    {
      response = http_server_responses[i].response_type;
      response_data = (char *)http_server_responses[i].response;
      resp_len = http_server_responses[i].resp_len;
      break;
    }
  }

  /* USER CODE BEGIN http_process_response_2 */

  /* USER CODE END http_process_response_2 */

  if (response == BUTTON_STATE) /* Prepare a custom response for the button state */
  {
    LogInfo("\nPending request for the PIN STATUS received\n\n");
    /* Wait for the pin status update */
    (void)xEventGroupWaitBits(pin_handle, EVENT_FLAG_PIN, pdTRUE, pdFALSE, pdMS_TO_TICKS(PIN_TIMEOUT_MS));
    /* Send anyway the pin status update or the last known status if timeouted */
    /* Prepare the HTTP content data */
    char data[60];
    uint32_t green = (pins_info.led_green_state == GPIO_PIN_RESET) ? 0U : 1U;
    uint32_t red = (pins_info.led_red_state == GPIO_PIN_RESET) ? 0U : 1U;
    uint32_t button = (pins_info.btn_state == GPIO_PIN_RESET) ? 0U : 1U;
    (void)snprintf(data, sizeof(data),
                   "{\"LedGreenPin\":%" PRIu32 ",\"LedRedPin\":%" PRIu32 ",\"BtnPin\":%" PRIu32 "}",
                   green, red, button);

    resp_len = strlen(response_template);
    /* Append the content length and the data to the response */
    resp_len += snprintf(&response_template[strlen(response_template)],
                         sizeof(response_template) - strlen(response_template),
                         "Content-Length: %" PRIu32 "\r\n\r\n%s", (uint32_t)strlen(data), data);

    LogInfo("Pins status :\n%s\n", data);
    response_data = response_template;
  }
  else
  {
    /* Unknown request */
  }

  /* Send the response */
  if (1 == http_server_write(client, response_data, resp_len))
  {
    return;
  }

  /* Finalize the response */
  if (response == LED_RED_STATE) /* Toggle the red LED */
  {
    HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
  }
  else if (response == LED_GREEN_STATE) /* Toggle the green LED */
  {
    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
  }
  else
  {
    /* Nothing to do */
  }

  /* USER CODE BEGIN http_process_response_last */

  /* USER CODE END http_process_response_last */
}

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
