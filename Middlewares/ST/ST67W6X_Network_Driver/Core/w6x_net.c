/**
  ******************************************************************************
  * @file    w6x_net.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x Net API
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */
#include "common_parser.h" /* Common Parser functions */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_Net_Types ST67W6X Net Types
  * @ingroup  ST67W6X_Private_Net
  * @{
  */

/**
  * @brief  Socket status
  */
typedef enum
{
  W6X_SOCKET_RESET = 0,             /*!< Socket is reset */
  W6X_SOCKET_ALLOCATED = 1,         /*!< Socket is initialized and ready to be used */
  W6X_SOCKET_BIND = 2,              /*!< Socket is associated with a local address */
  W6X_SOCKET_LISTENING = 3,         /*!< Socket is listening for incoming connections */
  W6X_SOCKET_CONNECTED = 4,         /*!< Socket is connected */
  W6X_SOCKET_CLOSING = 5            /*!< Socket is closing and cannot be used */
} W6X_Net_Socket_Status_t;

/**
  * @brief  Connection structure
  */
typedef struct
{
  uint32_t TotalBytesReceived;      /*!< Number of bytes received in entire connection lifecycle */
  uint32_t TotalBytesSent;          /*!< Number of bytes sent in entire connection lifecycle */
  int32_t SoLinger;                 /*!< BSD Socket option SO_LINGER */
  uint32_t SoSndTimeo;              /*!< BSD Socket option SO_SNDTIMEO */
  uint32_t RecvTimeout;             /*!< BSD Socket option SO_RCVTIMEO */
  uint32_t RecvBuffSize;            /*!< BSD Socket option SO_RCVBUF */
  uint32_t SoKeepAlive;             /*!< BSD Socket option SO_KEEPALIVE */
  uint16_t RemotePort;              /*!< Remote PORT number */
  uint16_t LocalPort;               /*!< Local PORT number */
  uint8_t IsConnected;              /*!< Set to 1 if socket is a connection and not a server */
  uint8_t Client;                   /*!< Set to 1 if connection was made as client */
  uint8_t Number;                   /*!< Connection number */
  uint8_t RemoteIP[4];              /*!< IP address of device */
  uint8_t TcpNoDelay;               /*!< BSD Socket option TCP_NODELAY */
  char *Ca_Cert;                    /*!< CA certificate */
  char *Private_Key;                /*!< Private key */
  char *Certificate;                /*!< Server Certificate */
  char *PSK;                        /*!< Pre-shared key */
  char *PSK_Identity;               /*!< PSK identity */
  char ALPN[3][17];                 /*!< ALPN list */
  char SNI[17];                     /*!< Server Name Indication */
  W6X_Net_Socket_Status_t Status;   /*!< Socket status */
  W6X_Net_Protocol_e Protocol;      /*!< Connection type. Parameter is valid only if connection is made as client */
} W6X_Net_Socket_t;

/**
  * @brief  Internal structure to store credentials to use for SSL connections
  */
typedef struct
{
  uint8_t is_valid;                 /*!< Flag to indicate if the credential is valid */
  W6X_Net_Tls_Credential_e type;    /*!< Type of the credential */
  char *credential;                 /*!< Credential filename */
  uint32_t credential_len;          /*!< Credential filename length */
} W6X_Net_Credential_t;

/**
  * @brief  Connection structure
  */
typedef struct
{
  uint8_t SocketConnected;          /*!< Socket connected status */
  uint16_t RemotePort;              /*!< Remote PORT number */
  uint8_t RemoteIP[4];              /*!< IP address of device */
  SemaphoreHandle_t DataAvailable;  /*!< Semaphore for data available */
  uint32_t DataAvailableSize;       /*!< Counter for data available */
} W6X_Net_Connection_t;

/**
  * @brief  Internal Network context
  */
typedef struct
{
  W6X_Net_Socket_t Sockets[W61_NET_MAX_CONNECTIONS + 1];          /*!< Socket context */
  W6X_Net_Connection_t Connection[W61_NET_MAX_CONNECTIONS];       /*!< NCP connections context */
  W6X_Net_Credential_t Credentials[W61_NET_MAX_CONNECTIONS * 3];  /*!< Credentials context */
  int32_t NextSocketToUse;                                        /*!< Next socket to use */
} W6X_NetCtx_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @defgroup ST67W6X_Private_Net_Constants ST67W6X Net Constants
  * @ingroup  ST67W6X_Private_Net
  * @{
  */

#define TCP_PROTOCOL          6           /*!< TCP protocol number */

#define UDP_PROTOCOL          17          /*!< UDP protocol number */

#define SSL_PROTOCOL          282         /*!< SSL protocol number */

/** Fixed Timeout for pulling available data from NCP
  * Pull data might fail during heavy load if timeout is too small */
#define W6X_NET_PULL_DATA_TIMEOUT  100

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_Net_Variables ST67W6X Net Variables
  * @ingroup  ST67W6X_Private_Net
  * @{
  */

static W61_Object_t *p_DrvObj = NULL; /*!< Global W61 context pointer */

/** W6X Net init error string */
static const char W6X_Net_Uninit_str[] = "W6X Net module not initialized";

/** Net private context */
static W6X_NetCtx_t *p_net_ctx = NULL;

/** Net context pointer error string */
static const char W6X_Ctx_Null_str[] = "Net context not initialized";

/** Net buffer null string */
static const char W6X_Buf_Null_str[] = "Buffer is NULL";

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Private_Net_Functions ST67W6X Net Functions
  * @ingroup  ST67W6X_Private_Net
  * @{
  */

/**
  * @brief  Network callback function
  * @param  event_id: event ID
  * @param  event_args: event arguments
  */
static void W6X_Net_cb(W61_event_id_t event_id, void *event_args);

/**
  * @brief  Translate W61 status to Net status
  * @param  ret61: W61 status
  * @retval Net status
  */
static int32_t W6X_Net_TranslateErrorStatus(W61_Status_t ret61);

/**
  * @brief  Find a connection by remote IP and port
  * @param  remote_ip: remote IP address
  * @param  remoteport: remote port number
  * @retval Connection number
  */
static int32_t W6X_Net_Find_Connection(char *remote_ip, uint32_t remoteport);

/**
  * @brief  Poll the UDP sockets
  * @param  remote_addr: remote address
  * @retval Number of available sockets
  */
static int32_t W6X_Net_Poll_UDP_Sockets(struct sockaddr_in *remote_addr);

/**
  * @brief  Wait for data to be available on a connection
  * @param  sock: socket number
  * @param  connection_id: connection number
  * @param  buf: buffer to store the data
  * @param  max_len: maximum length of the buffer
  * @retval Number of bytes received
  */
static int32_t W6X_Net_Wait_Pull_Data(int32_t sock, int32_t connection_id, void *buf, size_t max_len);

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_Net_Public_Functions
  * @{
  */

W6X_Status_t W6X_Net_Init(void)
{
  W6X_App_Cb_t *p_cb_handler;
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint32_t ncp_buf_size_resp;

  /* Get the global W61 context pointer */
  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  /* Allocate the Net context */
  p_net_ctx = pvPortMalloc(sizeof(W6X_NetCtx_t));

  if (p_net_ctx == NULL)
  {
    LogError("Could not initialize Net context structure\n");
    goto _err;
  }
  memset(p_net_ctx, 0, sizeof(W6X_NetCtx_t));

  /* Check that application callback is registered */
  p_cb_handler = W6X_GetCbHandler();
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_net_cb == NULL))
  {
    LogError("Please register the APP callback before initializing the module\n");
    goto _err;
  }

  /* Register W61 driver callbacks */
  W61_RegisterULcb(p_DrvObj,
                   NULL,
                   NULL,
                   W6X_Net_cb,
                   NULL,
                   NULL);

  ret = TranslateErrorStatus(W61_Net_Init(p_DrvObj)); /* Initialize the default configuration */
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Initialize the connections */
  for (uint8_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
  {
    memset(&p_net_ctx->Connection[i], 0, sizeof(W6X_Net_Connection_t));
    /* Initialize a semaphore for each connection */
    p_net_ctx->Connection[i].DataAvailable = xSemaphoreCreateBinary();
    if (p_net_ctx->Connection[i].DataAvailable == NULL)
    {
      goto _err;
    }

    /* Set the buffer length available in NCP for each connection */
    ret = TranslateErrorStatus(W61_Net_SetReceiveBufferLen(p_DrvObj, i, W6X_NET_RECV_BUFFER_SIZE));
    if (ret != W6X_STATUS_OK)
    {
      if (ret == W6X_STATUS_ERROR)
      {
        LogError("Could not set Receive buffer size\n");
      }
      goto _err;
    }

    /* Verify the buffer length available in NCP for each connection */
    ret = TranslateErrorStatus(W61_Net_GetReceiveBufferLen(p_DrvObj, i, &ncp_buf_size_resp));
    if (ret != 0)
    {
      LogError("Could not set Receive buffer size\n");
      return ret;
    }
    else
    {
      if (W6X_NET_RECV_BUFFER_SIZE != ncp_buf_size_resp)
      {
        LogWarn("SO_RCVBUF was not set as expected, size id %" PRIu32 "\n", ncp_buf_size_resp);
      }
    }
  }

  for (uint32_t i = 0; i < (W61_NET_MAX_CONNECTIONS + 1); i++) /* Initialize the sockets */
  {
    memset(&p_net_ctx->Sockets[i], 0, sizeof(W6X_Net_Socket_t));
    p_net_ctx->Sockets[i].Number = W61_NET_MAX_CONNECTIONS + 1; /* Set the socket number to an invalid value */
    p_net_ctx->Sockets[i].SoSndTimeo = W6X_NET_SEND_TIMEOUT; /* Default value for SO_SNDTIMEO */
    p_net_ctx->Sockets[i].RecvTimeout = W6X_NET_RECV_TIMEOUT; /* Default value for SO_RCVTIMEO */
  }

  for (uint8_t i = 0; i < (W61_NET_MAX_CONNECTIONS * 3); i++)
  {
    /* Initialize the credentials */
    p_net_ctx->Credentials[i].is_valid = 0;
    p_net_ctx->Credentials[i].credential = NULL;
  }
  p_net_ctx->NextSocketToUse = 0; /* Initialize the rotary index for the socket usage */
_err:
  return ret;
}

void W6X_Net_DeInit(void)
{
  if (p_net_ctx == NULL)
  {
    return;
  }

  for (uint32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++) /* Delete all the socket semaphores */
  {
    vSemaphoreDelete(p_net_ctx->Connection[i].DataAvailable);
  }

  W61_Net_DeInit(p_DrvObj); /* Deinitialize the Net context */
  vPortFree(p_net_ctx); /* Free the Net context */
  p_DrvObj = NULL; /* Reset the global pointer */
  p_net_ctx = NULL;
}

W6X_Status_t W6X_Net_Ping(uint8_t *location, uint16_t length, uint16_t count, uint16_t interval_ms,
                          uint32_t *average_time, uint16_t *received_response)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Execute a ping process */
  return TranslateErrorStatus(W61_Net_Ping(p_DrvObj, (char *)location, length,
                                           count, interval_ms, average_time,
                                           received_response));
}

W6X_Status_t W6X_Net_GetHostAddress(const char *location, uint8_t *ipaddr)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Resolve the IP address from the host name in using the DNS Server */
  return TranslateErrorStatus(W61_Net_DNS_LookUp(p_DrvObj, location, ipaddr));
}

W6X_Status_t W6X_Net_GetSNTPConfiguration(uint8_t *Enable, int16_t *Timezone, uint8_t *SntpServer1,
                                          uint8_t *SntpServer2, uint8_t *SntpServer3)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the SNTP configuration */
  return TranslateErrorStatus(W61_Net_GetSNTPConfiguration(p_DrvObj, Enable, Timezone, SntpServer1,
                                                           SntpServer2, SntpServer3));
}

W6X_Status_t W6X_Net_SetSNTPConfiguration(uint8_t Enable, int16_t Timezone, uint8_t *SntpServer1,
                                          uint8_t *SntpServer2, uint8_t *SntpServer3)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Set the SNTP configuration */
  return TranslateErrorStatus(W61_Net_SetSNTPConfiguration(p_DrvObj, Enable, Timezone, SntpServer1,
                                                           SntpServer2, SntpServer3));
}

