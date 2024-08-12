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

#ifndef BOOTLOADER_STANDALONE
#include "logbook.h"
#endif

/* Private types -------------------------------------------------------------*/
#define FLASHSTART 	0x000000
//#define FLASHSTOP 	0x01FFFFFF all 32 MB with 4byte addressing
#define FLASHSTOP 	0x00FFFFFF 
//#define FLASHSTOP 	0x3FFFFF
#define RELEASE		1
#define HOLDCS		0

#define HEADER2OFFSET 0x400

typedef enum{
	EF_HEADER,
	EF_SAMPLE,
	EF_DEVICEDATA,
	EF_VPMDATA,
	EF_SETTINGS,
	EF_FIRMWARE,
	EF_FIRMWARE2,
}which_ring_enum;


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

#ifndef BOOTLOADER_STANDALONE
static uint32_t	entryPoint = 0;
static uint32_t	LengthLeftSampleRead = 0;
static uint32_t	actualPointerDevicedata_Read = DDSTART;
#endif

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
#ifndef BOOTLOADER_STANDALONE
static void error_led_on(void);
static void error_led_off(void);
static void ext_flash_decf_address_ring(uint8_t type);
static void ext_flash_disable_protection(void);
static _Bool ext_flash_test_remaining_space_of_page_empty(uint32_t pointer, uint16_t length);
static void ext_flash_set_to_begin_of_next_page(uint32_t *pointer, uint8_t type);
#endif
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

#ifndef BOOTLOADER_STANDALONE
static void ext_flash_overwrite_sample_without_erase(uint8_t *pSample, uint16_t length);
static void ext_flash_find_start(void);
#endif



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
			actualAddress += length1;
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


#ifndef BOOTLOADER_STANDALONE

void ext_flash_write_devicedata(uint8_t resetRing)
{
	uint8_t *pData;
	const uint16_t length = sizeof(SDevice);
	uint8_t length_lo, length_hi;
	uint8_t dataLength[2] = { 0 };
	uint32_t tmpBlockStart;

	ext_flash_disable_protection();

	pData = (uint8_t *)stateDeviceGetPointer();

	/* Reset the Ring to the start address if requested (e.g. because we write the default block during shutdown) */
	if((resetRing) || ((actualPointerDevicedata + length) >= DDSTOP))
	{
		actualPointerDevicedata = DDSTART;
	}
	tmpBlockStart = actualPointerDevicedata;

	length_lo = (uint8_t)(length & 0xFF);
	length_hi = (uint8_t)(length >> 8);
	dataLength[0] = length_lo;
	dataLength[1] = length_hi;

	ef_write_block(dataLength,2, EF_DEVICEDATA, 0);
	ef_write_block(pData,length, EF_DEVICEDATA, 0);

	actualPointerDevicedata_Read = tmpBlockStart;

}


uint16_t ext_flash_read_devicedata(uint8_t *buffer, uint16_t max_length)
{
	uint16_t length;
	uint8_t length_lo, length_hi;

	actualAddress = actualPointerDevicedata_Read; 

	length = 0;
	length_lo = 0;
	length_hi = 0;
	ext_flash_read_block_start();


	ext_flash_read_block(&length_lo, EF_DEVICEDATA);
	ext_flash_read_block(&length_hi, EF_DEVICEDATA);
	
	while ((length_lo != 0xFF) && (length_hi != 0xFF))
	{
		length = (length_hi * 256) + length_lo;

		if(length > max_length)
			return 0;

		ext_flash_read_block_multi(buffer,length,EF_DEVICEDATA);

		ext_flash_read_block(&length_lo, EF_DEVICEDATA);	/* check if another devicedata set is available */
		ext_flash_read_block(&length_hi, EF_DEVICEDATA);	/* length will be 0xFFFF if a empty memory is read */
	}
	ext_flash_decf_address_ring(EF_DEVICEDATA);				/* set pointer back to empty address */
	ext_flash_decf_address_ring(EF_DEVICEDATA);
	ext_flash_read_block_stop();

	if(actualAddress > actualPointerDevicedata)				/* the write pointer has not yet been set up probably */
	{
		actualPointerDevicedata = actualAddress;
	}
	return length;
}


void ext_flash_write_vpm(SVpm *vpmInput)
{
	uint8_t *pData;
	const uint16_t length = sizeof(SVpm);

	uint8_t length_lo, length_hi;
	uint8_t dataLength[2] = { 0 };
	
	pData = (uint8_t *)vpmInput;

	actualPointerVPM = VPMSTART;

	length_lo = (uint8_t)(length & 0xFF);
	length_hi = (uint8_t)(length >> 8);
	dataLength[0] = length_lo;
	dataLength[1] = length_hi;

	ef_write_block(dataLength,2, EF_VPMDATA, 0);
	ef_write_block(pData,length, EF_VPMDATA, 0);
}


int ext_flash_read_vpm(SVpm *vpmOutput)
{
	uint8_t *pData;
	const uint16_t length = sizeof(SVpm);
	uint8_t length_lo, length_hi;
	int output;
	
	actualAddress = VPMSTART;

	ext_flash_read_block_start();
	ext_flash_read_block(&length_lo, EF_VPMDATA);
	ext_flash_read_block(&length_hi, EF_VPMDATA);
	if((length_lo == (uint8_t)(length & 0xFF))
		&&(length_hi == (uint8_t)(length >> 8)))
	{
		pData = (uint8_t *)vpmOutput;
		for(uint16_t i = 0; i < length; i++)
			ext_flash_read_block(&pData[i], EF_VPMDATA);
		output = length;
	}
	else
		output = 0;
	
	ext_flash_read_block_stop();
	return output;
}

#ifdef DEMOMODE
void ext_flash_write_settings(void)
{
	return;
}
#else
void ext_flash_write_settings(uint8_t resetRing)
{
	uint8_t *pData;
	const uint16_t length = sizeof(SSettings);
	uint8_t length_lo, length_hi;
	uint8_t dataLength[2] = { 0 };

	ext_flash_disable_protection();

	if(stateRealGetPointer()->lastKnownBatteryPercentage)
	{
		settingsGetPointer()->lastKnownBatteryPercentage = stateRealGetPointer()->lastKnownBatteryPercentage;
	}
	settingsGetPointer()->backup_localtime_rtc_tr = stateRealGetPointer()->lifeData.timeBinaryFormat;
	settingsGetPointer()->backup_localtime_rtc_dr = stateRealGetPointer()->lifeData.dateBinaryFormat;

	pData = (uint8_t *)settingsGetPointer();

	/* Reset the Ring to the start address if requested (e.g. because we write the default block during shutdown) */
	if((resetRing) || ((actualPointerSettings + length) >= SETTINGSSTOP))
	{
		actualPointerSettings = SETTINGSSTART;
	}

	length_lo = (uint8_t)(length & 0xFF);
	length_hi = (uint8_t)(length >> 8);
	dataLength[0] = length_lo;
	dataLength[1] = length_hi;

	ef_write_block(dataLength,2, EF_SETTINGS, 0);
	ef_write_block(pData,length, EF_SETTINGS, 0);
//	ext_flash_enable_protection();
}
#endif


