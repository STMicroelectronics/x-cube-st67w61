/**
  ******************************************************************************
  * @file    w6x_types.h
  * @author  GPM Application Team
  * @brief   This file provides the different W6x core resources types
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef W6X_TYPES_H
#define W6X_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "w6x_default_config.h"
#include "w61_default_config.h"

/* Exported constants --------------------------------------------------------*/
/* ===================================================================== */
/** @defgroup ST67W6X_API_System_Public_Constants ST67W6X System Constants
  * @ingroup  ST67W6X_API_System
  * @{
  */
/* ===================================================================== */
/**
  * @brief  W6X Status enumeration
  */
typedef enum
{
  W6X_STATUS_OK                  = 0,  /*!< Operation successful */
  W6X_STATUS_BUSY                = 1,  /*!< Operation not done: driver in use by another task */
  W6X_STATUS_ERROR               = 2,  /*!< Operation failed */
  W6X_STATUS_TIMEOUT             = 3,  /*!< Operation timeout */
  W6X_STATUS_UNEXPECTED_RESPONSE = 4,  /*!< Response not supported */
  W6X_STATUS_NOT_SUPPORTED       = 5,  /*!< Operation not supported */

} W6X_Status_t;

#define W6X_SYS_FS_FILENAME_SIZE      32 /*!< Maximum size of the filename */

