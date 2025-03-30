/**
  ******************************************************************************
  * @file    externCPU2bootloader.c Template
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    23-Oct-2014
  * @version V0.0.1
  * @since   23-Oct-2014
  * @brief   Main Template to communicate with the second CPU in bootloader mode
	*						bootloader ROM build by ST and defined in AN4286
  *
  @verbatim
  ==============================================================================
                        ##### How to use #####
  ==============================================================================
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "ostc.h"
#include "settings.h"
#include "externCPU2bootloader.h"
#include "externLogbookFlash.h"
#include "tComm.h"


/* Exported variables --------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static uint8_t boot_sync_frame(void);
static uint8_t boot_ack(void);
static uint8_t boot_get_id(uint8_t *RxBuffer);
static uint8_t boot_get_version(uint8_t *RxBuffer);
static uint8_t boot_write_memory(uint32_t address, uint8_t length_minus_1, uint8_t *data);
static uint8_t boot_erase_memory(void);
static void	Bootloader_send_command(uint8_t command);
static void Bootloader_spi_single(uint8_t TxByte);
static void Bootloader_spi(uint16_t lengthData, uint8_t *aTxBuffer, uint8_t *aRxBuffer);


/* Exported functions --------------------------------------------------------*/

uint8_t extCPU2bootloader_start(uint8_t *version, uint16_t *chipID)
{
	uint8_t aRxBuffer[256] = { 0 };

	HAL_GPIO_WritePin(SMALLCPU_CSB_GPIO_PORT,SMALLCPU_CSB_PIN,GPIO_PIN_RESET);
	
	boot_sync_frame();
	boot_get_version(aRxBuffer);
	*version = aRxBuffer[1];
	HAL_Delay(10);
	boot_get_id(aRxBuffer);
	*chipID = ((uint16_t)aRxBuffer[2]) << 8;
	*chipID += (uint16_t)aRxBuffer[3];
	HAL_Delay(10);
	if((*chipID == 0x431) && (*version > 10) && (*version < 32))
		return 1;
	else
		return 0;
}


uint8_t extCPU2bootloader_internal(uint8_t* buffer, uint32_t length, char* display_text)
{
  uint8_t version = 0;
  uint16_t chipID = 0;

  if(!extCPU2bootloader_start(&version,&chipID))
    return 0;
	if(!boot_erase_memory())
	  return 0;
	HAL_Delay(100);
	uint16_t i=0;
	uint32_t lengthsave = length;
	uint8_t percent = 0;
  
	while(length)
	{
		percent = (100 * (i * 256)) /lengthsave;
		tComm_verlauf(percent);

	  if(length > 256)
	  {
	    if( !boot_write_memory(0x08000000 + (i * 256), 255, &buffer[i * 256]) )
				return 0;;
	    length -= 256;

	  }
	  else
    {
      if(!boot_write_memory(0x08000000 + (i * 256), length - 1, &buffer[i * 256]))
				return 0;
      length = 0;
    }
		i++;
	}
	return 2;
}


uint8_t extCPU2bootloader(uint8_t* buffer, uint32_t length, char* display_text)
{
	uint8_t result = 0;

	MX_SmallCPU_Reset_To_Boot();
	result = extCPU2bootloader_internal(buffer,length,display_text);
	MX_SmallCPU_Reset_To_Standard();
	return result;
}

/* Private functions --------------------------------------------------------*/

static uint8_t boot_sync_frame(void)
{
	Bootloader_spi_single(0x5a);
	return boot_ack();
}

static uint8_t boot_get_version(uint8_t *RxBuffer)
{
	Bootloader_spi_single(0x5a);
	Bootloader_send_command(0x01);
	if(!boot_ack())
		return 0;
	Bootloader_spi(3, NULL, RxBuffer);
	return boot_ack();
}


static uint8_t boot_get_id(uint8_t *RxBuffer)
{
	Bootloader_spi_single(0x5a);
	Bootloader_send_command(0x02);
	if(!boot_ack())
		return 0;
	Bootloader_spi(5, NULL, RxBuffer);
	return boot_ack();
}


