///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/simulation.c
/// \brief  Contains dive simulation functionality
/// \author Heinrichs Weikamp gmbh
/// \date   13-Oct-2014
///
/// \details
///     The simulation uses "extern SDiveState stateSim" defined in dataCentral.h"
///
///     simulation_start(void) sets stateUsed to stateSim and initializes simulation
///     simulation_UpdateLifeData should be called at least once per second
///     simulation_end() sets stateUsed back to stateReal
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

#include <string.h>
#include "simulation.h"

#include "decom.h"
#include "calc_crush.h"
#include "data_exchange.h"
#include "data_exchange_main.h"
#include "timer.h"
#include "check_warning.h"
#include "vpm.h"
#include "buehlmann.h"
#include "logbook_miniLive.h"
#include "logbook.h"

#include "configuration.h"

//Private state variables
static float sim_aim_depth_meter;
static float sim_aim_time_minutes;
static _Bool sim_heed_decostops = 1;

static float sim_descent_rate_meter_per_min = 20;

static uint16_t* pReplayData; /* pointer to source dive data */
static uint8_t simReplayActive = 0;

static uint16_t simScrubberTimeoutCount = 0;


//Private functions
static float sim_get_ambient_pressure(SDiveState * pDiveState);
static void sim_reduce_deco_time_one_second(SDiveState* pDiveState);
static void simulation_set_aim_depth(int depth_meter);

#define NUM_OF_SENSORS	(3u)
#define SIM_PPO2_STEP	(1.1f)
static float simSensmVOffset[NUM_OF_SENSORS];

/**
  ******************************************************************************
  * @brief  sets heed_decostops_while_ascending
  ******************************************************************************
  * @param heed_decostops_while_ascending : true -> deco_stops are considered while ascending
  * @return void
  */
void simulation_set_heed_decostops(_Bool heed_decostops_while_ascending)
{
	sim_heed_decostops = heed_decostops_while_ascending;
}

/**
  ******************************************************************************
  * @brief  start of simulation
  ******************************************************************************
  * @return void
  */
void simulation_start(int aim_depth, uint16_t aim_time_minutes)
{
    uint16_t replayDataLength = 0;
    uint8_t* pReplayMarker;
    uint16_t max_depth = 10;
    uint16_t diveMinutes = 0;

	copyDiveSettingsToSim();
    copyVpmRepetetiveDataToSim();

  //vpm_init(&stateSimGetPointerWrite()->vpm,  stateSimGetPointerWrite()->diveSettings.vpm_conservatism, 0, 0);
    stateSimGetPointerWrite()->lifeData.counterSecondsShallowDepth = 0;
    stateSimGetPointerWrite()->mode = MODE_DIVE;
    if(aim_depth <= 0)
        aim_depth = 20;
    sim_descent_rate_meter_per_min = 20;
    simulation_set_aim_depth(aim_depth);
    sim_aim_time_minutes = aim_time_minutes;
    timer_init();
    set_stateUsedToSim();
    stateSim.lifeData.boolResetAverageDepth = 1;
    decoLock = DECO_CALC_init_as_is_start_of_dive;

    stateSim.lifeData.apnea_total_max_depth_meter = 0;

    SSettings *settings = settingsGetPointer();
    memcpy(stateSim.scrubberDataDive, settings->scrubberData, sizeof(stateSim.scrubberDataDive));
    memset(simSensmVOffset,0,sizeof(simSensmVOffset));
   	if(getReplayOffset() != 0xFFFF)
   	{
   		simReplayActive = 1;
		getReplayInfo(&pReplayData, &pReplayMarker, &replayDataLength, &max_depth, &diveMinutes);
   	}
}

/**
  ******************************************************************************
  * @brief  end of simulation
  ******************************************************************************
  *
  * @return void
  */
void simulation_exit(void)
{
    timer_Stopwatch_Stop();

    disableTimer();

    set_stateUsedToReal();
}

/**
  ******************************************************************************
  * @brief  simulates change of Lifedata (saturation, depth change, etc.) within one second
  ******************************************************************************
  *
  * @param  checkOncePerSecond : true -> simulation in real time (function is evaluated only once per second)
  *																			 and copy of parts of LifeData from SmallCPU with each call from HAL_TIM_PeriodElapsedCallback()
  *                            : false -> fast simulation (many simulation cycles per second are possible)
  * @return void
  */
