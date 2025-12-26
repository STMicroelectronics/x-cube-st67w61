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
#include "w6x_types.h"     /* W6X_ARCH_** */
#if (ST67_ARCH == W6X_ARCH_T01)
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
  W6X_NET_SOCKET_RESET = 0,         /*!< Socket is reset */
  W6X_NET_SOCKET_ALLOCATED = 1,     /*!< Socket is initialized and ready to be used */
  W6X_NET_SOCKET_BIND = 2,          /*!< Socket is associated with a local address */
  W6X_NET_SOCKET_LISTENING = 3,     /*!< Socket is listening for incoming connections */
  W6X_NET_SOCKET_CONNECTED = 4,     /*!< Socket is connected */
  W6X_NET_SOCKET_CLOSING = 5        /*!< Socket is closing and cannot be used */
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
  uint32_t RemoteIP[4];             /*!< IP address of device */
  uint8_t TcpNoDelay;               /*!< BSD Socket option TCP_NODELAY */
  char *Ca_Cert;                    /*!< CA certificate */
  char *Private_Key;                /*!< Private key */
  char *Certificate;                /*!< Server Certificate */
  char *PSK;                        /*!< Pre-shared key */
  char *PSK_Identity;               /*!< PSK identity */
  char ALPN[3][17];                 /*!< ALPN list */
  char SNI[W6X_NET_SNI_MAX_SIZE];   /*!< Server Name Indication */
  W6X_Net_Socket_Status_t Status;   /*!< Socket status */
  W6X_Net_Protocol_e Protocol;      /*!< Connection type. Parameter is valid only if connection is made as client */
  W6X_Net_Family_e Family;          /*!< IP address family to use for the socket */
} W6X_Net_Socket_t;

/**
  * @brief  Internal structure to store credentials to use for SSL connections
  */
typedef struct
{
  uint8_t is_valid;                 /*!< Flag to indicate if the credential is valid */
  W6X_Net_Tls_Credential_e type;    /*!< Type of the credential */
  char *name;                       /*!< Credential filename */
} W6X_Net_Credential_t;

/**
  * @brief  Connection structure
  */
typedef struct
{
  uint8_t SocketConnected;          /*!< Socket connected status */
  uint16_t RemotePort;              /*!< Remote PORT number */
  uint32_t RemoteIP[4];             /*!< IP address of device */
  SemaphoreHandle_t DataAvailable;  /*!< Semaphore for data available */
  uint32_t DataAvailableSize;       /*!< Counter for data available */
  W6X_Net_Family_e Family;          /*!< IP address family used by the NCP for that connection */
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

#if (W6X_NET_IPV6_ENABLE == 1) && (W61_NET_IPV6_ENABLE == 0)
#undef W6X_NET_IPV6_ENABLE
#error "IPv6 support must be enabled in W61 for IPv6 W6x capabilities"
#endif /* W6X_NET_IPV6_ENABLE && W61_NET_IPV6_ENABLE */

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

static W6X_NetCtx_t *p_net_ctx = NULL; /*!< Global W6X Net context pointer */

#if (W6X_ASSERT_ENABLE == 1)
/** W6X Net init error string */
static const char W6X_Net_Uninit_str[] = "W6X Net module not initialized";

/** Net context pointer error string */
static const char W6X_Ctx_Null_str[] = "Net context not initialized";

/** Net buffer null string */
static const char W6X_Buf_Null_str[] = "Buffer is NULL";
#endif /* W6X_ASSERT_ENABLE */

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
static int32_t W6X_Net_Poll_UDP_Sockets(struct sockaddr *remote_addr);

/**
  * @brief  Wait for data to be available on a connection
  * @param  sock: socket number
  * @param  connection_id: connection number
  * @param  buf: buffer to store the data
  * @param  max_len: maximum length of the buffer
  * @retval Number of bytes received
  */
static int32_t W6X_Net_Wait_Pull_Data(int32_t sock, int32_t connection_id, void *buf, size_t max_len);

#if (W6X_NET_IPV6_ENABLE == 1)
/**
  * @brief  Convert IPv6 address to string
  * @param  src: pointer to the IPv6 address
  * @param  dst: pointer to the destination string buffer
  * @param  size: size of the destination buffer
  * @retval Pointer to the destination string buffer
  */
static const char *inet_ntop6(const uint8_t src[16], char *dst, size_t size);
#endif /* W6X_NET_IPV6_ENABLE */

/**
  * @brief  Clean the socket when disconnected
  * @param  sock: socket number
  */
static void W6X_Net_Clean_Socket(int32_t sock);

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_Net_Public_Functions
  * @{
  */

W6X_Status_t W6X_Net_Init(void)
{
  /* Initialize network context and variables for W6X Net */
  W6X_App_Cb_t *p_cb_handler;
  W6X_Status_t ret = W6X_STATUS_ERROR;
  uint32_t ncp_buf_size_resp;
  W61_Net_DhcpType_e State = W61_NET_DHCP_STA_AP_ENABLED;
  uint32_t zero = 0;
  uint32_t one = 1;
  uint8_t hostname[33] = {0};
  uint8_t Ipaddr[4] = W6X_NET_SAP_IP_SUBNET;
  uint8_t Netmask_addr[4] = {255, 255, 255, 0};
  Ipaddr[3] = 1; /* Set the last digit of the Soft-AP IP address to 1 */

  /* Get the global W61 context pointer */
  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  if (p_DrvObj->NetCtx.Supported != 1)
  {
    NET_LOG_ERROR("Network module not supported\n");
    goto _err;
  }

  /* Allocate the Net context */
  p_net_ctx = pvPortMalloc(sizeof(W6X_NetCtx_t));

  if (p_net_ctx == NULL)
  {
    NET_LOG_ERROR("Could not initialize Net context structure\n");
    goto _err;
  }
  memset(p_net_ctx, 0, sizeof(W6X_NetCtx_t));

  /* Check that application callback is registered */
  p_cb_handler = W6X_GetCbHandler();
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_net_cb == NULL))
  {
    NET_LOG_ERROR("Please register the APP callback before initializing the module\n");
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

  if ((uint32_t)W6X_NET_DHCP > W6X_NET_DHCP_STA_AP_ENABLED)
  {
    NET_LOG_ERROR("Invalid DHCP configuration\n");
    goto _err;
  }

  /* Reset the DHCP global configuration */
  ret = TranslateErrorStatus(W61_Net_SetDhcpConfig(p_DrvObj, &State, &zero));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set the defined DHCP global configuration */
  State = (W61_Net_DhcpType_e)W6X_NET_DHCP;
  ret = TranslateErrorStatus(W61_Net_SetDhcpConfig(p_DrvObj, &State, &one));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  if (strlen(W6X_NET_HOSTNAME) >= sizeof(hostname)) /* Check the length of the defined hostname */
  {
    NET_LOG_ERROR("Hostname too long\n");
    goto _err;
  }

  strncpy((char *)hostname, W6X_NET_HOSTNAME, sizeof(hostname) - 1);
  /* Set the hostname */
  ret = TranslateErrorStatus(W61_Net_SetHostname(p_DrvObj, hostname));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set default IP of the Soft-AP */
  ret = TranslateErrorStatus(W61_Net_AP_SetIPAddress(p_DrvObj, Ipaddr, Netmask_addr));
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
        NET_LOG_ERROR("Could not set Receive buffer size\n");
      }
      goto _err;
    }

    /* Verify the buffer length available in NCP for each connection */
    ret = TranslateErrorStatus(W61_Net_GetReceiveBufferLen(p_DrvObj, i, &ncp_buf_size_resp));
    if (ret != 0)
    {
      NET_LOG_ERROR("Could not set Receive buffer size\n");
      return ret;
    }
    else
    {
      if (W6X_NET_RECV_BUFFER_SIZE != ncp_buf_size_resp)
      {
        NET_LOG_WARN("SO_RCVBUF was not set as expected, size id %" PRIu32 "\n", ncp_buf_size_resp);
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
    p_net_ctx->Credentials[i].name = NULL;
  }
  p_net_ctx->NextSocketToUse = 0; /* Initialize the rotary index for the socket usage */
_err:
  return ret;
}

void W6X_Net_DeInit(void)
{
  /* Deinitialize the W6X Net context and free resources */
  if ((p_net_ctx == NULL) || (p_DrvObj == NULL))
  {
    return;
  }
  for (uint32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++) /* Delete all the socket semaphores */
  {
    vSemaphoreDelete(p_net_ctx->Connection[i].DataAvailable);
  }

  /* Call lower-level deinit and free memory */
  W61_Net_DeInit(p_DrvObj); /* Deinitialize the Net context */
  vPortFree(p_net_ctx); /* Free the Net context */
  p_DrvObj = NULL; /* Reset the global pointer */
  p_net_ctx = NULL;
}

W6X_Status_t W6X_Net_SetHostname(uint8_t Hostname[33])
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_WiFi_StaStateType_e wifi_station_state = W6X_WIFI_STATE_STA_OFF;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the station state. The hostname cannot be set if the Wi-Fi station is off */
  if (W6X_WiFi_Station_GetState(&wifi_station_state, NULL) != W6X_STATUS_OK)
  {
    NET_LOG_ERROR("Could not get the station state\n");
    return ret;
  }
  /* Check the station state */
  if (wifi_station_state == W6X_WIFI_STATE_STA_OFF)
  {
    NET_LOG_ERROR("Device is not in the appropriate state to run this command\n");
    return ret;
  }

  /* Set the host name */
  return TranslateErrorStatus(W61_Net_SetHostname(p_DrvObj, Hostname));
}

