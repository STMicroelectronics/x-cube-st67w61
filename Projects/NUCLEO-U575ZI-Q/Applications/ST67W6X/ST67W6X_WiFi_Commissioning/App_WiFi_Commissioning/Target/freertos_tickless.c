/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    freertos_tickless.c
  * @author  GPM Application Team
  * @brief   Management of timers and ticks
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "freertos_tickless.h"
#include "main.h"
#include "app_config.h"
#include "utilities_conf.h"
#if (LOW_POWER_MODE != LOW_POWER_DISABLE)
#include "stm32_lpm.h"
#endif /* LOW_POWER_MODE */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Global variables ----------------------------------------------------------*/
/* USER CODE BEGIN GV */

/* USER CODE END GV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private defines -----------------------------------------------------------*/
#define LPTIM_IDLE                              LPTIM1
#define LPTIM_IDLE_IRQn                         LPTIM1_IRQn

/* USER CODE BEGIN TICK_PD */
#define configTICK_RATE_HZ_1MS                  1000

#ifndef configSYSTICK_CLOCK_HZ
/** Override the default SysTick clock rate */
#define configSYSTICK_CLOCK_HZ                  ( configCPU_CLOCK_HZ )

/** Ensure the SysTick is clocked at the same frequency as the core. */
#define portNVIC_SYSTICK_CLK_BIT                ( 1UL << 2UL )

#else
/** The way the SysTick is clocked is not modified in case it is not the same as the core. */
#define portNVIC_SYSTICK_CLK_BIT                ( 0 )

