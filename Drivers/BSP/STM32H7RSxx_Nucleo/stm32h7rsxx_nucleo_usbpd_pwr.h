/**
  ******************************************************************************
  * @file    stm32h7rsxx_nucleo_usbpd_pwr.h
  * @author  MCD Application Team
  * @brief   Header file for stm32h7rsxx_nucleo_usbpd_pwr.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32H7RSXX_NUCLEO_USBPD_PWR_H
#define STM32H7RSXX_NUCLEO_USBPD_PWR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7rsxx_nucleo_conf.h"
#include "stm32h7rsxx_nucleo_errno.h"
#include "stm32h7rsxx_hal_adc.h"
#include "stm32h7rsxx_ll_bus.h"
#include "stm32h7rsxx_ll_gpio.h"
#include "stm32h7rsxx_ll_exti.h"
#include "stm32h7rsxx_ll_system.h"
#include "../Components/tcpp0203/tcpp0203.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32H7RSXX_NUCLEO
  * @{
  */

/** @defgroup USBPD_PWR USBPD_PWR
  * @{
  */

/** @defgroup STM32H7RSXX_NUCLEO_USBPD_PWR_Exported_Types Exported Types
  * @{
  */

#if !defined(USBPD_PWR_USER_H_)

/**
  * @brief  Power role
  */
typedef enum
{
  POWER_ROLE_SOURCE = 0,
  POWER_ROLE_SINK,
  POWER_ROLE_DUAL
} USBPD_PWR_PowerRoleTypeDef;

/**
  * @brief  Low Power Mode of TypeC ports
  */
typedef enum
{
  USBPD_PWR_MODE_OFF = 0,
  USBPD_PWR_MODE_HIBERNATE,
  USBPD_PWR_MODE_LOWPOWER,
  USBPD_PWR_MODE_NORMAL
} USBPD_PWR_PowerModeTypeDef;

/**
  * @brief  VBUS connection status
  */
typedef enum
{
  VBUS_CONNECTED = 0,
  VBUS_NOT_CONNECTED
} USBPD_PWR_VBUSConnectionStatusTypeDef;

/**
  * @brief  ADC buffer identifiers
  */
typedef enum
{
  ADCBUF_VSENSE = 0,
  ADCBUF_ISENSE
} USBPD_PWR_ADCBufIDTypeDef;

/**
  * @brief VBUS Detection Callback
  */
typedef void USBPD_PWR_VBUSDetectCallbackFunc(uint32_t PortNum,
                                              USBPD_PWR_VBUSConnectionStatusTypeDef VBUSConnectionStatus);

/**
  * @}
  */

/** @defgroup STM32H7RSXX_NUCLEO_USBPD_PWR_Exported_Constants Exported Constants
  * @{
  */
/**
  * @brief  HW configuration type of TypeC ports
  */
#define USBPD_PWR_HW_CONFIG_TYPE_DEFAULT          (0U)       /*!< Default configuration :
                                                                  Port available natively on board, no protection  */
#define USBPD_PWR_HW_CONFIG_TYPE_TCPP01           (1U)       /*!< TypeC Port protection available
                                                                  thanks to a TCPP01 component                     */
#define USBPD_PWR_HW_CONFIG_TYPE_TCPP02           (2U)       /*!< TypeC Port protection available
                                                                  thanks to a TCPP02 component                     */
#define USBPD_PWR_HW_CONFIG_TYPE_TCPP03           (3U)       /*!< TypeC Port protection available
                                                                  thanks to a TCPP03 component                     */


/**
  * @brief  Number of TypeC ports
  */
#define USBPD_PWR_INSTANCES_NBR           (1U)

/**
  * @brief  Type-C port identifier
  */
#define USBPD_PWR_TYPE_C_PORT_1           (0U)
#define USBPD_PWR_TYPE_C_PORT_2           (1U)
#define USBPD_PWR_TYPE_C_PORT_3           (2U)

/**
  * @brief  CC pin identifier
  */
#define USBPD_PWR_TYPE_C_CC1              (1U)
#define USBPD_PWR_TYPE_C_CC2              (2U)

/**
  * @brief  VBUS disconnection threshold values (in mV)
  */
