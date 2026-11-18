#ifndef __KEY_UP_H
#define __KEY_UP_H

#include "key.h"
#include "main.h"
#include "stm32f1xx_hal_def.h"
#include <string.h>


void keyup_init(void);
void keyup_scan(void);
void keyupClickCallBack(void);


#endif