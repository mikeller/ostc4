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

static float maxCeiling = 0.0;

#define CAVE_CALC_INTERVALL			20		/* in seconds. Because cave calculation is just an estimation it is not done on seconds base. */
#define CAVE_DECO_STOP_RESOLUTION	20		/* in seconds. Granularity of time spend at deco stops */
#define PRESSURE_150_CM 0.15f
#define PRESSURE_THREE_METER 0.333334f
#define PRESSURE_HALF_METER 0.05f

static uint32_t tts_Cave_Sec = 0;
static float tissue_nitrogen_return_bar[16];
static float tissue_helium_return_bar[16];



static float getGfAtPressure(SDiveSettings *pDiveSettings, float pressure, float surface_Bar, float ceiling)
{
	float gfSteigung = 0.0f;
	float GF_low_depth_bar = 0.0;

	float retGF = 0;

	if(ceiling > maxCeiling)
	{
		maxCeiling = ceiling;
	}
	if(maxCeiling >= (surface_Bar - PRESSURE_150_CM))
	{
		GF_low_depth_bar = maxCeiling - surface_Bar;
	}
	else
	{
		GF_low_depth_bar = ceiling - surface_Bar;
	}

	if((pressure - surface_Bar) <= PRESSURE_HALF_METER)
	{
		retGF = ((float)pDiveSettings->gf_high) / 100.0f;
	}
	else if(pressure >= surface_Bar + GF_low_depth_bar)
	{
		retGF = ((float)pDiveSettings->gf_low) / 100.0f;
	}
	else
	{
		if(GF_low_depth_bar < 0)
		{
				GF_low_depth_bar = PRESSURE_THREE_METER; // just to prevent erratic behaviour if variable is not set
		}
		gfSteigung = ((float)(pDiveSettings->gf_high - pDiveSettings->gf_low))/ GF_low_depth_bar;

		retGF = (pDiveSettings->gf_high - gfSteigung * (pressure - surface_Bar) )/ 100.0f;
	}
	return retGF;
}

