/**
  ******************************************************************************
	* @copyright heinrichs weikamp
  * @file    externLogbookFlash.c
  * @author  heinrichs weikamp gmbh
  * @date    07-Aug-2014
  * @version V0.0.4
  * @since   29-Sept-2015
  * @brief   Main File to access the new 1.8 Volt Spansion S25FS256S 256 Mbit (32 Mbyte)
	* @bug
	* @warning
  *
  @verbatim
  ==============================================================================
              ##### Logbook Header (TOC) #####
  ==============================================================================
  [..] Memory useage:
	NEW: Spansion S25FS-S256S
	
			only 8x 4KB and 1x 32KB, remaining is 64KB or 256KB
			Sector Size (kbyte) Sector Count Sector Range Address Range (Byte Address) Notes
			 4   8	SA00  00000000h-00000FFFh 
								:							:
							SA07  00007000h-00007FFFh

			32   1  SA08  00008000h-0000FFFFh

			64 511  SA09  00010000h-0001FFFFh
								: 						:
							SA519 01FF0000h-01FFFFFFh			
	OLD:
				1kB each header
				with predive header at beginning
				and postdive header with 0x400 HEADER2OFFSET
				4kB (one erase) has two dives with 4 headers total
				total of 512 kB (with 256 header ids (8 bit))
				Size is 280 Byte (as of 25.Nov. 2014)

	[..] Output to PC / UART is postdive header

	[..] Block Protection Lock-Down is to erase logbook only

	[..] Timing (see page 137 of LOGBOOK_V3_S25FS-S_00-271247.pdf
				bulk erase is 2 minutes typ., 6 minutes max.

  ==============================================================================
              ##### DEMOMODE #####
  ==============================================================================
	151215: ext_flash_write_settings() is DISABLED!
	
	==============================================================================
              ##### bug fixes #####
  ==============================================================================
	150917:	end in header and length of sample was one byte too long 
					as stated by Jef Driesen email 15.09.2015
					
	@endverbatim
	******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "externLogbookFlash.h"
#include "ostc.h"
#include "settings.h"
#include "gfx_engine.h"

/* Private types -------------------------------------------------------------*/
#define FLASHSTART 	0x000000
//#define FLASHSTOP 	0x01FFFFFF all 32 MB with 4byte addressing
#define FLASHSTOP 	0x00FFFFFF 
//#define FLASHSTOP 	0x3FFFFF
#define RELEASE		1
#define HOLDCS		0

#define HEADER2OFFSET 0x400

typedef struct{
uint8_t IsBusy:1;
uint8_t IsWriteEnabled:1;
uint8_t BlockProtect0:1;
uint8_t BlockProtect1:1;
uint8_t BlockProtect2:1;
uint8_t BlockProtect3:1;
uint8_t IsAutoAddressIncMode:1;
uint8_t BlockProtectL:1;
} extFlashStatusUbit8_t;

typedef union{
extFlashStatusUbit8_t ub;
uint8_t uw;
} extFlashStatusBit8_Type;


/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static uint32_t	actualAddress = 0;
static uint32_t	preparedPageAddress = 0;
static uint32_t closeSectorAddress = 0;
static uint32_t	actualPointerHeader = 0;
static uint32_t	actualPointerSample = 0;
static uint32_t	actualPointerDevicedata = DDSTART;
static uint32_t	actualPointerVPM = 0;
static uint32_t	actualPointerSettings = SETTINGSSTART;
static uint32_t	actualPointerFirmware = 0;
static uint32_t	actualPointerFirmware2 = 0;

/* Private function prototypes -----------------------------------------------*/
static void chip_unselect(void);
static void chip_select(void);
static void write_spi(uint8_t data, uint8_t unselect_CS_afterwards);
static uint8_t read_spi(uint8_t unselect_CS_afterwards);
static void write_address(uint8_t unselect_CS_afterwards);
static void Error_Handler_extflash(void);
static void wait_chip_not_busy(void);
static void ext_flash_incf_address(uint8_t type);
//void ext_flash_incf_address_ring(void);

static void ext_flash_erase4kB(void);
static void ext_flash_erase32kB(void);
static void ext_flash_erase64kB(void);
static uint8_t ext_flash_erase_if_on_page_start(void);

static void ef_write_block(uint8_t * sendByte, uint32_t length, uint8_t type, uint8_t do_not_erase);

