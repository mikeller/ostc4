///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuCvOption.c
/// \brief  Main Template file for Menu Page System settings
/// \author heinrichs weikamp gmbh
/// \date   24-Apr-2025
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
#include "tMenu.h"
#include "tMenuCvOption.h"
#include "tHome.h"  // for enum CUSTOMVIEWS and init_t7_compass()
#include "t7.h"

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

uint32_t tMCvOption_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    SSettings *data;
    uint8_t textPointer;

    data = settingsGetPointer();
    textPointer = 0;
    *tab = 300;
    *subtext = 0;

    resetLineMask(StMOption);

    if((line == 0) || (line == 1))
    {
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_Compass;
        text[textPointer++] = '\t';

        if(settingsGetPointer()->compassBearing != 0)
        {
            textPointer += snprintf(&text[textPointer], 20, "(%03u`)", settingsGetPointer()->compassBearing % 360);
        }
        text[textPointer] = 0;
    }
    nextline(text,&textPointer);
    if (line == 0 || line == 2)
    {
    	if(t7_customview_disabled(CVIEW_Timer))
    	{
    		text[textPointer++] = '\031';		/* change text color */
    	    textPointer += snprintf(&text[textPointer], 21, "%c%c\t%u:%02u \016\016[m:ss]\017", TXT_2BYTE, TXT2BYTE_Timer, data->timerDurationS / 60, data->timerDurationS % 60);
    	    disableLine(StMOption_Timer);
            text[textPointer++] = '\020';		/* restore text color */
    	}
    	else
    	{
    		textPointer += snprintf(&text[textPointer], 21, "%c%c\t%u:%02u \016\016[m:ss]\017", TXT_2BYTE, TXT2BYTE_Timer, data->timerDurationS / 60, data->timerDurationS % 60);
    	}
    }
    nextline(text,&textPointer);

    return StMOption;
}
void tMCvOption_checkLineStatus(void)
{
	uint8_t localLineMask = 0;
	uint8_t lineMask = getLineMask(StMOption);

	if(t7_customview_disabled(CVIEW_Timer))
    {
    	localLineMask |= 1 << 2;
    }

	if(lineMask != localLineMask)
	{
		updateMenu();
	}
}

/* Private functions ---------------------------------------------------------*/
