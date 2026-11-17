#ifndef __KEY_H
#define __KEY_H

#include "main.h"
#include "stm32f103xe.h"
#include "stm32f1xx_hal_gpio.h"
typedef struct 
{
      GPIO_TypeDef* GPIO_Port ;
      uint16_t GPIO_Pin;

      GPIO_PinState previous;
      GPIO_PinState current;

      void(*ClickedCallBack)(void);

}KeyHandle_Typedef;



void key_init(KeyHandle_Typedef *handle);
void key_scan(KeyHandle_Typedef *handle );





#endif
