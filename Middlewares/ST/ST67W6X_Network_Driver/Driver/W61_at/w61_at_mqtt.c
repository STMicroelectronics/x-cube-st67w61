/**
  ******************************************************************************
  * @file    w61_at_mqtt.c
  * @author  GPM Application Team
  * @brief   This file provides code for W61 MQTT AT module
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
#include <inttypes.h>
#include <stdio.h>
#include "w61_at_api.h"
#include "w61_at_common.h"
#include "w61_at_internal.h"
#include "common_parser.h" /* Common Parser functions */

#if (SYS_DBG_ENABLE_TA4 >= 1)
#include "trcRecorder.h"
#endif /* SYS_DBG_ENABLE_TA4 */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_MQTT_Constants
  * @{
  */

#define W61_MQTT_CONNECT_TIMEOUT 10000 /*!< MQTT connect timeout in ms */

/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_MQTT_Functions
  * @{
  */

/**
  * @brief  Callback function to handle MQTT get subscription responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_getsub);

/**
  * @brief  Callback function to handle MQTT unsubscribe responses
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @param  argv: array of argument strings
  * @param  argc: number of argument
  * @return 0 on success, negative value on error
 */
MODEM_CMD_DECLARE(on_cmd_getunsub);

/**
  * @brief  Parses MQTT event and call related callback
  * @param  hObj: pointer to module handle
  * @param  argc: pointer to argument count
  * @param  argv: pointer to argument values
  */
static void W61_MQTT_AT_Event(void *hObj, uint16_t *argc, char **argv);

/**
  * @brief  Callback function to handle MQTT data events
  * @param  event_id: event ID
  * @param  data: pointer to the modem_cmd_handler_data structure
  * @param  len: length of the data
  * @return data length on success, negative value on error
  */
static int32_t W61_MQTT_Data_Event(uint32_t event_id, struct modem_cmd_handler_data *data, uint16_t len);

/* Functions Definition ------------------------------------------------------*/
W61_Status_t W61_MQTT_Init(W61_Object_t *Obj, uint8_t *p_recv_data, uint32_t recv_data_buf_len)
{
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(p_recv_data);

  /* Initialize the MQTT data buffer. Used to store received MQTT messages */
  Obj->MQTTCtx.AppBuffRecvData = p_recv_data;
  Obj->MQTTCtx.AppBuffRecvDataSize = recv_data_buf_len;

  /* Register the MQTT event callbacks */
  Obj->Callbacks.MQTT_event_cb = W61_MQTT_AT_Event;
  Obj->Callbacks.MQTT_event_data_cb = W61_MQTT_Data_Event;

  return W61_STATUS_OK;
}

W61_Status_t W61_MQTT_DeInit(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  Obj->MQTTCtx.AppBuffRecvData = NULL;
  Obj->MQTTCtx.AppBuffRecvDataSize = 0;

  Obj->Callbacks.MQTT_event_cb = NULL;
  Obj->Callbacks.MQTT_event_data_cb = NULL;

  return W61_STATUS_OK;
}

W61_Status_t W61_MQTT_SetUserConfiguration(W61_Object_t *Obj, uint32_t Scheme, uint8_t *ClientId, uint8_t *Username,
                                           uint8_t *Password, uint8_t *Certificate, uint8_t *PrivateKey,
                                           uint8_t *CaCertificate)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ClientId);
  W61_NULL_ASSERT(Username);
  W61_NULL_ASSERT(Password);
  W61_NULL_ASSERT(Certificate);
  W61_NULL_ASSERT(PrivateKey);
  W61_NULL_ASSERT(CaCertificate);

  if (ClientId[0] == '\0')
  {
    return W61_STATUS_ERROR; /* ClientId must not be empty */
  }

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+MQTTUSERCFG=0,%" PRIu32 ",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\r\n",
           Scheme, ClientId, Username, Password, Certificate, PrivateKey, CaCertificate);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_MQTT_GetUserConfiguration(W61_Object_t *Obj, uint8_t ClientId[32], uint8_t Username[32],
                                           uint8_t Password[32], uint8_t Certificate[64], uint8_t PrivateKey[64],
                                           uint8_t CaCertificate[64])
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ClientId);
  W61_NULL_ASSERT(Username);
  W61_NULL_ASSERT(Password);
  W61_NULL_ASSERT(Certificate);
  W61_NULL_ASSERT(PrivateKey);
  W61_NULL_ASSERT(CaCertificate);

  /* Initialize the output parameters */
  ClientId[0] = '\0';
  Username[0] = '\0';
  Password[0] = '\0';
  Certificate[0] = '\0';
  PrivateKey[0] = '\0';
  CaCertificate[0] = '\0';

  strncpy(cmd, "AT+MQTTUSERCFG?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+MQTTUSERCFG:", &argc, argv, W61_NCP_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 8)
  {
    return W61_STATUS_ERROR;
  }

  W61_AT_RemoveStrQuotes(argv[2]); /* ClientId */
  strncpy((char *)ClientId, argv[2], 31);
  ClientId[31] = '\0';  /* Ensure null termination */

  W61_AT_RemoveStrQuotes(argv[3]); /* Username */
  strncpy((char *)Username, argv[3], 31);
  Username[31] = '\0';  /* Ensure null termination */

  W61_AT_RemoveStrQuotes(argv[4]); /* Password */
  strncpy((char *)Password, argv[4], 31);
  Password[31] = '\0';  /* Ensure null termination */

  W61_AT_RemoveStrQuotes(argv[5]); /* Certificate */
  strncpy((char *)Certificate, argv[5], 63);
  Certificate[63] = '\0';  /* Ensure null termination */

  W61_AT_RemoveStrQuotes(argv[6]); /* Private Key */
  strncpy((char *)PrivateKey, argv[6], 63);
  PrivateKey[63] = '\0';  /* Ensure null termination */

  W61_AT_RemoveStrQuotes(argv[7]); /* CA Certificate */
  strncpy((char *)CaCertificate, argv[7], 63);
  CaCertificate[63] = '\0';  /* Ensure null termination */

  return ret;
}

