///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tComm.h
/// \brief  Header file communication with PC
/// \author heinrichs weikamp gmbh
/// \date   08-Aug-2014
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
#ifndef TCOMM_H
#define TCOMM_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>


/* types */
typedef enum
{
		BM_CONFIG_OFF = 0,
		BM_CONFIG_ECHO,
		BM_CONFIG_ESCAPE_DELAY,
		BM_CONFIG_SIGNAL_POLL,
		BM_CONFIG_BAUD,
		BM_CONFIG_SILENCE,
		BM_CONFIG_DONE,
		BM_CONFIG_RETRY,
#ifdef BOOTLOADER_STANDALONE
		BM_INIT_TRIGGER_ON = 100,
		BM_INIT_TRIGGER_OFF,
		BM_INIT_ECHO,
		BM_INIT_FACTORY,
		BM_INIT_MODE,
		BM_INIT_BLE,
		BM_INIT_NAME,
		BM_INIT_SSP_IDO_OFF,
		BM_INIT_SSP_IDO_ON,
		BM_INIT_SSP_ID1_OFF,
		BM_INIT_SSP_ID1_ON,
		BM_INIT_STORE,
		BM_INIT_RESTART,
		BM_INIT_DONE
#endif
} BlueModTmpConfig_t;

/* Exported functions --------------------------------------------------------*/

void tComm_init(void);
uint8_t tComm_control(void);
void tComm_refresh(void);
void tComm_exit(void);
void tComm_verlauf(uint8_t percentage_complete);
uint8_t tComm_Set_Bluetooth_Name(uint8_t force);
void tComm_StartBlueModBaseInit(void);
void tComm_StartBlueModConfig(void);
void tComm_RequestBluetoothStrength(void);

#endif /* TCOMM_H */