W6X_Status_t W6X_Net_GetTime(uint8_t *Time)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the current time */
  return TranslateErrorStatus(W61_Net_GetSNTPTime(p_DrvObj, Time));
}

W6X_Status_t W6X_Net_GetSNTPInterval(uint16_t *Interval)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the time interval to refresh the SNTP time */
  return TranslateErrorStatus(W61_Net_GetSNTPInterval(p_DrvObj, Interval));
}

W6X_Status_t W6X_Net_SetSNTPInterval(uint16_t Interval)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Set the time interval to refresh the SNTP time */
  return TranslateErrorStatus(W61_Net_SetSNTPInterval(p_DrvObj, Interval));
}

W6X_Status_t W6X_Net_GetConnectionStatus(uint8_t Socket, uint8_t *Protocol, uint8_t *RemoteIp, uint32_t *RemotePort,
                                         uint32_t *LocalPort, uint8_t *Type)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the socket connection status */
  return TranslateErrorStatus(W61_Net_GetSocketInformation(p_DrvObj, Socket, Protocol, RemoteIp, RemotePort,
                                                           LocalPort, Type));
}

int32_t W6X_Net_Socket(int32_t family, int32_t type, int32_t proto)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (family != AF_INET) /* Only IPv4 for the moment */
  {
    return ret;
  }

  for (int32_t i = 0; i < (W61_NET_MAX_CONNECTIONS + 1); i++) /* Find an available socket */
  {
    int32_t socket_to_use = (i + p_net_ctx->NextSocketToUse) % (W61_NET_MAX_CONNECTIONS + 1);
    if (p_net_ctx->Sockets[socket_to_use].Status == W6X_SOCKET_RESET) /* Check if the socket is available */
    {
      memset(&p_net_ctx->Sockets[socket_to_use], 0, sizeof(W6X_Net_Socket_t)); /* Reset the socket */
      if ((type == SOCK_STREAM) && (proto == TCP_PROTOCOL)) /* TCP */
      {
        p_net_ctx->Sockets[socket_to_use].Protocol = W6X_NET_TCP_PROTOCOL;
      }
      else if ((type == SOCK_DGRAM) && (proto == UDP_PROTOCOL)) /* UDP */
      {
        p_net_ctx->Sockets[socket_to_use].Protocol = W6X_NET_UDP_PROTOCOL;
      }
      else if ((type == SOCK_STREAM) && (proto == SSL_PROTOCOL)) /* SSL */
      {
        p_net_ctx->Sockets[socket_to_use].Protocol = W6X_NET_SSL_PROTOCOL;
      }
      else
      {
        return ret; /* Only TCP, UDP and SSL are supported */
      }

      /* Initialize the socket with default values */
      p_net_ctx->Sockets[socket_to_use].Status = W6X_SOCKET_ALLOCATED;
      p_net_ctx->Sockets[socket_to_use].SoLinger = -1;
      p_net_ctx->Sockets[socket_to_use].RecvTimeout = W6X_NET_RECV_TIMEOUT;
      p_net_ctx->Sockets[socket_to_use].RecvBuffSize = W6X_NET_RECV_BUFFER_SIZE;
      p_net_ctx->Sockets[socket_to_use].Number = W61_NET_MAX_CONNECTIONS + 1;

      /* Set the rotary index for the next socket to use */
      p_net_ctx->NextSocketToUse = (socket_to_use + 1) % (W61_NET_MAX_CONNECTIONS + 1);
      return socket_to_use;
    }
  }
  return ret; /* No available socket */
}

