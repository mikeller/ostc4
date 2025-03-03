/**
  ******************************************************************************
  * @file    check_warning.c
  * @author  heinrichs weikamp gmbh
  * @date    17-Nov-2014
  * @version V0.0.1
  * @since   17-Nov-2014
  * @brief   check and set warnings for warnings
  *
  @verbatim
  ==============================================================================
              ##### How to use #####
  ==============================================================================
  OSTC3 Warnings:
		niedriger Batteriezustand (
		zu hoher oder zu niedriger Sauerstoffpartialdruck (ppO2) 0.2 - 1.6
		zu hoher CNS (Gefahr der Sauerstoffvergiftung) 90%
		zu hohe Gradientenfaktoren 90 - 90
		Missachtung der Dekostopps (der �berschrittene Dekostopp wird rot angezeigt) 0 m
		zu hohe Aufstiegsgeschwindigkeit 30 m/min
		aGF-Warnung: die Berechnung der Dekompression wird �ber alternative GF-Werte durchgef�hrt
		Fallback-Warnung bei ausgefallenem Sensor

	@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>

#include "data_exchange.h"
#include "check_warning.h"
#include "settings.h"
#include "decom.h"
#include "tCCR.h"
#include "tHome.h"


#define DEBOUNCE_FALLBACK_TIME_MS	(5000u)		/* set warning after 5 seconds of pending error condition */
#define GUI_BUZZER_TIMEOUT_MS		(200u)      /* the buzzer should be active while Warning string is shown, but diver may be in a menu... */

#define SETPOINT_DECO_START_RANGE_M 3.0
#define SWITCH_DEPTH_LOW_MINIMUM_M 1.0

/* Private variables with access ----------------------------------------------*/
static uint8_t betterGasId = 0;
static uint8_t betterBailoutGasId = 0;
static uint8_t betterSetpointId = 1;
static int8_t fallback = 0;
static uint16_t debounceFallbackTimeMS = 0;
static uint8_t buzzerRequestActive = 0;

/* Private function prototypes -----------------------------------------------*/
static int8_t check_fallback(SDiveState * pDiveState);
static int8_t check_ppO2(SDiveState * pDiveState);
static int8_t check_O2_sensors(SDiveState * pDiveState);
static int8_t check_CNS(SDiveState * pDiveState);
static int8_t check_Deco(SDiveState * pDiveState);
static int8_t check_AscentRate(SDiveState * pDiveState);
static int8_t check_aGF(SDiveState * pDiveState);
static int8_t check_BetterGas(SDiveState * pDiveState);
static int8_t check_BetterSetpoint(SDiveState * pDiveState);
static int8_t check_Battery(SDiveState * pDiveState);
#ifdef ENABLE_BOTTLE_SENSOR
static int8_t check_pressureSensor(SDiveState * pDiveState);
#endif
#ifdef ENABLE_CO2_SUPPORT
static int8_t check_co2(SDiveState * pDiveState);
#endif
static int8_t check_helper_same_oxygen_and_helium_content(SGasLine * gas1, SGasLine * gas2);
#ifdef HAVE_DEBUG_WARNINGS
static int8_t check_debug(SDiveState * pDiveState);
#endif
static uint8_t buzzerOn = 0;			/* current state of the buzzer */

static void setBuzzer(int8_t warningActive);
/* Exported functions --------------------------------------------------------*/

void requestBuzzerActivation(uint8_t active)
{
	buzzerRequestActive = active;
}

uint8_t getBuzzerActivationState()
{
	return buzzerOn;
}

static void setBuzzer(int8_t warningActive)
{
	static uint32_t guiTimeoutCnt = 0;		/* max delay till buzzer will be activated independend from gui request */
	static uint32_t stateTick = 0;			/* activation tick of current state */
	static uint8_t lastWarningState = 0;	/* the parameter value of the last call*/

	uint32_t tick =  HAL_GetTick();

	if(warningActive)
	{
		if(!lastWarningState)				/* init structures */
		{
			guiTimeoutCnt = tick;
			stateTick = tick;
		}
		if(buzzerOn)
		{
			if(time_elapsed_ms(stateTick, tick) > EXT_INTERFACE_BUZZER_STABLE_TIME_MS)	/* buzzer has to be on for a certain time */
			{
				if((!buzzerRequestActive) || (time_elapsed_ms(stateTick, tick) > EXT_INTERFACE_BUZZER_ON_TIME_MS))
				{
					buzzerOn = 0;
					stateTick = tick;
					guiTimeoutCnt = tick;
				}
			}
		}
		else
		{
			if(time_elapsed_ms(stateTick, tick) > EXT_INTERFACE_BUZZER_STABLE_TIME_MS)	/* buzzer has to be off for a certain time */
			{
				if((buzzerRequestActive) || (time_elapsed_ms(guiTimeoutCnt, tick) > EXT_INTERFACE_BUZZER_ON_TIME_MS + GUI_BUZZER_TIMEOUT_MS))
				{
					buzzerOn = 1;
					stateTick = tick;
				}
			}
		}
	}
	else
	{
		buzzerOn = 0;
	}
	lastWarningState = warningActive;
}