#define USBPD_PWR_HIGH_VBUS_THRESHOLD     (2800U)
#define USBPD_PWR_LOW_VBUS_THRESHOLD      (750U)
#define USBPD_PWR_VBUS_THRESHOLD_5V       (3900U)
#define USBPD_PWR_VBUS_THRESHOLD_9V       (7000U)
#define USBPD_PWR_VBUS_THRESHOLD_15V      (12500U)
#define USBPD_PWR_VBUS_THRESHOLD_20V      (17000U)
#define USBPD_PWR_VBUS_THRESHOLD_APDO     (2150U)

/**
  * @brief  VBUS discharge parameters
  */
#define USBPD_PWR_DISCHARGE_MARGIN        (500U)
#define USBPD_PWR_DISCHARGE_TIME          (6U)

/**
  * @brief  Standard VBUS voltage levels
  */
#define USBPD_PWR_VBUS_5V                 5000U
#define USBPD_PWR_VBUS_9V                 9000U
#define USBPD_PWR_VBUS_15V                15000U

/**
  * @brief  power timeout
  */
#define USBPD_PWR_TIMEOUT_PDO             250U         /* Timeout for PDO to PDO or PDO to APDO at 250ms */
#define USBPD_PWR_TIMEOUT_APDO            25U          /* Timeout for APDO to APDO at 25ms */

/**
  * @brief  Invalid value set during issue with voltage setting
  */
#define USBPD_PWR_INVALID_VALUE           0xFFFFFFFFU

#endif /* !USBPD_PWR_USER_H_ */

/**
  * @brief TCPP0203_FLGn pin
  * To be defined for each Port, protected by a TCPP0203 component
  */
#define TCPP0203_PORT0_FLG_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOM_CLK_ENABLE()
#define TCPP0203_PORT0_FLG_GPIO_CLK_DISABLE()       __HAL_RCC_GPIOM_CLK_DISABLE()
#define TCPP0203_PORT0_FLG_GPIO_PORT                GPIOM
#define TCPP0203_PORT0_FLG_GPIO_PIN                 LL_GPIO_PIN_8
#define TCPP0203_PORT0_FLG_GPIO_MODE                LL_GPIO_MODE_INPUT
#define TCPP0203_PORT0_FLG_GPIO_PUPD                LL_GPIO_PULL_UP
#define TCPP0203_PORT0_FLG_SET_EXTI()                             \
  do                                                              \
  {                                                               \
    __HAL_RCC_SBS_CLK_ENABLE();                                   \
    LL_SBS_SetEXTISource(LL_SBS_EXTI_PORTM, LL_SBS_EXTI_LINE8);   \
  } while(0);
#define TCPP0203_PORT0_FLG_EXTI_LINE                LL_EXTI_LINE_8
#define TCPP0203_PORT0_FLG_EXTI_ENABLE()                          \
  do                                                              \
  {                                                               \
    LL_EXTI_DisableEvent_0_31(TCPP0203_PORT0_FLG_EXTI_LINE);      \
    LL_EXTI_EnableIT_0_31(TCPP0203_PORT0_FLG_EXTI_LINE);          \
  } while(0);
#define TCPP0203_PORT0_FLG_EXTI_DISABLE()           LL_EXTI_DisableIT_0_31(TCPP0203_PORT0_FLG_EXTI_LINE);
#define TCPP0203_PORT0_FLG_TRIG_ENABLE()                          \
  do                                                              \
  {                                                               \
    LL_EXTI_DisableRisingTrig_0_31(TCPP0203_PORT0_FLG_EXTI_LINE); \
    LL_EXTI_EnableFallingTrig_0_31(TCPP0203_PORT0_FLG_EXTI_LINE); \
  } while(0);
#define TCPP0203_PORT0_FLG_TRIG_DISABLE()           LL_EXTI_DisableFallingTrig_0_31(TCPP0203_PORT0_FLG_EXTI_LINE);
#define TCPP0203_PORT0_FLG_EXTI_IS_ACTIVE_FLAG()    LL_EXTI_IsActiveFlag_0_31(TCPP0203_PORT0_FLG_EXTI_LINE)
#define TCPP0203_PORT0_FLG_EXTI_CLEAR_FLAG()        LL_EXTI_ClearFlag_0_31(TCPP0203_PORT0_FLG_EXTI_LINE);
#define TCPP0203_PORT0_FLG_EXTI_IRQN                EXTI8_IRQn
#define TCPP0203_PORT0_FLG_EXTI_IRQHANDLER          EXTI8_IRQHandler

