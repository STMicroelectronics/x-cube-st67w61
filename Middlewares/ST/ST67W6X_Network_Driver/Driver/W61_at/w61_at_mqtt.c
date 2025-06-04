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
#include "w61_at_rx_parser.h"
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

#define W61_MQTT_EVT_CONNECTED_KEYWORD             "+MQTT:0,CONNECTED"      /*!< Connected event keyword */
#define W61_MQTT_EVT_DISCONNECTED_KEYWORD          "+MQTT:0,DISCONNECTED"   /*!< Disconnected event keyword */
#define W61_MQTT_EVT_SUBSCRIPTION_RECEIVED_KEYWORD "+MQTT:SUBRECV"          /*!< Subscription received event keyword */
#define W61_MQTT_EVT_GET_SUBSCRIPTION_KEYWORD      "+MQTTSUB:"              /*!< Get subscription event keyword */

#define W61_MQTT_CONNECT_TIMEOUT 10000 /*!< MQTT connect timeout in ms */
/** @} */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_MQTT_Functions
  * @{
  */

/**
  * @brief  Parses MQTT event and call related callback.
  * @param  hObj: pointer to module handle
  * @param  p_evt: pointer to event buffer
  * @param  evt_len: event length
  */
static void W61_MQTT_AT_Event(void *hObj, const uint8_t *p_evt, int32_t evt_len);

/* Functions Definition ------------------------------------------------------*/
W61_Status_t W61_MQTT_Init(W61_Object_t *Obj, uint8_t *p_recv_data, uint32_t recv_data_buf_len)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(p_recv_data);

  Obj->MQTTCtx.AppBuffRecvData = p_recv_data;
  Obj->MQTTCtx.AppBuffRecvDataSize = recv_data_buf_len;

  Obj->MQTT_event_cb = W61_MQTT_AT_Event;

  return W61_STATUS_OK;
}

W61_Status_t W61_MQTT_DeInit(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  Obj->MQTTCtx.AppBuffRecvData = NULL;
  Obj->MQTTCtx.AppBuffRecvDataSize = 0;

  Obj->MQTT_event_cb = NULL;

  return W61_STATUS_OK;
}

