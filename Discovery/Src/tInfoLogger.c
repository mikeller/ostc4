///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tInfoLogger.c
/// \brief  Show data which is received / send through UART interface
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
#include "tInfoLogger.h"
#include "tMenuEdit.h"
#include "data_exchange_main.h"

#include <string.h>
#include <inttypes.h>


/* Private variables ---------------------------------------------------------*/
static uint8_t lines[MAX_LOGGER_LINES][MAX_CHAR_PER_LINE + LINE_HEADER_BYTES];
static uint8_t lineWriteIndex = 0;
static uint8_t lineReadIndex = 0;
static uint32_t receivedLinesCount = 0;

static uint32_t loggerUpdateTime = 0;
static uint32_t previousGlobalState = 0;

/* Exported functions --------------------------------------------------------*/


void InfoLogger_writeLine(uint8_t* pLine,uint8_t lineLength,uint8_t direction)
{
	uint8_t LogIndex = 0;

	if(lineLength <= MAX_CHAR_PER_LINE)
	{
		if(direction == LOG_TX_LINE)
		{
			lines[lineWriteIndex][LogIndex] = '\002';	/* align right */
			LogIndex++;
		}
		memcpy(&lines[lineWriteIndex][LogIndex],pLine,lineLength);
		lines[lineWriteIndex][LogIndex + lineLength] = 0;	/* make sure string is zero terminated */
		lineWriteIndex++;
		if(lineWriteIndex == MAX_LOGGER_LINES)
		{
			lineWriteIndex = 0;
		}
		receivedLinesCount++;
	}
}

uint8_t InfoLogger_isUpdated()
{
	uint8_t ret = 0;
	if(lineReadIndex != lineWriteIndex)
	{
		ret = 1;
	}
	return ret;
}

void openInfo_Logger()
{
	previousGlobalState = get_globalState();
	set_globalState(StILOGGER);

	loggerUpdateTime = HAL_GetTick();

 //   lineWriteIndex = 0;
 //   receivedLinesCount = 0;
 //   memset(lines,0,sizeof(lines));
}

extern void refresh_Heartbeat(void);
void refreshInfo_Logger(GFX_DrawCfgScreen s)
{
	uint8_t index = 0;
    char text[31];
    uint8_t displayLine = 0;
    uint8_t color = CLUT_Font020;

    text[0] = '\001';
	text[1] = TXT_PreDive;
	text[2] = 0;

	tInfo_write_content_simple(  30, 770, ME_Y_LINE_BASE, &FontT48, text, CLUT_MenuPageHardware);


	if(InfoLogger_isUpdated())
	{
		loggerUpdateTime = HAL_GetTick();
	}
	if(receivedLinesCount > MAX_LOGGER_LINES)
	{
		displayLine = MAX_LOGGER_LINES-1;
	}
	else
	{
		displayLine = lineWriteIndex;
	}

	while (index < displayLine)
	{
		color = CLUT_Font020;
		if(lines[index][0] == '\002')
		{
			color = CLUT_Font021;
		}
		if(lineReadIndex == lineWriteIndex)
		{
			color = CLUT_Font022;
		}
		tInfo_write_content_simple(  30, 770, 50 + (index * 30), &FontT24, (char*)lines[index], color);

		if(lineReadIndex != lineWriteIndex)			/* needed for isUpdated function */
		{
			lineReadIndex++;
			if(lineReadIndex == MAX_LOGGER_LINES)
			{
				lineReadIndex = 0;
			}
		}
		index++;
	}


	if((time_elapsed_ms(loggerUpdateTime, HAL_GetTick()) > 20000))
	{
		set_globalState(previousGlobalState); /* restore state which was active before log data was received */
	}
	refresh_Heartbeat();
}

void sendActionToInfoLogger(uint8_t sendAction)
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

