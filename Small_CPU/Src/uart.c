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
#include "externalInterface.h"
#include "data_exchange.h"
#include <string.h>	/* memset */


/* Private variables ---------------------------------------------------------*/



#define CHUNK_SIZE				(25u)		/* the DMA will handle chunk size transfers */
#define CHUNKS_PER_BUFFER		(5u)



DMA_HandleTypeDef  hdma_usart1_rx, hdma_usart6_rx, hdma_usart6_tx;

uint8_t rxBuffer[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */
uint8_t rxBufferUart6[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */
uint8_t txBufferUart6[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */

static uint8_t rxWriteIndex;							/* Index of the data item which is analysed */
static uint8_t rxReadIndex;								/* Index at which new data is stared */
static uint8_t lastCmdIndex;							/* Index of last command which has not been completely received */
static uint8_t dmaActive;								/* Indicator if DMA reception needs to be started */




/* Exported functions --------------------------------------------------------*/


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

  memset(rxBuffer,BUFFER_NODATA,sizeof(rxBuffer));
  rxReadIndex = 0;
  lastCmdIndex = 0;
  rxWriteIndex = 0;
  dmaActive = 0;
}



void MX_USART1_UART_DeInit(void)
{
	HAL_DMA_Abort(&hdma_usart1_rx);
	HAL_DMA_DeInit(&hdma_usart1_rx);
	HAL_UART_DeInit(&huart1);
	dmaActive = 0;
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

  /* DMA interrupt init */
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
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
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* USART6 DMA Init */
	/* USART6_RX Init */
	hdma_usart6_rx.Instance = DMA2_Stream2;
	hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;
	hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_usart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
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
	hdma_usart6_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
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

		HAL_UART_Transmit(&huart1,indexstr,4,10);
	}
}


void UART_SendCmdString(uint8_t *cmdString)
{
	uint8_t cmdLength = strlen((char*)cmdString);

	if(cmdLength < 20)		/* A longer string is an indication for a missing 0 termination */
	{
		if(dmaActive == 0)
		{
			UART_StartDMA_Receiption();
		}
		HAL_UART_Transmit(&huart1,cmdString,cmdLength,10);
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
	if(dmaActive == 0)
	{
		if(HAL_OK == HAL_UART_Receive_DMA (&huart1, &rxBuffer[rxWriteIndex], CHUNK_SIZE))
		{
			dmaActive = 1;
		}
	}
}

void UART_ChangeBaudrate(uint32_t newBaudrate)
{
	uint8_t dmaWasActive = dmaActive;
//	HAL_DMA_Abort(&hdma_usart1_rx);
		MX_USART1_UART_DeInit();
		//HAL_UART_Abort(&huart1);
		//HAL_DMA_DeInit(&hdma_usart1_rx);


//	huart1.Instance->BRR = UART_BRR_SAMPLING8(HAL_RCC_GetPCLK2Freq()/2, newBaudrate);
	huart1.Init.BaudRate = newBaudrate;
	HAL_UART_Init(&huart1);
	MX_USART1_DMA_Init();
	if(dmaWasActive)
	{
		memset(rxBuffer,BUFFER_NODATA,sizeof(rxBuffer));
		rxReadIndex = 0;
		rxWriteIndex = 0;
		dmaActive = 0;
		UART_StartDMA_Receiption();
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
    	dmaActive = 0;
    	rxWriteIndex+=CHUNK_SIZE;
    	if(rxWriteIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
    	{
    		rxWriteIndex = 0;
    	}
    	if((rxWriteIndex / CHUNK_SIZE) != (rxReadIndex / CHUNK_SIZE) || (rxWriteIndex == rxReadIndex))	/* start next transfer if we did not catch up with read index */
    	{
			UART_StartDMA_Receiption();
    	}
    }
}

void UART_ReadData(uint8_t sensorType)
{
	uint8_t localRX = rxReadIndex;

	while((rxBuffer[localRX]!=BUFFER_NODATA))
	{
		switch (sensorType)
		{
			case SENSOR_MUX:
			case SENSOR_DIGO2:	uartO2_ProcessData(rxBuffer[localRX]);
				break;
#ifdef ENABLE_CO2_SUPPORT
			case SENSOR_CO2:	uartCo2_ProcessData(rxBuffer[localRX]);
				break;
#endif
#ifdef ENABLE_SENTINEL_MODE
			case SENSOR_SENTINEL:	uartSentinel_ProcessData(rxBuffer[localRX]);
				break;
#endif
			default:
				break;
		}

		rxBuffer[localRX] = BUFFER_NODATA;
		localRX++;
		rxReadIndex++;
		if(rxReadIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
		{
			localRX = 0;
			rxReadIndex = 0;
		}
	}
}

void UART_FlushRxBuffer(void)
{
	while(rxBuffer[rxReadIndex] != BUFFER_NODATA)
	{
		rxBuffer[rxReadIndex] = BUFFER_NODATA;
		rxReadIndex++;
		if(rxReadIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
		{
			rxReadIndex = 0;
		}
	}
}

uint8_t UART_isComActive(uint8_t sensorId)
{
	uint8_t active = 1;

	uint8_t ComState = externalInterface_GetSensorState(sensorId + EXT_INTERFACE_MUX_OFFSET);

	if((ComState == UART_COMMON_INIT) || (ComState == UART_COMMON_IDLE) || (ComState == UART_COMMON_ERROR))
	{
		active = 0;
	}
	return active;
}

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
