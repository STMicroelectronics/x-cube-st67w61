/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    https_client.c
  * @author  ST67 Application Team
  * @brief   Https client application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "https_client.h"
#include "main.h"

#include "FreeRTOS.h"
#include "w6x_api.h"
#include "common_parser.h" /* Common Parser functions */
#include "shell.h"
#include "cJSON.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/**
  *  @brief  City structure
  */
typedef struct
{
  char *name;           /*!< City name */
  float latitude;       /*!< City latitude */
  float longitude;      /*!< City longitude */
} city_t;

/**
  *  @brief  Metrics structure
  */
typedef struct
{
  char *name;           /*!< Metrics name */
  char *tag;            /*!< Metrics tag */
} metrics_t;

/**
  *  @brief  Weather codes structure
  */
typedef struct
{
  int32_t code;         /*!< Weather code */
  char *string;         /*!< Weather string */
} weather_codes_t;

/**
  *  @brief  Wind direction structure
  */
typedef struct
{
  float dir;            /*!< Wind direction in degrees */
  char string[3];       /*!< Wind direction in cardinal direction */
} wind_dir_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
/** API Weather server url */
#define WEATHER_SERVER_NAME "api.open-meteo.com"

/** API Weather server url request */
#define WEATHER_URL_REQUEST "https://" WEATHER_SERVER_NAME "/v1/forecast?latitude=%3.2f&" \
  "longitude=%3.2f&timezone=auto&daily=%s"

/** Weather request minimum temperature parameter */
#define MIN_TEMP_TAG       "temperature_2m_min"

/** Weather request maximum temperature parameter */
#define MAX_TEMP_TAG       "temperature_2m_max"

/** Weather request rain duration parameter */
#define RAIN_DURATION_TAG  "precipitation_hours"

/** Weather request rain quantity parameter */
#define RAIN_QUANTITY_TAG  "rain_sum"

/** Weather request wind direction parameter */
#define WIND_DIR_TAG       "wind_direction_10m_dominant"

/** Weather request wind speed parameter */
#define WIND_SPEED_TAG     "wind_speed_10m_max"

/** Weather request weather code parameter */
#define WEATHER_CODE_TAG   "weather_code"

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/** Cities list */
city_t Cities[] =
{
  {"Paris", 48.95, 2.35},
  {"Nice", 43.71, 7.26},
  {"New York", 40.71, -74.00},
  {"London", 51.50, -0.13},
  {"Roma", 41.90, 12.48},
  {"Madrid", 40.42, 3.7},
  {"Dubai", 25.2, 55.27},
  {"Istanbul", 41.0, 28.98},
  {"Singapore", 1.35, 103.82},
  {"Seoul", 37.55, 126.99},
  {"Tokyo", 35.67, 139.65},
  {"Melbourne", -37.81, 144.96},
  {"Miami", 25.76, 80.19},
  {0}
};

/** Weather metrics list */
char *weather_metrics[] =
{
  WEATHER_CODE_TAG,
  MIN_TEMP_TAG,
  MAX_TEMP_TAG,
  RAIN_QUANTITY_TAG,
  RAIN_DURATION_TAG,
  WIND_SPEED_TAG,
  WIND_DIR_TAG,
  NULL
};

/** Weather codes list */
weather_codes_t weather_codes_list[] =
{
  {0, "Clear sky"},
  {1, "Mainly clear sky"},
  {2, "Partly cloudy sky"},
  {3, "Overcast sky"},
  {45, "Fog"},
  {48, "Depositing rime fog"},
  {51, "Light drizzle"},
  {53, "Moderate drizzle"},
  {55, "Dense drizzle"},
  {56, "Light freezing drizzle"},
  {57, "Dense freezing drizzle"},
  {61, "Slight rain"},
  {63, "Moderate rain"},
  {65, "Heavy rain"},
  {66, "Light freezing rain"},
  {67, "Heavy freezing rain"},
  {71, "Slight Snow fall"},
  {73, "Moderate Snow fall"},
  {75, "Heavy Snow Fall"},
  {77, "Snow grains"},
  {80, "Slight rain showers"},
  {81, "Moderate rain showers"},
  {82, "Violent rain showers"},
  {85, "Slight snow showers"},
  {86, "Heavy snow showers"},
  {95, "Thunderstorm"},
  {96, "Thunderstorm with slight hail"},
  {99, "Thunderstorm with heavy hail"},
  {0}
};