W61_Status_t W61_MQTT_SetConfiguration(W61_Object_t *Obj, uint32_t KeepAlive, uint32_t DisableCleanSession,
                                       uint8_t *WillTopic, uint8_t *WillMessage, uint32_t WillQos,
                                       uint32_t WillRetain)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE,
           "AT+MQTTCONNCFG=0,%" PRIu32 ",%" PRIu32 ",\"%s\",\"%s\",%" PRIu32 ",%" PRIu32 "\r\n",
           KeepAlive, DisableCleanSession, WillTopic, WillMessage, WillQos, WillRetain);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_MQTT_GetConfiguration(W61_Object_t *Obj, uint32_t *KeepAlive, uint32_t *DisableCleanSession,
                                       uint8_t WillTopic[128], uint8_t WillMessage[128], uint32_t *WillQos,
                                       uint32_t *WillRetain)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  uint16_t argc = 0;
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(KeepAlive);
  W61_NULL_ASSERT(DisableCleanSession);
  W61_NULL_ASSERT(WillTopic);
  W61_NULL_ASSERT(WillMessage);
  W61_NULL_ASSERT(WillQos);
  W61_NULL_ASSERT(WillRetain);

  strncpy(cmd, "AT+MQTTCONNCFG?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+MQTTCONNCFG:", &argc, argv, W61_NET_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 7)
  {
    return W61_STATUS_ERROR;
  }

  *KeepAlive = atoi(argv[1]);
  *DisableCleanSession = atoi(argv[2]);

  W61_AT_RemoveStrQuotes(argv[3]); /* WillTopic */
  strncpy((char *)WillTopic, argv[3], 127);
  WillTopic[127] = '\0';  /* Ensure null termination */

  W61_AT_RemoveStrQuotes(argv[4]); /* WillMessage */
  strncpy((char *)WillMessage, argv[4], 127);
  WillMessage[127] = '\0';  /* Ensure null termination */

  *WillQos = atoi(argv[5]);
  *WillRetain = atoi(argv[6]);

  return ret;
}

W61_Status_t W61_MQTT_SetSNI(W61_Object_t *Obj, uint8_t *SNI)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(SNI);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+MQTTSNI=0,\"%s\"\r\n", SNI);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_MQTT_GetSNI(W61_Object_t *Obj, uint8_t SNI[128])
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(SNI);

  strncpy(cmd, "AT+MQTTSNI?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+MQTTSNI:", &argc, argv, W61_NET_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 2)
  {
    return W61_STATUS_ERROR;
  }

  /* Verify the SNI is valid */
  if (strncmp(argv[1], "\"", 1) == 0)
  {
    W61_AT_RemoveStrQuotes(argv[1]); /* Remove quotes from the string */
    strncpy((char *)SNI, argv[1], 127);
    SNI[127] = '\0';  /* Ensure null termination */
  }
  else
  {
    ret = W61_STATUS_ERROR;
  }

  return ret;
}

