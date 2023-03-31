/**
  ******************************************************************************
	* @copyright heinrichs weikamp
  * @file   		data_central.c
  * @author 		heinrichs weikamp gmbh
  * @date   		10-November-2014
  * @version		V1.0.2
  * @since			10-Nov-2014
  * @brief			All the data EXCEPT
  *							 - settings (settings.c)
	*									feste Werte, die nur an der Oberfl�che ge�ndert werden
	*							 - dataIn and dataOut (data_exchange.h and data_exchange_main.c)
	*									Austausch mit Small CPU
	* @bug
	* @warning
  @verbatim
  ==============================================================================
              ##### SDiveState Real and Sim #####
  ==============================================================================
  [..] SDiveSettings
				copy of parts of Settings that are necessary during the dive
				and could be modified during the dive without post dive changes.

  [..] SLifeData
				written in DataEX_copy_to_LifeData();
				block 1 "lifedata" set by SmallCPU in stateReal
				block 2 "actualGas" set by main CPU from user input and send to Small CPU
				block 3 "calculated data" set by main CPU based on "lifedata"

  [..] SVpm

	[..] SEvents

  [..] SDecoinfo

  [..] mode
				set by SmallCPU in stateReal, can be surface, dive, ...

  [..] data_old__lost_connection_to_slave
				set by DataEX_copy_to_LifeData();

  ==============================================================================
              ##### SDiveState Deco #####
  ==============================================================================
  [..] kjbkldafj�lasdfjasdf

  ==============================================================================
              ##### decoLock #####
  ==============================================================================
  [..] The handler that synchronizes the data between IRQ copy and main deco loop


	 @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <math.h>
#include "data_central.h"
#include "calc_crush.h"
#include "decom.h"
#include "stm32f4xx_hal.h"
#include "settings.h"
#include "data_exchange_main.h"
#include "ostc.h" // for button adjust on hw testboard 1
#include "tCCR.h"
#include "crcmodel.h"
#include "configuration.h"

static SDiveState stateReal = { 0 };
SDiveState stateSim = { 0 };
SDiveState stateDeco = { 0 };

static SDevice stateDevice =
{
	/* max is 0x7FFFFFFF, min is 0x80000000 but also defined in stdint.h :-) */

	/* count, use 0 */
	.batteryChargeCompleteCycles.value_int32 = 0,
	.batteryChargeCycles.value_int32 = 0,
	.diveCycles.value_int32 = 0,
	.hoursOfOperation.value_int32 = 0,

	/* max values, use min. */
	.temperatureMaximum.value_int32 = INT32_MIN,
	.depthMaximum.value_int32 = INT32_MIN,

	/* min values, use max. */
	.temperatureMinimum.value_int32 = INT32_MAX,
	.voltageMinimum.value_int32 = INT32_MAX,
};

static SVpmRepetitiveData stateVPM =
{
	.repetitive_variables_not_valid = 1,
	.is_data_from_RTE_CPU = 0,
};

const SDiveState *stateUsed = &stateReal;
SDiveState *stateUsedWrite = &stateReal;


#define COMPASS_FRACTION		(4.0f)		/* delay till value changes to new actual */

static float compass_compensated = 0;

void set_stateUsedToReal(void)
{
	stateUsed = stateUsedWrite = &stateReal;
}

void set_stateUsedToSim(void)
{
	stateUsed = stateUsedWrite = &stateSim;
}

_Bool is_stateUsedSetToSim(void)
{
	return stateUsed == &stateSim;
}

const SDiveState * stateRealGetPointer(void)
{
	return &stateReal;
}

SDiveState * stateRealGetPointerWrite(void)
{
	return &stateReal;
}


const SDiveState * stateSimGetPointer(void)
{
	return &stateSim;
}


SDiveState * stateSimGetPointerWrite(void)
{
	return &stateSim;
}


const SDevice * stateDeviceGetPointer(void)
{
	return &stateDevice;
}


SDevice * stateDeviceGetPointerWrite(void)
{
	return &stateDevice;
}