int32_t W6X_Net_Close(int32_t sock)
{
  W61_Net_Connection_t conn;
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if ((sock < 0) || (sock >= W61_NET_MAX_CONNECTIONS + 1))
  {
    return ret;
  }

  /* Check if the socket is used */
  if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_CONNECTED)
  {
    conn.Number = p_net_ctx->Sockets[sock].Number;
    /* Set the socket status to closing to terminate remaining process */
    p_net_ctx->Sockets[sock].Status = W6X_SOCKET_CLOSING;

    /* Stop the connection */
    int retry = 0;
    while (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].SocketConnected == 1)
    {
      if (W61_Net_StopClientConnection(p_DrvObj, &conn) != W61_STATUS_OK)
      {
        retry++;
        vTaskDelay(100);
      }
    }
    /* Take Semaphore in case some data was not read */
    if (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].DataAvailable != NULL)
    {
      (void)xSemaphoreTake(p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].DataAvailable, (TickType_t) 10);
    }
    /* Reset the connection */
    taskENTER_CRITICAL();
    p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].DataAvailableSize = 0;
    taskEXIT_CRITICAL();
    p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].SocketConnected = 0;
  }

  /* Free credentials that were allocated */
  if (p_net_ctx->Sockets[sock].Ca_Cert != NULL)
  {
    vPortFree(p_net_ctx->Sockets[sock].Ca_Cert);
  }
  if (p_net_ctx->Sockets[sock].Private_Key != NULL)
  {
    vPortFree(p_net_ctx->Sockets[sock].Private_Key);
  }
  if (p_net_ctx->Sockets[sock].Certificate != NULL)
  {
    vPortFree(p_net_ctx->Sockets[sock].Certificate);
  }
  if (p_net_ctx->Sockets[sock].PSK != NULL)
  {
    vPortFree(p_net_ctx->Sockets[sock].PSK);
  }
  if (p_net_ctx->Sockets[sock].PSK_Identity != NULL)
  {
    vPortFree(p_net_ctx->Sockets[sock].PSK_Identity);
  }

  memset(&p_net_ctx->Sockets[sock], 0, sizeof(W6X_Net_Socket_t)); /* Reset the socket */

  /* Set the socket number to an invalid value */
  p_net_ctx->Sockets[sock].Number = W61_NET_MAX_CONNECTIONS + 1;

  return 0;
}

int32_t W6X_Net_Shutdown(int32_t sock, int32_t close_connection)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  ret = 0;
  if (p_net_ctx->Sockets[sock].IsConnected == 1) /* Check if the socket is a server */
  {
    LogWarn("Given socket is a connection not a server\n");
    return ret;
  }

  /* Set the socket status to closing to terminate remaining process */
  p_net_ctx->Sockets[sock].Status = W6X_SOCKET_CLOSING;

  if (close_connection == 1) /* Requested to close all pending connections */
  {
    for (int32_t i = 0; i <  W61_NET_MAX_CONNECTIONS + 1; i++)
    {
      /* Check if the socket is used */
      if ((p_net_ctx->Sockets[i].Status != W6X_SOCKET_RESET) && (i != sock) && (p_net_ctx->Sockets[i].Client == 0))
      {
        ret = W6X_Net_Close(i);
        if (ret != 0) /* Close the connection */
        {
          LogError("Could not close connection %" PRIu16 "\n", p_net_ctx->Sockets[i].Number);
        }
      }
    }
  }

  if (ret != 0)
  {
    return ret;
  }

  ret = W6X_Net_TranslateErrorStatus(W61_Net_StopServer(p_DrvObj, close_connection));
  if (ret != 0)
  {
    if (ret != -2) /* BUSY */
    {
      LogError("Failed to stop the server\n");
    }
    return ret;
  }

  p_net_ctx->Sockets[sock].Status = W6X_SOCKET_RESET; /* Set the socket status to reset */

  return 0;
}

int32_t W6X_Net_Bind(int32_t sock, const struct sockaddr *addr, socklen_t addrlen)
{
  int32_t ret = -1;
  struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  /* Check if the socket is allocated */
  if (p_net_ctx->Sockets[sock].Status != W6X_SOCKET_ALLOCATED)
  {
    LogError("Wrong Unexpected socket status: %" PRIi32 " status is %" PRIi32 "\n",
             sock, (int32_t)p_net_ctx->Sockets[sock].Status);
    return ret;
  }

  p_net_ctx->Sockets[sock].IsConnected = 0; /* Set the socket as a server */
  p_net_ctx->Sockets[sock].LocalPort = PP_NTOHS(addr_in->sin_port); /* Set the local port */
  p_net_ctx->Sockets[sock].Status = W6X_SOCKET_BIND; /* Set the socket status to bind */

  return 0;
}

