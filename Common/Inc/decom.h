///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Common/Inc/decom.h
/// \brief  This code is used to calculate desat, calculated by RTE and send to Firmware
/// \author Heinrichs Weikamp
/// \date   22-Feb-2016
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

#ifndef DECOM_H
#define DECOM_H

#include "data_central.h"

#define	WATER_VAPOUR_PRESSURE	(0.0493f) // Schreiner 1971
#define	FRACTION_N2_AIR			0.7902

extern const float helium_time_constant[];
extern const float nitrogen_time_constant[];

extern const float buehlmann_N2_a[];
extern const float buehlmann_N2_b[];

extern const float buehlmann_He_a[];
extern const float buehlmann_He_b[];

extern const float buehlmann_N2_t_halflife[];
extern const float buehlmann_He_t_halflife[];


void decom_get_inert_gases(const float ambient_pressure_bar,const SGas* pGas, float* fraction_nitrogen, float* fraction_helium );
void decom_tissues_exposure(int period_in_seconds, SLifeData* pLifeData);
void decom_tissues_exposure2(int period_in_seconds, SGas* pActualGas, float pressure_ambient_bar, float *tissue_N2_selected_stage, float *tissue_He_selected_stage);
float decom_schreiner_equation(float initial_inspired_gas_pressure, float rate_change_insp_gas_pressure, float interval_time_minutes, const float gas_time_constant, float initial_gas_pressure);
void decom_reset_with_1000mbar(SLifeData * pLifeData);
void decom_reset_with_ambientmbar(float ambient, SLifeData * pLifeData);

void decom_tissues_exposure_stage_schreiner(int period_in_seconds, SGas* pGas, float  starting_ambient_pressure_bar, float ending_ambient_pressure_bar,
																		 float* pTissue_nitrogen_bar, float* pTissue_helium_bar);
void decom_CreateGasChangeList(SDiveSettings* pInput, const SLifeData* lifeData);
uint8_t decom_tissue_test_tolerance(float* Tissue_nitrogen_bar, float* Tissue_helium_bar, float GF_value, float depth_in_bar_absolute);
void decom_tissues_desaturation_time(const SLifeData* pLifeData, SLifeData2* pOutput);
void test_decom_CreateGasChangeList(void);

float decom_calc_ppO2(const float ambiant_pressure_bar, const SGas* pGas);
float decom_calc_SimppO2(float ambiant_pressure_bar, const SGas* pGas);
float decom_calc_SimppO2_O2based(float ambiant_pressure_bar, uint8_t O2PerCent, float factor);
void decom_oxygen_calculate_otu(float* oxygen_otu, float pressure_oxygen_real);
void decom_oxygen_calculate_otu_degrade(float* oxygen_otu, long seconds_since_last_dive);
void decom_oxygen_calculate_cns_degrade(float* oxygen_cns, long seconds_since_last_dive);
void decom_oxygen_calculate_cns(float* oxygen_cns, float pressure_oxygen_real);
void decom_oxygen_calculate_cns_stage_SchreinerStyle(int period_in_seconds, SGas* pGas, float  starting_ambient_pressure_bar, float ending_ambient_pressure_bar, float* oxygen_cns);
void decom_oxygen_calculate_cns_exposure(int period_in_seconds, SGas* pActualGas, float pressure_ambient_bar, float* oxygen_cns);
uint8_t decom_get_actual_deco_stop(SDiveState* pDiveState);

// wird nur in smallCPU verwendet:
int decom_calc_desaturation_time(float* Tissue_nitrogen_bar, float* Tissue_helium_bar, float surface_pressure_bar);

#endif /* DECOM_H */