const SVpmRepetitiveData * stateVpmRepetitiveDataGetPointer(void)
{
	return &stateVPM;
}


SVpmRepetitiveData * stateVpmRepetitiveDataGetPointerWrite(void)
{
	return &stateVPM;
}


uint32_t time_elapsed_ms(uint32_t ticksstart,uint32_t ticksnow)
{
	if(ticksstart <= ticksnow)
		return ticksnow - ticksstart;
	else
		return 0xFFFFFFFF - ticksstart + ticksnow;
}


uint8_t decoLock = DECO_CALC_undefined;

static int descent_rate_meter_per_min  = 20;
static int max_depth = 70;
static int bottom_time = 10;

_Bool vpm_crush(SDiveState* pDiveState);
void setSimulationValues(int _ascent_rate_meter_per_min, int _descent_rate_meter_per_min, int _max_depth, int _bottom_time )
{
    descent_rate_meter_per_min = _descent_rate_meter_per_min;
    max_depth = _max_depth;
    bottom_time = _bottom_time;
}

int current_second(void) {

    return HAL_GetTick() / 1000;
}

#define OXY_ONE_SIXTIETH_PART 			0.0166667f

uint8_t calc_MOD(uint8_t gasId)
{
	int16_t oxygen, maxppO2, result;
	SSettings *pSettings;

	pSettings = settingsGetPointer();

	oxygen = (int16_t)(pSettings->gas[gasId].oxygen_percentage);

	if(pSettings->gas[gasId].note.ub.deco > 0)
		maxppO2 =(int16_t)(pSettings->ppO2_max_deco);
	else
		maxppO2 =(int16_t)(pSettings->ppO2_max_std);

	result = 10 *  maxppO2;
	result /= oxygen;
	result -= 10;

	if(result < 0)
		return 0;

	if(result > 255)
		return 255;

	return result;
}

float get_ambiant_pressure_simulation(long dive_time_seconds, float surface_pressure_bar )
{
  static
    long descent_time;
    float depth_meter;

    descent_time = 60 * max_depth / descent_rate_meter_per_min;

    if(dive_time_seconds <= descent_time)
    {
        depth_meter = ((float)(dive_time_seconds * descent_rate_meter_per_min)) / 60;
        return surface_pressure_bar + depth_meter / 10;
    }
    //else if(dive_time_seconds <= (descent_time + bottom_time * 60))
    return surface_pressure_bar + max_depth / 10;



}

void UpdateLifeDataTest(SDiveState * pDiveState)
{
    static int last_second = -1;
    int now =  current_second();
    if(last_second == now)
        return;
    last_second = now;

    pDiveState->lifeData.dive_time_seconds += 1;
    pDiveState->lifeData.pressure_ambient_bar = get_ambiant_pressure_simulation(pDiveState->lifeData.dive_time_seconds,pDiveState->lifeData.pressure_surface_bar);

    pDiveState->lifeData.depth_meter = (pDiveState->lifeData.pressure_ambient_bar - pDiveState->lifeData.pressure_surface_bar) * 10.0f;
		if(pDiveState->lifeData.max_depth_meter < pDiveState->lifeData.depth_meter)
				pDiveState->lifeData.max_depth_meter = pDiveState->lifeData.depth_meter;
    decom_tissues_exposure(1, &pDiveState->lifeData);
    pDiveState->lifeData.ppO2 = decom_calc_ppO2( pDiveState->lifeData.pressure_ambient_bar, &pDiveState->lifeData.actualGas);
    decom_oxygen_calculate_cns(& pDiveState->lifeData.cns, pDiveState->lifeData.ppO2);

    vpm_crush(pDiveState);
}