void check_warning(void)
{
  check_warning2(stateUsedWrite);
}


void check_warning2(SDiveState * pDiveState)
{
  pDiveState->warnings.numWarnings = 0;

/* Warnings checked before the SetBuzzer call will activate the buzzer */
	pDiveState->warnings.numWarnings += check_AscentRate(pDiveState);
	pDiveState->warnings.numWarnings += check_Deco(pDiveState);
	pDiveState->warnings.numWarnings += check_ppO2(pDiveState);
	pDiveState->warnings.numWarnings += check_O2_sensors(pDiveState);
	pDiveState->warnings.numWarnings += check_fallback(pDiveState);
#ifdef ENABLE_CO2_SUPPORT
	pDiveState->warnings.numWarnings += check_co2(pDiveState);
#endif

	if(settingsGetPointer()->warningBuzzer)
	{
		setBuzzer(pDiveState->warnings.numWarnings);
	}

/* Warnings checked after this line will not cause activation of the buzzer */
	pDiveState->warnings.numWarnings += check_aGF(pDiveState);
	pDiveState->warnings.numWarnings += check_CNS(pDiveState);
	pDiveState->warnings.numWarnings += check_BetterGas(pDiveState);
	pDiveState->warnings.numWarnings += check_BetterSetpoint(pDiveState);
	pDiveState->warnings.numWarnings += check_Battery(pDiveState);
#ifdef ENABLE_BOTTLE_SENSOR
	pDiveState->warnings.numWarnings += check_pressureSensor(pDiveState);
#endif

#ifdef HAVE_DEBUG_WARNINGS
	pDiveState->warnings.numWarnings += check_debug(pDiveState);
#endif
}


void set_warning_fallback(void)
{
	fallback = 1;
}


void clear_warning_fallback(void)
{
	fallback = 0;
	debounceFallbackTimeMS = 0;
}


uint8_t actualBetterGasId(void)
{
	return betterGasId;
}


uint8_t actualBetterBailoutGasId(void)
{
	return betterBailoutGasId;
}


uint8_t actualBetterSetpointId(void)
{
	return betterSetpointId;
}


uint8_t actualLeftMaxDepth(const SDiveState * pDiveState)
{
	if(pDiveState->lifeData.depth_meter > (pDiveState->lifeData.max_depth_meter - 3.0f))
		return 0;
	else
		return 1;
}


/* Private functions ---------------------------------------------------------*/
static int8_t check_fallback(SDiveState * pDiveState)
{
	if(fallback && ((pDiveState->mode != MODE_DIVE) || (!isLoopMode(pDiveState->diveSettings.diveMode))))
		fallback = 0;
	
	pDiveState->warnings.fallback = fallback;
	return pDiveState->warnings.fallback;
}


static int8_t check_ppO2(SDiveState * pDiveState)
{
	if((pDiveState->mode != MODE_DIVE) || ((isLoopMode(pDiveState->diveSettings.diveMode) && (pDiveState->warnings.fallback))))
	{
		pDiveState->warnings.ppO2Low = 0;
		pDiveState->warnings.ppO2High = 0;
		return 0;
	}

	uint8_t localPPO2, testPPO2high;

	if(pDiveState->lifeData.ppO2 < 0)
		localPPO2 = 0;
	else
	if(pDiveState->lifeData.ppO2 >= 2.5f)
		localPPO2 = 255;
	else
	localPPO2 = (uint8_t)(pDiveState->lifeData.ppO2 * 100);

	if((localPPO2 + 1) <= settingsGetPointer()->ppO2_min)
			pDiveState->warnings.ppO2Low = 1;
	else
			pDiveState->warnings.ppO2Low = 0;
	
	if(actualLeftMaxDepth(pDiveState))
		testPPO2high = settingsGetPointer()->ppO2_max_deco;
	else
		testPPO2high = settingsGetPointer()->ppO2_max_std;

	if(localPPO2 >= (testPPO2high + 1))
			pDiveState->warnings.ppO2High = 1;
	else
			pDiveState->warnings.ppO2High = 0;

	return pDiveState->warnings.ppO2Low + pDiveState->warnings.ppO2High;
}