void simulation_UpdateLifeData( _Bool checkOncePerSecond)
{
    SDiveState * pDiveState = &stateSim;
    const SDiveState * pRealState = stateRealGetPointer();
	SSettings *pSettings;

    static int last_second = -1;
    static _Bool two_second = 0;
    static float lastPressure_bar = 0;

    pSettings = settingsGetPointer();

    if ((sim_aim_time_minutes && sim_aim_time_minutes * 60 <= pDiveState->lifeData.dive_time_seconds)
    		&& (!simReplayActive))
    {
        simulation_set_aim_depth(0);
    }

    float localCalibCoeff[3] = { 0.0, 0.0, 0.0 };
    uint8_t index, index2;

    if(checkOncePerSecond)
    {
        int now =  current_second();
        if( last_second == now)
                return;
        last_second = now;

        if(!two_second)
            two_second = 1;
        else
        {
            two_second = 0;
        }

        for(index = 0; index < 3; index++)
        {
        	if(pDiveState->lifeData.extIf_sensor_map[index] == SENSOR_DIGO2M)
        	{
        		localCalibCoeff[index] = 0.01;
        	}
        	else
        	{
				localCalibCoeff[index] = pSettings->ppo2sensors_calibCoeff[index];
				if(localCalibCoeff[index] < 0.01)
				{
					for(index2 = 0; index2 < 3; index2++)		/* no valid coeff => check other entries */
					{
						if(pSettings->ppo2sensors_calibCoeff[index2] > 0.01)
						{
							localCalibCoeff[index] = pSettings->ppo2sensors_calibCoeff[index2];
							break;
						}
						if(index2 == 3)		/* no coeff at all => use default */
						{
							localCalibCoeff[index] = 0.02;
						}
					}
				}
        	}
        }

        pDiveState->lifeData.temperature_celsius = pRealState->lifeData.temperature_celsius;
        pDiveState->lifeData.battery_charge = pRealState->lifeData.battery_charge;
        pDiveState->lifeData.compass_heading = pRealState->lifeData.compass_heading;
        pDiveState->lifeData.compass_roll = pRealState->lifeData.compass_roll;
        pDiveState->lifeData.compass_pitch = pRealState->lifeData.compass_pitch;

        for(index = 0; index < 3; index++)
        {
        	memcpy(&pDiveState->lifeData.extIf_sensor_data[index], &pRealState->lifeData.extIf_sensor_data[index], 32);
        }

#ifdef ENABLE_BOTTLE_SENSOR
        pDiveState->lifeData.bottle_bar[pDiveState->lifeData.actualGas.GasIdInSettings] = pRealState->lifeData.bottle_bar[pRealState->lifeData.actualGas.GasIdInSettings];
        pDiveState->lifeData.bottle_bar_age_MilliSeconds[pDiveState->lifeData.actualGas.GasIdInSettings] = pRealState->lifeData.bottle_bar_age_MilliSeconds[pRealState->lifeData.actualGas.GasIdInSettings];
#endif
    }
    else if(pDiveState->lifeData.depth_meter <= (float)(decom_get_actual_deco_stop(pDiveState) + 0.001))
    {
    	if(decoLock == DECO_CALC_FINSHED_vpm)
    	{
    		sim_reduce_deco_time_one_second(&stateDeco);
    	}
    	else
    	{
    		sim_reduce_deco_time_one_second(pDiveState);
    	}
    }

    pDiveState->lifeData.dive_time_seconds += 1;
    pDiveState->lifeData.pressure_ambient_bar = sim_get_ambient_pressure(pDiveState);
    if(pDiveState->lifeData.depth_meter < 1.5)
    {
    	lastPressure_bar = 0;
    	pDiveState->lifeData.ascent_rate_meter_per_min = 0;
    }

    if (isScrubberTimerRunning(pDiveState, pSettings)) {
        simScrubberTimeoutCount++;

        if (simScrubberTimeoutCount >= 60) {
            /* resolution is minutes */
            simScrubberTimeoutCount = 0;

            int16_t maxScrubberTime = INT16_MIN;
            SScrubberData *longestScrubberData = NULL;
            for (unsigned timerId = 0; timerId < 2; timerId++) {
                if (pSettings->scrubberActiveId & (1 << timerId)) {
                    SScrubberData *scrubberData = &pDiveState->scrubberDataDive[timerId];
                    if (scrubberData->TimerCur > MIN_SCRUBBER_TIME) {
                        scrubberData->TimerCur--;
                    }

                    if (scrubberData->TimerCur > maxScrubberTime) {
                        maxScrubberTime = scrubberData->TimerCur;
                        longestScrubberData = scrubberData;
                    }

                    translateDate(stateUsed->lifeData.dateBinaryFormat, &scrubberData->lastDive);
                }
            }

            logScrubberState(longestScrubberData);
        }
    }

    if(lastPressure_bar > 0)
     {
         //1 second * 60 == 1 minute, bar * 10 = meter
         pDiveState->lifeData.ascent_rate_meter_per_min = (lastPressure_bar - pDiveState->lifeData.pressure_ambient_bar)  * 600.0;
     }
     lastPressure_bar = pDiveState->lifeData.pressure_ambient_bar;

    pDiveState->lifeData.sensorVoltage_mV[0] = pRealState->lifeData.sensorVoltage_mV[0] + simSensmVOffset[0];
    if(pDiveState->lifeData.sensorVoltage_mV[0] < 0.0) { pDiveState->lifeData.sensorVoltage_mV[0] = 0.0; }
    pDiveState->lifeData.sensorVoltage_mV[1] = pRealState->lifeData.sensorVoltage_mV[1] + simSensmVOffset[1];
    if(pDiveState->lifeData.sensorVoltage_mV[1] < 0.0) { pDiveState->lifeData.sensorVoltage_mV[1] = 0.0; }
    pDiveState->lifeData.sensorVoltage_mV[2] = pRealState->lifeData.sensorVoltage_mV[2] + simSensmVOffset[2];
    if(pDiveState->lifeData.sensorVoltage_mV[2] < 0.0) { pDiveState->lifeData.sensorVoltage_mV[2] = 0.0; }

    pDiveState->lifeData.ppO2Sensor_bar[0]  = pDiveState->lifeData.sensorVoltage_mV[0] * localCalibCoeff[0] * pDiveState->lifeData.pressure_ambient_bar;
    pDiveState->lifeData.ppO2Sensor_bar[1]  = pDiveState->lifeData.sensorVoltage_mV[1] * localCalibCoeff[1] * pDiveState->lifeData.pressure_ambient_bar;
    pDiveState->lifeData.ppO2Sensor_bar[2]  = pDiveState->lifeData.sensorVoltage_mV[2] * localCalibCoeff[2] * pDiveState->lifeData.pressure_ambient_bar;

    pDiveState->lifeData.CO2_data.CO2_ppm  = pRealState->lifeData.CO2_data.CO2_ppm;

    if(is_ambient_pressure_close_to_surface(&pDiveState->lifeData)) // new hw 170214
    {
        if(!(stateSimGetPointer()->lifeData.counterSecondsShallowDepth))
        {
            if(pDiveState->diveSettings.diveMode != DIVEMODE_Apnea)
            {
                pDiveState->lifeData.counterSecondsShallowDepth = settingsGetPointer()->timeoutDiveReachedZeroDepth - 15;
                if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
                {
					if((stateDeco.decolistBuehlmann.gf_surf * 100.0) < 255.0)
					{
						pDiveState->lifeData.gf_surf_log = stateDeco.decolistBuehlmann.gf_surf *100.0;
					}
					else
					{
						pDiveState->lifeData.gf_surf_log = 255;
					}
                }
            }
            else
            {
                pDiveState->lifeData.apnea_last_dive_time_seconds = pDiveState->lifeData.dive_time_seconds;
                if(pDiveState->lifeData.apnea_last_dive_time_seconds > pDiveState->lifeData.dive_time_seconds_without_surface_time)
                    pDiveState->lifeData.apnea_last_dive_time_seconds = pDiveState->lifeData.dive_time_seconds_without_surface_time;
                pDiveState->lifeData.apnea_last_max_depth_meter = pDiveState->lifeData.max_depth_meter;
                pDiveState->lifeData.counterSecondsShallowDepth = 1;
            }
        }
    }
    else
    {
        pDiveState->lifeData.counterSecondsShallowDepth = 0;
    }

    if(!is_ambient_pressure_close_to_surface(&pDiveState->lifeData) && !(stateSimGetPointer()->lifeData.counterSecondsShallowDepth) )
    {
    	pDiveState->lifeData.dive_time_seconds_without_surface_time += 1;
    }

    pDiveState->lifeData.depth_meter = (pDiveState->lifeData.pressure_ambient_bar - pDiveState->lifeData.pressure_surface_bar) * 10.0f;
    if(pDiveState->lifeData.max_depth_meter < pDiveState->lifeData.depth_meter)
            pDiveState->lifeData.max_depth_meter = pDiveState->lifeData.depth_meter;

    /* apnoe specials
     */
    if(pDiveState->diveSettings.diveMode == DIVEMODE_Apnea)
    {
        if(pDiveState->lifeData.max_depth_meter > pDiveState->lifeData.apnea_total_max_depth_meter)
            pDiveState->lifeData.apnea_total_max_depth_meter = pDiveState->lifeData.max_depth_meter;

        if(pDiveState->lifeData.counterSecondsShallowDepth)
        {
            pDiveState->lifeData.dive_time_seconds = 0;
            pDiveState->lifeData.max_depth_meter = 0;
            pDiveState->lifeData.boolResetAverageDepth = 1;
        }
    }

    setAvgDepth(pDiveState);

    /* Exposure Tissues
     */
    decom_tissues_exposure(1, &pDiveState->lifeData);
    decom_oxygen_calculate_cns_exposure(1, &pDiveState->lifeData.actualGas, pDiveState->lifeData.pressure_ambient_bar, &pDiveState->lifeData.cns);

    if(stateSimGetPointer()->lifeData.counterSecondsShallowDepth)
    {
            stateSimGetPointerWrite()->lifeData.counterSecondsShallowDepth += 1;
            if(stateSimGetPointer()->lifeData.counterSecondsShallowDepth >= settingsGetPointer()->timeoutDiveReachedZeroDepth)
            {
#ifdef SIM_WRITES_LOGBOOK
				pDiveState->mode = MODE_SURFACE;
				logbook_InitAndWrite((SDiveState*)pDiveState);
#endif
                simulation_exit();
            }
    }
    vpm_crush(pDiveState);
}

