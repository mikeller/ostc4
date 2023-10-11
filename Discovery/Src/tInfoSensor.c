///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tInfoCompass.c
/// \brief  there is only compass_DX_f, compass_DY_f, compass_DZ_f output during this mode
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
#include "tInfoSensor.h"
#include "tMenuEdit.h"

#include <string.h>
#include <inttypes.h>

extern void openEdit_O2Sensors(void);
uint8_t OnAction_Sensor	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

/* Private variables ---------------------------------------------------------*/
static uint8_t	activeSensorId = 0;
static uint8_t sensorActive = 0;
/* Exported functions --------------------------------------------------------*/
void openInfo_Sensor(uint8_t sensorId)
{
	SSettings *pSettings = settingsGetPointer();
	activeSensorId = sensorId;
    set_globalState(StISENINFO);
    switch (activeSensorId)
    {
    	case 2: setBackMenu((uint32_t)openEdit_O2Sensors,0,3);
    		break;
    	case 1: setBackMenu((uint32_t)openEdit_O2Sensors,0,2);
    	    		break;
    	default:
    	case 0: setBackMenu((uint32_t)openEdit_O2Sensors,0,1);
    	    		break;
    }

    sensorActive = 1;
    if(pSettings->ppo2sensors_deactivated & (1 << (activeSensorId)))
    {
    	sensorActive = 0;
    }
}


uint8_t OnAction_Sensor(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	if(settingsGetPointer()->ppo2sensors_deactivated & (1 << (activeSensorId)))
	{
		settingsGetPointer()->ppo2sensors_deactivated &= ~(1 << (activeSensorId));
		tMenuEdit_set_on_off(editId, 1);
	}
	else
	{
		settingsGetPointer()->ppo2sensors_deactivated |= (1 << (activeSensorId));
		tMenuEdit_set_on_off(editId, 0);
	}
    return UPDATE_DIVESETTINGS;
}



uint64_t mod64(uint64_t a, uint64_t b)
{
   uint64_t div;
   div=(a/10);
   b=(10*div);
   return (a-b);
}

void uint64ToString(uint64_t value, char* pbuf)
{
	char tmpBuf[32];
	uint8_t index = 31;

	tmpBuf[index--] = 0;	/* zero termination */
	while((index != 0)  && (value != 0))
	{
		tmpBuf[index--] = '0' + (value % 10);// mod64(worker64,10);
		value /= 10;
	}
	strcpy(pbuf,&tmpBuf[index+1]);
}