/** Wind direction list */
static const wind_dir_t wind_dir_list[] =
{
  {22.5, "N"},
  {67.5, "NE"},
  {112.5, "E"},
  {157.5, "SE"},
  {202.5, "S"},
  {247.5, "SW"},
  {292.5, "W"},
  {337.5, "NW"},
  {361.0, "N"},
};

/* USER CODE BEGIN PV */
#if (LFS_ENABLE == 0)
/** Weather server certificate content. r12.pem subordinate CA issued by ISRG Root X1.
  * Available on https://letsencrypt.org/certificates/
  */
static const char weather_cert[] =
  "-----BEGIN CERTIFICATE-----\r\n"
  "MIIFBjCCAu6gAwIBAgIRAMISMktwqbSRcdxA9+KFJjwwDQYJKoZIhvcNAQELBQAw\r\n"
  "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\r\n"
  "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw\r\n"
  "WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\r\n"
  "RW5jcnlwdDEMMAoGA1UEAxMDUjEyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\r\n"
  "CgKCAQEA2pgodK2+lP474B7i5Ut1qywSf+2nAzJ+Npfs6DGPpRONC5kuHs0BUT1M\r\n"
  "5ShuCVUxqqUiXXL0LQfCTUA83wEjuXg39RplMjTmhnGdBO+ECFu9AhqZ66YBAJpz\r\n"
  "kG2Pogeg0JfT2kVhgTU9FPnEwF9q3AuWGrCf4yrqvSrWmMebcas7dA8827JgvlpL\r\n"
  "Thjp2ypzXIlhZZ7+7Tymy05v5J75AEaz/xlNKmOzjmbGGIVwx1Blbzt05UiDDwhY\r\n"
  "XS0jnV6j/ujbAKHS9OMZTfLuevYnnuXNnC2i8n+cF63vEzc50bTILEHWhsDp7CH4\r\n"
  "WRt/uTp8n1wBnWIEwii9Cq08yhDsGwIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB\r\n"
  "hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB\r\n"
  "/wIBADAdBgNVHQ4EFgQUALUp8i2ObzHom0yteD763OkM0dIwHwYDVR0jBBgwFoAU\r\n"
  "ebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAC\r\n"
  "hhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcG\r\n"
  "A1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcN\r\n"
  "AQELBQADggIBAI910AnPanZIZTKS3rVEyIV29BWEjAK/duuz8eL5boSoVpHhkkv3\r\n"
  "4eoAeEiPdZLj5EZ7G2ArIK+gzhTlRQ1q4FKGpPPaFBSpqV/xbUb5UlAXQOnkHn3m\r\n"
  "FVj+qYv87/WeY+Bm4sN3Ox8BhyaU7UAQ3LeZ7N1X01xxQe4wIAAE3JVLUCiHmZL+\r\n"
  "qoCUtgYIFPgcg350QMUIWgxPXNGEncT921ne7nluI02V8pLUmClqXOsCwULw+PVO\r\n"
  "ZCB7qOMxxMBoCUeL2Ll4oMpOSr5pJCpLN3tRA2s6P1KLs9TSrVhOk+7LX28NMUlI\r\n"
  "usQ/nxLJID0RhAeFtPjyOCOscQBA53+NRjSCak7P4A5jX7ppmkcJECL+S0i3kXVU\r\n"
  "y5Me5BbrU8973jZNv/ax6+ZK6TM8jWmimL6of6OrX7ZU6E2WqazzsFrLG3o2kySb\r\n"
  "zlhSgJ81Cl4tv3SbYiYXnJExKQvzf83DYotox3f0fwv7xln1A2ZLplCb0O+l/AK0\r\n"
  "YE0DS2FPxSAHi0iwMfW2nNHJrXcY3LLHD77gRgje4Eveubi2xxa+Nmk/hmhLdIET\r\n"
  "iVDFanoCrMVIpQ59XWHkzdFmoHXHBV7oibVjGSO7ULSQ7MJ1Nz51phuDJSgAIU7A\r\n"
  "0zrLnOrAj/dfrlEWRhCvAgbuwLZX1A2sjNjXoPOHbsPiy+lO1KF8/XY7\r\n"
  "-----END CERTIFICATE-----\r\n";
#endif /* LFS_ENABLE */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Shell command to get the weather forecast
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t https_weather_shell(int32_t argc, char **argv);