/**
  ******************************************************************************
  * @brief  adds extra time for fast simulation
  ******************************************************************************
  *@param minutes
  * @return float : new pressure
  */
static void simulation_add_time(int minutes)
{
  for(int i = 0; i < 60 * minutes; i++)
  {
    simulation_UpdateLifeData(0);
    updateMiniLiveLogbook(0);
        timer_UpdateSecond(0);
  }
}

/**
  ******************************************************************************
  * @brief  get aim_depth
  ******************************************************************************
  * @return sim_aim_depth_meter;
  */

uint16_t simulation_get_aim_depth(void)
{
    return (uint16_t)sim_aim_depth_meter;
}

/**
  ******************************************************************************
  * @brief  get heed decostops
  ******************************************************************************
  * @return true if ascend follows decostops;
  */

_Bool simulation_get_heed_decostops(void)
{
    return sim_heed_decostops;
}

/**
  ******************************************************************************
  * @brief sets aim_depth
  ******************************************************************************
  *@param depth_meter
  * @return float : new pressure
  */
static void simulation_set_aim_depth(int depth_meter)
{
    sim_aim_depth_meter = depth_meter;
}

/**
  ******************************************************************************
  * @brief  simulates ambient pressure depending on aim depth
  ******************************************************************************
  * @note if aim_depth != actual depth, the depth change within one second
  *       (depending on descent or ascent) rate is calculated
  * @param  SDiveState* pDiveState:
  * @return float : new ambient pressure
  */
