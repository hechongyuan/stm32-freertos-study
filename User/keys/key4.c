#include "key4.h"
#include "key.h"
#include "main.h"
#include "stm32f1xx_hal_def.h"
#include <string.h>
#include"buzzer_task.h"
KeyHandle_Typedef hkey4;
extern UART_HandleTypeDef huart1;
void key4ClickCallBack(void);

void key4_init(void)
{
      hkey4.ClickedCallBack=key4ClickCallBack;
      hkey4.GPIO_Pin=Key4_Pin;
      hkey4.GPIO_Port = Key4_GPIO_Port;

      key_init(&hkey4);

}

void key4_scan(void)
{
      key_scan(&hkey4);
}

void key4ClickCallBack(void)
{

   Buzzer_beep();  

}