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
extern sUartComCtrl Uart6Ctrl;
#endif

/* Private variables ---------------------------------------------------------*/

DMA_HandleTypeDef  hdma_usart1_rx, hdma_usart1_tx;

uint8_t rxBuffer[CHUNK_SIZE * CHUNKS_PER_BUFFER];		/* The complete buffer has a X * chunk size to allow variations in buffer read time */
uint8_t txBuffer[TX_BUF_SIZE];							/* tx uses less bytes */
uint8_t txBufferQue[TX_BUF_SIZE];						/* In MUX mode command may be send shortly after each other => allow q 1 entry que */


static uint8_t lastCmdIndex;							/* Index of last command which has not been completely received */

sUartComCtrl Uart1Ctrl;
static  sUartComCtrl* pGnssCtrl = NULL;

static uint32_t LastCmdRequestTick = 0;					/* Used by ADC handler to avoid interferance with UART communication */

/* Exported functions --------------------------------------------------------*/


void UART_SetGnssCtrl(sUartComCtrl* pTarget)
{
	pGnssCtrl = pTarget;
}

sUartComCtrl* UART_GetGnssCtrl()
{
	return pGnssCtrl;
}


void UART_clearRxBuffer(sUartComCtrl* pUartCtrl)
{
	uint16_t index = 0;
	do
	{
		pUartCtrl->pRxBuffer[index++] = BUFFER_NODATA_LOW;
		pUartCtrl->pRxBuffer[index++] = BUFFER_NODATA_HIGH;
	} while (index < sizeof(rxBuffer));

	pUartCtrl->rxReadIndex = 0;
	pUartCtrl->rxWriteIndex = 0;
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

  UART_clearRxBuffer(&Uart1Ctrl);
  lastCmdIndex = 0;

  Uart1Ctrl.pHandle = &huart1;
  Uart1Ctrl.rxWriteIndex = 0;
  Uart1Ctrl.rxReadIndex = 0;
  Uart1Ctrl.dmaRxActive = 0;
  Uart1Ctrl.dmaTxActive = 0;
  Uart1Ctrl.pRxBuffer = rxBuffer;
  Uart1Ctrl.pTxBuffer = txBuffer;
  Uart1Ctrl.txBufferQueLen = 0;

#ifndef ENABLE_GPIO_V2
  UART_SetGnssCtrl(&Uart1Ctrl);
#endif
}