/* CHANGES 150929 hw 
 * this now allows to read old settings too
 * but make sure that everything is fixed in 
 * set_new_settings_missing_in_ext_flash
 * new settings should be fine as they are added
 * and loaded before calling this function
 */
uint8_t ext_flash_read_settings(void)
{
	uint8_t returnValue = HAL_BUSY;
	uint8_t exit = 0;
	uint8_t *pData;
	const uint16_t lengthStandardNow = sizeof(SSettings);
	uint8_t length_lo, length_hi;
	uint16_t lengthOnEEPROM;
	uint32_t header;
	SSettings *pSettings = settingsGetPointer();

	actualAddress = SETTINGSSTART;

	ext_flash_read_block_start();
	ext_flash_read_block(&length_lo, EF_SETTINGS);
	ext_flash_read_block(&length_hi, EF_SETTINGS);
	
	while ((length_lo != 0xFF) && (length_hi != 0xFF) && (exit == 0))		/* get the latest stored setting block */
	{
		lengthOnEEPROM = length_hi * 256;
		lengthOnEEPROM += length_lo;
		if(lengthOnEEPROM <= lengthStandardNow) 			/* EEPROM Header size equal or smaller => settings constant or upgraded */
		{
			ext_flash_read_block_multi(&header, 4, EF_SETTINGS);
			if((header <= pSettings->header) && (header >= pSettings->updateSettingsAllowedFromHeader))	/* check to allow update of header */
			{
				returnValue = HAL_OK;
				pSettings->header = header;
				pData = (uint8_t *)pSettings + 4; /* header */
				for(uint16_t i = 0; i < (lengthOnEEPROM-4); i++)
					ext_flash_read_block(&pData[i], EF_SETTINGS);
				if(header != pSettings->header)				/* setting layout changed => no additional setting sets expected */
				{
					exit = 1;
				}
			}
			else
			{
				returnValue = HAL_ERROR;
				exit = 1;
			}
		}
		else											/* size of settings decreased => possible downgrade of firmware */
		{
			ext_flash_read_block_multi(&header, 4, EF_SETTINGS);

			if(header > 0xFFFF0014)						/* verify that new (old) header should be compatible (only less bytes, no change in layout) */
			{
				returnValue = HAL_OK;
				pSettings->header = header;
				pData = (uint8_t *)pSettings + 4; /* header */
				for(uint16_t i = 0; i < (lengthStandardNow-4); i++) 		/* only read the data fitting into the structure */
					ext_flash_read_block(&pData[i], EF_SETTINGS);
			}
			else
			{
				returnValue = HAL_ERROR;
			}
			exit = 1;
		}
		ext_flash_read_block(&length_lo, EF_SETTINGS);
		ext_flash_read_block(&length_hi, EF_SETTINGS);
	}
	ext_flash_decf_address_ring(EF_SETTINGS);				/* set pointer back to empty address */
	ext_flash_decf_address_ring(EF_SETTINGS);
	ext_flash_read_block_stop();

	if(actualAddress > actualPointerSettings)				/* the write pointer has not yet been set up probably */
	{
		actualPointerSettings = actualAddress;
	}
	ext_flash_read_block_stop();
	return returnValue;
}




	/* ext_flash_start_new_dive_log_and_set_actualPointerSample
	 * prepares the write sample pointer
	 * to be used by ext_flash_write_sample()
	 * to be set in the * pHeaderPreDive
	 * for write with ext_flash_create_new_dive_log() and ext_flash_close_new_dive_log()
	 */
void ext_flash_start_new_dive_log_and_set_actualPointerSample(uint8_t *pHeaderPreDive)
{
	convert_Type data;
	SSettings *settings = settingsGetPointer();

	/* new 5. Jan. 2015 */
	actualPointerSample = settings->logFlashNextSampleStartAddress;

	if(!ext_flash_test_remaining_space_of_page_empty(actualPointerSample, 4))
		ext_flash_set_to_begin_of_next_page(&actualPointerSample, EF_SAMPLE);

	if((actualPointerSample < SAMPLESTART) || (actualPointerSample > SAMPLESTOP))
		actualPointerSample = SAMPLESTART;

	data.u32bit = actualPointerSample;
	pHeaderPreDive[2] = data.u8bit.byteLow;
	pHeaderPreDive[3] = data.u8bit.byteMidLow;
	pHeaderPreDive[4] = data.u8bit.byteMidHigh;
	/* to start sample writing and header etc. pp. */
	ext_flash_disable_protection_for_logbook();
}


	/* ext_flash_create_new_dive_log
	 * uses the first header without HEADER2OFFSET
	 * for the header it is not important to be complete
	 * and can be reconstructed
	 * ext_flash_start_new_dive_log_and_set_actualPointerSample()
	 * has to be called before to set the actualPointerSample
	 * in the header
	 * the following func writes to header to the ext_flash
	 */
void ext_flash_create_new_dive_log(uint8_t *pHeaderPreDive)
{
	SSettings *settings;
	uint8_t id;
	uint8_t  header1, header2;

	settings = settingsGetPointer();
	id = settings->lastDiveLogId;

	actualAddress = HEADERSTART + (0x800 * id);
	ext_flash_read_block_start();
	ext_flash_read_block(&header1, EF_SAMPLE); /* the sample ring is increased instead of header... not sure if that is planned intention */
	ext_flash_read_block(&header2, EF_SAMPLE); /* does not matter because actual address is reset in write_block call */
	ext_flash_read_block_stop();

	/* TODO Cleanup_Ref_2: The code below should not be necessary in case of a proper shutdown and startup */
	/* the implementation fixes an issue which might happen at Cleanup_Ref_1 (in case of more then 254 dives) */
	if((header1 == 0xFA) && (header2 == 0xFA))
	{
		id += 1; /* 0-255, auto rollover */
		if(id & 1)
		{
			actualAddress = HEADERSTART + (0x800 * id);
			ext_flash_read_block_start();
			ext_flash_read_block(&header1, EF_SAMPLE);
			ext_flash_read_block(&header2, EF_SAMPLE);
			ext_flash_read_block_stop();
			if((header1 == 0xFA) && (header2 == 0xFA))
				id += 1;
		}
	}
	else
	{
		id = 0;
	}

	settings->lastDiveLogId = id;
	actualPointerHeader = HEADERSTART + (0x800 * id);

	if(pHeaderPreDive != 0)
		ef_write_block(pHeaderPreDive,HEADERSIZE, EF_HEADER, 0);
}