W6X_Status_t W6X_Net_GetHostname(uint8_t Hostname[33])
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_WiFi_StaStateType_e wifi_station_state = W6X_WIFI_STATE_STA_OFF;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the station state. The host name cannot be get if the Wi-Fi station is off */
  if (W6X_WiFi_Station_GetState(&wifi_station_state, NULL) != W6X_STATUS_OK)
  {
    NET_LOG_ERROR("Could not get the station state\n");
    return ret;
  }
  /* Check the station state */
  if (wifi_station_state == W6X_WIFI_STATE_STA_OFF)
  {
    NET_LOG_ERROR("Device is not in the appropriate state to run this command\n");
    return ret;
  }

  /* Get the host name */
  return TranslateErrorStatus(W61_Net_GetHostname(p_DrvObj, Hostname));
}

W6X_Status_t W6X_Net_Station_GetIPAddress(uint8_t Ip_addr[4], uint8_t Gateway_addr[4], uint8_t Netmask_addr[4])
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_WiFi_StaStateType_e wifi_station_state = W6X_WIFI_STATE_STA_OFF;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the station state. The IP address cannot be get if the Wi-Fi station is off */
  if (W6X_WiFi_Station_GetState(&wifi_station_state, NULL) != W6X_STATUS_OK)
  {
    NET_LOG_ERROR("Could not get the station state\n");
    return ret;
  }
  /* Check if station is connected */
  if (!((wifi_station_state == W6X_WIFI_STATE_STA_CONNECTED) || (wifi_station_state == W6X_WIFI_STATE_STA_GOT_IP)))
  {
    NET_LOG_ERROR("Station is not connected. Connect to an Access Point before querying IPs\n");
    return ret;
  }

  /* Get the IP address of the Station */
  ret = TranslateErrorStatus(W61_Net_Station_GetIPAddress(p_DrvObj));
  if (ret == W6X_STATUS_OK)
  {
    /* Copy the IP address information */
    memcpy(Ip_addr, p_DrvObj->NetCtx.Net_sta_info.IP_Addr, 4);
    memcpy(Gateway_addr, p_DrvObj->NetCtx.Net_sta_info.Gateway_Addr, 4);
    memcpy(Netmask_addr, p_DrvObj->NetCtx.Net_sta_info.IP_Mask, 4);
  }
  return ret;
}

#if (W6X_NET_IPV6_ENABLE == 1)
W6X_Status_t W6X_Net_Station_GetIPv6Address(ip6_addr_t *Ip6ll_addr, ip6_addr_t *Ip6gl_addr)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_WiFi_StaStateType_e wifi_station_state = W6X_WIFI_STATE_STA_OFF;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the station state. The IP address cannot be get if the Wi-Fi station is off */
  if (W6X_WiFi_Station_GetState(&wifi_station_state, NULL) != W6X_STATUS_OK)
  {
    NET_LOG_ERROR("Could not get the station state\n");
    return ret;
  }
  /* Check if station is connected */
  if (!((wifi_station_state == W6X_WIFI_STATE_STA_CONNECTED) || (wifi_station_state == W6X_WIFI_STATE_STA_GOT_IP)))
  {
    NET_LOG_ERROR("Station is not connected. Connect to an Access Point before querying IPs\n");
    return ret;
  }

  /* Get the IP address of the Station */
  ret = TranslateErrorStatus(W61_Net_Station_GetIPAddress(p_DrvObj));
  if (ret == W6X_STATUS_OK)
  {
    /* Copy host-order 32-bit words directly if parameter is not NULL */
    if (Ip6ll_addr != NULL)
    {
      for (uint8_t i = 0; i < 4; ++i)
      {
        Ip6ll_addr->addr[i] = p_DrvObj->NetCtx.Net_sta_info.IP6_LinkLocal[i];
      }
    }
    if (Ip6gl_addr != NULL)
    {
      for (uint8_t i = 0; i < 4; ++i)
      {
        Ip6gl_addr->addr[i] = p_DrvObj->NetCtx.Net_sta_info.IP6_Global[i];
      }
    }
  }
  return ret;
}
#endif /* W6X_NET_IPV6_ENABLE */

W6X_Status_t W6X_Net_Station_SetIPAddress(uint8_t Ipaddr[4], uint8_t Gateway_addr[4], uint8_t Netmask_addr[4])
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_WiFi_StaStateType_e wifi_station_state = W6X_WIFI_STATE_STA_OFF;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the station state. The IP address cannot be set if the Wi-Fi station is off */
  if (W6X_WiFi_Station_GetState(&wifi_station_state, NULL) != W6X_STATUS_OK)
  {
    NET_LOG_ERROR("Could not get the station state\n");
    return ret;
  }
  /* Check if station is connected */
  if (!((wifi_station_state == W6X_WIFI_STATE_STA_CONNECTED) || (wifi_station_state == W6X_WIFI_STATE_STA_GOT_IP)))
  {
    NET_LOG_ERROR("Station is not connected. Connect to an Access Point before setting IPs\n");
    return ret;
  }

  /* Set the IP address of the Station */
  return TranslateErrorStatus(W61_Net_Station_SetIPAddress(p_DrvObj, Ipaddr, Gateway_addr, Netmask_addr));
}

W6X_Status_t W6X_Net_AP_GetIPAddress(uint8_t Ipaddr[4], uint8_t Netmask_addr[4])
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the IP address of the Soft-AP */
  ret = TranslateErrorStatus(W61_Net_AP_GetIPAddress(p_DrvObj));
  if (ret == W6X_STATUS_OK)
  {
    memcpy(Ipaddr, p_DrvObj->NetCtx.Net_ap_info.IP_Addr, 4);
    memcpy(Netmask_addr, p_DrvObj->NetCtx.Net_ap_info.IP_Mask, 4);
  }
  return ret;
}

W6X_Status_t W6X_Net_AP_SetIPAddress(uint8_t Ipaddr[4], uint8_t Netmask_addr[4])
{
  uint8_t ip_addr[4] = {0};
  uint8_t gateway_addr[4] = {0};
  uint8_t netmask_addr[4] = {0};
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Check the AP is not started */
  if (p_DrvObj->WifiCtx.ApState == W61_WIFI_STATE_AP_RUNNING)
  {
    NET_LOG_ERROR("Soft-AP is running. Stop it before setting the IP address\n");
    return W6X_STATUS_ERROR;
  }

  /* If the station is connected, check that the station and Soft-AP do not have the same subnet IP */
  if (p_DrvObj->WifiCtx.StaState == W61_WIFI_STATE_STA_GOT_IP)
  {
    /* Get the IP address of the Station */
    if (W6X_Net_Station_GetIPAddress(ip_addr, gateway_addr, netmask_addr) != W6X_STATUS_OK)
    {
      LogInfo("Could not get Station IP address\n");
    }

    /* Check if the station and Soft-AP have the same subnet IP */
    if (memcmp(gateway_addr, Ipaddr, 4) == 0)
    {
      LogWarn("Station and Soft-AP have the same subnet IP, Soft-AP IP need to be changed\n");
      return W6X_STATUS_ERROR;
    }
  }

  /* Set the IP address of the Soft-AP */
  return TranslateErrorStatus(W61_Net_AP_SetIPAddress(p_DrvObj, Ipaddr, Netmask_addr));
}

W6X_Status_t W6X_Net_GetDnsAddress(uint8_t Dns1_addr[4], uint8_t Dns2_addr[4], uint8_t Dns3_addr[4])
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the DNS status and DNS addresses */
  ret = TranslateErrorStatus(W61_Net_GetDnsAddress(p_DrvObj));
  if (ret == W6X_STATUS_OK)
  {
    memcpy(Dns1_addr, p_DrvObj->NetCtx.Net_sta_info.DNS1, 4);
    memcpy(Dns2_addr, p_DrvObj->NetCtx.Net_sta_info.DNS2, 4);
    memcpy(Dns3_addr, p_DrvObj->NetCtx.Net_sta_info.DNS3, 4);
  }
  else
  {
    NET_LOG_ERROR("Station is not connected. Connect to an Access Point before querying DNS IPs\n");
  }
  return ret;
}

W6X_Status_t W6X_Net_SetDnsAddress(uint8_t Dns1_addr[4], uint8_t Dns2_addr[4], uint8_t Dns3_addr[4])
{
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Set the DNS mode and DNS addresses */
  return TranslateErrorStatus(W61_Net_SetDnsAddress(p_DrvObj, Dns1_addr, Dns2_addr, Dns3_addr));
}

W6X_Status_t W6X_Net_GetDhcp(W6X_Net_DhcpType_e *State, uint32_t *lease_time, uint8_t start_ip[4], uint8_t end_ip[4])
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the DHCP server configuration */
  ret = TranslateErrorStatus(W61_Net_GetDhcpsConfig(p_DrvObj, lease_time, start_ip, end_ip));
  if (ret == W6X_STATUS_OK)
  {
    /* Get the DHCP configuration */
    ret = TranslateErrorStatus(W61_Net_GetDhcpConfig(p_DrvObj, (W61_Net_DhcpType_e *)State));
  }
  return ret;
}

