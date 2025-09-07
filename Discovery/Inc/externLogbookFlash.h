///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/externLogbookFlash.h
/// \brief  Header File to access the new 1.8 Volt Spansion S25FS256S 256 Mbit (32 Mbyte)
/// \author heinrichs weikamp gmbh
/// \date   07-Aug-2014
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2018 Heinrichs Weikamp gmbh
///
///     This program is free software: you can redistribute it and/or modify
///     it under the terms of the GNU General Public License as published by
///     the Free Software Foundation, either version 3 of the License, or
///     (at your option) any later version.
///
///     This program is distributed in the hope that it will be useful,
///     but WITHOUT ANY WARRANTY; without even the implied warranty of
///     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///     GNU General Public License for more details.
///
///     You should have received a copy of the GNU General Public License
///     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef EXTERN_LOGBOOK_FLASH_H
#define EXTERN_LOGBOOK_FLASH_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "logbook.h"

/* Exported variables --------------------------------------------------------*/

/* 4 KB
 * one for the basics like min./max. temperature, max. depth, charge cycles
 * after that two or more 0for settings (less than one necessary as of 26. March 2015)
 *
 */
#define DDSTART				0x00000000
#define DDSTOP				0x00000FFF

#define PROFILE0_START	0x00001000		/* store profiles within one 4k sector because they are always read / written as one block */
#define PROFILE0_STOP	0x000013FF		/* no ring functionality is implemented because changes are not expected very often */
#define PROFILE1_START	0x00001400
#define PROFILE1_STOP	0x000017FF
#define PROFILE2_START	0x00001800
#define PROFILE2_STOP	0x00001BFF
#define PROFILE3_START	0x00001C00
#define PROFILE3_STOP	0x00001FFF

#define unused1START	0x00002000
#define unused1STOP		0x00007FFF

/* 32 KB */
#define unused2START	0x00008000
#define unused2STOP		0x0000FFFF

/* 64 KB
 * 001x used for settings
 * 001x used for VPM
 * 005x unused
 * 008x for header		(0.5 MB)
 * 192x for samples		(12 MB)
 * 016x for firmware	( 1 MB)
 * 032x for firmware2	( 2 MB)
 */
#define SETTINGSSTART	0x00010000
#define SETTINGSSTOP 	0x0001FFFF
#define VPMSTART		0x00020000
#define VPMSTOP			0x0002FFFF
#define unused3START	0x00030000
#define unused3STOP		0x0007FFFF
#define HEADERSTART		0x00080000
#define HEADERSTOP		0x000FFFFF
#define SAMPLESTART		0x00100000
#define SAMPLESTOP		0x00CFFFFF
#define FWSTART			0x00D00000
#define FWSTOP			0x00DFFFFF
#define FWSTART2		0x00E00000
#define FWSTOP2			0x00FFFFFF
/* 16 MB with 4 Byte addressing */
#define unused4START	0x01000000
#define unused4STOP		0x01FFFFFF

#define HEADERSIZE sizeof(SLogbookHeader)
#define HEADERSIZEOSTC3 sizeof(SLogbookHeaderOSTC3)

/* Sample ring buffer sector states derived from the usage at begin and end of a sector */
#define SECTOR_CLOSED		(0)
#define SECTOR_NOTUSED		(1)
#define SECTOR_INUSE		(4)
#define SECTOR_EMPTY		(5)


typedef enum{
	EF_HEADER,
	EF_SAMPLE,
	EF_DEVICEDATA,
	EF_VPMDATA,
	EF_SETTINGS,
	EF_FIRMWARE,
	EF_FIRMWARE2,
	EF_PROFILE0,
	EF_PROFILE1,
	EF_PROFILE2,
	EF_PROFILE3,
}which_ring_enum;


/* Exported types ------------------------------------------------------------*/
typedef struct{
uint8_t byteLow;
uint8_t byteMidLow;
uint8_t byteMidHigh;
uint8_t byteHigh;
} addressToByte_t;

typedef struct{
uint8_t byteLow;
uint8_t byteHigh;
} WordToByte_t;

