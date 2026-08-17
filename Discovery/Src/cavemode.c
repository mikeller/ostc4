/**
  ******************************************************************************
  * @file    hud.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    09-May-2026
  * @brief   Implementation for cave mode calculations
  *
  @verbatim
  ==============================================================================
                        ##### How to use #####
  ==============================================================================
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT(c) 2026 heinrichs weikamp
  *
  ******************************************************************************
  */

#include "data_central.h"
#include "data_exchange.h"
#include "cavemode.h"
#include "settings.h"
#include "logbook_miniLive.h"
#include "math.h"
#include "decom.h"
//#include "buehlmann.h"
#include "check_warning.h"
#include "simulation.h"
#include "t7.h"
#include "t3.h"
#include "tHome.h"

typedef struct stopEntry
{
	float depth;
	uint16_t length;
} s_stopEntry;

#ifdef ENABLE_CAVEMODE

s_stopEntry stopList[10];

static cavemodeState_t caveModeState = CAVEMODE_OFF;
static SLifeData caveData;

static uint16_t caveDataIndex = 0;
static uint16_t returnStartIndex = 0;
static SGas startGas;
static uint8_t liveDive = 1;	/* the recording of the dive data was not interrupted */
static uint32_t returnTime_seconds = 0;
static uint32_t returnTime_last = 0;
static uint16_t replayDataLength = 0;

static uint32_t gasUsedUpdate_Tick = 0;

static float maxCeiling = 0.0;

#define CAVE_CALC_INTERVALL			20		/* in seconds. Because cave calculation is just an estimation it is not done on seconds base. */
#define CAVE_DECO_STOP_RESOLUTION	20		/* in seconds. Granularity of time spend at deco stops */
#define PRESSURE_150_CM 0.15f
#define PRESSURE_THREE_METER 0.333334f
#define PRESSURE_HALF_METER 0.05f

static uint32_t tts_Cave_Sec = 0;
static float tissue_nitrogen_return_bar[16];
static float tissue_helium_return_bar[16];
static float endPressure = 0.0;


/* Precondition: diver is in deco zone => GF_low_depth_bar is known */
static float getGfAtPressure(SDiveSettings *pDiveSettings, float pressure, float surface_Bar, float GF_low_depth_bar)
{
	float gfSteigung = 0.0f;
	float retGF = 0;

	float gf_Low = pDiveSettings->gf_low / 100.0;
	float gf_High = pDiveSettings->gf_high / 100.0;

	if((pressure - surface_Bar) <= PRESSURE_HALF_METER)	/* close to surface */
	{
		retGF = ((float)pDiveSettings->gf_high) / 100.0f;
	}
	else if((pressure - surface_Bar) >= GF_low_depth_bar)	/* deeper than max ceiling */
	{
		retGF = ((float)pDiveSettings->gf_low) / 100.0f;
	}
	else
	{
		if(GF_low_depth_bar < 0)
		{
			GF_low_depth_bar = PRESSURE_THREE_METER; // just to prevent erratic behaviour if variable is not set
		}
		gfSteigung = ((float)(gf_High - gf_Low))/ GF_low_depth_bar;

		retGF = (gf_High - gfSteigung * (pressure - surface_Bar));
	}

	return retGF;
}

static float calcDeepestDecoPressure(SDiveSettings *pDiveSettings, float surface_Bar, float ceiling)
{
	float decoPressure_bar = 0.0;

	ceiling -= surface_Bar;

	if(ceiling > 0.05 ) /* ignore small deviations cause by 20 second calculation cycle */
	{
		if((ceiling - pDiveSettings->last_stop_depth_bar) <= 0.0)
		{
			decoPressure_bar =  pDiveSettings->last_stop_depth_bar;
		}
		else
		{
			decoPressure_bar = pDiveSettings->input_second_to_last_stop_depth_bar;
			ceiling -= pDiveSettings->input_second_to_last_stop_depth_bar;
			while(ceiling > 0.0)
			{
				ceiling -= pDiveSettings->input_next_stop_increment_depth_bar;
				decoPressure_bar += pDiveSettings->input_next_stop_increment_depth_bar;
			}
		}
	}
	return decoPressure_bar;
}