W6X_Status_t W6X_Net_SetDhcp(W6X_Net_DhcpType_e *State, uint32_t *Operate, uint32_t lease_time)
{
  W6X_Status_t ret;
  W61_WiFi_ApStateType_e previous_ap_state = p_DrvObj->WifiCtx.ApState;
  uint8_t Reconnect = 1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Set the DHCP global configuration */
  ret = TranslateErrorStatus(W61_Net_SetDhcpConfig(p_DrvObj, (W61_Net_DhcpType_e *)State, Operate));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  if ((*Operate == 1) && ((*State == W6X_NET_DHCP_AP_ENABLED) || (*State == W6X_NET_DHCP_STA_AP_ENABLED)))
  {
    /* Need to switch the Soft-AP OFF before modifying DHCPS configuration */
    if (previous_ap_state == W61_WIFI_STATE_AP_RUNNING)
    {
      /* Stop the Soft-AP. The current configuration will be kept */
      ret = TranslateErrorStatus(W61_WiFi_AP_Stop(p_DrvObj, Reconnect));
      if (ret != W6X_STATUS_OK)
      {
        goto _err;
      }
    }

    /* Set the DHCP server configuration */
    ret = TranslateErrorStatus(W61_Net_SetDhcpsConfig(p_DrvObj, lease_time));
    if (ret != W6X_STATUS_OK)
    {
      NET_LOG_ERROR("DHCPS config set failed\n");
      goto _err;
    }

    if (previous_ap_state == W61_WIFI_STATE_AP_RUNNING)
    {
      /* Restart the Soft-AP with the previous configuration */
      ret = TranslateErrorStatus(W61_WiFi_SetDualMode(p_DrvObj));
      if (ret != W6X_STATUS_OK)
      {
        NET_LOG_ERROR("Soft-AP failed to restart after DHCPS config change\n");
      }
    }
  }

_err:
  return ret;
}

W6X_Status_t W6X_Net_Ping(uint8_t *location, uint16_t length, uint16_t count, uint16_t interval_ms,
                          uint16_t timeout, uint32_t *average_time, uint16_t *received_response)
{
  W6X_Status_t ret;
  W61_Net_PingResult_t ping_result;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Execute a ping process */
  ret = TranslateErrorStatus(W61_Net_Ping(p_DrvObj, (char *)location, length,
                                          count, interval_ms, timeout, &ping_result));
  if (ret == W6X_STATUS_OK)
  {
    *average_time = ping_result.average_time;
    *received_response = ping_result.response_count;
  }
  return ret;
}

W6X_Status_t W6X_Net_ResolveHostAddress(const char *location, uint8_t *ipaddr)
{
  ip_addr_t temp;
  W6X_Status_t ret;
  /* Use new resolver API for an IPv4 address */
  ret = W6X_Net_ResolveHostAddressByType(location,  &temp, W6X_NET_DNS_ADDRTYPE_IPV4);
  if (ret != W6X_STATUS_OK)
  {
    return ret;
  }
  /* Sanity: ensure we really got an IPv4 address */
  if (temp.type != W6X_NET_IPV4)
  {
    return W6X_STATUS_ERROR;
  }
  /* Copy binary IPv4 (network order) into legacy uint8_t[4] buffer */
  for (uint32_t i = 0; i < 4; i++)
  {
    ipaddr[i] = ((uint8_t *)&temp.u_addr.ip4.addr)[i];
  }
  return W6X_STATUS_OK;
}

W6X_Status_t W6X_Net_ResolveHostAddressByType(const char *location, ip_addr_t *out_addr, uint8_t family)
{
  W61_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(location, W6X_Location_Null_str);

  W61_Net_Ip_addr_t ip_addr_struct;
  W61_Net_Dns_ResolveType_e at_family;

  /* Depending on request DNS address type, the W61 AT parameter will be adjusted */
  if (family == W6X_NET_DNS_ADDRTYPE_IPV4)
  {
    at_family = W61_NET_DNS_IPV4;
  }
#if (W6X_NET_IPV6_ENABLE == 1)
  else if (family == W6X_NET_DNS_ADDRTYPE_IPV6)
  {
    at_family = W61_NET_DNS_IPV6;
  }
#endif /* W6X_NET_IPV6_ENABLE */
  else
  {
    at_family = W61_NET_DNS_IPV4_IPV6;
  }
  ret = W61_Net_ResolveHostAddress(p_DrvObj, location, &ip_addr_struct, &at_family);
  if (ret != W61_STATUS_OK)
  {
    return TranslateErrorStatus(ret);
  }
  /* Copy in the ip_addr_t the IP address obtained and update the family type*/
  if (at_family == W61_NET_DNS_IPV4)
  {
    memcpy(&out_addr->u_addr.ip4.addr, &ip_addr_struct.u_addr.ip4.addr, sizeof(ip_addr_struct.u_addr.ip4.addr));
    out_addr->type = W6X_NET_IPV4;
  }
#if  W6X_NET_IPV6_ENABLE
  else
  {
    memcpy(&out_addr->u_addr.ip6.addr[0], &ip_addr_struct.u_addr.ip6.addr[0], sizeof(ip_addr_struct.u_addr.ip6.addr));
    out_addr->type = W6X_NET_IPV6;
  }
#endif /* W6X_NET_IPV6_ENABLE */
  return W6X_STATUS_OK;
}

W6X_Status_t W6X_Net_SNTP_GetConfiguration(uint8_t *Enable, int16_t *Timezone, uint8_t *SntpServer1,
                                           uint8_t *SntpServer2, uint8_t *SntpServer3)
{
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the SNTP configuration */
  return TranslateErrorStatus(W61_Net_SNTP_GetConfiguration(p_DrvObj, Enable, Timezone, SntpServer1,
                                                            SntpServer2, SntpServer3));
}

W6X_Status_t W6X_Net_SNTP_SetConfiguration(uint8_t Enable, int16_t Timezone, uint8_t *SntpServer1,
                                           uint8_t *SntpServer2, uint8_t *SntpServer3)
{
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Set the SNTP configuration */
  return TranslateErrorStatus(W61_Net_SNTP_SetConfiguration(p_DrvObj, Enable, Timezone, SntpServer1,
                                                            SntpServer2, SntpServer3));
}

W6X_Status_t W6X_Net_SNTP_GetInterval(uint16_t *Interval)
{
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the time interval to refresh the SNTP time */
  return TranslateErrorStatus(W61_Net_SNTP_GetInterval(p_DrvObj, Interval));
}

W6X_Status_t W6X_Net_SNTP_SetInterval(uint16_t Interval)
{
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Set the time interval to refresh the SNTP time */
  return TranslateErrorStatus(W61_Net_SNTP_SetInterval(p_DrvObj, Interval));
}

W6X_Status_t W6X_Net_SNTP_GetTime(W6X_Net_Time_t *Time)
{
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the current time */
  return TranslateErrorStatus(W61_Net_SNTP_GetTime(p_DrvObj, (W61_Net_Time_t *)Time));
}

W6X_Status_t W6X_Net_GetConnectionStatus(uint8_t Socket, uint8_t *Protocol, uint32_t *RemoteIp, uint32_t *RemotePort,
                                         uint32_t *LocalPort, uint8_t *Type)
{
  W6X_Status_t ret;
  W61_Net_Connection_t conn;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);

  /* Get the socket connection status */
  ret = TranslateErrorStatus(W61_Net_GetSocketInformation(p_DrvObj, Socket, &conn));
  if (ret != W6X_STATUS_OK)
  {
    NET_LOG_ERROR("Could not get socket information\n");
    return ret;
  }
  *Protocol = conn.Protocol;
  *RemotePort = conn.RemotePort;
  *LocalPort = conn.LocalPort;
  /* Convert the IP address string to binary format */
  (void)W6X_Net_Inet_pton(AF_INET, (char *)&conn.RemoteIP, RemoteIp);
  *Type = conn.TeType;

  return ret;
}

