#include"FreeRTOS.h"
#include "key.h"
#include "key2.h"
#include "key3.h"
#include "key4.h"
#include "projdefs.h"
#include"task.h"
#include"key1.h"
#include"key_down.h"
#include"key_up.h"
extern KeyHandle_Typedef hkey1;
extern KeyHandle_Typedef hkey2;
extern KeyHandle_Typedef hkey3;
extern KeyHandle_Typedef hkey4;
extern KeyHandle_Typedef hkeyup;
extern KeyHandle_Typedef hkeydown;
void keytask(void*pvParameter)
{
      key1_init();
      key2_init();
      key3_init();
      key4_init();
      keydown_init();
      keyup_init();
      for(;;)
      {
            key1_scan();
            key2_scan();
            key3_scan();
            key4_scan();
            keyup_scan();
            keydown_scan();
            vTaskDelay(pdMS_TO_TICKS(10));
      }

}