void ext_flash_close_new_dive_log(uint8_t *pHeaderPostDive )
{
	SSettings *	settings = settingsGetPointer();
	uint8_t id;
	convert_Type startAddress;
	convert_Type data;
	uint32_t backup;

	uint8_t	sampleData[3];
  actualAddress = actualPointerSample;
	sampleData[0] = 0xFD;
	sampleData[1] = 0xFD;
  ext_flash_write_sample(sampleData, 2);
	
	/* end of sample data, pointing to the last sample 0xFD
	*/
  actualAddress = actualPointerSample; // change hw 17.09.2015
	ext_flash_decf_address_ring(EF_SAMPLE); // 17.09.2015: this decf actualAddress only!!
	actualPointerSample = actualAddress; // change hw 17.09.2015
	data.u32bit = actualPointerSample;
	
	pHeaderPostDive[5] = data.u8bit.byteLow;
	pHeaderPostDive[6] = data.u8bit.byteMidLow;
	pHeaderPostDive[7] = data.u8bit.byteMidHigh;

	/* take data written before, calculate length and write
		SLogbookHeader has different order: length (byte# 8,9,10) prior to profile version (byte# 11)
	*/
	startAddress.u8bit.byteLow 			= pHeaderPostDive[2];
	startAddress.u8bit.byteMidLow 	= pHeaderPostDive[3];
	startAddress.u8bit.byteMidHigh 	= pHeaderPostDive[4];
	startAddress.u8bit.byteHigh = 0;

	if(startAddress.u32bit < actualPointerSample)
		data.u32bit = 1 + actualPointerSample - startAddress.u32bit;
	else
		data.u32bit = 2 + (actualPointerSample - SAMPLESTART) + (SAMPLESTOP - startAddress.u32bit);

	pHeaderPostDive[8] = data.u8bit.byteLow;
	pHeaderPostDive[9] = data.u8bit.byteMidLow;
	pHeaderPostDive[10] = data.u8bit.byteMidHigh;

	/* set id and write post-dive-header
	*/
	id = settings->lastDiveLogId;
	actualPointerHeader = HEADERSTART + (0x800 * id) + HEADER2OFFSET;

	ef_write_block(pHeaderPostDive,HEADERSIZE, EF_HEADER, 0);

	/* write length at beginning of sample
		and write proper beginning for next dive to actualPointerSample
	*/
	backup = actualPointerSample;
	actualPointerSample = startAddress.u32bit; // is still 0xFF
	sampleData[0] = data.u8bit.byteLow;
	sampleData[1] = data.u8bit.byteMidLow;
	sampleData[2] = data.u8bit.byteMidHigh;
  ext_flash_overwrite_sample_without_erase(sampleData, 3);

	actualAddress = backup;
	ext_flash_incf_address(EF_SAMPLE);
	actualPointerSample = actualAddress;
	ext_flash_enable_protection();
}


void ext_flash_write_sample(uint8_t *pSample, uint16_t length)
{
	uint32_t actualAdressBackup = 0;

	ef_write_block(pSample,length, EF_SAMPLE, 0);

	SSettings *settings = settingsGetPointer();
	settings->logFlashNextSampleStartAddress = actualPointerSample;

	if(0xFFFF - (actualAddress & 0x0000FFFF) < 255)								/* are we close to a sector border? */
	{
		if (((actualAddress & 0x0000FFFF) != 0) && (preparedPageAddress == 0))		/* only prepare if not already at start of sector */
		{
			actualAdressBackup = actualAddress;
			actualAddress = (actualAddress & 0xFFFF0000) + 0x00010000;	/* Set to start of next 64k sector */
			if(actualAddress >= SAMPLESTOP)
			{
				actualAddress = SAMPLESTART;
			}
			preparedPageAddress = actualAddress;
			ext_flash_erase64kB();
			actualAddress = actualAdressBackup;
		}
	}
}

static void ext_flash_overwrite_sample_without_erase(uint8_t *pSample, uint16_t length)
{
	ef_write_block(pSample,length, EF_SAMPLE, 1);
}


uint8_t ext_flash_count_dive_headers(void)
{
	uint8_t id = 0;
	uint8_t counter = 0;
	uint16_t headerStartData = 0x0000;
	
	id = settingsGetPointer()->lastDiveLogId;

	do
	{
		actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
		ext_flash_read_block_start();
		ext_flash_read_block_multi((uint8_t *)&headerStartData, 2, EF_HEADER);
		ext_flash_read_block_stop();
		counter++;
		id -=1;
	} while((headerStartData == 0xFAFA) && (counter < 255));
	return (counter - 1);
}


void ext_flash_read_dive_header(uint8_t *pHeaderToFill, uint8_t StepBackwards)
{
	SSettings *settings;
	uint8_t id;
	uint16_t i;

	settings = settingsGetPointer();
	id = settings->lastDiveLogId;
	id -= StepBackwards; /* 0-255, auto rollover */

	actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
	ext_flash_read_block_start();
	for(i = 0; i < HEADERSIZE; i++)
		ext_flash_read_block(&pHeaderToFill[i], EF_HEADER);
	ext_flash_read_block_stop();

}

void ext_flash_read_dive_header2(uint8_t *pHeaderToFill, uint8_t id, _Bool bOffset)
{

	uint16_t i;
	actualAddress = HEADERSTART + (0x800 * id) ;

	if(bOffset)
	  actualAddress += HEADER2OFFSET;
	ext_flash_read_block_start();
	for(i = 0; i < HEADERSIZE; i++)
		ext_flash_read_block(&pHeaderToFill[i], EF_HEADER);
	ext_flash_read_block_stop();
}


uint32_t ext_flash_read_dive_raw_with_double_header_1K(uint8_t *data, uint32_t max_size, uint8_t StepBackwards)
{
	if(max_size < 0x800) 
		return 0;
	
	uint8_t id;
	convert_Type dataStart, dataEnd;
	uint32_t LengthAll = 0;
	
	id = settingsGetPointer()->lastDiveLogId;
	id -= StepBackwards; /* 0-255, auto rollover */

	// clear data
	for(int i=0;i<0x800;i++)
		data[i] = 0xFF;
	
	// copy primary/pre-dive
	actualAddress = HEADERSTART + (0x800 * id);
	ext_flash_read_block_start();
	for(int i = 0; i < HEADERSIZE; i++)
		ext_flash_read_block(&data[i], EF_HEADER);
	ext_flash_read_block_stop();

	// copy main/secondary/post-dive
	actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
	ext_flash_read_block_start();
	for(int i = 0x400; i < HEADERSIZE+0x400; i++)
		ext_flash_read_block(&data[i], EF_HEADER);
	ext_flash_read_block_stop();
	
	// data

	dataStart.u8bit.byteHigh = 0;
	dataStart.u8bit.byteLow 		= data[0x402];
	dataStart.u8bit.byteMidLow 	= data[0x403];
	dataStart.u8bit.byteMidHigh	= data[0x404];

	dataEnd.u8bit.byteHigh = 0;
	dataEnd.u8bit.byteLow 			= data[0x405];
	dataEnd.u8bit.byteMidLow 		= data[0x406];
	dataEnd.u8bit.byteMidHigh		= data[0x407];
	
	actualPointerSample = dataStart.u32bit;
	if(dataEnd.u32bit >= dataStart.u32bit)
		LengthAll = 1 + dataEnd.u32bit - dataStart.u32bit;
	else
		LengthAll = 2 + (dataStart.u32bit - SAMPLESTART) + (SAMPLESTOP - dataEnd.u32bit);

	LengthAll += 0x800;
	
	if(LengthAll > max_size)
		return 0x800;

	actualAddress = actualPointerSample;
	ext_flash_read_block_start();
	for(uint32_t i = 0x800; i < LengthAll; i++)
		ext_flash_read_block(&data[i], EF_SAMPLE);
	ext_flash_read_block_stop();
	return LengthAll;
}

