#include "key_down.h"
#include "main.h"

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

    HAL_UART_Transmit(&huart1,(const uint8_t*)"keydown clicked.\n",strlen("keydown clicked.\n"),HAL_MAX_DELAY);  

}