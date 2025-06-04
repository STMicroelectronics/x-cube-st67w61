/**
  ******************************************************************************
  * @file    w61_at_rx_parser.c
  * @author  GPM Application Team
  * @brief   This file provides the low layer implementations of the AT driver
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
#include "w61_at_rx_parser.h"
#include "w61_at_api.h"
#include "w61_at_common.h"
#include "w61_at_internal.h"
#include "w61_io.h" /* SPI_XFER_MTU_BYTES */
#include "common_parser.h" /* Common Parser functions */

#if (SYS_DBG_ENABLE_TA4 >= 1)
#include "trcRecorder.h"
#endif /* SYS_DBG_ENABLE_TA4 */

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/** @defgroup ST67W61_AT_RX_Parser_Types ST67W61 AT Driver Rx Parser Types
  * @ingroup  ST67W61_AT_RX_Parser
  * @{
  */

/**
  * @brief  AT events list structure
  */
typedef struct
{
  /** Event keyword */
  const char evtDetectCharacters[W61_ATD_EVT_SEGMENT_NEEDED];
  /** Length of the event keyword */
  const uint8_t evtDetectCharactersLen;
  /** Type of the event */
  const uint8_t type;
} W61_AtEvtIdList_t;

/**
  * @brief  resp/Event structure to queue
  */
typedef struct
{
  uint32_t type;  /*!< Type of the response/event message */
  uint32_t len;   /*!< Length of the response/event message */
  char *buf;      /*!< Pointer to the response/event message */
} W61_resp_t;

/**
  * @brief  AT data events list structure
  */

typedef struct
{
  /** Event keyword */
  const char evtDataKeyword[W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER];
  /** Length of the event keyword */
  const uint8_t evtDataKeywordLen;
  /** Offset where the parameter length is situated in the string */
  const uint8_t evtParamLenOffset;
  /** Type of the data */
  const uint8_t type;
} W61_AtDataEvtIdList_t;

/** @} */

/* Private defines -----------------------------------------------------------*/
/** @addtogroup ST67W61_AT_RX_Parser_Constants
  * @{
  */

/** size of /r/n string */
#define CRLF_SIZE  2

/** offset from which to start parsing for finding /r/n end of message delimiter */
#define CRLF_START_SEARCH_OFFSET  2

/** Number of items in the AT events list */
#define ITEMS_IN_EVT_LIST   (sizeof(AtEvtList) / sizeof(W61_AtEvtIdList_t))

/** Number of items in the AT data events list */
#define ITEMS_IN_DATA_EVT_LIST  (sizeof(AtDataEvtList) / sizeof(W61_AtDataEvtIdList_t))

/** Data buffer size */
#define RX_DATA_BUFFER_SIZE           SPI_XFER_MTU_BYTES

/** Timeout to receive an event from the stream buffer */
#define EVENT_TIMEOUT                 portMAX_DELAY

/** Timeout to receive a response from the stream buffer */
#define RESPONSE_TIMEOUT_MS           10000

/** Maximum number of parser loops */
#define MAX_PARSER_LOOPS              100

/** Receive command, response or events type */
#define RECV_TYPE_NONE                0

/** Receive Wi-Fi type */
#define RECV_TYPE_WIFI                1

/** Receive BLE type */
#define RECV_TYPE_BLE                 2

/** Receive Net type */
#define RECV_TYPE_NET                 3

/** Receive MQTT type */
#define RECV_TYPE_MQTT                4

/** Queue size */
#define W61_ATD_CMDRSP_QUEUE_SIZE     30

/** @} */

/* Private macros ------------------------------------------------------------*/
/** @defgroup ST67W61_AT_RX_Parser_Macros ST67W61 AT Driver Rx Parser Macros
  * @ingroup  ST67W61_AT_RX_Parser
  * @{
  */

/** Check if the character is CR */
#define IS_CHAR_CR(ch)                ((ch)=='\r')

/** Check if the character is LF */
#define IS_CHAR_LF(ch)                ((ch)=='\n')

/** Check if the character is '>' */
#define IS_CHAR_SEND(ch)              ((ch)=='>')

/** @} */

/* Private variables ---------------------------------------------------------*/
/** @defgroup ST67W61_AT_RX_Parser_Variables ST67W61 AT Driver Rx Parser Variables
  * @ingroup  ST67W61_AT_RX_Parser
  * @{
  */

/** Data received from the AT module can be fragmented, this buffer retains the DataHeader */
static uint8_t RetainRecvDataHeader[W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER];

/** Wi-Fi AT events list */

