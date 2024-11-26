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


static uint8_t isEndIndication6(uint8_t index);
static uint8_t gnssState = UART_GNSS_INIT;

/* Private variables ---------------------------------------------------------*/


#define TX_BUF_SIZE				(40u)		/* max length for commands */
#define CHUNK_SIZE				(25u)		/* the DMA will handle chunk size transfers */
#define CHUNKS_PER_BUFFER		(6u)

#define REQUEST_INT_SENSOR_MS	(1500)		/* Minimum time interval for cyclic sensor data requests per sensor (UART mux) */
#define COMMAND_TX_DELAY		(30u)		/* The time the sensor needs to recover from a invalid command request */
#define TIMEOUT_SENSOR_ANSWER	(300)		/* Time till a request is repeated if no answer was received */


static receiveStateGnss_t rxState = GNSSRX_READY;
static uint8_t GnssConnected = 0;						/* Binary indicator if a sensor is connected or not */

static uint8_t writeIndex = 0;

static uint8_t dataToRead = 0;

DMA_HandleTypeDef  hdma_usart6_rx, hdma_usart6_tx;

uint8_t tx6Buffer[CHUNK_SIZE];							/* tx uses less bytes */
uint8_t tx6BufferQue[TX_BUF_SIZE];						/* In MUX mode command may be send shortly after each other => allow q 1 entry que */
uint8_t tx6BufferQueLen;

uint8_t rxBufferUart6[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */
uint8_t txBufferUart6[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */

static uint8_t rx6WriteIndex;							/* Index of the data item which is analysed */
static uint8_t rx6ReadIndex;								/* Index at which new data is stared */

static uint8_t dmaRx6Active;								/* Indicator if DMA reception needs to be started */
static uint8_t dmaTx6Active;								/* Indicator if DMA reception needs to be started */


/* Exported functions --------------------------------------------------------*/

void UART_clearRx6Buffer(void)
{
	uint16_t index = 0;
	do
	{
		rxBufferUart6[index++] = BUFFER_NODATA_LOW;
		rxBufferUart6[index++] = BUFFER_NODATA_HIGH;
	} while (index < sizeof(rxBufferUart6));

	rx6ReadIndex = 0;
	rx6WriteIndex = 0;
}

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

	UART_clearRx6Buffer();
	dmaRx6Active = 0;
	dmaTx6Active = 0;
	tx6BufferQueLen = 0;
}



void UART6_SendCmdUbx(const uint8_t *cmd, uint8_t len)
{
	if(len < TX_BUF_SIZE)		/* A longer string is an indication for a missing 0 termination */
	{
		if(dmaRx6Active == 0)
		{
			UART6_StartDMA_Receiption();
		}
		memcpy(tx6Buffer, cmd, len);
		if(HAL_OK == HAL_UART_Transmit_DMA(&huart6,tx6Buffer,len))
		{
			dmaTx6Active = 1;
		}
	}
}

uint8_t isEndIndication6(uint8_t index)
{
	uint8_t ret = 0;
	if(index % 2)
	{
		if(rxBufferUart6[index] == BUFFER_NODATA_HIGH)
		{
			ret = 1;
		}
	}
	else
	{
		if(rxBufferUart6[index] == BUFFER_NODATA_LOW)
		{
			ret = 1;
		}
	}

	return ret;
}

void UART6_StartDMA_Receiption()
{
	if(dmaRx6Active == 0)
	{
    	if(((rx6WriteIndex / CHUNK_SIZE) != (rx6ReadIndex / CHUNK_SIZE)) || ((isEndIndication6(rx6WriteIndex)) && (isEndIndication6(rx6WriteIndex + 1))))	/* start next transfer if we did not catch up with read index */
    	{
			if(HAL_OK == HAL_UART_Receive_DMA (&huart6, &rxBufferUart6[rx6WriteIndex], CHUNK_SIZE))
			{
				dmaRx6Active = 1;
			}
    	}
	}
}



void UART6_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart6)
    {
    	dmaRx6Active = 0;
    	rx6WriteIndex+=CHUNK_SIZE;
    	if(rx6WriteIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
    	{
    		rx6WriteIndex = 0;
    	}
		UART6_StartDMA_Receiption();
    }
}
void UART6_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart6)
	{
		dmaTx6Active = 0;
		UART6_WriteData();
		if(tx6BufferQueLen)
		{
			memcpy(tx6Buffer, tx6BufferQue, tx6BufferQueLen);
			HAL_UART_Transmit_DMA(&huart6,tx6Buffer,tx6BufferQueLen);
			dmaTx6Active = 1;
			tx6BufferQueLen = 0;
		}
	}
}