void tInfo_write_buttonTextline_simple(uint8_t left2ByteCode, char middle2ByteCode, char right2ByteCode)
{
    char localtext[32];

    if(left2ByteCode)
    {
        localtext[0] = TXT_2BYTE;
        localtext[1] = left2ByteCode;
        localtext[2] = 0;
        tInfo_write_content_simple(0, 800, 480-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }

    if(middle2ByteCode)
    {
        localtext[0] = '\001';
        localtext[1] = TXT_2BYTE;
        localtext[2] = middle2ByteCode;
        localtext[3] = 0;
        tInfo_write_content_simple(0, 800, 480-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }

    if(right2ByteCode)
    {
        localtext[0] = '\002';
        localtext[1] = TXT_2BYTE;
        localtext[2] = right2ByteCode;
        localtext[3] = 0;
        tInfo_write_content_simple(0, 800, 480-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }
}

//  ===============================================================================
void refreshInfo_Sensor(GFX_DrawCfgScreen s)
{
	const SDiveState *pStateReal = stateRealGetPointer();
    SSensorDataDiveO2* pDiveO2Data;
    char text[31];
    uint8_t strIndex = 0;
    char *textPointer = text;

    float pressure = 0.0;

    text[0] = '\001';
	text[1] = TXT_Sensor;
	text[2] = ' ';
	text[3] = TXT_Information;
	text[4] = ' ';
	text[5] = '1' + activeSensorId;
	text[6] = 0;
	tInfo_write_content_simple(  30, 770, ME_Y_LINE_BASE, &FontT48, text, CLUT_MenuPageHardware);

    pDiveO2Data = (SSensorDataDiveO2*)&stateRealGetPointer()->lifeData.extIf_sensor_data[activeSensorId];

    strIndex = snprintf(text,32,"ID: ");
    if(pDiveO2Data->sensorId != 0)
    {
    	uint64ToString(pDiveO2Data->sensorId,&text[strIndex]);
    }
    tInfo_write_content_simple(  30, 770, ME_Y_LINE1, &FontT48, text, CLUT_Font020);
    snprintf(text,32,"%c: %02.1f",TXT_Temperature , (float)pDiveO2Data->temperature / 1000.0);
    tInfo_write_content_simple(  30, 770, ME_Y_LINE2, &FontT48, text, CLUT_Font020);

#ifdef ENABLE_EXTERNAL_PRESSURE
    pressure = (float)(stateRealGetPointer()->lifeData.ppO2Sensor_bar[2]);
#else
    pressure = (float)pDiveO2Data->pressure / 1000.0;
#endif
    snprintf(text,32,"Druck: %02.1f (%02.1f)", (float)pDiveO2Data->pressure / 1000.0, pressure *1000.0);

    tInfo_write_content_simple(  30, 770, ME_Y_LINE3, &FontT48, text, CLUT_Font020);
    snprintf(text,32,"Feuchtigkeit: %02.1f", (float)pDiveO2Data->humidity / 1000.0);
    tInfo_write_content_simple(  30, 770, ME_Y_LINE4, &FontT48, text, CLUT_Font020);
    snprintf(text,32,"Status: 0x%lx", pDiveO2Data->status);
    tInfo_write_content_simple(  30, 770, ME_Y_LINE5, &FontT48, text, CLUT_Font020);
#ifdef ENABLE_EXTERNAL_PRESSURE
    snprintf(text,32,"Norm ppO2: %02.3f (%02.1f)", (float)(stateRealGetPointer()->lifeData.ppO2Sensor_bar[0] / (pressure / 1000.0)),(float)(stateRealGetPointer()->lifeData.ppO2Sensor_bar[0]));
    tInfo_write_content_simple(  30, 770, ME_Y_LINE6, &FontT48, text, CLUT_Font020);
#endif

    if(sensorActive)
    {
    	*textPointer++ = '\005';
    }
    else
    {
    	*textPointer++ = '\006';
    }
    *textPointer++ = ' ';
    *textPointer++ = TXT_2BYTE;
    *textPointer++ = TXT2BYTE_Sensor;
    *textPointer++ = ' ';
    *textPointer++ = TXT_2BYTE;
    *textPointer++ = TXT2BYTE_O2IFDigital;
    *textPointer++ = '1' + activeSensorId;

    snprintf(textPointer, 20,": %01.2f, %01.1f mV",  pStateReal->lifeData.ppO2Sensor_bar[activeSensorId], pStateReal->lifeData.sensorVoltage_mV[activeSensorId]);

    tInfo_write_content_simple(  30, 770, ME_Y_LINE6, &FontT48, text, CLUT_Font020);

    tInfo_write_buttonTextline_simple(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,0);
}

void sendActionToInfoSensor(uint8_t sendAction)
{
    switch(sendAction)
    {
    	case ACTION_BUTTON_BACK:
    		exitMenuEdit_to_BackMenu();
    			break;

    	case ACTION_BUTTON_ENTER:    	if(settingsGetPointer()->ppo2sensors_deactivated & (1 << (activeSensorId)))
										{
    										settingsGetPointer()->ppo2sensors_deactivated &= ~(uint8_t)(1 << (activeSensorId));
											sensorActive = 1;
										}
										else
										{
											settingsGetPointer()->ppo2sensors_deactivated |= (uint8_t)(1 << (activeSensorId));
											sensorActive = 0;
										}
    		break;
		case ACTION_BUTTON_NEXT:
		case ACTION_TIMEOUT:
		case ACTION_MODE_CHANGE:
	    case ACTION_IDLE_TICK:
	    case ACTION_IDLE_SECOND:
		default:
	        break;
    }
}

