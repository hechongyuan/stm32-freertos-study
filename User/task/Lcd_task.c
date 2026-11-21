#include "Lcd_task.h"
#include"main.h"
#include"FreeRTOS.h"
#include "portmacro.h"
#include "stm32f103xe.h"
#include"task.h"
#include "projdefs.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"
#include"semphr.h"
#include "lcd.h"
void LcdBlackLight_Control(uint8_t on);
void Lcd_Reset(void);
void SendComand (uint8_t cmd);
void SendData (uint8_t* pData,uint16_t size);

SemaphoreHandle_t xSemphrToDma;

void Lcd_Init(void)
{
      xSemphrToDma =xSemaphoreCreateBinary();
      /* static：保证 LCD 库如果保存了这个结构体的指针，函数返回后内容仍然有效 */
      static LCD_InitTypeDef LCD_InitStruct;
      LCD_InitStruct.back_light_control_callback = LcdBlackLight_Control;
      LCD_InitStruct.reset_callback= Lcd_Reset;
      LCD_InitStruct.send_command_callback= SendComand;
      LCD_InitStruct.send_data_callback= SendData;

      lcd_init(&LCD_InitStruct);
}

void vLcdTask(void* pvParameter)
{
      /* 必须在调度器启动之后才初始化 LCD：
       * lcd_init() 会通过 SendData() 用 DMA 发送数据，
       * 发送完成中断里调用 portYIELD_FROM_ISR() 才安全。 */
      Lcd_Init();

      lcd_clear();
      //绘制粉色标题背景
      lcd_set_cursor(260, 8);
      lcd_set_brush(0xff7bff);
      lcd_fill_rect(60, 15);
      //显示标题
      lcd_set_cursor(262,21);
      lcd_set_brush(0x000000);
      lcd_print_string("Run Mode");
      //绘制灰色面版
      lcd_set_cursor(260, 23);
      lcd_set_brush(0xcccccc);
      lcd_fill_rect(60, 35);
      //显示文字
      lcd_set_cursor(265,43);
      lcd_set_brush(0x000000);
      lcd_print_string("One-Shot");
	for(;;)
	{
		
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
      if(hspi->Instance==SPI1)
      {
            /* 注意：这里要传变量的地址，并且 portYIELD_FROM_ISR 要传
             * “是否需要切换任务”的标志，而不是信号量句柄。
             * 原来的写法：
             *   BaseType_t* p = pdFALSE;                 // 空指针
             *   xSemaphoreGiveFromISR(xSemphrToDma, p);  // 传了 NULL
             *   portYIELD_FROM_ISR(xSemphrToDma);        // 句柄非 0，永远 yield
             * 在调度器启动前这样 yield 会直接 HardFault。 */
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xSemphrToDma, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
}

//@作用 将lcd的背光点亮
//@参数on 如果on=0则熄灭背光，否则就点亮背光
void LcdBlackLight_Control(uint8_t on)
{
      if(on==0)
      {
            HAL_GPIO_WritePin(Lcd_BL_GPIO_Port, Lcd_BL_Pin, GPIO_PIN_RESET);
      }
      else
      {
            HAL_GPIO_WritePin(Lcd_BL_GPIO_Port, Lcd_BL_Pin, GPIO_PIN_SET);
      }
}

//@作用 重启lcd屏幕
void Lcd_Reset(void)
{
      HAL_GPIO_WritePin(Lcd_RST_GPIO_Port, Lcd_RST_Pin, GPIO_PIN_RESET);
      HAL_Delay(1);
      HAL_GPIO_WritePin(Lcd_RST_GPIO_Port, Lcd_RST_Pin, GPIO_PIN_SET);
      HAL_Delay(5);
}

//@作用 发送命令
//@参数cmd 命令的数据
void SendComand (uint8_t cmd)
{
      extern SPI_HandleTypeDef hspi1;
      HAL_GPIO_WritePin(Lcd_RS_GPIO_Port, Lcd_RS_Pin, GPIO_PIN_RESET);		//将RS置零即发送命令
      HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_RESET);
      HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
      HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_SET);
}

//@作用 向lcd发送数据
//@参数pData 发送的数据的数组
//@参数size  发送的数据的个数
void SendData (uint8_t* pData,uint16_t size)
{
      extern SPI_HandleTypeDef hspi1;
      HAL_GPIO_WritePin(Lcd_RS_GPIO_Port, Lcd_RS_Pin, GPIO_PIN_SET);		//将RS置一即发送数据
      HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_RESET);
      HAL_SPI_Transmit_DMA(&hspi1, pData, size);
      xSemaphoreTake(xSemphrToDma, portMAX_DELAY);
      HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_SET); 
}