void ext_flash_write_dive_raw_with_double_header_1K(uint8_t *data, uint32_t length)
{
	convert_Type dataStart, dataEnd;
	SLogbookHeader  headerTemp;

	// set actualPointerSample and get pointer to sample storage and disable flash write protect
	ext_flash_start_new_dive_log_and_set_actualPointerSample((uint8_t *)&headerTemp);
	
	dataStart.u8bit.byteHigh = 0;
	dataStart.u8bit.byteLow 		= headerTemp.pBeginProfileData[0];
	dataStart.u8bit.byteMidLow 	= headerTemp.pBeginProfileData[1];
	dataStart.u8bit.byteMidHigh	= headerTemp.pBeginProfileData[2];

	dataEnd.u32bit = dataStart.u32bit + length - 0x801;
	if(dataEnd.u32bit > SAMPLESTOP)
		dataEnd.u32bit -=  SAMPLESTOP + SAMPLESTART - 1;
	
	data[0x002] = data[0x402] = dataStart.u8bit.byteLow;
	data[0x003] = data[0x403] = dataStart.u8bit.byteMidLow;
	data[0x004] = data[0x404] = dataStart.u8bit.byteMidHigh;
	data[0x005] = data[0x405] = dataEnd.u8bit.byteLow;
	data[0x006] = data[0x406] = dataEnd.u8bit.byteMidLow;
	data[0x007] = data[0x407] = dataEnd.u8bit.byteMidHigh;

	// set actualPointerHeader to next free header and update lastDiveLogId
	ext_flash_create_new_dive_log(0);

	// copy header data
	ef_write_block(data,0x800,EF_HEADER, 1);
	
 // copy sample data
	ef_write_block(&data[0x800], length-0x800, EF_SAMPLE, 1);
	
	// update logFlashNextSampleStartAddress
	settingsGetPointer()->logFlashNextSampleStartAddress = actualPointerSample;
}


//  ===============================================================================
//  ext_flash_read_header_memory
/// @brief		This function returns the entire header space 1:1
///	@date			04-April-2016
///
/// @param	*data 256KB output
//  ===============================================================================
void ext_flash_read_header_memory(uint8_t *data)
{
	actualAddress = HEADERSTART;
	actualPointerHeader = actualAddress;
	ext_flash_read_block_start();
	for(int i=0;i<8;i++)
		ext_flash_read_block_multi(&data[0x8000 * i], 0x8000, EF_HEADER);
	ext_flash_read_block_stop();
}


//  ===============================================================================
//  ext_flash_write_header_memory
/// @brief		This function erases and overwrites the entire logbook header block
///	@date			04-April-2016
///
/// @param	*data 256KB input of header memory 1:1
//  ===============================================================================
void ext_flash_write_header_memory(uint8_t *data)
{
	actualAddress = HEADERSTART;
	actualPointerHeader = actualAddress;
	ef_write_block(data, 0x40000, EF_HEADER, 0);
}

void ext_flash_read_sample_memory(uint8_t *data,uint16_t blockId)
{
	actualAddress = SAMPLESTART;
	actualAddress += blockId * 0x8000;	/* add 32k Block offset */
	actualPointerSample = actualAddress;
	ext_flash_read_block_start();
	ext_flash_read_block_multi(data, 0x8000, EF_SAMPLE);
	ext_flash_read_block_stop();
}

void ext_flash_write_sample_memory(uint8_t *data,uint16_t blockId)
{
	actualAddress = SAMPLESTART;
	actualAddress += blockId * 0x8000;	/* add 32k Block offset */
	actualPointerSample = actualAddress;
	ef_write_block(data, 0x8000, EF_SAMPLE,0);
}


void ext_flash_open_read_sample(uint8_t StepBackwards, uint32_t *totalNumberOfBytes)
{
	SSettings *settings = settingsGetPointer();
	uint8_t id;
	convert_Type dataStart, dataEnd;
	uint8_t header1, header2;

	id = settings->lastDiveLogId;
	id -= StepBackwards; /* 0-255, auto rollover */
#
	actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
	actualPointerHeader = actualAddress;

	ext_flash_read_block_start();
	/* little endian */
	ext_flash_read_block(&header1, EF_HEADER);
	ext_flash_read_block(&header2, EF_HEADER);
	dataStart.u8bit.byteHigh = 0;
	ext_flash_read_block(&dataStart.u8bit.byteLow, EF_HEADER);
	ext_flash_read_block(&dataStart.u8bit.byteMidLow, EF_HEADER);
	ext_flash_read_block(&dataStart.u8bit.byteMidHigh, EF_HEADER);
	dataEnd.u8bit.byteHigh = 0;
	ext_flash_read_block(&dataEnd.u8bit.byteLow, EF_HEADER);
	ext_flash_read_block(&dataEnd.u8bit.byteMidLow, EF_HEADER);
	ext_flash_read_block(&dataEnd.u8bit.byteMidHigh, EF_HEADER);
	ext_flash_read_block_stop();

	actualPointerSample = dataStart.u32bit;
	if(dataEnd.u32bit >= dataStart.u32bit)
		LengthLeftSampleRead = 1 + dataEnd.u32bit - dataStart.u32bit;
	else
		LengthLeftSampleRead = 2 + (dataStart.u32bit - SAMPLESTART) + (SAMPLESTOP - dataEnd.u32bit);
	*totalNumberOfBytes = LengthLeftSampleRead;

	actualAddress = actualPointerSample;
	ext_flash_read_block_start();
}


void ext_flash_read_next_sample_part(uint8_t *pSample, uint8_t length)
{
	for(uint16_t i = 0; i < length; i++)
		ext_flash_read_block(&pSample[i], EF_SAMPLE);
}


