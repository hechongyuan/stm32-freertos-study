#ifndef __KEY_DOWN_H
#define __KEY_DOWN_H

#include "key.h"
#include "main.h"
#include "stm32f1xx_hal_def.h"
#include <string.h>


void keydown_init(void);
void keydown_scan(void);
void keydownClickCallBack(void);


#endif