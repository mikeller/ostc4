///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tInfoLog.c
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
#include "tInfoLog.h"

#include "gfx_fonts.h"
#include "logbook.h"
#include "show_logbook.h"
#include "tHome.h"
#include "tInfo.h"
#include "tMenu.h"
#include "unit.h"
#include "externLogbookFlash.h"
#include "configuration.h"
#include "logbook_miniLive.h"

/* Exported variables --------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/
typedef struct
{
    uint8_t		page;
    uint8_t		line;
    uint8_t		linesAvailableForPage;
    uint8_t		modeFlipPages;
    uint8_t		maxpages;
} SInfoLogMemory;

/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	INFOLOGscreen;
GFX_DrawCfgScreen	*pMenuCursor, *pMenuCursorDesignSolo;
GFX_DrawCfgWindow	INFOLOGwindow;

SInfoLogMemory infolog;

/* Private function prototypes -----------------------------------------------*/
void tInfoLog_BuildAndShowNextPage(void);
void tInfoLog_nextLine(void);
void showLog(void);
void showNextLogPage(void);
void stepBackInfo(void);
void stepForwardInfo(void);
void showLogExit(void);

/* Exported functions --------------------------------------------------------*/
void tInfoLog_init(void)
{
    INFOLOGscreen.FBStartAdress = 0;
    INFOLOGscreen.ImageHeight = 480;
    INFOLOGscreen.ImageWidth = 800;
    INFOLOGscreen.LayerIndex = 1;

    INFOLOGwindow.Image = &INFOLOGscreen;
    INFOLOGwindow.WindowNumberOfTextLines = 6;
    INFOLOGwindow.WindowLineSpacing = 65;
    INFOLOGwindow.WindowTab = 400;
    INFOLOGwindow.WindowX0 = 20;
    INFOLOGwindow.WindowX1 = 779;
    if(!settingsGetPointer()->FlipDisplay)
    {
    	INFOLOGwindow.WindowY0 = 4 + 25;
    	INFOLOGwindow.WindowY1 = 390 + 25;
    }
    else
    {
    	INFOLOGwindow.WindowY0 = 479 - 390;
    	INFOLOGwindow.WindowY1 = 479 - 25;
    }
}


void openInfoLogLastDive(void)
{
    infolog.page = 0;
    SLogbookHeader logbookHeader;
    if(logbook_getHeader(0,&logbookHeader))
    {
        set_globalState(StILOGSHOW); // all the rest with zeros
        infolog.page = 1;
        infolog.line = 1;
        show_logbook_test(1, 0);
    }
    else
        openLog(0);
}


void openLog(_Bool recallKeepPage)
{
    if(recallKeepPage && infolog.page)
        infolog.page--;
    else
        infolog.page = 0;

    infolog.modeFlipPages = 1;
    set_globalState_Log_Page(infolog.page);
    infolog.maxpages = (logbook_getNumberOfHeaders() + 5) / 6;
    tInfoLog_BuildAndShowNextPage();

    pMenuCursor = get_PointerMenuCursorScreen();
    pMenuCursorDesignSolo = get_PointerMenuCursorDesignSoloScreen();

    change_CLUT_entry(CLUT_MenuLineSelectedSides, 		CLUT_InfoPageLogbook);
    change_CLUT_entry(CLUT_MenuLineSelectedSeperator, CLUT_InfoPageLogbook);

    //GFX_ResetLayer(TOP_LAYER);
    //GFX_ResetLayer(BACKGRD_LAYER);

    if(infolog.page == 255)
        GFX_SetFrameBottom((INFOLOGscreen.FBStartAdress), 0, 0, 800, 480);
    else
    {
//	very old: GFX_SetFrameBottom((pMenuCursor->FBStartAdress), 0, 0, 800, 390);
// no, set cursor to firt line instead with tInfoLog_nextLine()		GFX_SetFrameBottom((pMenuCursorDesignSolo->FBStartAdress), 0, 25, 800, 390);
        tInfoLog_nextLine();
    }
}


void sendActionToInfoLogList(uint8_t sendAction)
{
    switch(sendAction)
    {
    case ACTION_BUTTON_ENTER:
        stepForwardInfo();
        break;
    case ACTION_BUTTON_NEXT:

        if(infolog.modeFlipPages)
        {
            tInfoLog_BuildAndShowNextPage();
//				GFX_SetFrameBottom((pMenuCursor->FBStartAdress), 0, 25, 800, 390);
        }
        else
            tInfoLog_nextLine();
        break;
    case ACTION_TIMEOUT:
    case ACTION_MODE_CHANGE:
    case ACTION_BUTTON_BACK:
        stepBackInfo();
        break;
    default:
        break;
    case ACTION_IDLE_TICK:
    case ACTION_IDLE_SECOND:
        break;
    }
}