_Bool vpm_crush(SDiveState* pDiveState)
{
    int i = 0;
    static float starting_ambient_pressure = 0;
    static float ending_ambient_pressure = 0;
    static float time_calc_begin = -1;
	static float initial_helium_pressure[16];
	static float initial_nitrogen_pressure[16];
	ending_ambient_pressure = pDiveState->lifeData.pressure_ambient_bar * 10;

	if((pDiveState->lifeData.dive_time_seconds <= 4) || (starting_ambient_pressure >= ending_ambient_pressure))
	{
		time_calc_begin = pDiveState->lifeData.dive_time_seconds;
		starting_ambient_pressure = pDiveState->lifeData.pressure_ambient_bar * 10;
		for( i = 0; i < 16; i++)
		{
			initial_helium_pressure[i] = pDiveState->lifeData.tissue_helium_bar[i] * 10;
			initial_nitrogen_pressure[i] = pDiveState->lifeData.tissue_nitrogen_bar[i] * 10;
		}
		return false;
	}
	if(pDiveState->lifeData.dive_time_seconds - time_calc_begin >= 4)
	{
		if(ending_ambient_pressure > starting_ambient_pressure + 0.5f)
		{
			float rate = (ending_ambient_pressure - starting_ambient_pressure) * 60 / 4;
			calc_crushing_pressure(&pDiveState->lifeData, &pDiveState->vpm, initial_helium_pressure, initial_nitrogen_pressure, starting_ambient_pressure, rate);

			time_calc_begin = pDiveState->lifeData.dive_time_seconds;
			starting_ambient_pressure = pDiveState->lifeData.pressure_ambient_bar * 10;
			for( i = 0; i < 16; i++)
			{
				initial_helium_pressure[i] = pDiveState->lifeData.tissue_helium_bar[i] * 10;
				initial_nitrogen_pressure[i] =  pDiveState->lifeData.tissue_nitrogen_bar[i] * 10;
			}

			return true;
		}

	}
	return false;
};


void createDiveSettings(void)
{
	int i;
	SSettings* pSettings = settingsGetPointer();

	stateReal.diveSettings.compassHeading = pSettings->compassBearing;
	stateReal.diveSettings.ascentRate_meterperminute = 10;

	stateReal.diveSettings.diveMode = pSettings->dive_mode;
	stateReal.diveSettings.CCR_Mode = pSettings->CCR_Mode;
	if((stateReal.diveSettings.diveMode == DIVEMODE_PSCR) && (stateReal.diveSettings.CCR_Mode == CCRMODE_FixedSetpoint))
	{
		/* TODO: update selection of sensor used on/off (currently sensor/fixpoint). As PSCR has no fixed setpoint change to simulated ppo2 if sensors are not active */
		stateReal.diveSettings.CCR_Mode = CCRMODE_Simulation;
	}

	if(isLoopMode(stateReal.diveSettings.diveMode))
		stateReal.diveSettings.ccrOption = 1;
	else
		stateReal.diveSettings.ccrOption = 0;
	memcpy(stateReal.diveSettings.gas, pSettings->gas,sizeof(pSettings->gas));
	memcpy(stateReal.diveSettings.setpoint, pSettings->setpoint,sizeof(pSettings->setpoint));

	setActualGasFirst(&stateReal.lifeData);

	stateReal.diveSettings.gf_high = pSettings->GF_high;
	stateReal.diveSettings.gf_low = pSettings->GF_low;
	stateReal.diveSettings.input_next_stop_increment_depth_bar = ((float)pSettings->stop_increment_depth_meter) / 10.0f;
	stateReal.diveSettings.last_stop_depth_bar = ((float)pSettings->last_stop_depth_meter) / 10.0f;
	stateReal.diveSettings.vpm_conservatism = pSettings->VPM_conservatism.ub.standard;
	stateReal.diveSettings.deco_type.uw = pSettings->deco_type.uw;
	stateReal.diveSettings.fallbackOption = pSettings->fallbackToFixedSetpoint;
	stateReal.diveSettings.ppo2sensors_deactivated = pSettings->ppo2sensors_deactivated;
	stateReal.diveSettings.future_TTS_minutes = pSettings->future_TTS;
	
	stateReal.diveSettings.pscr_lung_ratio = pSettings->pscr_lung_ratio;
	stateReal.diveSettings.pscr_o2_drop = pSettings->pscr_o2_drop;

	if(stateReal.diveSettings.diveMode == DIVEMODE_PSCR)
	{
		for(i=0; i<5; i++)
		{
			stateReal.diveSettings.decogaslist[i].pscr_factor = 1.0 / stateReal.diveSettings.pscr_lung_ratio * stateReal.diveSettings.pscr_o2_drop;
		}
	}

	decom_CreateGasChangeList(&stateReal.diveSettings, &stateReal.lifeData); // decogaslist
	stateReal.diveSettings.internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero = 0;

	/* for safety */
	stateReal.diveSettings.input_second_to_last_stop_depth_bar = stateReal.diveSettings.last_stop_depth_bar + stateReal.diveSettings.input_next_stop_increment_depth_bar;
	/* and the proper calc */
	for(i = 1; i <10; i++)
	{
		if(stateReal.diveSettings.input_next_stop_increment_depth_bar * i > stateReal.diveSettings.last_stop_depth_bar)
		{
			 stateReal.diveSettings.input_second_to_last_stop_depth_bar = stateReal.diveSettings.input_next_stop_increment_depth_bar * i;
			 break;
		}
	}
}


