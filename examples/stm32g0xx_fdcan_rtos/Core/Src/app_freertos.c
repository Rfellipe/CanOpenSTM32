/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "CO_app_STM32.h"
#include "fdcan.h"
#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  CO_CiA402_feedback_t feedback;
  int8_t mode;
} AppCia402Drive;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static AppCia402Drive cia402Drive = {
  .feedback = {
    .targetReached = true
  },
  .mode = (int8_t)CO_CiA402_MODE_NONE
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for canopen */
osThreadId_t canopenHandle;
const osThreadAttr_t canopen_attributes = {
  .name = "canopen",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static bool_t cia402_set_bool(void *object, bool_t value);
static bool_t cia402_set_configured_bool(void *object, bool_t value, const CO_CiA402_motionConfig_t *config);
static bool_t cia402_set_mode(void *object, int8_t mode);
static bool_t cia402_fault_reset(void *object);
static bool_t cia402_run_profile_position(void *object, int32_t targetPosition, bool_t relative, bool_t override,
                                          bool_t changeOnSetPoint, const CO_CiA402_motionConfig_t *config);
static bool_t cia402_run_profile_velocity(void *object, int32_t targetVelocity,
                                          const CO_CiA402_motionConfig_t *config);
static bool_t cia402_run_profile_torque(void *object, int16_t targetTorque,
                                        const CO_CiA402_motionConfig_t *config);
static bool_t cia402_run_homing(void *object, int8_t homingMethod, const CO_CiA402_motionConfig_t *config);
static bool_t cia402_run_csp(void *object, int32_t targetPosition, const CO_CiA402_motionConfig_t *config);
static bool_t cia402_run_csv(void *object, int32_t targetVelocity, const CO_CiA402_motionConfig_t *config);
static bool_t cia402_run_cst(void *object, int16_t targetTorque, const CO_CiA402_motionConfig_t *config);
static bool_t cia402_get_feedback(void *object, CO_CiA402_feedback_t *feedback);

static const CO_CiA402_hwInterface_t cia402Hw = {
  .setEnableVoltage = cia402_set_bool,
  .setSwitchOn = cia402_set_bool,
  .setOperationEnabled = cia402_set_bool,
  .setQuickStop = cia402_set_configured_bool,
  .setHalt = cia402_set_configured_bool,
  .setMode = cia402_set_mode,
  .faultReset = cia402_fault_reset,
  .runProfilePosition = cia402_run_profile_position,
  .runProfileVelocity = cia402_run_profile_velocity,
  .runProfileTorque = cia402_run_profile_torque,
  .runHoming = cia402_run_homing,
  .runCyclicSynchronousPosition = cia402_run_csp,
  .runCyclicSynchronousVelocity = cia402_run_csv,
  .runCyclicSynchronousTorque = cia402_run_cst,
  .getFeedback = cia402_get_feedback
};

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void canopen_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of canopen */
  canopenHandle = osThreadNew(canopen_task, NULL, &canopen_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_canopen_task */
/**
* @brief Function implementing the canopen thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_canopen_task */
void canopen_task(void *argument)
{
  /* USER CODE BEGIN canopen_task */
  CANopenNodeSTM32 canOpenNodeSTM32;
  canOpenNodeSTM32.CANHandle = &hfdcan1;
  canOpenNodeSTM32.HWInitFunction = MX_FDCAN1_Init;
  canOpenNodeSTM32.timerHandle = &htim17;
  canOpenNodeSTM32.desiredNodeID = 21;
  canOpenNodeSTM32.baudrate = 125;
  canOpenNodeSTM32.cia402HwObject = &cia402Drive;
  canOpenNodeSTM32.cia402Hw = &cia402Hw;
  canopen_app_init(&canOpenNodeSTM32);
  /* Infinite loop */
  for(;;)
  {
	  //Reflect CANopenStatus on LEDs
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, !canOpenNodeSTM32.outStatusLEDGreen);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, !canOpenNodeSTM32.outStatusLEDRed);
    canopen_app_process();
    // Sleep for 1ms, you can decrease it if required, in the canopen_app_process we will double check to make sure 1ms passed
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  /* USER CODE END canopen_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static bool_t cia402_set_bool(void *object, bool_t value)
{
  (void)object;
  (void)value;
  return true;
}

static bool_t cia402_set_configured_bool(void *object, bool_t value, const CO_CiA402_motionConfig_t *config)
{
  (void)object;
  (void)value;
  (void)config;
  return true;
}

static bool_t cia402_set_mode(void *object, int8_t mode)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  drive->mode = mode;
  return true;
}

static bool_t cia402_fault_reset(void *object)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  drive->feedback.faultActive = false;
  drive->feedback.faultCode = CO_CIA402_ERR_NONE;
  return true;
}

static bool_t cia402_run_profile_position(void *object, int32_t targetPosition, bool_t relative, bool_t override,
                                          bool_t changeOnSetPoint, const CO_CiA402_motionConfig_t *config)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  (void)relative;
  (void)override;
  (void)changeOnSetPoint;
  (void)config;
  drive->feedback.positionActualValue = targetPosition;
  drive->feedback.targetReached = true;
  drive->feedback.setPointAcknowledged = true;
  return true;
}

static bool_t cia402_run_profile_velocity(void *object, int32_t targetVelocity,
                                          const CO_CiA402_motionConfig_t *config)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  (void)config;
  drive->feedback.velocityActualValue = targetVelocity;
  drive->feedback.targetReached = true;
  return true;
}

static bool_t cia402_run_profile_torque(void *object, int16_t targetTorque,
                                        const CO_CiA402_motionConfig_t *config)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  (void)config;
  drive->feedback.torqueActualValue = targetTorque;
  drive->feedback.targetReached = true;
  return true;
}

static bool_t cia402_run_homing(void *object, int8_t homingMethod, const CO_CiA402_motionConfig_t *config)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  (void)homingMethod;
  (void)config;
  drive->feedback.homingAttained = true;
  drive->feedback.homingCompleted = true;
  drive->feedback.targetReached = true;
  return true;
}

static bool_t cia402_run_csp(void *object, int32_t targetPosition, const CO_CiA402_motionConfig_t *config)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  (void)config;
  drive->feedback.positionActualValue = targetPosition;
  return true;
}

static bool_t cia402_run_csv(void *object, int32_t targetVelocity, const CO_CiA402_motionConfig_t *config)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  (void)config;
  drive->feedback.velocityActualValue = targetVelocity;
  return true;
}

static bool_t cia402_run_cst(void *object, int16_t targetTorque, const CO_CiA402_motionConfig_t *config)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  (void)config;
  drive->feedback.torqueActualValue = targetTorque;
  return true;
}

static bool_t cia402_get_feedback(void *object, CO_CiA402_feedback_t *feedback)
{
  AppCia402Drive *drive = (AppCia402Drive *)object;
  *feedback = drive->feedback;
  return true;
}

/* USER CODE END Application */