void sendActionToInfoLogShow(uint8_t sendAction)
{
#ifdef ENABLE_T3_PROFILE_VIEW
	uint8_t stepBack;
#endif
    switch(sendAction)
    {
    case ACTION_BUTTON_ENTER:
#ifdef ENABLE_T3_PROFILE_VIEW
    	if(getActiveLogPage() == 1)
    	{
   		    stepBack = (6 * (infolog.page - 1)) + infolog.line - 1;
    		prepareReplayLog(stepBack);
    		updateReplayIncdicator(&INFOLOGscreen);
    	}
#endif
        break;
    case ACTION_BUTTON_NEXT:
        showNextLogPage();
        break;
    case ACTION_TIMEOUT:
    case ACTION_MODE_CHANGE:
    case ACTION_BUTTON_BACK:
        if(get_globalState() == StILOGSHOW) // no page nor line
        {
            openLog(1);
        }
        else
        {
            showLogExit();
        }
        show_logbook_exit();
        break;
    default:
        break;
    case ACTION_IDLE_TICK:
    case ACTION_IDLE_SECOND:
        break;
    }
}


/* Private functions ---------------------------------------------------------*/

void exitLog(void)
{
    //set_globalState_tHome();
    exitInfo();
    releaseFrame(15,INFOLOGscreen.FBStartAdress);
}


void stepBackInfo(void)
{
    if(infolog.modeFlipPages == 0)
    {
        infolog.line = 0;
        infolog.modeFlipPages = 1;

        tInfo_write_buttonTextline(&INFOLOGscreen, TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);

        if(!settingsGetPointer()->FlipDisplay)
        {
        	GFX_SetFrameBottom(pMenuCursorDesignSolo->FBStartAdress, 0, 25, 800, 390);
        }
        else
        {
        	GFX_SetFrameBottom(pMenuCursorDesignSolo->FBStartAdress, 0, 65, 800, 390);
        }

    }
    else
        exitLog();
}


void stepForwardInfo(void)
{
    if(infolog.modeFlipPages == 1)
    {
        tInfoLog_nextLine();
    }
    else
        showLog();
}