void ext_flash_close_read_sample(void)
{
  actualPointerSample =	actualAddress;
	ext_flash_read_block_stop();
}


void ext_flash_set_entry_point(void)
{
	entryPoint = actualAddress;
}


void ext_flash_reopen_read_sample_at_entry_point(void)
{
	error_led_on();
	chip_unselect();
	wait_chip_not_busy();

	actualAddress = entryPoint;
	ext_flash_read_block_start();
	error_led_off();
}

/*
uint8_t ext_flash_point_to_64k_block_in_headerSpace(uint8_t logId)
{
	uint32_t pointerToData = logId * 0x800;
	
	return pointerToData / 0x10000;
}
*/


//  ===============================================================================
//	ext_flash_repair_dive_numbers_starting_count_helper
/// @brief	
///	@date			22-June-2016

//  ===============================================================================
static uint16_t ext_flash_repair_dive_numbers_starting_count_helper(uint8_t *data, uint8_t *change64k, uint16_t startNumber, uint8_t lastLogId)
{
	const uint32_t headerStep = 0x800;
	uint8_t	actualLogId = 0;
	uint16_t oldNumber = 0;
	uint16_t actualNumber = 0;
	SLogbookHeader *ptrLogbookHeader = 0;
	
	if(startNumber == 0)
		return 0;

	actualNumber = startNumber - 1;	

	// where is the oldest dive (Which is now getting startNumber)
	// use first header for ease (without HEADER2OFFSET for end of dive header)
	// compare for lastLogId to prevent endless loop

	if(*(uint16_t*)&data[lastLogId * headerStep] != 0xFAFA)
		return 0;
	
	actualLogId = lastLogId - 1;
	while((*(uint16_t*)&data[actualLogId * headerStep] == 0xFAFA) && (actualLogId != lastLogId))
	{
		actualLogId--;
	}
	
	// now pointing to one behind the last
	while(actualLogId != lastLogId)
	{
		actualLogId++;
		actualNumber++;
		ptrLogbookHeader = (SLogbookHeader *)&data[actualLogId * headerStep];
		
		oldNumber = ptrLogbookHeader->diveNumber;
		if(oldNumber != actualNumber)
		{
//			change64k[ext_flash_point_to_64k_block_in_headerSpace(actualLogId )] = 1;
			change64k[(actualLogId * 0x800)/0x10000] = 1;
			ptrLogbookHeader->diveNumber = actualNumber;
			ptrLogbookHeader = (SLogbookHeader *)(&data[actualLogId * headerStep] + HEADER2OFFSET);
			ptrLogbookHeader->diveNumber = actualNumber;
		}
	}
	
	return actualNumber;
}

	//  ===============================================================================
//	ext_flash_repair_SPECIAL_dive_numbers_starting_count_with
/// @brief	This function
///	@date    04-April-2016
/// problem (160621): 64K blocks (32 dives) in the new flash memory chip
/// This block needs to be deleted
/// these where only 4KB block before
/// @output endCount, last diveNumber

//  ===============================================================================
uint16_t ext_flash_repair_SPECIAL_dive_numbers_starting_count_with(uint16_t startCount)
{
	uint32_t logCopyDataPtr = 0;
	uint8_t *data;
	uint16_t lastCount;
	uint8_t listOfChanged64kBlocks[8]; // 32 dives each 64K
	
	logCopyDataPtr = getFrame(97);
	data = (uint8_t *)logCopyDataPtr;

	for(int i=0;i<8;i++)
		listOfChanged64kBlocks[i] = 0;

	actualAddress = HEADERSTART;
	ext_flash_read_block_start();
	ext_flash_read_block_multi(data,0x100000,EF_HEADER);
	ext_flash_read_block_stop();

	lastCount = ext_flash_repair_dive_numbers_starting_count_helper(data, listOfChanged64kBlocks, startCount, settingsGetPointer()->lastDiveLogId);
	
	for(int i=0;i<8;i++)
	{
		if(listOfChanged64kBlocks[i] != 0)
		{
			actualPointerHeader = HEADERSTART + (i * 0x10000);
			ef_write_block(&data[i * 0x10000], 0x10000, EF_HEADER, 0);
		}
	}

	releaseFrame(97,logCopyDataPtr);
	if(settingsGetPointer()->totalDiveCounter < lastCount)
	{
		settingsGetPointer()->totalDiveCounter = lastCount;
	}
	return lastCount;
}

