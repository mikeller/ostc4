///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/data_exchange_main.h
/// \brief  Header file for communciation of Master with second CPU
/// \author heinrichs weikamp gmbh
/// \date   13-Oct-2014
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
#ifndef DATA_EXCHANGE_MAIN_H
#define DATA_EXCHANGE_MAIN_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "data_exchange.h"

/* Exported functions --------------------------------------------------------*/

void DataEX_set_update_RTE_not_power_on(void);
uint8_t DataEX_was_power_on(void);
void DataEX_init(void);
uint8_t DataEX_call(void);
uint32_t DataEX_time_elapsed_ms(uint32_t ticksstart,uint32_t ticksnow);

uint32_t get_num_SPI_CALLBACKS(void);
SDataExchangeSlaveToMaster* get_dataInPointer(void);
void DataEX_copy_to_LifeData(_Bool *modeChangeFlag);
void DataEX_copy_to_deco(void);
void DateEx_copy_to_dataOut(void);
void DataEX_merge_devicedata(void);
uint32_t DataEX_lost_connection_count(void);
void DataEX_control_connection_while_asking_for_sleep(void);
uint8_t DataEX_check_RTE_version__needs_update(void);
void setAvgDepth(SDiveState *pStateReal);
uint8_t DataEX_external_ADC_Present(void);

SDataReceiveFromMaster * dataOutGetPointer(void);

uint16_t DataEX_debug_data(uint16_t *dataOut20x5);

void DataEX_setExtInterface_Cmd(uint16_t Cmd,uint8_t sensorId);

#endif /* DATA_EXCHANGE_MAIN_H */