static float sim_get_ambient_pressure(SDiveState * pDiveState)
{
    //Calc next depth
    uint8_t actual_deco_stop = decom_get_actual_deco_stop(pDiveState);
    float depth_meter = pDiveState->lifeData.depth_meter;
    float surface_pressure_bar = pDiveState->lifeData.pressure_surface_bar;
    static uint8_t sampleToggle = 0;
	static float sim_ascent_rate_meter_per_min_local = 0;
	uint8_t sampleTime = getReplayDataResolution();

    if(simReplayActive) /* precondition: function is called once per second, sample rate is a multiple of second */
    {
    	if(sampleToggle == 0)
    	{
    		sampleToggle = sampleTime - 1;
    		sim_aim_depth_meter = (float)(*pReplayData++/100.0);
    		if(sim_aim_depth_meter > depth_meter)
    		{
    			sim_descent_rate_meter_per_min = (sim_aim_depth_meter - depth_meter) * (60 / sampleTime);
    		}
    		else
    		{
    			sim_ascent_rate_meter_per_min_local = (depth_meter - sim_aim_depth_meter) * (60 / sampleTime);
    		}
    	}
    	else
    	{
    		sampleToggle--;
    	}
    }
    else
    {
    	sim_ascent_rate_meter_per_min_local = pDiveState->diveSettings.ascentRate_meterperminute;
    }

    if(depth_meter < sim_aim_depth_meter)
    {
        depth_meter = depth_meter + sim_descent_rate_meter_per_min / 60;
        if(depth_meter > sim_aim_depth_meter)
            depth_meter = sim_aim_depth_meter;
    }
    else if(depth_meter > sim_aim_depth_meter)
    {

        depth_meter -=  sim_ascent_rate_meter_per_min_local / 60;
        if(depth_meter < sim_aim_depth_meter)
            depth_meter = sim_aim_depth_meter;

        if(sim_heed_decostops && depth_meter < actual_deco_stop)
        {
            if(actual_deco_stop < (depth_meter +  sim_ascent_rate_meter_per_min_local / 60))
                 depth_meter = actual_deco_stop;
            else
                depth_meter += sim_ascent_rate_meter_per_min_local / 60;
        }

   }

   return surface_pressure_bar + depth_meter / 10;
}


