#include"key.h"
#include "main.h"
#include "projdefs.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include"FreeRTOS.h"
#include"task.h"
#include "stm32f1xx_hal_uart.h"
#include <string.h>



KeyHandle_Typedef hkey1;
static void Key1ClickCallBack (void)
{
      extern UART_HandleTypeDef huart1;
      HAL_UART_Transmit(&huart1, (const uint8_t*)"key1 clicked.\n", strlen("key1 clicked.\n"), HAL_MAX_DELAY);
}

void key1task(void*pvParameter)

{
      hkey1.GPIO_Pin = Key1_Pin;
      hkey1.GPIO_Port = Key1_GPIO_Port;
      hkey1.ClickedCallBack=Key1ClickCallBack;
      key_init(&hkey1);
      for(;;)
      {
            key_scan(&hkey1);
            vTaskDelay(pdMS_TO_TICKS(10));
      }
}