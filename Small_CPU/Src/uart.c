/**
  ******************************************************************************
  * @file    uart.c 
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    27-March-2014
  * @brief   button control
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
#include "uartProtocol_O2.h"
#include "uartProtocol_Co2.h"
#include "uartProtocol_Sentinel.h"
#include "uartProtocol_GNSS.h"
#include "externalInterface.h"
#include "data_exchange.h"
#include <string.h>	/* memset */

#ifdef ENABLE_GPIO_V2
extern UART_HandleTypeDef huart6;
extern void UART6_RxCpltCallback(UART_HandleTypeDef *huart);
extern void UART6_TxCpltCallback(UART_HandleTypeDef *huart);
#endif

/* Private variables ---------------------------------------------------------*/


#define TX_BUF_SIZE				(40u)		/* max length for commands */
#define CHUNK_SIZE				(25u)		/* the DMA will handle chunk size transfers */
#define CHUNKS_PER_BUFFER		(6u)



DMA_HandleTypeDef  hdma_usart1_rx, hdma_usart1_tx;

uint8_t rxBuffer[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */
uint8_t txBuffer[CHUNK_SIZE];							/* tx uses less bytes */
uint8_t txBufferQue[TX_BUF_SIZE];						/* In MUX mode command may be send shortly after each other => allow q 1 entry que */
uint8_t txBufferQueLen;

static uint8_t rxWriteIndex;							/* Index of the data item which is analysed */
static uint8_t rxReadIndex;								/* Index at which new data is stared */
static uint8_t lastCmdIndex;							/* Index of last command which has not been completely received */
static uint8_t dmaRxActive;								/* Indicator if DMA reception needs to be started */
static uint8_t dmaTxActive;								/* Indicator if DMA reception needs to be started */

static uint32_t LastCmdRequestTick = 0;					/* Used by ADC handler to avoid interferance with UART communication */

static uint8_t isEndIndication(uint8_t index);

/* Exported functions --------------------------------------------------------*/

void UART_clearRxBuffer(void)
{
	uint16_t index = 0;
	do
	{
		rxBuffer[index++] = BUFFER_NODATA_LOW;
		rxBuffer[index++] = BUFFER_NODATA_HIGH;
	} while (index < sizeof(rxBuffer));

	rxReadIndex = 0;
	rxWriteIndex = 0;
}

void MX_USART1_UART_Init(void)
{
/* regular init */	
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 19200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;

  HAL_UART_Init(&huart1);

  MX_USART1_DMA_Init();

  UART_clearRxBuffer();
  rxReadIndex = 0;
  lastCmdIndex = 0;
  rxWriteIndex = 0;
  dmaRxActive = 0;
  dmaTxActive = 0;
  txBufferQueLen = 0;
}



void MX_USART1_UART_DeInit(void)
{
	HAL_DMA_Abort(&hdma_usart1_rx);
	HAL_DMA_DeInit(&hdma_usart1_rx);
	HAL_DMA_Abort(&hdma_usart1_tx);
	HAL_DMA_DeInit(&hdma_usart1_tx);
	HAL_UART_DeInit(&huart1);
	dmaRxActive = 0;
	dmaTxActive = 0;
	txBufferQueLen = 0;
}

void  MX_USART1_DMA_Init()
{
  /* DMA controller clock enable */
  __DMA2_CLK_ENABLE();

  /* Peripheral DMA init*/
  hdma_usart1_rx.Instance = DMA2_Stream5;
  hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
  hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY; //DMA_MEMORY_TO_PERIPH;
  hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart1_rx.Init.PeriphDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart1_rx.Init.Mode = DMA_NORMAL;
  hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  HAL_DMA_Init(&hdma_usart1_rx);

  __HAL_LINKDMA(&huart1,hdmarx,hdma_usart1_rx);

  hdma_usart1_tx.Instance = DMA2_Stream7;
  hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
  hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart1_tx.Init.PeriphDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart1_tx.Init.Mode = DMA_NORMAL;
  hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
  hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  HAL_DMA_Init(&hdma_usart1_tx);

  __HAL_LINKDMA(&huart1,hdmatx,hdma_usart1_tx);


  /* DMA interrupt init */
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 2, 2);
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 2, 1);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

