///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tInfoPredive.c
/// \brief  Show information which might be of interest during predive checks
/// \author heinrichs weikamp gmbh
/// \date   23-Feb-2015
///
/// \details
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

/* Includes ------------------------------------------------------------------*/

#include "gfx_engine.h"
#include "gfx_fonts.h"
#include "tHome.h"
#include "tInfo.h"
#include "tInfoPreDive.h"
#include "tMenuEdit.h"
#include "data_exchange_main.h"

#include <string.h>
#include <inttypes.h>

#define MEASURE_INTERVALL		(10u)	/* refresh function should be called every 100ms => one second interval */
#define HISTORY_BUF_SIZE 		(240u)	/* store 240 entries a one second */
#define INVALID_PRESSURE_VALUE	(0xFFFF)
#define DELTA_SHIFT				(50)	/* the graph printers do not support negative values => shift data into positiv area (assumes pressure range +- 300mBar) */

/* Private variables ---------------------------------------------------------*/
static uint16_t surfacePressureStart = INVALID_PRESSURE_VALUE;
static uint16_t surfaceTemperatureStart = 0;
static uint16_t pressureHistory[HISTORY_BUF_SIZE];
static uint8_t pressureHistoryIndex = 0;
static uint16_t temperatureHistory[HISTORY_BUF_SIZE];
static uint8_t temperatureHistoryIndex = 0;
static uint8_t measureCnt = MEASURE_INTERVALL;
static uint8_t referenceSensor = 0xff;

/* Exported functions --------------------------------------------------------*/



static void storePressureValue(int16_t deltapressure_mBar)
{
	uint16_t newValue = DELTA_SHIFT - deltapressure_mBar;	/* invert value because graph is drawing larger values to bottom direction */
	if(pressureHistoryIndex + 2 < HISTORY_BUF_SIZE)
	{
		pressureHistoryIndex++;
		pressureHistory[pressureHistoryIndex] = newValue;
		pressureHistoryIndex++;
		pressureHistory[pressureHistoryIndex] = newValue;
	}
	else
	{
		memcpy (&pressureHistory[0],&pressureHistory[2],sizeof(pressureHistory) - 2);
		pressureHistory[pressureHistoryIndex] = newValue;
	}
}

static void storeTemperatureValue(uint16_t temperature)
{
	uint16_t newValue =  temperature;		/* todo: consider negativ temperature */
	if(temperatureHistoryIndex + 2 < HISTORY_BUF_SIZE)
	{
		temperatureHistoryIndex++;
		temperatureHistory[temperatureHistoryIndex] = newValue;
		temperatureHistoryIndex++;
		temperatureHistory[temperatureHistoryIndex] = newValue;
	}
	else
	{
		memcpy (&temperatureHistory[0],&temperatureHistory[2],sizeof(temperatureHistory) - 2);
		temperatureHistory[temperatureHistoryIndex] = newValue;
	}
}

void openInfo_PreDive()
{
	const SDiveState *pStateReal = stateRealGetPointer();
	uint8_t index = 0;
	SSensorDataDiveO2* pDiveO2Data = NULL;
	SSettings *pSettings = settingsGetPointer();
    set_globalState(StIPREDIVE);

    surfacePressureStart = 0;
    pressureHistoryIndex = 0;
    referenceSensor = 0xff;

	for(index = 0; index < EXT_INTERFACE_MUX_OFFSET; index++)
	{
		if((pSettings->ext_sensor_map[index] == SENSOR_DIGO2M) &&  pStateReal->lifeData.ppO2Sensor_bar[index] != 0)
		{
			referenceSensor = index;
			pDiveO2Data = (SSensorDataDiveO2*)stateRealGetPointer()->lifeData.extIf_sensor_data[index];
			if(pDiveO2Data->pressure != 0)
			{
				surfacePressureStart = pDiveO2Data->pressure / 1000;
				surfaceTemperatureStart = pDiveO2Data->temperature;
			}
			break;
		}
	}
    for(index = 0; index < HISTORY_BUF_SIZE; index++)
    {
    	pressureHistory[index] = DELTA_SHIFT;
    	temperatureHistory[index] = 0;
    }
}

