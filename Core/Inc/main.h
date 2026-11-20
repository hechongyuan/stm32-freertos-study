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
#define Key_UP_Pin GPIO_PIN_13
#define Key_UP_GPIO_Port GPIOC
#define Key_DOWN_Pin GPIO_PIN_14
#define Key_DOWN_GPIO_Port GPIOC
#define Key3_Pin GPIO_PIN_15
#define Key3_GPIO_Port GPIOC
#define Key4_Pin GPIO_PIN_0
#define Key4_GPIO_Port GPIOC
#define LED3_Pin GPIO_PIN_1
#define LED3_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_3
#define LED1_GPIO_Port GPIOC
#define Buzzer_Pin GPIO_PIN_8
#define Buzzer_GPIO_Port GPIOC
#define Key1_Pin GPIO_PIN_9
#define Key1_GPIO_Port GPIOC
#define Key2_Pin GPIO_PIN_8
#define Key2_GPIO_Port GPIOA
#define Lcd_RS_Pin GPIO_PIN_12
#define Lcd_RS_GPIO_Port GPIOC
#define Lcd_NSS_Pin GPIO_PIN_2
#define Lcd_NSS_GPIO_Port GPIOD
#define Lcd_SCK_Pin GPIO_PIN_3
#define Lcd_SCK_GPIO_Port GPIOB
#define Lcd_RST_Pin GPIO_PIN_4
#define Lcd_RST_GPIO_Port GPIOB
#define Lcd_MOSI_Pin GPIO_PIN_5
#define Lcd_MOSI_GPIO_Port GPIOB
#define Lcd_BL_Pin GPIO_PIN_6
#define Lcd_BL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