/*
void OLD_ext_flash_repair_SPECIAL_dive_numbers_starting_count_with(uint16_t startCount)
{
	uint16_t counterStorage[256];
	uint8_t start = 0xFF;
	uint32_t logCopyDataPtr = 0;
	uint8_t *data;
	uint8_t startAbsolute = 0;
	int16_t count = 0;
	_Bool repair = 0;
	uint8_t startBackup = 0;
	
	SLogbookHeader  tempLogbookHeader;
	SLogbookHeader *ptrHeaderInData1a;
	SLogbookHeader *ptrHeaderInData1b;
	SLogbookHeader *ptrHeaderInData2a;
	SLogbookHeader *ptrHeaderInData2b;

	logCopyDataPtr = getFrame(97);
	data = (uint8_t *)logCopyDataPtr;
	ptrHeaderInData1a = (SLogbookHeader *)logCopyDataPtr;
	ptrHeaderInData1b = (SLogbookHeader *)(logCopyDataPtr + HEADER2OFFSET);
	ptrHeaderInData2a = (SLogbookHeader *)(logCopyDataPtr + 0x800);
	ptrHeaderInData2b = (SLogbookHeader *)(logCopyDataPtr + 0x800 + HEADER2OFFSET);
	
	// get data
	for(int StepBackwards = 0; StepBackwards < 255; StepBackwards++)
	{
		logbook_getHeader(StepBackwards, &tempLogbookHeader);
		counterStorage[StepBackwards+1] = tempLogbookHeader.diveNumber;
		if(tempLogbookHeader.diveHeaderStart == 0xFAFA)
			start = StepBackwards;
		else
			break;
	}
	
	if(start == 0xFF)
		return;
	
	count = start + 1;
	startAbsolute = settingsGetPointer()->lastDiveLogId;



	if(start%2)
	{
		if(counterStorage[start] != startCount)
		{
			// clear data
			for(int i=0;i<0x800*2;i++)
				data[i] = 0xFF;

			uint8_t id = settingsGetPointer()->lastDiveLogId;
			id -= start; // 0-255, auto rollover
			
			// copy primary/pre-dive
			actualAddress = HEADERSTART + (0x800 * id);
			ext_flash_read_block_start();
			for(int i = 0; i < HEADERSIZE; i++)
				ext_flash_read_block(&data[i], EF_HEADER);
			ext_flash_read_block_stop();

			// copy main/secondary/post-dive
			actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
			ext_flash_read_block_start();
			for(int i = 0x400; i < HEADERSIZE+0x400; i++)
				ext_flash_read_block(&data[i], EF_HEADER);
			ext_flash_read_block_stop();
			
			// repair
			ptrHeaderInData2a->diveNumber = startCount;
			ptrHeaderInData2b->diveNumber = startCount;
			startCount++;

			// write
			actualAddress = HEADERSTART + (0x800 * (id-1));
			ef_write_block(data,0x800*2,EF_HEADER, 0);
		}
		start--;
	}

//	for(int count = start; count > -1; count -= 2)

	while(count > 0)
	{
			// clear data
			for(int i=0;i<0x1000;i++)
				data[i] = 0xFF;

		repair = 0;
		
		startBackup = startAbsolute;
		
		if(startAbsolute%2) // 0x800 to 0x1000
		{
			// copy second pre-dive
			actualAddress = HEADERSTART + (0x800 * startAbsolute);
			ext_flash_read_block_start();
			for(int i = 0x800; i < HEADERSIZE+0x800; i++)
				ext_flash_read_block(&data[i], EF_HEADER);
			ext_flash_read_block_stop();

			// copy second post-dive
			actualAddress = HEADERSTART + HEADER2OFFSET + (0x800 * startAbsolute);
			ext_flash_read_block_start();
			for(int i = 0xC00; i < HEADERSIZE+0xC00; i++)
				ext_flash_read_block(&data[i], EF_HEADER);
			ext_flash_read_block_stop();

			if(counterStorage[count] != startCount)
			{
				ptrHeaderInData2a->diveNumber = startCount;
				ptrHeaderInData2b->diveNumber = startCount;
				repair = 1; 
			}
			startCount += 1;
			
			startAbsolute -= 1;
			count -= 1;
			
			if(count > 0)
			{
				// copy  first pre-dive
				actualAddress = HEADERSTART + (0x800 * startAbsolute);
				ext_flash_read_block_start();
				for(int i = 0; i < HEADERSIZE; i++)
					ext_flash_read_block(&data[i], EF_HEADER);
				ext_flash_read_block_stop();

				// copy first post-dive
				actualAddress = HEADERSTART + (0x800 * startAbsolute);
				ext_flash_read_block_start();
				for(int i = 0x400; i < HEADERSIZE+0x400; i++)
					ext_flash_read_block(&data[i], EF_HEADER);
				ext_flash_read_block_stop();

				if(counterStorage[count] != startCount)
				{
					ptrHeaderInData1a->diveNumber = startCount;
					ptrHeaderInData1b->diveNumber = startCount;
					repair = 1;
				}
				startCount += 1;

				startAbsolute -= 1;
				count -= 1;
			}
		}
		else
		{
			// copy  first pre-dive
			actualAddress = HEADERSTART + (0x800 * startAbsolute);
			ext_flash_read_block_start();
			for(int i = 0; i < HEADERSIZE; i++)
				ext_flash_read_block(&data[i], EF_HEADER);
			ext_flash_read_block_stop();

			// copy first post-dive
			actualAddress = HEADERSTART + (0x800 * startAbsolute);
			ext_flash_read_block_start();
			for(int i = 0x400; i < HEADERSIZE+0x400; i++)
				ext_flash_read_block(&data[i], EF_HEADER);
			ext_flash_read_block_stop();

			if(counterStorage[count] != startCount)
			{
				ptrHeaderInData1a->diveNumber = startCount;
				ptrHeaderInData1b->diveNumber = startCount;
				repair = 1;
			}
			startCount += 1;

			startAbsolute -= 1;
			count -= 1;
		}
			
		// write
		if(repair)
		{
			actualPointerHeader = HEADERSTART + (0x1000 * startBackup%2);
			ef_write_block(data,0x1000,EF_HEADER, 0);
		}
	}
	releaseFrame(97,logCopyDataPtr);
	settingsGetPointer()->totalDiveCounter = startCount;
}
*/

uint8_t ext_dive_log_consistent(void)
{
	uint8_t ret = 0;
	uint8_t  header1, header2;
	uint8_t id;
	convert_Type dataStart;
	convert_Type dataEnd;

	SSettings *settings = settingsGetPointer();
	id = settings->lastDiveLogId;

	if(settings->lastDiveLogId != 0) /* not trust LogID 0 which might be reset by a settings problem */
	{								 /* ==> Find function will be called restoring the Id belonging to the latest sample */
		actualAddress = HEADERSTART + (0x800 * id);
		ext_flash_read_block_start();
		ext_flash_read_block(&header1, EF_HEADER);
		ext_flash_read_block(&header2, EF_HEADER);
		ext_flash_read_block_stop();
		if((header1 == 0xFA) && (header2 == 0xFA))						/* Header is indicating the start of a dive */
		{
		  actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
		  ext_flash_read_block_start();
		  ext_flash_read_block(&header1, EF_HEADER);
		  ext_flash_read_block(&header2, EF_HEADER);
		  dataStart.u8bit.byteHigh = 0;
		  ext_flash_read_block(&dataStart.u8bit.byteLow, EF_HEADER);
		  ext_flash_read_block(&dataStart.u8bit.byteMidLow, EF_HEADER);
		  ext_flash_read_block(&dataStart.u8bit.byteMidHigh, EF_HEADER);
		  dataEnd.u8bit.byteHigh = 0;
		  ext_flash_read_block(&dataEnd.u8bit.byteLow, EF_HEADER);
		  ext_flash_read_block(&dataEnd.u8bit.byteMidLow, EF_HEADER);
		  ext_flash_read_block(&dataEnd.u8bit.byteMidHigh, EF_HEADER);
		  ext_flash_read_block_stop();

		  if((header1 == 0xFA) && (header2 == 0xFA))					/* Secondary header was written at the end of a dive */
		  {
			  if(dataStart.u32bit < dataEnd.u32bit)
			  {
				  ret = 1;												/* => lastDiveLogID points to a valid dive entry */
			  }
			  else														/* check if address wrap is valid */
			  {
				  if(ext_flash_SampleOverrunValid())
				  {
					  ret = 1;
				  }
			  }
		  }
		}
	}
    return ret;
}

//  ===============================================================================
//	ext_flash_repair_dive_log
/// @brief	This function 
///					does set
///						logFlashNextSampleStartAddress
///					and
///						lastDiveLogId
///