int32_t W6X_Net_Socket(int32_t family, int32_t type, int32_t proto)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (family != AF_INET
#if (W6X_NET_IPV6_ENABLE == 1)
      && family != AF_INET6
#endif /* W6X_NET_IPV6_ENABLE */
     ) /* IPv4 or IPv6 */
  {
    return ret;
  }

  for (int32_t i = 0; i < (W61_NET_MAX_CONNECTIONS + 1); i++) /* Find an available socket */
  {
    int32_t socket_to_use = (i + p_net_ctx->NextSocketToUse) % (W61_NET_MAX_CONNECTIONS + 1);
    if (p_net_ctx->Sockets[socket_to_use].Status == W6X_NET_SOCKET_RESET) /* Check if the socket is available */
    {
      memset(&p_net_ctx->Sockets[socket_to_use], 0, sizeof(W6X_Net_Socket_t)); /* Reset the socket */
      if (family == AF_INET)
      {
        p_net_ctx->Sockets[socket_to_use].Family = W6X_NET_IPV4;
      }
#if (W6X_NET_IPV6_ENABLE == 1)
      else
      {
        p_net_ctx->Sockets[socket_to_use].Family = W6X_NET_IPV6;
      }
#endif /* W6X_NET_IPV6_ENABLE */
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
      p_net_ctx->Sockets[socket_to_use].Status = W6X_NET_SOCKET_ALLOCATED;
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
  if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_CONNECTED)
  {
    conn.Number = p_net_ctx->Sockets[sock].Number;
    /* Set the socket status to closing to terminate remaining process */
    p_net_ctx->Sockets[sock].Status = W6X_NET_SOCKET_CLOSING;

    /* Stop the connection */
    if (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].SocketConnected == 1)
    {
      taskENTER_CRITICAL();
      p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].DataAvailableSize = 0;
      taskEXIT_CRITICAL();
      if (W61_Net_StopClientConnection(p_DrvObj, &conn) != W61_STATUS_OK)
      {
        return ret;
      }
    }
    else
    {
      taskENTER_CRITICAL();
      p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].DataAvailableSize = 0;
      taskEXIT_CRITICAL();
      W6X_Net_Clean_Socket(conn.Number);
    }
  }

  return 0;
}

int32_t W6X_Net_Shutdown(int32_t sock, int32_t close_connection)
{
  int32_t ret = 0;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (p_net_ctx->Sockets[sock].IsConnected == 1) /* Check if the socket is a server */
  {
    NET_LOG_WARN("Given socket is a connection not a server\n");
    return ret;
  }

  /* Set the socket status to closing to terminate remaining process */
  p_net_ctx->Sockets[sock].Status = W6X_NET_SOCKET_CLOSING;

  if (close_connection == 1) /* Requested to close all pending connections */
  {
    for (int32_t i = 0; i <  W61_NET_MAX_CONNECTIONS + 1; i++)
    {
      /* Check if the socket is used */
      if ((p_net_ctx->Sockets[i].Status != W6X_NET_SOCKET_RESET) && (i != sock) && (p_net_ctx->Sockets[i].Client == 0))
      {
        ret = W6X_Net_Close(i);
        if (ret != 0) /* Close the connection */
        {
          NET_LOG_ERROR("Could not close connection %" PRIu16 "\n", p_net_ctx->Sockets[i].Number);
        }
      }
    }
  }

  if (ret != 0)
  {
    return ret;
  }

  /* Stop the server */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_StopServer(p_DrvObj, close_connection));
  if (ret != 0)
  {
    if (ret != -2) /* BUSY */
    {
      NET_LOG_ERROR("Failed to stop the server\n");
    }
    return ret;
  }

  p_net_ctx->Sockets[sock].Status = W6X_NET_SOCKET_RESET; /* Set the socket status to reset */

  return 0;
}

int32_t W6X_Net_Bind(int32_t sock, const struct sockaddr *addr, socklen_t addrlen)
{
  int32_t ret = -1;
  struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  /* Check if the socket is allocated */
  if (p_net_ctx->Sockets[sock].Status != W6X_NET_SOCKET_ALLOCATED)
  {
    NET_LOG_ERROR("Wrong Unexpected socket status: %" PRIi32 " status is %" PRIi32 "\n",
                  sock, (int32_t)p_net_ctx->Sockets[sock].Status);
    return ret;
  }

  p_net_ctx->Sockets[sock].IsConnected = 0; /* Set the socket as a server */
  p_net_ctx->Sockets[sock].LocalPort = PP_NTOHS(addr_in->sin_port); /* Set the local port */
  p_net_ctx->Sockets[sock].Status = W6X_NET_SOCKET_BIND; /* Set the socket status to bind */

  return 0;
}

int32_t W6X_Net_Connect(int32_t sock, const struct sockaddr *addr, socklen_t addrlen)
{
  struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
#if (W6X_NET_IPV6_ENABLE == 1)
  struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;
#endif /* W6X_NET_IPV6_ENABLE */
  W61_Net_Connection_t conn;
  int32_t conn_to_use = -1;
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  if (p_net_ctx->Sockets[sock].Status != W6X_NET_SOCKET_ALLOCATED) /* Check if the socket is allocated */
  {
    return ret;
  }

  p_net_ctx->Sockets[sock].IsConnected = 1; /* Set the socket as a client */
  p_net_ctx->Sockets[sock].Client = 1;

  /* Set the remote port and IP address */
  if (addr_in->sin_family == AF_INET)
  {
    p_net_ctx->Sockets[sock].RemotePort = PP_NTOHS(addr_in->sin_port);
    p_net_ctx->Sockets[sock].RemoteIP[0] = addr_in->sin_addr.s_addr;
  }
#if  W6X_NET_IPV6_ENABLE
  else
  {
    p_net_ctx->Sockets[sock].RemotePort = PP_NTOHS(addr_in6->sin6_port);
    memcpy(p_net_ctx->Sockets[sock].RemoteIP, addr_in6->sin6_addr.un.u32_addr,
           sizeof(p_net_ctx->Sockets[sock].RemoteIP));
  }
#endif /* W6X_NET_IPV6_ENABLE */

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
        if ((p_net_ctx->Sockets[j].Number == i) && (p_net_ctx->Sockets[j].Status != W6X_NET_SOCKET_RESET))
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
  p_net_ctx->Sockets[sock].Number = conn_to_use;  /* Set the connection number */

  /* Set the connection parameters */
  conn.Number = p_net_ctx->Sockets[sock].Number;
  conn.RemotePort = p_net_ctx->Sockets[sock].RemotePort;
  switch (p_net_ctx->Sockets[sock].Protocol)
  {
    case W6X_NET_TCP_PROTOCOL:
      if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV4 && addr_in->sin_family == AF_INET)
      {
        conn.Protocol = W61_NET_TCP_CONNECTION;
      }
#if (W6X_NET_IPV6_ENABLE == 1)
      else if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV6 && addr_in6->sin6_family == AF_INET6)
      {
        conn.Protocol = W61_NET_TCPV6_CONNECTION;
      }
#endif /* W6X_NET_IPV6_ENABLE */
      else
      {
        LogError("IP address family mismatch");
        return ret;
      }
      break;
    case W6X_NET_UDP_PROTOCOL:
      if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV4 && addr_in->sin_family == AF_INET)
      {
        conn.Protocol = W61_NET_UDP_CONNECTION;
      }
#if (W6X_NET_IPV6_ENABLE == 1)
      else if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV6 && addr_in6->sin6_family == AF_INET6)
      {
        conn.Protocol = W61_NET_UDPV6_CONNECTION;
      }
#endif /* W6X_NET_IPV6_ENABLE */
      else
      {
        LogError("IP address family mismatch");
        return ret;
      }
      break;
    case W6X_NET_SSL_PROTOCOL:
      if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV4 && addr_in->sin_family == AF_INET)
      {
        conn.Protocol = W61_NET_TCP_SSL_CONNECTION;
      }
#if (W6X_NET_IPV6_ENABLE == 1)
      else if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV6 && addr_in6->sin6_family == AF_INET6)
      {
        conn.Protocol = W61_NET_TCPV6_SSL_CONNECTION;
      }
#endif /* W6X_NET_IPV6_ENABLE */
      else
      {
        LogError("IP address family mismatch");
        return ret;
      }
      int32_t auth_mode = 0; /* Set the authentication mode. Can be client, server or both */
      if (p_net_ctx->Sockets[sock].Ca_Cert != NULL) /* Check if the CA certificate is used */
      {
        auth_mode += 2; /* Server authentication */
      }
      /* Check if the client certificate and private key are used */
      if ((p_net_ctx->Sockets[sock].Certificate != NULL) && (p_net_ctx->Sockets[sock].Private_Key != NULL))
      {
        auth_mode++; /* Client authentication */
      }
      ret = W6X_Net_TranslateErrorStatus(W61_Net_SSL_SetConfiguration(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                      auth_mode,
                                                                      (uint8_t *)p_net_ctx->Sockets[sock].Certificate,
                                                                      (uint8_t *)p_net_ctx->Sockets[sock].Private_Key,
                                                                      (uint8_t *)p_net_ctx->Sockets[sock].Ca_Cert));
      if (ret != 0)
      {
        if (ret != -2) /* BUSY */
        {
          NET_LOG_ERROR("Could not set SSL Client configuration\n");
        }
        return ret;
      }

      if (auth_mode == 0) /* Setup PSK if no certificate is used */
      {
        if (p_net_ctx->Sockets[sock].PSK != NULL && p_net_ctx->Sockets[sock].PSK_Identity != NULL)
        {
          /* Set the PSK configuration */
          ret = W6X_Net_TranslateErrorStatus(W61_Net_SSL_SetPSK(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                (uint8_t *)p_net_ctx->Sockets[sock].PSK,
                                                                (uint8_t *)p_net_ctx->Sockets[sock].PSK_Identity));
          if (ret != 0)
          {
            if (ret != -2) /* BUSY */
            {
              NET_LOG_ERROR("Could not set SSL Client PSK configuration\n");
            }
            return ret;
          }
        }
      }

      /* Set the ALPN */
      ret = W6X_Net_TranslateErrorStatus(W61_Net_SSL_SetALPN(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                             (uint8_t *)p_net_ctx->Sockets[sock].ALPN[0],
                                                             (uint8_t *)p_net_ctx->Sockets[sock].ALPN[1],
                                                             (uint8_t *)p_net_ctx->Sockets[sock].ALPN[2]));
      if (ret != 0)
      {
        if (ret != -2) /* BUSY */
        {
          NET_LOG_ERROR("Could not set SSL ALPN\n");
        }
        return ret;
      }

      /* Set the Server Name Indication */
      ret = W6X_Net_TranslateErrorStatus(W61_Net_SSL_SetServerName(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                   (uint8_t *)p_net_ctx->Sockets[sock].SNI));
      if (ret != 0)
      {
        if (ret != -2) /* BUSY */
        {
          NET_LOG_ERROR("Could not set SSL ALPN\n");
        }
        return ret;
      }
      break;
  }

  /* Set the remote IP address */
  if (addr_in->sin_family == AF_INET)
  {
    W6X_Net_Inet_ntop(addr_in->sin_family, &p_net_ctx->Sockets[sock].RemoteIP, conn.RemoteIP, sizeof(conn.RemoteIP));
  }