void  UART_MUX_SelectAddress(uint8_t muxAddress)
{
	uint8_t indexstr[4];

	if(muxAddress <= MAX_MUX_CHANNEL)
	{
		indexstr[0] = '~';
		indexstr[1] = muxAddress;
		indexstr[2] = 0x0D;
		indexstr[3] = 0x0A;
		if(!dmaTxActive)
		{
			memcpy(txBuffer, indexstr, 4);
			dmaTxActive = 0;
			if(HAL_OK == HAL_UART_Transmit_DMA(&huart1,txBuffer,4))
			{
				dmaTxActive = 1;
				while(dmaTxActive)
				{
					HAL_Delay(1);
				}
			}
		}
		else
		{
			memcpy(txBufferQue, indexstr, 4);
			txBufferQueLen = 4;
		}
	}
}


void UART_SendCmdString(uint8_t *cmdString)
{
	uint8_t cmdLength = strlen((char*)cmdString);

	if(dmaTxActive == 0)
	{
		if(cmdLength < TX_BUF_SIZE)		/* A longer string is an indication for a missing 0 termination */
		{
			if(dmaRxActive == 0)
			{
				UART_StartDMA_Receiption();
			}
			memcpy(txBuffer, cmdString, cmdLength);
			if(HAL_OK == HAL_UART_Transmit_DMA(&huart1,txBuffer,cmdLength))
			{
				dmaTxActive = 1;
				LastCmdRequestTick = HAL_GetTick();
			}
		}
	}
	else
	{
		memcpy(txBufferQue, cmdString, cmdLength);
		txBufferQueLen = cmdLength;
	}
}

void UART_SendCmdUbx(const uint8_t *cmd, uint8_t len)
{
	if(len < TX_BUF_SIZE)		/* A longer string is an indication for a missing 0 termination */
	{
		if(dmaRxActive == 0)
		{
			UART_StartDMA_Receiption();
		}
		memcpy(txBuffer, cmd, len);
		if(HAL_OK == HAL_UART_Transmit_DMA(&huart1,txBuffer,len))
		{
			dmaTxActive = 1;
			LastCmdRequestTick = HAL_GetTick();
		}
	}
}


void StringToInt(char *pstr, uint32_t *puInt32)
{
	uint8_t index = 0;
	uint32_t result = 0;
	while((pstr[index] >= '0') && (pstr[index] <= '9'))
	{
		result *=10;
		result += pstr[index] - '0';
		index++;
	}
	*puInt32 = result;
}

void StringToUInt64(char *pstr, uint64_t *puint64)
{
	uint8_t index = 0;
	uint64_t result = 0;
	while((pstr[index] >= '0') && (pstr[index] <= '9'))
	{
		result *=10;
		result += pstr[index] - '0';
		index++;
	}
	*puint64 = result;
}

void UART_StartDMA_Receiption()
{
	if(dmaRxActive == 0)
	{
    	if(((rxWriteIndex / CHUNK_SIZE) != (rxReadIndex / CHUNK_SIZE)) || ((isEndIndication(rxWriteIndex)) && (isEndIndication(rxWriteIndex + 1))))	/* start next transfer if we did not catch up with read index */
    	{
			if(HAL_OK == HAL_UART_Receive_DMA (&huart1, &rxBuffer[rxWriteIndex], CHUNK_SIZE))
			{
				dmaRxActive = 1;
			}
    	}
	}
}

void UART_ChangeBaudrate(uint32_t newBaudrate)
{
	MX_USART1_UART_DeInit();
	huart1.Init.BaudRate = newBaudrate;
	HAL_UART_Init(&huart1);
	MX_USART1_DMA_Init();
	HAL_NVIC_SetPriority(USART1_IRQn, 1, 3);
	HAL_NVIC_EnableIRQ(USART1_IRQn);

	UART_clearRxBuffer();
	rxReadIndex = 0;
	rxWriteIndex = 0;
	dmaRxActive = 0;
	txBufferQueLen = 0;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
    	dmaRxActive = 0;
    	rxWriteIndex+=CHUNK_SIZE;
    	if(rxWriteIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
    	{
    		rxWriteIndex = 0;
    	}
		UART_StartDMA_Receiption();
    }
#ifdef ENABLE_GPIO_V2
    if(huart == &huart6)
    {
    	UART6_RxCpltCallback(huart);
    }
#endif
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart1)
	{
		dmaTxActive = 0;
		UART_WriteData();
		if(txBufferQueLen)
		{
			memcpy(txBuffer, txBufferQue, txBufferQueLen);
			HAL_UART_Transmit_DMA(&huart1,txBuffer,txBufferQueLen);
			dmaTxActive = 1;
			txBufferQueLen = 0;
		}
	}
