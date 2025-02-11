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


#define BUFFER_NODATA_LOW	(0x15)		/* The read function needs a signiture which indicates that no data for processing is available.*/
#define BUFFER_NODATA_HIGH  (0xA5)

#define TX_BUF_SIZE				(80u)		/* max length for commands */
#define CHUNK_SIZE				(80u)		/* the DMA will handle chunk size transfers */
#define CHUNKS_PER_BUFFER		(3u)

 typedef struct
 {
	 UART_HandleTypeDef* pHandle;						/* Pointer to UART handle structure */

	 uint8_t* pRxBuffer;								/* Pointer to receive buffer */
	 uint8_t* pTxBuffer;								/* Pointer to transmit buffer */
	 uint8_t* pTxQue;									/* Pointer to transmit que */
	 uint8_t rxWriteIndex	;							/* Index of the data item which is analyzed */
	 uint8_t rxReadIndex;								/* Index at which new data is stared */
	 uint8_t txBufferQueLen;							/* Length of qued data waiting for transmission */

	 uint8_t dmaRxActive;								/* Indicator if DMA reception needs to be started */
	 uint8_t dmaTxActive;								/* Indicator if DMA reception needs to be started */

 } sUartComCtrl;

extern sUartComCtrl Uart1Ctrl;

UART_HandleTypeDef huart1;

void MX_USART1_UART_Init(void);
void MX_USART1_UART_DeInit(void);
void MX_USART1_DMA_Init(void);

void MX_USART6_UART_Init(void);
void MX_USART6_DMA_Init(void);
void MX_USART6_UART_DeInit(void);
void GNSS_IO_init(void);

uint8_t UART_ButtonAdjust(uint8_t *array);
void UART_StartDMA_Receiption(sUartComCtrl* pUartCtrl);
#ifdef ENABLE_CO2_SUPPORT
void UART_HandleCO2Data(void);
void DigitalCO2_SendCmd(uint8_t CO2Cmd, uint8_t *cmdString, uint8_t *cmdLength);
#endif

#ifdef ENABLE_GNSS_SUPPORT
void UART_HandleGnssData(void);
#endif
#ifdef ENABLE_SENTINEL_MODE
void UART_HandleSentinelData(void);
#endif
void UART_SetGnssCtrl(sUartComCtrl* pTarget);
sUartComCtrl* UART_GetGnssCtrl();
void UART_clearRxBuffer(sUartComCtrl* pUartCtrl);
uint8_t UART_isCO2Connected();
uint8_t UART_isSentinelConnected();
void UART_setTargetChannel(uint8_t channel);
void  UART_MUX_SelectAddress(uint8_t muxAddress);
void UART_SendCmdString(uint8_t *cmdString);
void UART_SendCmdUbx(const uint8_t *cmd, uint8_t len);
void UART_ReadData(uint8_t sensorType, uint8_t flush);
void UART_WriteData(sUartComCtrl* pUartCtrl);
void UART_ChangeBaudrate(uint32_t newBaudrate);
uint8_t UART_isComActive(uint8_t sensorId);
uint8_t UART_isEndIndication(sUartComCtrl* pCtrl, uint8_t index);

void StringToInt(char *pstr, uint32_t *puInt32);
void StringToUInt64(char *pstr, uint64_t *puint64);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