void copyDiveSettingsToSim(void)
{
	memcpy(&stateSim, &stateReal, sizeof(stateReal));
}


void copyVpmRepetetiveDataToSim(void)
{
	SDiveState * pSimData = stateSimGetPointerWrite();
	const SVpmRepetitiveData * pVpmData = stateVpmRepetitiveDataGetPointer();
	
	if(pVpmData->is_data_from_RTE_CPU)
	{
		for(int i=0; i<16;i++)
		{
			pSimData->vpm.adjusted_critical_radius_he[i] = pVpmData->adjusted_critical_radius_he[i];
			pSimData->vpm.adjusted_critical_radius_n2[i] = pVpmData->adjusted_critical_radius_n2[i];

			pSimData->vpm.adjusted_crushing_pressure_he[i] = pVpmData->adjusted_crushing_pressure_he[i];
			pSimData->vpm.adjusted_crushing_pressure_n2[i] = pVpmData->adjusted_crushing_pressure_n2[i];

			pSimData->vpm.initial_allowable_gradient_he[i] = pVpmData->initial_allowable_gradient_he[i];
			pSimData->vpm.initial_allowable_gradient_n2[i] = pVpmData->initial_allowable_gradient_n2[i];

			pSimData->vpm.max_actual_gradient[i] = pVpmData->max_actual_gradient[i];
		}
		pSimData->vpm.repetitive_variables_not_valid = pVpmData->repetitive_variables_not_valid;
	}
}




void updateSetpointStateUsed(void)
{
	if(!isLoopMode(stateUsed->diveSettings.diveMode))
	{
		stateUsedWrite->lifeData.actualGas.setPoint_cbar = 0;
		stateUsedWrite->lifeData.ppO2 = decom_calc_ppO2(stateUsed->lifeData.pressure_ambient_bar, &stateUsed->lifeData.actualGas);
	}
	else
	{
		if(stateUsed->diveSettings.CCR_Mode == CCRMODE_Sensors)
		{
			stateUsedWrite->lifeData.actualGas.setPoint_cbar = get_ppO2SensorWeightedResult_cbar();
		}
#ifdef ENABLE_PSCR_MODE
		if(stateUsed->diveSettings.diveMode == DIVEMODE_PSCR)	/* calculate a ppO2 value based on assumptions ( transfered approach from hwos code) */
		{
			stateUsedWrite->lifeData.ppo2Simulated_bar = decom_calc_SimppO2_O2based(stateUsed->lifeData.pressure_ambient_bar, stateReal.diveSettings.gas[stateUsed->lifeData.actualGas.GasIdInSettings].oxygen_percentage, stateUsed->lifeData.actualGas.pscr_factor);
			if(stateUsed->diveSettings.CCR_Mode == CCRMODE_Simulation)
			{
				stateUsedWrite->lifeData.actualGas.setPoint_cbar = stateUsedWrite->lifeData.ppo2Simulated_bar * 100;
			}
		}
#endif
	/* limit calculated value to the physically possible if needed */
		if((stateUsed->lifeData.pressure_ambient_bar * 100) < stateUsed->lifeData.actualGas.setPoint_cbar)
			stateUsedWrite->lifeData.ppO2 = stateUsed->lifeData.pressure_ambient_bar;
		else
			stateUsedWrite->lifeData.ppO2 = ((float)stateUsed->lifeData.actualGas.setPoint_cbar) / 100;
	}
}

