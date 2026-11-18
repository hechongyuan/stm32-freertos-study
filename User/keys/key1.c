#include "key1.h"

KeyHandle_Typedef hkey1;
extern UART_HandleTypeDef huart1;
void key1ClickCallBack(void);

void key1_init(void)
{
      hkey1.ClickedCallBack=key1ClickCallBack;
      hkey1.GPIO_Pin=Key1_Pin;
      hkey1.GPIO_Port = Key1_GPIO_Port;

      key_init(&hkey1);

}

void key1_scan(void)
{
      key_scan(&hkey1);
}

void key1ClickCallBack(void)
{

    HAL_UART_Transmit(&huart1,(const uint8_t*)"key1 clicked.\n",strlen("key1 clicked.\n"),HAL_MAX_DELAY);  

}