W61_Status_t W61_MQTT_Connect(W61_Object_t *Obj, uint8_t *Host, uint16_t Port)
{
  uint8_t reconnect = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Host);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+MQTTCONN=0,\"%s\",%" PRIu16 ",%" PRIu16 "\r\n", Host, Port, reconnect);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_MQTT_CONNECT_TIMEOUT);
}

W61_Status_t W61_MQTT_GetConnectionStatus(W61_Object_t *Obj, uint8_t *Host, uint32_t *Port,
                                          uint32_t *Scheme, uint32_t *State)
{
  char *argv[CONFIG_MODEM_CMD_HANDLER_MAX_PARAM_COUNT];
  char cmd[W61_CMD_MATCH_BUFF_SIZE];
  uint16_t argc = 0;
  W61_Status_t ret;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Host);
  W61_NULL_ASSERT(Port);
  W61_NULL_ASSERT(Scheme);
  W61_NULL_ASSERT(State);

  strncpy(cmd, "AT+MQTTCONN?\r\n", sizeof(cmd));
  ret = W61_AT_Common_Query_Parse(Obj, cmd, "+MQTTCONN:", &argc, argv, W61_NET_TIMEOUT);
  if (ret != W61_STATUS_OK)
  {
    return ret;
  }
  if (argc < 6)
  {
    return W61_STATUS_ERROR;
  }

  *State = atoi(argv[1]);
  *Scheme = atoi(argv[2]);
  W61_AT_RemoveStrQuotes(argv[3]);
  strncpy((char *)Host, argv[3], 63);
  *Port = atoi(argv[4]);

  return ret;
}

W61_Status_t W61_MQTT_Disconnect(W61_Object_t *Obj)
{
  W61_NULL_ASSERT(Obj);

  return W61_AT_Common_SetExecute(Obj, (uint8_t *)"AT+MQTTCLEAN=0\r\n", W61_NET_TIMEOUT);
}

W61_Status_t W61_MQTT_Subscribe(W61_Object_t *Obj, uint8_t *Topic)
{
  uint32_t qos = 0;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Topic);

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+MQTTSUB=0,\"%s\",%" PRIu32 "\r\n", Topic, qos);
  return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
}

W61_Status_t W61_MQTT_GetSubscribedTopics(W61_Object_t *Obj)
{
  struct modem *mdm = (struct modem *) &Obj->Modem;
  W61_NULL_ASSERT(Obj);

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+MQTTSUB:", on_cmd_getsub, 4U, ","),
  };

  return W61_Status(modem_cmd_send(&mdm->iface,
                                   &mdm->modem_cmd_handler,
                                   handlers,
                                   ARRAY_SIZE(handlers),
                                   (const uint8_t *)"AT+MQTTSUB?\r\n",
                                   mdm->sem_response,
                                   W61_NET_TIMEOUT));
}

W61_Status_t W61_MQTT_Unsubscribe(W61_Object_t *Obj, uint8_t *Topic)
{
  struct modem *mdm = (struct modem *) &Obj->Modem;
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Topic);

  struct modem_cmd handlers[] =
  {
    MODEM_CMD("+MQTTUNSUB:", on_cmd_getunsub, 1U, ""),
  };

  snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+MQTTUNSUB=0,\"%s\"\r\n", Topic);
  return W61_Status(modem_cmd_send(&mdm->iface,
                                   &mdm->modem_cmd_handler,
                                   handlers,
                                   ARRAY_SIZE(handlers),
                                   (const uint8_t *)cmd,
                                   mdm->sem_response,
                                   W61_NET_TIMEOUT));
}

W61_Status_t W61_MQTT_Publish(W61_Object_t *Obj, uint8_t *Topic, uint8_t *Message, uint32_t Message_len, uint32_t Qos,
                              uint32_t Retain)
{
  char cmd[W61_CMDRSP_STRING_SIZE];
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Topic);
  W61_NULL_ASSERT(Message);

  if (Message_len > 0)
  {
    snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+MQTTPUBRAW=0,\"%s\",%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
             Topic, Message_len, Qos, Retain);
    return W61_AT_Common_RequestSendData(Obj, (uint8_t *)cmd, Message, Message_len, W61_NET_TIMEOUT, true);
  }
  else
  {
    snprintf(cmd, W61_CMDRSP_STRING_SIZE, "AT+MQTTPUB=0,\"%s\",\"\",%" PRIu32 ",%" PRIu32 "\r\n", Topic, Qos, Retain);
    return W61_AT_Common_SetExecute(Obj, (uint8_t *)cmd, W61_NET_TIMEOUT);
  }
}

