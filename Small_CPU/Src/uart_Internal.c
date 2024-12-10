/**
  ******************************************************************************
  * @file    uart_Internal.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    03-November-2044
  * @brief   Control functions for devices connected to the internal UART
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 
/* Includes ------------------------------------------------------------------*/
#include "uart.h"
#include "uart_Internal.h"
#include "uartProtocol_GNSS.h"
#include "GNSS.h"
#include "externalInterface.h"
#include "data_exchange.h"
#include <string.h>	/* memset */


/* Private variables ---------------------------------------------------------*/

#define REQUEST_INT_SENSOR_MS	(1500)		/* Minimum time interval for cyclic sensor data requests per sensor (UART mux) */
#define COMMAND_TX_DELAY		(30u)		/* The time the sensor needs to recover from a invalid command request */
#define TIMEOUT_SENSOR_ANSWER	(300)		/* Time till a request is repeated if no answer was received */

DMA_HandleTypeDef  hdma_usart6_rx, hdma_usart6_tx;

uint8_t tx6Buffer[CHUNK_SIZE];							/* tx uses less bytes */

uint8_t rxBufferUart6[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */
uint8_t txBufferUart6[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */

sUartComCtrl Uart6Ctrl;

/* Exported functions --------------------------------------------------------*/

void GNSS_IO_init() {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* Peripheral clock enable */
	__HAL_RCC_USART6_CLK_ENABLE()
	;

	__HAL_RCC_GPIOA_CLK_ENABLE()
	;
	/**USART6 GPIO Configuration
	 PA11     ------> USART6_TX
	 PA12     ------> USART6_RX
	 */
	GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
	GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* USART6 DMA Init */
	/* USART6_RX Init */
	hdma_usart6_rx.Instance = DMA2_Stream2;
	hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;
	hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_usart6_rx.Init.PeriphDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart6_rx.Init.Mode = DMA_NORMAL;
	hdma_usart6_rx.Init.Priority = DMA_PRIORITY_LOW;
	hdma_usart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	HAL_DMA_Init(&hdma_usart6_rx);

	__HAL_LINKDMA(&huart6, hdmarx, hdma_usart6_rx);

	/* USART6_TX Init */
	hdma_usart6_tx.Instance = DMA2_Stream6;
	hdma_usart6_tx.Init.Channel = DMA_CHANNEL_5;
	hdma_usart6_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_usart6_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_usart6_tx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_usart6_tx.Init.PeriphDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart6_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart6_tx.Init.Mode = DMA_NORMAL;
	hdma_usart6_tx.Init.Priority = DMA_PRIORITY_LOW;
	hdma_usart6_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	HAL_DMA_Init(&hdma_usart6_tx);

	__HAL_LINKDMA(&huart6, hdmatx, hdma_usart6_tx);

	/* USART6 interrupt Init */
	HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USART6_IRQn);

	MX_USART6_DMA_Init();

}

void MX_USART6_DMA_Init() {
	  /* DMA controller clock enable */
	  __HAL_RCC_DMA2_CLK_ENABLE();

	  /* DMA interrupt init */
	  /* DMA2_Stream2_IRQn interrupt configuration */
	  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	  /* DMA2_Stream6_IRQn interrupt configuration */
	  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}


void MX_USART6_UART_DeInit(void)
{
	HAL_DMA_Abort(&hdma_usart6_rx);
	HAL_DMA_DeInit(&hdma_usart6_rx);
	HAL_DMA_Abort(&hdma_usart6_tx);
	HAL_DMA_DeInit(&hdma_usart6_tx);
	HAL_UART_DeInit(&huart6);
	HAL_UART_DeInit(&huart6);
}

void MX_USART6_UART_Init(void) {
	huart6.Instance = USART6;
	huart6.Init.BaudRate = 9600;
	huart6.Init.WordLength = UART_WORDLENGTH_8B;
	huart6.Init.StopBits = UART_STOPBITS_1;
	huart6.Init.Parity = UART_PARITY_NONE;
	huart6.Init.Mode = UART_MODE_TX_RX;
	huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart6.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_Init(&huart6);

	UART_clearRxBuffer(&Uart6Ctrl);

	Uart6Ctrl.pHandle = &huart6;
	Uart6Ctrl.dmaRxActive = 0;
	Uart6Ctrl.dmaTxActive = 0;
	Uart6Ctrl.pRxBuffer = rxBufferUart6;
	Uart6Ctrl.pTxBuffer = txBufferUart6;
	Uart6Ctrl.rxReadIndex = 0;
	Uart6Ctrl.rxWriteIndex = 0;
	Uart6Ctrl.txBufferQueLen = 0;

	UART_SetGnssCtrl(&Uart6Ctrl);
}

void UART6_HandleUART()
{
	static uint8_t retryRequest = 0;
	static uint32_t lastRequestTick = 0;
	static uint32_t TriggerTick = 0;
	static uint16_t timeToTrigger = 0;
	uint32_t tick =  HAL_GetTick();

	uartGnssStatus_t gnssState = uartGnss_GetState();

		if(gnssState != UART_GNSS_INIT)
		{
			UART_ReadData(SENSOR_GNSS);
			UART_WriteData(&Uart6Ctrl);
		}
		if(gnssState == UART_GNSS_INIT)
		{
			lastRequestTick = tick;
			TriggerTick = tick - 10;	/* just to make sure control is triggered */
			timeToTrigger = 1;
			retryRequest = 0;
		}
		else if((gnssState == UART_GNSS_INACTIVE) && (!uartGnss_isPowerDownRequested()))		/* send dummy bytes to wakeup receiver */
		{
			txBufferUart6[0] = 0xFF;
			txBufferUart6[1] = 0xFF;
			HAL_UART_Transmit_DMA(Uart6Ctrl.pHandle, Uart6Ctrl.pTxBuffer,2);
			timeToTrigger = 500;						/* receiver needs 500ms for wakeup */
			lastRequestTick = tick;
			gnssState = UART_GNSS_PWRUP;
			uartGnss_SetState(gnssState);
		}
		else if(((retryRequest == 0)		/* timeout or error */
				&& (((time_elapsed_ms(lastRequestTick,tick) > (TIMEOUT_SENSOR_ANSWER)) && (gnssState != UART_GNSS_IDLE))	/* retry if no answer after half request interval */
					|| (gnssState == UART_GNSS_ERROR))))
		{
			/* The channel switch will cause the sensor to respond with an error message. */
			/* The sensor needs ~30ms to recover before he is ready to receive the next command => transmission delay needed */

			TriggerTick = tick;
			timeToTrigger = COMMAND_TX_DELAY;
			retryRequest = 1;
		}

		else if(time_elapsed_ms(lastRequestTick,tick) > 1000)	/* switch sensor and / or trigger next request */
		{
			lastRequestTick = tick;
			TriggerTick = tick;
			retryRequest = 0;
			timeToTrigger = 1;

			if((gnssState == UART_GNSS_GET_SAT) || (gnssState == UART_GNSS_GET_PVT) || (gnssState == UART_GNSS_PWRUP))	/* timeout */
			{
				gnssState = UART_GNSS_IDLE;
				uartGnss_SetState(gnssState);
			}
			timeToTrigger = 1;
		}
		if((timeToTrigger != 0) && (time_elapsed_ms(TriggerTick,tick) > timeToTrigger))
		{
			timeToTrigger = 0;
			uartGnss_Control();
		}

}


/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
