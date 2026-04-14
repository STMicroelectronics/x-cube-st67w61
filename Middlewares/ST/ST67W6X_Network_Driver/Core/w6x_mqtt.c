/**
  ******************************************************************************
  * @file    w6x_mqtt.c
  * @author  ST67 Application Team
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
static W61_Object_t *W6X_MQTT_drv_obj = NULL; /*!< Global W61 context pointer */

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
W6X_Status_t W6X_MQTT_Init(W6X_MQTT_Data_t *mqtt_data)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  W6X_App_Cb_t *p_cb_handler;
  NULL_ASSERT(mqtt_data, "MQTT configuration pointer is NULL");

  W6X_MQTT_drv_obj = W61_ObjGet();
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_Obj_Null_str);

  /* Check that application callback is registered */
  p_cb_handler = W6X_GetCbHandler();
  if ((p_cb_handler == NULL) || (p_cb_handler->APP_mqtt_cb == NULL))
  {
    MQTT_LOG_ERROR("Please register the APP callback before initializing the module\n");
    return ret;
  }

  /* Register W61 driver callbacks */
  (void)W61_RegisterULcb(W6X_MQTT_drv_obj,
                         NULL,
                         NULL,
                         NULL,
                         W6X_MQTT_cb,
                         NULL);

  return TranslateErrorStatus(W61_MQTT_Init(W6X_MQTT_drv_obj, mqtt_data->p_recv_data,
                                            mqtt_data->recv_data_buf_size));
}

void W6X_MQTT_DeInit(void)
{
  if (W6X_MQTT_drv_obj == NULL)
  {
    return; /* Nothing to do */
  }
  (void)W61_MQTT_DeInit(W6X_MQTT_drv_obj); /* Deinitialize MQTT */

  W6X_MQTT_drv_obj = NULL; /* Reset the global pointer */
}

W6X_Status_t W6X_MQTT_SetRecvDataPtr(W6X_MQTT_Data_t *mqtt_data)
{
  NULL_ASSERT(mqtt_data, "MQTT configuration pointer is NULL");

  W6X_MQTT_drv_obj = W61_ObjGet();
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_Obj_Null_str);

  return TranslateErrorStatus(W61_MQTT_Init(W6X_MQTT_drv_obj, mqtt_data->p_recv_data,
                                            mqtt_data->recv_data_buf_size));
}

W6X_Status_t W6X_MQTT_Configure(W6X_MQTT_Connect_t *config)
{
  W6X_Status_t ret = W6X_STATUS_ERROR;
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  if ((config->Scheme == 2U) || (config->Scheme == 4U)) /* Server certificate */
  {
    if (config->CACertificateName[0] == '\0')
    {
      MQTT_LOG_ERROR("CA certificate is mandatory for scheme 2 or 4\n");
      goto _err;
    }

    if (config->CACertificateContent != NULL)
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByContent((char *)config->CACertificateName, config->CACertificateContent,
                                    strlen(config->CACertificateContent)) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", config->CACertificateName);
        goto _err;
      }
    }
    else
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByName((char *)config->CACertificateName) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", config->CACertificateName);
        goto _err;
      }
    }
  }

  if ((config->Scheme == 3U) || (config->Scheme == 4U)) /* Client certificate */
  {
    if ((config->CertificateName[0] == '\0') || (config->PrivateKeyName[0] == '\0'))
    {
      MQTT_LOG_ERROR("Client certificate and private key are mandatory for scheme 3 or 4\n");
      goto _err;
    }
    if (config->CertificateContent != NULL)
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByContent((char *)config->CertificateName, config->CertificateContent,
                                    strlen(config->CertificateContent)) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", config->CertificateName);
        goto _err;
      }
    }
    else
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByName((char *)config->CertificateName) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", config->CertificateName);
        goto _err;
      }
    }
    if (config->PrivateKeyContent != NULL)
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByContent((char *)config->PrivateKeyName, config->PrivateKeyContent,
                                    strlen(config->PrivateKeyContent)) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", config->PrivateKeyName);
        goto _err;
      }
    }
    else
    {
      /* Write the file into the NCP */
      if (W6X_FS_WriteFileByName((char *)config->PrivateKeyName) != W6X_STATUS_OK)
      {
        MQTT_LOG_ERROR("Writing file %s into the NCP failed\n", config->PrivateKeyName);
        goto _err;
      }
    }
  }

  /* Set the MQTT User configuration */
  ret = TranslateErrorStatus(W61_MQTT_SetUserConfiguration(W6X_MQTT_drv_obj, config->Scheme, config->MQClientId,
                                                           config->MQUserName, config->MQUserPwd,
                                                           config->CertificateName, config->PrivateKeyName,
                                                           config->CACertificateName));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  if (config->SNI[0] != 0U)
  {
    /* Set the SNI */
    ret = TranslateErrorStatus(W61_MQTT_SetSNI(W6X_MQTT_drv_obj, config->SNI));
    if (ret != W6X_STATUS_OK)
    {
      goto _err;
    }
  }

  /* Set the MQTT Connection configuration */
  ret = TranslateErrorStatus(W61_MQTT_SetConfiguration(W6X_MQTT_drv_obj, config->KeepAlive, config->DisableCleanSession,
                                                       config->WillTopic, config->WillMessage, config->WillQos,
                                                       config->WillRetain));

