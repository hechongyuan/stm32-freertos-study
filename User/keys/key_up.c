#include "key_up.h"
#include "main.h"

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

    HAL_UART_Transmit(&huart1,(const uint8_t*)"keyup clicked.\n",strlen("keyup clicked.\n"),HAL_MAX_DELAY);  

}