void setActualGasFirst(SLifeData *lifeData)
{
	SSettings* pSettings = settingsGetPointer();
	uint8_t start = 0;
	uint8_t gasId = 0;
	uint8_t setpoint_cbar = 0;

	if(isLoopMode(pSettings->dive_mode))
	{
		setpoint_cbar = pSettings->setpoint[1].setpoint_cbar;
		start = NUM_OFFSET_DILUENT+1;
	}
	else
	{
		setpoint_cbar = 0;
		start = 1;
	}

	gasId = start;
	for(int i=start;i<=NUM_GASES+start;i++)
	{
		if(pSettings->gas[i].note.ub.first)
		{
			gasId = i;
			break;
		}
	}
	setActualGas(lifeData, gasId, setpoint_cbar);
}

void setActualGasAir(SLifeData *lifeData)
{
	uint8_t nitrogen;
	nitrogen = 79;
	lifeData->actualGas.GasIdInSettings = 0;
	lifeData->actualGas.nitrogen_percentage = nitrogen;
	lifeData->actualGas.helium_percentage =0;
	lifeData->actualGas.setPoint_cbar = 0;
	lifeData->actualGas.change_during_ascent_depth_meter_otherwise_zero = 0;
	lifeData->actualGas.AppliedDiveMode = stateUsed->diveSettings.diveMode;
}


void setActualGas(SLifeData *lifeData, uint8_t gasId, uint8_t setpoint_cbar)
{
	SSettings* pSettings = settingsGetPointer();
	uint8_t nitrogen;

	nitrogen = 100;
	nitrogen -= pSettings->gas[gasId].oxygen_percentage;
	nitrogen -= pSettings->gas[gasId].helium_percentage;

	lifeData->actualGas.GasIdInSettings = gasId;
	lifeData->actualGas.nitrogen_percentage = nitrogen;
	lifeData->actualGas.helium_percentage = pSettings->gas[gasId].helium_percentage;
	lifeData->actualGas.setPoint_cbar = setpoint_cbar;
	lifeData->actualGas.change_during_ascent_depth_meter_otherwise_zero = 0;
	lifeData->actualGas.AppliedDiveMode = stateUsed->diveSettings.diveMode;
	lifeData->actualGas.pscr_factor = 1.0 / pSettings->pscr_lung_ratio * pSettings->pscr_o2_drop;
	if (isLoopMode(pSettings->dive_mode) && gasId > NUM_OFFSET_DILUENT) {
		lifeData->lastDiluent_GasIdInSettings = gasId;
        lifeData->lastSetpointChangeDepthM = lifeData->depth_meter;
    }
}


void setActualGas_DM(SLifeData *lifeData, uint8_t gasId, uint8_t setpoint_cbar)
{
    if(stateUsed->diveSettings.ccrOption && gasId < 6)
    {
      if(lifeData->actualGas.GasIdInSettings != gasId)
      {
        SSettings* pSettings = settingsGetPointer();
        stateUsedWrite->events.bailout = 1;
        stateUsedWrite->events.info_bailoutO2 = pSettings->gas[gasId].oxygen_percentage;
        stateUsedWrite->events.info_bailoutHe = pSettings->gas[gasId].helium_percentage;
      }
    }
    else
    {
      if(lifeData->actualGas.GasIdInSettings != gasId)
      {
    	  stateUsedWrite->events.gasChange = 1;
    	  stateUsedWrite->events.info_GasChange = gasId;
      }
      if(	lifeData->actualGas.setPoint_cbar != setpoint_cbar)
      {
				// setPoint_cbar = 255 -> change to sensor mode
    	  stateUsedWrite->events.setpointChange = 1;
    	  stateUsedWrite->events.info_SetpointChange = setpoint_cbar;
      }
    }
	setActualGas(lifeData, gasId, setpoint_cbar);
}