static void ext_flash_read_block(uint8_t *getByte, uint8_t type);
static void ext_flash_read_block_multi(void *getByte, uint32_t size, uint8_t type);
static void ext_flash_read_block_stop(void);

static void ef_hw_rough_delay_us(uint32_t delayUs);
static void ef_erase_64K(uint32_t blocks);


/* Exported functions --------------------------------------------------------*/

void ext_flash_write_firmware(uint8_t *pSample1, uint32_t length1)//, uint8_t *pSample2, uint32_t length2)
{
	general32to8_Type lengthTransform;

	lengthTransform.u32 = length1;
	
	actualPointerFirmware = FWSTART;
	ef_write_block(lengthTransform.u8,4, EF_FIRMWARE, 1);
	ef_write_block(pSample1,length1, EF_FIRMWARE, 1);

//	if(length2)
//		ef_write_block(pSample2,length2, EF_FIRMWARE, 1);
}

uint8_t ext_flash_read_firmware_version(char *text)
{
	uint32_t backup = actualAddress;
	uint8_t buffer[4];
	
	// + 4 for length data, see ext_flash_write_firmware
	actualAddress = FWSTART + 4 + 0x10000;
	ext_flash_read_block_start();
	ext_flash_read_block(&buffer[0], EF_FIRMWARE);
	ext_flash_read_block(&buffer[1], EF_FIRMWARE);
	ext_flash_read_block(&buffer[2], EF_FIRMWARE);
	ext_flash_read_block(&buffer[3], EF_FIRMWARE);

	ext_flash_read_block_stop();
	actualAddress = backup;

	uint8_t ptr = 0;
	text[ptr++] = 'V';
	ptr += gfx_number_to_string(2,0,&text[ptr],buffer[0] & 0x3F);
	text[ptr++] = '.';
	ptr += gfx_number_to_string(2,0,&text[ptr],buffer[1] & 0x3F);
	text[ptr++] = '.';
	ptr += gfx_number_to_string(2,0,&text[ptr],buffer[2] & 0x3F);
	text[ptr++] = ' ';
	if(buffer[3])
	{
		text[ptr++] = 'b';
		text[ptr++] = 'e';
		text[ptr++] = 't';
		text[ptr++] = 'a';
		text[ptr++] = ' ';
	}
	return ptr;
}


uint32_t ext_flash_read_firmware(uint8_t *pSample1, uint32_t max_length, uint8_t *magicByte)
{
	uint32_t backup = actualAddress;
	general32to8_Type lengthTransform;
	
	actualAddress = FWSTART;
	ext_flash_read_block_start();

	ext_flash_read_block(&lengthTransform.u8[0], EF_FIRMWARE);
	ext_flash_read_block(&lengthTransform.u8[1], EF_FIRMWARE);
	ext_flash_read_block(&lengthTransform.u8[2], EF_FIRMWARE);
	ext_flash_read_block(&lengthTransform.u8[3], EF_FIRMWARE);

	
	if(lengthTransform.u32 == 0xFFFFFFFF)
	{
		lengthTransform.u32 = 0xFFFFFFFF;
	}
	else
	if(lengthTransform.u32 > max_length)
	{
		lengthTransform.u32 = 0xFF000000;
	}
	else
	{
		for(uint32_t i = 0; i<lengthTransform.u32; i++)
		{
			ext_flash_read_block(&pSample1[i], EF_FIRMWARE);
		}
		
	}
	
	ext_flash_read_block_stop();
	
	if(magicByte)
	{
		*magicByte = pSample1[0x10000 + 0x3E]; // 0x3E == 62 
	}
	
	actualAddress = backup;
	return lengthTransform.u32;
}


void ext_flash_write_firmware2(uint32_t offset, uint8_t *pSample1, uint32_t length1, uint8_t *pSample2, uint32_t length2)
{
	general32to8_Type lengthTransform, offsetTransform;

	lengthTransform.u32 = length1 + length2;
	offsetTransform.u32 = offset;
	
	actualPointerFirmware2 = FWSTART2;
	ef_write_block(lengthTransform.u8,4, EF_FIRMWARE2, 1);
	ef_write_block(offsetTransform.u8,4, EF_FIRMWARE2, 1);
	ef_write_block(pSample1,length1, EF_FIRMWARE2, 1);
	if(length2)
		ef_write_block(pSample2,length2, EF_FIRMWARE2, 1);
}


