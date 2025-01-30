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

    memcpy(stateSim.scrubberDataDive, settingsGetPointer()->scrubberData, sizeof(stateSim.scrubberDataDive));
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

    if((pSettings->scrubTimerMode != SCRUB_TIMER_OFF) && (isLoopMode(pSettings->dive_mode)) && (pDiveState->mode == MODE_DIVE) && isLoopMode(pDiveState->diveSettings.diveMode))
    {
    	simScrubberTimeoutCount++;
    	if(simScrubberTimeoutCount >= 60)		/* resolution is minutes */
    	{
    		simScrubberTimeoutCount = 0;
    		if(pDiveState->scrubberDataDive[pSettings->scubberActiveId].TimerCur > MIN_SCRUBBER_TIME)
    		{
    			pDiveState->scrubberDataDive[pSettings->scubberActiveId].TimerCur--;
    		}
    		translateDate(stateUsed->lifeData.dateBinaryFormat, &stateUsedWrite->scrubberDataDive[pSettings->scubberActiveId].lastDive);
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
                pDiveState->lifeData.counterSecondsShallowDepth = settingsGetPointer()->timeoutDiveReachedZeroDepth - 15;
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
                simulation_exit();
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

SDecoinfo* simulation_decoplaner(uint16_t depth_meter, uint16_t intervall_time_minutes, uint16_t dive_time_minutes, uint8_t *gasChangeListDepthGas20x2)
{
    uint8_t ptrGasChangeList = 0; // new hw 160704
#ifdef ENABLE_DECOCALC_OPTION
    uint8_t index = 0;
#endif
    for (int i = 0; i < 40; i++)
    	gasChangeListDepthGas20x2[i] = 0;

    SDiveState * pDiveState = &stateSim;
    copyDiveSettingsToSim();

#ifdef ENABLE_DECOCALC_OPTION
    /* activate deco calculation for all deco gases */
    for(index = 0; index < 1 + (2*NUM_GASES); index++)
    {
    	if(pDiveState->diveSettings.gas[index].note.ub.deco)
    	{
    		pDiveState->diveSettings.gas[index].note.ub.decocalc = 1;
    	}
    }
#endif

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

    // new hw 160704
    if(gasChangeListDepthGas20x2)
    {
        gasChangeListDepthGas20x2[ptrGasChangeList++] = 0;
        gasChangeListDepthGas20x2[ptrGasChangeList++] = pDiveState->lifeData.actualGas.GasIdInSettings;
        gasChangeListDepthGas20x2[0] =0; // depth zero
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
            if(gasChangeListDepthGas20x2 && (pDiveState->diveSettings.diveMode == DIVEMODE_OC))
            {
                gasChangeListDepthGas20x2[ptrGasChangeList++] = pDiveState->lifeData.depth_meter;
                gasChangeListDepthGas20x2[ptrGasChangeList++] = actualBetterGasId();
            }
        }
    }

    decom_CreateGasChangeList(&pDiveState->diveSettings, &pDiveState->lifeData); // was there before and needed for buehlmann_calc_deco and vpm_calc

    // new hw 160704
    if(gasChangeListDepthGas20x2 && (pDiveState->diveSettings.diveMode == DIVEMODE_OC))
    {
        // change direction from better gas to deco gas
        gasChangeListDepthGas20x2[ptrGasChangeList++] = 255;
        gasChangeListDepthGas20x2[ptrGasChangeList++] = 255;

        // ascend (deco) gases
        for(int i=1; i<=5;i++)
        {
            if((pDiveState->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero == 0)
#ifdef ENABLE_DECOCALC_OPTION
            		|| (pDiveState->diveSettings.gas[pDiveState->diveSettings.decogaslist[i].GasIdInSettings].note.ub.decocalc == 0)
#endif
					)
                break;
            gasChangeListDepthGas20x2[ptrGasChangeList++] = pDiveState->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero;
            gasChangeListDepthGas20x2[ptrGasChangeList++] = pDiveState->diveSettings.decogaslist[i].GasIdInSettings;
        }
        gasChangeListDepthGas20x2[0] = 0;
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


/**
  ******************************************************************************
  * @brief  simulation_helper_change_points
  ******************************************************************************
    * @param
  * @return void
  */
void simulation_helper_change_points(SSimDataSummary *outputSummary, uint16_t depth_meter, uint16_t dive_time_minutes, SDecoinfo *decoInfoInput, const uint8_t *gasChangeListDepthGas20x2)
{
    uint8_t ptrDecoInfo = 0;
    uint16_t actualDepthPoint = 0;
    uint16_t nextDepthPoint = 0;
    uint8_t actualConsumGasId = 0;
    uint8_t nextGasChangeMeter = 0;
    uint8_t ptrChangeList = 0;

    float timeThis = 0;
    float timeSummary = 0;
    float	sim_descent_rate_meter_per_min_local = 10;
    float	sim_ascent_rate_meter_per_min_local = 10;

    SDiveState * pDiveState = &stateSim;

    uint8_t depthDecoNext, depthLast, depthSecond, depthInc;

    if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
    {
        sim_descent_rate_meter_per_min_local = sim_descent_rate_meter_per_min; // const float
        sim_ascent_rate_meter_per_min_local = pDiveState->diveSettings.ascentRate_meterperminute;
    }
    else
    {
        sim_descent_rate_meter_per_min_local = sim_descent_rate_meter_per_min; // const float
        sim_ascent_rate_meter_per_min_local = 10;// fix in vpm_calc_deco();
    }

    outputSummary->descentRateMeterPerMinute = sim_descent_rate_meter_per_min_local;
    outputSummary->ascentRateMeterPerMinute = sim_ascent_rate_meter_per_min_local;

    // bottom gas ppO2
    if(gasChangeListDepthGas20x2)
    {
        nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];
        actualConsumGasId = gasChangeListDepthGas20x2[ptrChangeList++];
        nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];

        while(actualDepthPoint < depth_meter)
        {
            if(nextGasChangeMeter && (nextGasChangeMeter < depth_meter) && (gasChangeListDepthGas20x2[ptrChangeList] != 255))  // list has 255,255 for turn from travel to deco
            {
                nextDepthPoint = nextGasChangeMeter;
            }
            else
            {
                nextDepthPoint = depth_meter;
            }

            if(actualConsumGasId > 5) // safety first
                actualConsumGasId = 0;

            actualDepthPoint = nextDepthPoint;

            if(actualDepthPoint != depth_meter)
            {
                actualConsumGasId = gasChangeListDepthGas20x2[ptrChangeList++];
                nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];
            }
        }
    }
    else
    {
        actualConsumGasId = pDiveState->lifeData.actualGas.GasIdInSettings;
        nextGasChangeMeter = 0;
    }
    outputSummary->ppO2AtBottom = (sGChelper_bar(depth_meter) - WATER_VAPOUR_PRESSURE) * pDiveState->diveSettings.gas[actualConsumGasId].oxygen_percentage / 100.0f;


    // going down
    actualDepthPoint = 0;
    nextDepthPoint = depth_meter;

    timeThis = ((float)(nextDepthPoint - actualDepthPoint)) / sim_descent_rate_meter_per_min_local;
    timeSummary += timeThis;
    outputSummary->timeToBottom = (uint16_t)timeThis;

    // bottom time
    timeThis = ((float)dive_time_minutes) - timeSummary;
    timeSummary += timeThis;
    outputSummary->timeAtBottom = (uint16_t)timeSummary;


    // ascend to first deco stop
    actualDepthPoint = depth_meter; // that is where we are
    timeThis = 0;

    if(!decoInfoInput->output_stop_length_seconds[0]) // NDL dive
    {
        depthLast = 0;
        ptrDecoInfo = 0;
        depthDecoNext = 0;
    }
    else
    {
        // prepare deco stop list
        depthLast 		= (uint8_t)(stateUsed->diveSettings.last_stop_depth_bar * 10);
        depthSecond 	= (uint8_t)(stateUsed->diveSettings.input_second_to_last_stop_depth_bar * 10);
        depthInc 			= (uint8_t)(stateUsed->diveSettings.input_next_stop_increment_depth_bar * 10);

        for(ptrDecoInfo=DECOINFO_STRUCT_MAX_STOPS-1; ptrDecoInfo>0; ptrDecoInfo--)
            if(decoInfoInput->output_stop_length_seconds[ptrDecoInfo]) break;

        if(ptrDecoInfo == 0)
        {
            depthDecoNext = depthLast;
        }
        else
            depthDecoNext = depthSecond + (( ptrDecoInfo - 1 )* depthInc);
    }

    nextDepthPoint = depthDecoNext;
    if(actualDepthPoint > nextDepthPoint)
    {
        // flip signs! It's going up
        timeThis = ((float)(actualDepthPoint - nextDepthPoint)) / sim_ascent_rate_meter_per_min_local;
        actualDepthPoint = nextDepthPoint; // that is where we are
    }
    timeSummary += timeThis;
    outputSummary->timeToFirstStop = (uint16_t)timeSummary;
    outputSummary->depthMeterFirstStop = actualDepthPoint;

    if(decoInfoInput->output_time_to_surface_seconds)
    {
    	outputSummary->timeToSurface = outputSummary->timeAtBottom + (decoInfoInput->output_time_to_surface_seconds / 60);
    }
    else
    {
    	outputSummary->timeToSurface = outputSummary->timeToFirstStop;
    }
}