void setActualGas_ExtraGas(SLifeData *lifeData, uint8_t oxygen, uint8_t helium, uint8_t setpoint_cbar)
{
	uint8_t nitrogen;

	nitrogen = 100;
	nitrogen -= oxygen;
	nitrogen -= helium;


	if((lifeData->actualGas.nitrogen_percentage != nitrogen) || (lifeData->actualGas.helium_percentage != helium) || (lifeData->actualGas.AppliedDiveMode != DIVEMODE_OC))
    {
    	stateUsedWrite->events.manualGasSet = 1;
    	stateUsedWrite->events.info_manualGasSetHe = helium;
    	stateUsedWrite->events.info_manualGasSetO2 = oxygen;
    }
    if(	lifeData->actualGas.setPoint_cbar != setpoint_cbar)
    {
    	stateUsedWrite->events.setpointChange = 1;
    	stateUsedWrite->events.info_SetpointChange = setpoint_cbar;
    }
  lifeData->actualGas.GasIdInSettings = 0;
  lifeData->actualGas.nitrogen_percentage = nitrogen;
  lifeData->actualGas.helium_percentage = helium;
  lifeData->actualGas.setPoint_cbar = setpoint_cbar;
  lifeData->actualGas.change_during_ascent_depth_meter_otherwise_zero = 0;
  lifeData->actualGas.AppliedDiveMode = stateUsed->diveSettings.diveMode;
}

void setButtonResponsiveness(uint8_t *ButtonSensitivyList)
{
	SDataReceiveFromMaster	*pDataOut = dataOutGetPointer();

	for(int i=0; i<4; i++)
	{
		pDataOut->data.buttonResponsiveness[i] = settingsHelperButtonSens_translate_percentage_to_hwOS_values(ButtonSensitivyList[i]);
	}
	pDataOut->setButtonSensitivityNow = 1;
}


void setDate(RTC_DateTypeDef Sdate)
{
	SDataReceiveFromMaster	*pDataOut = dataOutGetPointer();

	pDataOut->data.newDate = Sdate;
	pDataOut->setDateNow = 1;
}


void setTime(RTC_TimeTypeDef Stime)
{
	SDataReceiveFromMaster	*pDataOut = dataOutGetPointer();

	pDataOut->data.newTime = Stime;
	pDataOut->setTimeNow = 1;
}


void setBatteryPercentage(uint8_t newChargePercentage)
{
	SDataReceiveFromMaster	*pDataOut = dataOutGetPointer();

	pDataOut->data.newBatteryGaugePercentageFloat = settingsGetPointer()->lastKnownBatteryPercentage;
	pDataOut->setBatteryGaugeNow = 1;
}


void calibrateCompass(void)
{
	SDataReceiveFromMaster	*pDataOut = dataOutGetPointer();
	pDataOut->calibrateCompassNow = 1;
}


void clearDeco(void)
{
	SDataReceiveFromMaster	*pDataOut = dataOutGetPointer();
	pDataOut->clearDecoNow = 1;
	
	stateRealGetPointerWrite()->cnsHigh_at_the_end_of_dive = 0;
	stateRealGetPointerWrite()->decoMissed_at_the_end_of_dive	= 0;
}


static int32_t helper_days_from_civil(int32_t y, uint32_t m, uint32_t d)
{
		y += 2000;
    y -= m <= 2;
    int32_t era = (y >= 0 ? y : y-399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);      // [0, 399]
    uint32_t doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d-1;  // [0, 365]
    uint32_t doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]
    return era * 146097 + (int32_t)(doe) - 719468;
}


static uint8_t helper_weekday_from_days(int32_t z)
{
    return (uint8_t)(z >= -4 ? (z+4) % 7 : (z+5) % 7 + 6);
}