int32_t W6X_Net_Connect(int32_t sock, const struct sockaddr *addr, socklen_t addrlen)
{
  struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
  W61_Net_Connection_t conn;
  int32_t conn_to_use = -1;
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (p_net_ctx->Sockets[sock].Status != W6X_SOCKET_ALLOCATED) /* Check if the socket is allocated */
  {
    return ret;
  }

  p_net_ctx->Sockets[sock].IsConnected = 1; /* Set the socket as a client */
  p_net_ctx->Sockets[sock].Client = 1;

  /* Set the remote port and IP address */
  p_net_ctx->Sockets[sock].RemotePort = PP_NTOHS(addr_in->sin_port);
  p_net_ctx->Sockets[sock].RemoteIP[0] = (uint8_t)(addr_in->sin_addr_t.s_addr >> 24);
  p_net_ctx->Sockets[sock].RemoteIP[1] = (uint8_t)((addr_in->sin_addr_t.s_addr >> 16) & 0xff);
  p_net_ctx->Sockets[sock].RemoteIP[2] = (uint8_t)((addr_in->sin_addr_t.s_addr >> 8) & 0xff);
  p_net_ctx->Sockets[sock].RemoteIP[3] = (uint8_t)(addr_in->sin_addr_t.s_addr & 0xff);

  /* Find an available connection */
  for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
  {
    /* Check if the connection is available */
    int32_t found = 0;
    if (p_net_ctx->Connection[i].SocketConnected == 0) /* Check if the socket is connected */
    {
      for (int32_t j = 0; j < W61_NET_MAX_CONNECTIONS + 1; j++)
      {
        /* Check if the connection found is already in use */
        if ((p_net_ctx->Sockets[j].Number == i) && (p_net_ctx->Sockets[j].Status != W6X_SOCKET_RESET))
        {
          found = 1;
          break;
        }
      }
      if (found == 0)
      {
        conn_to_use = i;
        break;
      }
    }
  }

  if (conn_to_use == -1) /* No available connection */
  {
    return ret;
  }
  /* Set the connection number */
  p_net_ctx->Sockets[sock].Number = conn_to_use;

  /* Set the connection parameters */
  conn.Number = p_net_ctx->Sockets[sock].Number;
  conn.RemotePort = p_net_ctx->Sockets[sock].RemotePort;
  switch (p_net_ctx->Sockets[sock].Protocol)
  {
    case W6X_NET_TCP_PROTOCOL:
      conn.Type = W61_NET_TCP_CONNECTION;
      break;
    case W6X_NET_UDP_PROTOCOL:
      conn.Type = W61_NET_UDP_CONNECTION;
      break;
    case W6X_NET_SSL_PROTOCOL:
      conn.Type = W61_NET_TCP_SSL_CONNECTION;
      int32_t auth_mode = 0; /* Set the authentication mode. Can be client, server or both */
      if (p_net_ctx->Sockets[sock].Ca_Cert != NULL) /* Check if the CA certificate is used */
      {
        auth_mode++; /* Client authentication */
      }
      /* Check if the client certificate and private key are used */
      if ((p_net_ctx->Sockets[sock].Certificate != NULL) && (p_net_ctx->Sockets[sock].Private_Key != NULL))
      {
        auth_mode += 2; /* Server authentication */
      }
      ret = W6X_Net_TranslateErrorStatus(W61_Net_SetSSLConfiguration(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                     auth_mode,
                                                                     (uint8_t *)p_net_ctx->Sockets[sock].Certificate,
                                                                     (uint8_t *)p_net_ctx->Sockets[sock].Private_Key,
                                                                     (uint8_t *)p_net_ctx->Sockets[sock].Ca_Cert));
      if (ret != 0)
      {
        if (ret != -2) /* BUSY */
        {
          LogError("Could not set SSL Client configuration\n");
        }
        return ret;
      }

      if (auth_mode == 0) /* Setup PSK if no certificate is used */
      {
        if (p_net_ctx->Sockets[sock].PSK != NULL && p_net_ctx->Sockets[sock].PSK_Identity != NULL)
        {
          /* Set the PSK configuration */
          ret = W6X_Net_TranslateErrorStatus(W61_Net_SetSSLPSK(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                               (uint8_t *)p_net_ctx->Sockets[sock].PSK,
                                                               (uint8_t *)p_net_ctx->Sockets[sock].PSK_Identity));
          if (ret != 0)
          {
            if (ret != -2) /* BUSY */
            {
              LogError("Could not set SSL Client PSK configuration\n");
            }
            return ret;
          }
        }
      }

      /* Set the ALPN */
      ret = W6X_Net_TranslateErrorStatus(W61_Net_SetSSLALPN(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                            (uint8_t *)p_net_ctx->Sockets[sock].ALPN[0],
                                                            (uint8_t *)p_net_ctx->Sockets[sock].ALPN[1],
                                                            (uint8_t *)p_net_ctx->Sockets[sock].ALPN[2]));
      if (ret != 0)
      {
        if (ret != -2) /* BUSY */
        {
          LogError("Could not set SSL ALPN\n");
        }
        return ret;
      }

      /* Set the Server Name Indication */
      ret = W6X_Net_TranslateErrorStatus(W61_Net_SetSSLServerName(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                  (uint8_t *)p_net_ctx->Sockets[sock].SNI));
      if (ret != 0)
      {
        if (ret != -2) /* BUSY */
        {
          LogError("Could not set SSL ALPN\n");
        }
        return ret;
      }
      break;
  }

  conn.RemoteIP[0] = p_net_ctx->Sockets[sock].RemoteIP[0];
  conn.RemoteIP[1] = p_net_ctx->Sockets[sock].RemoteIP[1];
  conn.RemoteIP[2] = p_net_ctx->Sockets[sock].RemoteIP[2];
  conn.RemoteIP[3] = p_net_ctx->Sockets[sock].RemoteIP[3];

  /* Set the keep alive idle time */
  conn.KeepAlive = p_net_ctx->Sockets[sock].SoKeepAlive;

  /* Set the TCP options */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_SetTCPOpt(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                       p_net_ctx->Sockets[sock].SoLinger,
                                                       p_net_ctx->Sockets[sock].TcpNoDelay,
                                                       p_net_ctx->Sockets[sock].SoSndTimeo,
                                                       p_net_ctx->Sockets[sock].SoKeepAlive));
  if (ret != 0)
  {
    if (ret != -2) /* BUSY */
    {
      LogError("Could not set TCP options\n");
    }
    return ret;
  }

  /* Set the Net Receive buffer length */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_SetReceiveBufferLen(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                 p_net_ctx->Sockets[sock].RecvBuffSize));
  if (ret != 0)
  {
    if (ret != -2) /* BUSY */
    {
      LogError("Could not set Receive buffer size\n");
    }
    return ret;
  }

  /* Start the connection */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_StartClientConnection(p_DrvObj, &conn));
  if (ret != 0)
  {
    if (ret != -2) /* BUSY */
    {
      LogError("Could not connect\n");
    }
    return ret;
  }

  if (p_net_ctx->Connection[conn_to_use].SocketConnected == 0) /* Normally it arrives before SEND OK msg */
  {
    LogDebug("Wait a bit more Connect Callback\n");
    vTaskDelay(10);
    if (p_net_ctx->Connection[conn_to_use].SocketConnected == 0) /* To check if semaphore if needed */
    {
      LogError("Could not connect\n");
      return ret;
    }
  }

  p_net_ctx->Sockets[sock].Status = W6X_SOCKET_CONNECTED; /* Set the socket status to connected */

  return 0;
}

int32_t W6X_Net_Listen(int32_t sock, int32_t backlog)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  /* Check if the socket is a server and in bind mode */
  if ((p_net_ctx->Sockets[sock].IsConnected != 0) || (p_net_ctx->Sockets[sock].Status != W6X_SOCKET_BIND))
  {
    return ret;
  }

  if (backlog > W61_NET_MAX_CONNECTIONS) /* Check if the backlog is in socket limits */
  {
    return ret;
  }
  else if (backlog == 0)
  {
    backlog = W61_NET_MAX_CONNECTIONS; /* When set to 0, use maximum amount available */
  }

  /* Set the server maximum connections */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_SetServerMaxConnections(p_DrvObj, (uint8_t)backlog));
  if (ret != 0)
  {
    return ret;
  }
  W61_Net_ConnectionType_e type;
  switch (p_net_ctx->Sockets[sock].Protocol)
  {
    case W6X_NET_TCP_PROTOCOL:
      type = W61_NET_TCP_CONNECTION;
      break;
    case W6X_NET_UDP_PROTOCOL:
      type = W61_NET_UDP_CONNECTION;
      break;
    case W6X_NET_SSL_PROTOCOL:
      type = W61_NET_TCP_SSL_CONNECTION;
      break;
    default:
      type = W61_NET_TCP_CONNECTION;
      break;
  }

  /* Start the server */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_StartServer(p_DrvObj, p_net_ctx->Sockets[sock].LocalPort,
                                                         type, 0, p_net_ctx->Sockets[sock].SoKeepAlive));
  if (ret != 0)
  {
    return ret;
  }

  p_net_ctx->Sockets[sock].Status = W6X_SOCKET_LISTENING; /* Set the socket status to listening */
  return 0;
}

