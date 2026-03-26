/**
  ******************************************************************************
  * @file    hud.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    09-Mar-2026
  * @brief   Support function for HUD configuration
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

#include "hud.h"
#include "settings.h"
#include "data_central.h"
#include "data_exchange.h"
#include "data_exchange_main.h"
#include "gfx_colors.h"
#include "math.h"
#include "tHome.h"

uint8_t hudActive;
static uint8_t hudAddress;
static uint8_t hudVersion;

static uint8_t hudLEDPerFct[NUM_OF_HUD_FCT];									/* array providing the version depending LED per function mapping */
static uint8_t hudLEDNeedPerFct[HUD_FCT_END+1] = {0, 1, 2, 2, 2, 2, 2, 2, 0};	/* array providing information how many LEDs are needed to provide a function */

void hud_Init()
{
	uint8_t index = 0;
	SDiveState * pStateReal = stateRealGetPointerWrite();
	SSettings *pSettings = settingsGetPointer();

	memset(pStateReal->lifeData.HUD_led_sequence,0,EXT_INTERFACE_HUD_LED_MAX);
	hudActive = 0;
	hudAddress = 0xFF;
	hudVersion = 0;

	for(index = EXT_INTERFACE_MUX_OFFSET; index < EXT_INTERFACE_SENSOR_CNT; index++)
	{
		if(pSettings->ext_sensor_map[index] == SENSOR_HUD)
		{
			hudActive = 1;
			hudAddress = index;
			break;
		}
	}
}

uint8_t hud_IsActive()
{
	return hudActive;
}

uint8_t hud_GetAddress(void)
{
	return hudAddress;
}

void hud_GetString(uint8_t id, uint8_t* pText)
{
	switch(id)
	{
		case HUD_FCT_NONE: sprintf((char*)pText,"%c", TXT_Off);
			break;
		case HUD_FCT_WARNING:	sprintf((char*)pText,"%c", TXT_Warning);
			break;
		case HUD_FCT_PPO2SUM: sprintf((char*)pText,"%c%c (1 - 3)", TXT_2BYTE, TXT2BYTE_O2monitor);
			break;
		case HUD_FCT_PPO2_0:
		case HUD_FCT_PPO2_1:
		case HUD_FCT_PPO2_2: sprintf((char*)pText,"%c%c (%d)", TXT_2BYTE, TXT2BYTE_O2monitor, (id - HUD_FCT_PPO2_0));
			break;
		case HUD_FCT_ASCENT_SPEED:	sprintf((char*)pText,"%c%c", TXT_2BYTE, TXT2BYTE_AscentSpeed);
			break;
		case HUD_FCT_DECO:	sprintf((char*)pText,"%c%c", TXT_2BYTE, TXT2BYTE_WarnDecoMissed);
			break;
		default:
			break;
	}
}

uint8_t hud_NextFct(uint8_t curFct, uint8_t fctId)
{
	uint8_t done = 0;
	uint8_t nextFct = curFct;
    SSettings *pSettings = settingsGetPointer();


	while (done == 0)
	{
		nextFct++;
		if(nextFct < HUD_FCT_END)
		{
			if(hudLEDNeedPerFct[nextFct] <= hudLEDPerFct[fctId])
			{
				switch(nextFct)						/* this switch handles conditional function. e.g. monitor2 should be skipped if no sensor2 is connected */
				{
					case HUD_FCT_PPO2_0:
					case HUD_FCT_PPO2_1:
					case HUD_FCT_PPO2_2:			if((pSettings->ext_sensor_map[nextFct - HUD_FCT_PPO2_0] >= SENSOR_ANALOG) && (pSettings->ext_sensor_map[nextFct - HUD_FCT_PPO2_0] < SENSOR_TYPE_O2_END))
													{
														done = 1;
													}
						break;
					default:						done = 1;
						break;
				}
			}
		}
		else
		{
			nextFct = HUD_FCT_NONE;
			break;
		}
	}
	return nextFct;
}

static void hud_UpdateWarning(uint8_t fctId)
{

	if(hudLEDPerFct[fctId] == 1)		/* single LED implementation (e.g. blue LED) */
	{
		if(stateUsed->warnings.numWarnings)
		{
			stateUsedWrite->lifeData.HUD_led_sequence[fctId * 2] = HUD_LED_STATUS_1s1;		/* blink  */
		}
		else
		{
			stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = HUD_LED_PERMANENT;	/* Constant  */
		}
	}
	else
	{
		if(stateUsed->warnings.numWarnings)
		{
			stateUsedWrite->lifeData.HUD_led_sequence[fctId * 2] = HUD_LED_STATUS_1s1;		/* blink red */
		}
		else
		{
			stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = HUD_LED_PERMANENT;	/* Constant green */
		}
	}
}

static void hud_UpdatePPO2Monitor(uint8_t fctId, float ppO2)
{
	stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = 0;
	stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = 0;

	if(fabs(stateUsed->lifeData.actualGas.setPoint_cbar - ppO2) < 0.06)		/*	green constant */
	{
		stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = HUD_LED_PERMANENT;
	}
	else if((stateUsed->lifeData.actualGas.setPoint_cbar - ppO2) < 0.2)	/* to low => blink green */
	{
		stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = HUD_LED_STATUS_1s1;
	}
	else if((stateUsed->lifeData.actualGas.setPoint_cbar - ppO2) < 0.2)	/* to high => blink red */
	{
		stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = HUD_LED_STATUS_1s1;
	}
	else																	/* out of range => red */
	{
		stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = HUD_LED_PERMANENT;
	}
}