void setWeekday(RTC_DateTypeDef *sDate)
{
	uint8_t day;
	// [0, 6] -> [Sun, Sat]
	day = helper_weekday_from_days(helper_days_from_civil(sDate->Year, sDate->Month, sDate->Date));
	// [1, 7] -> [Mon, Sun]
	if(day == 0)
		day = 7;
	sDate->WeekDay = day;
}


void translateDate(uint32_t datetmpreg, RTC_DateTypeDef *sDate)
{
  datetmpreg = (uint32_t)(datetmpreg & RTC_DR_RESERVED_MASK);

  /* Fill the structure fields with the read parameters */
  sDate->Year = (uint8_t)((datetmpreg & (RTC_DR_YT | RTC_DR_YU)) >> 16);
  sDate->Month = (uint8_t)((datetmpreg & (RTC_DR_MT | RTC_DR_MU)) >> 8);
  sDate->Date = (uint8_t)(datetmpreg & (RTC_DR_DT | RTC_DR_DU));
  sDate->WeekDay = (uint8_t)((datetmpreg & (RTC_DR_WDU)) >> 13);

	/* Convert the date structure parameters to Binary format */
	sDate->Year = (uint8_t)RTC_Bcd2ToByte(sDate->Year);
	sDate->Month = (uint8_t)RTC_Bcd2ToByte(sDate->Month);
	sDate->Date = (uint8_t)RTC_Bcd2ToByte(sDate->Date);
}

void translateTime(uint32_t tmpreg, RTC_TimeTypeDef *sTime)
{
  tmpreg = (uint32_t)(tmpreg & RTC_TR_RESERVED_MASK);

  /* Fill the structure fields with the read parameters */
  sTime->Hours = (uint8_t)((tmpreg & (RTC_TR_HT | RTC_TR_HU)) >> 16);
  sTime->Minutes = (uint8_t)((tmpreg & (RTC_TR_MNT | RTC_TR_MNU)) >>8);
  sTime->Seconds = (uint8_t)(tmpreg & (RTC_TR_ST | RTC_TR_SU));
  sTime->TimeFormat = (uint8_t)((tmpreg & (RTC_TR_PM)) >> 16);

	/* Convert the time structure parameters to Binary format */
	sTime->Hours = (uint8_t)RTC_Bcd2ToByte(sTime->Hours);
	sTime->Minutes = (uint8_t)RTC_Bcd2ToByte(sTime->Minutes);
	sTime->Seconds = (uint8_t)RTC_Bcd2ToByte(sTime->Seconds);
  sTime->SubSeconds = 0;
}

void resetEvents(const SDiveState *pStateUsed)
{
	memset((void *)&pStateUsed->events, 0, sizeof(SEvents));
}