static int8_t check_O2_sensors(SDiveState * pDiveState)
{
	pDiveState->warnings.sensorLinkLost = 0;
	pDiveState->warnings.sensorOutOfBounds[0] = 0;
	pDiveState->warnings.sensorOutOfBounds[1] = 0;
	pDiveState->warnings.sensorOutOfBounds[2] = 0;

	if(isLoopMode(pDiveState->diveSettings.diveMode) && (pDiveState->diveSettings.CCR_Mode == CCRMODE_Sensors))
	{
		if(settingsGetPointer()->ppo2sensors_source == O2_SENSOR_SOURCE_OPTIC)
		{
			{
				if(!get_HUD_battery_voltage_V())
					pDiveState->warnings.sensorLinkLost = 1;
			}
		}
		test_O2_sensor_values_outOfBounds(&pDiveState->warnings.sensorOutOfBounds[0], &pDiveState->warnings.sensorOutOfBounds[1], &pDiveState->warnings.sensorOutOfBounds[2]);
	}
	return 		pDiveState->warnings.sensorLinkLost
					+ pDiveState->warnings.sensorOutOfBounds[0]
					+ pDiveState->warnings.sensorOutOfBounds[1]
					+ pDiveState->warnings.sensorOutOfBounds[2];
}


static uint8_t getBetterGasId(bool getDiluent, uint8_t startingGasId, SDiveState *diveState)
{
    SDiveSettings diveSettings = diveState->diveSettings;
    SGasLine localGas;
    uint8_t betterGasIdLocal = startingGasId;
    uint8_t bestGasDepth = 255;
    uint8_t i;

    uint8_t gasIdOffset;
    if (getDiluent) {
        gasIdOffset = NUM_OFFSET_DILUENT;
    } else {
        gasIdOffset = 0;
    }

	/* life data is float, gas data is uint8 */
    if (actualLeftMaxDepth(diveState)) { /* deco gases */
        for (i=1+gasIdOffset; i<= 5+gasIdOffset; i++) {
        	memcpy(&localGas,&diveSettings.gas[i],sizeof(SGasLine));
        	if((localGas.note.ub.first) && (diveSettings.diveMode == DIVEMODE_PSCR))	/* handle first gas as if it would be a deco gas set to MOD */
        	{
        		localGas.note.ub.active = 1;
        		localGas.note.ub.deco = 1;
        		localGas.depth_meter = calc_MOD(i);
        	}
            if ((localGas.note.ub.active)
                && (localGas.note.ub.deco)
                && (localGas.depth_meter)
                && (localGas.depth_meter >= (diveState->lifeData.depth_meter - 0.9f ))
                && (localGas.depth_meter <= bestGasDepth)) {
                betterGasIdLocal = i;
                bestGasDepth = diveSettings.gas[i].depth_meter;
            }
        }
    } else { /* travel gases */
        bestGasDepth = 0;
        //check for travalgas
        for (i = 1 + gasIdOffset; i <= 5 + gasIdOffset; i++) {
            if ((diveSettings.gas[i].note.ub.active)
                && (diveSettings.gas[i].note.ub.travel)
                && (diveSettings.gas[i].depth_meter_travel)
                && (diveSettings.gas[i].depth_meter_travel <= (diveState->lifeData.depth_meter + 0.01f ))
                && (diveSettings.gas[i].depth_meter_travel >= bestGasDepth)) {
                betterGasIdLocal = i;
                bestGasDepth = diveSettings.gas[i].depth_meter;
            }
        }
    }
    if((!getDiluent) && (betterGasIdLocal > NUM_OFFSET_DILUENT))	/* an OC gas was requested but Id is pointing to a diluent => return first OC */
    {
    	for (i = 1 ; i <= NUM_OFFSET_DILUENT; i++)
    	{
    		if(diveSettings.gas[i].note.ub.first)
    		{
    			betterGasIdLocal = i;
    			break;
    		}
    	}
    }


    return betterGasIdLocal;
}


