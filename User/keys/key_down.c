#include "key_down.h"
#include "main.h"
#include "key.h"
#include "stm32f1xx_hal_def.h"
#include <string.h>
#include"buzzer_task.h"
KeyHandle_Typedef hkeydown;
extern UART_HandleTypeDef huart1;
void keydownClickCallBack(void);

void keydown_init(void)
{
      hkeydown.ClickedCallBack=keydownClickCallBack;
      hkeydown.GPIO_Pin=Key_DOWN_Pin;
      hkeydown.GPIO_Port = Key_DOWN_GPIO_Port;

      key_init(&hkeydown);

}

void keydown_scan(void)
{
      key_scan(&hkeydown);
}

void keydownClickCallBack(void)
{

    Buzzer_beep();

}