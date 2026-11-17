#include "key.h"
#include "stm32f1xx_hal_gpio.h"
#include"string.h"


extern UART_HandleTypeDef huart1;
GPIO_PinState previous ;
GPIO_PinState current ;
void key_init(KeyHandle_Typedef *handle)
{
     handle-> previous = GPIO_PIN_SET;
     handle-> current = GPIO_PIN_SET;
}


void key_scan(KeyHandle_Typedef *handle )
{
      handle->current = HAL_GPIO_ReadPin(handle->GPIO_Port, handle->GPIO_Pin);
            
            if((handle->current==GPIO_PIN_SET)&&(handle->previous==GPIO_PIN_RESET))
            {
                  handle->ClickedCallBack();
            }
            handle->previous = handle-> current;
}