static void hud_UpdateAscentSpeed(uint8_t fctId)
{
	uint8_t indicatorColor = 0;

	indicatorColor = drawingColor_from_ascentspeed(stateUsed->lifeData.ascent_rate_meter_per_min);
	stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = 0;
	stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = 0;

	switch(indicatorColor)			/* map color to LED operation */
	{
		case CLUT_NiceGreen:		stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = HUD_LED_PERMANENT; 	/*	green constant */
			break;
		case CLUT_WarningYellow:	stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = HUD_LED_STATUS_025s025; 	/*	fast blink red */
			break;
		case CLUT_WarningRed:		stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = HUD_LED_PERMANENT; 	/*	red constant */
			break;
	}
}

static void hud_UpdateDecoIndicator(uint8_t fctId)
{
    uint16_t 	nextstopLengthSeconds = 0;
    uint8_t 	nextstopDepthMeter = 0;
	const SDecoinfo * pDecoinfo = getDecoInfo();

	stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = 0;
	stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = 0;

	if(pDecoinfo->output_time_to_surface_seconds)
	{
		tHome_findNextStop(pDecoinfo->output_stop_length_seconds, &nextstopDepthMeter, &nextstopLengthSeconds);
		if(nextstopDepthMeter > 0)
		{
			if(fabs(stateUsed->lifeData.depth_meter - nextstopDepthMeter) < 1.5)	    /* close to stop */
			{
				if((stateUsed->lifeData.depth_meter + 0.1) >= nextstopDepthMeter)
				{
					stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = HUD_LED_PERMANENT; 	/*	close below deco stop => green constant */
				}
				else
				{
					stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = HUD_LED_STATUS_025s025; 		/*	close above deco stop => red fast blink */
				}
			}
			else if(((stateUsed->lifeData.depth_meter +0.1) > nextstopDepthMeter))
			{
				stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2) + 1] = HUD_LED_STATUS_1s1;		/*  Ascent to deco stop => green slow blink */
			}
			else
			{
				stateUsedWrite->lifeData.HUD_led_sequence[(fctId * 2)] = HUD_LED_PERMANENT;			/*  Missed deco stop => red constant */
			}
		}
	}
}

void hud_UpdateStatus()
{
	static uint32_t updateTick = 0;

	SDiveState * pStateReal = stateRealGetPointerWrite();
    SSettings *pSettings = settingsGetPointer();

	uint8_t index = 0;

	if(hudVersion == 0)
	{
		if((pStateReal->lifeData.extIf_sensor_data[hudAddress][HUD_INFO_VERSION_OFFSET] > 0) && (pStateReal->lifeData.extIf_sensor_data[hudAddress][HUD_INFO_VERSION_OFFSET] <= 1))
		{
			hudVersion = pStateReal->lifeData.extIf_sensor_data[hudAddress][HUD_INFO_VERSION_OFFSET];
			switch(hudVersion)
			{
				case 1:
				default:		hudLEDPerFct[0] = 2;
								hudLEDPerFct[1] = 2;
								hudLEDPerFct[2] = 2;
								hudLEDPerFct[3] = 1;
					break;
			}
		}
	}
	else
	{
		if(time_elapsed_ms(updateTick, HAL_GetTick()) > 2000)
		{
			if(stateUsed->mode == MODE_DIVE)
			{
				for( index = 0; index < NUM_OF_HUD_FCT; index++)
				{
					switch(pSettings->hudFunction[index])
					{
						case HUD_FCT_WARNING:	hud_UpdateWarning(index);
							break;
						case HUD_FCT_PPO2SUM:	hud_UpdatePPO2Monitor(index, stateUsed->lifeData.ppO2);
							break;
						case HUD_FCT_PPO2_0:
						case HUD_FCT_PPO2_1:
						case HUD_FCT_PPO2_2:	hud_UpdatePPO2Monitor(index, stateUsed->lifeData.ppO2Sensor_bar[pSettings->hudFunction[index] - HUD_FCT_PPO2_0]);
							break;
						case HUD_FCT_ASCENT_SPEED:	hud_UpdateAscentSpeed(index);
							break;
						case HUD_FCT_DECO:		hud_UpdateDecoIndicator(index);
							break;
						case HUD_FCT_NONE:
						default:
							break;
					}
				}
			}
			else
			{
#ifndef ENABLE_HUD_TESTING
				memset(pStateReal->lifeData.HUD_led_sequence,0,EXT_INTERFACE_HUD_LED_MAX);
				pStateReal->lifeData.HUD_led_sequence[6] = 0x01;							/* switch only blue LED on */
#endif
			}
			DataEX_setExtInterface_Cmd(EXT_INTERFACE_HUD_UPDATE, hudAddress);
			updateTick = HAL_GetTick();
		}
	}
}
