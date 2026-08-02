/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PRESS_OUT_Pin GPIO_PIN_0
#define PRESS_OUT_GPIO_Port GPIOA
#define PRESS_SCK_Pin GPIO_PIN_1
#define PRESS_SCK_GPIO_Port GPIOA
#define KEY1_Pin GPIO_PIN_4
#define KEY1_GPIO_Port GPIOA
#define KEY2_Pin GPIO_PIN_5
#define KEY2_GPIO_Port GPIOA
#define KEY3_Pin GPIO_PIN_6
#define KEY3_GPIO_Port GPIOA
#define KEY4_Pin GPIO_PIN_7
#define KEY4_GPIO_Port GPIOA
#define US_TRIG_Pin GPIO_PIN_0
#define US_TRIG_GPIO_Port GPIOB
#define US_ECHO_Pin GPIO_PIN_1
#define US_ECHO_GPIO_Port GPIOB
#define US_ECHO_EXTI_IRQn EXTI1_IRQn
#define RELAY_CTRL_Pin GPIO_PIN_12
#define RELAY_CTRL_GPIO_Port GPIOB
#define BUZZER_CTRL_Pin GPIO_PIN_13
#define BUZZER_CTRL_GPIO_Port GPIOB
#define RUN_LED_Pin GPIO_PIN_14
#define RUN_LED_GPIO_Port GPIOB
#define FAULT_LED_Pin GPIO_PIN_15
#define FAULT_LED_GPIO_Port GPIOB
#define OLED_SCL_Pin GPIO_PIN_5
#define OLED_SCL_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_6
#define OLED_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
