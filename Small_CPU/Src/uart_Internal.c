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
	//HAL_UART_DeInit(&huart6);
	Uart6Ctrl.dmaRxActive = 0;
	Uart6Ctrl.dmaTxActive = 0;
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

	Uart6Ctrl.pHandle = &huart6;
	Uart6Ctrl.dmaRxActive = 0;
	Uart6Ctrl.dmaTxActive = 0;
	Uart6Ctrl.pRxBuffer = rxBufferUart6;
	Uart6Ctrl.pTxBuffer = txBufferUart6;
	Uart6Ctrl.rxReadIndex = 0;
	Uart6Ctrl.rxWriteIndex = 0;
	Uart6Ctrl.txBufferQueLen = 0;

	UART_clearRxBuffer(&Uart6Ctrl);
	UART_SetGnssCtrl(&Uart6Ctrl);

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

/* Control of the internal receiver, called from the main loops of the RTE. Besides the cyclic polls (see uartGnss_Control) it runs the wake up of a receiver
 * which was put into power save mode before sleep or a dive (state INACTIVE) and keeps its states from blocking:
 *
 *   INACTIVE --(no power down requested)--> WAKE: transmit the wake up bytes 0xFF 0xFF (only if the transmitter is idle, else try again at the next call)
 *   WAKE     --(GNSS_WAKE_WAIT_MS later: the receiver needs about half a second)--> PWRUP
 *   PWRUP    : uartGnss_Control transmits the full power command (UBX-CFG-PMS), again if the transmitter was busy; the acknowledge is evaluated in
 *              uartGnss_ProcessData and ends the state with IDLE. Without acknowledge after GNSS_PWRUP_ACK_TIMEOUT_MS or with a rejection (NAK) the sequence is
 *              repeated (WAKE again after a wake up, only the command after the initialization), at most GNSS_WAKE_MAX_RETRIES times, then the state is left
 *              with IDLE and wakeFailed is set (the power state check after the wake up, see uartGnss_Control, may try again)
 *   WAKE, PWRUP --(power down requested, e.g. the dive starts or the unit goes to sleep)--> IDLE: the power down command is sent from there, this has priority
 *
 * The receiver is only woken by the bytes, never by the time out of the cyclic polls; the INACTIVE state is entered only by the power down sequence. */