void ext_flash_repair_dive_log(void)
{
	uint8_t  header1, header2;
  convert_Type dataStart;

  for(int id = 0; id < 255;id++)
  {
    actualAddress = HEADERSTART + (0x800 * id);
    ext_flash_read_block_start();
    ext_flash_read_block(&header1, EF_HEADER);
    ext_flash_read_block(&header2, EF_HEADER);
    dataStart.u8bit.byteHigh = 0;
    ext_flash_read_block(&dataStart.u8bit.byteLow, EF_HEADER);
    ext_flash_read_block(&dataStart.u8bit.byteMidLow, EF_HEADER);
    ext_flash_read_block(&dataStart.u8bit.byteMidHigh, EF_HEADER);
    ext_flash_read_block_stop();
    if((header1 == 0xFA) && (header2 == 0xFA))						/* Header is indicating the start of a dive */
    {
      actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
      ext_flash_read_block_start();
      ext_flash_read_block(&header1, EF_HEADER);
      ext_flash_read_block(&header2, EF_HEADER);
      ext_flash_read_block_stop();
      if((header1 != 0xFA) || (header2 != 0xFA))					/* Secondary header was not written at the end of a dive */
      {
        actualPointerSample = dataStart.u32bit;						/* Set datapointer to position stored in header written at beginning of dive */
        actualAddress = actualPointerSample;
        logbook_recover_brokenlog(id);
        SSettings *settings = settingsGetPointer();
        settings->logFlashNextSampleStartAddress = actualPointerSample;
      }
    }
  }
  ext_flash_find_start();
}


static void ext_flash_find_start(void)
{
	uint8_t id;
	uint8_t  header1, header2;
  convert_Type dataStart, dataEnd;

  /* TODO Cleanup_Ref_1: cleanup logFlashNextSampleStartAddress and lastDiveLogId */
  /* The implementation below would cause problems in case more then 254 dives would be done. */
  /* This is avoided by Cleanup_Ref2 */
  for(id = 0; id <= 255;id++)
  {
    actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
    ext_flash_read_block_start();
    ext_flash_read_block(&header1, EF_HEADER);
    ext_flash_read_block(&header2, EF_HEADER);
    dataStart.u8bit.byteHigh = 0;
    ext_flash_read_block(&dataStart.u8bit.byteLow, EF_HEADER);
    ext_flash_read_block(&dataStart.u8bit.byteMidLow, EF_HEADER);
    ext_flash_read_block(&dataStart.u8bit.byteMidHigh, EF_HEADER);
    ext_flash_read_block_stop();
    if((header1 == 0xFF) && (header2 == 0xFF))
    {
      break;
    }
  }
  id--;
  SSettings *settings = settingsGetPointer();
	settings->lastDiveLogId = id;

	actualAddress = HEADERSTART + (0x800 * id) + HEADER2OFFSET;
	actualPointerHeader = actualAddress;

  ext_flash_read_block_start();

  ext_flash_read_block(&header1, EF_HEADER);
  ext_flash_read_block(&header2, EF_HEADER);
  dataStart.u8bit.byteHigh = 0;
  ext_flash_read_block(&dataStart.u8bit.byteLow, EF_HEADER);
  ext_flash_read_block(&dataStart.u8bit.byteMidLow, EF_HEADER);
  ext_flash_read_block(&dataStart.u8bit.byteMidHigh, EF_HEADER);
  dataEnd.u8bit.byteHigh = 0;
  ext_flash_read_block(&dataEnd.u8bit.byteLow, EF_HEADER);
  ext_flash_read_block(&dataEnd.u8bit.byteMidLow, EF_HEADER);
  ext_flash_read_block(&dataEnd.u8bit.byteMidHigh, EF_HEADER);
  ext_flash_read_block_stop();

  //Find free space
  if((header1 == 0xFA) && (header2 == 0xFA))
  {
    uint8_t uiRead = 0;
    int countFF = 0;
    //End of last complete dive
    actualPointerSample = dataEnd.u32bit ;
    actualAddress = actualPointerSample;
    //Check if there are samples of dives with less than half a minute
    while(true)
    {
      ext_flash_read_block_start();
      ext_flash_read_block(&uiRead, EF_SAMPLE);
      if(uiRead == 0xFF)
        countFF++;
      else
        countFF = 0;



      if(countFF == 10)
      {
        actualAddress -= 10;
        break;
      }

      //New page: clear
			if(ext_flash_erase_if_on_page_start())
        break;
    }
    // Set new start address
    actualPointerSample = actualAddress;
    settings->logFlashNextSampleStartAddress = actualPointerSample;
  }
  else
  {
    settings->logFlashNextSampleStartAddress = SAMPLESTART;
  }
}




static void ext_flash_disable_protection(void)
{
/*	
	extFlashStatusBit8_Type status;

	status.uw = 0;

	wait_chip_not_busy();
	write_spi(0x50,RELEASE); 			// EWSR
	write_spi(0x01,HOLDCS); 			// WRSR
	write_spi(status.uw,RELEASE);	// new status
*/
}
#endif

void ext_flash_disable_protection_for_logbook(void)
{
	/*
	extFlashStatusBit8_Type status;

	status.uw = 0;
	status.ub.BlockProtect0 = 1;
	status.ub.BlockProtect1 = 0;
	status.ub.BlockProtect2 = 1;
	status.ub.BlockProtect3 = 0; // not set in OSTC3. Why?

	wait_chip_not_busy();
	write_spi(0x50,RELEASE); 			// EWSR
	write_spi(0x01,HOLDCS); 			// WRSR
	write_spi(status.uw,RELEASE);	// new status
*/	
}


void ext_flash_enable_protection(void)
{
/*	
	extFlashStatusBit8_Type status;

	status.uw = 0;
	status.ub.BlockProtect0 = 1;
	status.ub.BlockProtect1 = 1;
	status.ub.BlockProtect2 = 1;
	status.ub.BlockProtect3 = 1; // not set in OSTC3. Why?

	wait_chip_not_busy();
	write_spi(0x50,RELEASE); 			// EWSR
	write_spi(0x01,HOLDCS); 			// WRSR
	write_spi(status.uw,RELEASE); // new status
*/
}


/*void ext_flash_erase_chip(void)
{
	wait_chip_not_busy();
	write_spi(0x06,RELEASE);
	write_spi(0x60,RELEASE);
	wait_chip_not_busy();
}*/

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