#define TCPP0203_PORT0_FLG_IT_PRIORITY              (12U)
#define TCPP0203_PORT0_FLG_GENERATE_IT()            LL_EXTI_GenerateSWI_0_31(TCPP0203_PORT0_FLG_EXTI_LINE);

/**
  * @brief TCPP0203_PWRENn pin
  */
#define TCPP0203_PORT0_ENABLE_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOM_CLK_ENABLE()
#define TCPP0203_PORT0_ENABLE_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOM_CLK_DISABLE()
#define TCPP0203_PORT0_ENABLE_GPIO_PORT             GPIOM
#define TCPP0203_PORT0_ENABLE_GPIO_PIN              LL_GPIO_PIN_9
#define TCPP0203_PORT0_ENABLE_GPIO_MODE             LL_GPIO_MODE_OUTPUT
#define TCPP0203_PORT0_ENABLE_GPIO_OUTPUT           LL_GPIO_OUTPUT_PUSHPULL
#define TCPP0203_PORT0_ENABLE_GPIO_PUPD             LL_GPIO_PULL_NO
#define TCPP0203_PORT0_ENABLE_GPIO_DEFVALUE()       LL_GPIO_ResetOutputPin(TCPP0203_PORT0_ENABLE_GPIO_PORT,\
                                                                           TCPP0203_PORT0_ENABLE_GPIO_PIN);
#define TCPP0203_PORT0_ENABLE_GPIO_SET()            LL_GPIO_SetOutputPin(TCPP0203_PORT0_ENABLE_GPIO_PORT,\
                                                                         TCPP0203_PORT0_ENABLE_GPIO_PIN);
#define TCPP0203_PORT0_ENABLE_GPIO_RESET()          LL_GPIO_ResetOutputPin(TCPP0203_PORT0_ENABLE_GPIO_PORT,\
                                                                           TCPP0203_PORT0_ENABLE_GPIO_PIN);

#define TCPP0203_PORT0_IANA_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOF_CLK_ENABLE()
#define TCPP0203_PORT0_IANA_GPIO_CLK_DISABLE()      __HAL_RCC_GPIOF_CLK_DISABLE()
#define TCPP0203_PORT0_IANA_GPIO_PORT               GPIOF
#define TCPP0203_PORT0_IANA_GPIO_PIN                LL_GPIO_PIN_12
#define TCPP0203_PORT0_IANA_GPIO_MODE               LL_GPIO_MODE_ANALOG

#define TCPP0203_PORT0_VBUSC_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOF_CLK_ENABLE()
#define TCPP0203_PORT0_VBUSC_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOF_CLK_DISABLE()
#define TCPP0203_PORT0_VBUSC_GPIO_PORT              GPIOF
#define TCPP0203_PORT0_VBUSC_GPIO_PIN               LL_GPIO_PIN_13
#define TCPP0203_PORT0_VBUSC_GPIO_MODE              LL_GPIO_MODE_ANALOG
#define TCPP0203_PORT0_VBUSC_ADC_INSTANCE           ADC2
#define TCPP0203_PORT0_VBUSC_ADC_COMMON             ADC12_COMMON
#define TCPP0203_PORT0_VBUSC_ADC_RANK               LL_ADC_REG_RANK_1
#define TCPP0203_PORT0_VBUSC_ADC_CHANNEL            LL_ADC_CHANNEL_2
#define TCPP0203_PORT0_VBUSC_ADC_CLK_ENABLE()       __HAL_RCC_ADC12_CLK_ENABLE()

#define TCPP0203_PORT0_VBUSC_RA                     3300u /* VBUS voltage divider RA */
#define TCPP0203_PORT0_VBUSC_RB                     499u  /* VBUS voltage divider RB */

#define TCPP0203_PORT0_IANA_ISENSE_GA               42u   /* Current measure gain In V/V */
#define TCPP0203_PORT0_IANA_ISENSE_RS               22u   /* Current measure shunt resistor in milliohm */

