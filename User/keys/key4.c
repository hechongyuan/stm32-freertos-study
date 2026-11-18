#include "key4.h"
#include "main.h"

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

    HAL_UART_Transmit(&huart1,(const uint8_t*)"key4 clicked.\n",strlen("key4 clicked.\n"),HAL_MAX_DELAY);  

}