/**
  ******************************************************************************
  * @brief  Reduces deco time of deepest stop by one second
  ******************************************************************************
  * @note called during fast simulation
  * @param  SDiveState* pDiveState:
  * @return void
  */
static void sim_reduce_deco_time_one_second(SDiveState* pDiveState)
{
    SDecoinfo* pDecoinfo;
    int8_t index = 0;


    if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
        pDecoinfo = &pDiveState->decolistBuehlmann;
    else
        pDecoinfo = &pDiveState->decolistVPM;

    //Reduce deco time of deepest stop by one second
    for(index = DECOINFO_STRUCT_MAX_STOPS -1 ;index >= 0; index--)
    {
        if(pDecoinfo->output_stop_length_seconds[index] > 0)
        {
            pDecoinfo->output_stop_length_seconds[index]--;
            break;
        }
    }
    /* update TTS */
    if(pDecoinfo->output_time_to_surface_seconds)
    {
    	pDecoinfo->output_time_to_surface_seconds--;
    }
}

SDecoinfo* simulation_decoplaner(uint16_t depth_meter, uint16_t intervall_time_minutes, uint16_t dive_time_minutes, SgasChangeList *pGasChangeList)
{
    uint8_t GasChangeIndex = 0;

    for (GasChangeIndex = 0; GasChangeIndex < GAS_CHANGE_LIST_ITEMS; GasChangeIndex++)
    {
    	pGasChangeList[GasChangeIndex].depth = 0;
    	pGasChangeList[GasChangeIndex].gasId = 0;
    }
    SDiveState * pDiveState = &stateSim;
    copyDiveSettingsToSim();

    vpm_init(&pDiveState->vpm,  pDiveState->diveSettings.vpm_conservatism, 0, 0);
    //buehlmann_init();
    //timer_init();
    memset(&pDiveState->events,0, sizeof(SEvents));
    pDiveState->diveSettings.internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero = 0;
    //Calc desaturation during intervall (with Air)
    setActualGasAir(&pDiveState->lifeData);
    if(intervall_time_minutes > 0)
    {
        decom_tissues_exposure(intervall_time_minutes * 60, &pDiveState->lifeData);
        decom_oxygen_calculate_cns_degrade(&pDiveState->lifeData.cns, intervall_time_minutes * 60);
    }

    //Switch to first Gas
    setActualGasFirst(&pDiveState->lifeData);

    GasChangeIndex = 0;

    if(pGasChangeList)
    {
    	pGasChangeList[GasChangeIndex].depth = 0;
    	pGasChangeList[GasChangeIndex].gasId = pDiveState->lifeData.actualGas.GasIdInSettings;
    	GasChangeIndex++;
    }

    //Going down / descent
    simulation_set_aim_depth(depth_meter);
    sim_aim_time_minutes = 0;
    for(int i = 0; i < 60 * dive_time_minutes; i++)
    {
        simulation_UpdateLifeData(0);
        check_warning2(pDiveState);
        if(pDiveState->warnings.betterGas)
        {
            setActualGas(&pDiveState->lifeData,actualBetterGasId(),pDiveState->lifeData.actualGas.setPoint_cbar);
            if(pGasChangeList && (pDiveState->diveSettings.diveMode == DIVEMODE_OC))
            {
            	pGasChangeList[GasChangeIndex].depth = pDiveState->lifeData.depth_meter;
            	pGasChangeList[GasChangeIndex].gasId = actualBetterGasId();
            	GasChangeIndex++;
            }
        }
    }

    decom_CreateGasChangeList(&pDiveState->diveSettings, &pDiveState->lifeData); // was there before and needed for buehlmann_calc_deco and vpm_calc

    if(pGasChangeList && (pDiveState->diveSettings.diveMode == DIVEMODE_OC))
    {
        // change direction from better gas to deco gas
    	pGasChangeList[GasChangeIndex].depth = 255;
    	pGasChangeList[GasChangeIndex].gasId = 255;
    	GasChangeIndex++;

        // ascend (deco) gases
        for(int i=1; i<=5;i++)
        {
            if((pDiveState->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero != 0)
            		&& (pDiveState->diveSettings.gas[pDiveState->diveSettings.decogaslist[i].GasIdInSettings].note.ub.deco))
            {
               pGasChangeList[GasChangeIndex].depth = pDiveState->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero;
               pGasChangeList[GasChangeIndex].gasId = pDiveState->diveSettings.decogaslist[i].GasIdInSettings;
               GasChangeIndex++;
            }
        }
    }

    // deco and ascend calc
    if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
    {
        /* this does modify the cns now 11.06.2015 */
        buehlmann_calc_deco(&pDiveState->lifeData,&pDiveState->diveSettings,&pDiveState->decolistBuehlmann);
        pDiveState->lifeData.cns += buehlmann_get_gCNS();
        return &pDiveState->decolistBuehlmann;
    }
    else
    {
        /* this does modify the cns now 11.06.2015 */
        vpm_calc(&pDiveState->lifeData,&pDiveState->diveSettings,&pDiveState->vpm,&pDiveState->decolistVPM, DECOSTOPS);
        pDiveState->lifeData.cns += vpm_get_CNS();

        while(decoLock == DECO_CALC_FINSHED_vpm)
        {
        	HAL_Delay(2);	/* The deco data is copied during the timer ISR => wait till this has happened */
        }
        return &pDiveState->decolistVPM;
    }
}