/* optimized for less accuracy but speed */
static float depthMeterToBar(float depth_meter, float surface_Bar)
{
	float pressure_per_meter = 0.0981f; /* sweet water */
    float retBar = surface_Bar;

    if(depth_meter >= 0)
    {
    	if(settingsGetPointer()->salinity)
    	{
    		pressure_per_meter = 0.1f;	/* salt water */
    	}

        retBar = depth_meter * pressure_per_meter + surface_Bar;
    }
    return retBar;
}


static float barToDepthMeter(float pressure_bar, float surface_Bar)
{
    float pressure_per_meter = 0.0981f; /* sweet water */
    float depth_meter = 0.0f;

    if(settingsGetPointer()->salinity)
    {
        pressure_per_meter = 0.1f; /* salt water */
    }

    if(pressure_bar > surface_Bar)
    {
        depth_meter = (pressure_bar - surface_Bar) / pressure_per_meter;
    }
    return depth_meter;
}



static float calcCeiling(SDiveSettings *pDiveSettings, SLifeData *pLifeData, float pressure)
{
	float tissue_inertgas_saturation;
	float inertgas_a;
	float inertgas_b;
	float ceiling;
	float global_ceiling;
	uint8_t ci;
	float ceiling_maxGF;
	float GF_value = (float)pDiveSettings->gf_low / 100.0;;

	global_ceiling = -1;

	for (ci = 0; ci < 16; ci++)
	{
		if(pLifeData->tissue_helium_bar[ci] == 0)
		{
			tissue_inertgas_saturation = pLifeData->tissue_nitrogen_bar[ci];
			inertgas_a = buehlmann_N2_a[ci];
			inertgas_b = buehlmann_N2_b[ci];
		}
		else
		{
			tissue_inertgas_saturation =  pLifeData->tissue_nitrogen_bar[ci] + pLifeData->tissue_helium_bar[ci];
			inertgas_a = ( ( buehlmann_N2_a[ci] *  pLifeData->tissue_nitrogen_bar[ci]) + ( buehlmann_He_a[ci] * pLifeData->tissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
			inertgas_b = ( ( buehlmann_N2_b[ci] *  pLifeData->tissue_nitrogen_bar[ci]) + ( buehlmann_He_b[ci] * pLifeData->tissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
		}

		/* first calc deepest possible ceiling using GF_Low */
		ceiling_maxGF = (inertgas_b * ( tissue_inertgas_saturation - GF_value * inertgas_a ) ) / (GF_value - (inertgas_b * GF_value) + inertgas_b);
		if(ceiling_maxGF > maxCeiling)	/* maxCeiling is "internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero" */
		{
			maxCeiling = ceiling_maxGF;
		}

		if(maxCeiling > (pressure - PRESSURE_150_CM))		/* in deco zone => apply GF */
		{
			GF_value = getGfAtPressure(pDiveSettings, pressure, pLifeData->pressure_surface_bar, maxCeiling);
			ceiling = (inertgas_b * ( tissue_inertgas_saturation - GF_value * inertgas_a ) ) / (GF_value - (inertgas_b * GF_value) + inertgas_b);
		}
		else
		{
			ceiling = inertgas_b *  ( tissue_inertgas_saturation - inertgas_a );
		}

		if(ceiling > global_ceiling) global_ceiling = ceiling;
	}
	return global_ceiling;
}

void caveMode_Init()
{
	uint8_t index = 0;
	SSettings *pSettings = settingsGetPointer();
    const SGasLine * pGasLine = stateUsed->diveSettings.gas;

	pGasLine = stateUsed->diveSettings.gas;

   for(index=1; index <= NUM_GASES; index++)
   {
	   if(((pGasLine[index].note.ub.first) || (pGasLine[index].note.ub.deco))
			   && (pGasLine[index].bottle_id_bar != 0) && (pGasLine[index].bottle_size_liter != 0))
	   {
		   stateUsedWrite->lifeData.gasDemand_Ltr[index] = 0;
		   caveData.gasDemand_Ltr[index] = 0;
		   stateUsedWrite->lifeData.gasUsed_Ltr[index] = 0;
		   caveData.gasUsed_Ltr[index] = 0;
	   }
   }
   startGas.GasIdInSettings = 0xFF;
   maxCeiling = 0.0;
   liveDive = 1;
   returnTime_seconds = 0;
   returnTime_last = 0;
   if((pSettings->caveModeAutoStart) && ((!t3_customview_disabled(CVIEW_T3_Cavemode)) || (!t7_customview_disabled(CVIEW_Cave))))
   {
	   caveModeState = CAVEMODE_RECORDING;
   }
   else
   {
	   caveModeState = CAVEMODE_OFF;
   }
}

uint8_t caveMode_isActive(void)
{
	uint8_t retActive = 0;

	if((caveModeState == CAVEMODE_RECORDING) || (caveModeState == CAVEMODE_RETURNING))
	{
		retActive = 1;
	}
	return retActive;

}
void caveMode_Update(SDiveState *pDiveState)
{
	int8_t caveDataStep = 0;
	uint8_t betterGasId;
	uint8_t betterDecoGasId;

	static uint32_t ttsWork = 0;

	uint16_t* pDepthDataReplay;
	uint16_t* pDepthCalcSource = NULL;
	uint16_t dataResolutionSec = getReplayDataResolution();
	SSettings *pSettings = settingsGetPointer();
	float startPressure = 0.0;
	static float targetPressure = 0.0;
	float gasChangePressure = 0.0;
	float ceiling = 0.0;
	static float nextStop_Bar = 0.0;
	float currentStop_Bar = 0.0;
	uint8_t doStop = 0;
	static uint8_t stopId = 0;

	static uint16_t caveStartIndex = 0;
	static uint8_t compuCycleComplete = 0;	/* indicator if the last profile computation was completed or not */
	float currentDepthMeter = 0.0;
	uint8_t gasIndex = 0;
    uint8_t iterationCnt = 10;		/* do 10 calculations a 20 seconds => 3.x minutes */

    uint8_t calcNormalDecoDemand = 0;
    float maxDeltaPressure = 0.0;
//    uint8_t timeForAscent_sec = 0;
    float deltaPressure = 0.0;

    if((!t7_customview_disabled(CVIEW_GasDemand)) && (caveModeState == CAVEMODE_OFF))
    {
    	calcNormalDecoDemand = 1;
    }

	if((pDiveState->mode == MODE_DIVE) && (getMiniLiveReplayLength() > 30) && ((caveModeState != CAVEMODE_OFF) || (calcNormalDecoDemand)))	/* start calculation one minute after begin of dive */
	{
		if(!calcNormalDecoDemand)
		{
			getReplayInfo(&pDepthDataReplay, NULL, &replayDataLength, NULL, NULL);
			if(caveModeState == CAVEMODE_RECORDING)		/* not yet returning => use live data */
			{
				pDepthCalcSource = getMiniLiveReplayPointerToData(1);
				replayDataLength = getMiniLiveReplayLength();
			}
			else
			{
				pDepthCalcSource = pDepthDataReplay;
			}

			if(returnTime_last != pDiveState->lifeData.dive_time_seconds)
			{
				if(caveModeState == CAVEMODE_RETURNING)
				{
					returnTime_seconds += (pDiveState->lifeData.dive_time_seconds - returnTime_last);	/* In Sim mode more than one second may have been past */
				}
				returnTime_last = pDiveState->lifeData.dive_time_seconds;
			}
		}
		if(endPressure <= caveData.pressure_surface_bar + 0.1)			/* start next iteration */
		{
			caveData.pressure_surface_bar = pDiveState->lifeData.pressure_surface_bar;
			if(!calcNormalDecoDemand)
			{
				if((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
				{
					caveDataIndex = returnStartIndex / dataResolutionSec;
					caveStartIndex = caveDataIndex;
					memcpy(&caveData.actualGas, &startGas, sizeof(caveData.actualGas));
					memcpy (caveData.tissue_nitrogen_bar, tissue_nitrogen_return_bar, sizeof(caveData.tissue_nitrogen_bar));
					memcpy (caveData.tissue_helium_bar, tissue_helium_return_bar, sizeof(caveData.tissue_helium_bar));
					if(compuCycleComplete)
					{
						MiniLiveLogbook_releaseModData();
					}
					MiniLiveLogbook_resetModData();
				}
				else
				{
					caveDataIndex = getMiniLiveReplayLength() -1;
					memcpy(&caveData.actualGas, &pDiveState->lifeData.actualGas, sizeof(caveData.actualGas));
					memcpy (caveData.tissue_nitrogen_bar, pDiveState->lifeData.tissue_nitrogen_bar, sizeof(caveData.tissue_nitrogen_bar));
					memcpy (caveData.tissue_helium_bar, pDiveState->lifeData.tissue_helium_bar, sizeof(caveData.tissue_helium_bar));
				}
				currentDepthMeter = pDepthCalcSource[caveDataIndex - 1] / 100.0;
				endPressure = depthMeterToBar(currentDepthMeter, caveData.pressure_surface_bar);
				targetPressure = endPressure;
			}
			else
			{
				memcpy(&caveData.actualGas, &pDiveState->lifeData.actualGas, sizeof(caveData.actualGas));
				memcpy (caveData.tissue_nitrogen_bar, pDiveState->lifeData.tissue_nitrogen_bar, sizeof(caveData.tissue_nitrogen_bar));
				memcpy (caveData.tissue_helium_bar, pDiveState->lifeData.tissue_helium_bar, sizeof(caveData.tissue_helium_bar));
				endPressure = pDiveState->lifeData.pressure_ambient_bar;
				targetPressure = caveData.pressure_surface_bar;
				currentDepthMeter =  pDiveState->lifeData.depth_meter;
			}
			memcpy (pDiveState->lifeData.gasDemand_Ltr, caveData.gasDemand_Ltr, sizeof(caveData.gasDemand_Ltr));	/* publish results of last iteration */
			memset (caveData.gasDemand_Ltr, 0, sizeof(caveData.gasDemand_Ltr));
			if(caveData.actualGas.GasIdInSettings == pDiveState->lifeData.actualGas.GasIdInSettings)
			{
				caveData.gasDemand_Ltr[caveData.actualGas.GasIdInSettings] = 5;										/* make sure actual gas is != 0 => displayed */
			}
			tts_Cave_Sec = ttsWork;		/* copy result of last calculation cycle */
			ttsWork = 0;
			memset (stopList, 0, sizeof (stopList));
			stopId = 0;
			ceiling = calcCeiling(&pDiveState->diveSettings, &caveData, endPressure);
			if(ceiling > endPressure)
			{
				nextStop_Bar = calcDeepestDecoPressure(&pDiveState->diveSettings, caveData.pressure_surface_bar, ceiling);
			}
			else
			{
				nextStop_Bar = 0;
			}
			caveData.depth_meter = pDiveState->lifeData.max_depth_meter;			/* use max depth to create a list of all possible gases */
			decom_CreateGasChangeList(&stateUsedWrite->diveSettings, &caveData);
			compuCycleComplete = 0;
		}

		while((endPressure - 0.1 > caveData.pressure_surface_bar) && (iterationCnt > 0))
		{
			iterationCnt--;
			startPressure = endPressure;
			if(targetPressure == endPressure)			/* get next depth */
			{
				if(!calcNormalDecoDemand)
				{
					caveDataStep = CAVE_CALC_INTERVALL / dataResolutionSec;
					if((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
					{
						if(caveDataIndex == replayDataLength - 1)		/* last data entry already processed => force set to depth zero below */
						{
							caveDataIndex++;
						}
						else if((caveDataIndex + caveDataStep) >= replayDataLength )
						{
							caveDataIndex = replayDataLength - 1;
						}
						else
						{
							caveDataIndex += caveDataStep;					/* use record */
							pDepthCalcSource = pDepthDataReplay;
						}
					}
					else
					{
						if((caveDataIndex - caveDataStep) < 0)
						{
							caveDataIndex = 0;
						}
						else
						{
							caveDataIndex -= caveDataStep;
						}
					}

					if((caveDataIndex == 0) || (caveDataIndex >= replayDataLength)) /* last iteration => make sure we do not skip last stop because of compressed profile data */
					{
						currentDepthMeter = 0;
						compuCycleComplete = 1;
					}
					else
					{
						currentDepthMeter = pDepthCalcSource[caveDataIndex] / 100.0;
					}
					endPressure = depthMeterToBar(currentDepthMeter, pDiveState->lifeData.pressure_surface_bar);
				}
				else
				{
					if(endPressure == targetPressure)
					{
						endPressure = caveData.pressure_surface_bar;
					}
				}
				targetPressure = endPressure;
				if(((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE) || (calcNormalDecoDemand)) && (endPressure - 0.1 > caveData.pressure_surface_bar)) /* close to surface => mark computation as valid */
				{
					compuCycleComplete = 1;
				}
			}
			else										/* continue ascending to target depth */
			{
				endPressure = targetPressure;
			}

			if((nextStop_Bar != 0) && (endPressure < (nextStop_Bar + pDiveState->lifeData.pressure_surface_bar)))		/* we reached a deco stop => wait till ceiling is safe again */
			{
				endPressure = nextStop_Bar + pDiveState->lifeData.pressure_surface_bar;
			}

		/* limit ascent to delta which may be passed in 20 seconds */
			if(startPressure > endPressure)
			{
				deltaPressure = startPressure - endPressure;
				maxDeltaPressure = pDiveState->diveSettings.ascentRate_meterperminute * CAVE_CALC_INTERVALL / 600.0f;
				if(deltaPressure > maxDeltaPressure)
				{
					endPressure = startPressure - maxDeltaPressure;
					deltaPressure = startPressure - endPressure;
				}
			}
			else
			{
				deltaPressure = endPressure - startPressure;
			}
		/* Change depth */
			decom_tissues_exposure_stage_schreiner(CAVE_CALC_INTERVALL, &caveData.actualGas, startPressure, endPressure,
																		caveData.tissue_nitrogen_bar,  caveData.tissue_helium_bar);
			/* switch gas */
				betterGasId = caveData.actualGas.GasIdInSettings;
				betterDecoGasId = 0;	/* current gas should be at index 0 */
				for(gasIndex = 1; gasIndex < 6; gasIndex++)
			    {
			        if(pDiveState->diveSettings.decogaslist[gasIndex].change_during_ascent_depth_meter_otherwise_zero != 0)
			        {
			          	gasChangePressure = pDiveState->lifeData.pressure_surface_bar + ((float)pDiveState->diveSettings.decogaslist[gasIndex].change_during_ascent_depth_meter_otherwise_zero  / 10);
						if(endPressure <= gasChangePressure)
						{
							if(gasChangePressure > pDiveState->lifeData.pressure_surface_bar + ((float)pDiveState->diveSettings.decogaslist[betterDecoGasId].change_during_ascent_depth_meter_otherwise_zero  / 10))
							{
								betterGasId = pDiveState->diveSettings.decogaslist[gasIndex].GasIdInSettings;
								betterDecoGasId = gasIndex;
							}
						}
			         }
			    }
				if(betterGasId != caveData.actualGas.GasIdInSettings)
				{
					caveData.actualGas.helium_percentage = pSettings->gas[betterGasId].helium_percentage;
					caveData.actualGas.nitrogen_percentage = 100 - pSettings->gas[betterGasId].oxygen_percentage - pSettings->gas[betterGasId].helium_percentage;
					caveData.actualGas.AppliedDiveMode = pDiveState->diveSettings.diveMode;
					caveData.actualGas.setPoint_cbar = 1.0;		/* TODO: solve CCR mode */
					caveData.actualGas.GasIdInSettings = betterGasId;
				}

		/* stay on depth for the rest of the calculation interval */
#if 0
			timeForAscent_sec = deltaPressure * 600 / pDiveState->diveSettings.ascentRate_meterperminute;
			if(timeForAscent_sec < CAVE_CALC_INTERVALL)
			{
				decom_tissues_exposure2(CAVE_CALC_INTERVALL - timeForAscent_sec, &caveData.actualGas, endPressure, caveData.tissue_nitrogen_bar, caveData.tissue_helium_bar);
			}
#endif
			ceiling = calcCeiling(&pDiveState->diveSettings, &caveData, endPressure);
			nextStop_Bar = calcDeepestDecoPressure(&pDiveState->diveSettings, caveData.pressure_surface_bar, ceiling);
			if((nextStop_Bar != 0) && (endPressure <= (nextStop_Bar + pDiveState->lifeData.pressure_surface_bar)))		/* we reached a deco stop => wait till ceiling is safe again */
			{
				endPressure = nextStop_Bar + pDiveState->lifeData.pressure_surface_bar;						/* take care if stop depth changed during ascend (time in between old / new step is ignored) */
				doStop = 1;
			}

			if(!((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))		/* This includes normal deco calculation */
				|| ((((caveDataIndex - caveStartIndex + MiniLiveLogbook_getModDataOffset()) * dataResolutionSec) > returnTime_seconds )))
			{
				ttsWork += CAVE_DECO_STOP_RESOLUTION;
				caveData.gasDemand_Ltr[caveData.actualGas.GasIdInSettings] += pSettings->gasConsumption_travel_l_min * endPressure / (float)(60.0 / CAVE_DECO_STOP_RESOLUTION);
			}

		/* do stop if needed */
			if(doStop)
			{
				doStop = 0;
				currentStop_Bar = nextStop_Bar;
				while ((nextStop_Bar >= currentStop_Bar) && (currentStop_Bar != 0))							/* stay at deco stop till next stop is safe */
				{
					decom_tissues_exposure2(CAVE_DECO_STOP_RESOLUTION, &caveData.actualGas, endPressure, caveData.tissue_nitrogen_bar, caveData.tissue_helium_bar);
					ceiling = calcCeiling(&pDiveState->diveSettings, &caveData, endPressure);
					nextStop_Bar = calcDeepestDecoPressure(&pDiveState->diveSettings, pDiveState->lifeData.pressure_surface_bar, ceiling);
					if(!((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
								|| ((((caveDataIndex - caveStartIndex + MiniLiveLogbook_getModDataOffset()) * dataResolutionSec) > returnTime_seconds )))
					{
						ttsWork += CAVE_DECO_STOP_RESOLUTION;
						caveData.gasDemand_Ltr[caveData.actualGas.GasIdInSettings] += pSettings->gasConsumption_deco_l_min * endPressure / (float)(60.0 / CAVE_DECO_STOP_RESOLUTION);
					}
					if((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
					{
						MiniLiveLogbook_insertData(caveDataIndex, (barToDepthMeter(endPressure, pDiveState->lifeData.pressure_surface_bar) * 100), CAVE_DECO_STOP_RESOLUTION);
					}
					if(stopId < 10)
					{
						stopList[stopId].length += CAVE_DECO_STOP_RESOLUTION;
					}
				}
				if(stopId < 10)
				{
					stopList[stopId].depth = endPressure;
				}
				stopId++;
			}
		}
		/* sum up used gas */
		if(time_elapsed_ms(gasUsedUpdate_Tick, HAL_GetTick()) > (CAVE_CALC_INTERVALL * 1000))
		{
			caveMode_AddGasUsed(CAVE_CALC_INTERVALL);
		}
	}
}

uint32_t caveMode_GetTTS()
{
	return tts_Cave_Sec;
}

void caveMode_SetActive(uint8_t activeRequest)
{
	if(activeRequest)
	{
		switch(caveModeState)
		{
			case CAVEMODE_OFF:	MiniLiveLogbook_resetLiveData();
								liveDive = 0;
			// no break;
			case CAVEMODE_RECORDING_PAUSE: caveModeState = CAVEMODE_RECORDING;
				break;
			case CAVEMODE_RETURNING_PAUSE: caveModeState = CAVEMODE_RETURNING;
				break;
			default:
				break;
		}
	}
	else
	{
		liveDive = 0;
		switch(caveModeState)
		{
			case CAVEMODE_RECORDING: caveModeState = CAVEMODE_RECORDING_PAUSE;
				break;
			case CAVEMODE_RETURNING: caveModeState = CAVEMODE_RETURNING_PAUSE;
				break;
			default:
				break;
		}
	}
}

void caveMode_SetReturn(uint8_t returnRequest)
{
	uint16_t currentIndex = 0;

	if(!caveMode_isOff())
	{
		if(returnRequest)
		{
			if((caveModeState != CAVEMODE_RETURNING)) /* start to return */
			{
				returnStartIndex = (getMiniLiveReplayLength() -1 ) * getReplayDataResolution();	/* normalize value to seconds => a change of the data resolution will automatically be covered */
				caveDataIndex = 0;
				endPressure = caveData.pressure_surface_bar;
				returnTime_seconds = 0;
				returnTime_last = stateUsed->lifeData.dive_time_seconds;
				caveModeState = CAVEMODE_RETURNING;
				MiniLiveLogbook_mirrowMiniLiveToReplayLog();
				MiniLiveLogbook_copyReplayToModLive();
				Sim_SetReplayState(1);
				memcpy(&startGas, &stateUsed->lifeData.actualGas, sizeof(caveData.actualGas));
				memcpy (tissue_nitrogen_return_bar, &stateUsed->lifeData.tissue_nitrogen_bar, sizeof(caveData.tissue_nitrogen_bar));
				memcpy (tissue_helium_return_bar,  &stateUsed->lifeData.tissue_helium_bar, sizeof(caveData.tissue_helium_bar));
			}
		}
		else
		{
			if((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
			{
				caveDataIndex = 0;
				currentIndex = returnTime_seconds / getReplayDataResolution();
				MiniLiveLogbook_cutReplayAt(currentIndex);
				MiniLiveLogbook_copyLiveToModLive();
				caveModeState = CAVEMODE_RECORDING;
			}
		}
	}
}

uint8_t caveMode_isReturning(void)
{
	uint8_t retReturning = 0;
	if((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
	{
		retReturning = 1;
	}
	return retReturning;
}

uint8_t caveMode_isLiveDive(void)
{
	return liveDive;
}

void caveMode_SyncToMarker()
{
	uint16_t markerIndex = MiniLiveLogbook_getMarkerIndex();
	if((markerIndex != 0) && (caveMode_isReturning()))
	{
		returnTime_seconds = (markerIndex - returnStartIndex) * getReplayDataResolution();
		returnTime_last = returnTime_seconds;
		/* fill or cut live data to have the profil view in sync */
		MiniLiveLogbook_syncLiveDataTo(markerIndex);
	}
}

void caveMode_NotifyCompression()
{
	if(caveDataIndex == replayDataLength - 1) /* special condition last data item processed. Make sure condition is still true after compressing */
	{
		caveDataIndex /= 2;
		replayDataLength = caveDataIndex + 1;
	}
	else
	{
		caveDataIndex /= 2;
		replayDataLength /=2;
	}
}

void caveMode_AddGasUsed(uint16_t seconds)
{
	SSettings *pSettings = settingsGetPointer();
	uint8_t index = 0;
	const SDecoinfo * pDecoinfo = getDecoInfo();
	float nextStopDepth = 0.0;
	uint8_t currentGasConsumtion = 0;
	float localGasUsed = 0.0;
	/* find deepest stop */
	for(index = DECOINFO_STRUCT_MAX_STOPS-1; index > 0; index--)
		if(pDecoinfo->output_stop_length_seconds[index] != 0) break;

	if(pDecoinfo->output_stop_length_seconds[index] != 0)
	{
		if(index == 0)
		{
			nextStopDepth = stateUsed->diveSettings.last_stop_depth_bar;
		}
		else
		{
			index -= 1;		/* the first to stops are not defined linear */
			nextStopDepth = stateUsed->diveSettings.input_second_to_last_stop_depth_bar + (index * stateUsed->diveSettings.input_next_stop_increment_depth_bar);
		}
	}
	if((stateUsed->lifeData.pressure_ambient_bar - stateUsed->lifeData.pressure_surface_bar) < nextStopDepth + PRESSURE_150_CM)
	{
		currentGasConsumtion = pSettings->gasConsumption_deco_l_min;
	}
	else
	{
		currentGasConsumtion = pSettings->gasConsumption_travel_l_min;
	}
	localGasUsed = currentGasConsumtion * stateUsed->lifeData.pressure_ambient_bar / (float)(60.0 / seconds);
	if(localGasUsed < 1.0)
	{
		localGasUsed = 1.0;
	}
	stateUsedWrite->lifeData.gasUsed_Ltr[stateUsed->lifeData.actualGas.GasIdInSettings] += localGasUsed;
	gasUsedUpdate_Tick = HAL_GetTick();		/* updating tick in Addgas function allows the combined use with simulator which may trigger a longer time period */
}

#endif

uint8_t caveMode_isOff(void)
{
	uint8_t retOff = 1;
#ifdef ENABLE_CAVEMODE
	if(caveModeState != CAVEMODE_OFF)
	{
		retOff = 0;
	}
#endif
	return retOff;
}