W61_Status_t W61_MQTT_SetUserConfiguration(W61_Object_t *Obj, uint32_t Scheme, uint8_t *ClientId, uint8_t *Username,
                                           uint8_t *Password, uint8_t *Certificate, uint8_t *PrivateKey,
                                           uint8_t *CaCertificate)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(ClientId);
  W61_NULL_ASSERT(Username);
  W61_NULL_ASSERT(Password);
  W61_NULL_ASSERT(Certificate);
  W61_NULL_ASSERT(PrivateKey);
  W61_NULL_ASSERT(CaCertificate);

  if (ClientId[0] == '\0')
  {
    return ret;
  }

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+MQTTUSERCFG=0,%" PRIu32 ",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\r\n",
             Scheme, ClientId, Username, Password, Certificate, PrivateKey, CaCertificate);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_NET_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_GetUserConfiguration(W61_Object_t *Obj, uint8_t *ClientId, uint8_t *Username,
                                           uint8_t *Password, uint8_t *Certificate, uint8_t *PrivateKey,
                                           uint8_t *CaCertificate)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t cmp = -1;
  int32_t recv_len;
  char *parameters_strings[6];
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

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTUSERCFG?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, W61_NET_TIMEOUT);
      if (recv_len > 0)
      {
        Obj->CmdResp[recv_len] = 0;
        cmp = strncmp((char *) Obj->CmdResp, "+MQTTUSERCFG:", sizeof("+MQTTUSERCFG:") - 1);
      }
      else
      {
        ret = W61_STATUS_TIMEOUT;
      }

      if (cmp == 0)
      {
        char *end = strstr((char *)Obj->CmdResp, "\r");
        if (end)
        {
          *end = 0;
        }
        strtok((char *) Obj->CmdResp + sizeof("+MQTTUSERCFG:") - 1, ","); /* Skip link_id */
        strtok(NULL, ","); /* Skip Scheme */
        for (uint32_t i = 0; i < 6; i++)
        {
          parameters_strings[i] = strtok(NULL, ",");
          if (parameters_strings[i] == NULL)
          {
            break;
          }
        }
        strtok((char *)Obj->CmdResp, "\r\n");

        /* For each parameter, check if not empty between double quotes */
        /* Client ID */
        if ((parameters_strings[0] != NULL) && (strlen(parameters_strings[0]) >= 2))
        {
          strncpy((char *)ClientId, &(parameters_strings[0][1]), strlen(parameters_strings[0]) - 2);
          ClientId[strlen(parameters_strings[0]) - 2] = '\0';
        }

        /* Username */
        if ((parameters_strings[1] != NULL) && (strlen(parameters_strings[1]) >= 2))
        {
          strncpy((char *)Username, &(parameters_strings[1][1]), strlen(parameters_strings[1]) - 2);
          Username[strlen(parameters_strings[1]) - 2] = '\0';
        }

        /* Password */
        if ((parameters_strings[2] != NULL) && (strlen(parameters_strings[2]) >= 2))
        {
          strncpy((char *)Password, &(parameters_strings[2][1]), strlen(parameters_strings[2]) - 2);
          Password[strlen(parameters_strings[2]) - 2] = '\0';
        }

        /* Certificate */
        if ((parameters_strings[3] != NULL) && (strlen(parameters_strings[3]) >= 2))
        {
          strncpy((char *)Certificate, &(parameters_strings[3][1]), strlen(parameters_strings[3]) - 2);
          Certificate[strlen(parameters_strings[3]) - 2] = '\0';
        }

        /* Private Key */
        if ((parameters_strings[4] != NULL) && (strlen(parameters_strings[4]) >= 2))
        {
          strncpy((char *)PrivateKey, &(parameters_strings[4][1]), strlen(parameters_strings[4]) - 2);
          PrivateKey[strlen(parameters_strings[4]) - 2] = '\0';
        }

        /* CA Certificate */
        if ((parameters_strings[5] != NULL) && (strlen(parameters_strings[5]) >= 2))
        {
          strncpy((char *)CaCertificate, &(parameters_strings[5][1]), strlen(parameters_strings[5]) - 2);
          CaCertificate[strlen(parameters_strings[5]) - 2] = '\0';
        }

        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, Obj->NcpTimeout);
        if (recv_len > 0)
        {
          Obj->CmdResp[recv_len] = 0;
          ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
        }
        else
        {
          ret = W61_STATUS_ERROR;
        }
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_SetConfiguration(W61_Object_t *Obj, uint32_t KeepAlive, uint32_t CleanSession,
                                       uint8_t *WillTopic, uint8_t *WillMessage, uint32_t WillQos,
                                       uint32_t WillRetain)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+MQTTCONNCFG=0,%" PRIu32 ",%" PRIu32 ",\"%s\",\"%s\",%" PRIu32 ",%" PRIu32 "\r\n",
             KeepAlive, CleanSession, WillTopic, WillMessage, WillQos, WillRetain);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_NET_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_GetConfiguration(W61_Object_t *Obj, uint32_t *KeepAlive, uint32_t *CleanSession,
                                       uint8_t *WillTopic, uint8_t *WillMessage, uint32_t *WillQos,
                                       uint32_t *WillRetain)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t cmp;
  uint32_t link_id;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(KeepAlive);
  W61_NULL_ASSERT(CleanSession);
  W61_NULL_ASSERT(WillTopic);
  W61_NULL_ASSERT(WillMessage);
  W61_NULL_ASSERT(WillQos);
  W61_NULL_ASSERT(WillRetain);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTCONNCFG?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_NET_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      cmp = sscanf((const char *)Obj->CmdResp,
                   "+MQTTCONNCFG:%" SCNu32 ",%" SCNu32 ",\"%[^\"]\",\"%[^\"]\",%" SCNu32 ",%" SCNu32 "\r\n",
                   &link_id, CleanSession, WillTopic, WillMessage, WillQos, WillRetain);
      if (cmp < 2)
      {
        ret = W61_STATUS_ERROR;
      }
      else
      {
        *KeepAlive = link_id;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_SetSNI(W61_Object_t *Obj, uint8_t *SNI)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(SNI);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTSNI=0,\"%s\"\r\n", SNI);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_NET_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_GetSNI(W61_Object_t *Obj, uint8_t *SNI)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t cmp;
  uint32_t link_id;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(SNI);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTSNI?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_NET_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      cmp = sscanf((const char *)Obj->CmdResp,
                   "+MQTTSNI:%" SCNu32 ",\"%[^\"]\"\r\n", &link_id, SNI);
      if (cmp < 2)
      {
        ret = W61_STATUS_ERROR;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_Connect(W61_Object_t *Obj, uint8_t *Host, uint16_t Port)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint8_t reconnect = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Host);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+MQTTCONN=0,\"%s\",%" PRIu16 ",%" PRIu16 "\r\n",
             Host, Port, reconnect);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_MQTT_CONNECT_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_GetConnectionStatus(W61_Object_t *Obj, uint8_t *Host, uint32_t *Port,
                                          uint32_t *Scheme, uint32_t *State)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int32_t cmp;
  uint32_t link_id;
  uint32_t reconnect;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Host);
  W61_NULL_ASSERT(Port);
  W61_NULL_ASSERT(Scheme);
  W61_NULL_ASSERT(State);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTCONN?\r\n");
    ret = W61_AT_Query(Obj, Obj->CmdResp, Obj->CmdResp, W61_NET_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      cmp = sscanf((const char *)Obj->CmdResp,
                   "+MQTTCONN:%" SCNu32 ",%" SCNu32 ",%" SCNu32 ",\"%[^\"]\",%" SCNu32 ",%" SCNu32 "\r\n",
                   &link_id, State, Scheme, Host, Port, &reconnect);
      if (cmp < 2)
      {
        ret = W61_STATUS_ERROR;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_Disconnect(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTCLEAN=0\r\n");
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_NET_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_Subscribe(W61_Object_t *Obj, uint8_t *Topic)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t qos = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Topic);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+MQTTSUB=0,\"%s\",%" PRIu32 "\r\n", Topic, qos);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_NET_TIMEOUT);
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