uint32_t ext_flash_read_firmware2(uint32_t *offset, uint8_t *pSample1, uint32_t max_length1, uint8_t *pSample2, uint32_t max_length2)
{
	uint32_t backup = actualAddress;
	uint32_t length1, length2;
	general32to8_Type lengthTransform, offsetTransform;

	actualAddress = FWSTART2;
	ext_flash_read_block_start();

	ext_flash_read_block(&lengthTransform.u8[0], EF_FIRMWARE2);
	ext_flash_read_block(&lengthTransform.u8[1], EF_FIRMWARE2);
	ext_flash_read_block(&lengthTransform.u8[2], EF_FIRMWARE2);
	ext_flash_read_block(&lengthTransform.u8[3], EF_FIRMWARE2);

	ext_flash_read_block(&offsetTransform.u8[0], EF_FIRMWARE2);
	ext_flash_read_block(&offsetTransform.u8[1], EF_FIRMWARE2);
	ext_flash_read_block(&offsetTransform.u8[2], EF_FIRMWARE2);
	ext_flash_read_block(&offsetTransform.u8[3], EF_FIRMWARE2);
	
	*offset = offsetTransform.u32;
	
	if(lengthTransform.u32 == 0xFFFFFFFF)
	{
		lengthTransform.u32 = 0xFFFFFFFF;
	}
	else
	if(lengthTransform.u32 > max_length1 + max_length2)
	{
		lengthTransform.u32 = 0xFF000000;
	}
	else
	{
		if(lengthTransform.u32 <  max_length1)
		{
			length1 = lengthTransform.u32;
			length2 = 0;
		}
		else
		{
			length1 = max_length1;
			length2 = lengthTransform.u32 - max_length1;
		}
		
		if(pSample1)
		{
			for(uint32_t i = 0; i<length1; i++)
			{
				ext_flash_read_block(&pSample1[i], EF_FIRMWARE2);
			}
			if(pSample2)
			{
				for(uint32_t i = 0; i<length2; i++)
				{
					ext_flash_read_block(&pSample2[i], EF_FIRMWARE2);
				}
			}
		}
		else if(pSample2)
		{
			/* actualAddress += length1; do dummy read to get EEPROM to the correct address */
			for(uint32_t i = 0; i<length1; i++)
			{
				ext_flash_read_block(&pSample2[0], EF_FIRMWARE2);
			}
			for(uint32_t i = 0; i<length2; i++)
			{
				ext_flash_read_block(&pSample2[i], EF_FIRMWARE2);
			}
		}
	}
	ext_flash_read_block_stop();
	actualAddress = backup;
	return lengthTransform.u32;
}


void ext_flash_read_fixed_16_devicedata_blocks_formated_128byte_total(uint8_t *buffer)
{
	SDeviceLine data[16];
	uint8_t tempLengthIngnore;
	uint16_t count;
	uint8_t transfer;

	RTC_DateTypeDef Sdate;
	RTC_TimeTypeDef Stime;
	
	actualAddress = DDSTART;

	ext_flash_read_block_start();
	ext_flash_read_block(&tempLengthIngnore, EF_DEVICEDATA);
	ext_flash_read_block(&tempLengthIngnore, EF_DEVICEDATA);
	
	ext_flash_read_block_multi((uint8_t *)data,16*3*4, EF_DEVICEDATA);
	ext_flash_read_block_stop();

	count = 0;
	for(int i=0;i<16;i++)
	{
		transfer = (data[i].value_int32 >> 24) & 0xFF;
		buffer[count++] = transfer;
		transfer = (data[i].value_int32 >> 16) & 0xFF;
		buffer[count++] = transfer;
		transfer = (data[i].value_int32 >> 8) & 0xFF;
		buffer[count++] = transfer;
		transfer = (data[i].value_int32) & 0xFF;
		buffer[count++] = transfer;

		translateDate(data[i].date_rtc_dr, &Sdate);
		translateTime(data[i].time_rtc_tr, &Stime);
		buffer[count++] = Sdate.Year;
		buffer[count++] = Sdate.Month;
		buffer[count++] = Sdate.Date;
		buffer[count++] = Stime.Hours;
	}
}

void ext_flash_erase_firmware(void)
{
	uint32_t size, blocks_64k;

	actualAddress = FWSTART;
	size = 1 + FWSTOP - FWSTART;
	blocks_64k = size / 0x10000;
	ef_erase_64K(blocks_64k);
}

