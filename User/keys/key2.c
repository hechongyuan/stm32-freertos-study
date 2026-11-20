#include "key2.h"
#include "key.h"
#include "main.h"
#include "stm32f1xx_hal_def.h"
#include <string.h>
#include"buzzer_task.h"
KeyHandle_Typedef hkey2;
extern UART_HandleTypeDef huart1;
void key2ClickCallBack(void);

void key2_init(void)
{
      hkey2.ClickedCallBack=key2ClickCallBack;
      hkey2.GPIO_Pin=Key2_Pin;
      hkey2.GPIO_Port = Key2_GPIO_Port;

      key_init(&hkey2);

}

void key2_scan(void)
{
      key_scan(&hkey2);
}

void key2ClickCallBack(void)
{

    Buzzer_beep();  

}