static int8_t check_BetterGas(SDiveState *diveState)
{
    diveState->warnings.betterGas = 0;

    if (stateUsed->mode != MODE_DIVE) {
        betterGasId = 0;

        return 0;
    }

    SDiveSettings diveSettings = diveState->diveSettings;
    SLifeData lifeData = diveState->lifeData;

    if (isLoopMode(diveSettings.diveMode)) {
        betterGasId = getBetterGasId(true, lifeData.actualGas.GasIdInSettings, diveState);
        betterBailoutGasId = getBetterGasId(false, lifeData.lastDiluent_GasIdInSettings, diveState);
    } else {
        betterGasId = getBetterGasId(false, lifeData.actualGas.GasIdInSettings, diveState);
    }

    if (betterGasId != lifeData.actualGas.GasIdInSettings && !check_helper_same_oxygen_and_helium_content(&diveSettings.gas[betterGasId], &diveSettings.gas[lifeData.actualGas.GasIdInSettings])) {
        diveState->warnings.betterGas = 1;
    }

    return diveState->warnings.betterGas;
}


uint8_t getSetpointLowId(void)
{
    SSettings *settings = settingsGetPointer();

    if (settings->autoSetpoint) {
        return SETPOINT_INDEX_AUTO_LOW;
    }

    uint8_t setpointLowId = 0;
    uint8_t setpointLowDepthM = UINT8_MAX;
    for (unsigned i = 1; i <= NUM_GASES; i++) {
        if (stateUsed->diveSettings.setpoint[i].depth_meter && stateUsed->diveSettings.setpoint[i].depth_meter < setpointLowDepthM) {
            setpointLowId = i;
            setpointLowDepthM = stateUsed->diveSettings.setpoint[i].depth_meter;
        }
    }

    return setpointLowId;
}


uint8_t getSetpointHighId(void)
{
    SSettings *settings = settingsGetPointer();

    if (settings->autoSetpoint) {
        return SETPOINT_INDEX_AUTO_HIGH;
    }

    uint8_t setpointHighId = 0;
    uint8_t setpointHighDepthM = 0;
    for (unsigned i = 1; i <= NUM_GASES; i++) {
        if (stateUsed->diveSettings.setpoint[i].depth_meter && stateUsed->diveSettings.setpoint[i].depth_meter >= setpointHighDepthM) {
            setpointHighId = i;
            setpointHighDepthM = stateUsed->diveSettings.setpoint[i].depth_meter;
        }
    }

    return setpointHighId;
}


uint8_t getSetpointDecoId(void)
{
    SSettings *settings = settingsGetPointer();

    if (settings->autoSetpoint && stateUsed->diveSettings.setpoint[SETPOINT_INDEX_AUTO_DECO].note.ub.active) {
        return SETPOINT_INDEX_AUTO_DECO;
    }

    return 0;
}


/* check for better travel!!! setpoint hw 151210
 */