/**
  * @}
  */

/** @addtogroup STM32H7RSXX_NUCLEO_USBPD_PWR_Exported_Variables
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32H7RSXX_NUCLEO_USBPD_PWR_Exported_Functions Exported Functions
  * @{
  */
/* Common functions */
int32_t BSP_USBPD_PWR_Init(uint32_t PortNum);
int32_t BSP_USBPD_PWR_Deinit(uint32_t PortNum);

int32_t BSP_USBPD_PWR_SetRole(uint32_t PortNum, USBPD_PWR_PowerRoleTypeDef Role);
int32_t BSP_USBPD_PWR_SetPowerMode(uint32_t PortNum, USBPD_PWR_PowerModeTypeDef PwrMode);
int32_t BSP_USBPD_PWR_GetPowerMode(uint32_t PortNum, USBPD_PWR_PowerModeTypeDef *PwrMode);

int32_t BSP_USBPD_PWR_VBUSInit(uint32_t PortNum);
int32_t BSP_USBPD_PWR_VBUSDeInit(uint32_t PortNum);

int32_t BSP_USBPD_PWR_VBUSOn(uint32_t PortNum);
int32_t BSP_USBPD_PWR_VBUSOff(uint32_t PortNum);
int32_t BSP_USBPD_PWR_VBUSIsOn(uint32_t PortNum, uint8_t *pState);

int32_t BSP_USBPD_PWR_VBUSSetVoltage_Fixed(uint32_t PortNum, uint32_t VbusTargetInmv, uint32_t OperatingCurrent,
                                           uint32_t MaxOperatingCurrent);
int32_t BSP_USBPD_PWR_VBUSSetVoltage_Variable(uint32_t PortNum, uint32_t VbusTargetMaxInmv,
                                              uint32_t VbusTargetMinInmv, uint32_t OperatingCurrent,
                                              uint32_t MaxOperatingCurrent);
int32_t BSP_USBPD_PWR_VBUSSetVoltage_Battery(uint32_t PortNum, uint32_t VbusTargetMin, uint32_t VbusTargetMax,
                                             uint32_t OperatingPower, uint32_t MaxOperatingPower);
int32_t BSP_USBPD_PWR_VBUSSetVoltage_APDO(uint32_t PortNum, uint32_t VbusTargetInmv, uint32_t OperatingCurrent,
                                          int32_t Delta);

int32_t BSP_USBPD_PWR_SetVBUSDisconnectionThreshold(uint32_t PortNum, uint32_t VoltageThreshold);

int32_t BSP_USBPD_PWR_RegisterVBUSDetectCallback(uint32_t PortNum,
                                                 USBPD_PWR_VBUSDetectCallbackFunc *pfnVBUSDetectCallback);

int32_t BSP_USBPD_PWR_VBUSGetVoltage(uint32_t PortNum, uint32_t *pVoltage);
int32_t BSP_USBPD_PWR_VBUSGetCurrent(uint32_t PortNum, int32_t *pCurrent);

int32_t BSP_USBPD_PWR_VBUSDischargeOn(uint32_t PortNum);
int32_t BSP_USBPD_PWR_VBUSDischargeOff(uint32_t PortNum);

int32_t BSP_USBPD_PWR_VCONNInit(uint32_t PortNum, uint32_t CCPinId);
int32_t BSP_USBPD_PWR_VCONNDeInit(uint32_t PortNum, uint32_t CCPinId);

int32_t BSP_USBPD_PWR_VCONNOn(uint32_t PortNum, uint32_t CCPinId);
int32_t BSP_USBPD_PWR_VCONNOff(uint32_t PortNum, uint32_t CCPinId);
int32_t BSP_USBPD_PWR_VCONNIsOn(uint32_t PortNum, uint32_t CCPinId, uint8_t *pState);

int32_t BSP_USBPD_PWR_VCONNDischargeOn(uint32_t PortNum);
int32_t BSP_USBPD_PWR_VCONNDischargeOff(uint32_t PortNum);

void    BSP_USBPD_PWR_EventCallback(uint32_t PortNum);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32H7RSXX_NUCLEO_USBPD_PWR_H */