W61_Status_t W61_MQTT_GetSubscribedTopics(W61_Object_t *Obj)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int16_t recv_len;
  uint32_t count;
  char *param_str[4] = {NULL};
  W61_NULL_ASSERT(Obj);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTSUB?\r\n");
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      do
      {
        /* Check the command has been sent correctly, OK is received */
        recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, 5000);
        if (recv_len > 0)
        {
          Obj->CmdResp[recv_len] = 0;
          if (strncmp((char *) Obj->CmdResp, W61_MQTT_EVT_GET_SUBSCRIPTION_KEYWORD,
                      sizeof(W61_MQTT_EVT_GET_SUBSCRIPTION_KEYWORD) - 1) == 0)
          {
            param_str[0] = strtok((char *) Obj->CmdResp + sizeof(W61_MQTT_EVT_GET_SUBSCRIPTION_KEYWORD) - 1, ",");
            for (count = 1; count < 4; count++)
            {
              param_str[count] = strtok(NULL, ",");
              if (param_str[count] == NULL)
              {
                break;
              }
            }
            LogDebug("LinkID: %s, state: %s, topic: %s, qos: %s\n",
                     param_str[0], param_str[1], param_str[2], param_str[3]);
          }
          else
          {
            /* Data received doesn't match the expected format. can be the last OK */
            break;
          }
        }
        else
        {
          ret = W61_STATUS_TIMEOUT;
          break;
        }
      } while (recv_len > 0);

      /* Check the command has been sent correctly, OK is received */
      if (recv_len > 0)
      {
        Obj->CmdResp[recv_len] = 0;
        ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_MQTT_Unsubscribe(W61_Object_t *Obj, uint8_t *Topic)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  int16_t recv_len;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Topic);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE, "AT+MQTTUNSUB=0,\"%s\"\r\n", Topic);
    if (W61_ATsend(Obj, Obj->CmdResp, strlen((char *)Obj->CmdResp), Obj->NcpTimeout) > 0)
    {
      /* Check the command has been sent correctly, OK is received */
      recv_len = W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, 5000);
      if (recv_len > 0)
      {
        Obj->CmdResp[recv_len] = 0;
        if (strncmp((char *) Obj->CmdResp, "+MQTTUNSUB:NO_UNSUBSCRIBE", sizeof("+MQTTUNSUB:NO_UNSUBSCRIBE") - 1) == 0)
        {
          /* Check the next line to flush the OK/ERROR message */
          (void)W61_ATD_Recv(Obj, Obj->CmdResp, W61_ATD_RSP_SIZE, 5000);

          /* Error because the topic is not subscribed */
          ret = W61_STATUS_ERROR;
        }
        else
        {
          ret = W61_AT_ParseOkErr((char *)Obj->CmdResp);
        }
      }
      else
      {
        ret = W61_STATUS_TIMEOUT;
      }
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }
  return ret;
}