void UART6_HandleUART()
{
	static uint8_t retryRequest = 0;
	static uint32_t lastRequestTick = 0;
	static uint32_t TriggerTick = 0;
	static uint16_t timeToTrigger = 0;
	static uint32_t wakeTick = 0;
	static uint32_t txWaitTick = 0;
	static uint8_t waitingForTx = 0;
	uint32_t tick =  HAL_GetTick();

	uartGnssStatus_t gnssState = uartGnss_GetState();

	if(Uart6Ctrl.pHandle != 0)
	{
		if(gnssState != UART_GNSS_INIT)		/* also in WAKE and PWRUP: the acknowledge of the full power command is evaluated */
		{
			UART_ReadData(SENSOR_GNSS, 0);
			UART_WriteData(&Uart6Ctrl);
		}
		if(gnssState == UART_GNSS_INIT)
		{
			lastRequestTick = tick;
			TriggerTick = tick - 10;	/* just to make sure control is triggered */
			timeToTrigger = 1;
			retryRequest = 0;
		}
		else if((gnssState == UART_GNSS_INACTIVE) && (!uartGnss_isPowerDownRequested()))		/* start the wake up of the receiver */
		{
			GNSS_Handle.pwrupWake = 1;
			GNSS_Handle.pwrupSent = 0;
			GNSS_Handle.pwrupNak = 0;
			GNSS_Handle.wakeSent = 0;
			GNSS_Handle.wakeTries = 0;
			GNSS_Handle.wakeFailed = 0;
			GNSS_Handle.pvtParsed = 0;		/* the power save state is not known until the next UBX-NAV-PVT */
			GNSS_Handle.psmZeroValid = 0;
			lastRequestTick = tick;
			gnssState = UART_GNSS_WAKE;
			uartGnss_SetState(gnssState);
		}
		else if(gnssState == UART_GNSS_WAKE)
		{
			if(uartGnss_isPowerDownRequested())
			{
				gnssState = UART_GNSS_IDLE;
				uartGnss_SetState(gnssState);
			}
			else if(!GNSS_Handle.wakeSent)
			{
				if(!waitingForTx)
				{
					waitingForTx = 1;
					txWaitTick = tick;
				}
				if(UART_isTxIdle(&Uart6Ctrl))		/* the buffer of a transmission which is still running is not overwritten */
				{
					txBufferUart6[0] = 0xFF;
					txBufferUart6[1] = 0xFF;
					if(HAL_OK == HAL_UART_Transmit_DMA(Uart6Ctrl.pHandle, Uart6Ctrl.pTxBuffer,2))
					{
						GNSS_Handle.wakeSent = 1;
						wakeTick = tick;
						waitingForTx = 0;
					}
				}
				else if(time_elapsed_ms(txWaitTick, tick) > GNSS_WAKE_TX_TIMEOUT_MS)		/* the transmitter does not get idle: do not wait for ever */
				{
					waitingForTx = 0;
					GNSS_Handle.wakeFailed = 1;
					gnssState = UART_GNSS_IDLE;
					uartGnss_SetState(gnssState);
				}
			}
			else if(time_elapsed_ms(wakeTick, tick) >= GNSS_WAKE_WAIT_MS)		/* the receiver is ready for the command */
			{
				GNSS_Handle.wakeSent = 0;
				GNSS_Handle.pwrupSent = 0;
				GNSS_Handle.pwrupNak = 0;
				lastRequestTick = tick;
				TriggerTick = tick;
				timeToTrigger = 1;
				gnssState = UART_GNSS_PWRUP;
				uartGnss_SetState(gnssState);
			}
		}
		else if(gnssState == UART_GNSS_PWRUP)
		{
			if(uartGnss_isPowerDownRequested())
			{
				gnssState = UART_GNSS_IDLE;
				uartGnss_SetState(gnssState);
			}
			else if(!GNSS_Handle.pwrupSent)		/* the command was not transmitted yet (transmitter busy): the control repeats it */
			{
				if(timeToTrigger == 0)		/* (a new trigger with every call would postpone the control for ever) */
				{
					TriggerTick = tick;
					timeToTrigger = 1;
				}
			}
			else if(GNSS_Handle.pwrupNak || (time_elapsed_ms(GNSS_Handle.pwrupSendTick, tick) > GNSS_PWRUP_ACK_TIMEOUT_MS))		/* rejected or not acknowledged */
			{
				GNSS_Handle.pwrupNak = 0;
				GNSS_Handle.pwrupSent = 0;
				if(GNSS_Handle.wakeTries < GNSS_WAKE_MAX_RETRIES)
				{
					GNSS_Handle.wakeTries++;
					if(GNSS_Handle.pwrupWake)
					{
						GNSS_Handle.wakeSent = 0;
						gnssState = UART_GNSS_WAKE;		/* wake up bytes, wait, command */
						uartGnss_SetState(gnssState);
					}
				}
				else
				{
					GNSS_Handle.wakeFailed = 1;
					gnssState = UART_GNSS_IDLE;
					uartGnss_SetState(gnssState);
				}
			}
		}
		else if(((retryRequest == 0)		/* timeout or error */
				&& (((time_elapsed_ms(lastRequestTick,tick) > (TIMEOUT_SENSOR_ANSWER)) && (gnssState != UART_GNSS_IDLE) && (gnssState != UART_GNSS_PWRUP))	/* retry if no answer after half request interval */
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

			if((gnssState == UART_GNSS_GET_SAT) || (gnssState == UART_GNSS_GET_PVT))	/* timeout */
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
}


/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
