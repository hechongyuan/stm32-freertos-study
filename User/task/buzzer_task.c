#include"buzzer_task.h"
#include"FreeRTOS.h"
#include "portmacro.h"
#include "projdefs.h"
#include"semphr.h"
#include"main.h"
#include "stm32f1xx_hal_gpio.h"
static SemaphoreHandle_t xBuzzerSemphr;
void Buzzer_Init(void)
{
     xBuzzerSemphr= xSemaphoreCreateBinary();

}
void vBuzzerTask(void*pvParameter)
{
      Buzzer_Init();
      for(;;)
      {
            xSemaphoreTake(xBuzzerSemphr,portMAX_DELAY);
            HAL_GPIO_WritePin(Buzzer_GPIO_Port , Buzzer_Pin, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
      }
}
void Buzzer_beep(void)
{
      xSemaphoreGive(xBuzzerSemphr);
}