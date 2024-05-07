/**
  ******************************************************************************
  * @file    uart.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    27-March-2014
  * @brief   button control
  *           
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef UART_H
#define UART_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx_hal.h"


#define BUFFER_NODATA			(7u)		/* The read function needs a byte which indicated that no data for processing is available.*/
											/* This byte shall never appear in a normal data steam */

void MX_USART1_UART_Init(void);
void MX_USART1_UART_DeInit(void);
void MX_USART1_DMA_Init(void);
uint8_t UART_ButtonAdjust(uint8_t *array);
void UART_StartDMA_Receiption(void);
#ifdef ENABLE_CO2_SUPPORT
void UART_HandleCO2Data(void);
void DigitalCO2_SendCmd(uint8_t CO2Cmd, uint8_t *cmdString, uint8_t *cmdLength);
#endif
#ifdef ENABLE_SENTINEL_MODE
void UART_HandleSentinelData(void);
#endif
uint8_t UART_isCO2Connected();
uint8_t UART_isSentinelConnected();
void UART_setTargetChannel(uint8_t channel);
void  UART_MUX_SelectAddress(uint8_t muxAddress);
void UART_SendCmdString(uint8_t *cmdString);
void UART_ReadData(uint8_t sensorType);
void UART_FlushRxBuffer(void);
void UART_ChangeBaudrate(uint32_t newBaudrate);
uint8_t UART_isComActive(uint8_t sensorId);

void StringToInt(char *pstr, uint32_t *puInt32);
void StringToUInt64(char *pstr, uint64_t *puint64);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