void refreshInfo_PreDive(GFX_DrawCfgScreen s)
{
	const SDiveState *pStateReal = stateRealGetPointer();
	static int16_t deltaPressure = 0;
	static uint16_t temperature = 0;
	SSettings *pSettings = settingsGetPointer();
	uint8_t index = 0;
    char text[31];
    SSensorDataDiveO2* pDiveO2Data = NULL;
    point_t start, stop;

    SWindowGimpStyle wintempppO2;
    SWindowGimpStyle wintemptemp;

    if(--measureCnt == 0)
    {
    	measureCnt = MEASURE_INTERVALL;
    	pDiveO2Data = (SSensorDataDiveO2*)stateRealGetPointer()->lifeData.extIf_sensor_data[referenceSensor];
    	if(pDiveO2Data->pressure != 0)
    	{
    		if(surfacePressureStart == 0)
    		{
    			surfacePressureStart = pDiveO2Data->pressure / 1000;
    			surfaceTemperatureStart = pDiveO2Data->temperature;
    		}
    		deltaPressure = (pDiveO2Data->pressure / 1000) - surfacePressureStart;
    		storePressureValue(deltaPressure);
    		temperature = pDiveO2Data->temperature;
    		storeTemperatureValue(temperature);
    	}
    }

    text[0] = '\001';
	text[1] = TXT_PreDive;
	text[2] = 0;
	tInfo_write_content_simple(  30, 770, ME_Y_LINE_BASE, &FontT48, text, CLUT_MenuPageHardware);

	for(index = 0; index < EXT_INTERFACE_MUX_OFFSET; index++)
	{
		if(pSettings->ext_sensor_map[index] == SENSOR_DIGO2M)
		{
			snprintf(text,32,"%c%c%d: %01.2f", TXT_2BYTE, TXT2BYTE_Sensor, index, pStateReal->lifeData.ppO2Sensor_bar[index]);
			tInfo_write_content_simple(  30, 200, ME_Y_LINE1 + (index * ME_Y_LINE_STEP), &FontT48, text, CLUT_Font020);
		}
		else if(pSettings->ext_sensor_map[index] == SENSOR_CO2M)
		{
			snprintf(text,32,"CO2: %4ld", pStateReal->lifeData.CO2_data.CO2_ppm);
			tInfo_write_content_simple(  30, 200, ME_Y_LINE5, &FontT48, text, CLUT_Font020);
		}
	}

    wintempppO2.left = 350;
    wintempppO2.right = 590;

    wintemptemp.left = 350;
    wintemptemp.right = 590;

    if(!pSettings->FlipDisplay)
    {
    	wintempppO2.top = ME_Y_LINE3;
    	wintempppO2.bottom = wintempppO2.top + DELTA_SHIFT * 2;
    	wintemptemp.top = ME_Y_LINE5;
    	wintemptemp.bottom = wintemptemp.top + DELTA_SHIFT * 2;
    }
    else
    {
    	wintempppO2.top = 470;		/* TODO: consider flip display */
    	wintempppO2.bottom = wintempppO2.top + 100;
    }
    GFX_graph_print(&s, &wintempppO2, 1,1,0, DELTA_SHIFT * 2, pressureHistory, HISTORY_BUF_SIZE, CLUT_Font030, NULL);

    GFX_graph_print(&s, &wintemptemp, 1,1, surfaceTemperatureStart - 2000, surfaceTemperatureStart + 10000, temperatureHistory, HISTORY_BUF_SIZE, CLUT_Font030, NULL);

    start.x =  wintempppO2.left - 5;
    start.y =  480 - wintemptemp.bottom - 5;
    stop.x = wintempppO2.right- start.x + 5;
    stop.y = DELTA_SHIFT * 2 + 10;
    GFX_draw_box(&s, start, stop,1, CLUT_Font020);

    start.y =  480 - wintempppO2.bottom - 5;
    GFX_draw_box(&s, start, stop,1, CLUT_Font020);

/* Graph labeling */
    snprintf(text,32,"%c%c", TXT_2BYTE, TXT2BYTE_CounterLung);
   	tInfo_write_content_simple(  350, 780, ME_Y_LINE2, &FontT48, text, CLUT_Font020);

   	snprintf(text,32,"\002\016\016%c%c", TXT_2BYTE, TXT2BYTE_Pressure);
   	tInfo_write_content_simple(  600, 780, ME_Y_LINE3, &FontT48, text, CLUT_Font020);
    snprintf(text,32,"\002%d",deltaPressure);
	tInfo_write_content_simple(  600, 780, ME_Y_LINE4, &FontT48, text, CLUT_Font020);
	snprintf(text,32,"\002\016\016%c",TXT_Temperature);
	tInfo_write_content_simple(  600, 780, ME_Y_LINE5, &FontT48, text, CLUT_Font020);
    snprintf(text,32,"\002%2.2f",(temperature / 1000.0));
	tInfo_write_content_simple(  600, 780, ME_Y_LINE6, &FontT48, text, CLUT_Font020);
}

void sendActionToInfoPreDive(uint8_t sendAction)
{
    switch(sendAction)
    {
    	case ACTION_BUTTON_BACK:
    	//	exitInfo();
    		exitMenuEdit_to_Menu_with_Menu_Update();
    			break;

    	case ACTION_BUTTON_ENTER:
    		break;
		case ACTION_BUTTON_NEXT:
			break;
		case ACTION_TIMEOUT:
		case ACTION_MODE_CHANGE:
	    case ACTION_IDLE_TICK:
	    case ACTION_IDLE_SECOND:
		default:
	        break;
    }
}