#if (W6X_NET_IPV6_ENABLE == 1)
  else
  {
    W6X_Net_Inet_ntop(addr_in6->sin6_family, &p_net_ctx->Sockets[sock].RemoteIP, conn.RemoteIP, sizeof(conn.RemoteIP));
  }
#endif /* W6X_NET_IPV6_ENABLE */
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
      NET_LOG_ERROR("Could not set TCP options\n");
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
      NET_LOG_ERROR("Could not set Receive buffer size\n");
    }
    return ret;
  }

  /* Start the connection */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_StartClientConnection(p_DrvObj, &conn));
  if (ret != 0)
  {
    if (ret != -2) /* BUSY */
    {
      NET_LOG_ERROR("Could not connect\n");
    }
    return ret;
  }

  if (p_net_ctx->Connection[conn_to_use].SocketConnected == 0) /* Normally it arrives before SEND OK msg */
  {
    NET_LOG_DEBUG("Wait a bit more Connect Callback\n");
    vTaskDelay(10);
    if (p_net_ctx->Connection[conn_to_use].SocketConnected == 0) /* To check if semaphore if needed */
    {
      NET_LOG_ERROR("Could not connect\n");
      return ret;
    }
  }

  p_net_ctx->Sockets[sock].Status = W6X_NET_SOCKET_CONNECTED; /* Set the socket status to connected */

  return 0;
}

int32_t W6X_Net_Listen(int32_t sock, int32_t backlog)
{
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  /* Check if the socket is a server and in bind mode */
  if ((p_net_ctx->Sockets[sock].IsConnected != 0) || (p_net_ctx->Sockets[sock].Status != W6X_NET_SOCKET_BIND))
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
  /* Translate the protocol */
  W61_Net_Protocol_e protocol = W61_NET_TCP_CONNECTION;
  switch (p_net_ctx->Sockets[sock].Protocol)
  {
    case W6X_NET_TCP_PROTOCOL:
      if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV4)
      {
        protocol = W61_NET_TCP_CONNECTION;
      }
#if (W6X_NET_IPV6_ENABLE == 1)
      else
      {
        protocol = W61_NET_TCPV6_CONNECTION;
      }
#endif /* W6X_NET_IPV6_ENABLE */
      break;
    case W6X_NET_UDP_PROTOCOL:
      if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV4)
      {
        protocol = W61_NET_UDP_CONNECTION;
      }
#if (W6X_NET_IPV6_ENABLE == 1)
      else
      {
        protocol = W61_NET_UDPV6_CONNECTION;
      }
#endif /* W6X_NET_IPV6_ENABLE */
      break;
    case W6X_NET_SSL_PROTOCOL:
      if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV4)
      {
        protocol = W61_NET_TCP_SSL_CONNECTION;
      }
#if (W6X_NET_IPV6_ENABLE == 1)
      else
      {
        protocol = W61_NET_TCPV6_SSL_CONNECTION;
      }
#endif /* W6X_NET_IPV6_ENABLE */
      break;
    default:
      break;
  }

  /* Start the server */
  ret = W6X_Net_TranslateErrorStatus(W61_Net_StartServer(p_DrvObj, p_net_ctx->Sockets[sock].LocalPort,
                                                         protocol, 0, p_net_ctx->Sockets[sock].SoKeepAlive));
  if (ret != 0)
  {
    return ret;
  }

  p_net_ctx->Sockets[sock].Status = W6X_NET_SOCKET_LISTENING; /* Set the socket status to listening */
  return 0;
}

int32_t W6X_Net_Accept(int32_t sock, struct sockaddr *addr, socklen_t *addrlen)
{
  int32_t type;
  int32_t proto;
  int32_t new_socket;
  int32_t ret = -1;
  W61_Net_Connection_t conn;
  int32_t family = AF_INET;
  struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
#if (W6X_NET_IPV6_ENABLE == 1)
  struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)addr;
#endif /* W6X_NET_IPV6_ENABLE */
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

#if (W6X_NET_IPV6_ENABLE == 1)
  if (p_net_ctx->Sockets[sock].Family == W6X_NET_IPV6)
  {
    family = AF_INET6;
  }
