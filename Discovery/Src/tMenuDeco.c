///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuDeco.c
/// \brief  Main Template file for Menu Page Deco
/// \author heinrichs weikamp gmbh
/// \date   31-July-2014
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
#include "tMenuDeco.h"
#include "unit.h"

/* Exported functions --------------------------------------------------------*/

uint32_t tMDeco_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    uint8_t textPointer;
    uint8_t	futureTTS;
    double ppO2max_deco, ppO2max_std;
    char divemode;
    textPointer = 0;
    *tab = 370;
    *subtext = 0;

    SSettings *data = settingsGetPointer();

    futureTTS = data->future_TTS;
    ppO2max_std = data->ppO2_max_std;
    ppO2max_std /= 100.0;
    ppO2max_deco = data->ppO2_max_deco;
    ppO2max_deco /= 100.0;

    if((line == 0) || (line == 1))
    {
        switch(data->dive_mode)
        {
        case DIVEMODE_OC:
            divemode = TXT_OpenCircuit;
            break;
        case DIVEMODE_CCR:
            divemode = TXT_ClosedCircuit;
            break;
        case DIVEMODE_Gauge:
            divemode = TXT_Gauge;
            break;
        case DIVEMODE_Apnea:
            divemode = TXT_Apnoe;
            break;
        case DIVEMODE_PSCR:
            divemode = TXT_PSClosedCircuit;
        break;
        default :
        	divemode = TXT_OpenCircuit;
        	break;
        }
        textPointer += snprintf(&text[textPointer], 60,\
            "%c"
            "\t"
            "%c"
            , TXT_DiveMode, divemode
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 2))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "ppO2"
            "\016\016"
            "max"
            "\017"
            "\t"
            "%0.2f"
            "\016\016"
            " bar "
            "  deco "
            "\017"
            "%0.2f"
            "\016\016"
            " bar"
            "\017"
            , ppO2max_std, ppO2max_deco
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 3))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "%c"
            "\t"
            "%u"
            "\016\016"
            " %c"
            "\017"
            "  @     "
            "%u"
            "\016\016"
            " %c%c"
            "\017"
            , TXT_SafetyStop
            , settingsGetPointer()->safetystopDuration
            ,TXT_Minutes
            , unit_depth_integer(settingsGetPointer()->safetystopDepth)
            , unit_depth_char1()
            , unit_depth_char2()
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 4))
    {
    	if(settingsGetPointer()->slowExitTime !=  0)
    	{
			textPointer += snprintf(&text[textPointer], 60,\
				"%c%c\t%u\016\016 %c\017  ^      %u\016\016 %c%c\017"
				, TXT_2BYTE
				, TXT2BYTE_SlowExit
				, settingsGetPointer()->slowExitTime
				,TXT_Minutes
				, unit_depth_integer(settingsGetPointer()->last_stop_depth_meter)
				, unit_depth_char1()
				, unit_depth_char2()
			);
    	}
    	else
    	{
			textPointer += snprintf(&text[textPointer], 60, "%c%c\t%c%c"
				, TXT_2BYTE
				, TXT2BYTE_SlowExit
				, TXT_2BYTE
				, TXT2BYTE_MoCtrlNone
				);
    	}
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;
    if((line == 0) || (line == 5))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "%c"
            "\t"
            "%u"
            "\016\016"
            " %c"
            "\017"
            ,TXT_FutureTTS
            ,futureTTS
            ,TXT_Minutes
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 6))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "%c"
            "\t"
            "%u"
            "\016\016"
            " %%"
            "\017"
            , TXT_Salinity
            , settingsGetPointer()->salinity
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;


    return StMDECO;
}