int32_t W6X_Net_Accept(int32_t sock, struct sockaddr *addr, socklen_t *addrlen)
{
  int32_t type;
  int32_t proto;
  int32_t new_socket;
  int32_t ret = -1;
  uint8_t protocol[8];
  char remote_ip[16];
  uint8_t connection_type;
  uint32_t local_port;
  uint32_t remote_port;
  struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  /* Check if the socket is a server and in listening mode */
  if ((p_net_ctx->Sockets[sock].IsConnected != 0) || (p_net_ctx->Sockets[sock].Status != W6X_SOCKET_LISTENING))
  {
    LogError("Socket is not a server in Listening\n");
    return ret;
  }

  /* Check the protocol. Only TCP and UDP are supported */
  if (p_net_ctx->Sockets[sock].Protocol == W6X_NET_TCP_PROTOCOL) /* TCP */
  {
    type = SOCK_STREAM;
    proto = 6;
  }
  else /* UDP */
  {
    type = SOCK_DGRAM;
    proto = 17;
  }

  new_socket = W6X_Net_Socket(AF_INET, type, proto); /* Create a new socket */
  if (new_socket < 0)
  {
    LogError("Unable to create a new socket, limit reached\n");
    return ret;
  }

  while (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_LISTENING) /* Wait for a connection */
  {
    for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
    {
      int32_t found = 0;
      if (p_net_ctx->Connection[i].SocketConnected == 1) /* Check if the socket is connected */
      {
        for (int32_t j = 0; j < W61_NET_MAX_CONNECTIONS + 1; j++)
        {
          /* Check if the connected socket found is already in use */
          if ((p_net_ctx->Sockets[j].Number == i) && (p_net_ctx->Sockets[j].Status == W6X_SOCKET_CONNECTED))
          {
            found = 1;
            break;
          }
        }
        if (found == 0) /* If the connected socket is not in use */
        {
          ret = W6X_Net_TranslateErrorStatus(W61_Net_GetSocketInformation(p_DrvObj, i, protocol,
                                                                          (uint8_t *)remote_ip,
                                                                          &remote_port, &local_port,
                                                                          &connection_type));

          if (ret == 0)
          {
            if (connection_type == 0) /* Server connection */
            {
              LogError("Found an unregistered connection that is not flagged as server connection");
              return -1;
            }
            addr_in->sin_port = PP_HTONS(remote_port);
            /* Set the remote IP address */
            W6X_Net_Inet_pton(AF_INET, remote_ip, (const void *) & (addr_in->sin_addr_t.s4_addr));
          }
          /* Set the new socket parameters */
          p_net_ctx->Sockets[new_socket].Number = i;
          p_net_ctx->Sockets[new_socket].IsConnected = 1;
          p_net_ctx->Sockets[new_socket].Client = 0;
          p_net_ctx->Sockets[new_socket].RecvTimeout = p_net_ctx->Sockets[sock].RecvTimeout;
          p_net_ctx->Sockets[new_socket].Status = W6X_SOCKET_CONNECTED;
          return new_socket;
        }
      }
    }
    vTaskDelay(100); /* Wait a bit before checking again */
  }
  return ret;
}

ssize_t W6X_Net_Send(int32_t sock, const void *buf, size_t len, int32_t flags)
{
  uint32_t SentDataLen = 0;
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);
  NULL_ASSERT(buf, W6X_Buf_Null_str);

  /* Check if the socket is connected */
  if ((p_net_ctx->Sockets[sock].Status != W6X_SOCKET_CONNECTED) ||
      (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].SocketConnected == 0))
  {
    LogError("Socket state is not connected\n");
    return ret;
  }

  /* Send the data */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_SendData(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                      (uint8_t *)buf, (uint32_t)len, &SentDataLen,
                                                      p_net_ctx->Sockets[sock].SoSndTimeo + 1000));
  if (ret != 0)
  {
    return (ssize_t)ret;
  }

  return (ssize_t) SentDataLen;
}

ssize_t W6X_Net_Recv(int32_t sock, void *buf, size_t max_len, int32_t flags)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);
  NULL_ASSERT(buf, W6X_Buf_Null_str);

  if (p_net_ctx->Sockets[sock].Status != W6X_SOCKET_CONNECTED) /* Check if the socket is connected */
  {
    LogError("Socket state is not connected\n");
    return ret;
  }
  if ((p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].SocketConnected == 0) &&
      (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].DataAvailableSize == 0))
  {
    return -1;
  }
  /* Pull data from the socket */
  ret = W6X_Net_Wait_Pull_Data(sock, p_net_ctx->Sockets[sock].Number, buf, max_len);
  if ((ret < 0) && (ret != -2))
  {
    LogError("Pull data from socket failed\n");
  }
  else if (ret == 0)
  {
    /* Check if the socket is still connected */
    if (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].SocketConnected == 0)
    {
      return -1;
    }
  }

  return (ssize_t)ret;
}

ssize_t W6X_Net_Sendto(int32_t sock, const void *buf, size_t len, int32_t flags, const struct sockaddr *dest_addr,
                       socklen_t addrlen)
{
  int32_t ret = -1;
  int32_t conn_to_use = -1;
  int32_t connection_id;
  uint32_t SentDataLen = 0;
  char ipaddr[INET_ADDRSTRLEN + 1] = {0};
  struct sockaddr_in *addr_in = (struct sockaddr_in *) dest_addr;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);
  NULL_ASSERT(buf, W6X_Buf_Null_str);

  if (p_net_ctx->Sockets[sock].Protocol != W6X_NET_UDP_PROTOCOL) /* Check if the socket protocol is UDP */
  {
    LogError("Socket protocol is not UDP\n");
    return ret;
  }

  p_net_ctx->Sockets[sock].RemotePort = PP_NTOHS(addr_in->sin_port);
  /* Get the destination IP address */
  char *ip_ptr = W6X_Net_Inet_ntop(AF_INET, (void *) & (addr_in->sin_addr_t.s_addr), ipaddr, INET_ADDRSTRLEN + 1);
  if (ip_ptr == NULL)
  {
    LogError("Could not decode ip address\n");
    return ret;
  }

  if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_ALLOCATED) /* Check if the socket is allocated */
  {
    /* Find an available connection */
    for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
    {
      /* Check if the connection is available */
      if (p_net_ctx->Connection[i].SocketConnected == 0)
      {
        conn_to_use = i; /* Set the connection to use */
        break;
      }
    }

    if (conn_to_use == -1) /* No available connection */
    {
      LogError("No connection available\n");
      return ret;
    }
    p_net_ctx->Sockets[sock].Number = conn_to_use;
    p_net_ctx->Connection[conn_to_use].SocketConnected = 1;
    p_net_ctx->Sockets[sock].Status = W6X_SOCKET_CONNECTED;
  }

  if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_CONNECTED)
  {
    /* Send the data */
    ret = W6X_Net_TranslateErrorStatus(W61_Net_SendData_Non_Connected(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                      ip_ptr, p_net_ctx->Sockets[sock].RemotePort,
                                                                      (uint8_t *)buf, (uint32_t)len, &SentDataLen,
                                                                      p_net_ctx->Sockets[sock].SoSndTimeo + 1000));
    if (ret == 0)
    {
      return (ssize_t) SentDataLen;
    }
  }
  else if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_LISTENING)
  {
    /* Check if the socket is a server */
    connection_id = W6X_Net_Find_Connection(ip_ptr, p_net_ctx->Sockets[sock].RemotePort);
    if (connection_id >= 0) /* Check if the connection is found */
    {
      /* Send the data */
      ret = W6X_Net_TranslateErrorStatus(W61_Net_SendData(p_DrvObj, connection_id, (uint8_t *)buf, (uint32_t)len,
                                                          &SentDataLen, p_net_ctx->Sockets[sock].SoSndTimeo + 1000));
      if (ret == 0)
      {
        return (ssize_t) SentDataLen;
      }
    }
  }
  return ret;
}

