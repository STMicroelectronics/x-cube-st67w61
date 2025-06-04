/**
  ******************************************************************************
  * @file    iperf_shell.c
  * @author  GPM Application Team
  * @brief   Iperf shell command implementation
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
#include <string.h>
#include <stdint.h>

#include "iperf.h"
#include "w6x_api.h"
#include "shell.h"
#include "common_parser.h" /* Common Parser functions */

/* Private constants ---------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Iperf_Constants ST67W6X Utility Performance Iperf Constants
  * @ingroup  ST67W6X_Utilities_Performance_Iperf
  * @{
  */

#define IPERF_DEFAULT_PORT          5001            /*!< Default port */
#define IPERF_DEFAULT_TIME          10              /*!< Default time */
#define IPERF_NO_BW_LIMIT           -1              /*!< No bandwidth limit */
#define IPERF_DEFAULT_BW_LIMIT      1               /*!< UDP default bandwidth limit */

/** @} */

/* Private macros ------------------------------------------------------------*/
/** @defgroup ST67W6X_Utilities_Performance_Iperf_Macros ST67W6X Utility Performance Iperf Macros
  * @ingroup  ST67W6X_Utilities_Performance_Iperf
  * @{
  */

/** Macro to compare the argument with the input string */
#define IPERF_CMP_ARG(s) ((strncmp(argv[current_arg], (s), 2) == 0) && strlen(argv[current_arg]) == 2)

/** @} */

/* Private variables ---------------------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Iperf_Variables
  * @{
  */

#if (IPERF_ENABLE == 1)
/** Iperf usage string */
static const char *iperf_usage[] =
{
  "Usage: iperf [options]",
  "-c <server_addr>: run in client mode",
  "-s:               run in server mode",
  "-u:               UDP",
  "-p <port>:        specify port",
  "-l <length>:      set read/write buffer size",
  "-i <interval>:    seconds between bandwidth reports",
  "-t <time>:        time in seconds to run",
  "-b <bandwidth>:   bandwidth to send in Mbps",
  "-S <tos>:         TOS",
  "-n <MB>:          number of MB to send/recv",
  "-P <priority>:    traffic task priority",
#if (IPERF_DUAL_MODE == 1)
  "-d:               dual mode",
#endif /* IPERF_DUAL_MODE */
  "-a:               abort running iperf",
  NULL
};
#endif /* IPERF_ENABLE */

