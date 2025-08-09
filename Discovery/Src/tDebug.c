///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tDebug.c
/// \brief  Screen with Terminal Out
/// \author heinrichs weikamp gmbh
/// \date   06-April-2016
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
#include "tDebug.h"

#include "data_exchange_main.h"
#include "gfx_engine.h"
#include "gfx_fonts.h"
#include "ostc.h"
#include "tInfo.h"
#include "tComm.h"

#include "stm32f4xx_hal.h"

#include <string.h>

uint8_t tD_selection_page = 1;
uint8_t tD_debugModeActive = 0;
uint8_t tD_status = 0;

char tD_communication[6][40];

void tDebug_Action(void);
void tDebug_NextPage(void);


void setDebugMode(void)
{
    tD_debugModeActive = 1;
}


void exitDebugMode(void)
{
	if(tD_debugModeActive)
	{
		MX_Bluetooth_PowerOff();
		settingsGetPointer()->debugModeOnStart = 0;
		tD_debugModeActive = 0;
	}
}


uint8_t inDebugMode(void)
{
    return tD_debugModeActive;
/*
    if(settingsGetPointer()->showDebugInfo == 2)
        return 1;
    else
        return 0;
*/
}


void tDebug_start(void)
{
    MX_Bluetooth_PowerOn();
    tComm_StartBlueModConfig();

    tD_debugModeActive = 1;
    tD_status = 0;
    for(int i=0;i<6;i++)
        tD_communication[i][0] = 0;

    set_globalState(StIDEBUG);
}


void tDebugControl(uint8_t sendAction)
{
    switch(sendAction)
    {
    case ACTION_BUTTON_ENTER:
            tDebug_Action();
        break;
    case ACTION_BUTTON_NEXT:
            tDebug_NextPage();
        break;
    case ACTION_TIMEOUT:
    case ACTION_MODE_CHANGE:
    case ACTION_BUTTON_BACK:
    exitInfo();
    	break;
    default:
        break;
    case ACTION_IDLE_TICK:
    case ACTION_IDLE_SECOND:
        break;
    }
}


void tDebug_refresh(void)
{
    uint8_t color;
    char text[50];

    tInfo_write_content_simple( 700,780,  20, &FontT24, "\0021/1", CLUT_NiceGreen);

    tInfo_write_content_simple(  20,780,  20, &FontT24, "Debug Terminal", CLUT_NiceGreen);
/*
    snprintf(text,50,"X: %i  Y: %i  Z: %i  %03.0f  %03.0f"
                                    ,stateUsed->lifeData.compass_DX_f
                                    ,stateUsed->lifeData.compass_DY_f
                                    ,stateUsed->lifeData.compass_DZ_f
                                    ,stateUsed->lifeData.compass_roll
                                    ,stateUsed->lifeData.compass_pitch
    );
*/
    snprintf(text,50,"roll  %.0f  pitch  %.0f"
                                    ,stateUsed->lifeData.compass_roll
                                    ,stateUsed->lifeData.compass_pitch
    );

    tInfo_write_content_simple(  20,780,  60, &FontT24, text, CLUT_NiceGreen);

    for(int i=0;i<6;i++)
    {
        if(i%2)
            color = CLUT_WarningRed;
        else
            color = CLUT_WarningYellow;
        tInfo_write_content_simple(  20,780, (60*i)+100, &FontT42, tD_communication[i], color);
    }
}


void tDebug_NextPage(void)
{

}


void tDebug_helper_replaceCRLF(char *text, uint8_t maxlength)
{
    for(int i=0; i<maxlength; i++)
    {
        if(text[i] == 0)
            break;
        if((text[i] == '\r') || (text[i] == '\n'))
            text[i] = ' ';
    }
}


void tDebug_Action(void)
{
    char aRxBuffer[50];

    char aTxBufferEscapeSequence[4] = "+++";
    char aTxBufferName[22] = "AT+BNAME=OSTC4-12345\r";
    char aTxBufferWrite[6] = "AT&W\r";
    gfx_number_to_string(5,1,&aTxBufferName[15],hardwareDataGetPointer()->primarySerial);

    tD_status++;

    switch(tD_status)
    {
    case 1:
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferEscapeSequence, 3, 2000) == HAL_OK)
        {
            strcpy(tD_communication[0],aTxBufferEscapeSequence);
        }
        else
        {
            strcpy(tD_communication[0],"Error.");
        }

        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 3, 2000) == HAL_OK)
        {
            aRxBuffer[3] = 0;
            tDebug_helper_replaceCRLF(aRxBuffer, 3);
            strcpy(tD_communication[1],aRxBuffer);
        }
        else
        {
            strcpy(tD_communication[1],"Error.");
        }
        break;

    case 2:
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferName, 21, 2000) == HAL_OK)
        {
            strcpy(tD_communication[2],aTxBufferName);
        }
        else
        {
            strcpy(tD_communication[2],"Error.");
        }

        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 21+6, 2000) == HAL_OK)
        {
            aRxBuffer[21+6] = 0;
            tDebug_helper_replaceCRLF(aRxBuffer, 21+6);
            strcpy(tD_communication[3],aRxBuffer);
        }
        else
        {
            strcpy(tD_communication[3],"Error.");
        }
        break;

    case 3:
        if(HAL_UART_Transmit(&UartHandle, (uint8_t*)aTxBufferWrite, 5, 2000) == HAL_OK)
        {
            strcpy(tD_communication[4],aTxBufferWrite);
        }
        else
        {
            strcpy(tD_communication[4],"Error.");
        }

        if(HAL_UART_Receive(&UartHandle, (uint8_t*)aRxBuffer, 5+6, 2000) == HAL_OK)
        {
            aRxBuffer[5+6] = 0;
            tDebug_helper_replaceCRLF(aRxBuffer, 5+6);
            strcpy(tD_communication[5],aRxBuffer);
        }
        else
        {
            strcpy(tD_communication[5],"Error.");
        }
        break;

    default:
        tD_status = 0;
        break;
    }
}



/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/