#endif /* W6X_NET_IPV6_ENABLE */
  /* Check if the socket is a server and in listening mode */
  if ((p_net_ctx->Sockets[sock].IsConnected != 0) || (p_net_ctx->Sockets[sock].Status != W6X_NET_SOCKET_LISTENING))
  {
    NET_LOG_ERROR("Socket is not a server in Listening\n");
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

  new_socket = W6X_Net_Socket(family, type, proto); /* Create a new socket */
  if (new_socket < 0)
  {
    NET_LOG_ERROR("Unable to create a new socket, limit reached\n");
    return ret;
  }

  while (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_LISTENING) /* Wait for a connection */
  {
    for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
    {
      int32_t found = 0;
      if (p_net_ctx->Connection[i].SocketConnected == 1) /* Check if the socket is connected */
      {
        for (int32_t j = 0; j < W61_NET_MAX_CONNECTIONS + 1; j++)
        {
          /* Check if the connected socket found is already in use */
          if ((p_net_ctx->Sockets[j].Number == i) && ((p_net_ctx->Sockets[j].Status == W6X_NET_SOCKET_CONNECTED) ||
                                                      (p_net_ctx->Sockets[j].Status == W6X_NET_SOCKET_CLOSING)))
          {
            found = 1;
            break;
          }
        }
        if (found == 0) /* If the connected socket is not in use */
        {
          ret = W6X_Net_TranslateErrorStatus(W61_Net_GetSocketInformation(p_DrvObj, i, &conn));
          if (ret == 0)
          {
            if (conn.TeType == 0) /* Server connection */
            {
              NET_LOG_ERROR("Found an unregistered connection that is not flagged as server connection\n");
              p_net_ctx->Sockets[new_socket].Status = W6X_NET_SOCKET_RESET;
              return -1;
            }
            if (family == AF_INET)
            {
              addr_in->sin_port = PP_HTONS(conn.RemotePort); /* Set the remote port */
              /* Convert the IP address to binary format */
              (void)W6X_Net_Inet_pton(family, conn.RemoteIP, (void *)&addr_in->sin_addr.s_addr);
            }
#if (W6X_NET_IPV6_ENABLE == 1)
            else
            {
              addr_in6->sin6_port = PP_HTONS(conn.RemotePort); /* Set the remote port */
              /* Convert the IP address to binary format */
              (void)W6X_Net_Inet_pton(family, conn.RemoteIP, (void *)addr_in6->sin6_addr.un.u32_addr);
            }
#endif /* W6X_NET_IPV6_ENABLE */
          }
          /* Set the new socket parameters */
          p_net_ctx->Sockets[new_socket].Number = i;
          p_net_ctx->Sockets[new_socket].IsConnected = 1;
          p_net_ctx->Sockets[new_socket].Client = 0;
          p_net_ctx->Sockets[new_socket].RecvTimeout = p_net_ctx->Sockets[sock].RecvTimeout;
          p_net_ctx->Sockets[new_socket].Status = W6X_NET_SOCKET_CONNECTED;
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
  /* Send data over a network socket, checking connection state and handling errors */
  uint32_t SentDataLen = 0;
  int32_t ret = -1;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);
  NULL_ASSERT(buf, W6X_Buf_Null_str);

  /* Check if the socket is connected */
  if ((p_net_ctx->Sockets[sock].Status != W6X_NET_SOCKET_CONNECTED) ||
      (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].SocketConnected == 0))
  {
    NET_LOG_ERROR("Socket state is not connected\n");
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

  if (p_net_ctx->Sockets[sock].Status != W6X_NET_SOCKET_CONNECTED) /* Check if the socket is connected */
  {
    NET_LOG_ERROR("Socket state is not connected\n");
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
    NET_LOG_ERROR("Pull data from socket failed\n");
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
  /* Support both IPv4 and IPv6. Allocate buffer large enough for IPv6 textual form */
  char ipaddr[INET6_ADDRSTRLEN] = {0};
  const sa_family_t fam = dest_addr->sa_family;
  const struct sockaddr_in  *addr_in  = (const struct sockaddr_in *)dest_addr;
#if (W6X_NET_IPV6_ENABLE == 1)
  const struct sockaddr_in6 *addr_in6 = (const struct sockaddr_in6 *)dest_addr;
#endif /* W6X_NET_IPV6_ENABLE */
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);
  NULL_ASSERT(buf, W6X_Buf_Null_str);

  if (p_net_ctx->Sockets[sock].Protocol != W6X_NET_UDP_PROTOCOL) /* Check if the socket protocol is UDP */
  {
    NET_LOG_ERROR("Socket protocol is not UDP\n");
    return ret;
  }

  if (fam == AF_INET)
  {
    p_net_ctx->Sockets[sock].RemotePort = PP_NTOHS(addr_in->sin_port);
    /* Convert IPv4 address (stored as uint32_t in sin_addr.s_addr) to dotted string */
    if (W6X_Net_Inet_ntop(AF_INET, (const void *)&addr_in->sin_addr.s_addr, ipaddr, INET_ADDRSTRLEN) == NULL)
    {
      NET_LOG_ERROR("Could not encode IPv4 address\n");
      return ret;
    }
    /* Store raw IPv4 in first word of RemoteIP (host order) */
    p_net_ctx->Sockets[sock].RemoteIP[0] = addr_in->sin_addr.s_addr;
  }
#if (W6X_NET_IPV6_ENABLE == 1)
  else if (fam == AF_INET6)
  {
    p_net_ctx->Sockets[sock].RemotePort = PP_NTOHS(addr_in6->sin6_port);
    /* Convert host-order 4 word IPv6 representation to compressed textual form */
    if (W6X_Net_Inet_ntop(AF_INET6, (const void *)addr_in6->sin6_addr.un.u32_addr, ipaddr, INET6_ADDRSTRLEN) == NULL)
    {
      NET_LOG_ERROR("Could not encode IPv6 address\n");
      return ret;
    }
    /* Store host-order words */
    memcpy(p_net_ctx->Sockets[sock].RemoteIP, addr_in6->sin6_addr.un.u32_addr,
           sizeof(p_net_ctx->Sockets[sock].RemoteIP));
  }
#endif /* W6X_NET_IPV6_ENABLE */
  else
  {
    NET_LOG_ERROR("Unsupported address family in sendto: %d\n", fam);
    return ret;
  }

  if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_ALLOCATED) /* Check if the socket is allocated */
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
      NET_LOG_ERROR("No connection available\n");
      return ret;
    }
    p_net_ctx->Sockets[sock].Number = conn_to_use;
    p_net_ctx->Connection[conn_to_use].SocketConnected = 1;
    p_net_ctx->Sockets[sock].Status = W6X_NET_SOCKET_CONNECTED;
  }

  if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_CONNECTED)
  {
    /* Send the data */
    ret = W6X_Net_TranslateErrorStatus(W61_Net_SendData_Non_Connected(p_DrvObj, p_net_ctx->Sockets[sock].Number,
                                                                      ipaddr, p_net_ctx->Sockets[sock].RemotePort,
                                                                      (uint8_t *)buf, (uint32_t)len, &SentDataLen,
                                                                      p_net_ctx->Sockets[sock].SoSndTimeo + 1000));
    if (ret == 0)
    {
      return (ssize_t) SentDataLen;
    }
  }
  else if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_LISTENING)
  {
    /* Check if the socket is a server */
    connection_id = W6X_Net_Find_Connection(ipaddr, p_net_ctx->Sockets[sock].RemotePort);
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
  TickType_t startTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
  TickType_t currentTime;
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);
  NULL_ASSERT(buf, W6X_Buf_Null_str);

  if (p_net_ctx->Sockets[sock].Protocol != W6X_NET_UDP_PROTOCOL) /* Check if the socket protocol is UDP */
  {
    NET_LOG_ERROR("Socket protocol is not UDP\n");
    return ret;
  }

  /* Cannot receive data on a socket that is not connected */
  if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_ALLOCATED)
  {
    return ret;
  }

  /* If the socket is connected, receive data */
  if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_CONNECTED)
  {
    return W6X_Net_Recv(sock, buf, max_len, flags); /* Receive data */
  }

  /* If the socket is a server, move to listening mode */
  if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_BIND)
  {
    if (p_net_ctx->Sockets[sock].Protocol == W6X_NET_UDP_PROTOCOL)
    {
      if (W6X_Net_Listen(sock, 0) != 0) /* Start listening */
      {
        return ret;
      }
    }
  }

  /* If the socket is a server, receive data from the client */
  if (p_net_ctx->Sockets[sock].Status == W6X_NET_SOCKET_LISTENING)
  {
    while (1)
    {
      udp_connection = W6X_Net_Poll_UDP_Sockets(src_addr); /* Poll the UDP sockets */
      if (udp_connection != -1)
      {
        break;
      }

      vTaskDelay(1); /* Wait a bit before polling again */
      currentTime = xPortIsInsideInterrupt() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
      /* Check if time to receive data has elapsed */
      if ((currentTime - startTime) > (TickType_t) p_net_ctx->Sockets[sock].RecvTimeout)
      {
        NET_LOG_ERROR("No data received within %" PRIu32 " s\n", p_net_ctx->Sockets[sock].RecvTimeout / 1000);
        return 0;
      }
    }

    ret = W6X_Net_Wait_Pull_Data(sock, udp_connection, buf, max_len); /* Pull data from the socket */
    if ((ret < 0) && (ret != -2))
    {
      NET_LOG_ERROR("Pull data from socket failed\n");
    }
    else if (ret == 0)
    {
      /* Check if the socket is still connected */
      NET_LOG_DEBUG("No Data available to read in connection %" PRIi32 "\n", udp_connection);
    }
  }
  else
  {
    NET_LOG_DEBUG("No Data available to read in socket %" PRIu16 "\n", p_net_ctx->Sockets[sock].Number);
    return 0;
  }

  return (ssize_t)ret; /* Return the error or the number of bytes read */
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
  if (p_net_ctx->Sockets[sock].Status != W6X_NET_SOCKET_ALLOCATED)
  {
    if ((level == SOL_TLS) || ((optname != SO_SNDTIMEO) && (optname != SO_RCVTIMEO)))
    {
      /* Only Timeouts can be set once server or client is started */
      NET_LOG_ERROR("Socket has already been started\n");
      return ret;
    }
  }
  if (level == SOL_SOCKET) /* Check if the legacy socket options are being set */
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
      case SO_RCVBUF: /* Set the Receive buffer length */
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
  /* Check if the socket option is for TLS */
  else if ((level == SOL_TLS) && (p_net_ctx->Sockets[sock].Protocol == W6X_NET_SSL_PROTOCOL))
  {
    switch (optname)
    {
      case TLS_SEC_TAG_LIST: /* Store the certificates, keys filenames or PSK identifiers */
        for (uint8_t i = 0; i < optlen; i++)
        {
          char *credential = p_net_ctx->Credentials[tags[i]].name;
          if (credential == NULL) /* Check if the credential is valid */
          {
            NET_LOG_ERROR("Invalid TLS credential\n");
            return -1;
          }
          uint32_t credential_len = strlen(credential) + 1;

          switch (p_net_ctx->Credentials[tags[i]].type)
          {
            case W6X_NET_TLS_CREDENTIAL_CA_CERTIFICATE:
              if (p_net_ctx->Sockets[sock].Ca_Cert != NULL) /* Check if the CA Certificate is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].Ca_Cert);
              }

              p_net_ctx->Sockets[sock].Ca_Cert = (char *) pvPortMalloc(credential_len);
              if (p_net_ctx->Sockets[sock].Ca_Cert == NULL)
              {
                NET_LOG_ERROR("Malloc failed\n");
                return -1;
              }
              /* Store the CA Certificate filename */
              strncpy(p_net_ctx->Sockets[sock].Ca_Cert, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_SERVER_CERTIFICATE:
              if (p_net_ctx->Sockets[sock].Certificate != NULL) /* Check if the Certificate is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].Certificate);
              }

              p_net_ctx->Sockets[sock].Certificate = (char *) pvPortMalloc(credential_len);
              if (p_net_ctx->Sockets[sock].Certificate == NULL)
              {
                NET_LOG_ERROR("Malloc failed\n");
                return -1;
              }
              /* Store the Certificate filename */
              strncpy(p_net_ctx->Sockets[sock].Certificate, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_PRIVATE_KEY:
              if (p_net_ctx->Sockets[sock].Private_Key != NULL) /* Check if the Private Key is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].Private_Key);
              }

              p_net_ctx->Sockets[sock].Private_Key = (char *) pvPortMalloc(credential_len);
              if (p_net_ctx->Sockets[sock].Private_Key == NULL)
              {
                NET_LOG_ERROR("Malloc failed\n");
                return -1;
              }
              /* Store the Private Key filename */
              strncpy(p_net_ctx->Sockets[sock].Private_Key, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_PSK:
              if (p_net_ctx->Sockets[sock].PSK != NULL) /* Check if the PSK is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].PSK);
              }

              p_net_ctx->Sockets[sock].PSK = (char *) pvPortMalloc(credential_len);
              if (p_net_ctx->Sockets[sock].PSK == NULL)
              {
                NET_LOG_ERROR("Malloc failed\n");
                return -1;
              }
              /* Store the PSK key string */
              strncpy(p_net_ctx->Sockets[sock].PSK, credential, credential_len);
              break;
            case W6X_NET_TLS_CREDENTIAL_PSK_ID:
              if (p_net_ctx->Sockets[sock].PSK_Identity != NULL) /* Check if the PSK Identity is already set */
              {
                vPortFree(p_net_ctx->Sockets[sock].PSK_Identity);
              }

              p_net_ctx->Sockets[sock].PSK_Identity = (char *) pvPortMalloc(credential_len);
              if (p_net_ctx->Sockets[sock].PSK_Identity == NULL)
              {
                NET_LOG_ERROR("Malloc failed\n");
                return -1;
              }
              /* Store the PSK Identity (hint) */
              strncpy(p_net_ctx->Sockets[sock].PSK_Identity, credential, credential_len);
              break;
            default:
              return -1;
          }
        }

        break;
      case TLS_ALPN_LIST: /* Set the Client Application Layer Protocol Negotiation */
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
      case TLS_HOSTNAME: /* Set the Server Name Indication */
        strncpy(p_net_ctx->Sockets[sock].SNI, (char *)optval, sizeof(p_net_ctx->Sockets[sock].SNI) - 1);
        break;
      default:
        NET_LOG_ERROR("Unsupported TLS option");
        return ret;
    }
  }
  else
  {
    return ret;
  }
  return 0;
}

int32_t W6X_Net_TLS_Credential_AddByName(uint32_t tag, W6X_Net_Tls_Credential_e type, const char *name)
{
  /* Call the AddByContent with content NULL. Requires the file to be present in the host filesystem */
  return W6X_Net_TLS_Credential_AddByContent(tag, type, name, NULL, 0);
}

int32_t W6X_Net_TLS_Credential_AddByContent(uint32_t tag, W6X_Net_Tls_Credential_e type,
                                            const char *name, const char *content, uint32_t len)
{
  NULL_ASSERT(p_DrvObj, W6X_Net_Uninit_str);
  NULL_ASSERT(p_net_ctx, W6X_Ctx_Null_str);

  /* Check if the name is valid to be used as filename in the filesystem of the NCP */
  if ((name == NULL) || (strlen(name) == 0))
  {
    NET_LOG_ERROR("Invalid name\n");
    return -1;
  }

  if (tag >= W61_NET_MAX_CONNECTIONS * 3) /* Check if the tag is in valid range */
  {
    NET_LOG_ERROR("Tag cannot be higher than %" PRIu32 ", value is %" PRIu32 "\n",
                  (uint32_t)W61_NET_MAX_CONNECTIONS * 3, tag);
    return -1;
  }
  if (p_net_ctx->Credentials[tag].is_valid == 1) /* Check if the tag is already defined */
  {
    NET_LOG_ERROR("Tag %" PRIu32 " already in use\n", tag);
    return -1; /* Tag already used */
  }
  /* Store the credential information */
  p_net_ctx->Credentials[tag].type = type;
  p_net_ctx->Credentials[tag].name = (char *) pvPortMalloc(strlen(name) + 1);
  if (p_net_ctx->Credentials[tag].name == NULL)
  {
    NET_LOG_ERROR("Malloc failed\n");
    return -1;
  }
  strncpy(p_net_ctx->Credentials[tag].name, (char *)name, strlen(name) + 1);

  if ((type == W6X_NET_TLS_CREDENTIAL_PSK) || (type == W6X_NET_TLS_CREDENTIAL_PSK_ID))
  {
    /* For PSK and PSK_ID just store the string, no need to write a file into the NCP */
    p_net_ctx->Credentials[tag].is_valid = 1;
    return 0;
  }

  if (content != NULL)
  {
    /* Write the file into the NCP using the local content buffer */
    if (W6X_FS_WriteFileByContent(p_net_ctx->Credentials[tag].name, content, len) != W6X_STATUS_OK)
    {
      NET_LOG_ERROR("Writing file %s into the NCP failed\n", p_net_ctx->Credentials[tag].name);
      return -1;
    }
  }
  else
  {
    /* Write the file into the NCP using the file already present in the filesystem */
    if (W6X_FS_WriteFileByName(p_net_ctx->Credentials[tag].name) != W6X_STATUS_OK)
    {
      NET_LOG_ERROR("Writing file %s into the NCP failed\n", p_net_ctx->Credentials[tag].name);
      return -1;
    }
  }
  p_net_ctx->Credentials[tag].is_valid = 1; /* Mark the tag as valid */
  return 0;
}

int32_t W6X_Net_TLS_Credential_Delete(uint32_t tag, W6X_Net_Tls_Credential_e type)
{
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
  if (p_net_ctx->Credentials[tag].name != NULL)
  {
    /* Forget the credential filename information */
    vPortFree(p_net_ctx->Credentials[tag].name);
    p_net_ctx->Credentials[tag].name = NULL;
  }
  p_net_ctx->Credentials[tag].is_valid = 0; /* Mark the tag as invalid */
  return 0;
}

char *W6X_Net_Inet_ntop(int32_t af, const void *src, char *dst, socklen_t size)
{
  char *ret = NULL;
  int32_t size_int = (int)size;
#if (W6X_NET_IPV6_ENABLE == 1)
  struct in6_addr in6;
#endif /* W6X_NET_IPV6_ENABLE */

  if (size_int < 0)
  {
    return NULL;
  }

  switch (af)
  {
    case AF_INET:
    {
      uint32_t src_addr = *((uint32_t *) src);
      uint8_t ip_addr[4] = {0};
      NTOA_R(src_addr, ip_addr);
      snprintf(dst, size, "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16,
               ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);
      ret = dst;
      break;
    }
#if (W6X_NET_IPV6_ENABLE == 1)
    case AF_INET6: /* IPv6 */
      /* Copy each uint32_t into the 16-byte array, converting to network byte order */
      for (uint8_t i = 0; i < 4; ++i)
      {
        uint32_t net = PP_HTONL(((uint32_t *)src)[i]);
        memcpy(&in6.un.u32_addr[i], &net, 4);
      }
      /* Convert to string */
      inet_ntop6(in6.un.u8_addr, dst, size);
      ret = dst;
      break;
#endif /* W6X_NET_IPV6_ENABLE */
    default:
      break;
  }
  return ret;
}

int32_t W6X_Net_Inet_pton(int32_t af, char *src, const void *dst)
{
  int32_t ret = -1;

  switch (af)
  {
    case AF_INET:
    {
      uint8_t dst_addr_tmp[4] = {0};
      Parser_StrToIP(src, dst_addr_tmp);
      if (Parser_CheckValidAddress(dst_addr_tmp, 4) == 0)
      {
        uint32_t *dst_addr = (uint32_t *) dst;
        *dst_addr = ATON(dst_addr_tmp);
        ret = 1;
      }
      break;
    }
#if (W6X_NET_IPV6_ENABLE == 1)
    case AF_INET6:
    {
      /* Destination is treated generically as 4 host-order 32-bit words. */
      uint32_t *words = (uint32_t *)dst;
      if (W61_Net_Inet6_aton(src, words) == 1)
      {
        /* set_errno(ENOSPC); */
        ret = 1;
      }
      break;
    }
#endif /* W6X_NET_IPV6_ENABLE */
    default:
      break;
  }
  return ret;
}

/** @} */
#if (W6X_NET_IPV6_ENABLE == 1)
/* Public IPv6 aton wrapper delegating to driver implementation */
int32_t W6X_Net_Inet6_aton(const char *src, struct in6_addr *dst)
{
  if ((src == NULL) || (dst == NULL)) { return 0; }
  return W61_Net_Inet6_aton(src, dst->un.u32_addr);
}
#endif /* W6X_NET_IPV6_ENABLE */
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
    NET_LOG_ERROR("Please register the APP callback before initializing the module\n");
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
      /* Update Remote IP family using pton return status and update the connection RemotIP */
      if (W6X_Net_Inet_pton(AF_INET, p_param_net_data->remote_ip,
                            &p_net_ctx->Connection[p_param_net_data->socket_id].RemoteIP) != -1)
      {
        p_net_ctx->Connection[p_param_net_data->socket_id].Family = W6X_NET_IPV4;
      }
      else if (W6X_Net_Inet_pton(AF_INET6, p_param_net_data->remote_ip,
                                 &p_net_ctx->Connection[p_param_net_data->socket_id].RemoteIP) != -1)
      {
        p_net_ctx->Connection[p_param_net_data->socket_id].Family = W6X_NET_IPV6;
      }

      if (p_net_ctx->Connection[p_param_net_data->socket_id].DataAvailable != NULL)
      {
        (void)xSemaphoreGive(p_net_ctx->Connection[p_param_net_data->socket_id].DataAvailable);
      }
      /* Call the application callback to notify the data available */
      p_cb_handler->APP_net_cb(W6X_NET_EVT_SOCK_DATA_ID, (void *) p_param_net_data);
      break;

    case W61_NET_EVT_SOCK_CONNECTED_ID:
      NET_LOG_DEBUG("Socket %" PRIu32 " connected\n", p_param_net_data->socket_id);
      p_net_ctx->Connection[p_param_net_data->socket_id].SocketConnected = 1;
      break;

    case W61_NET_EVT_SOCK_DISCONNECTED_ID:
      NET_LOG_DEBUG("Socket %" PRIu32 " disconnected\n", p_param_net_data->socket_id);
      W6X_Net_Clean_Socket(p_param_net_data->socket_id);
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
  uint8_t ip_addr[4];
  W61_Net_Connection_t conn;

  Parser_StrToIP(remote_ip, ip_addr);

  for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
  {
    int32_t found = 0;
    if (p_net_ctx->Connection[i].SocketConnected == 1)
    {
      for (int32_t j = 0; j < W61_NET_MAX_CONNECTIONS + 1; j++)
      {
        if ((p_net_ctx->Sockets[j].Number == i) && (p_net_ctx->Sockets[j].Status == W6X_NET_SOCKET_CONNECTED))
        {
          found = 1; /* Already connected */
          break;
        }
      }
      if (found == 0)
      {
        /* Get the socket information */
        ret = W6X_Net_TranslateErrorStatus(W61_Net_GetSocketInformation(p_DrvObj, i, &conn));

        if (ret == 0)
        {
          /* Check if IP Address and port match with the current connection */
          if ((strncmp(conn.RemoteIP, remote_ip, sizeof(conn.RemoteIP)) == 0) && (remoteport == conn.RemotePort))
          {
            return i; /* Found the connection */
          }
        }
      }
    }
  }
  return ret;
}

static int32_t W6X_Net_Poll_UDP_Sockets(struct sockaddr *remote_addr)
{
  int32_t found_socket = 0;
  struct sockaddr_in *addr = (struct sockaddr_in *)remote_addr;
#if (W6X_NET_IPV6_ENABLE == 1)
  struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)remote_addr;
#endif /* W6X_NET_IPV6_ENABLE */
  for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS; i++)
  {
    /* Find the first active connection with pending data */
    if ((p_net_ctx->Connection[i].SocketConnected != 1) || (p_net_ctx->Connection[i].DataAvailableSize == 0))
    {
      continue; /* Connection not active or no data available */
    }
    /* Since this function is used for UDP server to look for data to read,
     * skip a connection if used by an existing socket (which would be a client) */
    for (int32_t j = 0; j < W61_NET_MAX_CONNECTIONS + 1; j++)
    {
      if (p_net_ctx->Sockets[j].Number == i)
      {
        found_socket = 1; /* Connection already used by a socket */
        break;
      }
    }
    if (found_socket == 0) /* Connection not used by any socket */
    {
#if (W6X_NET_IPV6_ENABLE == 1)
      if (p_net_ctx->Connection[i].Family == W6X_NET_IPV6)
      {
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = PP_HTONS(p_net_ctx->Connection[i].RemotePort);
        memcpy(addr6->sin6_addr.un.u32_addr, p_net_ctx->Connection[i].RemoteIP, sizeof(addr6->sin6_addr.un.u32_addr));
      }
      else
#endif /* W6X_NET_IPV6_ENABLE */
      {
        addr->sin_family = AF_INET;
        addr->sin_port = PP_HTONS(p_net_ctx->Connection[i].RemotePort);
        memcpy(&(addr->sin_addr.s_addr), &(p_net_ctx->Connection[i].RemoteIP[0]), sizeof(addr->sin_addr.s_addr));

      }
      return i;
    }
  }
  return -1;
}

