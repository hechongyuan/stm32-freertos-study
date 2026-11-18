#include "key2.h"
#include "main.h"

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

    HAL_UART_Transmit(&huart1,(const uint8_t*)"key2 clicked.\n",strlen("key2 clicked.\n"),HAL_MAX_DELAY);  

}