/** @} */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ST67W6X_Utilities_Performance_Iperf_Functions
  * @{
  */

#if (IPERF_ENABLE == 1)
/**
  * @brief  iperf shell function
  * @param  argc: number of arguments
  * @param  argv: pointer to the arguments
  * @retval ::SHELL_STATUS_OK on success
  * @retval ::SHELL_STATUS_UNKNOWN_ARGS if wrong arguments
  * @retval ::SHELL_STATUS_ERROR otherwise
  */
int32_t iperf_cmd(int32_t argc, char **argv);

/* Private Functions Definition ----------------------------------------------*/
int32_t iperf_cmd(int32_t argc, char **argv)
{
  int32_t current_arg = 1;
  int32_t o_c = 0;
  int32_t o_s = 0;
  int32_t o_u = 0;
  int32_t o_p = IPERF_DEFAULT_PORT;
  int32_t o_l = 0;
  int32_t o_i = 0;
  int32_t o_t = IPERF_DEFAULT_TIME;
  int32_t o_b = IPERF_DEFAULT_BW_LIMIT;
  int32_t o_S = 0;
  int32_t o_n = 0;
#if (IPERF_DUAL_MODE == 1)
  int32_t o_d = 0;
#endif /* IPERF_DUAL_MODE */
  int32_t o_P = IPERF_TRAFFIC_TASK_PRIORITY;
  uint32_t dst_addr = 0;
  uint8_t ip_addr[4] = {0};

  iperf_cfg_t cfg;

  /* Display usage when no argument is provided */
  if (argc == 1)
  {
    for (uint32_t i = 0; iperf_usage[i] != NULL; i++)
    {
      SHELL_PRINTF("%s\n", iperf_usage[i]);
    }
    return SHELL_STATUS_OK;
  }

  while (current_arg < argc)
  {
    /* Display usage when help option is provided */
    if (IPERF_CMP_ARG("-h"))
    {
      for (uint32_t i = 0; iperf_usage[i] != NULL; i++)
      {
        SHELL_PRINTF("%s\n", iperf_usage[i]);
      }
      return SHELL_STATUS_OK;
    }
    /* Abort running iperf */
    else if (IPERF_CMP_ARG("-a"))
    {
      iperf_stop();
      return SHELL_STATUS_OK;
    }
    /* Client mode with server address */
    else if (IPERF_CMP_ARG("-c"))
    {
      ++o_c;
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToIP(argv[current_arg], ip_addr);
      dst_addr = ATON_R(ip_addr);
    }
    /* Server mode */
    else if (IPERF_CMP_ARG("-s"))
    {
      ++o_s;
    }
    /* UDP mode. Default is TCP */
    else if (IPERF_CMP_ARG("-u"))
    {
      ++o_u;
    }
    /* Port number */
    else if (IPERF_CMP_ARG("-p"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_p);
    }
    /* Buffer size */
    else if (IPERF_CMP_ARG("-l"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_l);
    }
    /* Interval time to display current bandwidth */
    else if (IPERF_CMP_ARG("-i"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_i);
    }
    /* Duration of the execution. Client mode */
    else if (IPERF_CMP_ARG("-t"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_t);
    }
    /* Bandwidth limit. Client mode */
    else if (IPERF_CMP_ARG("-b"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_b);
    }
    /* TOS */
    else if (IPERF_CMP_ARG("-S"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_S);
    }
    /* Number of bytes to send/recv */
    else if (IPERF_CMP_ARG("-n"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_n);
    }
    /* Traffic task priority */
    else if (IPERF_CMP_ARG("-P"))
    {
      current_arg++;
      if (current_arg == argc)
      {
        return SHELL_STATUS_UNKNOWN_ARGS;
      }
      Parser_StrToInt(argv[current_arg], NULL, &o_P);
    }
#if (IPERF_DUAL_MODE == 1)
    /* Dual mode */
    else if (IPERF_CMP_ARG("-d"))
    {
      ++o_d;
    }
#endif /* IPERF_DUAL_MODE */
    else
    {
      return SHELL_STATUS_UNKNOWN_ARGS;
    }

    current_arg++;
  }

  memset(&cfg, 0, sizeof(cfg));
  cfg.type = IPERF_IP_TYPE_IPV4;

  if (!((o_c && !o_s) || (!o_c && o_s)))
  {
    SHELL_E("client/server required\n");
    return SHELL_STATUS_ERROR;
  }

  /* Client or Server mode */
  if (o_c)
  {
    cfg.destination_ip4 = dst_addr;
    cfg.flag |= IPERF_FLAG_CLIENT;
  }
  else
  {
    cfg.flag |= IPERF_FLAG_SERVER;
  }

  /* UDP or TCP mode */
  if (o_u)
  {
    cfg.flag |= IPERF_FLAG_UDP;
  }
  else
  {
    cfg.flag |= IPERF_FLAG_TCP;
  }

#if (IPERF_DUAL_MODE == 1)
  /* Dual mode */
  if (o_c && !o_u && o_d)
  {
    cfg.flag |= IPERF_FLAG_DUAL;
  }
#endif /* IPERF_DUAL_MODE */

  cfg.len_buf = o_l;
  cfg.sport = o_p;
  cfg.dport = o_p;
  cfg.interval = o_i;
  cfg.time = o_t;

  /* Minimum duration is interval */
  if (cfg.time < cfg.interval)
  {
    cfg.time = cfg.interval;
  }

  cfg.bw_lim = o_b;
  cfg.tos = o_S;
  /* Convert MB parameter to bytes */
  cfg.num_bytes = o_n * 1000 * 1000;

  /* No bandwidth limit */
  if (cfg.bw_lim <= 0 || !o_u)
  {
    cfg.bw_lim = IPERF_NO_BW_LIMIT;
  }

  cfg.traffic_task_priority = o_P;

  /* Start the iperf execution */
  iperf_start(&cfg);
  return SHELL_STATUS_OK;
}

SHELL_CMD_EXPORT_ALIAS(iperf_cmd, iperf,
                       iperf [ options ]. Iperf command line tool for network performance measurement. [ -h ] for help);

#endif /* IPERF_ENABLE */

/** @} */