static int32_t W6X_Net_Wait_Pull_Data(int32_t sock, int32_t connection_id, void *buf, size_t max_len)
{
  uint32_t received_data_len = 0;
  int32_t ret;
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

    while (p_net_ctx->Connection[connection_id].DataAvailableSize < max_len)
    {
      /* In TCP, wait another +IPD for maximum 3 ms to maximize data available to fetch */
      /* In UDP, wait more +IPDs for maximum 3 ms to maximize data available to fetch */
      BaseType_t sem_status = xSemaphoreTake(p_net_ctx->Connection[connection_id].DataAvailable, (TickType_t) 3);
      if ((sem_status == pdFAIL) || (p_net_ctx->Sockets[sock].Protocol == W6X_NET_TCP_PROTOCOL))
      {
        break;
      }
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
        NET_LOG_ERROR("Pull data from socket failed\n");
      }
      return ret;
    }
    /* Should not happen */
    taskENTER_CRITICAL();
    if (received_data_len > p_net_ctx->Connection[connection_id].DataAvailableSize)
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

#if (W6X_NET_IPV6_ENABLE == 1)
static const char *inet_ntop6(const uint8_t src[16], char *dst, size_t size)
{
  /* Find the longest run of zeros for "::" compression (per RFC 5952) */
  int32_t best_base = -1; /* Start index of the best zero run */
  int32_t best_len = 0;   /* Length of the best zero run */
  int32_t cur_base = -1;  /* Start index of the current zero run */
  int32_t cur_len = 0;    /* Length of the current zero run */
  uint16_t words[8];      /* IPv6 address split into 8 16-bit words */

  /* Convert the 16-byte IPv6 address into 8 16-bit words */
  for (int32_t word_cnt = 0; word_cnt < 8; ++word_cnt)
  {
    words[word_cnt] = (src[2 * word_cnt] << 8) | src[2 * word_cnt + 1];
  }

  /* Find the longest sequence of zero words for possible '::' compression */
  for (int32_t word_cnt = 0; word_cnt < 8; ++word_cnt)
  {
    if (words[word_cnt] == 0)
    {
      /* Start a new zero run if not already in one */
      if (cur_base == -1)
      {
        cur_base = word_cnt;
        cur_len = 1;
      }
      else
      {
        /* Continue the current zero run */
        cur_len++;
      }
    }
    else
    {
      /* End of a zero run, check if it's the best so far */
      if (cur_len > best_len)
      {
        best_base = cur_base;
        best_len = cur_len;
      }
      cur_base = -1;
      cur_len = 0;
    }
  }

  /* Check at the end in case the best run is at the end of the address */
  if (cur_len > best_len)
  {
    best_base = cur_base;
    best_len = cur_len;
  }

  /* Only compress if the run is at least two words long */
  if (best_len < 2)
  {
    best_base = -1;
  } /* Don't compress if less than 2 zeros */

  /* Build the output string, applying '::' compression if found */
  char *ptr = dst;
  size_t left = size;
  for (int32_t word_cnt = 0; word_cnt < 8;)
  {
    /* Check if we are at the start of the best zero run for compression */
    if (word_cnt == best_base)
    {
      int32_t pos;
      /* Insert '::' at the start or ':' elsewhere */
      if (word_cnt == 0)
      {
        pos = snprintf(ptr, left, "::");
      }
      else
      {
        pos = snprintf(ptr, left, ":");
      }
      ptr += pos;
      left -= pos;
      word_cnt += best_len; /* Skip the compressed zeros */
      if (word_cnt >= 8) { break; }
    }
    else
    {
      /* Print the current word in hexadecimal */
      int32_t pos = snprintf(ptr, left, "%x", words[word_cnt]);
      ptr += pos;
      left -= pos;
      /* Add ':' between words, except after the last one */
      if (++word_cnt < 8)
      {
        pos = snprintf(ptr, left, ":");
        ptr += pos;
        left -= pos;
      }
    }
  }
  /* Return the resulting string */
  return dst;
}
#endif /* W6X_NET_IPV6_ENABLE */