void tInfoLog_BuildAndShowNextPage(void)
{
    char text[MAX_PAGE_TEXTSIZE];
    uint16_t textPointer = 0;
    SLogbookHeader logbookHeader;
//	uint16_t divetime = logbookHeader.diveTimeMinutes;
//	uint16_t maxDepth =  logbookHeader.maxDepth/100;
    int i = 0;
    uint8_t date[2], month,day;
    char timeSuffix;
    uint8_t hours;

    if(INFOLOGscreen.FBStartAdress)
        releaseFrame(15,INFOLOGscreen.FBStartAdress);
    INFOLOGscreen.FBStartAdress = getFrame(15);

    infolog.page += 1;
    infolog.linesAvailableForPage = 0;

    if((infolog.page < 1) || (infolog.page > 43)) /* max with 256 entries */
        infolog.page = 1;


    text[0] = '\001';
    text[1] = TXT_Logbook;
    text[2] = 0;
    gfx_write_topline_simple(&INFOLOGscreen, text, CLUT_InfoPageLogbook);

    *text = 0;
    if(!logbook_getHeader((infolog.page - 1) * 6,&logbookHeader))
    {
        infolog.page = 1;
        if(!logbook_getHeader((infolog.page - 1) * 6,&logbookHeader))
        {
            infolog.page = 255;
            infolog.linesAvailableForPage = 0;
            text[0] = TXT_LogbookEmpty;
            text[1] = 0;
        }
    }

    if((*text == 0) && (infolog.maxpages > 1))
    {
        snprintf(text,10, "\002" "%u/%u", infolog.page, infolog.maxpages);
        gfx_write_topline_simple(&INFOLOGscreen, text, CLUT_InfoPageLogbook);
        *text = 0;
    }

    if(*text == 0)
    {
        infolog.line = 0;
        textPointer = 0;
        if(settingsGetPointer()->date_format == DDMMYY)
        {
            day = 0;
            month = 1;
        }
        else
        {
            day = 1;
            month = 0;
        }
        for(i = 0; i < 6; i++)
        {
            if(i)
            {
                if(!logbook_getHeader(((infolog.page - 1) * 6) + i, &logbookHeader))
                        break;
            }
            infolog.linesAvailableForPage += 1;
            uint16_t divetime = logbookHeader.diveTimeMinutes;
            uint16_t maxDepthMeter =  logbookHeader.maxDepth/100;
            uint16_t maxDepthSubmeter =  (logbookHeader.maxDepth - maxDepthMeter * 100)/10;
            uint16_t number = ((infolog.page - 1) * 6) + i + 1;
            if(settingsGetPointer()->logbookOffset)
            {
                if(number <= settingsGetPointer()->logbookOffset)
                    number = settingsGetPointer()->logbookOffset + 1 - number;
            }
            date[day] = logbookHeader.dateDay;
            date[month] = logbookHeader.dateMonth;

            text[textPointer++] = '\034';// monospaced space large size mode
            textPointer += snprintf(&text[textPointer], 20,"\031%04u \020", number);
/*			if(number < 1000)
                textPointer += snprintf(&text[textPointer], 20,"\031%2u \020", number);
            else
                textPointer += snprintf(&text[textPointer], 20,"\031\016\016%3u \017\020", number);
*/
            textPointer += snprintf(&text[textPointer], 20,"%02d.%02d ",date[0],date[1]);

            if (settingsGetPointer()->amPMTime)
            {
				if (logbookHeader.timeHour > 11)
				{
					timeSuffix = 'P';
				}
				else
				{
					timeSuffix = 'A';
				}

				if (logbookHeader.timeHour % 12 == 0)
				{
					hours = 12;
				}
				else
				{
					hours = (logbookHeader.timeHour % 12);
				}

				textPointer += snprintf(&text[textPointer], 20,"%02d:%02d\016\016%cM\017", hours,logbookHeader.timeMinute, timeSuffix);
            }
            else
            {
            	textPointer += snprintf(&text[textPointer], 20,"%02d:%02d ",logbookHeader.timeHour,logbookHeader.timeMinute);
            }

            switch(logbookHeader.decoModel)
            {
            case 1:
            	if(!settingsGetPointer()->nonMetricalSystem)	/* safe space to avoid cursor overlap */
            	{
            		if(settingsGetPointer()->amPMTime)
            		{
            			textPointer += snprintf(&text[textPointer],20,"\016\016 GF \017");
            		}
            		else
            		{
            			textPointer += snprintf(&text[textPointer],20,"\016\016GF \017");
            		}
            	}
            	else
            	{
            		textPointer += snprintf(&text[textPointer],20,"\016\016 GF  \017");
            	}
                break;
            case 2:
                textPointer += snprintf(&text[textPointer],20,"\016\016VPM\017");
                break;
            default:
                textPointer += snprintf(&text[textPointer],20,"\016\016  *  \017");
                break;
            }

            if(settingsGetPointer()->nonMetricalSystem)
            {
                float maxDepthFeet = 0;
                maxDepthFeet = unit_depth_float(((float)logbookHeader.maxDepth)/100);
                textPointer += snprintf(&text[textPointer], 20,"%3.0f\016\016ft\017 ", maxDepthFeet);
            }
            else
            {
            	textPointer += snprintf(&text[textPointer], 20,"%3d.%d\016\016m \017", maxDepthMeter,maxDepthSubmeter);
            }
            textPointer += snprintf(&text[textPointer], 20,"%3d\016\016min\017\n\r", divetime);

        }
    }
    GFX_write_string(&FontT48, &INFOLOGwindow, text,1);

    if(infolog.page == 255) {
        tInfo_write_buttonTextline(&INFOLOGscreen, TXT2BYTE_ButtonBack,0,0);
    } else {
        tInfo_write_buttonTextline(&INFOLOGscreen, TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
    }

    GFX_SetFrameTop(INFOLOGscreen.FBStartAdress);
    set_globalState_Log_Page(infolog.page);
}


void tInfoLog_nextLine(void)
{
    if(infolog.linesAvailableForPage == 0)
        return;

    infolog.line += 1;
    if(infolog.line > infolog.linesAvailableForPage)
        infolog.line = 1;

    infolog.modeFlipPages = 0;

    if (infolog.linesAvailableForPage > 1) {
        tInfo_write_buttonTextline(&INFOLOGscreen, TXT2BYTE_Page, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonNext);
    } else {
        tInfo_write_buttonTextline(&INFOLOGscreen, TXT2BYTE_Page, TXT2BYTE_ButtonEnter, 0);
    }

    if(!settingsGetPointer()->FlipDisplay)
    {
    	GFX_SetFrameBottom((pMenuCursor->FBStartAdress) + 65*2*(infolog.line - 1), 0, 25, 800, 390);
    }
    else
    {
    	GFX_SetFrameBottom((pMenuCursor->FBStartAdress)+ (390 - 65 *(infolog.line)) *2, 0, 480-390-25, 800, 390);
    }
}


void showLogExit(void)
{
    GFX_SetFrameTop(INFOLOGscreen.FBStartAdress);
    GFX_SetFrameBottom((pMenuCursor->FBStartAdress) + 65*2*(infolog.line - 1), 0, 25, 800, 390);
    set_globalState_Log_Page(infolog.page);
}


void showLog(void)
{
    uint8_t stepBack;

    if(infolog.page == 255)
        return;

    stepBack = (6 * (infolog.page - 1)) + infolog.line - 1;
    //build_logbook_test();
    show_logbook_test(1, stepBack);
}


void showNextLogPage(void)
{
    uint8_t stepBack;

    if(infolog.page == 255)
        return;

    stepBack = (6 * (infolog.page - 1)) + infolog.line - 1;
    //build_logbook_test();
    show_logbook_test(0, stepBack);
}