uint8_t boot_write_memory(uint32_t address, uint8_t length_minus_1, uint8_t *data)
{
	uint8_t addressNew[4];
	uint8_t checksum = 0;
	uint16_t length;

	Bootloader_spi_single(0x5a);
	Bootloader_send_command(0x31);
	if(!boot_ack())
		return 1;
	HAL_Delay(5);
	addressNew[0] = (uint8_t)((address >> 24) & 0xFF);
	addressNew[1] = (uint8_t)((address >> 16) & 0xFF);
	addressNew[2] = (uint8_t)((address >>  8) & 0xFF);
	addressNew[3] = (uint8_t)((address >>  0) & 0xFF);
	Bootloader_spi(4, addressNew, NULL);
	checksum = 0;
	checksum ^= addressNew[0];
	checksum ^= addressNew[1];
	checksum ^= addressNew[2];
	checksum ^= addressNew[3];
	Bootloader_spi_single(checksum);
	if(!boot_ack())
		return 0;
	HAL_Delay(1);
	Bootloader_spi_single(length_minus_1);
	length = ((uint16_t)length_minus_1) + 1;
	Bootloader_spi(length, data, NULL);
	HAL_Delay(26);
	checksum = 0;
	checksum ^= length_minus_1;
	for(int i=0;i<length;i++)
		checksum ^= data[i];
	Bootloader_spi_single(checksum);
	
	if(!boot_ack())
		return 0;
	HAL_Delay(1);
  return 1;
}

static uint8_t boot_erase_memory(void)
{
	uint8_t special_erase_with_checksum[3] = {0xFF, 0xFF, 0x00};

	Bootloader_spi_single(0x5a);
	Bootloader_send_command(0x44);
	if(!boot_ack())
		return 0;
	Bootloader_spi(3, special_erase_with_checksum, NULL);
	HAL_Delay(11000); /* 5.5 to 11 seconds */
	if(!boot_ack())
		return 0;
  return 1;
}

/* write unprotect does reset the system !! */
uint8_t boot_write_unprotect(void)
{
	Bootloader_spi_single(0x5a);
	Bootloader_send_command(0x73);
	if(!boot_ack())
		return 0;
	return boot_ack();
}

static uint8_t boot_ack(void)
{
	uint8_t answer = 0;

	Bootloader_spi_single(0x00);
	for(int i=0; i< 1000; i++)
	{
		Bootloader_spi(1, NULL, &answer);
		if((answer == 0x79) || (answer == 0x1F))
		{
			Bootloader_spi_single(0x79);
			break;
		}
		HAL_Delay(10);
	}
	if(answer == 0x79)
		return 1;
	else
		return 0;
}

static void	Bootloader_send_command(uint8_t command)
{
	uint8_t send[2];
	uint8_t receive[2];

	send[0] = command;
	send[1] = 0xFF ^ command;
	Bootloader_spi(2, send, receive);
}

static void Bootloader_spi_single(uint8_t TxByte)
{
	Bootloader_spi(1,&TxByte, 0);
}


static void Bootloader_spi(uint16_t lengthData, uint8_t *aTxBuffer, uint8_t *aRxBuffer)
{
	uint8_t dummy[256] = { 0 };
	uint8_t *tx_data;
	uint8_t *rx_data;

	tx_data = aTxBuffer;
	rx_data = aRxBuffer;

	if(aTxBuffer == NULL)
		tx_data = dummy;
	if(aRxBuffer == NULL)
		rx_data = dummy;

	//HAL_GPIO_WritePin(OSCILLOSCOPE_GPIO_PORT,OSCILLOSCOPE_PIN,GPIO_PIN_RESET); // only for testing with Oscilloscope

	
	HAL_SPI_TransmitReceive(&cpu2DmaSpi, (uint8_t *)tx_data, (uint8_t *)rx_data, (uint16_t)lengthData,1000);
/*
	if(HAL_SPI_TransmitReceive_DMA(&cpu2DmaSpi, (uint8_t *)tx_data, (uint8_t *)rx_data, (uint16_t)lengthData) != HAL_OK)
	if(HAL_SPI_TransmitReceive_DMA(&cpu2DmaSpi, (uint8_t *)tx_data, (uint8_t *)rx_data, (uint16_t)lengthData) != HAL_OK)
			Bootloader_Error_Handler();

	while (HAL_SPI_GetState(&cpu2DmaSpi) != HAL_SPI_STATE_READY)// only for testing with Oscilloscope
  {
  }
	HAL_GPIO_WritePin(OSCILLOSCOPE_GPIO_PORT,OSCILLOSCOPE_PIN,GPIO_PIN_SET); // only for testing with Oscilloscope
*/
}


/*
static void Bootloader_Error_Handler(void)
{
	while(1);
}
*/
