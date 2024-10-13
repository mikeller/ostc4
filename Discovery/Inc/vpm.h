///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/vpm.h
/// \brief  Varying Permeability Model (VPM) Decompression Program
/// \author Heinrichs Weikamp
/// \date   23.04.2013
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

#ifndef VPM_H
#define VPM_H

#include "buehlmann.h"
#include "data_central.h"

extern long vpm_time_calc_begin;
enum DECOLIST{DECOSTOPS,FUTURESTOPS,BAILOUTSTOPS, OFF = -1}; // order is important!!
enum VPM_CALC_STATUS{CALC_END, CALC_BEGIN, CALC_CRITICAL, CALC_FINAL_DECO, CALC_NDL };

typedef enum
{
	VPM_TABLE_INIT = 0,
	VPM_TABLE_ACTIVE,
	VPM_TABLE_WARNING,
	VPM_TABLE_MISSED
} SvpmTableState;

float schreiner_equation__2(float *initial_inspired_gas_pressure,float *rate_change_insp_gas_pressure,float *interval_time_minutes,  const float *gas_time_constant,float *initial_gas_pressure);

int  vpm_calc(SLifeData* pINPUT, SDiveSettings* diveSettings, SVpm* pVPM, SDecoinfo*  pDECOINFO, int calc_what);
void  vpm_saturation_after_ascent(SLifeData* input);
extern  const float helium_time_constant[16];
extern  const float nitrogen_time_constant[16];

float vpm_get_CNS(void);

void vpm_table_init(void);
uint8_t vpm_get_decozone(void);
SvpmTableState vpm_get_TableState(void);


#endif /* VPM_H */