ssize_t W6X_Net_Recvfrom(int32_t sock, void *buf, size_t max_len, int32_t flags, struct sockaddr *src_addr,
                         socklen_t *addrlen)
{
  /* If the socket do not need to connect to peer then ret will not be updated so initialize to 0 */
  int32_t ret = -1;
  int32_t udp_connection;
  struct sockaddr_in *addr_in = (struct sockaddr_in *) src_addr;
  TickType_t startTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
  TickType_t currentTime;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);
  NULL_ASSERT(buf, W6X_Buf_Null_str);

  if (p_net_ctx->Sockets[sock].Protocol != W6X_NET_UDP_PROTOCOL)
  {
    LogError("Socket protocol is not UDP\n");
    return -1;
  }

  /* Cannot receive data on a socket that is not connected */
  if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_ALLOCATED)
  {
    return -1;
  }

  /* If the socket is connected, receive data */
  if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_CONNECTED)
  {
    return W6X_Net_Recv(sock, buf, max_len, flags); /* Receive data */
  }

  /* If the socket is a server, move to listening mode */
  if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_BIND)
  {
    if (p_net_ctx->Sockets[sock].Protocol == W6X_NET_UDP_PROTOCOL)
    {
      if (W6X_Net_Listen(sock, 0) != 0) /* Start listening */
      {
        return -1;
      }
    }
  }

  /* If the socket is a server, receive data from the client */
  if (p_net_ctx->Sockets[sock].Status == W6X_SOCKET_LISTENING)
  {
    while (1)
    {
      udp_connection = W6X_Net_Poll_UDP_Sockets(addr_in); /* Poll the UDP sockets */
      if (udp_connection != -1)
      {
        break;
      }

      vTaskDelay(1);
      currentTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
      if ((currentTime - startTime) > (TickType_t) p_net_ctx->Sockets[sock].RecvTimeout)
      {
        LogError("No data received within %" PRIu32 " s\n", p_net_ctx->Sockets[sock].RecvTimeout / 1000);
        return 0;
      }
    }

    ret = W6X_Net_Wait_Pull_Data(sock, udp_connection, buf, max_len); /* Pull data from the socket */
    if ((ret < 0) && (ret != -2))
    {
      LogError("Pull data from socket failed\n");
    }
    else if (ret == 0)
    {
      /* Check if the socket is still connected */
      LogDebug("No Data available to read in connection %" PRIi32 "\n", udp_connection);
    }
  }
  else
  {
    LogDebug("No Data available to read in socket %" PRIu16 "\n", p_net_ctx->Sockets[sock].Number);
    return 0;
  }

  return (ssize_t)ret;
}

char *W6X_Net_Inet_ntop(int32_t af, const void *src, char *dst, socklen_t size)
{
  char *ret = NULL;
  int32_t size_int = (int)size;
  uint32_t src_addr = *((uint32_t *) src);

  if (size_int < 0)
  {
    /* set_errno(ENOSPC); */
    return NULL;
  }
  switch (af)
  {
    case AF_INET:
      snprintf(dst, size, "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16,
               (uint8_t)(src_addr >> 24), (uint8_t)((src_addr >> 16) & 0xff),
               (uint8_t)((src_addr >> 8) & 0xff), (uint8_t)(src_addr & 0xff));
      ret = dst;
      if (ret == NULL)
      {
        /* set_errno(ENOSPC); */
      }
      break;

    case AF_INET6:
      /* ret = ip6addr_ntoa_r((const ip6_addr_t *)src, dst, size_int); */
      if (ret == NULL)
      {
        /* set_errno(ENOSPC); */
      }
      break;

    default:
      /* set_errno(EAFNOSUPPORT); */
      break;
  }
  return ret;
}

int32_t W6X_Net_Inet_pton(int32_t af, char *src, const void *dst)
{
  int32_t ret = -1;
  uint8_t *dst_addr = ((uint8_t *) dst);
  uint8_t dst_addr_tmp[4] = {0};

  switch (af)
  {
    case AF_INET:
      Parser_StrToIP(src, dst_addr_tmp);
      if (dst_addr_tmp[0] == 0)
      {
        /* set_errno(ENOSPC); */
      }
      else
      {
        /* use correct endianness */
        dst_addr[0] = dst_addr_tmp[3];
        dst_addr[1] = dst_addr_tmp[2];
        dst_addr[2] = dst_addr_tmp[1];
        dst_addr[3] = dst_addr_tmp[0];
        ret = 0;
      }
      break;

    case AF_INET6:

      if (ret == -1)
      {
        /* set_errno(ENOSPC); */
      }
      break;

    default:
      /* set_errno(EAFNOSUPPORT); */
      break;
  }
  return ret;
}

int32_t W6X_Net_Getsockopt(int32_t sock, int32_t level, int32_t optname, void *optval, socklen_t *optlen)
{
  int32_t value = 0;
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (level == SOL_SOCKET)
  {
    switch (optname) /* Get the socket option */
    {
      case SO_LINGER: /* Get the Linger time */
        value = p_net_ctx->Sockets[sock].SoLinger;
        break;
      case TCP_NODELAY: /* Get the TCP No Delay */
        value = p_net_ctx->Sockets[sock].TcpNoDelay;
        break;
      case SO_KEEPALIVE: /* Get the Keep Alive */
        value = p_net_ctx->Sockets[sock].SoKeepAlive;
        break;
      case SO_SNDTIMEO: /* Get the Send Timeout */
        value = p_net_ctx->Sockets[sock].SoSndTimeo;
        break;
      case SO_RCVTIMEO: /* Get the Receive Timeout */
        value = p_net_ctx->Sockets[sock].RecvTimeout;
        break;
      case SO_RCVBUF: /* Get the Receive buffer length */
        value = p_net_ctx->Sockets[sock].RecvBuffSize;
        break;
      default:
        return ret;
        break;
    }
  }
  else
  {
    return ret;
  }
  *(int32_t *)optval = value; /* Return the value of the requested option */
  return 0;
}

