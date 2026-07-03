/**
 ******************************************************************************
 * @file    spi.c
 * @author  heinrichs weikamp gmbh
 * @version V0.0.1
 * @date    16-Sept-2014
 * @brief   Source code for spi control
 *
 @verbatim
 ==============================================================================
 ##### How to use #####
 ==============================================================================
 @endverbatim
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/

#include "global_constants.h"
#include "spi.h"
#include "dma.h"
#include "batteryGasGauge.h"
#include "pressure.h"

//#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "scheduler.h"
#include "device_time_hooks.h"   /* sim_skip_blocking_polls (Phase 0 overlay) */

#ifdef DEBUG_GPIO
extern void GPIO_new_DEBUG_LOW(void);
extern void GPIO_new_DEBUG_HIGH(void);
#endif

uint8_t data_error = 0;
uint32_t data_error_time = 0;
uint8_t SPIDataRX = 0; /* Flag to signal that SPI RX callback has been triggered */

static void SPI_Error_Handler(void);

/* USER CODE END 0 */

static uint8_t SPI_check_header_and_footer_ok(void);
static uint8_t DataEX_check_header_and_footer_shifted(void);

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi3;

DMA_HandleTypeDef hdma_tx;
DMA_HandleTypeDef hdma_rx;

// SPI3 init function
void MX_SPI3_Init(void) {
	hspi3.Instance = SPI3;
	hspi3.Init.Mode = SPI_MODE_MASTER;
	hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi3.Init.CLKPolarity = SPI_POLARITY_HIGH;
	hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi3.Init.NSS = SPI_NSS_SOFT;
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi3.Init.TIMode = SPI_TIMODE_DISABLED;
	hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	hspi3.Init.CRCPolynomial = 7;
	HAL_SPI_Init(&hspi3);
}

void MX_SPI3_DeInit(void) {
	HAL_SPI_DeInit(&hspi3);
}

uint8_t SPI3_ButtonAdjust(uint8_t *arrayInput, uint8_t *arrayOutput) {
	if(sim_skip_blocking_polls)
	{
		/* 7-iter loop with 3*10ms HAL_Delay/iter = 210 ms simulated time.
		 * The PIC button controller is not modelled under Renode; the
		 * harness injects buttons via cpu2_mock_button_request directly.
		 * Skip and report success. Production sets the flag to 0. */
		(void)arrayInput;
		(void)arrayOutput;
		return 1;
	}
	HAL_StatusTypeDef status;
	uint8_t answer[10];
	uint8_t rework[10];

	rework[0] = 0xFF;
	for (int i = 0; i < 3; i++) {
		// limiter
		if (arrayInput[i] == 0xFF)
			arrayInput[i] = 0xFE;
		if (arrayInput[i] >= 15) {
			// copy - ausl�se-schwelle
			rework[i + 1] = arrayInput[i];
			// wieder-scharf-schalte-schwelle
			rework[i + 3 + 1] = arrayInput[i] - 10;
		} else if (arrayInput[i] >= 10) {
			// copy - ausl�se-schwelle
			rework[i + 1] = arrayInput[i];
			// wieder-scharf-schalte-schwelle
			rework[i + 3 + 1] = arrayInput[i] - 5;
		} else {
			// copy - ausl�se-schwelle
			rework[i + 1] = 7;
			// wieder-scharf-schalte-schwelle
			rework[i + 3 + 1] = 6;
		}
	}

	status = HAL_OK; /* = 0 */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	for (int i = 0; i < 7; i++) {
		HAL_Delay(10);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
		HAL_Delay(10);
		status += HAL_SPI_TransmitReceive(&hspi3, &rework[i], &answer[i], 1,
				20);
		HAL_Delay(10);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	}

	if (status == HAL_OK) {
		for (int i = 0; i < 3; i++) {
			arrayOutput[i] = answer[i + 2]; // first not, return of 0xFF not
		}
		return 1;
	} else

		return 0;
}

// SPI5 init function
void MX_SPI1_Init(void) {
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_SLAVE;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_HARD_INPUT; //SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLED;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED; //_DISABLED; _ENABLED;
	hspi1.Init.CRCPolynomial = 7;
	HAL_SPI_Init(&hspi1);
}