void ext_flash_erase_firmware2(void)
{
	uint32_t size, blocks_64k;

	actualAddress = FWSTART2;
	size = 1 + FWSTOP2 - FWSTART2;
	blocks_64k = size / 0x10000;
	ef_erase_64K(blocks_64k);
}



static void ext_flash_erase4kB(void)
{
	wait_chip_not_busy();
	write_spi(0x06,RELEASE);/* WREN */
	write_spi(0x20,HOLDCS);/* sector erase cmd */
	write_address(RELEASE);
}

/* be careful - might not work with entire family and other products
 * see page 14 of LOGBOOK_V3_S25FS-S_00-271247.pdf
 */
static void ext_flash_erase32kB(void)
{
	uint32_t actualAddress_backup;
	
	actualAddress_backup = actualAddress;
	actualAddress = 0;
	wait_chip_not_busy();
	write_spi(0x06,RELEASE);/* WREN */
	write_spi(0xD8,HOLDCS);/* sector erase cmd */
	write_address(RELEASE);
	actualAddress = actualAddress_backup;
}


static void ext_flash_erase64kB(void)
{
	wait_chip_not_busy();
	write_spi(0x06,RELEASE);/* WREN */
	write_spi(0xD8,HOLDCS);/* sector erase cmd */
	write_address(RELEASE);
}


void ext_flash_read_block_start(void)
{
	wait_chip_not_busy();
	write_spi(0x03,HOLDCS);		/* WREN */
	write_address(HOLDCS);
}

/* 4KB, 32KB, 64 KB, not the upper 16 MB with 4 Byte address at the moment */
static uint8_t ext_flash_erase_if_on_page_start(void)
{
	if(actualAddress < 0x00008000)
	{
		/* 4K Byte is 0x1000 */
		if((actualAddress & 0xFFF) == 0)
		{
			ext_flash_erase4kB();
			return 1;
		}
	}
	else
	if(actualAddress < 0x00010000)
	{
		/* 32K Byte is only one page */
		if(actualAddress == 0x00010000)
		{
			ext_flash_erase32kB();
			return 1;
		}
	}
	else
	{
		/* 64K Byte is 0x10000 */
		if((actualAddress & 0xFFFF) == 0)
		{
			if(preparedPageAddress == actualAddress)	/* has page already been prepared before? (at the moment for samples only) */
			{
				preparedPageAddress = 0;

			}
			else
			{
				ext_flash_erase64kB();
			}
			return 1;
		}
	}

	return 0;
}


static void ext_flash_read_block(uint8_t *getByte, uint8_t type)
{
	*getByte = read_spi(HOLDCS);/* read data */
	ext_flash_incf_address(type);
}


static void ext_flash_read_block_multi(void *getByte, uint32_t size, uint8_t type)
{
	uint8_t  *data;
	data = getByte;

	for(uint32_t i=0;i<size;i++)
	{
		data[i] = read_spi(HOLDCS);/* read data */
		ext_flash_incf_address(type);
	}
}


static void ext_flash_read_block_stop(void)
{
	chip_unselect();
}


/* Private functions ---------------------------------------------------------*/