/**
  ******************************************************************************
  * @brief  simulation_gas_consumption
  ******************************************************************************
  * @note called by openEdit_PlanResult() in tMenuEditPlanner.c
  * @note the ascend and descend time is taken from pDiveState->lifeData.ascent_rate_meter_per_min and const float sim_descent_rate_meter_per_min
  * @param  outputConsumptionList list from 1 to 5 for gas 1 to 5
  * @param  depth_meter for descend
  * @param  dive_time_minutes for descend and bottom time
  * @param  the calculated deco list
    * @param  gasConsumTravelInput: how many l/min for all but deco stops
    * @param  gasConsumDecoInput: how many l/min for deco stops only
  * @return void
  */

void simulation_gas_consumption(uint16_t *outputConsumptionList, uint16_t depth_meter, uint16_t dive_time_minutes, SDecoinfo *decoInfoInput, uint8_t gasConsumTravelInput, uint8_t gasConsumDecoInput, const uint8_t *gasChangeListDepthGas20x2)
{
    uint8_t ptrDecoInfo = 0;
    uint8_t ptrChangeList = 0;
    uint8_t actualConsumGasId = 0;
    uint8_t nextGasChangeMeter = 0;
    uint16_t actualDepthPoint = 0;
    uint16_t nextDepthPoint = 0;
    uint16_t inBetweenDepthPoint = 0;
    float timeThis = 0;
    float consumThis = 0;
    float timeSummary = 0;
    float outputConsumptionTempFloat[6];
    float	sim_descent_rate_meter_per_min_local = 10;
    float	sim_ascent_rate_meter_per_min_local = 10;

    SDiveState * pDiveState = &stateSim;

    uint8_t depthDecoNext = 0;
    uint8_t depthLast = 0;
    uint8_t depthSecond = 0;
	uint8_t depthInc = 0;

    for(int i = 1; i < 6; i++)
        outputConsumptionTempFloat[i] = 0;

    if(gasChangeListDepthGas20x2)
    {
        nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];
        actualConsumGasId = gasChangeListDepthGas20x2[ptrChangeList++];
        nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];
    }
    else
    {
        actualConsumGasId = pDiveState->lifeData.actualGas.GasIdInSettings;
        nextGasChangeMeter = 0;
    }

    if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
    {
        sim_descent_rate_meter_per_min_local = sim_descent_rate_meter_per_min; // const float
        sim_ascent_rate_meter_per_min_local = pDiveState->diveSettings.ascentRate_meterperminute;
    }
    else
    {
        sim_descent_rate_meter_per_min_local = sim_descent_rate_meter_per_min; // const float
        sim_ascent_rate_meter_per_min_local = 10;// fix in vpm_calc_deco();
    }

