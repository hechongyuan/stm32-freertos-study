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
#include"image1.h"
#include"image2.h"
#include"semphr.h"
void LcdBlackLight_Control(uint8_t on);
void Lcd_Reset(void);
void SendComand (uint8_t cmd);
void SendData (uint8_t* pData,uint16_t size);
static void SetRowRange(uint16_t start ,uint16_t end);
static void SetColumnRange(uint16_t start ,uint16_t end);
static void ShowImage(uint16_t *pImage,uint16_t size);
void ShowImage1(void);
void ShowImage2(void);

SemaphoreHandle_t xSemphrToDma;

void Lcd_Init(void)
{
      xSemphrToDma =xSemaphoreCreateBinary();
      LcdBlackLight_Control(1);
      Lcd_Reset();
      
}

void vLcdTask(void* pvParameter)
{
      uint8_t pDataToSend[10];
      SendComand(0x11);       //此命令为退出睡眠模式
      HAL_Delay(125);       //延迟125ms等待从睡眠模式苏醒
      SendComand(0x3a);       
      pDataToSend[0]=0x55;          
      SendData(pDataToSend, 1);     //发送命令0x3a和数据0x55以设置数据格式RGB565
      SendComand(0x29);       //开启屏幕显示
	for(;;)
	{
		ShowImage1();
		vTaskDelay(pdMS_TO_TICKS(200));
		ShowImage2();
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
      if(hspi->Instance==SPI1)
      {
            BaseType_t* pxHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(xSemphrToDma,pxHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xSemphrToDma);
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

//@作用 设置图像的行范围
//@参数 start——起始行号
//@参数 end——结束行号
static void SetRowRange(uint16_t start ,uint16_t end)
{
	SendComand(0x2b);
	uint8_t pDataToSend[4];
	pDataToSend[0]=(start>>8)&0xff;
	pDataToSend[1]=(start&0xff);
	pDataToSend[2]=(end>>8)&0xff;
	pDataToSend[3]=(end&0xff);
	SendData(pDataToSend, 4);
}

//@作用 设置图像的列范围
//@参数 start——起始列号
//@参数 end——结束列号
static void SetColumnRange(uint16_t start ,uint16_t end)
{
	SendComand(0x2a);
	uint8_t pDataToSend[4];
	pDataToSend[0]=(start>>8)&0xff;
	pDataToSend[1]=(start&0xff);
	pDataToSend[2]=(end>>8)&0xff;
	pDataToSend[3]=(end&0xff);
	SendData(pDataToSend, 4);
}

//@作用 显示图像
//@参数 pImage —— 显示图像的数据
//@参数 size —— 显示图像的像素的个数
static void ShowImage(uint16_t *pImage,uint16_t size)
{
	SendComand(0x2c);
	SendData((uint8_t*)pImage,size*2);
}

void ShowImage1(void)
{
	SetRowRange(80, 239);
	SetColumnRange(60, 179);
	ShowImage(image1, 120*160);
}
void ShowImage2(void)
{
	SetRowRange(80, 239);
	SetColumnRange(60, 179);
	ShowImage(image2, 120*160);
}