void MX_SPI_DeInit(void) {
	HAL_SPI_DeInit(&hspi1);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi) {

	GPIO_InitTypeDef GPIO_InitStruct;

	if (hspi->Instance == SPI1) {
		SPIDataRX = 0;
		// Peripheral clock enable
		__SPI1_CLK_ENABLE();
		__GPIOA_CLK_ENABLE();
		//SPI1 GPIO Configuration  
		//PA4   ------> SPI1_CS 
		//PA5   ------> SPI1_SCK
		//PA6   ------> SPI1_MISO 
		//PA7   ------> SPI1_MOSI 

		GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
//    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FAST; /* Decision is based on errata which recommends FAST for GPIO at 90Mhz */
		GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		//##-3- Configure the DMA streams ##########################################
		// Configure the DMA handler for Transmission process 
		hdma_tx.Instance = DMA2_Stream3;
		hdma_tx.Init.Channel = DMA_CHANNEL_3;
		hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
		hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_tx.Init.Mode = DMA_NORMAL;
		hdma_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
		hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		hdma_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
		hdma_tx.Init.MemBurst = DMA_MBURST_INC4;
		hdma_tx.Init.PeriphBurst = DMA_PBURST_INC4;

		HAL_DMA_Init(&hdma_tx);

		// Associate the initialized DMA handle to the the SPI handle
		__HAL_LINKDMA(hspi, hdmatx, hdma_tx);

		// Configure the DMA handler for Transmission process
		hdma_rx.Instance = DMA2_Stream0;
		hdma_rx.Init.Channel = DMA_CHANNEL_3;
		hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
		hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_rx.Init.Mode = DMA_NORMAL;
		hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;
		hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		hdma_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
		hdma_rx.Init.MemBurst = DMA_MBURST_INC4;
		hdma_rx.Init.PeriphBurst = DMA_PBURST_INC4;

		HAL_DMA_Init(&hdma_rx);

		// Associate the initialized DMA handle to the the SPI handle
		__HAL_LINKDMA(hspi, hdmarx, hdma_rx);

		//##-4- Configure the NVIC for DMA #########################################
		//NVIC configuration for DMA transfer complete interrupt (SPI3_RX)
		HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
		HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

		// NVIC configuration for DMA transfer complete interrupt (SPI1_TX)
		HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 1, 1);
		HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
	} else if (hspi->Instance == SPI3) {
		__GPIOC_CLK_ENABLE();
		__SPI3_CLK_ENABLE();

		//SPI1 GPIO Configuration  
		//PC10   ------> SPI3_SCK
		//PC11   ------> SPI3_MISO 
		//PC12   ------> SPI3_MOSI 
		//PA15   ------> SPI3_NSS (official)
		//PC9    ------> SPI3_NSS (hw)

		GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
		GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	}
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi) {
	if (hspi->Instance == SPI1) {
		__SPI1_FORCE_RESET();
		__SPI1_RELEASE_RESET();

		//SPI1 GPIO Configuration  
		//PA5   ------> SPI1_SCK
		//PA6   ------> SPI1_MISO 
		//PA7   ------> SPI1_MOSI 

		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

		HAL_DMA_DeInit(&hdma_tx);
		HAL_DMA_DeInit(&hdma_rx);

		HAL_NVIC_DisableIRQ(DMA2_Stream3_IRQn);
		HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);
	} else if (hspi->Instance == SPI3) {
		__SPI3_FORCE_RESET();
		__SPI3_RELEASE_RESET();

		//SPI1 GPIO Configuration  
		//PC10   ------> SPI3_SCK
		//PC11   ------> SPI3_MISO 
		//PC12   ------> SPI3_MOSI 
		//PA15   ------> SPI3_NSS (official)
		//PC9    ------> SPI3_NSS (hw)
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
	}
}

void SPI_synchronize_with_Master(void) {
#ifdef USE_OLD_SYNC_METHOD
	GPIO_InitTypeDef GPIO_InitStruct;
//
	__GPIOA_CLK_ENABLE();
	/**SPI1 GPIO Configuration
	 PA5   ------> SPI1_SCK
	 */
	GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
	if(!sim_skip_blocking_polls)
	{
		/* These polling waits sit on real GPIO pins driven by the LCD
		 * master. Under emulation the pins float and the while-loops
		 * never exit. */
		HAL_Delay(10);
		while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == 0);
		HAL_Delay(10);
		while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == 1);
		HAL_Delay(50);
	}
#endif
}