//	while((nextGasChangeMeter < depth_meter) && (actualDepthPoint < depth_meter))
    while(actualDepthPoint < depth_meter)
    {
        if(nextGasChangeMeter && (nextGasChangeMeter < depth_meter) && (gasChangeListDepthGas20x2[ptrChangeList] != 255))  // list has 255,255 for turn from travel to deco
        {
            nextDepthPoint = nextGasChangeMeter;
        }
        else
        {
            nextDepthPoint = depth_meter;
        }

        if(actualConsumGasId > 5) // safety first
            actualConsumGasId = 0;

        timeThis = ((float)(nextDepthPoint - actualDepthPoint)) / sim_descent_rate_meter_per_min_local;
        if(actualDepthPoint) // not if on surface
        {
            consumThis = ((float)gasConsumTravelInput) * sGChelper_bar(actualDepthPoint) * timeThis;
        }
        consumThis += ((float)gasConsumTravelInput) * sGChelper_bar(nextDepthPoint -actualDepthPoint) * timeThis / 2;
        outputConsumptionTempFloat[actualConsumGasId] += consumThis;
        timeSummary += timeThis;

        actualDepthPoint = nextDepthPoint;

        if(actualDepthPoint != depth_meter)
        {
            actualConsumGasId = gasChangeListDepthGas20x2[ptrChangeList++];
            nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];
        }
    }

    // bottom Time
    timeThis = ((float)dive_time_minutes) - timeSummary;

    if(timeThis > 0)
    {
        consumThis = ((float)gasConsumTravelInput) * sGChelper_bar(depth_meter) * timeThis;
        outputConsumptionTempFloat[actualConsumGasId] += consumThis;
    }

    // ascend with deco stops prepare
    if(gasChangeListDepthGas20x2)
    {
        ptrChangeList++;// gasChangeListDepthGas20x2[ptrChangeList++]; // should be the 255
        nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];
    }
    else
    {
        nextGasChangeMeter = 0;
    }


    if(!decoInfoInput->output_stop_length_seconds[0]) // NDL dive
    {
        depthLast = 0;
        ptrDecoInfo = 0;
    }
    else
    {
        // prepare deco stop list
        depthLast 		= (uint8_t)(stateUsed->diveSettings.last_stop_depth_bar * 10);
        depthSecond 	= (uint8_t)(stateUsed->diveSettings.input_second_to_last_stop_depth_bar * 10);
        depthInc 			= (uint8_t)(stateUsed->diveSettings.input_next_stop_increment_depth_bar * 10);

        for(ptrDecoInfo=DECOINFO_STRUCT_MAX_STOPS-1; ptrDecoInfo>0; ptrDecoInfo--)
            if(decoInfoInput->output_stop_length_seconds[ptrDecoInfo]) break;
    }

    actualDepthPoint = depth_meter; // that is where we are

    // ascend with deco stops
    while(actualDepthPoint)
    {
        if(ptrDecoInfo == 0)
        {
            depthDecoNext = depthLast;
        }
        else
            depthDecoNext = depthSecond + (( ptrDecoInfo - 1 )* depthInc);

        if(nextGasChangeMeter && (nextGasChangeMeter > depthDecoNext))
        {
            nextDepthPoint = nextGasChangeMeter;
        }
        else
        {
            nextDepthPoint = depthDecoNext;
        }

        if(actualConsumGasId > 5) // safety first
            actualConsumGasId = 0;

        if(actualDepthPoint > nextDepthPoint)
        {
            // flip signs! It's going up
            timeThis = ((float)(actualDepthPoint - nextDepthPoint)) / sim_ascent_rate_meter_per_min_local;
            inBetweenDepthPoint = nextDepthPoint + ((actualDepthPoint - nextDepthPoint)/2);
            consumThis = ((float)gasConsumDecoInput) * sGChelper_bar(inBetweenDepthPoint) * timeThis;
/*
            if(nextDepthPoint)
            {
                consumThis = ((float)gasConsumDecoInput) * sGChelper_bar(nextDepthPoint) * timeThis;
            }
            else
            {
                consumThis = 0;
            }
            consumThis += ((float)gasConsumDecoInput) * sGChelper_bar(actualDepthPoint - nextDepthPoint) * timeThis / 2;
*/
            outputConsumptionTempFloat[actualConsumGasId] += consumThis;
        }

        if(nextGasChangeMeter && (nextDepthPoint == nextGasChangeMeter))
        {
            actualConsumGasId = gasChangeListDepthGas20x2[ptrChangeList++];
            nextGasChangeMeter = gasChangeListDepthGas20x2[ptrChangeList++];
        }

        if(actualConsumGasId > 5) // safety first
            actualConsumGasId = 0;

        if(nextDepthPoint && (nextDepthPoint == depthDecoNext))
        {
            if(decoInfoInput->output_stop_length_seconds[ptrDecoInfo])
            {
                timeThis = ((float)(decoInfoInput->output_stop_length_seconds[ptrDecoInfo])) / 60.0f;
                consumThis = ((float)gasConsumDecoInput) * sGChelper_bar(nextDepthPoint) * timeThis;
                outputConsumptionTempFloat[actualConsumGasId] += consumThis;
            }
            if(ptrDecoInfo != 0)
            {
                ptrDecoInfo--;
            }
            else
            {
                depthLast = 0;
            }
        }
        actualDepthPoint = nextDepthPoint;
    }

    // copy and return
    for(int i = 1; i < 6; i++)
        outputConsumptionList[i] = (uint16_t)(outputConsumptionTempFloat[i]);
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
