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

UART_HandleTypeDef huart6;

void MX_USART6_UART_Init(void);
void MX_USART6_DMA_Init(void);
void MX_USART6_UART_DeInit(void);
void GNSS_IO_init(void);

void UART6_HandleUART();

#ifdef __cplusplus
}
#endif

#endif /* UARTINTERNAL_H */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
