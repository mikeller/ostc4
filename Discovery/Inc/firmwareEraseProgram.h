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

#endif // FIRMWARE_ERASE_PROGRAM_H
