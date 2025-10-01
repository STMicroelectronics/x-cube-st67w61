/**
  ******************************************************************************
  * @file    w6x_mqtt.c
  * @author  GPM Application Team
  * @brief   This file provides code for W6x MQTT API
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
#include <string.h>
#include "w6x_api.h"       /* Prototypes of the functions implemented in this file */
#include "w61_at_api.h"    /* Prototypes of the functions called by this file */
#include "w6x_internal.h"
#include "w61_io.h"        /* Prototypes of the BUS functions to be registered */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W6X_Private_MQTT_Variables ST67W6X MQTT Variables
  * @ingroup  ST67W6X_Private_MQTT
  * @{
  */
static W61_Object_t *p_DrvObj = NULL; /*!< Global W61 context pointer */

#if (W6X_ASSERT_ENABLE == 1)
/** W6X MQTT init error string */
static const char W6X_MQTT_Uninit_str[] = "W6X MQTT module not initialized";
#endif /* W6X_ASSERT_ENABLE */

/** MQTT state string */
static const char *const W6X_MQTT_State_str[] =
{
  "Not initialized",
  "", /* Unused "User configuration done" */
  "Configured", /* "Connection configuration done" */
  "Disconnected",
  "Connected",
  "", /* Unused "Connected, no subscription" */
  "", /* Unused "Connected, and subscribed to MQTT topics" */
};

static uint32_t W6X_MQTT_SNI_enabled = 0; /*!< Server Name Indication (SNI) enabled */

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup ST67W6X_Private_MQTT_Functions ST67W6X MQTT Functions
  * @ingroup  ST67W6X_Private_MQTT
  * @{
  */
/**
  * @brief  MQTT callback function
  * @param  event_id: event ID
  * @param  event_args: event arguments
  */
static void W6X_MQTT_cb(W61_event_id_t event_id, void *event_args);

/** @} */

/* Functions Definition ------------------------------------------------------*/
/** @addtogroup ST67W6X_API_MQTT_Public_Functions
  * @{
  */
W6X_Status_t W6X_MQTT_Init(W6X_MQTT_Data_t *p_mqtt_config)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_App_Cb_t *p_cb_handler;
  NULL_ASSERT(p_mqtt_config, "MQTT configuration pointer is NULL");

  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  /* Check that application callback is registered */
  p_cb_handler = W6X_GetCbHandler();
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_mqtt_cb == NULL))
  {
    MQTT_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return ret;
  }

  /* Register W61 driver callbacks */
  W61_RegisterULcb(p_DrvObj,
                   NULL,
                   NULL,
                   NULL,
                   W6X_MQTT_cb,
                   NULL);

  return TranslateErrorStatus(W61_MQTT_Init(p_DrvObj, p_mqtt_config->p_recv_data, p_mqtt_config->recv_data_buf_size));
}

void W6X_MQTT_DeInit(void)
{
  if (p_DrvObj == NULL)
  {
    return; /* Nothing to do */
  }
  W61_MQTT_DeInit(p_DrvObj); /* Deinitialize MQTT */

  p_DrvObj = NULL; /* Reset the global pointer */
}

W6X_Status_t W6X_MQTT_SetRecvDataPtr(W6X_MQTT_Data_t *p_mqtt_config)
{
  NULL_ASSERT(p_mqtt_config, "MQTT configuration pointer is NULL");

  p_DrvObj = W61_ObjGet();
  NULL_ASSERT(p_DrvObj, W6X_Obj_Null_str);

  return TranslateErrorStatus(W61_MQTT_Init(p_DrvObj, p_mqtt_config->p_recv_data, p_mqtt_config->recv_data_buf_size));
}

