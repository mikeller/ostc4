///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/firmwareEraseProgram.h
/// \brief  erase and program the STM32F4xx internal FLASH memory
/// \author heinrichs weikamp gmbh
/// \date   05-May-2015
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
#ifndef FIRMWARE_ERASE_PROGRAM_H
#define FIRMWARE_ERASE_PROGRAM_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Exported variables --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

#define HARDWAREDATA_ADDRESS	(0x08000000 + 0x0000A040)

/* Flash memory layout - Base address of Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

/* Base address of Flash sectors Bank 2 */
#define ADDR_FLASH_SECTOR_12     ((uint32_t)0x08100000) /* Base @ of Sector 12, 16 Kbytes */
#define ADDR_FLASH_SECTOR_13     ((uint32_t)0x08104000) /* Base @ of Sector 13, 16 Kbytes */
#define ADDR_FLASH_SECTOR_14     ((uint32_t)0x08108000) /* Base @ of Sector 14, 16 Kbytes */
#define ADDR_FLASH_SECTOR_15     ((uint32_t)0x0810C000) /* Base @ of Sector 15, 16 Kbytes */
#define ADDR_FLASH_SECTOR_16     ((uint32_t)0x08110000) /* Base @ of Sector 16, 64 Kbytes */
#define ADDR_FLASH_SECTOR_17     ((uint32_t)0x08120000) /* Base @ of Sector 17, 128 Kbytes */
#define ADDR_FLASH_SECTOR_18     ((uint32_t)0x08140000) /* Base @ of Sector 18, 128 Kbytes */
#define ADDR_FLASH_SECTOR_19     ((uint32_t)0x08160000) /* Base @ of Sector 19, 128 Kbytes */
#define ADDR_FLASH_SECTOR_20     ((uint32_t)0x08180000) /* Base @ of Sector 20, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_21     ((uint32_t)0x081A0000) /* Base @ of Sector 21, 128 Kbytes  */
#define ADDR_FLASH_SECTOR_22     ((uint32_t)0x081C0000) /* Base @ of Sector 22, 128 Kbytes */
#define ADDR_FLASH_SECTOR_23     ((uint32_t)0x081E0000) /* Base @ of Sector 23, 128 Kbytes */

#define SECTOR_SIZE_128KB     ((uint32_t)0x00020000)

/* Flash memory regions */
#define FLASH_BOOT_START_ADDR   ADDR_FLASH_SECTOR_0
#define FLASH_BOOT_END_ADDR     (ADDR_FLASH_SECTOR_5 - 1)

#define FLASH_FW_START_ADDR     ADDR_FLASH_SECTOR_6
#define FLASH_FW_END_ADDR       (ADDR_FLASH_SECTOR_12 - 1)

#define FLASH_FW2_START_ADDR    ADDR_FLASH_SECTOR_12
#define FLASH_FW2_END_ADDR      (ADDR_FLASH_SECTOR_22 + SECTOR_SIZE_128KB - 1)

/* Exported functions --------------------------------------------------------*/

 typedef struct
 {
 	// 8 bytes
 	uint16_t primarySerial;
 	uint8_t primaryLicence;
 	uint8_t revision8bit;
 	uint8_t production_year;
 	uint8_t production_month;
 	uint8_t production_day;
 	uint8_t production_bluetooth_name_set;

 	// 44 bytes
 	char production_info[44];

 	// 8 bytes
 	uint16_t secondarySerial;
 	uint8_t secondaryLicence;
 	uint8_t secondaryReason8bit;
 	uint8_t secondary_year;
 	uint8_t secondary_month;
 	uint8_t secondary_day;
 	uint8_t secondary_bluetooth_name_set;

 	// 4 bytes
 	char secondary_info[4];
 } SHardwareData;


uint32_t CalcFletcher32(uint32_t startAddr, uint32_t endAddr);

uint8_t firmware_eraseFlashMemory(void);
uint8_t firmware_programFlashMemory(uint8_t *pBuffer1, uint32_t length1);//, uint8_t *pBuffer2, uint32_t length2)

uint8_t bootloader_eraseFlashMemory(void);
uint8_t bootloader_programFlashMemory(uint8_t *pBuffer1, uint32_t length1, SHardwareData* pHwInfo);

uint8_t firmware2_variable_upperpart_eraseFlashMemory(uint32_t length, uint32_t offset);
uint8_t firmware2_variable_upperpart_programFlashMemory(uint32_t length, uint32_t offset, uint8_t *pBuffer1, uint32_t pBuffer1Size, uint8_t *pBuffer2);

uint8_t hardware_programmProductionData(uint8_t *buffer52); 	// uint16_t serial, uint16_t revision, 	uint8_t year, uint8_t month, uint8_t day, uint8_t sub, uint8_t *info[48]
uint8_t hardware_programmSecondarySerial(uint8_t *buffer12); 	// uint16_t serial, uint16_t reason, 		uint8_t year, uint8_t month, uint8_t day, uint8_t sub
uint8_t hardware_programmPrimaryBluetoothNameSet(void);
uint8_t hardware_programmSecondaryBluetoothNameSet(void);

#ifdef __cplusplus
 }
#endif
#endif // FIRMWARE_ERASE_PROGRAM_H
