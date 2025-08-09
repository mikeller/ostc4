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
#include "t7.h""

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
#ifdef ENABLE_LOGGER_WINDOW
	uint8_t LogIndex = 0;

	if(!t7_customview_disabled(CVIEW_Logger))
	{
		if(lineLength <= MAX_CHAR_PER_LINE)
		{
			memset(&lines[lineWriteIndex][0],0, (MAX_CHAR_PER_LINE + LINE_HEADER_BYTES));
			if(direction == LOG_TX_LINE)
			{
				lines[lineWriteIndex][LogIndex] = '\002';	/* align right */
				LogIndex++;
			}
			memcpy(&lines[lineWriteIndex][LogIndex],pLine,lineLength);
			lineWriteIndex++;
			if(lineWriteIndex == MAX_LOGGER_LINES)
			{
				lineWriteIndex = 0;
			}
			receivedLinesCount++;
		}
	}
#endif
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
}

void refreshInfo_Logger(GFX_DrawCfgScreen s)
{
	uint8_t index = 0;
    char text[31];
    uint8_t displayLine = 0;
    uint8_t color = CLUT_Font020;

	sprintf(text,"\001%c%c", TXT_2BYTE, TXT2BYTE_Logger);

	tInfo_write_content_simple(  30, 770, ME_Y_LINE_BASE, &FontT48, text, CLUT_InfoSurface);


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

	if(lineReadIndex != lineWriteIndex)			/* needed for isUpdated function */
	{
		lineReadIndex++;
		if(lineReadIndex == MAX_LOGGER_LINES)
		{
			lineReadIndex = 0;
		}
	}
	while (index < displayLine)
	{
		color = CLUT_Font020;
		if(lines[index][0] == '\002')
		{
			color = CLUT_NiceGreen;
		}
		if(lineReadIndex == index)
		{
			color = CLUT_NiceBlue;
		}
		tInfo_write_content_simple(  30, 770, 60 + (index * 30), &FontT24, (char*)lines[index], color);

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
	if((time_elapsed_ms(loggerUpdateTime, HAL_GetTick()) > 10000))
	{
		set_globalState(previousGlobalState); /* restore state which was active before log data was received */
		exitInfo();
	}
}

void sendActionToInfoLogger(uint8_t sendAction)
{
/* TODO: at the moment a generic return to home is used because the logger window could be opened from everywhere => implement context based return from view */
    set_globalState(previousGlobalState); /* restore state which was active before log data was received */
    exitInfoSilent();
}

