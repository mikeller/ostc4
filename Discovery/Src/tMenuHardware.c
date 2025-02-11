///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuHardware.c
/// \brief  Main Template file for Menu Page Hardware
/// \author heinrichs weikamp gmbh
/// \date   05-Aug-2014
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
#include "tMenu.h"
#include "tMenuHardware.h"

//#define NEXTLINE(text, textPointer) 	{text[(textPointer)++] = '\n'; text[textPointer++] = '\r'; text[textPointer] = 0;}
//	NEXTLINE(text,textPointer);


//\	text[(*textPointer)++] = '\r';	text[*textPointer] = 0;

/* Exported functions --------------------------------------------------------*/

uint32_t tMHardware_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{

    SSettings *data;
    uint8_t textPointer, actualBrightness;
    char sensorStatusColor[3];
    int32_t button[4];
    data = settingsGetPointer();
    textPointer = 0;
    *tab = 450;
    *subtext = 0;
/*
    const char content[6] =
    {	TXT_Compass,
        TXT_o2Sensors,
        TXT_Brightness,
        TXT_FirmwareUpdate,//TXT_Luftintegration,
        0,//TXT_FirmwareUpdate,
        0
    };
    tM_refresh(text,&textPointer,line,content);
*/
    if((line == 0) || (line == 1))
    {
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_Bluetooth;
        text[textPointer++] = '\t';
        if(settingsGetPointer()->bluetoothActive)
            text[textPointer++] = '\005';
        else
            text[textPointer++] = '\006';
        text[textPointer] = 0;
    }
    nextline(text,&textPointer);

    if((line == 0) || (line == 2))
    {
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_Compass;
        text[textPointer++] = '\t';

        if(settingsGetPointer()->compassBearing != 0)
        {
            textPointer += snprintf(&text[textPointer], 20, "(%03u`)", settingsGetPointer()->compassBearing % 360);
        }
        text[textPointer] = 0;
/*
        textPointer += snprintf(&text[textPointer],20,"%i  %i  %i"
            ,stateUsed->lifeData.compass_DX_f
            ,stateUsed->lifeData.compass_DY_f
            ,stateUsed->lifeData.compass_DZ_f);
*/
    }
    nextline(text,&textPointer);

    if((line == 0) || (line == 3))
    {
        text[textPointer++] = TXT_o2Sensors;
        if((stateUsed->lifeData.ppO2Sensor_bar[0] != 0) || (stateUsed->lifeData.ppO2Sensor_bar[1] != 0) || (stateUsed->lifeData.ppO2Sensor_bar[2] != 0))
        {
            text[textPointer++] = '\t';
            sensorStatusColor[0] = '\020';
            sensorStatusColor[1] = '\020';
            sensorStatusColor[2] = '\020';

            if((stateUsed->diveSettings.ppo2sensors_deactivated & 1) || (data->ext_sensor_map[0] > SENSOR_DIGO2M))
                    sensorStatusColor[0] = '\031';
            if((stateUsed->diveSettings.ppo2sensors_deactivated & 2) || (data->ext_sensor_map[1] > SENSOR_DIGO2M))
                    sensorStatusColor[1] = '\031';
            if((stateUsed->diveSettings.ppo2sensors_deactivated & 4) || (data->ext_sensor_map[1] > SENSOR_DIGO2M))
                    sensorStatusColor[2] = '\031';

            textPointer += snprintf(&text[textPointer],20,"%c%01.1f  %c%01.1f  %c%01.1f\020"
                ,sensorStatusColor[0], stateUsed->lifeData.ppO2Sensor_bar[0]
                ,sensorStatusColor[1], stateUsed->lifeData.ppO2Sensor_bar[1]
                ,sensorStatusColor[2], stateUsed->lifeData.ppO2Sensor_bar[2]);
        }
    }
    nextline(text,&textPointer);

    if((line == 0) || (line == 4))
    {
        text[textPointer++] = TXT_Brightness;
        text[textPointer++] = '\t';

        actualBrightness = data->brightness;

        if(actualBrightness == 0)
            text[textPointer++] = TXT_Cave;
        else if(actualBrightness == 1)
            text[textPointer++] = TXT_Eco;
        else if(actualBrightness == 2)
            text[textPointer++] = TXT_Normal;
        else if(actualBrightness == 3)
            text[textPointer++] = TXT_Bright;
        else if(actualBrightness == 4)
            text[textPointer++] = TXT_Ultrabright;
        else
            text[textPointer++] = '+';
    }
    nextline(text,&textPointer);

    if((line == 0) || (line == 5))
    {
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_ButtonSensitivity;
        text[textPointer++] = '\t';

        for(int i=0;i<=3;i++)
        {
            button[i] = (uint8_t)settingsGetPointer()->ButtonResponsiveness[i];
        }
//		textPointer += snprintf(&text[textPointer],25,"%i  %i  %i",button[0],button[1],button[2]);
        textPointer += snprintf(&text[textPointer],25,
            "%li"
            "\016\016"
            " %%"
            "\017",
            button[3]);
    }
    nextline(text,&textPointer);

    if((line == 0) || (line == 6))
    {
            text[textPointer++] = TXT_2BYTE;
            text[textPointer++] = TXT2BYTE_FLIPDISPLAY;
            text[textPointer++] = '\t';
            if(settingsGetPointer()->FlipDisplay)
                text[textPointer++] = '\005';
            else
                text[textPointer++] = '\006';
            text[textPointer] = 0;
            nextline(text,&textPointer);
    }

    return StMHARD;
}
