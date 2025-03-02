///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/simulation.h
/// \brief
/// \author Heinrichs Weikamp
/// \date   2018
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

#ifndef SIMULATION_H
#define SIMULATION_H

#include "stm32f4xx_hal.h"
#include "data_central.h"

#define GAS_CHANGE_LIST_ITEMS	(10)
typedef struct
{
	uint8_t depth;
	uint8_t gasId;
} SgasChangeList;

typedef struct
{
    uint8_t depthMeterFirstStop;
    uint8_t descentRateMeterPerMinute;
    uint8_t ascentRateMeterPerMinute;
    uint16_t timeToBottom;
    uint16_t timeAtBottom;
    uint16_t timeToFirstStop;
    uint16_t timeToSurface;
    float	ppO2AtBottom;
} SSimDataSummary;

void simulation_start(int aim_depth, uint16_t aim_time_minutes);
void simulation_exit(void);

void simulation_set_heed_decostops(_Bool heed_decostops_while_ascending);

void simulation_UpdateLifeData( _Bool checkOncePerSecond);
void simulation_set_zero_time_descent(void);

uint16_t simulation_get_aim_depth(void);
_Bool simulation_get_heed_decostops(void);
SDecoinfo* simulation_decoplaner(uint16_t depth_meter, uint16_t intervall_time_minutes, uint16_t dive_time_minutes, SgasChangeList *pGasChangeList);
SDecoinfo* simulation_decoplaner_Bachelorarbeit_VPM(uint16_t depth_meter, uint16_t intervall_time_minutes, uint16_t dive_time_minutes, SgasChangeList *pGasChangeList);
void simulation_gas_consumption(uint16_t *outputConsumptionList, uint16_t depth_meter, uint16_t dive_time_minutes, SDecoinfo *decoInfoInput, uint8_t gasConsumTravelInput, uint8_t gasConsumDecoInput, const SgasChangeList *pGasChangeList);
void simulation_helper_change_points(SSimDataSummary *outputSummary, uint16_t depth_meter, uint16_t dive_time_minutes, SDecoinfo *decoInfoInput, const SgasChangeList *pGasChangeList);


void Sim_Descend (void);
void Sim_Ascend (void);
void Sim_Divetime (void);
void Sim_Quit (void);

void Sim_IncreasePPO(uint8_t sensorIdx);
void Sim_DecreasePPO(uint8_t sensorIdx);


#endif /* SIMULATION_H */