#define W6X_SYS_FS_MAX_FILES          20 /*!< Maximum number of files */

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_WiFi_Public_Constants ST67W6X Wi-Fi Constants
  * @ingroup  ST67W6X_API_WiFi
  * @{
  */
/* ===================================================================== */
#define W6X_WIFI_MAX_SSID_SIZE          32                        /*!< Maximum size of the SSID */

#define W6X_WIFI_MAX_PASSWORD_SIZE      63                        /*!< Maximum size of the password */

#define W6X_WIFI_MAX_CONNECTED_STATIONS 4                         /*!< Maximum number of connected stations */

#define W6X_WIFI_SIZEOF_IPV4_BYTES      4                         /*!< Size of an IPv4 address in bytes */

/* W6X_event_id_t */
#define W6X_WIFI_EVT_CONNECTED_ID                      102  /*!< No param */
#define W6X_WIFI_EVT_DISCONNECTED_ID                   103  /*!< No param */
#define W6X_WIFI_EVT_GOT_IP_ID                         104  /*!< No param */
#define W6X_WIFI_EVT_CONNECTING_ID                     105  /*!< No param */
#define W6X_WIFI_EVT_REASON_ID                         106  /*!< No param */

#define W6X_WIFI_EVT_STA_CONNECTED_ID                  110  /*!< Not yet used */
#define W6X_WIFI_EVT_STA_DISCONNECTED_ID               111  /*!< Not yet used */
#define W6X_WIFI_EVT_DIST_STA_IP_ID                    112  /*!< Not yet used */

/**
  * @brief  Wi-Fi Scan type enumeration
  */
typedef enum
{
  W6X_WIFI_SCAN_ACTIVE = 0,
  W6X_WIFI_SCAN_PASSIVE = 1
} W6X_WiFi_scan_type_e;

/**
  * @brief  Wi-Fi security encryption method
  */
typedef enum
{
  W6X_WIFI_SECURITY_OPEN = 0x00,             /*!< Wi-Fi is open */
  W6X_WIFI_SECURITY_WEP = 0x01,              /*!< Wired Equivalent Privacy option for Wi-Fi security. */
  W6X_WIFI_SECURITY_WPA_PSK = 0x02,          /*!< Wi-Fi Protected Access */
  W6X_WIFI_SECURITY_WPA2_PSK = 0x03,         /*!< Wi-Fi Protected Access 2 */
  W6X_WIFI_SECURITY_WPA_WPA2_PSK = 0x04,     /*!< Wi-Fi Protected Access with both modes */
  W6X_WIFI_SECURITY_WPA_ENT = 0x05,          /*!< Wi-Fi Protected Access */
  W6X_WIFI_SECURITY_WPA3_SAE = 0x06,         /*!< Wi-Fi Protected Access 3 */
  W6X_WIFI_SECURITY_WPA2_WPA3_SAE = 0x07,    /*!< Wi-Fi Protected Access with both modes */
  W6X_WIFI_SECURITY_UNKNOWN = 0x08,          /*!< Other modes */
} W6X_WiFi_SecurityType_e;

/**
  * @brief  Wi-Fi Soft-AP security encryption method
  */
typedef enum
{
  W6X_WIFI_AP_SECURITY_OPEN = 0x00,          /*!< Wi-Fi is open */
  W6X_WIFI_AP_SECURITY_WEP = 0x01,           /*!< Wired Equivalent Privacy option for Wi-Fi security. */
  W6X_WIFI_AP_SECURITY_WPA_PSK = 0x02,       /*!< Wi-Fi Protected Access */
  W6X_WIFI_AP_SECURITY_WPA2_PSK = 0x03,      /*!< Wi-Fi Protected Access 2 */
  W6X_WIFI_AP_SECURITY_WPA3_PSK = 0x04,      /*!< Wi-Fi Protected Access 3 */
} W6X_WiFi_ApSecurityType_e;

/**
  * @brief  Wi-Fi Connection pairwise cipher type enumeration
  */
typedef enum
{
  W6X_WIFI_EVENT_BEACON_IND_CIPHER_NONE     = 0,    /*!< None */
  W6X_WIFI_EVENT_BEACON_IND_CIPHER_WEP      = 1,    /*!< WEP40 */
  W6X_WIFI_EVENT_BEACON_IND_CIPHER_AES      = 4,    /*!< AES/CCMP */
  W6X_WIFI_EVENT_BEACON_IND_CIPHER_TKIP     = 3,    /*!< TKIP */
  W6X_WIFI_EVENT_BEACON_IND_CIPHER_TKIP_AES = 5,    /*!< TKIP and AES/CCMP */
  W6X_WIFI_EVENT_BEACON_IND_UNKNOW          = 7,    /*!< Unknown */
} W6X_WiFi_CipherType_e;

/**
  * @brief  Wi-Fi station status enumeration
  */
typedef enum
{
  W6X_WIFI_STATE_STA_NO_STARTED_CONNECTION  = 0,    /*!< No connection started */
  W6X_WIFI_STATE_STA_CONNECTED              = 1,    /*!< Station connected to an Access Point */
  W6X_WIFI_STATE_STA_GOT_IP                 = 2,    /*!< Station got IP address */
  W6X_WIFI_STATE_STA_CONNECTING             = 3,    /*!< Station connecting to an Access Point */
  W6X_WIFI_STATE_STA_DISCONNECTED           = 4,    /*!< Station disconnected from an Access Point */
  W6X_WIFI_STATE_STA_OFF                    = 5,    /*!< Station off */
} W6X_WiFi_StaStateType_e;

/**
  * @brief  Wi-Fi Soft-AP status enumeration
  */
typedef enum
{
  W6X_WIFI_STATE_AP_RESET                   = 0,    /*!< Soft-AP reset */
  W6X_WIFI_STATE_AP_RUNNING                 = 1,    /*!< Soft-AP running */
  W6X_WIFI_STATE_AP_OFF                     = 2,    /*!< Soft-AP off */
} W6X_WiFi_ApStateType_e;

/**
  * @brief  Wi-Fi DHCP Client type enumeration
  */
typedef enum
{
  W6X_WIFI_DHCP_DISABLED               = 0,  /*!< DHCP disabled */
  W6X_WIFI_DHCP_STA_ENABLED            = 1,  /*!< DHCP client enabled for station */
  W6X_WIFI_DHCP_AP_ENABLED             = 2,  /*!< DHCP server enabled for Soft-AP */
  W6X_WIFI_DHCP_STA_AP_ENABLED         = 3,  /*!< DHCP client enabled for station and DHCP server for Soft-AP */
} W6X_WiFi_DhcpType_e;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_Net_Public_Constants ST67W6X Net Constants
  * @ingroup  ST67W6X_API_Net
  * @{
  */
/* ===================================================================== */
/* W6X_event_id_t */
/** event_args parameter to be casted to W6X_Net_CbParamData_t */
#define W6X_NET_EVT_SOCK_DATA_ID                       150

#define NET_IPV6          0             /*!< IPv6 enabled */

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN  40            /*!< IPv6 string length */
#endif /* INET6_ADDRSTRLEN */

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN   15            /*!< IPv4 string length */
#endif /* INET_ADDRSTRLEN */

/* Socket protocol types */
#define SOCK_STREAM       1             /*!< TCP socket */
#define SOCK_DGRAM        2             /*!< UDP socket */
#define SOCK_RAW          3             /*!< IP socket */

/* Socket address families */
#define AF_UNSPEC         0             /*!< Unspecified */
#define AF_INET           2             /*!< Internet domain sockets for use with IPv4 addresses. */
#define AF_INET6          10            /*!< Internet domain sockets for use with IPv6 addresses. */

/* Socket options */
#define IPPROTO_IP        0             /*!< IP protocol */
#define IPPROTO_ICMP      1             /*!< ICMP protocol */
#define IPPROTO_TCP       6             /*!< TCP protocol */
#define IPPROTO_UDP       17            /*!< UDP protocol */
#if NET_IPV6
#define IPPROTO_IPV6      41            /*!< IPv6 protocol */
#define IPPROTO_ICMPV6    58            /*!< ICMPv6 protocol */
#endif /* NET_IPV6 */
#define IPPROTO_UDPLITE   136           /*!< UDPLite protocol */
#define IPPROTO_RAW       255           /*!< Raw protocol */
#define IPPROTO_TLS_1_2   282           /*!< TLS 1.2 protocol */

#define SOL_SOCKET        0             /*!< Socket level */
#define SOL_TLS           282           /*!< TLS level */

/* Socket options */
#define TCP_NODELAY       0x0001        /*!< Don't delay send to coalesce packets */
#define SO_KEEPALIVE      0x0008        /*!< Connections are kept alive with periodic messages */
#define SO_LINGER         0x0080        /*!< Socket lingers on close if data present */
#define SO_RCVBUF         0x1002        /*!< Receive buffer size */
#define SO_SNDTIMEO       0x1005        /*!< Send timeout */
#define SO_RCVTIMEO       0x1006        /*!< Receive timeout */

#define TLS_SEC_TAG_LIST  1             /*!< Security tag list */
#define TLS_HOSTNAME      2             /*!< Hostname for SNI */
#define TLS_ALPN_LIST     7             /*!< ALPN list */

/**
  * @brief  Network connection type
  */
typedef enum
{
  W6X_NET_TCP_PROTOCOL = 0,             /*!< TCP protocol */
  W6X_NET_UDP_PROTOCOL = 1,             /*!< UDP protocol */
  W6X_NET_SSL_PROTOCOL = 2              /*!< SSL protocol */
} W6X_Net_Protocol_e;

/**
  * @brief  SSL socket credential type
  */
typedef enum
{
  /** No Credential */
  W6X_NET_TLS_CREDENTIAL_NONE = 0,
  /** CA Certificate, to authenticate peer */
  W6X_NET_TLS_CREDENTIAL_CA_CERTIFICATE = 1,
  /** Certificate for to be authenticated by peer, to be used with private key */
  W6X_NET_TLS_CREDENTIAL_SERVER_CERTIFICATE = 2,
  /** Private key to be used with Certificate to be authenticated by peer */
  W6X_NET_TLS_CREDENTIAL_PRIVATE_KEY = 3,
  /** Pre shared key for PSK authentication */
  W6X_NET_TLS_CREDENTIAL_PSK = 4,
  /** Identity to be used with PSK for PSK authentication */
  W6X_NET_TLS_CREDENTIAL_PSK_ID = 5
} W6X_Net_Tls_Credential_e;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_MQTT_Public_Constants ST67W6X MQTT Constants
  * @ingroup  ST67W6X_API_MQTT
  * @{
  */
/* ===================================================================== */
/* W6X_event_id_t */
#define W6X_MQTT_EVT_CONNECTED_ID                      170  /*!< no param */
#define W6X_MQTT_EVT_DISCONNECTED_ID                   171  /*!< no param */
#define W6X_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID          172  /*!< event_args parameter to be casted to W6X_MQTT_Data_t */

/**
  * @brief  MQTT status enumeration
  */
typedef enum
{
  W6X_MQTT_STATE_UNINIT                = 0,   /*!< MQTT client uninitialized */
  W6X_MQTT_STATE_USRCFG_DONE           = 1,   /*!< User configuration done */
  W6X_MQTT_STATE_CONNCFG_DONE          = 2,   /*!< Connection configuration done */
  W6X_MQTT_STATE_DISCONNECTED          = 3,   /*!< MQTT client disconnected */
  W6X_MQTT_STATE_CONNECTED             = 4,   /*!< MQTT client connected */
  W6X_MQTT_STATE_CONNECTED_NO_SUB      = 5,   /*!< MQTT client connected but not subscribed */
  W6X_MQTT_STATE_CONNECTED_SUBSCRIBED  = 6,   /*!< MQTT client connected and subscribed */
} W6X_MQTT_State_e;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_BLE_Public_Constants ST67W6X BLE Constants
  * @ingroup  ST67W6X_API_BLE
  * @{
  */
/* ===================================================================== */
/* W6X_event_id_t */
#define W6X_BLE_EVT_CONNECTED_ID                       121  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_DISCONNECTED_ID                    122  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_CONNECTION_PARAM_ID                123  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_READ_ID                            124  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_WRITE_ID                           125  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_SERVICE_FOUND_ID                   126  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_CHAR_FOUND_ID                      127  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_INDICATION_STATUS_ENABLED_ID       128  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_INDICATION_STATUS_DISABLED_ID      129  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_NOTIFICATION_STATUS_ENABLED_ID     130  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_NOTIFICATION_STATUS_DISABLED_ID    131  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_NOTIFICATION_DATA_ID               132  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_MTU_SIZE_ID                        133  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_PAIRING_FAILED_ID                  134  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_PAIRING_COMPLETED_ID               135  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_PAIRING_CONFIRM_ID                 136  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_PASSKEY_ENTRY_ID                   137  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_PASSKEY_DISPLAY_ID                 138  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_PASSKEY_CONFIRM_ID                 139  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_INDICATION_ACK_ID                  140  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */
#define W6X_BLE_EVT_INDICATION_NACK_ID                 141  /*!< event_args parameter to be casted to W6X_Ble_CbParamData_t */

/** Maximum number of BLE connections */
#define W6X_BLE_MAX_CONN_NBR                           W61_BLE_MAX_CONN_NBR

/** Maximum number of BLE bonded devices */
#define W6X_BLE_MAX_BONDED_DEVICES                     W61_BLE_MAX_BONDED_DEVICES

/** Maximum number of BLE service */
#define W6X_BLE_MAX_SERVICE_NBR                        W61_BLE_MAX_SERVICE_NBR

/** Maximum number of BLE characteristics per service */
#define W6X_BLE_MAX_CHAR_NBR                           W61_BLE_MAX_CHAR_NBR

/** BLE Device Address size */
#define W6X_BLE_BD_ADDR_SIZE                           6

/** BLE Device Name size */
#define W6X_BLE_DEVICE_NAME_SIZE                       26

/** BLE Manufacturer Data size */
#define W6X_BLE_MANUF_DATA_SIZE                        30

/** BLE Service/Characteristic UUID maximum size size */
#define W6X_BLE_MAX_UUID_SIZE                          17

/**
  * @brief  BLE characteristic property index enumeration
  */
typedef enum
{
  W6X_BLE_CHAR_PROP_READ = 2,                               /*!< Property read */
  W6X_BLE_CHAR_PROP_WRITE_WITHOUT_RESP = 4,                 /*!< Property write without response */
  W6X_BLE_CHAR_PROP_WRITE_WITH_RESP = 8,                    /*!< Property write with response */
  W6X_BLE_CHAR_PROP_NOTIFY = 16,                            /*!< Property notify */
  W6X_BLE_CHAR_PROP_INDICATE = 32                           /*!< Property indicate */
} W6X_Ble_Char_Property_e;

/**
  * @brief  BLE characteristic permission enumeration
  */
typedef enum
{
  W6X_BLE_CHAR_PERM_READ = 1,                               /*!< Permission read */
  W6X_BLE_CHAR_PERM_WRITE = 2                               /*!< Permission write */
} W6X_Ble_Char_Permission_e;

/**
  * @brief  BLE mode enumeration
  */
typedef enum
{
  W6X_BLE_MODE_CLIENT = 1,
  W6X_BLE_MODE_SERVER = 2
} W6X_Ble_Mode_e;

/**
  * @brief  BLE Advertising type enumeration
  */
typedef enum
{
  W6X_BLE_ADV_TYPE_IND = 0,
  W6X_BLE_ADV_TYPE_SCAN_IND = 1,
  W6X_BLE_ADV_TYPE_NONCONN_IND = 2
} W6X_Ble_AdvType_e;

/**
  * @brief  BLE Advertising channel enumeration
  */
typedef enum
{
  W6X_BLE_ADV_CHANNEL_37  = 1,
  W6X_BLE_ADV_CHANNEL_38  = 2,
  W6X_BLE_ADV_CHANNEL_39  = 4,
  W6X_BLE_ADV_CHANNEL_ALL = 7
} W6X_Ble_AdvChannel_e;

/**
  * @brief  BLE Scan type enumeration
  */
typedef enum
{
  W6X_BLE_SCAN_PASSIVE = 0,
  W6X_BLE_SCAN_ACTIVE = 1
} W6X_Ble_ScanType_e;

/**
  * @brief  BLE address type
  */
typedef enum
{
  W6X_BLE_PUBLIC_ADDR = 0,
  W6X_BLE_RANDOM_ADDR = 1,
  W6X_BLE_RPA_PUBLIC_ADDR = 2,
  W6X_BLE_RPA_RANDOM_ADDR = 3
} W6X_Ble_AddrType_e;

/**
  * @brief  BLE address type
  */
typedef enum
{
  W6X_BLE_SCAN_FILTER_ALLOW_ALL = 0,
  W6X_BLE_SCAN_FILTER_ALLOW_ONLY_WLST = 1,
  W6X_BLE_SCAN_FILTER_ALLOW_UND_RPA_DIR = 2,
  W6X_BLE_SCAN_FILTER_ALLOW_WLIST_PRA_DIR = 3
} W6X_Ble_FilterPolicy_e;

/**
  * @brief  BLE security parameters
  */
typedef enum
{
  W6X_BLE_SEC_IO_DISPLAY_ONLY = 0,
  W6X_BLE_SEC_IO_DISPLAY_YESNO = 1,
  W6X_BLE_SEC_IO_KEYBOARD_ONLY = 2,
  W6X_BLE_SEC_IO_NO_INPUT_OUTPUT = 3,
  W6X_BLE_SEC_IO_KEYBOARD_DISPLAY = 4
} W6X_Ble_SecurityParameter_e;

/**
  * @brief  BLE UUID types
  */
typedef enum
{
  W6X_BLE_UUID_TYPE_16 = 0,
  W6X_BLE_UUID_TYPE_128 = 2,
} W6X_Ble_UuidType_e;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_HTTP_Public_Constants ST67W6X HTTP Constants
  * @ingroup  ST67W6X_API_HTTP
  * @{
  */
/* ===================================================================== */

/* Different HTTP request type */
#define W6X_HTTP_REQ_TYPE_HEAD  1                                  /*!< HTTP HEAD request */
#define W6X_HTTP_REQ_TYPE_GET   2                                  /*!< HTTP GET request */
#define W6X_HTTP_REQ_TYPE_POST  3                                  /*!< HTTP POST request */
#define W6X_HTTP_REQ_TYPE_PUT   4                                  /*!< HTTP PUT request */

/**
  * @brief  Define generic categories for HTTP response codes
  */
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
} W6X_HTTP_Status_Code_e;

/**
  * @brief  Define generic categories for HTTP response codes
  */
typedef enum
{
  HTTP_CATEGORY_UNKNOWN = 0,
  HTTP_CATEGORY_INFORMATIONAL = 1,
  HTTP_CATEGORY_SUCCESS = 2,
  HTTP_CATEGORY_REDIRECTION = 3,
  HTTP_CATEGORY_CLIENT_ERROR = 4,
  HTTP_CATEGORY_SERVER_ERROR = 5
} W6X_HTTP_Status_Category_e;

/**
  * @brief  Define POST and PUT data structure
  */
typedef enum
{
  W6X_HTTP_CONTENT_TYPE_PLAIN_TEXT = 0,
  W6X_HTTP_CONTENT_TYPE_URL_ENCODED,
  W6X_HTTP_CONTENT_TYPE_JSON,
  W6X_HTTP_CONTENT_TYPE_XML,
  W6X_HTTP_CONTENT_TYPE_OCTET_STREAM
} W6X_HTTP_Content_Type_e;

/** @} */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup ST67W6X_API_Net_Public_Macros ST67W6X Net Macros
  * @ingroup  ST67W6X_API_Net
  * @{
  */

/**
  * @brief  Convert 16-bit value from host byte order to network byte order
  */
#define PP_HTONS(x)    ((uint16_t)((((x) & (uint16_t)0x00ffU) << 8) | (((x) & (uint16_t)0xff00U) >> 8)))

/**
  * @brief  Convert 16-bit value from host byte order to network byte order
  */
#define PP_NTOHS(x)    PP_HTONS(x)

/**
  * @brief  Convert 32-bit value from host byte order to network byte order
  */
#define PP_HTONL(x)    ((((x) & 0x000000ffUL) << 24) | \
                        (((x) & 0x0000ff00UL) <<  8) | \
                        (((x) & 0x00ff0000UL) >>  8) | \
                        (((x) & 0xff000000UL) >> 24))

/**
  * @brief  Convert 4-bytes array to 32-bit value
  */
#define ATON_R(x)      (uint32_t)(((x)[0] << 24) + ((x)[1] << 16) + ((x)[2]  << 8) + (x)[3])

/** @} */

/* Exported types ------------------------------------------------------------*/
/* ===================================================================== */
/** @defgroup ST67W6X_API_System_Public_Types ST67W6X System Types
  * @ingroup  ST67W6X_API_System
  * @{
  */
/* ===================================================================== */
/**
  * @brief  W6X event type
  */
typedef  uint8_t W6X_event_id_t;

#if defined(__ICCARM__) || defined(__ICCRX__) /* For IAR Compiler */
#ifdef __SIZE_T_TYPE__
typedef __SIZE_T_TYPE__ ssize_t;
#else
#error __SIZE_T_TYPE__ not defined
#endif /* __SIZE_T_TYPE__ */
#elif defined (__ARMCC_VERSION)
typedef uint32_t ssize_t;
#endif /* __ICCARM__ */

/**
  * @brief  W6X error callback function type
  * @param  ret_w6x: W6X status
  * @param  func_name: function name
  */
typedef void (*W6X_Error_cb_t)(W6X_Status_t ret_w6x, char const *func_name);

/**
  * @brief  W6X module information
  */
typedef struct
{
  uint8_t AT_Version[32];                     /*!< AT version string */
  uint8_t SDK_Version[32];                    /*!< SDK version string */
  uint8_t MAC_Version[32];                    /*!< MAC version string */
  uint8_t Build_Date[32];                     /*!< Build date string */
  uint32_t BatteryVoltage;                    /*!< Battery voltage */
  int8_t trim_wifi_hp[14];                    /*!< Wi-Fi high power trim */
  int8_t trim_wifi_lp[14];                    /*!< Wi-Fi low power trim */
  int8_t trim_ble[5];                         /*!< BLE trim */
  int8_t trim_xtal;                           /*!< XTAL Frequency compensation */
  uint8_t Mac_Address[6];                     /*!< Customer MAC address */
  uint8_t AntiRollbackBootloader;             /*!< AntiRollback Bootloader counter */
  uint8_t AntiRollbackApp;                    /*!< AntiRollback Application counter */
  char ModuleID[25];                          /*!< Module name */
  uint16_t BomID;                             /*!< BOM ID */
  uint8_t Manufacturing_Week;                 /*!< Manufacturing week */
  uint8_t Manufacturing_Year;                 /*!< Manufacturing year */
} W6X_ModuleInfo_t;

/**
  * @brief  File System list structure
  */
typedef struct
{
  char filename[W6X_SYS_FS_MAX_FILES][W6X_SYS_FS_FILENAME_SIZE];  /*!< List of filenames */
  uint32_t nb_files;                                              /*!< Number of files */
} W6X_FS_FilesList_t;

/**
  * @brief  File System Host and NCP list structure
  */
typedef struct
{
#if (LFS_ENABLE == 1)
  EfLfsInfo_t lfs_files_list[20];         /*!< List of files in LFS */
  uint32_t nb_files;                    /*!< Number of files */
#endif /* LFS_ENABLE */
  W6X_FS_FilesList_t ncp_files_list;    /*!< List of files in NCP */
} W6X_FS_FilesListFull_t;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_WiFi_Public_Types ST67W6X Wi-Fi Types
  * @ingroup  ST67W6X_API_WiFi
  * @{
  */
/* ===================================================================== */
/**
  * @brief  callback for Wi-Fi events. the type of event_args depends of the event_id:\
  *         - W6X_WiFi_CbParamData_t for [W6X_WIFI_EVT_CONNECTED_ID, W6X_WIFI_EVT_DISCONNECTED_ID,
  *                                      W6X_WIFI_EVT_GOT_IP_ID]
  *         - .. for [W6X_WIFI_EVT_STA_CONNECTED_ID, W6X_WIFI_EVT_STA_DISCONNECTED_ID, W6X_WIFI_EVT_DIST_STA_IP_ID]
  */
typedef void (*W6X_WiFi_App_cb_t)(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  Callback parameters for the Wi-Fi application
  */
typedef struct
{
  uint8_t IP[4];                              /*!< IP address of the station */
  uint8_t MAC[6];                             /*!< MAC address of Wi-Fi interface */
} W6X_WiFi_CbParamData_t;

/**
  * @brief  Wi-Fi mode bitmask
  */
typedef union
{
  struct
  {
    uint8_t b_mode:1;                         /*!< 802.11 b */
    uint8_t g_mode:1;                         /*!< 802.11 g */
    uint8_t n_mode:1;                         /*!< 802.11n at 2.4GHz */
    uint8_t ax_mode:1;                        /*!< 802.11ax at 2.4GHz */
  } bit;                                      /*!< Bitfield */
  uint8_t byte;                               /*!< Byte */
} W6X_WiFi_Mode_t;

/**
  * @brief  Access Point structure
  */
typedef struct
{
  uint8_t SSID[W6X_WIFI_MAX_SSID_SIZE + 1];   /*!< Service Set Identifier value. Wi-Fi spot name */
  W6X_WiFi_SecurityType_e Security;           /*!< Security of Wi-Fi spot */
  int16_t RSSI;                               /*!< Signal strength of Wi-Fi spot */
  uint8_t MAC[6];                             /*!< MAC address of spot */
  uint8_t Channel;                            /*!< Wi-Fi channel */
  W6X_WiFi_CipherType_e Group_cipher;         /*!< Group cipher value */
  W6X_WiFi_Mode_t WiFi_version;               /*!< Wi-Fi revision supported */
  uint8_t WPS;                                /*!< Is WPS enabled */
} W6X_WiFi_Ap_t;

/**
  * @brief  Scan results structure
  */
typedef struct
{
  uint32_t Count;                             /*!< Number of APs detected */
  W6X_WiFi_Ap_t *AP;                          /*!< Array of AP Beacon information */
} W6X_WiFi_Scan_Result_t;

/**
  * @brief  Wi-Fi Scan result callback
  */
typedef void(* W6X_WiFi_Scan_Result_cb_t)(int32_t status, W6X_WiFi_Scan_Result_t *entry);

/**
  * @brief  Wi-Fi scan options structure
  */
typedef struct
{
  uint8_t SSID[W6X_WIFI_MAX_SSID_SIZE + 1];   /*!< SSID filter */
  uint8_t MAC[6];                             /*!< BSSID filter */
  W6X_WiFi_scan_type_e Scan_type;             /*!< Scan type (0: Active, 1: Passive) */
  uint8_t Channel;                            /*!< Wi-Fi channel filter */
  uint8_t MaxCnt;                             /*!< Max number of APs to return */
} W6X_WiFi_Scan_Opts_t;

/**
  * @brief  Wi-Fi Connection options structure
  */
typedef struct
{
  /** Service Set Identifier value. Wi-Fi Access Point name */
  uint8_t SSID[W6X_WIFI_MAX_SSID_SIZE + 1];
  /** Password of Wi-Fi Access Point */
  uint8_t Password[W6X_WIFI_MAX_PASSWORD_SIZE + 1];
  /** MAC address of Wi-Fi Access Point */
  uint8_t MAC[6];
  /** Interval between Wi-Fi connection retries. Unit: second. Default: 0. Maximum: 7200 */
  uint16_t Reconnection_interval;
  /** Number of attempts the device makes to reconnect to the AP. Default 0 (always try), Max = 1000 */
  uint16_t Reconnection_nb_attempts;
  /** WPS mode. 0: WPS not used, 1: WPS PBC */
  uint32_t WPS;
  /** Connect to Access Point with WEP encryption. 0: WEP not used (default), 1: WEP used */
  uint32_t WEP;
} W6X_WiFi_Connect_Opts_t;

/**
  * @brief  Wi-Fi Connection status structure
  */
typedef struct
{
  /** Service Set Identifier value. Wi-Fi Access Point name */
  uint8_t SSID[W6X_WIFI_MAX_SSID_SIZE + 1];
  /** MAC address of Wi-Fi Access Point */
  uint8_t MAC[6];
  /** Wi-Fi channel */
  uint32_t Channel;
  /** Signal strength of connected Access Point */
  int32_t Rssi;
  /** Interval between Wi-Fi connection retries. Unit: second. Default: 0. Maximum: 7200 */
  uint32_t Reconnection_interval;
} W6X_WiFi_Connect_t;

/**
  * @brief  Wi-Fi Soft-AP configuration structure
  */
typedef struct
{
  uint8_t SSID[W6X_WIFI_MAX_SSID_SIZE + 1];         /*!< Service Set Identifier value. Wi-Fi Soft-AP name */
  uint8_t Password[W6X_WIFI_MAX_PASSWORD_SIZE + 1]; /*!< Password of Wi-Fi Soft-AP */
  W6X_WiFi_ApSecurityType_e Security;               /*!< Security encryption of the Wi-Fi Soft-AP */
  uint32_t Channel;                                 /*!< Channel Wi-Fi is operating at */
  uint32_t MaxConnections;                          /*!< Max number of stations that Soft-AP will allow to connect. Range [1,4] */
  uint32_t Hidden;                                  /*!< Flag to indicate if the SSID is hidden. 0: broadcasting SSID, 1: hidden SSID */
} W6X_WiFi_ApConfig_t;

/**
  * @brief  Wi-Fi connected station information
  */
typedef struct
{
  uint8_t IP[4];                                    /*!< IP address of connected station */
  uint8_t MAC[6];                                   /*!< MAC address of connected station */
} W6X_WiFi_Connected_Sta_Info_t;

/**
  * @brief  Wi-Fi connected station structure
  */
typedef struct
{
  uint32_t Count;                                             /*!< Number of connected stations */
  W6X_WiFi_Connected_Sta_Info_t STA[W6X_WIFI_MAX_CONNECTED_STATIONS];   /*!< Array of connected stations */
} W6X_WiFi_Connected_Sta_t;

/**
  * @brief  Wi-Fi TWT setup parameters
  */
typedef struct
{
  /** TWT Setup command (0: Request, 1: Suggest, 2: Demand) */
  uint8_t setup_type;
  /** Flow Type (0: Announced, 1: Unannounced) */
  uint8_t flow_type;
  /** Wake interval Exponent */
  uint8_t wake_int_exp;
  /** Nominal Minimum TWT Wake Duration */
  uint8_t min_twt_wake_dur;
  /** TWT Wake Interval Mantissa */
  uint16_t wake_int_mantissa;
} W6X_WiFi_TWT_Setup_Params_t;

/**
  * @brief  Wi-Fi TWT teardown parameters
  */
typedef struct
{
  /** Flag indicating whether to teardown all TWT flows */
  uint8_t all_twt;
  /** TWT flow ID */
  uint8_t id;
} W6X_WiFi_TWT_Teardown_Params_t;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_Net_Public_Types ST67W6X Net Types
  * @ingroup  ST67W6X_API_Net
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Callback for Net events. the type of event_args depends of the event_id:
  *         - W6X_Net_CbParamData_t for all existing events
  */
typedef void (*W6X_Net_App_cb_t)(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  Callback parameters for the Net application
  */
typedef struct
{
  uint32_t socket_id;             /*!< Socket ID */
  uint32_t available_data_length; /*!< Length of the available data */
} W6X_Net_CbParamData_t;

/**
  * @brief  Socket length type
  */
typedef size_t socklen_t;

/**
  * @brief  Socket address structure for Internet address family
  */
typedef uint8_t sa_family_t;

/**
  * @brief  Socket address structure for Internet address family
  */
typedef uint16_t in_port_t;

/**
  * @brief  IPv4 address union
  */
struct in_addr
{
  union
  {
    uint8_t s4_addr[4];         /*!< IPv4 address 1-byte type array */
    uint16_t s4_addr16[2];      /*!< IPv4 address 2-bytes type array */
    uint32_t s4_addr32[1];      /*!< IPv4 address 4-bytes type array */
    uint32_t s_addr;            /*!< IPv4 address 4-bytes type */
  };
};

/**
  * @brief  IPv6 address union
  */
struct in6_addr
{
  union
  {
    uint32_t u32_addr[4];       /*!< IPv6 address 4-bytes type array */
    uint8_t  u8_addr[16];       /*!< IPv6 address 1-byte type array */
  } un;                         /*!< IPv6 address union */
};

/**
  * @brief  Socket IPv4 address structure for Internet address family
  * @note   Members are in network byte order
  */
struct sockaddr_in
{
  uint8_t         sin_len;                /*!< Length of this structure */
  sa_family_t     sin_family;             /*!< AF_INET */
  in_port_t       sin_port;               /*!< Transport layer port # */
  struct in_addr  sin_addr_t;             /*!< IPv4 address */
#define SIN_ZERO_LEN 8                    /*!< Reserved */
  char            sin_zero[SIN_ZERO_LEN]; /*!< Reserved */
};

/**
  * @brief  Socket IPv6 address structure for Internet address family
  * @note   Members are in network byte order
  */
struct sockaddr_in6
{
  uint8_t         sin6_len;               /*!< Length of this structure */
  sa_family_t     sin6_family;            /*!< AF_INET6 */
  in_port_t       sin6_port;              /*!< Transport layer port # */
  uint32_t        sin6_flowinfo;          /*!< IPv6 flow information */
  struct in6_addr sin6_addr_t;            /*!< IPv6 address */
  uint32_t        sin6_scope_id;          /*!< Set of interfaces for scope */
};

/**
  * @brief  Socket address structure for Internet address family
  */
struct sockaddr
{
  uint8_t     sa_len;                     /*!< Length of this structure */
  sa_family_t sa_family;                  /*!< Address family */
  char        sa_data[14];                /*!< Address data */
};

/**
  * @brief  Socket address structure paddings design
  */
struct sockaddr_storage
{
  uint8_t     s2_len;                     /*!< Length of this structure */
  sa_family_t ss_family;                  /*!< Address family */
  char        s2_data1[2];                /*!< Transport layer port # as raw */
  uint32_t    s2_data2[3];                /*!< Reserved */
#if NET_IPV6
  uint32_t    s2_data3[3];                /*!< Reserved */
#endif /* NET_IPV6 */
};

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_MQTT_Public_Types ST67W6X MQTT Types
  * @ingroup  ST67W6X_API_MQTT
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Callback for MQTT events. the type of event_args depends of the event_id:
  *         - W6X_MQTT_CbParamData_t for [W6X_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID]
  *         The callback shall be re-entrant because the lower driver can call it from
  *         different tasks: (W61_ATD_RxPooling_task and W61_ATD_EventsPooling_task with different prio)
  */
typedef void (*W6X_MQTT_App_cb_t)(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  Callback parameters for the MQTT application
  */
typedef struct
{
  uint32_t topic_length;          /*!< Length of the topic */
  uint32_t message_length;        /*!< Length of the message */
} W6X_MQTT_CbParamData_t;

/**
  * @brief  MQTT connection structure
  */
typedef struct
{
  uint32_t State;           /*!< Connection state ::W6X_MQTT_State_e */
  /** Authentication scheme (0: No security, 1: Username/Password, 2: CACertificate, 4: CACertificate/PrivateKey) */
  uint32_t Scheme;
  uint32_t HostPort;        /*!< Host port */
  uint8_t HostName[64];     /*!< Host name */
  uint8_t MQClientId[32];   /*!< Client ID */
  uint8_t MQUserName[32];   /*!< User name. Not required when the scheme is greater or equal to 1 */
  uint8_t MQUserPwd[32];    /*!< User password. Not required when the scheme is greater or equal to 1 */
  uint8_t Certificate[64];  /*!< Client Certificate. Required when the scheme is greater or equal to 2 */
  uint8_t PrivateKey[64];   /*!< Client Private key. Required when the scheme is greater or equal to 2 */
  uint8_t CACertificate[64];/*!< CA certificate. Required when the scheme is greater or equal to 2 */
  uint32_t SNI_enabled;     /*!< Server Name Indication (SNI) enabled */
} W6X_MQTT_Connect_t;

/**
  * @brief  MQTT data structure
  */
typedef struct
{
  uint8_t *p_recv_data;           /*!< Pointer to Data buffer allocated by the application */
  uint32_t recv_data_buf_size;    /*!< Length of the buffer to contain received topic + message strings */
} W6X_MQTT_Data_t;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_BLE_Public_Types ST67W6X BLE Types
  * @ingroup  ST67W6X_API_BLE
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Callback for BLE events. the type of event_args depends of the event_id:\
  *         - W6X_Ble_CbParamData_t for all existing events
  *         The callback shall be re-entrant because the lower driver can call it from
  *         different tasks: (W61_ATD_RxPooling_task and W61_ATD_EventsPooling_task with different prio)
  */
typedef void (*W6X_Ble_App_cb_t)(W6X_event_id_t event_id, void *event_args);

/**
  * @brief  BLE characteristic structure
  */
typedef struct
{
  uint8_t char_idx;                                     /*!< Characteristic index */
  uint8_t uuid_type;                                    /*!< UUID type (16-bit or 128-bit) */
  char char_uuid[W6X_BLE_MAX_UUID_SIZE];                /*!< Characteristic UUID */
  uint8_t char_property;                                /*!< Characteristic Property */
  uint8_t char_permission;                              /*!< Characteristic Permission */
  uint8_t char_handle;                                  /*!< Characteristic handle */
  uint8_t char_value_handle;                            /*!< Characteristic value handle */
} W6X_Ble_Characteristic_t;

/**
  * @brief  BLE service structure
  */
typedef struct
{
  uint8_t service_idx;                                  /*!< Service index */
  uint8_t service_type;                                 /*!< Service type */
  uint8_t uuid_type;                                    /*!< UUID type (16-bit or 128-bit) */
  char service_uuid[W6X_BLE_MAX_UUID_SIZE];             /*!< Service UUID */
  W6X_Ble_Characteristic_t charac[W6X_BLE_MAX_CHAR_NBR];/*!< Service characteristics */
} W6X_Ble_Service_t;

/**
  * @brief  BLE security options structure
  */
typedef struct
{
  W6X_Ble_SecurityParameter_e sec_param;                /*!< Security parameter */
  uint8_t security_level;                               /*!< Security level */
} W6X_Ble_Security_Opts_t;

/**
  * @brief  BLE device structure
  */
typedef struct
{
  int16_t RSSI;                                       /*!< Signal strength of BLE connection */
  uint8_t IsConnected;                                /*!< Connection status */
  uint8_t conn_handle;                                /*!< Connection handle */
  uint8_t DeviceName[W6X_BLE_DEVICE_NAME_SIZE];       /*!< BLE device name */
  uint8_t ManufacturerData[W6X_BLE_MANUF_DATA_SIZE];  /*!< Device manufacturer data */
  uint8_t BDAddr[W6X_BLE_BD_ADDR_SIZE];               /*!< BD address of BLE Device */
  uint8_t bd_addr_type;                               /*!< Type of BD address */
  W6X_Ble_Service_t Service[W6X_BLE_MAX_SERVICE_NBR]; /*!< BLE services */
} W6X_Ble_Device_t;

/**
  * @brief  BLE scanned device structure
  */
typedef struct
{
  int16_t RSSI;                                      /*!< Signal strength of BLE connection */
  uint8_t DeviceName[W6X_BLE_DEVICE_NAME_SIZE];      /*!< BLE device name */
  uint8_t ManufacturerData[W6X_BLE_MANUF_DATA_SIZE]; /*!< Device manufacturer data */
  uint8_t BDAddr[W6X_BLE_BD_ADDR_SIZE];              /*!< BD address of BLE Device */
  uint8_t bd_addr_type;                              /*!< Type of BD address */
} W6X_Ble_Scanned_Device_t;

/**
  * @brief  BLE bonded device structure
  */
typedef struct
{
  uint8_t BDAddr[W6X_BLE_BD_ADDR_SIZE];              /*!< BD address of BLE Device */
} W6X_Ble_Bonded_Device_t;

/**
  * @brief  Callback parameters for the BLE application
  */
typedef struct
{
  uint8_t service_idx;                                    /*!< Service index */
  uint8_t charac_idx;                                     /*!< Characteristic index */
  uint8_t notification_status[2];                         /*!< Notification status */
  uint8_t indication_status[2];                           /*!< Indication status */
  uint16_t mtu_size;                                      /*!< MTU size */
  uint32_t PassKey;                                       /*!< BLE Security passkey */
  uint32_t available_data_length;                         /*!< Length of the available data */
  W6X_Ble_Device_t remote_ble_device;                     /*!< BLE Remote device */
} W6X_Ble_CbParamData_t;

/**
  * @brief  BLE scan options structure
  */
typedef struct
{
  W6X_Ble_ScanType_e scan_type;                    /*!< Scan type (1: Active, 0: Passive) */
  W6X_Ble_AddrType_e own_addr_type;                /*!< BLE Address type */
  W6X_Ble_FilterPolicy_e filter_policy;            /*!< BLE Scan filter policy */
  /** BLE Scan interval. The scan interval equals this parameter multiplied by 0.625 ms.
    * Should be more than or equal to scan_window. */
  uint16_t scan_interval;
  /** BLE Scan window. The scan window equals this parameter multiplied by 0.625 ms.
    * Should be less than or equal to scan_interval.*/
  uint16_t scan_window;
} W6X_Ble_Scan_Opts_t;

/**
  * @brief  Scan results structure
  */
typedef struct
{
  uint32_t Count;                                   /*!< Number of BLE peripherals detected */
  W6X_Ble_Scanned_Device_t *Detected_Peripheral;    /*!< Array of BLE peripherals information. */
} W6X_Ble_Scan_Result_t;

/**
  * @brief  Scan results structure
  */
typedef struct
{
  uint32_t Count;                                                     /*!< Number of BLE bonded device detected */
  W6X_Ble_Bonded_Device_t Bonded_device[W6X_BLE_MAX_BONDED_DEVICES];  /*!< Array of BLE bonded devices information. */
} W6X_Ble_Bonded_Devices_Result_t;

/**
  * @brief  BLE Scan result callback
  */
typedef void(* W6X_Ble_Scan_Result_cb_t)(W6X_Ble_Scan_Result_t *entry);

/**
  * @brief  BLE Connection options structure
  */
typedef struct
{
  W6X_Ble_AddrType_e remote_addr_type;              /*!< BLE Address type */
  uint8_t BDAddr[W6X_BLE_BD_ADDR_SIZE];             /*!< BD address of BLE Device */
  uint32_t timeout;                                 /*!< Connection timeout (ms) */
} W6X_Ble_Connect_Opts_t;

/** @} */

/* ===================================================================== */
/** @addtogroup ST67W6X_API_System_Public_Types
  * @{
  */
/* ===================================================================== */
/**
  * @brief  Callbacks for the application
  */
typedef struct
{
  W6X_WiFi_App_cb_t     APP_wifi_cb;                            /*!< Callback for Wi-Fi events */
  W6X_Net_App_cb_t      APP_net_cb;                             /*!< Callback for Network events */
  W6X_MQTT_App_cb_t     APP_mqtt_cb;                            /*!< Callback for MQTT events */
  W6X_Ble_App_cb_t      APP_ble_cb;                             /*!< Callback for BLE events */
  W6X_Error_cb_t        APP_error_cb;                           /*!< Callback for error events */
} W6X_App_Cb_t;

/** @} */

/* ===================================================================== */
/** @defgroup ST67W6X_API_HTTP_Public_Types ST67W6X HTTP Types
  * @ingroup  ST67W6X_API_HTTP
  * @{
  */
/* ===================================================================== */

/**
  * @brief  HTTP Data buffer structure
  */
typedef struct
{
  uint8_t data[4096];         /*!< Data buffer */
  int32_t length;             /*!< Data length */
} W6X_HTTP_buffer_t;

/**
  * @brief  HTTP IPv4 address structure
  */
typedef uint32_t ip4_addr_t;

/**
  * @brief  HTTP IPv6 address structure
  */
typedef struct ip6_addr
{
  uint32_t addr[4];           /*!< IPv6 address */
} ip6_addr_t;

/**
  * @brief  HTTP IP address structure
  */
typedef struct ip_addr
{
  union
  {
    ip6_addr_t ip6;           /*!< IPv6 address */
    ip4_addr_t ip4;           /*!< IPv4 address */
  } u_addr;                   /*!< IP address union */
  uint8_t type;               /*!< IP address type */
} ip_addr_t;

/**
  * @brief  HTTP connection state
  */
typedef int32_t W6X_HTTP_state_t;

/**
  * @brief  HTTP result callback
  * @param  arg: Callback argument
  * @param  httpc_result: HTTP result
  * @param  rx_content_len: Received content length
  * @param  srv_res: Server response
  * @param  err: Error code
  */
typedef void (*W6X_HTTP_result_cb_t)(void *arg, W6X_HTTP_Status_Code_e httpc_result, uint32_t rx_content_len,
                                     uint32_t srv_res, int32_t err);

/**
  * @brief  HTTP headers done callback
  * @param  connection: HTTP connection state
  * @param  arg: Callback argument
  * @param  hdr: Header
  * @param  hdr_len: Header length
  * @param  content_len: Content length
  */
typedef int32_t (*W6X_HTTP_headers_done_cb_t)(W6X_HTTP_state_t *connection, void *arg, uint8_t *hdr, uint16_t hdr_len,
                                              uint32_t content_len);

/**
  * @brief  HTTP data callback
  * @param  arg: Callback argument
  * @param  p: Pointer to data
  * @param  err: Error code
  */
typedef int32_t (*W6X_HTTP_data_cb_t)(void *arg, W6X_HTTP_buffer_t *p, int32_t err);

/**
  * @brief  HTTP POST data structure
  */
typedef struct
{
  W6X_HTTP_Content_Type_e type;               /*!< HTTP POST/PUT Content type */
  char *data;                                 /*!< Pointer to data buffer */
} W6X_HTTP_Post_Data_t;

/**
  * @brief  HTTP connection structure
  */
typedef struct
{
  uint8_t use_proxy;                          /*!< Use proxy */
  uint16_t proxy_port;                        /*!< Proxy port */
  const ip_addr_t *proxy_addr;                /*!< Proxy address */
  char *https_certificate;                    /*!< HTTPS certificate */
  uint32_t https_certificate_len;             /*!< HTTPS certificate length */
  char *server_name;                          /*!< Server name */
  W6X_HTTP_headers_done_cb_t headers_done_fn; /*!< Headers done callback */
  W6X_HTTP_result_cb_t result_fn;             /*!< Result callback */
  void *callback_arg;                         /*!< Callback argument */
  W6X_HTTP_data_cb_t recv_fn;                 /*!< Receive callback */
  void *recv_fn_arg;                          /*!< Receive callback argument */
  uint32_t timeout;                           /*!< Timeout */
  uint32_t max_response_len;                  /*!< Maximum response length */
} W6X_HTTP_connection_t;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* W6X_TYPES_H */