void ext_flash_erase_logbook(void)
{
	uint32_t size, blocks_64k;

	ext_flash_disable_protection_for_logbook();

	actualAddress = SAMPLESTART;
	size = 1 + SAMPLESTOP - SAMPLESTART;
	blocks_64k = size / 0x10000;
	ef_erase_64K(blocks_64k);

	actualAddress = HEADERSTART;
	size = 1 + HEADERSTOP - HEADERSTART;
	blocks_64k = size / 0x10000;
	ef_erase_64K(blocks_64k);

	ext_flash_enable_protection();
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

#ifndef BOOTLOADER_STANDALONE
static _Bool ext_flash_test_remaining_space_of_page_empty(uint32_t pointer, uint16_t length)
{
	if((pointer & 0xFFF) == 0)
		return 1;

	uint32_t backup = actualAddress;
	uint8_t data;
	uint32_t size_to_page_end;

	size_to_page_end = 0x1000 - (pointer & 0xFFF);
	if(length > size_to_page_end)
		length = size_to_page_end;

	actualAddress = pointer;
	ext_flash_read_block_start();

	for(uint16_t i = 0; i<length; i++)
	{
		ext_flash_read_block(&data, 255); // 255 = ENTIRE FLASH
		if(data != 0xFF)
		{
			ext_flash_read_block_stop();
			actualAddress = backup;
			return 0;
		}
	}
	ext_flash_read_block_stop();
	actualAddress = backup;
	return 1;
}


static void ext_flash_set_to_begin_of_next_page(uint32_t *pointer, uint8_t type)
{
	uint32_t ringStart, ringStop;

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
		default:
			ringStart = FLASHSTART;
			ringStop = FLASHSTOP;
			break;
	}

	*pointer = (*pointer & 0xFFF) + 0x1000;

	if((*pointer < ringStart) || (*pointer >= ringStop))
		*pointer = ringStart;
}
#endif

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
#ifndef BOOTLOADER_STANDALONE
static void error_led_on(void)
{
		HAL_GPIO_WritePin(OSCILLOSCOPE_GPIO_PORT,OSCILLOSCOPE_PIN,GPIO_PIN_SET);
}

static void error_led_off(void)
{
		HAL_GPIO_WritePin(OSCILLOSCOPE_GPIO_PORT,OSCILLOSCOPE_PIN,GPIO_PIN_RESET);
}
#endif

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

#ifndef BOOTLOADER_STANDALONE
static void ext_flash_decf_address_ring(uint8_t type)
{
	uint32_t ringStart, ringStop;
	
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
	
	if((actualAddress <= ringStart) || (actualAddress > ringStop))
		actualAddress = ringStop;
	else
		actualAddress -= 1;
}
#endif

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

/* This function validates a potential jump of sample address by checking the last sector for empty memory cells */
uint8_t ext_flash_SampleOverrunValid(void)
{
	uint8_t jumpvalid = 1;
	uint32_t curAddress, actualaddrbackup;
	uint8_t tmpBuffer;
	uint8_t emptyCellCnt = 0;

	actualaddrbackup = actualAddress;
	curAddress = SAMPLESTOP - 20;	/* check the last 20 bytes of the last sample sector */
	actualAddress = curAddress;
	ext_flash_read_block_start();
	while(actualAddress < SAMPLESTOP)
	{
		tmpBuffer = read_spi(HOLDCS);/* read data */
		if(tmpBuffer == 0xFF)
		{
			emptyCellCnt++;
		}
		actualAddress++;
	}
	ext_flash_read_block_stop();

	if(emptyCellCnt == 20)
	{
		jumpvalid = 0;
	}
	actualAddress = actualaddrbackup;
	return jumpvalid;
}

uint32_t ext_flash_AnalyseSampleBuffer()
{
	uint8_t sectorState[192];	/* samples are stored in 192 sector / 64k each */
	uint32_t curAddress = SAMPLESTART;
	uint8_t curSector = 0;
	uint8_t samplebuffer[10];
	uint32_t actualAddressBackup = actualAddress;
	uint8_t emptyCellCnt = 0;
	uint32_t i = 0;
	uint8_t startedSectors = 0;
	uint8_t	lastSectorInuse = 0;

/* check if a sector is used till its end */
	for(curSector = 0; curSector < 192; curSector++)
	{
		sectorState[curSector] = 0;
		emptyCellCnt = 0;
		curAddress = SAMPLESTART + (curSector * 0x10000);	/* set address to begin of sector and check if it is used */
		actualAddress = curAddress;
		ext_flash_read_block_start();
		for(uint32_t i=0;i<10;i++)
		{
			samplebuffer[i] = read_spi(HOLDCS);/* read data */
			if(samplebuffer[i] == 0xFF)
			{
				emptyCellCnt++;
			}
		}
		ext_flash_read_block_stop();
		if(emptyCellCnt == 10)
		{
			sectorState[curSector] = SECTOR_NOTUSED;
		}
		emptyCellCnt = 0;
		curAddress = SAMPLESTART + (curSector * 0x10000) + 0xFFF5;	/* set address to end of sector and check if it is used */
		actualAddress = curAddress;
		ext_flash_read_block_start();
		for(i=0;i<10;i++)
		{
			samplebuffer[i] = read_spi(HOLDCS);/* read data */
			if(samplebuffer[i] == 0xFF)
			{
				emptyCellCnt++;
			}
		}
		ext_flash_read_block_stop();
		if(emptyCellCnt == 10)
		{
			sectorState[curSector] |= SECTOR_INUSE;		/* will become SECTOR_EMPTY if start is NOTUSED */
		}
	}

	for(i=0;i<192;i++)
	{
		if( sectorState[i] == SECTOR_INUSE)
		{
			startedSectors++;
			lastSectorInuse = i;
		}
	}

	if(startedSectors > 1)	/* more than one sector is in used => ring buffer corrupted */
	{
		if(startedSectors == 2)			/* only fix issue if only two sectors are in used. Otherwise fixing will cause more worries than help */
		{
		/* the logic behind healing of the problem is that the larger address is the oldest one => restore the largest address */
			curAddress = SAMPLESTART + (lastSectorInuse * 0x10000);
			emptyCellCnt = 0;
			actualAddress = curAddress;
			ext_flash_read_block_start();
			while((emptyCellCnt < 10) && (actualAddress < curAddress + 0x10000))
			{
				samplebuffer[0] = read_spi(HOLDCS);/* read data */
				if(samplebuffer[0] == 0xFF)
				{
					emptyCellCnt++;
				}
				else
				{
					emptyCellCnt = 0;
				}
				actualAddress++;
			}
			ext_flash_read_block_stop();
			actualAddress -= 10;	/* step 10 bytes back to the start of free bytes */
			actualPointerSample = actualAddress;

			closeSectorAddress = settingsGetPointer()->logFlashNextSampleStartAddress & 0xFFFF0000;
			closeSectorAddress += 0xFFF5; /* to be used once next time a dive is logged. Needed because NextSampleID is derived at every startup */
			settingsGetPointer()->logFlashNextSampleStartAddress = actualPointerSample;	/* store new position to be used for next dive */
			ext_flash_CloseSector();
		}
	}
	actualAddress = actualAddressBackup;
	return startedSectors;
}

uint32_t ext_flash_read_profilelength_small_header(uint32_t smallHeaderAddr)
{
	uint32_t profileLength = 0;
	actualPointerSample = smallHeaderAddr;
	actualAddress = actualPointerSample;
	ext_flash_read_block_start();
	ext_flash_read_next_sample_part((uint8_t*)&profileLength,  3);
	ext_flash_close_read_sample();
	return profileLength;
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