static float sGChelper_bar(uint16_t depth_meter)
{
    SDiveState * pDiveState = &stateSim;
    float ambient, surface, density, meter;

    surface = pDiveState->lifeData.pressure_surface_bar;

    if(!depth_meter)
        return surface;

    density = ((float)( 100 + settingsGetPointer()->salinity)) / 100.0f;
    meter = depth_meter * (0.09807f * density);
    ambient = (meter + surface);

    return ambient;
}

void getNextDecoDepthAndTime(uint8_t* pDepth, uint16_t* pTime, uint8_t currentDepth, SDecoinfo *decoInfoInput)
{
    uint8_t depthLast, depthSecond, depthInc;
    uint8_t decoIndex = 0;

    depthLast 		= (uint8_t)(stateUsed->diveSettings.last_stop_depth_bar * 10);
    depthSecond 	= (uint8_t)(stateUsed->diveSettings.input_second_to_last_stop_depth_bar * 10);
    depthInc 			= (uint8_t)(stateUsed->diveSettings.input_next_stop_increment_depth_bar * 10);

    if(currentDepth > depthLast)
    {
		 for(decoIndex = DECOINFO_STRUCT_MAX_STOPS-1; decoIndex > 0; decoIndex--)
		 {
			 if(decoInfoInput->output_stop_length_seconds[decoIndex])
			 {
				 *pDepth = depthSecond + ( decoIndex - 1 ) * depthInc;
				 if(*pDepth < currentDepth)
				 {
					 break;
				 }
			 }
		 }

		 if(decoIndex == 0)
		 {
			 *pDepth = depthLast;
		 }
		 *pTime = decoInfoInput->output_stop_length_seconds[decoIndex];
    }
    else
    {
    	*pDepth = 0;
    	*pTime = 0;
    }
}