void UART6_ReadData()
{
	uint8_t localRX = rx6ReadIndex;
	uint8_t futureIndex = rx6ReadIndex + 1;
	uint8_t moreData = 0;

	if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
	{
		futureIndex = 0;
	}

	if((!isEndIndication6(localRX)) || (!isEndIndication6(futureIndex)))	do
	{
		while((!isEndIndication6(localRX)) || (moreData))
		{
			moreData = 0;
			UART6_Gnss_ProcessData(rxBufferUart6[localRX]);

			if(localRX % 2)
			{
				rxBufferUart6[localRX] = BUFFER_NODATA_HIGH;
			}
			else
			{
				rxBufferUart6[localRX] = BUFFER_NODATA_LOW;
			}

			localRX++;
			rx6ReadIndex++;
			if(rx6ReadIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
			{
				localRX = 0;
				rx6ReadIndex = 0;
			}
			futureIndex++;
			if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
			{
				futureIndex = 0;
			}
		}
		if(!isEndIndication6(futureIndex))
		{
			moreData = 1;
		}
	} while(moreData);
}

void UART6_WriteData(void)
{
	if(huart6.hdmatx->State == HAL_DMA_STATE_READY)
	{
		huart6.gState = HAL_UART_STATE_READY;
		dmaTx6Active = 0;
	}
	if(huart6.hdmarx->State == HAL_DMA_STATE_READY)
	{
		huart6.RxState = HAL_UART_STATE_READY;
		dmaRx6Active = 0;
	}
}

void UART6_Gnss_SendCmd(uint8_t GnssCmd)
{
	const uint8_t* pData;
	uint8_t txLength = 0;

	switch (GnssCmd)
	{
		case GNSSCMD_LOADCONF_0:	pData = configUBX;
									txLength = sizeof(configUBX) / sizeof(uint8_t);
				break;
		case GNSSCMD_LOADCONF_1:	pData = setNMEA410;
									txLength = sizeof(setNMEA410) / sizeof(uint8_t);
				break;
		case GNSSCMD_LOADCONF_2:	pData = setGNSS;
									txLength = sizeof(setGNSS) / sizeof(uint8_t);
				break;
		case GNSSCMD_GET_PVT_DATA:	pData = getPVTData;
									txLength = sizeof(getPVTData) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_NAV_DATA:	pData = getNavigatorData;
									txLength = sizeof(getNavigatorData) / sizeof(uint8_t);
			break;

		default:
			break;
	}
	if(txLength != 0)
	{
		UART6_SendCmdUbx(pData, txLength);
	}
}

void UART6_Gnss_Control(void)
{
	static uint32_t warmupTick = 0;

	switch (gnssState)
	{
		case UART_GNSS_INIT:  		gnssState = UART_GNSS_WARMUP;
									warmupTick =  HAL_GetTick();
									UART_clearRxBuffer();
				break;
		case UART_GNSS_WARMUP:		if(time_elapsed_ms(warmupTick,HAL_GetTick()) > 1000)
									{
										gnssState = UART_GNSS_LOADCONF_0;
									}
				break;
		case UART_GNSS_LOADCONF_0:	UART6_Gnss_SendCmd(GNSSCMD_LOADCONF_0);
									gnssState = UART_GNSS_LOADCONF_1;
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_LOADCONF_1:	UART6_Gnss_SendCmd(GNSSCMD_LOADCONF_1);
									gnssState = UART_GNSS_LOADCONF_2;
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_LOADCONF_2:	UART6_Gnss_SendCmd(GNSSCMD_LOADCONF_2);
									gnssState = UART_GNSS_IDLE;
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_IDLE:		UART6_Gnss_SendCmd(GNSSCMD_GET_PVT_DATA);
									gnssState = UART_GNSS_GET_PVT;
									rxState = GNSSRX_DETECT_HEADER_0;
				break;
		default:
				break;
	}
}

void UART6_Gnss_ProcessData(uint8_t data)
{
	GNSS_Handle.uartWorkingBuffer[writeIndex++] = data;
	switch(rxState)
	{
		case GNSSRX_DETECT_ACK_0:
		case GNSSRX_DETECT_HEADER_0:	if(data == 0xB5)
										{
											writeIndex = 0;
											memset(GNSS_Handle.uartWorkingBuffer,0, sizeof(GNSS_Handle.uartWorkingBuffer));
											GNSS_Handle.uartWorkingBuffer[writeIndex++] = data;
											rxState++;
										}
			break;
		case GNSSRX_DETECT_ACK_1:
		case GNSSRX_DETECT_HEADER_1:	if(data == 0x62)
			 	 	 	 	 	 	 	{
											rxState++;
			 	 	 	 	 	 	 	}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_ACK_2:		if(data == 0x05)
										{
											rxState++;
										}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_ACK_3:		if((data == 0x01) || (data == 0x00))
										{
											GnssConnected = 1;
											rxState = GNSSRX_READY;
										}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_2:	if(data == 0x01)
			 	 	 	 	 	 	 	{
											rxState++;
			 	 	 	 	 	 	 	}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_3:
										switch(data)
										{
											case 0x21: rxState = GNSSRX_READ_NAV_DATA;
														dataToRead = 20;
												break;
											case 0x07:	rxState = GNSSRX_READ_PVT_DATA;
														dataToRead = 92;
												break;
											case 0x02:  rxState = GNSSRX_READ_POSLLH_DATA;
												break;
											default: 	rxState = GNSSRX_DETECT_HEADER_0;
												break;
										}
			break;
			case GNSSRX_READ_NAV_DATA:
			case GNSSRX_READ_PVT_DATA:
			case GNSSRX_READ_POSLLH_DATA:		if(dataToRead > 0)
												{
													dataToRead--;
												}
												else
												{
														switch(rxState)
														{
															case GNSSRX_READ_NAV_DATA: GNSS_ParseNavigatorData(&GNSS_Handle);
																break;
															case GNSSRX_READ_PVT_DATA:	GNSS_ParsePVTData(&GNSS_Handle);
																break;
															case GNSSRX_READ_POSLLH_DATA:  GNSS_ParsePOSLLHData(&GNSS_Handle);
																break;
															default: 	rxState = GNSSRX_DETECT_HEADER_0;
																break;
														}
														rxState = GNSSRX_DETECT_HEADER_0;
														gnssState = UART_GNSS_IDLE;
												}
				break;
		default:	rxState = GNSSRX_READY;
			break;
	}
}


void UART6_HandleUART()
{
	static uint8_t retryRequest = 0;
	static uint32_t lastRequestTick = 0;
	static uint32_t TriggerTick = 0;
	static uint8_t timeToTrigger = 0;
	uint32_t tick =  HAL_GetTick();

		if(gnssState != UART_GNSS_INIT)
		{
			UART6_ReadData();
			UART6_WriteData();
		}
		if(gnssState == UART_GNSS_INIT)
		{
			lastRequestTick = tick;
			TriggerTick = tick - 10;	/* just to make sure control is triggered */
			timeToTrigger = 1;
			retryRequest = 0;
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

			if((gnssState == UART_GNSS_GET_PVT))	/* timeout */
			{
				gnssState = UART_GNSS_IDLE;
			}
			timeToTrigger = 1;
		}
		if((timeToTrigger != 0) && (time_elapsed_ms(TriggerTick,tick) > timeToTrigger))
		{
			timeToTrigger = 0;
			UART6_Gnss_Control();
		}

}


/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
