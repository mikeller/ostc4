///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuCvOptionText.c
/// \brief  File providing functions for generation of text lines
/// \author heinrichs weikamp gmbh
/// \date   03-Feb-2026
///
/// \details
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2025 Heinrichs Weikamp gmbh
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
#include "tMenuCvOptionText.h"
#include "tMenuEditCvOption.h"
#include "tMenu.h"
#include "tHome.h"  // for enum CUSTOMVIEWS and init_t7_compass()
#include "t3.h"
#include "t7.h"

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/


static refreshFunc_t refreshFctPointerTable[MAXLINES];		/* function pointer for refresh */
static uint8_t activeLines = 0;									/* number of active lines */

refreshFunc_t* tMCvOptText_GetTable()
{
	return refreshFctPointerTable;
}
uint8_t tMCvOptText_GetTableItemCnt()
{
	return activeLines;
}

uint8_t tMCvOptText_refreshCompass(char* pText)
{
	uint8_t textPointer = 0;
    pText[textPointer++] = TXT_2BYTE;
    pText[textPointer++] = TXT2BYTE_Compass;
    pText[textPointer++] = '\t';

    if(settingsGetPointer()->compassBearing != 0)
    {
        textPointer += snprintf(&pText[textPointer], 20, "(%03u`)", settingsGetPointer()->compassBearing % 360);
    }
    pText[textPointer] = 0;
    return strlen(pText);
}

uint8_t tMCvOptText_refreshTimer(char* pText)
{
	SSettings *settings = settingsGetPointer();
	snprintf(pText, 21, "%c%c\t%u:%02u \016\016[m:ss]\017", TXT_2BYTE, TXT2BYTE_Timer, settings->timerDurationS / 60, settings->timerDurationS % 60);
	return strlen(pText);
}

uint8_t tMCvOptText_refreshO2(char* pText)
{
	char sensorStatusColor[3];
	uint8_t textPointer = 0;
	SSettings *pSettings = settingsGetPointer();


	pText[textPointer++] = TXT_2BYTE;
	pText[textPointer++] = TXT2BYTE_Sensor;

	textPointer += snprintf(&pText[textPointer],20,"O2 %c%c",TXT_2BYTE,TXT2BYTE_Sensor);

	if((stateUsed->lifeData.ppO2Sensor_bar[0] != 0) || (stateUsed->lifeData.ppO2Sensor_bar[1] != 0) || (stateUsed->lifeData.ppO2Sensor_bar[2] != 0))
	{
		pText[textPointer++] = '\t';
		sensorStatusColor[0] = '\020';
		sensorStatusColor[1] = '\020';
		sensorStatusColor[2] = '\020';

		/* Warning */
		if(stateUsed->warnings.sensorOutOfBounds) sensorStatusColor[0] = '\024';
		if(stateUsed->warnings.sensorOutOfBounds) sensorStatusColor[1] = '\024';
		if(stateUsed->warnings.sensorOutOfBounds) sensorStatusColor[2] = '\024';
		/* Grey out */
		if(stateUsed->diveSettings.ppo2sensors_deactivated & 1) sensorStatusColor[0] = '\031';
		if(stateUsed->diveSettings.ppo2sensors_deactivated & 2) sensorStatusColor[1] = '\031';
		if(stateUsed->diveSettings.ppo2sensors_deactivated & 4) sensorStatusColor[2] = '\031';

		if((pSettings->ext_sensor_map[0] == SENSOR_ANALOG) || (pSettings->ext_sensor_map[0] == SENSOR_DIGO2M) || (pSettings->ext_sensor_map[0] == SENSOR_SENTINELM))
		{
			textPointer += snprintf(&pText[textPointer],20,"%c%01.1f \020\033",sensorStatusColor[0], stateUsed->lifeData.ppO2Sensor_bar[0]);
		}
		if((pSettings->ext_sensor_map[1] == SENSOR_ANALOG) || (pSettings->ext_sensor_map[1] == SENSOR_DIGO2M) || (pSettings->ext_sensor_map[1] == SENSOR_SENTINELM))
		{
			textPointer += snprintf(&pText[textPointer],20,"%c%01.1f \020\033",sensorStatusColor[1], stateUsed->lifeData.ppO2Sensor_bar[1]);
		}
		if((pSettings->ext_sensor_map[2] == SENSOR_ANALOG) || (pSettings->ext_sensor_map[2] == SENSOR_DIGO2M) || (pSettings->ext_sensor_map[2] == SENSOR_SENTINELM))
		{
			textPointer += snprintf(&pText[textPointer],20,"%c%01.1f",sensorStatusColor[2], stateUsed->lifeData.ppO2Sensor_bar[2]);
		}
	}
	pText[textPointer++] = '\020';
	pText[textPointer] = 0;
	return strlen(pText);
}

