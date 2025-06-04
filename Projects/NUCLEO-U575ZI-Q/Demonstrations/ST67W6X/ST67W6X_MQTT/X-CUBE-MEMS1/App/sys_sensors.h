/**
  ******************************************************************************
  * @file    sys_sensors.h
  * @author  GPM Application Team
  * @brief   Header for sensors application
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
#ifndef SYS_SENSORS_H
#define SYS_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
/* Config ------------------------------------------------------------------*/
#define SENSOR_ENABLED          1

/* Exported types ------------------------------------------------------------*/
typedef struct
{
  int32_t x_dir;
  int32_t y_dir;
  int32_t z_dir;
} Sys_Sensors_Axes_t;

/**
  * Sensor data parameters
  */
typedef struct
{
  float pressure;         /*!< in mbar */
  float temperature;      /*!< in degC */
  float humidity;         /*!< in % */
  Sys_Sensors_Axes_t acceleration;
  Sys_Sensors_Axes_t magnetic_field;
  Sys_Sensors_Axes_t angular_velocity;
} Sys_Sensors_Data_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
/**
  * @brief  initialize the environmental & motion sensors
  */
int32_t Sys_Sensors_Init(void);

/**
  * @brief  Environmental & motion sensors read.
  * @param  data sensor data
  */
int32_t Sys_Sensors_Read(Sys_Sensors_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* SYS_SENSORS_H */