static void ef_write_block(uint8_t * sendByte, uint32_t length, uint8_t type, uint8_t do_not_erase)
{
	uint32_t remaining_page_size, remaining_length, remaining_space_to_ring_end;
	uint32_t i=0;

	if(!length)
		return;

	uint32_t ringStart, ringStop;

	switch(type)
	{
		case EF_HEADER:
			actualAddress = actualPointerHeader;
			ringStart = HEADERSTART;
			ringStop = HEADERSTOP;
			break;
		case EF_SAMPLE:
			actualAddress = actualPointerSample;
			ringStart = SAMPLESTART;
			ringStop = SAMPLESTOP;
			break;
		case EF_DEVICEDATA:
			actualAddress = actualPointerDevicedata;
			ringStart = DDSTART;
			ringStop = DDSTOP;
			break;
		case EF_VPMDATA:
			actualAddress = actualPointerVPM;
			ringStart = VPMSTART;
			ringStop = VPMSTOP;
			break;
		case EF_SETTINGS:
			actualAddress = actualPointerSettings;
			ringStart = SETTINGSSTART;
			ringStop = SETTINGSSTOP;
			break;
		case EF_FIRMWARE:
			actualAddress = actualPointerFirmware;
			ringStart = FWSTART;
			ringStop = FWSTOP;
			break;
		case EF_FIRMWARE2:
			actualAddress = actualPointerFirmware2;
			ringStart = FWSTART2;
			ringStop = FWSTOP2;
			break;
		default:
			ringStart = FLASHSTART;
			ringStop = FLASHSTOP;
			break;
	}
	/* safety */
	if(actualAddress < ringStart)
		actualAddress = ringStart;

	if(do_not_erase == 0)
	{
		ext_flash_erase_if_on_page_start();
	}
	
	while( i<length)
	{
		ef_hw_rough_delay_us(5);
		wait_chip_not_busy();
		write_spi(0x06,RELEASE);		/* WREN */
		write_spi(0x02,HOLDCS);			/* write cmd */
		write_address(HOLDCS);
		
		remaining_length = length - i;
		remaining_page_size = 0xFF - (uint8_t)(actualAddress & 0xFF) +1;
		remaining_space_to_ring_end = ringStop - actualAddress;
		
		if(remaining_length >= 256)
		{
			remaining_length = 255;	/* up to 256 bytes may be written in one burst. Last byte is written with release */
		}
		else
		{
			remaining_length--;		/* last byte needed for release */
		}
		if(remaining_length >= (remaining_page_size) ) /* use 256 byte page and calculate number of bytes left */
		{
			remaining_length = remaining_page_size - 1;
		}
		if( (remaining_space_to_ring_end >= 256)) 
		{
			for(int j=0; j<remaining_length; j++)
			{
				write_spi(sendByte[i],HOLDCS);/* write data */
				actualAddress++;
				i++;
			}
		}
		/* byte with RELEASE */
		write_spi(sendByte[i],RELEASE);/* write data */
		actualAddress++;
		i++;

		if(actualAddress > ringStop)
			actualAddress = ringStart;

		if(do_not_erase == 0)
			ext_flash_erase_if_on_page_start();
	}
	switch(type)
	{
		case EF_HEADER:
			actualPointerHeader = actualAddress;
			break;
		case EF_SAMPLE:
			actualPointerSample = actualAddress;
			break;
		case EF_DEVICEDATA:
			actualPointerDevicedata = actualAddress;
			break;
		case EF_VPMDATA:
			actualPointerVPM = actualAddress;
			break;
		case EF_SETTINGS:
			actualPointerSettings = actualAddress;
			break;
		case EF_FIRMWARE:
			actualPointerFirmware = actualAddress;
			break;
		case EF_FIRMWARE2:
			actualPointerFirmware2 = actualAddress;
			break;
		default:
			break;
	}
}


static void ef_erase_64K(uint32_t blocks)
{
	for(uint32_t i = 0; i < blocks; i++)
	{
		wait_chip_not_busy();
		write_spi(0x06,RELEASE);/* WREN */
		write_spi(0xD8,HOLDCS);/* 64k erase cmd */
		write_address(RELEASE);
		actualAddress += 0x10000;
		HAL_Delay(25);
	}
}

static void chip_unselect(void)
{
		HAL_GPIO_WritePin(EXTFLASH_CSB_GPIO_PORT,EXTFLASH_CSB_PIN,GPIO_PIN_SET); // chip select
}

static void chip_select(void)
{
	HAL_GPIO_WritePin(EXTFLASH_CSB_GPIO_PORT,EXTFLASH_CSB_PIN,GPIO_PIN_RESET); // chip select
}

static uint8_t read_spi(uint8_t unselect_CS_afterwards)
{
	uint8_t byte;

	chip_select();

	if(HAL_SPI_Receive(&hspiDisplay, &byte, 1, 10000) != HAL_OK)
		Error_Handler_extflash();

	while (HAL_SPI_GetState(&hspiDisplay) != HAL_SPI_STATE_READY)
  {
  }
	if(unselect_CS_afterwards)
		chip_unselect();

	return byte;
}


static void write_spi(uint8_t data, uint8_t unselect_CS_afterwards)
{
	chip_select();

	if(HAL_SPI_Transmit(&hspiDisplay, &data, 1, 10000) != HAL_OK)
		Error_Handler_extflash();

	while (HAL_SPI_GetState(&hspiDisplay) != HAL_SPI_STATE_READY)
  {
  }
	if(unselect_CS_afterwards)
		chip_unselect();
}