W61_Status_t W61_MQTT_Publish(W61_Object_t *Obj, uint8_t *Topic, uint8_t *Message, uint32_t Message_len)
{
  W61_Status_t ret = W61_STATUS_ERROR;
  uint32_t qos = 0;
  uint32_t retain = 0;
  W61_NULL_ASSERT(Obj);
  W61_NULL_ASSERT(Topic);
  W61_NULL_ASSERT(Message);

  if (W61_ATlock(Obj, W61_AT_LOCK_TIMEOUT))
  {
    snprintf((char *)Obj->CmdResp, W61_ATD_CMDRSP_STRING_SIZE,
             "AT+MQTTPUBRAW=0,\"%s\",%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\r\n",
             Topic, Message_len, qos, retain);
    ret = W61_AT_SetExecute(Obj, Obj->CmdResp, W61_NET_TIMEOUT);
    if (ret == W61_STATUS_OK)
    {
      Obj->fops.IO_Delay(10);
      ret = W61_AT_RequestSendData(Obj, Message, Message_len, W61_NET_TIMEOUT);
    }
    W61_ATunlock(Obj);
  }
  else
  {
    ret = W61_STATUS_BUSY;
  }

  return ret;
}

/* Private Functions Definition ----------------------------------------------*/
static void W61_MQTT_AT_Event(void *hObj, const uint8_t *rxbuf, int32_t rxbuf_len)
{
  W61_Object_t *Obj = (W61_Object_t *)hObj;
  char *ptr = (char *)rxbuf;
  char *str_token;
  int32_t tmp = 0;
  W61_MQTT_CbParamData_t cb_param_mqtt_data;

  if ((Obj == NULL) || (Obj->ulcbs.UL_mqtt_cb == NULL))
  {
    return;
  }

  if (strncmp(ptr, W61_MQTT_EVT_CONNECTED_KEYWORD, sizeof(W61_MQTT_EVT_CONNECTED_KEYWORD) - 1) == 0)
  {
    Obj->ulcbs.UL_mqtt_cb(W61_MQTT_EVT_CONNECTED_ID, NULL);
    return;
  }

  if (strncmp(ptr, W61_MQTT_EVT_DISCONNECTED_KEYWORD, sizeof(W61_MQTT_EVT_DISCONNECTED_KEYWORD) - 1) == 0)
  {
    Obj->ulcbs.UL_mqtt_cb(W61_MQTT_EVT_DISCONNECTED_ID, NULL);
    return;
  }

  if (strncmp(ptr, W61_MQTT_EVT_SUBSCRIPTION_RECEIVED_KEYWORD,
              sizeof(W61_MQTT_EVT_SUBSCRIPTION_RECEIVED_KEYWORD) - 1) == 0)
  {
    ptr = ptr + sizeof(W61_MQTT_EVT_SUBSCRIPTION_RECEIVED_KEYWORD);

    /* Link ID - unused */
    str_token = strtok(ptr, ",");
    if (str_token == NULL)
    {
      return;
    }

    /* Topic length */
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }

    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_mqtt_data.topic_length = (uint32_t)tmp;
    /* Increment the size of 2 to include the quotes */
    cb_param_mqtt_data.topic_length += 2;

    /* Message length */
    str_token = strtok(NULL, ",");
    if (str_token == NULL)
    {
      return;
    }

    Parser_StrToInt(str_token, NULL, &tmp);
    cb_param_mqtt_data.message_length = (uint32_t)tmp;

    Obj->ulcbs.UL_mqtt_cb(W61_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID, &cb_param_mqtt_data);
    return;
  }
}

/** @} */
