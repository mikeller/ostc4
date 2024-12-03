/**
  ******************************************************************************
  * @file    uartInternal.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    03-November-2024
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
#ifndef UARTINTERNAL_H
#define UARTINTERNAL_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f4xx_hal.h"


#define BUFFER_NODATA_LOW	(0x15)		/* The read function needs a signature which indicates that no data for processing is available.*/
#define BUFFER_NODATA_HIGH  (0xA5)


UART_HandleTypeDef huart6;

void MX_USART6_UART_Init(void);
void MX_USART6_DMA_Init(void);
void MX_USART6_UART_DeInit(void);
void GNSS_IO_init(void);

void UART6_StartDMA_Receiption(void);

void UART_clearRx6Buffer(void);
void UART6_SendCmdUbx(const uint8_t *cmd, uint8_t len);
void UART6_ReadData(void);
void UART6_WriteData(void);
void UART6_Gnss_ProcessData(uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* UARTINTERNAL_H */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