static void W6X_Net_Clean_Socket(int32_t sock)
{
  for (int32_t i = 0; i < W61_NET_MAX_CONNECTIONS + 1; i++)
  {
    if (p_net_ctx->Sockets[i].Number == sock)
    {
      if (p_net_ctx->Connection[sock].DataAvailableSize == 0)
      {
        if (p_net_ctx->Connection[p_net_ctx->Sockets[sock].Number].DataAvailable != NULL)
        {
          (void)xSemaphoreTake(p_net_ctx->Connection[sock].DataAvailable, (TickType_t) 0);
        }

        /* Free credentials that were allocated */
        if (p_net_ctx->Sockets[i].Ca_Cert != NULL)
        {
          vPortFree(p_net_ctx->Sockets[i].Ca_Cert);
        }
        if (p_net_ctx->Sockets[i].Private_Key != NULL)
        {
          vPortFree(p_net_ctx->Sockets[i].Private_Key);
        }
        if (p_net_ctx->Sockets[i].Certificate != NULL)
        {
          vPortFree(p_net_ctx->Sockets[i].Certificate);
        }
        if (p_net_ctx->Sockets[i].PSK != NULL)
        {
          vPortFree(p_net_ctx->Sockets[i].PSK);
        }
        if (p_net_ctx->Sockets[i].PSK_Identity != NULL)
        {
          vPortFree(p_net_ctx->Sockets[i].PSK_Identity);
        }

        memset(&p_net_ctx->Sockets[i], 0, sizeof(W6X_Net_Socket_t)); /* Erase the socket context */
        /* Set the socket number to an invalid value */
        p_net_ctx->Sockets[i].Number = W61_NET_MAX_CONNECTIONS + 1;
      }
    }
  }
}
/** @} */

#endif /* ST67_ARCH */