int32_t W6X_Net_Setsockopt(int32_t sock, int32_t level, int32_t optname, const void *optval, socklen_t optlen)
{
  int32_t value = *(int32_t *)optval; /* Get the value of the option */
  int32_t ret = -1;
  int8_t *tags = (int8_t *)optval;
  char *alpn_start;
  char *alpn_end;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  /* Check if the socket is initialized but not yet used */
  if (p_net_ctx->Sockets[sock].Status != W6X_SOCKET_ALLOCATED)
  {
    if ((level == SOL_TLS) || ((optname != SO_SNDTIMEO) && (optname != SO_RCVTIMEO)))
    {
      /* Only Timeouts can be set once server or client is started */
      LogError("Socket has already been started\n");
      return ret;
    }
  }
  if (level == SOL_SOCKET)
  {
    switch (optname)
    {
      case SO_LINGER:
        if (value < -1) /* -1 for default, 0 for no linger, >0 for linger time */
        {
          return ret;
        }
        p_net_ctx->Sockets[sock].SoLinger = value;
        break;
      case TCP_NODELAY:
        if ((value < 0) || (value > 1)) /* Only 0 or 1 */
        {
          return ret;
        }
        p_net_ctx->Sockets[sock].TcpNoDelay = value;
        break;
      case SO_SNDTIMEO:
        if (value < 0) /* 0 for non-blocking, >0 for timeout */
        {
          return ret;
        }
        p_net_ctx->Sockets[sock].SoSndTimeo = value;
        break;
      case SO_RCVTIMEO:
        if (value < 0) /* 0 for non-blocking, >0 for timeout */
        {
          return ret;
        }
        p_net_ctx->Sockets[sock].RecvTimeout = value;
        break;
      case SO_RCVBUF:
        if (value < 0)
        {
          return ret;
        }
        p_net_ctx->Sockets[sock].RecvBuffSize = value;
        break;
      case SO_KEEPALIVE:
        if ((value < 0) || (value > 7200))
        {
          return ret;
        }
        p_net_ctx->Sockets[sock].SoKeepAlive = value;
        break;
      default:
        return ret;
        break;
    }
  }
  else if (level == SOL_TLS && p_net_ctx->Sockets[sock].Protocol == W6X_NET_SSL_PROTOCOL)
  {
    switch (optname)
    {
      case TLS_SEC_TAG_LIST:
        for (uint8_t i = 0; i < optlen; i++)
        {
          char *credential = p_net_ctx->Credentials[tags[i]].credential;
          uint32_t credential_len = p_net_ctx->Credentials[tags[i]].credential_len;

          switch (p_net_ctx->Credentials[tags[i]].type)
          {
            case W6X_NET_TLS_CREDENTIAL_CA_CERTIFICATE:
              if (p_net_ctx->Sockets[sock].Ca_Cert != NULL) /* Check if the CA Certificate is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].Ca_Cert);
              }

              p_net_ctx->Sockets[sock].Ca_Cert = (char *) pvPortMalloc(credential_len + 1);
              if (p_net_ctx->Sockets[sock].Ca_Cert == NULL)
              {
                LogError("Malloc failed\n");
                return -1;
              }
              /* Store the CA Certificate */
              strncpy(p_net_ctx->Sockets[sock].Ca_Cert, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_SERVER_CERTIFICATE:
              if (p_net_ctx->Sockets[sock].Certificate != NULL) /* Check if the Certificate is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].Certificate);
              }

              p_net_ctx->Sockets[sock].Certificate = (char *) pvPortMalloc(credential_len + 1);
              if (p_net_ctx->Sockets[sock].Certificate == NULL)
              {
                LogError("Malloc failed\n");
                return -1;
              }
              /* Store the Certificate */
              strncpy(p_net_ctx->Sockets[sock].Certificate, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_PRIVATE_KEY:
              if (p_net_ctx->Sockets[sock].Private_Key != NULL) /* Check if the Private Key is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].Private_Key);
              }

              p_net_ctx->Sockets[sock].Private_Key = (char *) pvPortMalloc(credential_len + 1);
              if (p_net_ctx->Sockets[sock].Private_Key == NULL)
              {
                LogError("Malloc failed\n");
                return -1;
              }
              /* Store the Private Key */
              strncpy(p_net_ctx->Sockets[sock].Private_Key, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_PSK:
              if (p_net_ctx->Sockets[sock].PSK != NULL) /* Check if the PSK is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].PSK);
              }

              p_net_ctx->Sockets[sock].PSK = (char *) pvPortMalloc(credential_len + 1);
              if (p_net_ctx->Sockets[sock].PSK == NULL)
              {
                LogError("Malloc failed\n");
                return -1;
              }
              /* Store the PSK */
              strncpy(p_net_ctx->Sockets[sock].PSK, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_PSK_ID:
              if (p_net_ctx->Sockets[sock].PSK_Identity != NULL) /* Check if the PSK Identity is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].PSK_Identity);
              }

              p_net_ctx->Sockets[sock].PSK_Identity = (char *) pvPortMalloc(credential_len + 1);
              if (p_net_ctx->Sockets[sock].PSK_Identity == NULL)
              {
                LogError("Malloc failed\n");
                return -1;
              }
              /* Store the PSK Identity */
              strncpy(p_net_ctx->Sockets[sock].PSK_Identity, credential, credential_len);
              break;
            default:
              return -1;
          }
        }

        break;
      case TLS_ALPN_LIST:
        memset(p_net_ctx->Sockets[sock].ALPN, 0, sizeof(p_net_ctx->Sockets[sock].ALPN));
        alpn_start = (char *) optval; /* Set the ALPN. The optval is a comma separated list of ALPNs */
        for (int32_t i = 0; i < 3; i++)
        {
          alpn_end = strstr(alpn_start, ",");
          if (alpn_end == NULL)
          {
            /* Copy the last ALPN */
            strncpy(p_net_ctx->Sockets[sock].ALPN[i], alpn_start, sizeof(p_net_ctx->Sockets[sock].ALPN[i]) - 1);
            break;
          }
          strncpy(p_net_ctx->Sockets[sock].ALPN[i], alpn_start, alpn_end - alpn_start);
          alpn_start = alpn_end + 1;
        }
        break;
      case TLS_HOSTNAME:
        /* Set the Server Name Indication */
        strncpy(p_net_ctx->Sockets[sock].SNI, (char *)optval, sizeof(p_net_ctx->Sockets[sock].SNI) - 1);
        break;
      default:
        LogError("Unsupported TLS option");
        return ret;
    }
  }
  else
  {
    return ret;
  }
  return 0;
}

int32_t W6X_Net_Tls_Credential_Add(uint32_t tag, W6X_Net_Tls_Credential_e type, const void *cred, size_t credlen)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (tag >= W61_NET_MAX_CONNECTIONS * 3)
  {
    LogError("Tag cannot be higher than %" PRIu32 ", value is %" PRIu32 "\n",
             (uint32_t)W61_NET_MAX_CONNECTIONS * 3, tag);
    return -1;
  }
  if (p_net_ctx->Credentials[tag].is_valid == 1)
  {
    LogError("Tag %" PRIu32 " already in use\n", tag);
    return -1; /* Tag already used */
  }
  /* Store the credential information */
  p_net_ctx->Credentials[tag].type = type;
  p_net_ctx->Credentials[tag].credential = (char *) pvPortMalloc(credlen + 1);
  if (p_net_ctx->Credentials[tag].credential == NULL)
  {
    LogError("Malloc failed\n");
    return -1;
  }
  p_net_ctx->Credentials[tag].credential_len = credlen;
  strncpy(p_net_ctx->Credentials[tag].credential, (char *)cred, credlen);

  /* Copy the file into the NCP if it does not already exist or is empty in its filesystem */
  uint32_t size = 0;
  if (type == W6X_NET_TLS_CREDENTIAL_PSK || type == W6X_NET_TLS_CREDENTIAL_PSK_ID)
  {
    p_net_ctx->Credentials[tag].is_valid = 1;
    return 0;
  }
  /* Check if the file is already in the NCP */
  else if (W6X_FS_GetSizeFile(p_net_ctx->Credentials[tag].credential, &size) == W6X_STATUS_OK)
  {
    if (size > 0)
    {
      p_net_ctx->Credentials[tag].is_valid = 1;
      return 0;
    }
  }
  /* Write the file into the NCP */
  if (W6X_FS_WriteFile(p_net_ctx->Credentials[tag].credential) != W6X_STATUS_OK)
  {
    LogError("Writing file %s into the NCP failed\n", p_net_ctx->Credentials[tag].credential);
    return -1;
  }
  p_net_ctx->Credentials[tag].is_valid = 1;
  return 0;
}

int32_t W6X_Net_Tls_Credential_Delete(uint32_t tag, W6X_Net_Tls_Credential_e type)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (tag >= W61_NET_MAX_CONNECTIONS * 3)
  {
    return -1;
  }
  if (p_net_ctx->Credentials[tag].is_valid == 0)
  {
    return -1; /* No such tag is registered */
  }
  if (p_net_ctx->Credentials[tag].credential != NULL)
  {
    /* Forget the credential filename information */
    vPortFree(p_net_ctx->Credentials[tag].credential);
    p_net_ctx->Credentials[tag].credential = NULL;
  }
  p_net_ctx->Credentials[tag].is_valid = 0;
  return 0;
}

/** @} */

/* Private Functions Definition ----------------------------------------------*/
/** @addtogroup ST67W6X_Private_Net_Functions
  * @{
  */

static void W6X_Net_cb(W61_event_id_t event_id, void *event_args)
{
  W61_Net_CbParamData_t *p_param_net_data = (W61_Net_CbParamData_t *) event_args;
  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_net_cb == NULL))
  {
    LogError("Please register the APP callback before initializing the module\n");
    return;
  }

  if (p_net_ctx == NULL)
  {
    return;
  }

  switch (event_id) /* Check the event ID and call the appropriate callback */
  {
    case W61_NET_EVT_SOCK_DATA_ID:
      taskENTER_CRITICAL();
      p_net_ctx->Connection[p_param_net_data->socket_id].DataAvailableSize += p_param_net_data->available_data_length;
      taskEXIT_CRITICAL();
      p_net_ctx->Connection[p_param_net_data->socket_id].RemotePort = p_param_net_data->remote_port;
      p_net_ctx->Connection[p_param_net_data->socket_id].RemoteIP[0] = p_param_net_data->remote_ip[3];
      p_net_ctx->Connection[p_param_net_data->socket_id].RemoteIP[1] = p_param_net_data->remote_ip[2];
      p_net_ctx->Connection[p_param_net_data->socket_id].RemoteIP[2] = p_param_net_data->remote_ip[1];
      p_net_ctx->Connection[p_param_net_data->socket_id].RemoteIP[3] = p_param_net_data->remote_ip[0];
      if (p_net_ctx->Connection[p_param_net_data->socket_id].DataAvailable != NULL)
      {
        (void)xSemaphoreGive(p_net_ctx->Connection[p_param_net_data->socket_id].DataAvailable);
      }
      /* Call the application callback to notify the data available */
      p_cb_handler->APP_net_cb(W6X_NET_EVT_SOCK_DATA_ID, (void *) p_param_net_data);
      break;

    case W61_NET_EVT_SOCK_CONNECTED_ID:
      LogDebug("Socket %" PRIu32 " connected\n", p_param_net_data->socket_id);
      p_net_ctx->Connection[p_param_net_data->socket_id].SocketConnected = 1;
      break;

    case W61_NET_EVT_SOCK_DISCONNECTED_ID:
      LogDebug("Socket %" PRIu32 " disconnected\n", p_param_net_data->socket_id);
      p_net_ctx->Connection[p_param_net_data->socket_id].SocketConnected = 0;
      break;

    default:
      break;
  }
}