_err:
  return ret;
}

W6X_Status_t W6X_MQTT_Connect(W6X_MQTT_Connect_t *config)
{
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  /* Connect to the MQTT broker */
  return TranslateErrorStatus(W61_MQTT_Connect(W6X_MQTT_drv_obj, config->HostName, config->HostPort));
}

W6X_Status_t W6X_MQTT_GetConnectionStatus(W6X_MQTT_Connect_t *config)
{
  W6X_Status_t ret;
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  /* Get the connection status */
  ret = TranslateErrorStatus(W61_MQTT_GetConnectionStatus(W6X_MQTT_drv_obj, config->HostName,
                                                          &config->HostPort, &config->Scheme,
                                                          &config->State));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  /* Get the MQTT user configuration */
  ret = TranslateErrorStatus(W61_MQTT_GetUserConfiguration(W6X_MQTT_drv_obj, config->MQClientId,
                                                           config->MQUserName, config->MQUserPwd,
                                                           config->CertificateName, config->PrivateKeyName,
                                                           config->CACertificateName));

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
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  /* Get the connection status */
  ret = TranslateErrorStatus(W61_MQTT_GetConnectionStatus(W6X_MQTT_drv_obj, HostName, &HostPort, &Scheme, &State));
  if (ret != W6X_STATUS_OK)
  {
    goto _err;
  }

  if ((State == W6X_MQTT_STATE_CONNECTED) || (State == W6X_MQTT_STATE_CONNECTED_SUBSCRIBED) ||
      (State == W6X_MQTT_STATE_CONNECTED_NO_SUB))
  {
    /* Disconnect from the MQTT broker */
    return TranslateErrorStatus(W61_MQTT_Disconnect(W6X_MQTT_drv_obj));
  }

  return W6X_STATUS_OK;
_err:
  return ret;
}

W6X_Status_t W6X_MQTT_Subscribe(uint8_t *topic)
{
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  /* Subscribe to the topic */
  return TranslateErrorStatus(W61_MQTT_Subscribe(W6X_MQTT_drv_obj, topic));
}

W6X_Status_t W6X_MQTT_GetSubscribedTopics(void)
{
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  /* Get the list of subscribed topics */
  return TranslateErrorStatus(W61_MQTT_GetSubscribedTopics(W6X_MQTT_drv_obj));
}

W6X_Status_t W6X_MQTT_Unsubscribe(uint8_t *topic)
{
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  /* Unsubscribe from the topic */
  return TranslateErrorStatus(W61_MQTT_Unsubscribe(W6X_MQTT_drv_obj, topic));
}

W6X_Status_t W6X_MQTT_Publish(uint8_t *topic, uint8_t *message, uint32_t message_len, uint32_t qos,
                              uint32_t retain)
{
  NULL_ASSERT(W6X_MQTT_drv_obj, W6X_MQTT_Uninit_str);

  /* Publish the message to the topic */
  return TranslateErrorStatus(W61_MQTT_Publish(W6X_MQTT_drv_obj, topic, message, message_len, qos, retain));
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
      /* MQTT events unmanaged */
      break;
  }
}

/** @} */

#endif /* ST67_ARCH */