static float calcDeepestDecoPressure(SDiveSettings *pDiveSettings, float surface_Bar, float ceiling)
{
	float decoPressure_bar = 0.0;

	ceiling -= surface_Bar;

	if(ceiling > 0.0)
	{
		if((float)(ceiling - (float)pDiveSettings->last_stop_depth_bar) <= 0.0)
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

    if(depth_meter > 0)
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
	int ci;

	float GF_value = 0;

	global_ceiling = -1;

	GF_value = (float)pDiveSettings->gf_low;

	for (ci = 0; ci < 16; ci++)
	{
		if(pLifeData->tissue_helium_bar[ci] == 0)
		{
			tissue_inertgas_saturation = pLifeData->tissue_nitrogen_bar[ci];
			//
			inertgas_a = buehlmann_N2_a[ci];
			inertgas_b = buehlmann_N2_b[ci];
		}
		else
		{
			tissue_inertgas_saturation =  pLifeData->tissue_nitrogen_bar[ci] + pLifeData->tissue_helium_bar[ci];
			//
			inertgas_a = ( ( buehlmann_N2_a[ci] *  pLifeData->tissue_nitrogen_bar[ci]) + ( buehlmann_He_a[ci] * pLifeData->tissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
			inertgas_b = ( ( buehlmann_N2_b[ci] *  pLifeData->tissue_nitrogen_bar[ci]) + ( buehlmann_He_b[ci] * pLifeData->tissue_helium_bar[ci]) ) / tissue_inertgas_saturation;
		}

	/* first calc common ceiling, then datapt to GF */
		ceiling = inertgas_b *  ( tissue_inertgas_saturation - inertgas_a );
		GF_value = getGfAtPressure(pDiveSettings, pressure, pLifeData->pressure_surface_bar, ceiling);
		ceiling = (inertgas_b * ( tissue_inertgas_saturation - GF_value * inertgas_a ) ) / (GF_value - (inertgas_b * GF_value) + inertgas_b);

		if(ceiling > global_ceiling)
			global_ceiling = ceiling;
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
		   stateUsedWrite->lifeData.caveGasNeed_Ltr[index] = 0;
		   caveData.caveGasNeed_Ltr[index] = 0;
	   }
   }
   startGas.GasIdInSettings = 0xFF;
   maxCeiling = 0.0;
   liveDive = 1;
   returnTime_seconds = 0;
   returnTime_last = 0;
   if(pSettings->caveModeAutoStart)
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
	uint8_t doStop = 0;
	uint8_t betterGasId;
	uint8_t betterDecoGasId;

	static uint32_t ttsWork = 0;

	uint16_t* pDepthDataReplay;
	uint16_t* pDepthCalcSource;
	uint16_t dataResolutionSec = getReplayDataResolution();
	SSettings *pSettings = settingsGetPointer();
	float startPressure = 0.0;
	static float endPressure = 0.0;
	static float targetPressure = 0.0;
	float gasChangePressure = 0.0;
	float ceiling = 0.0;
	static float nextStop_Bar = 0.0;
	float currentStop_Bar = 0.0;
	static uint8_t stopId = 0;
	static uint16_t replayDataLength = 0;

	static uint16_t caveStartIndex = 0;
	uint16_t currentDepthMeter = 0;
	uint8_t gasIndex = 0;
    uint8_t iterationCnt = 10;		/* do 10 calculations a 20 seconds => 3.x minutes */


	if((pDiveState->mode == MODE_DIVE) && (getMiniLiveReplayLength() > 30) && (caveModeState != CAVEMODE_OFF))	/* start calculation one minute after begin of dive */
	{
		getReplayInfo(&pDepthDataReplay, NULL, &replayDataLength, NULL, NULL);
		{

		}
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
		if((caveDataIndex == 0) || (((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE)) && (caveDataIndex >= replayDataLength )))			/* start next iteration */
		{
			caveData.pressure_surface_bar = pDiveState->lifeData.pressure_surface_bar;
			if((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
			{
				caveDataIndex = returnStartIndex / dataResolutionSec;
				caveStartIndex = caveDataIndex;
				memcpy(&caveData.actualGas, &startGas, sizeof(caveData.actualGas));
				memcpy (caveData.tissue_nitrogen_bar, tissue_nitrogen_return_bar, sizeof(caveData.tissue_nitrogen_bar));
				memcpy (caveData.tissue_helium_bar, tissue_helium_return_bar, sizeof(caveData.tissue_helium_bar));
				MiniLiveLogbook_releaseModData();
				MiniLiveLogbook_resetModData();
			}
			else
			{
				caveDataIndex = getMiniLiveReplayLength() -1;
				memcpy(&caveData.actualGas, &pDiveState->lifeData.actualGas, sizeof(caveData.actualGas));
				memcpy (caveData.tissue_nitrogen_bar, pDiveState->lifeData.tissue_nitrogen_bar, sizeof(caveData.tissue_nitrogen_bar));
				memcpy (caveData.tissue_helium_bar, pDiveState->lifeData.tissue_helium_bar, sizeof(caveData.tissue_helium_bar));
			}

			memcpy (pDiveState->lifeData.caveGasNeed_Ltr, caveData.caveGasNeed_Ltr, sizeof(caveData.caveGasNeed_Ltr));	/* publish results of last iteration */
			memset (caveData.caveGasNeed_Ltr, 0, sizeof(caveData.caveGasNeed_Ltr));
			tts_Cave_Sec = ttsWork;		/* copy result of last calculation cycle */
			ttsWork = 0;
			memset (stopList, 0, sizeof (stopList));
			stopId = 0;
			nextStop_Bar = 0.0;
			targetPressure = 0.0;
			endPressure = 0.0;
			caveData.depth_meter = pDiveState->lifeData.max_depth_meter;			/* use max depth to create a list of all possible gases */
			decom_CreateGasChangeList(&stateUsedWrite->diveSettings, &caveData);
		}
		while((caveDataIndex > 0) && (caveDataIndex < replayDataLength) && (iterationCnt > 0))
		{
			iterationCnt--;
			startPressure = endPressure;
			if(targetPressure == endPressure)			/* get next depth */
			{
				caveDataStep = CAVE_CALC_INTERVALL / dataResolutionSec;
				if((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
				{
					if(caveDataIndex == replayDataLength - 1)		/* last data entry already processed => exit loop */
					{
						caveDataIndex++;
						break;
					}
					if((caveDataIndex + caveDataStep) >= replayDataLength )
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

				currentDepthMeter = pDepthCalcSource[caveDataIndex] / 100.0;
				endPressure = depthMeterToBar(currentDepthMeter, pDiveState->lifeData.pressure_surface_bar);
				targetPressure = endPressure;
			}
			else										/* continue ascending to target depth */
			{
				endPressure = targetPressure;
			}

			if((nextStop_Bar != 0) && (endPressure < (nextStop_Bar + pDiveState->lifeData.pressure_surface_bar)))		/* we reached a deco stop => wait till ceiling is safe again */
			{
				doStop = 1;
				endPressure = nextStop_Bar + pDiveState->lifeData.pressure_surface_bar; // + PRESSURE_THREE_METER;	/* three meters above stop */
			}

		/* Ascent */
			decom_tissues_exposure_stage_schreiner(CAVE_CALC_INTERVALL, &caveData.actualGas, startPressure, endPressure,
																		caveData.tissue_nitrogen_bar,  caveData.tissue_helium_bar);

			ceiling = calcCeiling(&pDiveState->diveSettings, &caveData, endPressure);
			currentStop_Bar = nextStop_Bar;
			nextStop_Bar = calcDeepestDecoPressure(&pDiveState->diveSettings, pDiveState->lifeData.pressure_surface_bar, ceiling);

			if(!((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
				|| ((((caveDataIndex - caveStartIndex + MiniLiveLogbook_getModDataOffset()) * dataResolutionSec) > returnTime_seconds )))
			{
				ttsWork += CAVE_DECO_STOP_RESOLUTION;
				caveData.caveGasNeed_Ltr[caveData.actualGas.GasIdInSettings] += pSettings->gasConsumption_deco_l_min * barToDepthMeter(endPressure, pDiveState->lifeData.pressure_surface_bar ) / (60 / CAVE_DECO_STOP_RESOLUTION);
			}
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

		/* do stop id step is reached */
			if(doStop)
			{
				while(doStop)
				{
					currentStop_Bar = nextStop_Bar;
					doStop = 0;
					while (nextStop_Bar >= currentStop_Bar)									/* stay at deco stop till next stop is safe */
					{
						decom_tissues_exposure2(CAVE_DECO_STOP_RESOLUTION, &caveData.actualGas, endPressure, caveData.tissue_nitrogen_bar, caveData.tissue_helium_bar);
						ceiling = calcCeiling(&pDiveState->diveSettings, &caveData, endPressure);
						nextStop_Bar = calcDeepestDecoPressure(&pDiveState->diveSettings, pDiveState->lifeData.pressure_surface_bar, ceiling);
						if(!((caveModeState == CAVEMODE_RETURNING) || (caveModeState == CAVEMODE_RETURNING_PAUSE))
									|| ((((caveDataIndex - caveStartIndex + MiniLiveLogbook_getModDataOffset()) * dataResolutionSec) > returnTime_seconds )))
						{
							ttsWork += CAVE_DECO_STOP_RESOLUTION;
							caveData.caveGasNeed_Ltr[caveData.actualGas.GasIdInSettings] += pSettings->gasConsumption_travel_l_min * barToDepthMeter(endPressure, pDiveState->lifeData.pressure_surface_bar ) / (60 / CAVE_DECO_STOP_RESOLUTION);
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
			else
			{
				nextStop_Bar = calcDeepestDecoPressure(&pDiveState->diveSettings, pDiveState->lifeData.pressure_surface_bar, ceiling);
			}
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
	if((returnRequest) && (caveModeState != CAVEMODE_RETURNING))  /* start to return */
	{
		returnStartIndex = (getMiniLiveReplayLength() -1 ) * getReplayDataResolution();	/* normalize value to seconds => a change of the data resolution will automatically be covered */
		caveDataIndex = 0;
		caveModeState = CAVEMODE_RETURNING;
		MiniLiveLogbook_mirrowMiniModToReplayLog();
		MiniLiveLogbook_copyReplayToModLive();
		Sim_SetReplayState(1);
		memcpy(&startGas, &stateUsed->lifeData.actualGas, sizeof(caveData.actualGas));
		memcpy (tissue_nitrogen_return_bar, &stateUsed->lifeData.tissue_nitrogen_bar, sizeof(caveData.tissue_nitrogen_bar));
		memcpy (tissue_helium_return_bar,  &stateUsed->lifeData.tissue_helium_bar, sizeof(caveData.tissue_helium_bar));
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