#endif /* configSYSTICK_CLOCK_HZ */

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG               ( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG               ( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG      ( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSTICK_INT_BIT                ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT             ( 1UL << 0UL )

/* USER CODE END TICK_PD */

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if (LOW_POWER_MODE != LOW_POWER_DISABLE)
extern LPTIM_HandleTypeDef hlptim1;
static volatile uint16_t old_32k = 0;
static volatile uint16_t new_32k = 0;
static uint32_t ulTimerCountsForOneTick;
#endif /* LOW_POWER_MODE */

static uint32_t DisableSuppressTicksAndSleepBm;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
#if (LOW_POWER_MODE != LOW_POWER_DISABLE)
/**
  * @brief  Start a timer for ms_to_sleep ms
  * @retval none
  */
static void LpTimer_StartIT(uint32_t ms_to_sleep);

/**
  * @brief  Get_counter_value
  * @retval the counter
  */
static uint32_t LpTimer_GetCounter(void);

/**
  * @brief  Stop a timer interrupt
  * @retval none
  */
static void LpTimer_StopIT(void);

/**
  * @brief  Get elapsed time
  * @param  sysTickRem in SysTick
  * @retval value in milliseconds
  */
static uint32_t LpTimer_GetElapsedTime(uint32_t *sysTickRem);

/**
  * @brief  Convert time in ms to RTOS ticks
  * @param  value in milliseconds
  * @retval value in RTOS tick
  */
static uint32_t app_freertos_ms_to_tick(uint32_t ms);

/**
  * @brief  Convert time in RTOS ticks to ms
  * @param  value in RTOS tick
  * @retval value in milliseconds
  */
static uint32_t app_freertos_tick_to_ms(uint32_t tick);
#endif /* LOW_POWER_MODE */

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void DisableSuppressTicksAndSleep(uint32_t bitmask)
{
  /* USER CODE BEGIN DisableSuppressTicksAndSleep_1 */

  /* USER CODE END DisableSuppressTicksAndSleep_1 */
  UTILS_ENTER_CRITICAL_SECTION();

  DisableSuppressTicksAndSleepBm |= bitmask;

  UTILS_EXIT_CRITICAL_SECTION();

  /* USER CODE BEGIN DisableSuppressTicksAndSleep_End */

  /* USER CODE END DisableSuppressTicksAndSleep_End */
}

void EnableSuppressTicksAndSleep(uint32_t bitmask)
{
  /* USER CODE BEGIN EnableSuppressTicksAndSleep_1 */

  /* USER CODE END EnableSuppressTicksAndSleep_1 */
  UTILS_ENTER_CRITICAL_SECTION();

  DisableSuppressTicksAndSleepBm  &= (~bitmask);

  UTILS_EXIT_CRITICAL_SECTION();

  /* USER CODE BEGIN EnableSuppressTicksAndSleep_End */

  /* USER CODE END EnableSuppressTicksAndSleep_End */
}

#if (LOW_POWER_MODE != LOW_POWER_DISABLE)
void vPortSetupTimerInterrupt(void);
void vPortSetupTimerInterrupt(void)
{
  /* USER CODE BEGIN vPortSetupTimerInterrupt_1 */

  /* USER CODE END vPortSetupTimerInterrupt_1 */
  /* Enable autonomous mode for LPTIM1,
     this is mandatory to count in stop mode */
  __HAL_RCC_LPTIM1_CLKAM_ENABLE();

  /* Initialize DisableSuppressTicksAndSleepBm to 0 */
  DisableSuppressTicksAndSleepBm = 0 ;
  /* Calculate the constants required to configure the tick interrupt. */
  ulTimerCountsForOneTick = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ);

  /* Stop and clear the SysTick. */
  portNVIC_SYSTICK_CTRL_REG = 0UL;
  portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

  /* Configure SysTick to interrupt at the requested rate. */
  portNVIC_SYSTICK_LOAD_REG = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
  portNVIC_SYSTICK_CTRL_REG = (portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT);

  /* Start LPTIM counter */
  LL_LPTIM_Enable(LPTIM_IDLE);
  LL_LPTIM_SetAutoReload(LPTIM_IDLE, 0xFFFF);
  while (LL_LPTIM_IsActiveFlag_ARROK(LPTIM_IDLE) != 1);
  LL_LPTIM_StartCounter(LPTIM_IDLE, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

  /* USER CODE BEGIN vPortSetupTimerInterrupt_End */

  /* USER CODE END vPortSetupTimerInterrupt_End */
}
#endif /* LOW_POWER_MODE */

void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
#if (LOW_POWER_MODE != LOW_POWER_DISABLE)
  /* USER CODE BEGIN vPortSuppressTicksAndSleep_1 */

  /* USER CODE END vPortSuppressTicksAndSleep_1 */
  /* If low power is not used, do not stop the SysTick and continue execution */

  if (DisableSuppressTicksAndSleepBm == 0)
  {
    /* Although this is not documented as such, when xExpectedIdleTime = 0xFFFFFFFF = (~0),
       it likely means the system may enter low power for ever ( from a FreeRTOS point of view ).
       Otherwise, for a FreeRTOS tick set to 1ms, that would mean it is requested to wakeup in 8 years from now.
       When the system may enter low power mode for ever, FreeRTOS is not really interested to maintain a
       systick count and when the system exits from low power mode, there is no need to update the count with
       the time spent in low power mode */
    uint32_t ulCompleteTickPeriods;
    int32_t curSysTickValue = portNVIC_SYSTICK_CURRENT_VALUE_REG;

    /* Stop the SysTick  to avoid the interrupt to occur while in the critical section.
       Otherwise, this will prevent the device to enter low power mode
       At this time, an update of the systick will not be considered */
    portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
       method as that will mask interrupts that should exit sleep mode. */
    __disable_irq();
    __DSB();
    __ISB();

    /* If a context switch is pending or a task is waiting for the scheduler
       to be unsuspended then abandon the low power entry. */
    if (eTaskConfirmSleepModeStatus() == eAbortSleep)
    {
      /* Restart SysTick. */
      portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

      /* Re-enable interrupts - see comments above __disable_interrupt() call above. */
      __enable_irq();
    }
    else
    {
      if (xExpectedIdleTime != (~0))
      {
        /* Remove one tick to wake up before the event occurs */
        xExpectedIdleTime -= 1;
        /* Start the low power timer */
        LpTimer_StartIT(app_freertos_tick_to_ms(xExpectedIdleTime));
      }

      /* Enter low power mode */
      UTIL_LPM_EnterLowPower();

      if (xExpectedIdleTime != (~0))
      {
        /* Get the number of FreeRTOS ticks that has been suppressed
           In the current implementation, this shall be kept in critical section
           so that the timer returns the correct elapsed time */
        uint32_t sysTickRem = 0;

        ulCompleteTickPeriods = app_freertos_ms_to_tick(LpTimer_GetElapsedTime(&sysTickRem));

        curSysTickValue -= sysTickRem;

        if (curSysTickValue < 0)
        {
          ulCompleteTickPeriods++;
          curSysTickValue += ulTimerCountsForOneTick;
        }

        vTaskStepTick(ulCompleteTickPeriods);
        LpTimer_StopIT();
      }

      /* Restart SysTick */
      /* The 3 raws below are to force the systick counter to portNVIC_SYSTICK_CURRENT_VALUE_REG = curSysTickValue; */
      portNVIC_SYSTICK_LOAD_REG = curSysTickValue;
      portNVIC_SYSTICK_CURRENT_VALUE_REG = 0;
      portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
      /* Update the load reg back to ulTimerCountsForOneTick */
      portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;

      /* Exit with interrupts enabled. */
      __enable_irq();
    }
  }
  /* USER CODE BEGIN vPortSuppressTicksAndSleep_End */

  /* USER CODE END vPortSuppressTicksAndSleep_End */
#endif /* LOW_POWER_MODE */
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/* Private Functions Definition ----------------------------------------------*/
#if (LOW_POWER_MODE != LOW_POWER_DISABLE)
/* USER CODE BEGIN LP_PrFD_1 */

/* USER CODE END LP_PrFD_1 */

static void LpTimer_StartIT(uint32_t ms_to_sleep)
{
  uint16_t timeout = (uint16_t)((ms_to_sleep << 15) / 1000);

  old_32k = LpTimer_GetCounter();

  timeout += old_32k; /* Intentional wrap */
  /* Account for 2 32k period delay of LL_LPTIM_OC_SetCompareCH1 +1 for */
  timeout -= 3; /* Intentional wrap */

  if (timeout > UINT16_MAX - 10)
  {
    timeout = UINT16_MAX - 10;
  }

  LL_LPTIM_ClearFlag_CMP1OK(LPTIM_IDLE);
  LL_LPTIM_OC_SetCompareCH1(LPTIM_IDLE, timeout);
  /* Wait Compare Value is set */
  while (LL_LPTIM_IsActiveFlag_CMP1OK(LPTIM_IDLE) != 1);

  LL_LPTIM_ClearFlag_DIEROK(LPTIM_IDLE);
  LL_LPTIM_EnableIT_CC1(LPTIM_IDLE);
  /* Wait for the completion of the write operation to the LPTIM_DIER register */
  while (LL_LPTIM_IsActiveFlag_DIEROK(LPTIM_IDLE) != 1);

  HAL_NVIC_EnableIRQ(LPTIM_IDLE_IRQn);
}

static void LpTimer_StopIT(void)
{
  LL_LPTIM_ClearFlag_DIEROK(LPTIM_IDLE);
  /* Disable CC1 IT */
  LL_LPTIM_DisableIT_CC1(LPTIM_IDLE);
  /* Wait Interrupt is reset */
  while (LL_LPTIM_IsActiveFlag_DIEROK(LPTIM_IDLE) != 1)

    /* Clear Active Flag if set */
    if (LL_LPTIM_IsActiveFlag_CC1(LPTIM_IDLE) == 1)
    {
      LL_LPTIM_ClearFLAG_CC1(LPTIM_IDLE);
    }
  /* Wait flag is cleared */
  /* Clear  NVIC pending LPTIM IRQ */
  HAL_NVIC_ClearPendingIRQ(LPTIM_IDLE_IRQn);

  if (HAL_NVIC_GetPendingIRQ(LPTIM_IDLE_IRQn))
  {
    while (1);
  }
}

static uint32_t LpTimer_GetElapsedTime(uint32_t *sysTickRem)
{
  new_32k = LpTimer_GetCounter();

  uint32_t elapsed32k = (uint16_t)(new_32k - old_32k);
  uint32_t elapsed_ms = (elapsed32k * 1000) >> 15;
  uint32_t remainder = elapsed32k * 1000 - (elapsed_ms << 15);
  remainder = (remainder * ulTimerCountsForOneTick) >> 15;

  *sysTickRem = remainder;

  return elapsed_ms;
}

static uint32_t LpTimer_GetCounter(void)
{
  uint32_t cnt = 0;
  /* Read until 2 consecutive identical read */
  do
  {
    cnt =  LL_LPTIM_GetCounter(LPTIM_IDLE);
  } while (cnt != LL_LPTIM_GetCounter(LPTIM_IDLE));

  return cnt;
}

void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
  if (hlptim == &hlptim1)
  {
    /* Nothing to do */
  }
}

static uint32_t app_freertos_ms_to_tick(uint32_t ms)
{
  uint32_t tick = ms;
  if (configTICK_RATE_HZ != configTICK_RATE_HZ_1MS)
  {
    tick = (uint32_t)((((uint64_t)(ms)) * configTICK_RATE_HZ) / configTICK_RATE_HZ_1MS);
  }
  return tick;
}

static uint32_t app_freertos_tick_to_ms(uint32_t tick)
{
  uint32_t ms = tick;
  if (configTICK_RATE_HZ != configTICK_RATE_HZ_1MS)
  {
    ms = (uint32_t)((((uint64_t)(tick)) * configTICK_RATE_HZ_1MS) / configTICK_RATE_HZ);
  }
  return ms;
}
/* USER CODE BEGIN LP_PrFD_End */

/* USER CODE END LP_PrFD_End */

#endif /* LOW_POWER_MODE */

/* USER CODE BEGIN PFD */

/* USER CODE END PFD */