/** AT events list */
static const W61_AtEvtIdList_t AtEvtList[] = /* Used by w61_at_rx_parser.c */
{
  { "+IPD:",   sizeof("+IPD:") - 1,   RECV_TYPE_NET},
  { "+CIP:",   sizeof("+CIP:") - 1,   RECV_TYPE_NET},
  { "+MQTT:",  sizeof("+MQTT:") - 1,  RECV_TYPE_MQTT},
  { "+BLE:",   sizeof("+BLE:") - 1,   RECV_TYPE_BLE},
  { "+CW:",    sizeof("+CW:") - 1,    RECV_TYPE_WIFI},
  { "+CWLAP:", sizeof("+CWLAP:") - 1, RECV_TYPE_WIFI},
};

/** AT data events list */
static const W61_AtDataEvtIdList_t AtDataEvtList[] =
{
  { "+CIPRECVDATA:",   sizeof("+CIPRECVDATA:") - 1,   sizeof("+CIPRECVDATA:") - 1,         RECV_TYPE_NET},
  { "+MQTT:SUBRECV:",  sizeof("+MQTT:SUBRECV:") - 1,  sizeof("+MQTT:SUBRECV:y,") - 1,      RECV_TYPE_MQTT},
  { "+BLE:GATTWRITE:", sizeof("+BLE:GATTWRITE:") - 1, sizeof("+BLE:GATTWRITE:y,y,y,") - 1, RECV_TYPE_BLE},
  { "+BLE:GATTREAD:",  sizeof("+BLE:GATTREAD:") - 1,  sizeof("+BLE:GATTREAD:y,y,y,") - 1,  RECV_TYPE_BLE},
  { "+BLE:NOTIDATA:",  sizeof("+BLE:NOTIDATA:") - 1,  sizeof("+BLE:NOTIDATA:y,") - 1,      RECV_TYPE_BLE},
};

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W61_AT_RX_Parser_Functions
  * @{
  */
/**
  * @brief  Check if the parameter length to be extracted from the string is complete and return the offset
  * @param  p_msg: pointer to string
  * @param  string_len: length of the entire received string from which to extract the parameter length
  * @param  param_len_offset: offset where the parameter length is situated in the string
  * @retval int32_t: 0 if parameter length is incomplete, otherwise the number of chars of <param len + ,>
  */
static int32_t ATD_GetDataOffset(uint8_t *p_msg, int32_t string_len, int32_t param_len_offset);

/**
  * @brief  Check if /r/n are present at the begin of the message and return how many
  * @param  p_msg: pointer to string
  * @retval uint32_t: number of /r/n at the begin of the message, e.i. if /r/n/r/n crlf_count=2
  */
static uint32_t ATD_CountCrlfAtBeginMsg(uint8_t *p_msg);

/**
  * @brief  Check if a message is completed by searching is delimiter (CRLF)
  * @param  p_msg: pointer to string
  * @param  search_len: for how many bytes to search starting from the initial pointer
  * @retval int32_t: 0 if delimiter not found, else nr of bytes to the end of the message
  */
static uint32_t ATD_SearchCrlfDelimitingEndOfMsg(uint8_t *p_msg, uint32_t search_len);

/**
  * @brief  Extract parameter length value, retains msg header and return Data Offset.
  * @param  p_msg: pointer to string
  * @param  string_len: length of the entire received string
  * @param  Recv_Data_Type: pointer used to return the value of the RECV_TYPE
  * @param  Len_Extracted_From_Data_Header: pointer used to return the value of the RECV_TYPE
  * @retval int32_t: 0 evt not found, -1 error, otherwise data_offset is the number of bytes after which data starts
  */
static int32_t ATD_ProcessDataHeader(uint8_t *p_msg, int32_t string_len, int8_t *Recv_Data_Type,
                                     int32_t *Len_Extracted_From_Data_Header);

/**
  * @brief  Transfers the received data to the application buffer (pointer stored by the app in the Obj).
  * @param  Obj: pointer to module handle
  * @param  RecvDataType: NET, HTTP, MQTT or BLE data
  * @param  DataReceivedInPreviousStrings: Used when data does not arrive in one shot but fragmented
  * @param  p_data: points to the part of p_input buffer containing the data to be copied
  * @param  len: number of bytes to be copied
  */
static void ATD_CopyDataToAppBuffer(W61_Object_t *Obj, int8_t RecvDataType,
                                    int32_t DataReceivedInPreviousStrings, uint8_t *p_data, int32_t len);

/**
  * @brief  Send the Data Header to the upper module once all related data has been received.
  * @param  Obj: pointer to module handle
  * @param  RecvDataType: NET, HTTP, MQTT or BLE data
  */
static void ATD_SendDataHeaderToUpperLayer(W61_Object_t *Obj, int8_t RecvDataType);

/**
  * @brief  Check if the received and isolated single message is a classical Event and queue it.
  * @param  Obj: pointer to module handle
  * @param  p_evt: pointer to string
  * @param  single_evt_length: length of the isolated (decoupled) string to be parsed
  * @retval bool: False if event not found, True if event detected. .
  */
static bool ATD_DetectEvent(W61_Object_t *Obj, uint8_t *p_evt, int32_t single_evt_length);

/**
  * @brief  Pre_parse, decompose string and dispatch received strings
  * @param  Obj: pointer to module handle, reserved for future use
  * @param  p_input: pointer within the ATD_RecvBuf buffer to the begin of the unprocessed part.
  * @param  string_len: size of the unprocessed part of the ATD_RecvBuf to be parsed.
  * @retval int32_t remaining_len: 0 if msg ends with CRLF else length of incomplete message
  */
static int32_t ATD_RxDispatcher_func(W61_Object_t *Obj, uint8_t *p_input, int32_t string_len);

/**
  * @brief  Allocates response buffer
  * @param  size: size of response buffer the to allocate
  * @retval W61_resp_t buffer pointer; NULL if alloc has failed
  */
static W61_resp_t *ATD_RespAlloc(size_t size);

/**
  * @brief  Free response buffer to free
  * @param  resp: the response buffer
  */
static void ATD_RespFree(W61_resp_t *resp);

/* Functions Definition ------------------------------------------------------*/

int32_t W61_ATD_RxParserInit(W61_Object_t *Obj)
{
  BaseType_t xReturned;
  W61_Status_t ret = W61_STATUS_ERROR;

  Obj->EventsBuf = pvPortMalloc(W61_ATD_EVENT_STRING_SIZE);
  if (Obj->EventsBuf == NULL)
  {
    LogError("Unable to allocate EventsBuf\n");
    return ret;
  }

  Obj->ATD_RecvBuf = pvPortMalloc(SPI_XFER_MTU_BYTES);
  if (Obj->ATD_RecvBuf == NULL)
  {
    LogError("Unable to allocate ATD_RecvBuf\n");
    goto __err;
  }
  memset(Obj->ATD_RecvBuf, 0, SPI_XFER_MTU_BYTES);
  /* Create a Rx Message x-buffer that can hold W61_ATD_CMDRSP_XBUF_SIZE bytes */
  Obj->ATD_Resp_Queue = xQueueCreate(W61_ATD_CMDRSP_QUEUE_SIZE, sizeof(W61_resp_t *));
  if (Obj->ATD_Resp_Queue == NULL)
  {
    LogError("Resp queue creation failed\n");
    goto __err;
  }

  Obj->ATD_Evt_Queue = xQueueCreate(W61_ATD_CMDRSP_QUEUE_SIZE, sizeof(W61_resp_t *));
  if (Obj->ATD_Evt_Queue == NULL)
  {
    LogError("Evt queue creation failed\n");
    goto __err;
  }

  xReturned = xTaskCreate(W61_ATD_RxPooling_task,
                          (char *)"AtRxParser",
                          W61_ATD_RX_TASK_STACK_SIZE_BYTES >> 2, Obj,
                          W61_ATD_RX_TASK_PRIO,
                          &Obj->ATD_RxPooling_task_handle);

  if (xReturned != pdPASS)
  {
    LogError("xTaskCreate failed to create\n");
    goto __err;
  }

  xReturned = xTaskCreate(W61_ATD_EventsPooling_task,
                          (char *)"AtEvt",
                          W61_ATD_EVENTS_TASK_STACK_SIZE_BYTES >> 2, Obj,
                          W61_ATD_EVENTS_TASK_PRIO,
                          &Obj->ATD_EvtPooling_task_handle);

  if (xReturned != pdPASS)
  {
    LogError("xTaskCreate failed to create\n");
    goto __err;
  }

  return W61_STATUS_OK;

__err:
  if (Obj->ATD_Resp_Queue)
  {
    vQueueDelete(Obj->ATD_Resp_Queue);
  }

  if (Obj->ATD_RecvBuf)
  {
    vPortFree(Obj->ATD_RecvBuf);
    Obj->ATD_RecvBuf = NULL;
  }
  if (Obj->EventsBuf)
  {
    vPortFree(Obj->EventsBuf);
    Obj->EventsBuf = NULL;
  }
  return ret;
}

void W61_ATD_RxParserDeInit(W61_Object_t *Obj)
{
  vTaskDelete(Obj->ATD_RxPooling_task_handle);
  vTaskDelete(Obj->ATD_EvtPooling_task_handle);

  vQueueDelete(Obj->ATD_Resp_Queue);
  vQueueDelete(Obj->ATD_Evt_Queue);

  vPortFree(Obj->ATD_RecvBuf);
  vPortFree(Obj->EventsBuf);
  Obj->ATD_RecvBuf = NULL;
}

void W61_ATD_RxPooling_task(void *arg)
{
  W61_Object_t *Obj = arg;
  int32_t string_len = 0;
  int32_t concat_len = 0;      /* Concatenation length */
  int32_t remaining_len = 0;
  int32_t processed_len = 0;

  while (1)
  {
#if (SYS_DBG_ENABLE_TA4 >= 1)
    vTracePrintF("W61_ATD_RxPooling_task", "rx_task: Entering spisync_read Sleep ");
#endif /* SYS_DBG_ENABLE_TA4 */
    /* When the rsp/evt is not complete (\r\n not received) AT_RxDispatcher_func() returns a value != 0
       Which means it waits until \r\n before processing the ATD_RecvBuf[] content
       The IO_Receive function will keep previous data in ATD_RecvBuf[] and add next data after it (concat_len shift) */
    string_len = Obj->fops.IO_Receive((uint8_t *)(Obj->ATD_RecvBuf + concat_len),
                                      RX_DATA_BUFFER_SIZE - concat_len, pdMS_TO_TICKS(RESPONSE_TIMEOUT_MS));

    if (string_len > 0)
    {
      AT_LOG_HOST_IN(Obj->ATD_RecvBuf + concat_len, string_len);  /* Log info */
#if (SYS_DBG_ENABLE_TA4 >= 1)
      vTracePrintF("W61_ATD_RxPooling_task", "rx_task: Exiting spisync_read: data received");
#endif /* SYS_DBG_ENABLE_TA4 */
      concat_len += string_len;
      remaining_len = ATD_RxDispatcher_func(Obj, Obj->ATD_RecvBuf + processed_len, concat_len - processed_len);
      if (remaining_len == 0)
      {
        /* When last message in the pipe ends with "\r\n" the pointers can be reset to the begin of the ATD_RecvBuf */
        memset(Obj->ATD_RecvBuf, 0, concat_len);
        concat_len = 0;
        processed_len = 0;
      }
      else /* It can only happen if in the last part of the string there is un uncompleted Resp or Evt */
      {
        /* Processed_len covers the case where several msgs are in the pipe ATD_RecvBuf and last msg is not complete
           Example if ATD_RecvBuf[] contains "+CW:CONNECTED\r\n+CW:GOT" the dispatcher process "+CW:CONNECTED\r\n"
           But needs to wait for the missing "IP\r\n" before processing next msg
           In such scenario processed_len will be 15 (i.e. 22 - 7) */
        processed_len = concat_len - remaining_len;
      }

      if (concat_len == RX_DATA_BUFFER_SIZE) /* No more space in the (Obj->ATD_RecvBuf). it can only happen if */
      {
        /* In the last part of the buffer there is un uncompleted Resp or Evt
           In case of DATA remaining_len always 0 so also concat_len become 0 */
        memcpy(Obj->ATD_RecvBuf, Obj->ATD_RecvBuf + processed_len, remaining_len);
        concat_len = remaining_len;
        processed_len = 0;
        LogDebug("Obj->ATD_RecvBuf full, copy last fragmented message to the begin");
      }
    }
    else
    {
#if (SYS_DBG_ENABLE_TA4 >= 1)
      vTracePrintF("W61_ATD_RxPooling_task", "rx_task: Exiting spisync_read: Sleep expired");
#endif /* SYS_DBG_ENABLE_TA4 */
    }
  }
}

void W61_ATD_EventsPooling_task(void *arg)
{
  W61_Object_t *Obj = arg;
  int32_t x_next_event_len = 0;
  uint32_t event_type = RECV_TYPE_NONE; /* Event type to be used in the callback function */

  while (1)
  {
#if (SYS_DBG_ENABLE_TA4 >= 1)
    vTracePrintF("W61_ATD_EventsPooling_task", "event_task: Entering Sleep ");
#endif /* SYS_DBG_ENABLE_TA4 */

    W61_resp_t *resp = NULL;
    if (xQueueReceive(Obj->ATD_Evt_Queue, &resp, EVENT_TIMEOUT) == pdPASS)
    {
      x_next_event_len = resp->len;
      event_type = resp->type; /* Event type to be used in the callback function */
      memcpy(Obj->EventsBuf, resp->buf, x_next_event_len);
      ATD_RespFree(resp);
    }

    if (x_next_event_len > 0)
    {
      switch (event_type)
      {
        case RECV_TYPE_NET:  /* Net event */
          if (Obj->Net_event_cb != NULL)
          {
            Obj->Net_event_cb(Obj, Obj->EventsBuf, x_next_event_len);
          }
          break;
        case RECV_TYPE_MQTT:  /* MQTT event */
          if (Obj->MQTT_event_cb != NULL)
          {
            Obj->MQTT_event_cb(Obj, Obj->EventsBuf, x_next_event_len);
          }
          break;
        case RECV_TYPE_BLE:  /* BLE event */
          if (Obj->Ble_event_cb != NULL)
          {
            Obj->Ble_event_cb(Obj, Obj->EventsBuf, x_next_event_len);
          }
          break;
        case RECV_TYPE_WIFI:  /* Wi-Fi event */
          if (Obj->WiFi_event_cb != NULL)
          {
            Obj->WiFi_event_cb(Obj, Obj->EventsBuf, x_next_event_len);
          }
          break;
        default:
          LogWarn("Event not decoded correctly\n");
          break;
      }
      x_next_event_len = 0;
      event_type = RECV_TYPE_NONE; /* Reset event type */
    }
  }
}

int32_t W61_ATD_Recv(W61_Object_t *Obj, uint8_t *pBuf, uint32_t len, uint32_t timeout_ms)
{
  W61_resp_t *resp = NULL;
  int32_t resp_len = 0;
  if (xQueueReceive(Obj->ATD_Resp_Queue, &resp, pdMS_TO_TICKS(timeout_ms)) == pdPASS)
  {
    resp_len = resp->len;
    memcpy(pBuf, resp->buf, resp_len);
    ATD_RespFree(resp);
  }
  return resp_len;
}

/* Private Functions Definition ----------------------------------------------*/
static int32_t ATD_GetDataOffset(uint8_t *p_msg, int32_t string_len, int32_t param_len_offset)
{
  int32_t i;
  for (i = param_len_offset; i < string_len; i++) /* Don't go over the string_len received */
  {
    /* Make sure the whole number is received and not just a part of it, i.e. detect next comma */
    if (p_msg[i] == ',')
    {
      return (i + 1); /* Offset of the comma after the parameter length +1 (i.e. where data starts) */
    }
  }
  return 0; /* Comma not found, parameter length is incomplete, cannot be extracted from the string yet */
}

static uint32_t ATD_CountCrlfAtBeginMsg(uint8_t *p_msg)
{
  uint32_t crlf_count = 0; /* Count in case of several consecutive CRLF, e.i. if \r\n\r\n crlf_count=2 */
  uint32_t p_msg_len = strlen((char *)p_msg);
  if (p_msg_len > CRLF_SIZE)
  {
    while (1)
    {
      /* !IS_CHAR_SEND avoids to remove /r/n in from to ">", "/r/n>" is considered as a msg,
         too risky consider only ">" */
      if (IS_CHAR_CR(p_msg[crlf_count]) && IS_CHAR_LF(p_msg[crlf_count + 1]) &&
          !IS_CHAR_SEND(p_msg[crlf_count + CRLF_SIZE]))
      {
        /* skip \r\n by incrementing the p_msg pointer by CRLF_SIZE */
        crlf_count += CRLF_SIZE;
      }
      else
      {
        break;
      }
    }
  }
  return crlf_count;
}

static uint32_t ATD_SearchCrlfDelimitingEndOfMsg(uint8_t *p_msg, uint32_t search_len)
{
  uint32_t i;
  /* Events or responses have always at least CRLF_SEARCH_OFFSET characters before ending \r\n */
  for (i = CRLF_START_SEARCH_OFFSET; i < search_len; i++)
  {
    /* Search the end of the message to be sure msg line is complete
       Starting with i > 1 avoids the "/r/n before >" to be considered as message completed symbol */
    if (IS_CHAR_CR(p_msg[i - 1]) && IS_CHAR_LF(p_msg[i]))
    {
      return i + 1;
    }
  }
  return 0;
}

static int32_t ATD_ProcessDataHeader(uint8_t *p_msg, int32_t string_len, int8_t *Recv_Data_Type,
                                     int32_t *Len_Extracted_From_Data_Header)
{
  uint32_t i;
  int32_t data_offset = 0;
  int32_t param_len_offset = 0;
  int32_t mqtt_topic_offset = 0;
  int32_t topic_len_extracted_from_mqtt_header = 0;

  for (i = 0; i < ITEMS_IN_DATA_EVT_LIST; i++)
  {
    /* In case of fragmentation checking string_len is to avoid data existing
       in ATD_RecvBuf from previous command misleads the check */
    if (string_len >= AtDataEvtList[i].evtParamLenOffset + 2)
    {
      if (strncmp((char *) p_msg, AtDataEvtList[i].evtDataKeyword, AtDataEvtList[i].evtDataKeywordLen) == 0)
      {
        /* Data messages are the only that needs to be fully decoded by the AT_RxDispatcher_func() */
        *Len_Extracted_From_Data_Header = 0;
        param_len_offset = AtDataEvtList[i].evtParamLenOffset;
        if (AtDataEvtList[i].type == RECV_TYPE_MQTT)
        {
          mqtt_topic_offset = ATD_GetDataOffset(p_msg, string_len, param_len_offset);
          if (mqtt_topic_offset > 0) /* Check comma after parameter length is found */
          {
            /* Converting the number (topic_len_extracted_from_mqtt_header) only if comma is received */
            Parser_StrToInt((char *) p_msg + param_len_offset, NULL, &topic_len_extracted_from_mqtt_header);
            if (topic_len_extracted_from_mqtt_header < 0)
            {
              return -1; /* Error wrong Data length conversion */
            }

            /* Increment the size of 3 to include the comma and the two quotes */
            topic_len_extracted_from_mqtt_header += 3;
            param_len_offset = mqtt_topic_offset; /* Update for next parameter */
          }
          else
          {
            return 0; /* Comma not found */
          }
        }

        data_offset = ATD_GetDataOffset(p_msg, string_len, param_len_offset);
        if (data_offset > 0) /* Check comma after parameter length is found */
        {
          /* Converting the number (*Len_Extracted_From_Data_Header) only if comma is received */
          Parser_StrToInt((char *) p_msg + param_len_offset, NULL, Len_Extracted_From_Data_Header);
          /* Prepare string to be sent to upper layer (ATD_Resp_Queue or ATD_Evt_Queue) */
          memcpy(RetainRecvDataHeader, p_msg, W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER);
          if (*Len_Extracted_From_Data_Header >= 0)
          {
            *Recv_Data_Type = AtDataEvtList[i].type;
            *Len_Extracted_From_Data_Header += topic_len_extracted_from_mqtt_header;
          }
          else
          {
            return -1; /* Error wrong Data length conversion */
          }
        }

        return data_offset;
      }
    }
  }
  return 0;
}

static void ATD_CopyDataToAppBuffer(W61_Object_t *Obj, int8_t RecvDataType,
                                    int32_t DataReceivedInPreviousStrings, uint8_t *p_data, int32_t len)
{
  int32_t app_buffer_remaining_space = 0;
  uint8_t *recv_data = NULL;

  switch (RecvDataType)
  {
    case RECV_TYPE_NET:
      app_buffer_remaining_space = Obj->NetCtx.AppBuffRecvDataSize - DataReceivedInPreviousStrings;
      recv_data = Obj->NetCtx.AppBuffRecvData;
      break;
    case RECV_TYPE_MQTT:
      app_buffer_remaining_space = Obj->MQTTCtx.AppBuffRecvDataSize - DataReceivedInPreviousStrings;
      recv_data = Obj->MQTTCtx.AppBuffRecvData;
      break;
    case RECV_TYPE_BLE:
      app_buffer_remaining_space = Obj->BleCtx.AppBuffRecvDataSize - DataReceivedInPreviousStrings;
      recv_data = Obj->BleCtx.AppBuffRecvData;
      break;
    default:
      break;
  }

  if (recv_data != NULL)
  {
    if (app_buffer_remaining_space < len)
    {
      len = app_buffer_remaining_space;
      /* Notice that even in the application buffer there is no space to copy all received data,
         All expected data (LenExtractedFromDataHeader) shall be received before exiting the RecvDataType */
      LogWarn("Not enough space in the application buffer to copy all received data\n");
    }
    memcpy(recv_data + DataReceivedInPreviousStrings, p_data, len);
  }
  else
  {
    LogWarn("The application shall set the receiving buffer pointer otherwise data are lost\n");
  }
  return;
}

static void ATD_SendDataHeaderToUpperLayer(W61_Object_t *Obj, int8_t RecvDataType)
{
  switch (RecvDataType)
  {
    case RECV_TYPE_NET:
      /* The Data Header message is forwarded as a RESP to the blocking CMD
         via the Obj->ATD_Resp_Queue */
    {
      W61_resp_t *resp = ATD_RespAlloc(W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER);
      if (resp == NULL)
      {
        return;
      }
      resp->len = W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER;
      memcpy(resp->buf, RetainRecvDataHeader, W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER);
      if (xQueueSendToBack(Obj->ATD_Resp_Queue, &resp, 0) != pdPASS)
      {
        ATD_RespFree(resp);
        LogError("Evt queue full, message dropped\n");
      }
    }
    break;
    case RECV_TYPE_MQTT:
      /* The Data Header message is forwarded as EVENT to the w61_at_mqtt.c by calling MQTT event function */
      if (Obj->MQTT_event_cb != NULL)
      {
        Obj->MQTT_event_cb(Obj, RetainRecvDataHeader, W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER);
      }
      break;
    case RECV_TYPE_BLE:
      /* The Data Header message is forwarded as EVENT to the w61_at_ble.c by calling BLE event function */
      if (Obj->Ble_event_cb != NULL)
      {
        Obj->Ble_event_cb(Obj, RetainRecvDataHeader, W61_ATD_MAX_SIZEOF_RECV_DATA_HEADER);
      }
      break;
    default:
      break;
  }
  return;
}

static bool ATD_DetectEvent(W61_Object_t *Obj, uint8_t *p_evt, int32_t single_evt_length)
{
  uint32_t i = 0;

  for (i = 0; i < ITEMS_IN_EVT_LIST; i++)
  {
    /* Check if the event is in the list */
    if (strncmp((char *) p_evt, AtEvtList[i].evtDetectCharacters, AtEvtList[i].evtDetectCharactersLen) == 0)
    {
      W61_resp_t *resp = ATD_RespAlloc(single_evt_length);
      if (resp == NULL)
      {
        return true;
      }

      resp->type = AtEvtList[i].type; /* Set the type of the event */
      memcpy(resp->buf, p_evt, single_evt_length);

      if (xQueueSendToBack(Obj->ATD_Evt_Queue, &resp, 0) != pdPASS)
      {
        ATD_RespFree(resp);
        LogError("Evt queue full, message dropped\n");
      }
      return true;
    }
  }

  return false;
}

static int32_t ATD_RxDispatcher_func(W61_Object_t *Obj, uint8_t *p_input, int32_t string_len)
{
  static int32_t DataReceivedInPreviousStrings = 0;
  /* 0: in non-data, 1: NET, 2: MQTT, 3: BLE */
  static int8_t  RecvDataType = RECV_TYPE_NONE;
  /** Value extracted from the DataHeader of the events listed in AtDataEvtList[] */
  static int32_t LenExtractedFromDataHeader = 0;

  int32_t single_msg_length = 0;
  int32_t remainLen = string_len; /* Part of the string not yet processed */
  int32_t next_loop_index = 0;
  uint32_t unwanted_crlf_count; /* Count in case of several consecutive CRLF, e.i. if \r\n\r\n crlf_count=2 */
  uint8_t *p_msg = p_input;
  uint32_t loops_count = 0; /* String cannot contain more the MAX_PARSER_LOOPS messages */
  int32_t data_offset = 0;
  int32_t max_copy_len = 0;
  int32_t len_data_header_plus_crlf;

  while (remainLen > 0)
  {
    /* All flags are reset each loop cycle */
    loops_count++; /* Loop to check if next message in the string */

    /* ------------------- Process the DATA ------------------------------------------------------------------------ */
    if (RecvDataType != RECV_TYPE_NONE) /* Special case when pulling data (set by CheckIfStartDataEvent) */
    {
      if (string_len > data_offset) /* Check if DATA available at the p_input pointer in the ATD_RecvBuf buffer */
      {
        /* data_offset is always 0 a part when ATD_ProcessDataHeader() detects a data_header and loops
           in such case data_offset correspond to the length of the DataHeader */
        max_copy_len = configMIN(string_len - data_offset, LenExtractedFromDataHeader - DataReceivedInPreviousStrings);

        /* Copy the received data to the buffer set by the application in the Obj (NetCtx, HTTPCtx, MQTTCtx, BLECtx) */
        ATD_CopyDataToAppBuffer(Obj, RecvDataType, DataReceivedInPreviousStrings, p_msg + data_offset, max_copy_len);

        len_data_header_plus_crlf = LenExtractedFromDataHeader + strlen("\r\n");

        /* Check if all data (as expected from len_data_header_plus_crlf) has been received */
        if ((string_len - data_offset) < (len_data_header_plus_crlf - DataReceivedInPreviousStrings))
        {
          /* Not all data has been received
             Update accumulation for next string and return */
          DataReceivedInPreviousStrings += (string_len - data_offset - next_loop_index);
          return 0; /* Return to the W61_ATD_RxPooling_task to wait the rest of the data */
        }
        else /* All len_data_header_plus_crlf data received: can be sent to upper layer */
        {
          ATD_SendDataHeaderToUpperLayer(Obj, RecvDataType);
          RecvDataType = RECV_TYPE_NONE;   /* All data received: switch back to Msg mode */
          /* Check if after data some other messages are in the pipe */
          if ((string_len - data_offset) > (len_data_header_plus_crlf - DataReceivedInPreviousStrings))
          {
            next_loop_index += data_offset + len_data_header_plus_crlf - DataReceivedInPreviousStrings; /* Loops */
          }
          else
          {
            return 0; /* Nominal case: no other messages after DATA, return to the W61_ATD_RxPooling_task */
          }
        }
      }
      else /* No data available */
      {
        return 0; /* Return to the W61_ATD_RxPooling_task to wait the rest of the data */
      }
    }
    /* ------------------- Process the MESSAGES -------------------------------------------------------------------- */
    else /* (!RecvDataType) Nominal case when receiving AT Messages */
    {
      /* \r\n at the begin of the string are removed, except "\r\n>" which does not have \r\n afterwards */
      unwanted_crlf_count = ATD_CountCrlfAtBeginMsg(p_msg);
      p_msg += unwanted_crlf_count;
      next_loop_index += unwanted_crlf_count;

      /* If message is a special case "\r\n>" it calls xQueueSendToBack(Obj->ATD_Resp_Queue, ...) */
      if (IS_CHAR_CR(p_msg[0]) && IS_CHAR_LF(p_msg[1]) && IS_CHAR_SEND(p_msg[2]))
      {
        single_msg_length = strlen("\r\n>");
        W61_resp_t *resp = ATD_RespAlloc(single_msg_length);
        if (resp == NULL)
        {
          return 0;
        }

        memcpy(resp->buf, p_msg, single_msg_length);
        if (xQueueSendToBack(Obj->ATD_Resp_Queue, &resp, 0) != pdPASS)
        {
          ATD_RespFree(resp);;
          LogError("Log queue full, message dropped\n");
        }
        next_loop_index += strlen("\r\n>");
        goto prepare_next_loop;
      }

      /* Check if new data is about to arrive, by checking if the complete Data Header is received
         By complete data header it means the keyword + the len + the comma, e.g. +CIPRECVDATA:<len>,
         If no data header (or dataheader incomplete) the ATD_ProcessDataHeader function returns data_offset = 0
         RecvDataType and LenExtractedFromDataHeader are set by this function */
      data_offset = ATD_ProcessDataHeader(p_msg, string_len, &RecvDataType, &LenExtractedFromDataHeader);
      if (data_offset > 0) /* Data_offset != 0 means a new complete DataHeader has been found */
      {
        /* DataReceivedInPreviousStrings is set back to 0 and code loops to retrieve the data */
        DataReceivedInPreviousStrings = 0;
        goto prepare_next_loop;
      }

      /* Below code checks if message is complete by searching \r\n
         1) if message ("DataHeader", Event or Response) is incomplete, function returns to the W61_ATD_RxPooling_task
         2) if message is an event it ends up calling xQueueSendToBack(Obj->ATD_Evt_Queue, ...)
         3) if message is a response it ends up calling xQueueSendToBack(Obj->ATD_Resp_Queue, ...) */
      single_msg_length  = ATD_SearchCrlfDelimitingEndOfMsg(p_msg, string_len - next_loop_index);
      if (single_msg_length < 3) /* single_msg_length < 3 means CRLF not found message is not complete */
      {
        /* 1) Return to the W61_ATD_RxPooling_task to wait the rest of the message */
        return string_len - next_loop_index;
      }
      else /* single_msg_length > 2 means \r\n found, message is complete */
      {
        /* 2) Check if the message is an Event, in such case it will be queued in the Obj->ATD_Evt_xMessageBuffer */
        if (!ATD_DetectEvent(Obj, p_msg, single_msg_length))
        {
          if ((strnstr((char *)p_msg, "SEND OK", single_msg_length) == NULL) && /* filter SEND OK and SEND FAIL */
              (strnstr((char *)p_msg, "SEND FAIL", single_msg_length) == NULL))
          {
            W61_resp_t *resp = ATD_RespAlloc(single_msg_length);
            if (resp == NULL)
            {
              return 0;
            }

            memcpy(resp->buf, p_msg, single_msg_length);
            if (xQueueSendToBack(Obj->ATD_Resp_Queue, &resp, 0) != pdPASS)
            {
              ATD_RespFree(resp);
              LogError("Log queue full, message dropped\n");
            }
          }
#if (SYS_DBG_ENABLE_TA4 >= 1)
          vTracePrintF("AT_RxDispatcher_func", "rx_task: Send to Response ATD_Resp_Queue");
#endif /* SYS_DBG_ENABLE_TA4 */
        }
        next_loop_index += single_msg_length;
      }
    }

prepare_next_loop:
    /* Execution can get up to here either if CR-LF was found,
       in such case it checks if other messages are in the pipe
       Or when all DATA is arrived but the string is not completely parsed (other messages behind it) */
    remainLen = string_len - next_loop_index;
    if (remainLen > 0)
    {
      /* Update pointer to next message */
      p_msg = &p_input[next_loop_index];
    }
    if (loops_count > MAX_PARSER_LOOPS)
    {
      LogError("Error ATD_RxDispatcher_func() is stuck in the while loop\n");
      return 0;
    }
  }
  return 0;
}

static W61_resp_t *ATD_RespAlloc(size_t size)
{
  W61_resp_t *resp = (W61_resp_t *) pvPortMalloc(sizeof(W61_resp_t));
  if (resp == NULL)
  {
    return NULL;
  }
  resp->buf = pvPortMalloc(size);
  if (resp->buf == NULL)
  {
    vPortFree(resp);
    return NULL;
  }
  resp->len = size;
  return resp;
}

static void ATD_RespFree(W61_resp_t *resp)
{
  vPortFree(resp->buf);
  vPortFree(resp);
}

/** @} */