static int8_t check_BetterSetpoint(SDiveState *diveState)
{
    diveState->warnings.betterSetpoint = 0;

    if (stateUsed->mode != MODE_DIVE) {
        betterSetpointId = 1;

        return 0;
    }

    SSettings *settings = settingsGetPointer();

    float currentDepthM = diveState->lifeData.depth_meter;
    float lastChangeDepthM = diveState->lifeData.lastSetpointChangeDepthM;
    if (settings->dive_mode == DIVEMODE_CCR && lastChangeDepthM != currentDepthM) {
        bool descending = currentDepthM > lastChangeDepthM;
        uint8_t betterSetpointIdLocal = 0;

        if (settings->autoSetpoint) {
            bool decoSetpointEnabled = diveState->diveSettings.setpoint[SETPOINT_INDEX_AUTO_DECO].note.ub.active;
            const SDecoinfo *decoInfo = getDecoInfo();
            uint8_t nextDecoStopDepthM;
            uint16_t nextDecoStopTimeRemainingS;
            tHome_findNextStop(decoInfo->output_stop_length_seconds, &nextDecoStopDepthM, &nextDecoStopTimeRemainingS);

            if (decoSetpointEnabled && nextDecoStopDepthM && currentDepthM < nextDecoStopDepthM + SETPOINT_DECO_START_RANGE_M && !diveState->lifeData.setpointDecoActivated) {
                betterSetpointIdLocal = SETPOINT_INDEX_AUTO_DECO;
                diveState->lifeData.setpointDecoActivated = true;
            }

            if (descending) {
                uint8_t switchDepthHighM = diveState->diveSettings.setpoint[SETPOINT_INDEX_AUTO_HIGH].depth_meter;

                if (lastChangeDepthM < switchDepthHighM && switchDepthHighM < currentDepthM && !diveState->lifeData.setpointDecoActivated) {
                    betterSetpointIdLocal = SETPOINT_INDEX_AUTO_HIGH;
                }
            } else {
                uint8_t switchDepthLowM = diveState->diveSettings.setpoint[SETPOINT_INDEX_AUTO_LOW].depth_meter;

                if (lastChangeDepthM > SWITCH_DEPTH_LOW_MINIMUM_M && SWITCH_DEPTH_LOW_MINIMUM_M > currentDepthM) {
                    // Avoid draining the oxygen supply by surfacing with a setpoint >= 1.0
                    betterSetpointIdLocal = SETPOINT_INDEX_AUTO_LOW;
                    diveState->lifeData.setpointLowDelayed = false;
                } else if (lastChangeDepthM > switchDepthLowM && switchDepthLowM > currentDepthM) {
                    if (nextDecoStopDepthM && settings->delaySetpointLow) {
                        diveState->lifeData.setpointLowDelayed = true;
                    } else {
                        betterSetpointIdLocal = SETPOINT_INDEX_AUTO_LOW;
                    }
                }
            }

            if (!nextDecoStopDepthM) {
                // Update the state when the decompression obligation ends
                diveState->lifeData.setpointDecoActivated = false;

                if (diveState->lifeData.setpointLowDelayed) {
                    betterSetpointIdLocal = SETPOINT_INDEX_AUTO_LOW;
                    diveState->lifeData.setpointLowDelayed = false;
                }
            }
        } else {
            uint8_t setpointLowId = getSetpointLowId();
            uint8_t setpointHighId = getSetpointHighId();
            uint8_t betterSetpointSwitchDepthM = descending ? 0 : UINT8_MAX;

            for (unsigned i = 1; i <= NUM_GASES; i++) {
                uint8_t switchDepthM = diveState->diveSettings.setpoint[i].depth_meter;
                if (!switchDepthM || (descending && i == setpointLowId) || (!descending && i == setpointHighId)) {
                    continue;
                }

                if ((descending && lastChangeDepthM < switchDepthM && switchDepthM < currentDepthM && switchDepthM > betterSetpointSwitchDepthM)
                    || (!descending && lastChangeDepthM > switchDepthM && switchDepthM > currentDepthM && switchDepthM <= betterSetpointSwitchDepthM)) {
                    betterSetpointIdLocal = i;
                    betterSetpointSwitchDepthM = switchDepthM;
                }
            }
        }

        if (betterSetpointIdLocal) {
            betterSetpointId = betterSetpointIdLocal;
            if (diveState->diveSettings.diveMode == DIVEMODE_CCR && diveState->diveSettings.setpoint[betterSetpointIdLocal].setpoint_cbar != diveState->lifeData.actualGas.setPoint_cbar) {
                diveState->warnings.betterSetpoint = 1;
            }
        }
    }

    return diveState->warnings.betterSetpoint;
}


/* hw 151030
 */
static int8_t check_helper_same_oxygen_and_helium_content(SGasLine * gas1, SGasLine * gas2)
{
	if(gas1->helium_percentage != gas2->helium_percentage)
		return 0;
	else
	if(gas1->oxygen_percentage != gas2->oxygen_percentage)
		return 0;
	else
		return 1;
}


static int8_t check_CNS(SDiveState * pDiveState)
{
	if(stateUsed->mode != MODE_DIVE)
	{
		pDiveState->warnings.cnsHigh = 0;
		return 0;
	}
	
	if(pDiveState->lifeData.cns >= (float)(settingsGetPointer()->CNS_max))
			pDiveState->warnings.cnsHigh = 1;
	else
			pDiveState->warnings.cnsHigh = 0;
	return pDiveState->warnings.cnsHigh;
}


static int8_t check_Battery(SDiveState * pDiveState)
{
	if((pDiveState->lifeData.battery_charge > 0) && (pDiveState->lifeData.battery_charge < 10))
		pDiveState->warnings.lowBattery = 1;
	else
		pDiveState->warnings.lowBattery = 0;
	
  return pDiveState->warnings.lowBattery;
}


