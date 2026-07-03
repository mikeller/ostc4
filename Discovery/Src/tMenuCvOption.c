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
#include "tMenuCvOptionText.h"
#include "tHome.h"  // for enum CUSTOMVIEWS
#include "t7.h"

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/


uint32_t tMCvOption_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    uint8_t textPointer;
    refreshFunc_t* pRefreshTable;
    uint8_t activeLines;
    uint8_t index = 0;
    textPointer = 0;
    *tab = 450;
    *subtext = 0;

    resetLineMask(StMOption);

    activeLines = tMCvOptText_GetTableItemCnt();
    pRefreshTable = tMCvOptText_GetTable();

   	for(index = 1; index <= activeLines; index++)
   	{
   		if((line == 0) || (index == line))
   		{
   			textPointer += pRefreshTable[index - 1](&text[textPointer]);
   		}
   		nextline(text,&textPointer);
   	}
    text[textPointer] = 0;

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
