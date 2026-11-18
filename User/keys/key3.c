#include "key3.h"
#include "main.h"

KeyHandle_Typedef hkey3;
extern UART_HandleTypeDef huart1;
void key3ClickCallBack(void);

void key3_init(void)
{
      hkey3.ClickedCallBack=key3ClickCallBack;
      hkey3.GPIO_Pin=Key3_Pin;
      hkey3.GPIO_Port = Key3_GPIO_Port;

      key_init(&hkey3);

}

void key3_scan(void)
{
      key_scan(&hkey3);
}

void key3ClickCallBack(void)
{

    HAL_UART_Transmit(&huart1,(const uint8_t*)"key3 clicked.\n",strlen("key3 clicked.\n"),HAL_MAX_DELAY);  

}