static int8_t check_Deco(SDiveState * pDiveState)
{
	if(stateUsed->mode != MODE_DIVE)
	{
		pDiveState->warnings.decoMissed = 0;
		return 0;
	}

	uint8_t depthNext = decom_get_actual_deco_stop(pDiveState);
	
	if(!depthNext)
      pDiveState->warnings.decoMissed = 0;
	else
  if(pDiveState->lifeData.depth_meter + 0.1f < (float)depthNext)
      pDiveState->warnings.decoMissed = 1;
  else
      pDiveState->warnings.decoMissed = 0;
	
  return pDiveState->warnings.decoMissed;
}


static int8_t check_AscentRate(SDiveState * pDiveState)
{
	if(stateUsed->mode != MODE_DIVE)
	{
		pDiveState->warnings.ascentRateHigh = 0;
		return 0;
	}

	float warnAscentRateFloat;

	warnAscentRateFloat = (float)(settingsGetPointer()->ascent_MeterPerMinute_max);

	if(pDiveState->lifeData.ascent_rate_meter_per_min >= warnAscentRateFloat)
			pDiveState->warnings.ascentRateHigh = 1;
	else
			pDiveState->warnings.ascentRateHigh = 0;
	return pDiveState->warnings.ascentRateHigh;
}


static int8_t check_aGF(SDiveState * pDiveState)
{
	if(stateUsed->mode != MODE_DIVE)
	{
		pDiveState->warnings.aGf = 0;
		return 0;
	}

  pDiveState->warnings.aGf = 0;
  if(pDiveState->diveSettings.deco_type.ub.standard == GF_MODE)
  {
    if((pDiveState->diveSettings.gf_high != settingsGetPointer()->GF_high) || (pDiveState->diveSettings.gf_low != settingsGetPointer()->GF_low))
      pDiveState->warnings.aGf = 1;
  }
  return pDiveState->warnings.aGf;
}

#ifdef ENABLE_BOTTLE_SENSOR
static int8_t check_pressureSensor(SDiveState * pDiveState)
{
	int8_t ret = 0;
	if(pDiveState->lifeData.bottle_bar_age_MilliSeconds[pDiveState->lifeData.actualGas.GasIdInSettings] < 50)	/* we received a new value */
	{
		pDiveState->warnings.newPressure = stateUsed->lifeData.bottle_bar[stateUsed->lifeData.actualGas.GasIdInSettings];
		ret = 1;
	}
	else
	{
		pDiveState->warnings.newPressure = 0;
	}
	return ret;
}
#endif

#ifdef ENABLE_CO2_SUPPORT
static int8_t check_co2(SDiveState * pDiveState)
{
	if((pDiveState->mode != MODE_DIVE) || (settingsGetPointer()->co2_sensor_active == 0))
	{
		pDiveState->warnings.co2High = 0;
	}
	else
	{
		if(pDiveState->lifeData.CO2_data.CO2_ppm > CO2_ALARM_LEVEL_PPM)
		{
			pDiveState->warnings.co2High = 1;
		}
		else
		{
			pDiveState->warnings.co2High = 0;
		}
	}
	return pDiveState->warnings.co2High;
}
#endif

#ifdef HAVE_DEBUG_WARNINGS
static int8_t check_debug(SDiveState * pDiveState)
{
	uint8_t index = 0;

	pDiveState->warnings.debug = 0;

	if((settingsGetPointer()->ppo2sensors_source == O2_SENSOR_SOURCE_DIGITAL) || (settingsGetPointer()->ppo2sensors_source == O2_SENSOR_SOURCE_ANADIG))
	{
	    for(index=0; index<3; index++)
	    {
        	if(((pDiveState->lifeData.extIf_sensor_map[index] == SENSOR_DIGO2M) && (((SSensorDataDiveO2*)(stateUsed->lifeData.extIf_sensor_data[index]))->status & DVO2_FATAL_ERROR)))
        	{
        		pDiveState->warnings.debug = 1;
        	}
	    }
	}
	return pDiveState->warnings.debug;
}
#endif

uint8_t debounce_warning_fallback(uint16_t debounceStepms)
{
	uint8_t retVal = 0;

	debounceFallbackTimeMS += debounceStepms;
	if(debounceFallbackTimeMS > DEBOUNCE_FALLBACK_TIME_MS)
	{
		debounceFallbackTimeMS = DEBOUNCE_FALLBACK_TIME_MS;
		retVal = 1;
	}
	return retVal;
}
void reset_debounce_warning_fallback()
{
	debounceFallbackTimeMS = 0;
}
/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/