W6X_Status_t W6X_MQTT_Configure(W6X_MQTT_Connect_t *Config)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  if ((Config->Scheme == 2) || (Config->Scheme == 4)) /* Server certificate */
  {
    if (Config->CACertificateName[0] == '\0')
    {
      MQTT_LOG_ERROR("CA certificate is mandatory for scheme 2 or 4\n");
      goto _err;
    }

    if (Config->CACertificateContent != NULL)
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByContent((char *)Config->CACertificateName, Config->CACertificateContent,
                                    strlen(Config->CACertificateContent) + 1) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", Config->CACertificateName);
        goto _err;
      }
    }
    else
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByName((char *)Config->CACertificateName) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", Config->CACertificateName);
        goto _err;
      }
    }
  }

  if ((Config->Scheme == 3) || (Config->Scheme == 4)) /* Client certificate */
  {
    if ((Config->CertificateName[0] == '\0') || (Config->PrivateKeyName[0] == '\0'))
    {
      MQTT_LOG_ERROR("Client certificate and private key are mandatory for scheme 3 or 4\n");
      goto _err;
    }
    if (Config->CertificateContent != NULL)
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByContent((char *)Config->CertificateName, Config->CertificateContent,
                                    strlen(Config->CertificateContent) + 1) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", Config->CertificateName);
        goto _err;
      }
    }
    else
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByName((char *)Config->CertificateName) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", Config->CertificateName);
        goto _err;
      }
    }
    if (Config->PrivateKeyContent != NULL)
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByContent((char *)Config->PrivateKeyName, Config->PrivateKeyContent,
                                    strlen(Config->PrivateKeyContent) + 1) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", Config->PrivateKeyName);
        goto _err;
      }
    }
    else
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByName((char *)Config->PrivateKeyName) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", Config->PrivateKeyName);
        goto _err;
      }
    }
  }

  W6X_MQTT_SNI_enabled = Config->SNI_enabled;

  /* Set the MQTT User configuration */
  ret = TranslateErrorStatus(W61_MQTT_SetUserConfiguration(p_DrvObj, Config->Scheme, Config->MQClientId,
                                                           Config->MQUserName, Config->MQUserPwd,
                                                           Config->CertificateName, Config->PrivateKeyName,
                                                           Config->CACertificateName));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Set the MQTT Connection configuration */
  ret = TranslateErrorStatus(W61_MQTT_SetConfiguration(p_DrvObj, Config->KeepAlive, Config->DisableCleanSession,
                                                       Config->WillTopic, Config->WillMessage, Config->WillQos,
                                                       Config->WillRetain));

_err:
  return ret;
}

W6X_Status_t W6X_MQTT_Connect(W6X_MQTT_Connect_t *Config)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  if (W6X_MQTT_SNI_enabled == 1)
  {
    /* Set the SNI */
    ret = TranslateErrorStatus(W61_MQTT_SetSNI(p_DrvObj, Config->HostName));
    if (ret != W6X_STATUS_OK)
    {
      goto _err;
    }
  }

  /* Connect to the MQTT broker */
  ret = TranslateErrorStatus(W61_MQTT_Connect(p_DrvObj, Config->HostName, Config->HostPort));

_err:
  return ret;
}

W6X_Status_t W6X_MQTT_GetConnectionStatus(W6X_MQTT_Connect_t *Config)
{
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  /* Get the connection status */
  ret = TranslateErrorStatus(W61_MQTT_GetConnectionStatus(p_DrvObj, Config->HostName,
                                                          &Config->HostPort, &Config->Scheme,
                                                          &Config->State));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Get the MQTT user configuration */
  ret = TranslateErrorStatus(W61_MQTT_GetUserConfiguration(p_DrvObj, Config->MQClientId,
                                                           Config->MQUserName, Config->MQUserPwd,
                                                           Config->CertificateName, Config->PrivateKeyName,
                                                           Config->CACertificateName));

_err:
  return ret;
}