void simulation_evaluate_profil(uint16_t *outputConsumptionList,
								SSimDataSummary *outputSummary,
								uint16_t depth_meter, uint16_t dive_time_minutes,uint8_t gasConsumTravelInput, uint8_t gasConsumDecoInput,
								SDecoinfo *decoInfoInput,
								const SgasChangeList *pGasChangeList)
{
    uint16_t nextDecoTime = 0;
    uint8_t nextDecoDepth = 0;

    uint8_t currentConsumGasId = 0;
    uint8_t nextGasChangeMeter = 0;
    uint8_t nextGasChangeGasId = 0;
    uint8_t ChangeListIndex = 0;
    uint8_t firstDecoGasIndex = 0;
    float outputConsumptionTempFloat[6];

    float	sim_descent_rate_meter_per_sec_local = 10.0;
    float	sim_ascent_rate_meter_per_sec_local = 10.0;

    float currentDepth_m = 0.0;
    uint16_t  currentTime_sec = 0;
    float currentGasConsumption = 0.0;

    SDiveState * pDiveState = &stateSim;

    for(ChangeListIndex = 0; ChangeListIndex < 6; ChangeListIndex++)
    {
    	outputConsumptionTempFloat[ChangeListIndex] = 0.0;
    }

    if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
    {
        sim_descent_rate_meter_per_sec_local = sim_descent_rate_meter_per_min / 60.0;
        sim_ascent_rate_meter_per_sec_local = pDiveState->diveSettings.ascentRate_meterperminute / 60.0;
    }
    else
    {
        sim_descent_rate_meter_per_sec_local = sim_descent_rate_meter_per_min / 60.0;
        sim_ascent_rate_meter_per_sec_local = 10.0 / 60.0; // fix in vpm_calc_deco();
    }

    outputSummary->descentRateMeterPerMinute = sim_descent_rate_meter_per_sec_local * 60;
    outputSummary->ascentRateMeterPerMinute = sim_ascent_rate_meter_per_sec_local * 60;
    outputSummary->timeToBottom = 0;
    outputSummary->timeToFirstStop = 0;
    outputSummary->depthMeterFirstStop = 0;
    outputSummary->timeAtBottom = 0;
    outputSummary->timeToSurface = 0;

    currentConsumGasId = pGasChangeList[0].gasId;

    /* ascent + at depth loop at the moment work gas does not support change depth => no need to check */
    while(currentTime_sec < dive_time_minutes * 60)
    {
    	if(currentDepth_m < depth_meter)
    	{
    		currentDepth_m += sim_descent_rate_meter_per_sec_local;
    		currentGasConsumption = ((float)gasConsumTravelInput) * sGChelper_bar(currentDepth_m ) / 60.0;
    	}
    	else
    	{
    		if(outputSummary->timeToBottom == 0)
    		{
    			currentDepth_m = depth_meter;
    			outputSummary->timeToBottom = currentTime_sec / 60;
    		    outputSummary->ppO2AtBottom = (sGChelper_bar(depth_meter) - WATER_VAPOUR_PRESSURE) * pDiveState->diveSettings.gas[currentConsumGasId].oxygen_percentage / 100.0f;
    		}
    	}
    	currentTime_sec++;
    	outputConsumptionTempFloat[currentConsumGasId] += currentGasConsumption;
    }

    outputSummary->timeAtBottom = (currentTime_sec / 60);		/*  - outputSummary->timeToBottom; */

    /* move forward to deco gas section (behind 255 entry) */
    for(ChangeListIndex = 0; ChangeListIndex < GAS_CHANGE_LIST_ITEMS; ChangeListIndex++)
    {
    	if(pGasChangeList[ChangeListIndex].depth == 255)
    	{
    		ChangeListIndex++;
    		firstDecoGasIndex = ChangeListIndex;
    		nextGasChangeMeter = pGasChangeList[firstDecoGasIndex].depth;
    		nextGasChangeGasId = pGasChangeList[firstDecoGasIndex].gasId;
    	}
    	if((firstDecoGasIndex != 0) && (pGasChangeList[ChangeListIndex].depth > nextGasChangeMeter) /* find deepest gas switch */
    			&& (pGasChangeList[ChangeListIndex].depth < currentDepth_m))
    	{
    		nextGasChangeMeter = pGasChangeList[ChangeListIndex].depth;
    		nextGasChangeGasId = pGasChangeList[ChangeListIndex].gasId;
    	}
    }

    /* do ascent with stops */
    getNextDecoDepthAndTime(&nextDecoDepth, &nextDecoTime, currentDepth_m, decoInfoInput);
    while(currentDepth_m > 0)
    {
    	if(currentDepth_m > nextDecoDepth)
    	{
    		currentDepth_m -= sim_ascent_rate_meter_per_sec_local;
    		currentGasConsumption = ((float)gasConsumDecoInput) * sGChelper_bar(currentDepth_m ) / 60.0;
    	}
    	else
    	{
    		if(outputSummary->timeToFirstStop == 0)
    		{
    			currentDepth_m = nextDecoDepth;
    			outputSummary->timeToFirstStop = currentTime_sec / 60;
    			outputSummary->depthMeterFirstStop = nextDecoDepth;
    		}
    		if(nextDecoTime)
    		{
    			nextDecoTime--;
    		}
    		else
    		{
    			 getNextDecoDepthAndTime(&nextDecoDepth, &nextDecoTime, currentDepth_m, decoInfoInput);
    		}
    	}
    	if(currentDepth_m <= nextGasChangeMeter)	/* switch gas ? */
    	{
    		nextGasChangeMeter = 0;
    		currentConsumGasId = nextGasChangeGasId;
    		for(ChangeListIndex = firstDecoGasIndex; ChangeListIndex < GAS_CHANGE_LIST_ITEMS; ChangeListIndex++)
    		{
    		   	if((pGasChangeList[ChangeListIndex].depth > nextGasChangeMeter) 			/* find deepest gas switch */
    		    	&& (pGasChangeList[ChangeListIndex].depth < currentDepth_m))
    		    {
    		    		nextGasChangeMeter = pGasChangeList[ChangeListIndex].depth;
    		    		nextGasChangeGasId = pGasChangeList[ChangeListIndex].gasId;
    		    }
    		}
    	}
    	currentTime_sec++;
    	outputConsumptionTempFloat[currentConsumGasId] += currentGasConsumption;
    }

    if(decoInfoInput->output_time_to_surface_seconds)
    {
    	outputSummary->timeToSurface = outputSummary->timeAtBottom + (decoInfoInput->output_time_to_surface_seconds / 60);
    }
    else
    {
    	outputSummary->timeToSurface = currentTime_sec / 60;
    }

    for(ChangeListIndex = 0; ChangeListIndex < 6; ChangeListIndex++)
    {
    	outputConsumptionList[ChangeListIndex] = (uint16_t)outputConsumptionTempFloat[ChangeListIndex];
    }
}


