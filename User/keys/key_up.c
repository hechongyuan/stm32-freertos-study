#include "key_up.h"
#include "buzzer_task.h"
#include "main.h"
#include "key.h"
#include "stm32f1xx_hal_def.h"
#include <string.h>
KeyHandle_Typedef hkeyup;
extern UART_HandleTypeDef huart1;
void keyupClickCallBack(void);

void keyup_init(void)
{
      hkeyup.ClickedCallBack=keyupClickCallBack;
      hkeyup.GPIO_Pin=Key_UP_Pin;
      hkeyup.GPIO_Port = Key_UP_GPIO_Port;

      key_init(&hkeyup);

}

void keyup_scan(void)
{
      key_scan(&hkeyup);
}

void keyupClickCallBack(void)
{

    Buzzer_beep();  

}