void MX_USART1_UART_DeInit(void)
{
	HAL_DMA_Abort(&hdma_usart1_rx);
	HAL_DMA_DeInit(&hdma_usart1_rx);
	HAL_DMA_Abort(&hdma_usart1_tx);
	HAL_DMA_DeInit(&hdma_usart1_tx);
	HAL_UART_DeInit(&huart1);
	Uart1Ctrl.dmaRxActive = 0;
	Uart1Ctrl.dmaTxActive = 0;
	Uart1Ctrl.txBufferQueLen = 0;
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

void UART_MUX_SelectAddress(uint8_t muxAddress)
{
	uint8_t indexstr[4];

	if(muxAddress <= MAX_MUX_CHANNEL)
	{
		indexstr[0] = '~';
		indexstr[1] = muxAddress;
		indexstr[2] = 0x0D;
		indexstr[3] = 0x0A;
		if(!Uart1Ctrl.dmaTxActive)
		{
			memcpy(txBuffer, indexstr, 4);
			 Uart1Ctrl.dmaTxActive = 0;
			if(HAL_OK == HAL_UART_Transmit_DMA(&huart1,txBuffer,4))
			{
				 Uart1Ctrl.dmaTxActive = 1;
				while(Uart1Ctrl.dmaTxActive)
				{
					HAL_Delay(1);
				}
			}
		}
		else
		{
			memcpy(txBufferQue, indexstr, 4);
			Uart1Ctrl.txBufferQueLen = 4;
		}
	}
}


void UART_SendCmdString(uint8_t *cmdString)
{
	uint8_t cmdLength = strlen((char*)cmdString);

	if(Uart1Ctrl.dmaTxActive == 0)
	{
		if(cmdLength < TX_BUF_SIZE)		/* A longer string is an indication for a missing 0 termination */
		{
			if(Uart1Ctrl.dmaRxActive == 0)
			{
				UART_StartDMA_Receiption(&Uart1Ctrl);
			}
			memcpy(txBuffer, cmdString, cmdLength);
			if(HAL_OK == HAL_UART_Transmit_DMA(&huart1,txBuffer,cmdLength))
			{
				Uart1Ctrl.dmaTxActive = 1;
				LastCmdRequestTick = HAL_GetTick();
			}
		}
	}
	else
	{
		memcpy(txBufferQue, cmdString, cmdLength);
		Uart1Ctrl.txBufferQueLen = cmdLength;
	}
}

void UART_AddFletcher(uint8_t* pBuffer, uint8_t length)
{
	uint8_t ck_A = 0;
	uint8_t ck_B = 0;
	uint8_t index = 0;


	pBuffer += 2; /* skip sync chars */
	for(index = 2; index < length; index++)
	{
		ck_A += *pBuffer++;
		ck_B += ck_A;
	}
	*pBuffer++ = ck_A;
	*pBuffer++ = ck_B;
}

void UART_SendCmdUbx(const uint8_t *cmd, uint8_t len)
{
	if(len < TX_BUF_SIZE)		/* A longer string is an indication for a missing 0 termination */
	{
		if(pGnssCtrl != NULL)
		{
			if(pGnssCtrl->dmaRxActive == 0)
			{
				UART_StartDMA_Receiption(pGnssCtrl);
			}
			memcpy(pGnssCtrl->pTxBuffer, cmd, len);
			UART_AddFletcher(pGnssCtrl->pTxBuffer, len);
			len += 2;
			if(HAL_OK == HAL_UART_Transmit_DMA(pGnssCtrl->pHandle,pGnssCtrl->pTxBuffer,len))
			{
				pGnssCtrl->dmaTxActive = 1;
				LastCmdRequestTick = HAL_GetTick();
			}
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

void UART_StartDMA_Receiption(sUartComCtrl* pUartCtrl)
{
	if(pUartCtrl->dmaRxActive == 0)
	{
    	if(((pUartCtrl->rxWriteIndex / CHUNK_SIZE) != (pUartCtrl->rxReadIndex / CHUNK_SIZE)) || ((UART_isEndIndication(pUartCtrl, pUartCtrl->rxWriteIndex)) && (UART_isEndIndication(pUartCtrl, pUartCtrl->rxWriteIndex + 1))))	/* start next transfer if we did not catch up with read index */
    	{
			if(HAL_OK == HAL_UART_Receive_DMA (pUartCtrl->pHandle, &pUartCtrl->pRxBuffer[pUartCtrl->rxWriteIndex], CHUNK_SIZE))
			{
				pUartCtrl->dmaRxActive = 1;
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

	UART_clearRxBuffer(&Uart1Ctrl);
	Uart1Ctrl.rxReadIndex = 0;
	Uart1Ctrl.rxWriteIndex = 0;
	Uart1Ctrl.dmaRxActive = 0;
	Uart1Ctrl.dmaTxActive = 0;
	Uart1Ctrl.txBufferQueLen = 0;
}

void UART_HandleRxComplete(sUartComCtrl* pUartCtrl)
{
	pUartCtrl->dmaRxActive = 0;
	pUartCtrl->rxWriteIndex+=CHUNK_SIZE;
	if(pUartCtrl->rxWriteIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
	{
		pUartCtrl->rxWriteIndex = 0;
	}
	UART_StartDMA_Receiption(pUartCtrl);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart1)
	{
		UART_HandleRxComplete(&Uart1Ctrl);
	}
#ifdef ENABLE_GPIO_V2
	if(huart == &huart6)
	{
		UART_HandleRxComplete(&Uart6Ctrl);
	}
#endif
}

void UART_HandleTxComplete(sUartComCtrl* pUartCtrl)
{
	pUartCtrl->dmaTxActive = 0;
	UART_WriteData(pUartCtrl);
	if(pUartCtrl->txBufferQueLen)
	{
		memcpy(pUartCtrl->pTxBuffer, pUartCtrl->pTxQue, pUartCtrl->txBufferQueLen);
		HAL_UART_Transmit_DMA(pUartCtrl->pHandle,pUartCtrl->pTxBuffer,pUartCtrl->txBufferQueLen);
		pUartCtrl->dmaTxActive = 1;
		pUartCtrl->txBufferQueLen = 0;
	}
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart1)
	{
		UART_HandleTxComplete(&Uart1Ctrl);
	}
#ifdef ENABLE_GPIO_V2
	if(huart == &huart6)
	{
		UART_HandleTxComplete(&Uart6Ctrl);
	}
#endif
}

uint8_t UART_isEndIndication(sUartComCtrl* pCtrl, uint8_t index)
{
	uint8_t ret = 0;
	if(index % 2)
	{
		if(pCtrl->pRxBuffer[index] == BUFFER_NODATA_HIGH)
		{
			ret = 1;
		}
	}
	else
	{
		if(pCtrl->pRxBuffer[index] == BUFFER_NODATA_LOW)
		{
			ret = 1;
		}
	}

	return ret;
}
void UART_ReadData(uint8_t sensorType, uint8_t flush)	/* flush = 1 skips processing of data => data is discarded */
{
	uint8_t localRX;
	uint8_t futureIndex;
	uint8_t moreData = 0;

	sUartComCtrl* pUartCtrl;

	if(sensorType == SENSOR_GNSS)
	{
#ifdef ENABLE_GPIO_V2
		pUartCtrl = &Uart6Ctrl;
#else
		pUartCtrl = &Uart1Ctrl;
#endif
	}
	else
	{
		pUartCtrl = &Uart1Ctrl;
	}
	localRX = pUartCtrl->rxReadIndex;
	futureIndex = pUartCtrl->rxReadIndex + 1;
	if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
	{
		futureIndex = 0;
	}

	if(!UART_isEndIndication(pUartCtrl, futureIndex))
	{
		moreData = 1;
	}
	
	if((!UART_isEndIndication(pUartCtrl, localRX)) || (moreData))
	do
	{
		while((!UART_isEndIndication(pUartCtrl, localRX)) || (moreData))
		{
			moreData = 0;
			switch (sensorType)
			{
				case SENSOR_MUX:
				case SENSOR_DIGO2:	uartO2_ProcessData(pUartCtrl->pRxBuffer[localRX]);
					break;
	#ifdef ENABLE_CO2_SUPPORT
				case SENSOR_CO2:	uartCo2_ProcessData(pUartCtrl->pRxBuffer[localRX]);
					break;
	#endif
	#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
					case SENSOR_GNSS:	uartGnss_ProcessData(pUartCtrl->pRxBuffer[localRX]);
							break;
	#endif
	#ifdef ENABLE_SENTINEL_MODE
				case SENSOR_SENTINEL:	uartSentinel_ProcessData(pUartCtrl->pRxBuffer[localRX]);
					break;
	#endif
				default:
					break;
			}
			if(localRX % 2)
			{
				pUartCtrl->pRxBuffer[localRX] = BUFFER_NODATA_HIGH;
			}
			else
			{
				pUartCtrl->pRxBuffer[localRX] = BUFFER_NODATA_LOW;
			}

			localRX++;
			pUartCtrl->rxReadIndex++;
			if(pUartCtrl->rxReadIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
			{
				localRX = 0;
				pUartCtrl->rxReadIndex = 0;
			}
			futureIndex++;
			if(futureIndex >= CHUNK_SIZE * CHUNKS_PER_BUFFER)
			{
				futureIndex = 0;
			}
		}
		if(!UART_isEndIndication(pUartCtrl, futureIndex))
		{
			moreData = 1;
		}
	} while(moreData);
}

void UART_WriteData(sUartComCtrl* pUartCtrl)
{
	if(pUartCtrl->pHandle->hdmatx->State == HAL_DMA_STATE_READY)
	{
		pUartCtrl->pHandle->gState = HAL_UART_STATE_READY;
		pUartCtrl->dmaTxActive = 0;
	}
	if(pUartCtrl->pHandle->hdmarx->State == HAL_DMA_STATE_READY)
	{
		pUartCtrl->pHandle->RxState = HAL_UART_STATE_READY;
		pUartCtrl->dmaRxActive = 0;
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