static void write_address(uint8_t unselect_CS_afterwards)
{
	uint8_t hi, med ,lo;

	hi = (actualAddress >> 16) & 0xFF;
	med = (actualAddress >> 8) & 0xFF;
	lo = actualAddress & 0xFF;

	write_spi(hi, HOLDCS);
	write_spi(med, HOLDCS);
	write_spi(lo, unselect_CS_afterwards);
}


static void wait_chip_not_busy(void)
{
	uint8_t status;

	chip_unselect();

	write_spi(0x05,HOLDCS);		/* RDSR */
	status = read_spi(HOLDCS);/* read status */
	while(status & 0x01)
	{
		HAL_Delay(1);
		status = read_spi(HOLDCS);/* read status */
	}
	chip_unselect();
}


static void ext_flash_incf_address(uint8_t type)
{
	uint32_t ringStart, ringStop;
	
	actualAddress += 1;
	
	switch(type)
	{
		case EF_HEADER:
			ringStart = HEADERSTART;
			ringStop = HEADERSTOP;
			break;
		case EF_SAMPLE:
			ringStart = SAMPLESTART;
			ringStop = SAMPLESTOP;
			break;
		case EF_DEVICEDATA:
			ringStart = DDSTART;
			ringStop = DDSTOP;
			break;
		case EF_VPMDATA:
			ringStart = VPMSTART;
			ringStop = VPMSTOP;
			break;
		case EF_SETTINGS:
			ringStart = SETTINGSSTART;
			ringStop = SETTINGSSTOP;
			break;
		case EF_FIRMWARE:
			ringStart = FWSTART;
			ringStop = FWSTOP;
			break;
		case EF_FIRMWARE2:
			ringStart = FWSTART2;
			ringStop = FWSTOP2;
			break;
		default:
			ringStart = FLASHSTART;
			ringStop = FLASHSTOP;
			break;
	}
	
	if((actualAddress < ringStart) || (actualAddress > ringStop))
		actualAddress = ringStart;
}

static void ef_hw_rough_delay_us(uint32_t delayUs)
{
	if(!delayUs)
		return;
	delayUs*= 12;
	while(delayUs--);
	return;
}

static void Error_Handler_extflash(void)
{
  while(1)
  {
  }
}

void ext_flash_CloseSector(void)
{
	uint32_t actualAddressBackup = actualAddress;
	int i=0;

	if(closeSectorAddress != 0)
	{
	/* write some dummy bytes to the sector which is currently used for storing samples. This is done to "hide" problem if function is calles again */
		actualAddress = closeSectorAddress;

		wait_chip_not_busy();
		write_spi(0x06,RELEASE);		/* WREN */
		write_spi(0x02,HOLDCS);			/* write cmd */
		write_address(HOLDCS);
		for(i = 0; i<8; i++)
		{
			write_spi(0xA5,HOLDCS);/* write data */
			actualAddress++;
		}
		/* byte with RELEASE */
		write_spi(0xA5,RELEASE);/* write data */
		actualAddress = actualAddressBackup;
		closeSectorAddress = 0;
	}
}


uint8_t ext_flash_erase_firmware_if_not_empty(void)
{
	const uint8_t TESTSIZE_FW = 4;
	
	uint8_t data[TESTSIZE_FW];
	uint8_t notEmpty = 0;
	
	actualAddress = FWSTART;
	ext_flash_read_block_start();
	for(int i = 0; i < TESTSIZE_FW; i++)
	{
		ext_flash_read_block(&data[i], EF_FIRMWARE);
		if(data[i] != 0xFF)
			notEmpty = 1;
	}
	ext_flash_read_block_stop();
	
	if(notEmpty)
	{
		ext_flash_erase_firmware();
		return 1;
	}
	else
		return 0;
}

uint8_t ext_flash_erase_firmware2_if_not_empty(void)
{
	const uint8_t TESTSIZE_FW = 4;
	
	uint8_t data[TESTSIZE_FW];
	uint8_t notEmpty = 0;
	
	actualAddress = FWSTART2;
	ext_flash_read_block_start();
	for(int i = 0; i < TESTSIZE_FW; i++)
	{
		ext_flash_read_block(&data[i], EF_FIRMWARE2);
		if(data[i] != 0xFF)
			notEmpty = 1;
	}
	ext_flash_read_block_stop();
	
	if(notEmpty)
	{
		ext_flash_erase_firmware2();
		return 1;
	}
	else
		return 0;
}