/* Private Functions Definition ----------------------------------------------*/
MODEM_CMD_DEFINE(on_cmd_getsub)
{
  if (argc >= 4)
  {
    MQTT_LOG_DEBUG("LinkID: %" PRIu32 ", state: %" PRIu32 ", topic: %s, qos: %" PRIu32 "\n",
                   atoi((char *)argv[0]), atoi((char *)argv[1]), argv[2], atoi((char *)argv[3]));
  }

  return 0;
}

MODEM_CMD_DEFINE(on_cmd_getunsub)
{
  if ((argc >= 1) && (strcmp((char *)argv[0], "NO_UNSUBSCRIBE") == 0))
  {
    MQTT_LOG_WARN("No topic found\n");
  }

  return 0;
}

static void W61_MQTT_AT_Event(void *hObj, uint16_t *argc, char **argv)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;

  if ((Obj == NULL) || (Obj->ulcbs.UL_mqtt_cb == NULL) || (*argc < 2))
  {
    return;
  }

  /* Argv[0] = link id. not used */
  if (strcmp(argv[0], "CONNECTED") == 0)
  {
    Obj->ulcbs.UL_mqtt_cb(W61_MQTT_EVT_CONNECTED_ID, NULL);
    return;
  }

  if (strcmp(argv[0], "DISCONNECTED") == 0)
  {
    Obj->ulcbs.UL_mqtt_cb(W61_MQTT_EVT_DISCONNECTED_ID, NULL);
    return;
  }
}

static int32_t W61_MQTT_Data_Event(uint32_t event_id, struct modem_cmd_handler_data *data, uint16_t len)
{
  struct modem *mdm = (struct modem *)data->user_data;
  W61_Object_t *Obj = CONTAINER_OF(mdm, W61_Object_t, Modem);
  W61_MQTT_CbParamData_t cb_param_mqtt_data;
  uint8_t *ptr;
  uint8_t *endptr;
  uint32_t header_len;
  uint32_t topic_len;
  uint32_t message_len;
  uint16_t rx_data_len;

  if ((data->rx_buf == NULL) || (Obj->MQTTCtx.AppBuffRecvData == NULL))
  {
    return -EINVAL;
  }

  ptr = &data->rx_buf[len + 2]; /* Skip the link id and the comma */
  data->rx_buf[data->rx_buf_len] = 0;

  /* Get the topic length */
  topic_len = strtol((char *)ptr, (char **)&endptr, 10);
  if ((endptr == ptr) || (*endptr != ','))
  {
    return -EINVAL;
  }
  topic_len += 2; /* Include double-quotes */
  endptr++; /* Skip the comma */

  /* Get the message length */
  message_len = strtol((char *)endptr, (char **)&endptr, 10);
  if ((endptr == ptr) || (*endptr != ','))
  {
    return -EINVAL;
  }
  endptr++; /* Skip the comma */

  /* Check if the buffer is large enough to hold the topic and message */
  if ((topic_len + message_len + 1) > Obj->MQTTCtx.AppBuffRecvDataSize) /* +1 for the intermediate comma */
  {
    return -ENOMEM;
  }

  header_len = endptr - data->rx_buf;
  rx_data_len = header_len + topic_len + message_len + 1; /* +1 for the intermediate comma */
  if (data->rx_buf_len >= rx_data_len)
  {
    /* Copy the topic */
    memcpy(Obj->MQTTCtx.AppBuffRecvData, endptr, topic_len);
    /* Null-terminate the topic */
    Obj->MQTTCtx.AppBuffRecvData[topic_len] = '\0';
    endptr += topic_len + 1; /* Skip the comma after topic */
    /* Copy the message */
    memcpy(&Obj->MQTTCtx.AppBuffRecvData[topic_len + 1], endptr, message_len);
    cb_param_mqtt_data.topic_length = topic_len;
    cb_param_mqtt_data.message_length = message_len;
    if (Obj->ulcbs.UL_mqtt_cb != NULL)
    {
      Obj->ulcbs.UL_mqtt_cb(W61_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID, &cb_param_mqtt_data);
    }
    return rx_data_len;
  }
  else
  {
    return -EAGAIN;
  }
}
/** @} */