typedef union{
addressToByte_t u8bit;
uint32_t u32bit;
} convert_Type;

typedef union{
WordToByte_t u8bit;
uint16_t u16bit;
} convert16_Type;

/* Exported functions --------------------------------------------------------*/
void ext_flash_write_settings(uint8_t whichSettings, uint8_t resetRing);
uint8_t ext_flash_read_settings(uint8_t whichSettings);

void ext_flash_write_devicedata(uint8_t resetRing);
uint16_t ext_flash_read_devicedata(uint8_t *buffer, uint16_t max_length);
void ext_flash_read_fixed_16_devicedata_blocks_formated_128byte_total(uint8_t *buffer);

#ifndef BOOTLOADER_STANDALONE
void ext_flash_write_vpm(SVpm *vpmInput);
int ext_flash_read_vpm(SVpm *vpmOutput);
#endif

void ext_flash_start_new_dive_log_and_set_actualPointerSample(uint8_t *pHeaderPreDive);
void ext_flash_create_new_dive_log(uint8_t *pHeaderPreDive);
void ext_flash_close_new_dive_log(uint8_t *pHeaderPostDive);

void ext_flash_write_sample(uint8_t *pSample, uint16_t length);

uint8_t ext_flash_count_dive_headers(void);
void ext_flash_read_dive_header(uint8_t *pHeaderToFill, uint8_t StepBackwards);
void ext_flash_read_dive_header2(uint8_t *pHeaderToFill, uint8_t id, _Bool bOffset);
void ext_flash_open_read_sample(uint8_t StepBackwards, uint32_t *totalNumberOfBytes);
void ext_flash_read_next_sample_part(uint8_t *pSample, uint8_t length);
void ext_flash_close_read_sample(void);
void ext_flash_set_entry_point(void);
void ext_flash_reopen_read_sample_at_entry_point(void);

void ext_flash_write_dive_raw_with_double_header_1K(uint8_t *data, uint32_t length);
uint32_t ext_flash_read_dive_raw_with_double_header_1K(uint8_t *data, uint32_t max_size, uint8_t StepBackwards);

void ext_flash_read_header_memory(uint8_t *data);
void ext_flash_write_header_memory(uint8_t *data);

void ext_flash_read_sample_memory(uint8_t *data,uint16_t blockId);
void ext_flash_write_sample_memory(uint8_t *data,uint16_t blockId);

void ext_flash_erase_logbook(void);
void ext_flash_erase_chip(void);
void ext_flash_erase_firmware(void);
void ext_flash_erase_firmware2(void);
void ext_flash_disable_protection_for_logbook(void);
void ext_flash_enable_protection(void);

void ext_flash_read_block_start(void);
uint8_t ext_dive_log_consistent(void);
void ext_flash_repair_dive_log(void);

uint8_t ext_flash_erase_firmware_if_not_empty(void);
uint8_t ext_flash_erase_firmware2_if_not_empty(void);
void ext_flash_write_firmware(uint8_t *pSample1, uint32_t length1);//,uint8_t *pSample2, uint32_t length2);
uint32_t ext_flash_read_firmware(uint8_t *pSample1, uint32_t max_length, uint8_t *magicByte);
uint8_t ext_flash_read_firmware_version(char *text);

void ext_flash_write_firmware2(uint32_t offset, uint8_t *pSample1, uint32_t length1,uint8_t *pSample2, uint32_t length2);
uint32_t ext_flash_read_firmware2(uint32_t *offset, uint8_t *pSample1, uint32_t max_length1, uint8_t *pSample2, uint32_t max_length2);

uint16_t ext_flash_repair_SPECIAL_dive_numbers_starting_count_with(uint16_t startCount);

uint32_t ext_flash_AnalyseSampleBuffer(void);
void ext_flash_CloseSector(void);

uint32_t ext_flash_read_profilelength_small_header(uint32_t smallHeaderAddr);
uint8_t ext_flash_SampleOverrunValid(void);


#endif /* EXTERN_LOGBOOK_FLASH_H */