uint8_t tMCvOptText_refreshCO2(char* pText)
{
	char sensorStatusColor;
	uint8_t textPointer = 0;
	SSettings *pSettings = settingsGetPointer();


	pText[textPointer++] = TXT_2BYTE;
	pText[textPointer++] = TXT2BYTE_Sensor;

	textPointer += snprintf(&pText[textPointer],20,"CO2 %c%c",TXT_2BYTE,TXT2BYTE_Sensor);


	pText[textPointer++] = '\t';
	sensorStatusColor = '\020';

	/* Warning */
	if(stateUsed->warnings.co2High) sensorStatusColor = '\024';
	/* Grey out */
	if(pSettings->co2_sensor_active == 0) sensorStatusColor = '\031';

	textPointer += snprintf(&pText[textPointer],20,"%c%ld \020\033",sensorStatusColor, stateUsed->lifeData.CO2_data.CO2_ppm);

	pText[textPointer++] = '\020';
	pText[textPointer] = 0;
	return strlen(pText);
}


uint8_t tMCvOptText_BuildDynamicContentList()
{
	uint8_t cvOptIndex = 0;
	uint8_t CvOptAvailable = 0;
	uint8_t index = 0;
	uint8_t SensorActive[SENSOR_END];
	SSettings *settings = settingsGetPointer();

	memset(SensorActive, 0, sizeof(SensorActive));
	activeLines = 0;

	for (index = 0; index < EXT_INTERFACE_SENSOR_CNT; index++)
	{
		switch(settings->ext_sensor_map[index])
		{
			case SENSOR_ANALOG:	SensorActive[SENSOR_ANALOG] = 1;
				break;
			case SENSOR_SENTINEL:
			case SENSOR_DIGO2M:	SensorActive[SENSOR_DIGO2] = 1;
				break;
			case SENSOR_CO2:	SensorActive[SENSOR_CO2] = 1;
				break;
#if defined ENABLE_GNSS_INTERNAL || defined ENABLE_GNSS_EXTERN
			case SENSOR_GNSS:	SensorActive[SENSOR_GNSS] = 1;
				break;
#endif
			default:
				break;
		}
	}

	do
	{
		CvOptAvailable = 0;
		switch(cvOptIndex)
		{
			case CVOPT_Compass:	if((!t3_customview_disabled(CVIEW_T3_Compass)) || (!t3_customview_disabled(CVIEW_T3_Navigation)) || (!t7_customview_disabled(CVIEW_Compass)))
								{
									refreshFctPointerTable[activeLines] = tMCvOptText_refreshCompass;
									CvOptAvailable = 1;
								}
				break;
			case CVOPT_Timer:  if(!t7_customview_disabled(CVIEW_Timer))
								{
									refreshFctPointerTable[activeLines] = tMCvOptText_refreshTimer;
									CvOptAvailable = 1;
								}
				break;
			case CVOPT_O2_Sensor: if((SensorActive[SENSOR_ANALOG]) || (SensorActive[SENSOR_DIGO2]))
								{
									refreshFctPointerTable[activeLines] = tMCvOptText_refreshO2;
									CvOptAvailable = 1;
								}
				break;
			case CVOPT_CO2_Sensor: 	if(SensorActive[SENSOR_CO2])
									{
										refreshFctPointerTable[activeLines] = tMCvOptText_refreshCO2;
										CvOptAvailable = 1;
									}
						break;
			default:
				break;
		}
		if(CvOptAvailable)
		{
			tMCvOption_SetOpenFnct(cvOptIndex,activeLines);
			activeLines++;
		}
		cvOptIndex++;
	} while((activeLines < MAXLINES) && (cvOptIndex != CVOPT_END));

	for(index = activeLines; index < MAXLINES; index++)	/* delete pointers not in use */
	{
		tMCvOption_SetOpenFnct(CVOPT_END, index);
	}
	return activeLines;
}


/* Private functions ---------------------------------------------------------*/