W6X_Status_t W6X_MQTT_Disconnect(void)
{
  uint8_t HostName[64];
  uint32_t HostPort;
  uint32_t Scheme;
  uint32_t State = W6X_MQTT_STATE_UNINIT;
  W6X_Status_t ret;
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  /* Get the connection status */
  ret = TranslateErrorStatus(W61_MQTT_GetConnectionStatus(p_DrvObj, HostName, &HostPort, &Scheme, &State));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  if ((State == W6X_MQTT_STATE_CONNECTED) || (State == W6X_MQTT_STATE_CONNECTED_SUBSCRIBED) ||
      (State == W6X_MQTT_STATE_CONNECTED_NO_SUB))
  {
    /* Disconnect from the MQTT broker */
    return TranslateErrorStatus(W61_MQTT_Disconnect(p_DrvObj));
  }

  return W6X_STATUS_OK;
_err:
  return ret;
}

W6X_Status_t W6X_MQTT_Subscribe(uint8_t *Topic)
{
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  /* Subscribe to the topic */
  return TranslateErrorStatus(W61_MQTT_Subscribe(p_DrvObj, Topic));
}

W6X_Status_t W6X_MQTT_GetSubscribedTopics(void)
{
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  /* Get the list of subscribed topics */
  return TranslateErrorStatus(W61_MQTT_GetSubscribedTopics(p_DrvObj));
}

W6X_Status_t W6X_MQTT_Unsubscribe(uint8_t *Topic)
{
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  /* Unsubscribe from the topic */
  return TranslateErrorStatus(W61_MQTT_Unsubscribe(p_DrvObj, Topic));
}

W6X_Status_t W6X_MQTT_Publish(uint8_t *Topic, uint8_t *Message, uint32_t Message_len, uint32_t Qos, uint32_t Retain)
{
  NULL_ASSERT(p_DrvObj, W6X_MQTT_Uninit_str);

  /* Publish the message to the topic */
  return TranslateErrorStatus(W61_MQTT_Publish(p_DrvObj, Topic, Message, Message_len, Qos, Retain));
}

const char *W6X_MQTT_StateToStr(uint32_t state)
{
  if (state > W6X_MQTT_STATE_CONNECTED_SUBSCRIBED)
  {
    return "Unknown";
  }
  else if ((state == W6X_MQTT_STATE_CONNECTED_NO_SUB) || (state == W6X_MQTT_STATE_CONNECTED_SUBSCRIBED))
  {
    /* Return simplified connected status */
    return W6X_MQTT_State_str[W6X_MQTT_STATE_CONNECTED];
  }
  else
  {
    /* Return the MQTT state string */
    return W6X_MQTT_State_str[state];
  }
}

/** @} */

/* Private Functions Definition ----------------------------------------------*/
/* =================== Callbacks ===================================*/
/** @addtogroup ST67W6X_Private_MQTT_Functions
  * @{
  */
static void W6X_MQTT_cb(W61_event_id_t event_id, void *event_args)
{
  W6X_MQTT_CbParamData_t *cb_param_mqtt_data;
  W6X_App_Cb_t *p_cb_handler = W6X_GetCbHandler();
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_mqtt_cb == NULL))
  {
    MQTT_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return;
  }

  switch (event_id)
  {
    case W61_MQTT_EVT_CONNECTED_ID:
      p_cb_handler->APP_mqtt_cb(W6X_MQTT_EVT_CONNECTED_ID, NULL);
      break;

    case W61_MQTT_EVT_DISCONNECTED_ID:
      p_cb_handler->APP_mqtt_cb(W6X_MQTT_EVT_DISCONNECTED_ID, NULL);
      break;

    case W61_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID:
      cb_param_mqtt_data = (W6X_MQTT_CbParamData_t *)event_args;
      p_cb_handler->APP_mqtt_cb(W6X_MQTT_EVT_SUBSCRIPTION_RECEIVED_ID, (void *) cb_param_mqtt_data);
      break;

    default:
      break;
  }
}

/** @} */

#endif /* ST67_ARCH */