/**
  ******************************************************************************
  * @brief  Simulator control during simulated dive
  ******************************************************************************
  * @note called by user via tHomeDiveMenuControl()
  * @param  void
  * @return void
  */


void Sim_Descend (void)
{
    stateSimGetPointerWrite()->lifeData.counterSecondsShallowDepth = 0;
    if(simulation_get_aim_depth() < 200)
        simulation_set_aim_depth(simulation_get_aim_depth() + 1);
}


void Sim_Ascend (void)
{
    if(simulation_get_aim_depth() > 0)
        simulation_set_aim_depth(simulation_get_aim_depth() - 1);
}


void Sim_Divetime (void)
{
    simulation_add_time(5);
}


void Sim_Quit (void)
{
    if(stateSimGetPointer()->lifeData.counterSecondsShallowDepth)
    {
        simulation_exit();
        return;
    }

    if(simulation_get_aim_depth() > 0)
    {
        simulation_set_aim_depth(0);
    }
    else
    {
        stateSimGetPointerWrite()->lifeData.depth_meter = 0;
        if(stateSimGetPointer()->diveSettings.diveMode == DIVEMODE_Apnea)
        {
            stateSimGetPointerWrite()->lifeData.counterSecondsShallowDepth = 1;
        }
        else
        {
            stateSimGetPointerWrite()->lifeData.counterSecondsShallowDepth = settingsGetPointer()->timeoutDiveReachedZeroDepth - 15;
        }
    }
}

void Sim_IncreasePPO(uint8_t sensorIdx)
{
	if((sensorIdx < NUM_OF_SENSORS) && (simSensmVOffset[sensorIdx] + SIM_PPO2_STEP < 100.0) && ((stateUsed->diveSettings.ppo2sensors_deactivated & (1 << sensorIdx)) == 0))
	{
		simSensmVOffset[sensorIdx] += SIM_PPO2_STEP;
	}
}
void Sim_DecreasePPO(uint8_t sensorIdx)
{
	if((sensorIdx < NUM_OF_SENSORS) && (simSensmVOffset[sensorIdx] - SIM_PPO2_STEP >= -100.0))
	{
		simSensmVOffset[sensorIdx] -= SIM_PPO2_STEP;
	}
}