uint32_t	CRC_CalcBlockCRC_moreThan768000(uint32_t *buffer1, uint32_t *buffer2, uint32_t words)
{
 cm_t        crc_model;
 uint32_t      word_to_do;
 uint8_t       byte_to_do;
 int         i;
 
     // Values for the STM32F generator.
 
     crc_model.cm_width = 32;            // 32-bit CRC
     crc_model.cm_poly  = 0x04C11DB7;    // CRC-32 polynomial
     crc_model.cm_init  = 0xFFFFFFFF;    // CRC initialized to 1's
     crc_model.cm_refin = FALSE;         // CRC calculated MSB first
     crc_model.cm_refot = FALSE;         // Final result is not bit-reversed
     crc_model.cm_xorot = 0x00000000;    // Final result XOR'ed with this
 
     cm_ini(&crc_model);
 
     while (words--)
     {
         // The STM32F10x hardware does 32-bit words at a time!!!
				if(words > (768000/4))
					word_to_do = *buffer2++;
				else
					word_to_do = *buffer1++;
 
         // Do all bytes in the 32-bit word.
 
         for (i = 0; i < sizeof(word_to_do); i++)
         {
             // We calculate a *byte* at a time. If the CRC is MSB first we
             // do the next MS byte and vica-versa.
 
             if (crc_model.cm_refin == FALSE)
             {
                 // MSB first. Do the next MS byte.
 
                 byte_to_do = (uint8_t) ((word_to_do & 0xFF000000) >> 24);
                 word_to_do <<= 8;
             }
             else
             {
                 // LSB first. Do the next LS byte.
 
                 byte_to_do = (uint8_t) (word_to_do & 0x000000FF);
                 word_to_do >>= 8;
             }
 
             cm_nxt(&crc_model, byte_to_do);
         }
     }
 
     // Return the final result.
 
     return (cm_crc(&crc_model));
}
 
 
uint32_t	CRC_CalcBlockCRC(uint32_t *buffer, uint32_t words)
{
 cm_t        crc_model;
 uint32_t      word_to_do;
 uint8_t       byte_to_do;
 int         i;
 
     // Values for the STM32F generator.
 
     crc_model.cm_width = 32;            // 32-bit CRC
     crc_model.cm_poly  = 0x04C11DB7;    // CRC-32 polynomial
     crc_model.cm_init  = 0xFFFFFFFF;    // CRC initialized to 1's
     crc_model.cm_refin = FALSE;         // CRC calculated MSB first
     crc_model.cm_refot = FALSE;         // Final result is not bit-reversed
     crc_model.cm_xorot = 0x00000000;    // Final result XOR'ed with this
 
     cm_ini(&crc_model);
 
     while (words--)
     {
         // The STM32F10x hardware does 32-bit words at a time!!!
 
         word_to_do = *buffer++;
 
         // Do all bytes in the 32-bit word.
 
         for (i = 0; i < sizeof(word_to_do); i++)
         {
             // We calculate a *byte* at a time. If the CRC is MSB first we
             // do the next MS byte and vica-versa.
 
             if (crc_model.cm_refin == FALSE)
             {
                 // MSB first. Do the next MS byte.
 
                 byte_to_do = (uint8_t) ((word_to_do & 0xFF000000) >> 24);
                 word_to_do <<= 8;
             }
             else
             {
                 // LSB first. Do the next LS byte.
 
                 byte_to_do = (uint8_t) (word_to_do & 0x000000FF);
                 word_to_do >>= 8;
             }
 
             cm_nxt(&crc_model, byte_to_do);
         }
     }
 
     // Return the final result.
 
     return (cm_crc(&crc_model));
}
 
// This code is also in RTE. Keep it in sync when editing
_Bool is_ambient_pressure_close_to_surface(SLifeData *lifeData)
{
	if (lifeData->pressure_ambient_bar > 1.16)
		return false;
	else if(lifeData->pressure_ambient_bar < (lifeData->pressure_surface_bar + 0.1f))
		return true;
	else
		return false;
}

void compass_Inertia(float newHeading)
{
	float newTarget = newHeading;

	if(settingsGetPointer()->compassInertia == 0)
	{
		compass_compensated = newHeading;
	}
	else
	{
		if((compass_compensated > 270.0) && (newHeading < 90.0))		/* transition passing 0 clockwise */
		{
			newTarget = newHeading + 360.0;
		}

		if((compass_compensated < 90.0) && (newHeading > 270.0))		/* transition passing 0 counter clockwise */
		{
			newTarget = newHeading - 360.0;
		}

		compass_compensated = compass_compensated + ((newTarget - compass_compensated) / (COMPASS_FRACTION * (settingsGetPointer()->compassInertia)));
		if(compass_compensated < 0.0)
		{
			compass_compensated += 360.0;
		}
		if(compass_compensated >= 360.0)
		{
			compass_compensated -= 360.0;
		}
	}
}

float compass_getCompensated()
{
	return compass_compensated;
}

uint8_t isLoopMode(uint8_t Mode)
{
	uint8_t retVal = 0;
	if((Mode == DIVEMODE_CCR) || (Mode == DIVEMODE_PSCR))
	{
		retVal = 1;
	}
	return retVal;
}