static int32_t W6X_Net_TranslateErrorStatus(W61_Status_t ret61)
{
  /* Translate the W61 status to the W6X status */
  switch (ret61)
  {
    case W61_STATUS_OK:
      return 0;
    case W61_STATUS_BUSY:
      return -2;
    default:
      return -1;
  }
}

static int32_t W6X_Net_Find_Connection(char *remote_ip, uint32_t remoteport)
{
  int32_t ret = -1;
  for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
  {
    int32_t found = 0;
    if (p_net_ctx->Connection[i].SocketConnected == 1)
    {
      for (int32_t j = 0; j < W61_NET_MAX_CONNECTIONS + 1; j++)
      {
        if ((p_net_ctx->Sockets[j].Number == i) && (p_net_ctx->Sockets[j].Status == W6X_SOCKET_CONNECTED))
        {
          found = 1; /* Already connected */
          break;
        }
      }
      if (found == 0)
      {
        uint8_t cur_con_protocol[8];
        uint8_t cur_con_type;
        uint32_t cur_con_remoteport;
        uint32_t cur_con_localport;
        uint8_t cur_con_remoteip[24] = {0};

        /* Get the socket information */
        ret = W6X_Net_TranslateErrorStatus(W61_Net_GetSocketInformation(p_DrvObj, i, cur_con_protocol,
                                                                        cur_con_remoteip,
                                                                        &cur_con_remoteport,
                                                                        &cur_con_localport,
                                                                        &cur_con_type));
        if (ret == 0)
        {
          /* Check if IP Address and port match with the current connection */
          if ((strcmp(remote_ip, (char *)cur_con_remoteip) == 0) && (remoteport == cur_con_remoteport))
          {
            return i;
          }
        }
      }
    }
  }
  return ret;
}

static int32_t W6X_Net_Poll_UDP_Sockets(struct sockaddr_in *remote_addr)
{
  int32_t found_socket = 0;
  for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS ; i++)
  {
    /* Find the first active connection with pending data */
    if ((p_net_ctx->Connection[i].SocketConnected != 1) || (p_net_ctx->Connection[i].DataAvailableSize == 0))
    {
      continue; /* Connection not active or no data available */
    }
    /* Since this function is used for UDP server to look for data to read,
     * skip a connection if used by an existing socket (which would be a client) */
    for (int32_t j = 0; j < W61_NET_MAX_CONNECTIONS + 1 ; j++)
    {
      if (p_net_ctx->Sockets[j].Number == i)
      {
        found_socket = 1; /* Connection already used by a socket */
        break;
      }
    }
    if (found_socket == 0) /* Connection not used by any socket */
    {
      remote_addr->sin_port = PP_HTONS(p_net_ctx->Connection[i].RemotePort);
      remote_addr->sin_addr_t.s4_addr[0] = p_net_ctx->Connection[i].RemoteIP[3];
      remote_addr->sin_addr_t.s4_addr[1] = p_net_ctx->Connection[i].RemoteIP[2];
      remote_addr->sin_addr_t.s4_addr[2] = p_net_ctx->Connection[i].RemoteIP[1];
      remote_addr->sin_addr_t.s4_addr[3] = p_net_ctx->Connection[i].RemoteIP[0];
      return i;
    }
  }
  return -1;
}

static int32_t W6X_Net_Wait_Pull_Data(int32_t sock, int32_t connection_id, void *buf, size_t max_len)
{
  uint32_t received_data_len = 0;
  int32_t ret = -1;
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  int32_t cpt = 0;
  BaseType_t sem_taken = pdFALSE;
  do
  {
    if ((p_net_ctx->Connection[connection_id].SocketConnected == 1) ||
        (p_net_ctx->Connection[connection_id].DataAvailableSize > 0))
    {
      if (p_net_ctx->Connection[connection_id].DataAvailable != NULL)
      {
        sem_taken = xSemaphoreTake(p_net_ctx->Connection[connection_id].DataAvailable,
                                   (TickType_t)(100));
      }
    }
    cpt ++;
  } while (sem_taken != pdTRUE && cpt < (p_net_ctx->Sockets[sock].RecvTimeout / 100));

  /* Check recv len before semaphore take (in case previous set of data was not completely read) */
  if (sem_taken == pdTRUE)
  {
    if (max_len > p_net_ctx->Sockets[sock].RecvBuffSize) /* Attempt read */
    {
      max_len = p_net_ctx->Sockets[sock].RecvBuffSize;
    }
    if (p_net_ctx->Connection[connection_id].DataAvailableSize < max_len)
    {
      (void)xSemaphoreTake(p_net_ctx->Connection[connection_id].DataAvailable, (TickType_t) 3);
    }
    if (max_len > p_net_ctx->Connection[connection_id].DataAvailableSize) /* Attempt read */
    {
      max_len = p_net_ctx->Connection[connection_id].DataAvailableSize;
    }
    /* Request data from the socket */
    ret = W6X_Net_TranslateErrorStatus(W61_Net_PullDataFromSocket(p_DrvObj, connection_id,
                                                                  (uint32_t)max_len, (uint8_t *)buf,
                                                                  &received_data_len, W6X_NET_PULL_DATA_TIMEOUT));
    if (ret != 0)
    {
      if (ret != -2)
      {
        LogError("Pull data from socket failed\n");
      }
      return ret;
    }
    /* Should not happen */
    taskENTER_CRITICAL();
    if (received_data_len > p_net_ctx->Connection[connection_id].DataAvailableSize || received_data_len < max_len)
    {
      p_net_ctx->Connection[connection_id].DataAvailableSize = 0;
    }
    else
    {
      p_net_ctx->Connection[connection_id].DataAvailableSize -= received_data_len;
    }
    taskEXIT_CRITICAL();

    /* Check if remaining data is available */
    /* Notify the user if remaining data is available */
    if (p_net_ctx->Connection[connection_id].DataAvailableSize > 0)
    {
      if (p_net_ctx->Connection[connection_id].DataAvailable != NULL)
      {
        (void)xSemaphoreGive(p_net_ctx->Connection[connection_id].DataAvailable);
      }
    }
  }

  return received_data_len;
}

/** @} */