void SPI_Start_single_TxRx_with_Master(void) {
	static uint8_t DevicedataDelayCnt = 10;
	static uint8_t DeviceDataPending = 0;
	uint8_t * pOutput;
	HAL_StatusTypeDef retval;

	if ((global.dataSendToSlave.getDeviceDataNow) || (DeviceDataPending))
	{
		if(((DevicedataDelayCnt == 0) || (((get_voltage() != 6.0) && (get_temperature() != 0.0)
											&& global.deviceDataSendToMaster.hw_Info.checkCompass)
											&& global.deviceDataSendToMaster.hw_Info.checkADC)))			/* devicedata complete? */
		{
			global.dataSendToSlave.getDeviceDataNow = 0;
			DeviceDataPending = 0;
			pOutput = (uint8_t*) &(global.deviceDataSendToMaster);
		}
		else
		{
			DeviceDataPending = 1;
			DevicedataDelayCnt--;
			pOutput = (uint8_t*) &(global.dataSendToMaster);
		}

	}
	else
	{
		pOutput = (uint8_t*) &(global.dataSendToMaster);
	}
	retval = HAL_SPI_TransmitReceive_DMA(&hspi1, pOutput,(uint8_t*) &(global.dataSendToSlave), EXCHANGE_BUFFERSIZE);
	if ( retval!= HAL_OK) {
		SPI_Error_Handler();
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	/* restart SPI */
	if (hspi == &hspi1)
	{
		if(SPI_check_header_and_footer_ok())	/* process timestamp provided by main */
		{
			Scheduler_SyncToSPI(global.dataSendToSlave.header.checkCode[SPI_HEADER_INDEX_TX_TICK]);
		}
		else
		{
			Scheduler_SyncToSPI(0); /* => no async will be calculated */
		}

		SPIDataRX = 1;

		/* stop data exchange? */
		if (global.mode == MODE_SHUTDOWN) {
			global.dataSendToSlavePending = 0;
			global.dataSendToSlaveIsValid = 1;
			global.dataSendToSlaveIsNotValidCount = 0;
		}
	}
}

uint8_t SPI_Evaluate_RX_Data()
{
	uint8_t resettimeout = 1;
	uint8_t ret = SPIDataRX;

	if ((global.mode != MODE_SHUTDOWN) && ( global.mode != MODE_SLEEP) && (SPIDataRX))
	{
		SPIDataRX = 0;
		/* data consistent? */
		if (SPI_check_header_and_footer_ok()) {
			global.dataSendToMaster.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_OK;
	//		GPIO_new_DEBUG_HIGH(); //For debug.
			global.dataSendToSlaveIsValid = 1;
			global.dataSendToSlaveIsNotValidCount = 0;
			/* Master signal a data shift outside of his control => reset own DMA and resync */
			if(global.dataSendToSlave.header.checkCode[SPI_HEADER_INDEX_RX_STATE] == SPI_RX_STATE_SHIFTED)
			{
				HAL_SPI_Abort_IT(&hspi1);
				Scheduler_Request_sync_with_SPI(SPI_SYNC_METHOD_HARD);
			}
			else
			{
			}
			SPI_Start_single_TxRx_with_Master();
		}
		else
		{
	//		GPIO_new_DEBUG_LOW(); //For debug.
				global.dataSendToSlaveIsValid = 0;
				global.dataSendToSlaveIsNotValidCount++;
				if(DataEX_check_header_and_footer_shifted())
				{

					/* Reset own DMA */
					if ((global.dataSendToSlaveIsNotValidCount % 10) == 1)  //% 10
					{	
						HAL_SPI_Abort_IT(&hspi1); /* reset DMA only once */
					}
					/* Signal problem to master */
					if ((global.dataSendToSlaveIsNotValidCount ) >= 2)
					{
						global.dataSendToMaster.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_SHIFTED;
					}
				}
				else /* handle received data as if no data would have been received */
				{
					global.dataSendToMaster.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_OFFLINE;
					resettimeout = 0;
				}
				HAL_SPI_TransmitReceive_DMA(&hspi1,(uint8_t*) &(global.dataSendToMaster),(uint8_t*) &(global.dataSendToSlave), EXCHANGE_BUFFERSIZE);
		}

		if(global.dataSendToSlaveIsValid)
		{
			global.dataSendToMaster.power_on_reset = 0;
			global.deviceDataSendToMaster.power_on_reset = 0;

			scheduleSpecial_Evaluate_DataSendToSlave();
		}

		if(resettimeout)
		{
				global.check_sync_not_running = 0;
		}
	}
	return ret;
}

static uint8_t SPI_check_header_and_footer_ok(void) {
	if (global.dataSendToSlave.header.checkCode[0] != 0xBB)
		return 0;
#ifdef USE_OLD_HEADER_FORMAT
	if (global.dataSendToSlave.header.checkCode[1] != 0x01)
		return 0;
	if (global.dataSendToSlave.header.checkCode[2] != 0x01)
		return 0;
#endif
	if (global.dataSendToSlave.header.checkCode[3] != 0xBB)
		return 0;
	if (global.dataSendToSlave.footer.checkCode[0] != 0xF4)
		return 0;
	if (global.dataSendToSlave.footer.checkCode[1] != 0xF3)
		return 0;
	if (global.dataSendToSlave.footer.checkCode[2] != 0xF2)
		return 0;
	if (global.dataSendToSlave.footer.checkCode[3] != 0xF1)
		return 0;

	return 1;
}


/* Check if there is an empty frame providec by RTE (all 0) or even no data provided by RTE (all 0xFF)
 * If that is not the case the DMA is somehow not in sync
 */
uint8_t DataEX_check_header_and_footer_shifted()
{
	uint8_t ret = 1;
	if((global.dataSendToSlave.footer.checkCode[0] == 0x00)
	&& (global.dataSendToSlave.footer.checkCode[1] == 0x00)
	&& (global.dataSendToSlave.footer.checkCode[2] == 0x00)
	&& (global.dataSendToSlave.footer.checkCode[3] == 0x00)) { ret = 0; }

	if((global.dataSendToSlave.footer.checkCode[0] == 0xff)
	&& (global.dataSendToSlave.footer.checkCode[1] == 0xff)
	&& (global.dataSendToSlave.footer.checkCode[2] == 0xff)
	&& (global.dataSendToSlave.footer.checkCode[3] == 0xff)) { ret = 0; }

	return ret;
}

static void SPI_Error_Handler(void) {
	//The device is locks. Hard to recover.
//  while(1)
//  {
//  }
}

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