/**
  * @brief  Callback function to receive the data from the server
  * @param  arg: Callback argument
  * @param  p: Pointer to the data
  * @param  err: Error code
  * @return Operation status
  */
static int32_t HTTPS_recv_cb(void *arg, W6X_HTTP_buffer_t *p, int32_t err);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
int32_t https_client_weather(char *city, float latitude, float longitude)
{
  ip_addr_t addr = {0};
  W6X_HTTP_connection_t settings = {0};
  uint8_t https_server_addr[4] = {192, 168, 8, 1};
  int32_t i = 0;

  char *method = "GET";
  char server_name[32] = WEATHER_SERVER_NAME;
  char certificate[16] = "r12.pem";
  char url[256] = {0};
  char *token = NULL;
  char *address = NULL;
  float city_latitude = latitude;
  float city_longitude = longitude;

  /* USER CODE BEGIN https_client_weather_1 */

  /* USER CODE END https_client_weather_1 */

  /* Get the city latitude and longitude */
  if (city != NULL)
  {
    while (Cities[i].name != NULL)
    {
      if (strcmp(city, Cities[i].name) == 0)
      {
        /* City found in the list. Get the latitude and longitude */
        city_latitude = Cities[i].latitude;
        city_longitude = Cities[i].longitude;
        break;
      }
      i++;
    }
  }

  /* Create the URL */
  (void)snprintf(url, sizeof(url), WEATHER_URL_REQUEST,
                 (double)city_latitude, (double)city_longitude, weather_metrics[0]);

  i = 1;
  /* Add the metrics parameters to the URL */
  while (weather_metrics[i] != NULL)
  {
    (void)strncat(url, ",", strlen(",") + 1);
    (void)strncat(url, weather_metrics[i], strlen(weather_metrics[i]) + 1);
    i++;
  }

  token = strstr(url, "//");
  if ((token == NULL) || (strlen(token) < 3U))
  {
    LogError("Mal-formed URL\n");
    return -1;
  }

  /* Get the Base URL */
  address = token + 2;
  char *uri = strstr(address, "/");
  if (uri == NULL)
  {
    LogError("Mal-formed URL\n");
    return -1;
  }

  /* Get the IP address from the Base URL */
  if (W6X_Net_Inet_pton(AF_INET, address, (const void *) &addr.u_addr.ip4.addr) != 0)
  {
    if (W6X_Net_ResolveHostAddress(server_name, https_server_addr) != W6X_STATUS_OK)
    {
      LogError("IP Address identification failed\n");
      return -1;
    }
    addr.u_addr.ip4.addr = ATON(https_server_addr);
  }

  /* Set the HTTPS settings */
  settings.server_name = server_name;
  settings.https_certificate.name = certificate;

  /* USER CODE BEGIN main_app_cert */
#if (LFS_ENABLE == 0)
  settings.https_certificate.content = (char *)weather_cert;
#endif /* LFS_ENABLE */

  /* USER CODE END main_app_cert */

  /* Register Callback and callback argument */
  char *location_name = NULL;
  if (city != NULL)
  {
    /* Store the city name in global buffer to be used by the callback */
    location_name = (char *)pvPortMalloc(strlen(city) + 1);
    if (location_name == NULL)
    {
      return -1;
    }
    (void)strncpy(location_name, city, strlen(city));
    location_name[strlen(city)] = '\0';
    settings.recv_fn_arg = location_name;
  }
  /* Register the callback function */
  settings.recv_fn = HTTPS_recv_cb;

  /* Send the HTTP request. Non-blocking function. The response will be received by the callback */
  W6X_Status_t ret =  W6X_HTTP_Client_Request(&addr, 443, uri, method, NULL, 0, NULL, NULL, NULL, NULL, &settings);
  if (ret != W6X_STATUS_OK)
  {
    if (location_name != NULL)
    {
      vPortFree(location_name);
    }
    return -1;
  }

  /* USER CODE BEGIN https_client_weather_last */

  /* USER CODE END https_client_weather_last */

  return 0;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
static int32_t HTTPS_recv_cb(void *arg, W6X_HTTP_buffer_t *p, int32_t err)
{
  int32_t ret = -1;
  cJSON *root = NULL;
  cJSON *daily_units = NULL;
  cJSON *daily = NULL;
  cJSON *temp_unit = NULL;
  cJSON *duration_unit = NULL;
  cJSON *rain_qty_unit = NULL;
  cJSON *wind_dir_unit = NULL;
  cJSON *wind_speed_unit = NULL;
  cJSON *min_temp = NULL;
  cJSON *max_temp = NULL;
  cJSON *rain_dur = NULL;
  cJSON *rain_qty = NULL;
  cJSON *wind_dir = NULL;
  cJSON *wind_speed = NULL;
  cJSON *weather_code = NULL;
  cJSON_Hooks hooks =
  {
    .malloc_fn = pvPortMalloc,
    .free_fn = vPortFree,
  };

  /* USER CODE BEGIN HTTPS_recv_cb_1 */

  /* USER CODE END HTTPS_recv_cb_1 */
  if ((p == NULL) || (p->data == NULL))
  {
    return ret;
  }

  cJSON_InitHooks(&hooks);

  /* Search for the JSON start */
  char *json_start = strstr((char *)p->data, "{");
  if (json_start == NULL)
  {
    LogError("Malformed Json");
    return -1;
  }
  /* Parse the JSON and return the root object */
  root = cJSON_Parse((const char *)json_start);
  if (root == NULL)
  {
    LogError("Processing error of JSON message\n");
    return -1;
  }
  /* Get the data "daily_units" from the root content */
  daily_units = cJSON_GetObjectItemCaseSensitive(root, "daily_units");
  if (daily_units == NULL)
  {
    LogError("Did not find daily_units tag\n");
    goto _err;
  }
  /* Get the data "daily" from the root content */
  daily = cJSON_GetObjectItemCaseSensitive(root, "daily");
  if (daily == NULL)
  {
    LogError("Did not find daily tag\n");
    goto _err;
  }
  /* Get the city name from the callback argument */
  if (arg != NULL)
  {
    LogInfo("Weather forecast for %s:\n", (char *)arg);
    vPortFree(arg);
  }
  else
  {
    LogInfo("Weather forecast for the region you selected:\n");
  }

  /* Get the daily units objects */
  temp_unit = cJSON_GetObjectItemCaseSensitive(daily_units, MIN_TEMP_TAG);
  duration_unit = cJSON_GetObjectItemCaseSensitive(daily_units, RAIN_DURATION_TAG);
  rain_qty_unit = cJSON_GetObjectItemCaseSensitive(daily_units, RAIN_QUANTITY_TAG);
  wind_dir_unit = cJSON_GetObjectItemCaseSensitive(daily_units, WIND_DIR_TAG);
  wind_speed_unit = cJSON_GetObjectItemCaseSensitive(daily_units, WIND_SPEED_TAG);

  /* Check if the units are present */
  if ((temp_unit == NULL) || (duration_unit == NULL) || (rain_qty_unit == NULL) || (wind_dir_unit == NULL)
      || (wind_speed_unit == NULL))
  {
    LogError("Missing Units from the JSON");
    goto _err;
  }
  /* Check if the units are strings */
  if ((cJSON_IsString(temp_unit) != true) || (cJSON_IsString(duration_unit) != true) ||
      (cJSON_IsString(rain_qty_unit) != true) || (cJSON_IsString(wind_dir_unit) != true) ||
      (cJSON_IsString(wind_speed_unit) != true))
  {
    LogError("Missing Units from the JSON");
    goto _err;
  }

  /* Get the daily objects */
  min_temp = cJSON_GetObjectItemCaseSensitive(daily, MIN_TEMP_TAG);
  max_temp = cJSON_GetObjectItemCaseSensitive(daily, MAX_TEMP_TAG);
  rain_dur = cJSON_GetObjectItemCaseSensitive(daily, RAIN_DURATION_TAG);
  rain_qty = cJSON_GetObjectItemCaseSensitive(daily, RAIN_QUANTITY_TAG);
  wind_dir = cJSON_GetObjectItemCaseSensitive(daily, WIND_DIR_TAG);
  wind_speed = cJSON_GetObjectItemCaseSensitive(daily, WIND_SPEED_TAG);
  weather_code = cJSON_GetObjectItemCaseSensitive(daily, WEATHER_CODE_TAG);

  /* Check if the data are present */
  if ((min_temp == NULL) || (max_temp == NULL) || (rain_dur == NULL) || (rain_qty == NULL) ||
      (wind_dir == NULL) || (wind_speed == NULL) || (weather_code == NULL))
  {
    LogError("Missing data from the JSON");
    goto _err;
  }
  /* Check if the data are arrays */
  if ((cJSON_IsArray(min_temp) != true) || (cJSON_IsArray(max_temp) != true) || (cJSON_IsArray(rain_dur) != true) ||
      (cJSON_IsArray(rain_qty) != true) || (cJSON_IsArray(wind_dir) != true) || (cJSON_IsArray(wind_speed) != true) ||
      (cJSON_IsArray(weather_code) != true))
  {
    LogError("Wrong format\n");
    goto _err;
  }

  /* USER CODE BEGIN HTTPS_recv_cb_2 */

  /* USER CODE END HTTPS_recv_cb_2 */

  /* Loop through the daily data */
  for (int32_t day = 0; day < 7; day++)
  {
    if (day == 0)
    {
      LogInfo("Today:\n");
    }
    else if (day == 1)
    {
      LogInfo("Tomorrow:\n");
    }
    else
    {
      LogInfo("In %" PRIi32 " days:\n", day);
    }

    int32_t i = 0;
    char weather_code_string[30] = {0};
    /* Search the weather code in the list */
    int32_t weather_code_value = cJSON_GetArrayItem(weather_code, day)->valueint;
    while (weather_codes_list[i].string != NULL)
    {
      if (weather_codes_list[i].code == weather_code_value)
      {
        /* Weather code found. Copy the weather code string */
        (void)strncpy(weather_code_string, weather_codes_list[i].string, sizeof(weather_code_string) - 1);
        break;
      }
      i++;
    }

    /* Convert the wind dir from degrees to cardinal direction */
    float wind_dir_value = cJSON_GetArrayItem(wind_dir, day)->valuedouble;
    char *wind_dir_string = NULL;
    for (i = 0; i < (sizeof(wind_dir_list) / sizeof(wind_dir_t)); i++)
    {
      if (wind_dir_value < wind_dir_list[i].dir)
      {
        wind_dir_string = (char *)wind_dir_list[i].string;
        break;
      }
    }

    /* Print the weather forecast */
    LogInfo("\t%s,\n"
            "\tTemperature between  %.1f%s and  %.1f%s,\n"
            "\tWind blows up to %.1f%s mainly at %.0f%s (%s),\n"
            "\t%.0f%s of rain within a span of %.0f%s\n",
            weather_code_string,
            cJSON_GetArrayItem(min_temp, day)->valuedouble, temp_unit->valuestring,
            cJSON_GetArrayItem(max_temp, day)->valuedouble, temp_unit->valuestring,
            cJSON_GetArrayItem(wind_speed, day)->valuedouble, wind_speed_unit->valuestring,
            cJSON_GetArrayItem(wind_dir, day)->valuedouble, wind_dir_unit->valuestring, wind_dir_string,
            cJSON_GetArrayItem(rain_qty, day)->valuedouble, rain_qty_unit->valuestring,
            cJSON_GetArrayItem(rain_dur, day)->valuedouble, duration_unit->valuestring
           );
  }
  ret = 0;

  /* USER CODE BEGIN HTTPS_recv_cb_last */

  /* USER CODE END HTTPS_recv_cb_last */

_err:
  cJSON_Delete(root);
  return ret;
}

int32_t https_weather_shell(int32_t argc, char **argv)
{
  if (argc != 2)
  {
    return SHELL_STATUS_UNKNOWN_ARGS;
  }
  int32_t i = 0;

  /* USER CODE BEGIN https_weather_shell_1 */

  /* USER CODE END https_weather_shell_1 */

  /* Search the city in the list */
  while (Cities[i].name != NULL)
  {
    if (strcmp(argv[1], Cities[i].name) == 0)
    {
      /* City found in the list. Get the weather forecast */
      if (https_client_weather(Cities[i].name, Cities[i].latitude, Cities[i].longitude) != 0)
      {
        return SHELL_STATUS_ERROR;
      }

      return SHELL_STATUS_OK;
    }
    i++;
  }

  LogInfo("Unknown City, available cities are:\n");
  i = 0;
  /* Print the available cities */
  while (Cities[i].name != NULL)
  {
    LogInfo("-%s\n", Cities[i].name);
    i++;
  }

  /* USER CODE BEGIN https_weather_shell_last */

  /* USER CODE END https_weather_shell_last */

  return SHELL_STATUS_ERROR;
}

SHELL_CMD_EXPORT_ALIAS(https_weather_shell, weather, weather < City >. Display the 7 days weather of specific city);

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