#ifdef ENABLE_GPIO_V2
	if(huart == &huart6)
	{
		UART6_TxCpltCallback(huart);
	}
#endif
}

uint8_t isEndIndication(uint8_t index)
{
	uint8_t ret = 0;
	if(index % 2)
	{
		if(rxBuffer[index] == BUFFER_NODATA_HIGH)
		{
			ret = 1;
		}
	}
	else
	{
		if(rxBuffer[index] == BUFFER_NODATA_LOW)
		{
			ret = 1;
		}
	}

	return ret;
}
void UART_ReadData(uint8_t sensorType)
{
	uint8_t localRX = rxReadIndex;
	uint8_t futureIndex = rxReadIndex + 1;
	uint8_t moreData = 0;

	if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
	{
		futureIndex = 0;
	}

	if((!isEndIndication(localRX)) || (!isEndIndication(futureIndex)))	do
	{
		while((!isEndIndication(localRX)) || (moreData))
		{
			moreData = 0;
			switch (sensorType)
			{
				case SENSOR_MUX:
				case SENSOR_DIGO2:	uartO2_ProcessData(rxBuffer[localRX]);
					break;
	#ifdef ENABLE_CO2_SUPPORT
				case SENSOR_CO2:	uartCo2_ProcessData(rxBuffer[localRX]);
					break;
	#endif
	#ifdef ENABLE_GNSS_SUPPORT
					case SENSOR_GNSS:	uartGnss_ProcessData(rxBuffer[localRX]);
							break;
	#endif
	#ifdef ENABLE_SENTINEL_MODE
				case SENSOR_SENTINEL:	uartSentinel_ProcessData(rxBuffer[localRX]);
					break;
	#endif
				default:
					break;
			}
			if(localRX % 2)
			{
				rxBuffer[localRX] = BUFFER_NODATA_HIGH;
			}
			else
			{
				rxBuffer[localRX] = BUFFER_NODATA_LOW;
			}

			localRX++;
			rxReadIndex++;
			if(rxReadIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
			{
				localRX = 0;
				rxReadIndex = 0;
			}
			futureIndex++;
			if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
			{
				futureIndex = 0;
			}
		}
		if(!isEndIndication(futureIndex))
		{
			moreData = 1;
		}
	} while(moreData);
}

void UART_WriteData(void)
{
	if(huart1.hdmatx->State == HAL_DMA_STATE_READY)
	{
		huart1.gState = HAL_UART_STATE_READY;
		dmaTxActive = 0;
	}
	if(huart1.hdmarx->State == HAL_DMA_STATE_READY)
	{
		huart1.RxState = HAL_UART_STATE_READY;
		dmaRxActive = 0;
	}
}

void UART_FlushRxBuffer(void)
{
	uint8_t futureIndex = rxReadIndex + 1;

	if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
	{
		futureIndex = 0;
	}
	while((rxBuffer[rxReadIndex] != BUFFER_NODATA_LOW) && (rxBuffer[futureIndex] != BUFFER_NODATA_HIGH))
	{
		if(rxReadIndex % 2)
		{
			rxBuffer[rxReadIndex++] = BUFFER_NODATA_HIGH;
		}
		else
		{
			rxBuffer[rxReadIndex++] = BUFFER_NODATA_LOW;
		}
		if(rxReadIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
		{
			rxReadIndex = 0;
		}
		futureIndex++;
		if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
		{
			futureIndex = 0;
		}
	}
}

uint8_t UART_isComActive(uint8_t sensorId)
{
	uint8_t active = 1;

	if(time_elapsed_ms(LastCmdRequestTick, HAL_GetTick()) > 300) /* UART activity should be inactive 300ms after last command */
	{
		active = 0;
	}
	return active;
}


/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
