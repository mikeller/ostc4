///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenu.c
/// \brief  Major menu with extra page 0 for edit functionality since V0.0.2
/// \author heinrichs weikamp gmbh
/// \date   30-April-2014
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

#include "gfx_fonts.h"
#include "tHome.h"
#include "tMenuDeco.h"
#include "tMenuDecoParameter.h"
#include "tMenuEditDeco.h"
#include "tMenuEditDecoParameter.h"
#include "tMenuEditGasOC.h"
#include "tMenuEditHardware.h"
#include "tMenuEditPlanner.h"
#include "tMenuEditSetpoint.h"
#include "tMenuEditSystem.h"
#include "tMenuEditXtra.h"
#include "tMenuEditCustom.h"
#include "tMenuGas.h"
#include "tMenuHardware.h"
#include "tMenuPlanner.h"
#include "tMenuSetpoint.h"
#include "tMenuSystem.h"
#include "tMenuXtra.h"
#include "tMenuCustom.h"

/* Private types -------------------------------------------------------------*/
#define MAXPAGES 		11
#define CURSOR_HIGH 	25
#define TAB_HEADER_HIGH	25
#define TAB_BAR_HIGH	5
#define MENU_WDW_HIGH	390
#define KEY_LABEL_HIGH	25	/* Height of the label used for the the user keys */

#define SLOW_UPDATE_CNT	10	/* Some content shall not be update in short intervals => add prescalar */
#define MAXLINES	6

typedef struct
{
    uint32_t	StartAddressForPage[MAXPAGES+1];
    uint8_t		lineMemoryForNavigationForPage[MAXPAGES+1];
    uint8_t		pageMemoryForNavigation;
    uint8_t		linesAvailableForPage[MAXPAGES+1];
    uint8_t 	pagesAvailable;
    uint8_t 	pageCountNumber[MAXPAGES+1];
    uint8_t 	pageCountTotal;
    uint8_t		modeFlipPages;
    uint8_t		shadowPage[MAXPAGES+1];			/* the page is switched in the context of another page */
    uint8_t	    activeShadow;					/* Base page which is used for the shadow */
    uint8_t		disableLineMask[MAXPAGES+1];	/* Bitfield used to disable a line. The line is visible but will not be selected by cursor) */
} SMenuMemory;

/* Exported variables --------------------------------------------------------*/

/* Announced Private variables -----------------------------------------------*/
static GFX_DrawCfgScreen	tMdesignSolo;
static GFX_DrawCfgScreen	tMdesignCursor;

/* Private variables ---------------------------------------------------------*/
static GFX_DrawCfgWindow	tMwindow;
static GFX_DrawCfgScreen	tMscreen;

static SMenuMemory	menu;

uint8_t actual_menu_content  = MENU_UNDEFINED;




/* Private function prototypes -----------------------------------------------*/
static void draw_tMheader(uint8_t page);
static void draw_tMcursorDesign(void);

static void draw_tMdesignSubUnselected(uint32_t *ppDestination);
static void draw_tMdesignSubSelected(uint32_t *ppDestination);
static void draw_tMdesignSubSelectedBorder(uint32_t *ppDestination);
static void tMenu_write(uint8_t page, char *text, char *subtext);
static void nextLine(void);
static void clean_line_actual_page(void);
void tM_build_pages(void);

static void gotoMenuEdit(void);

/* Exported functions --------------------------------------------------------*/

GFX_DrawCfgScreen * get_PointerMenuCursorScreen(void)
{
    return &tMdesignCursor;
}


GFX_DrawCfgScreen * get_PointerMenuCursorDesignSoloScreen(void)
{
    return &tMdesignSolo;
}


void nextline(char * text, uint8_t *textPointer)
{
    text[(*textPointer)++] = '\n';
    text[(*textPointer)++] = '\r';
    text[*textPointer] = 0;
}


void clearDisabledMenuLines(void)
{
    for(unsigned i = 0; i <= MAXPAGES; i++) {
        menu.disableLineMask[i] = 0;
    }
}


void tM_init(void)
{
    uint8_t i;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

    tMdesignCursor.FBStartAdress = getFrame(3);
    tMdesignCursor.ImageHeight = 390;
    tMdesignCursor.ImageWidth = 800;
    tMdesignCursor.LayerIndex = 0;

    tMdesignSolo.FBStartAdress = getFrame(4);
    tMdesignSolo.ImageHeight = 390;
    tMdesignSolo.ImageWidth = 800;
    tMdesignSolo.LayerIndex = 0;

    menu.pagesAvailable = 0;
    menu.pageMemoryForNavigation = 0;
    for(i=0;i<=MAXPAGES;i++)
    {
        menu.lineMemoryForNavigationForPage[i] = 0;
        menu.StartAddressForPage[i] = 0;
        menu.linesAvailableForPage[i] = 0;
        menu.shadowPage[i] = 0;
    }

    clearDisabledMenuLines();

    tMscreen.FBStartAdress = 0;
    tMscreen.ImageHeight = 480;
    tMscreen.ImageWidth = 800;
    tMscreen.LayerIndex = 1;

    draw_tMcursorDesign();

    tMwindow.Image = &tMscreen;
    tMwindow.WindowNumberOfTextLines = 6;
    tMwindow.WindowLineSpacing = 65;
    tMwindow.WindowTab = 400;
    tMwindow.WindowX0 = 20;
    tMwindow.WindowX1 = 779;

    if(!pSettings->FlipDisplay)
    {
		tMwindow.WindowY0 = 4 + KEY_LABEL_HIGH;
		tMwindow.WindowY1 = 390 + KEY_LABEL_HIGH;
    }
    else
    {
		tMwindow.WindowY0 = 480 - MENU_WDW_HIGH - TAB_HEADER_HIGH;// - TAB_BAR_HIGH;
		tMwindow.WindowY1 = 480 - TAB_HEADER_HIGH - TAB_BAR_HIGH;
    }
    actual_menu_content = MENU_UNDEFINED;
}

void tM_refresh(char *text, uint8_t *textPointer, uint8_t line, const char content[6])
{
    for(uint8_t i=0; i<6; i++)
    {
        if(((line == 0) || (line == i)) && content[i])
        {
                text[(*textPointer)++] = content[i];
        }
        text[(*textPointer)++] = '\n';
        text[(*textPointer)++] = '\r';
        text[*textPointer] = 0;
    }
}


void tM_rebuild_pages(void)
{
    menu.pagesAvailable = 0;
//		menu.pageMemoryForNavigation = 0;
    for(int i=0;i<=MAXPAGES;i++)
    {
        menu.lineMemoryForNavigationForPage[i] = 0;
        menu.linesAvailableForPage[i] = 0;
        menu.StartAddressForPage[i] = 0; // only with GFX_forceReleaseFramesWithId(5); !!!!!
    }
    GFX_forceReleaseFramesWithId(5);
    tM_build_pages();
}


void tM_rebuild_menu_after_tComm(void)
{
    tM_rebuild_pages();
}

void tM_check_content(void)
{
    uint8_t mode = 0;

    if(stateUsed->mode == MODE_DIVE)
    {
        if(stateUsed == stateRealGetPointer())
            mode = MENU_DIVE_REAL;
        else
            mode = MENU_DIVE_SIM;
    }
    else
        mode = MENU_SURFACE;

    if(actual_menu_content != mode)
    {
        actual_menu_content = mode;
        tM_rebuild_pages();
    }
}

void disableLine(uint32_t lineId)
{
	SStateList idList;

	get_idSpecificStateList(lineId, &idList);

	if((idList.page < MAXPAGES) && (idList.line < MAXLINES))
	{
		menu.disableLineMask[idList.page] |= (1 << idList.line);
	}
}

void resetLineMask(uint32_t lineId)
{
	SStateList idList;
	get_idSpecificStateList(lineId, &idList);
	if(idList.page < MAXPAGES)
	{
		menu.disableLineMask[idList.page] = 0;
	}
}
void enableLine(uint32_t lineId)
{
	SStateList idList;

	get_idSpecificStateList(lineId, &idList);

	if((idList.page < MAXPAGES) && (idList.line < MAXLINES))
	{
		menu.disableLineMask[idList.page] &= ~(1 << idList.line);
	}
}

static void clean_line_actual_page(void)
{
    uint8_t line, page;

    page = menu.pageMemoryForNavigation;
    line =  menu.lineMemoryForNavigationForPage[page];
	if(settingsGetPointer()->FlipDisplay) /* Reselect line to be deleted if display is rotated */
	{
		line = 6 - line + 1;
	}
    tMscreen.FBStartAdress = menu.StartAddressForPage[page];
    GFX_clean_line(&tMwindow, line);
}


static void update_content_actual_page(char *text, uint16_t tab, char *subtext)
{
    uint8_t page;

    page = menu.pageMemoryForNavigation;

    tMscreen.FBStartAdress = menu.StartAddressForPage[page];
    if(tab == 0)
        tMwindow.WindowTab = 400;
    else
        tMwindow.WindowTab = tab;

    tMenu_write(page, text, subtext);
}


static void update_content_with_new_frame(uint8_t page, char *text, uint16_t tab, char *subtext)
{
    char localtext[32];

    uint32_t rememberPage = menu.StartAddressForPage[page];
    menu.StartAddressForPage[page] = getFrame(5);
    tMscreen.FBStartAdress = menu.StartAddressForPage[page];

    if(tab == 0)
        tMwindow.WindowTab = 400;
    else
        tMwindow.WindowTab = tab;

    draw_tMheader(page);
    tMenu_write(page, text, subtext);

    localtext[0] = TXT_2BYTE;
    localtext[1] = TXT2BYTE_ButtonBack;
    localtext[2] = 0;
    write_content_simple(&tMscreen, 0, 800, 480-KEY_LABEL_HIGH, &FontT24,localtext,CLUT_ButtonSurfaceScreen);

    localtext[0] = '\001';
    localtext[1] = TXT_2BYTE;
    localtext[2] = TXT2BYTE_ButtonEnter;
    localtext[3] = 0;
    write_content_simple(&tMscreen, 0, 800, 480-KEY_LABEL_HIGH, &FontT24,localtext,CLUT_ButtonSurfaceScreen);

    localtext[0] = '\002';
    localtext[1] = TXT_2BYTE;
    localtext[2] = TXT2BYTE_ButtonNext;
    localtext[3] = 0;
    write_content_simple(&tMscreen, 0, 800, 480-KEY_LABEL_HIGH, &FontT24,localtext,CLUT_ButtonSurfaceScreen);

//	gfx_write_page_number(&tMscreen ,menu.pageCountNumber[page],menu.pageCountTotal,0);

    if(page == menu.pageMemoryForNavigation)
        GFX_SetFrameTop(tMscreen.FBStartAdress);
    releaseFrame(5,rememberPage);
}


static void tM_create_pagenumbering(void)
{
    menu.pageCountTotal = 0;

    for(int i=0;i<=MAXPAGES;i++)
    {
        if(menu.pageCountNumber[i])
        {
                menu.pageCountTotal++;
                menu.pageCountNumber[i] = menu.pageCountTotal;
        }
    }
}


void tM_build_page(uint32_t id, char *text, uint16_t tab, char *subtext)
{
    uint8_t linesFound;
    uint16_t i;
    SStateList idList;
    uint8_t page;

    char localtext[32];

    if(menu.pagesAvailable > MAXPAGES)
        return;

    get_idSpecificStateList(id, &idList);

    if(idList.base != BaseMenu)
        return;

    if(idList.page == 0)
        return;

    if(idList.page > MAXPAGES)
        return;

    page = idList.page;

  	if((!menu.pageCountNumber[page]) && (!menu.shadowPage[page]))		/* shadow pages are not visible on top level */
	{
		return;
	}

	if(menu.pagesAvailable == 0)
		tM_create_pagenumbering();

    if(*text == 0)
        return;

    linesFound = 1;

    if(menu.StartAddressForPage[page])
        releaseFrame(5,menu.StartAddressForPage[page]);

    menu.StartAddressForPage[page] = getFrame(5);

    if(menu.StartAddressForPage[page] == 0)
        return;

    i = 0;
    while((i < MAX_PAGE_TEXTSIZE) && text[i])
    {
        if((text[i] == '\n') && ((i + 2) < MAX_PAGE_TEXTSIZE) && text[i+1] && text[i+2])
            linesFound += 1;
        i++;
    }

    menu.linesAvailableForPage[page] = linesFound;
    menu.pagesAvailable++; /* even if it was used before */

    tMscreen.FBStartAdress = menu.StartAddressForPage[page];
    if(tab == 0)
        tMwindow.WindowTab = 400;
    else
        tMwindow.WindowTab = tab;

    draw_tMheader(page);

    tMenu_write(page, text, subtext);

    localtext[0] = TXT_2BYTE;
    localtext[1] = TXT2BYTE_ButtonBack;
    localtext[2] = 0;
    write_content_simple(&tMscreen, 0, 800, 480-KEY_LABEL_HIGH, &FontT24,localtext,CLUT_ButtonSurfaceScreen);

    localtext[0] = '\001';
    localtext[1] = TXT_2BYTE;
    localtext[2] = TXT2BYTE_ButtonEnter;
    localtext[3] = 0;
    write_content_simple(&tMscreen, 0, 800, 480-KEY_LABEL_HIGH, &FontT24,localtext,CLUT_ButtonSurfaceScreen);

    localtext[0] = '\002';
    localtext[1] = TXT_2BYTE;
    localtext[2] = TXT2BYTE_ButtonNext;
    localtext[3] = 0;
    write_content_simple(&tMscreen, 0, 800, 480-KEY_LABEL_HIGH, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
}

static void findValidPosition(uint8_t *pageOuput, uint8_t *lineOutput)
{
    uint8_t page = 0;
    uint8_t line = 0;
    uint8_t first = 0;

    *pageOuput = 0;
    *lineOutput = 0;

    /* test */
    if(menu.pagesAvailable == 0)
        return;

    for(int i=1;i<=MAXPAGES;i++)
    {
        if((menu.pageCountNumber[i] != 0)
            && (menu.linesAvailableForPage[i] != 0)
            && (menu.StartAddressForPage[i] != 0))
        {
            first = i;
            break;
        }
    }

    /* select */
    if(menu.pageMemoryForNavigation > MAXPAGES)
        menu.pageMemoryForNavigation = first;

    page = menu.pageMemoryForNavigation;

    if(page == 0)
        page = first;

    while((page <= MAXPAGES) && ((menu.linesAvailableForPage[page] == 0) || (menu.StartAddressForPage[page] == 0) || ((menu.pageCountNumber[page] == 0) && !((menu.shadowPage[page]) && (menu.pageMemoryForNavigation == page)))))
        page += 1;

    if(page > MAXPAGES)
        page = first;

    line = menu.lineMemoryForNavigationForPage[page];

    if(line == 0)
        line = 1;

    while(menu.disableLineMask[page] & ( 1 << line))
    {
    	line++;
    	if(line > menu.linesAvailableForPage[page])
    	{
    	    line = 1;
    	}
    }

    if(line > menu.linesAvailableForPage[page])
        line = 1;

    *pageOuput = page;
    *lineOutput = line;
}


static void tM_add(uint32_t id)
{
    SStateList idList;
    uint8_t page;

    get_idSpecificStateList(id, &idList);

    page = idList.page;

    if(page > MAXPAGES)
        return;

    menu.pageCountNumber[page] = 1;
}

static void tM_addShadow(uint32_t id)
{
    SStateList idList;
    uint8_t page;

    get_idSpecificStateList(id, &idList);

    page = idList.page;

    if(page > MAXPAGES)
        return;

    menu.shadowPage[page] = 1;
}


void tM_build_pages(void)
{
    char text[MAX_PAGE_TEXTSIZE];
    char subtext[MAX_PAGE_TEXTSIZE];
    uint32_t id;
    uint16_t tabPosition;
    SSettings *pSettings = settingsGetPointer();

    menu.pagesAvailable = 0;
    for(int i=0;i<=MAXPAGES;i++)
        menu.pageCountNumber[i] = 0;

    tabPosition = 400;
    *text = 0;
    *subtext = 0;

    /* 2015 Feb 02, hw
     * max 8 Menu Pages
     */


    tM_add(StMSYS); //now in both modes
    if(actual_menu_content == MENU_SURFACE)
    {
        tM_add(StMDECO);
        tM_add(StMHARD);
        tM_add(StMCustom);
//		tM_add(StMSYS); now in both modes
    }
    else
    {
        tM_add(StMXTRA);
    }
    if(actual_menu_content == MENU_SURFACE)
    {
        tM_add(StMPLAN);
    }
//	if((pSettings->dive_mode != DIVEMODE_Gauge) && (pSettings->dive_mode != DIVEMODE_Apnea))
//	{
        tM_add(StMDECOP);
//	}

    if((isLoopMode(pSettings->dive_mode)) || (stateUsed->diveSettings.ccrOption == 1))
    {
        tM_add(StMCG);
        tM_add(StMSP);
        if (actual_menu_content == MENU_SURFACE)  /* StMOG is now accessed using StMCG in CCR mode*/
        {
        	tM_add(StMXTRA);
        	tM_addShadow(StMOG);
        }
        else
        {
        	tM_add(StMOG);
        }
    }
    else
    {
        tM_add(StMOG);
    }

    id = tMOG_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMCG_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMSP_refresh(text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMXtra_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMPlanner_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMDeco_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMDecoParameters_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMPlanner_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMHardware_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMSystem_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);

    id = tMCustom_refresh(0, text, &tabPosition, subtext);
    tM_build_page(id, text, tabPosition, subtext);
}


void tM_refresh_live_content(void)
{
	static uint8_t slowUpdate = SLOW_UPDATE_CNT;
    uint8_t page = 0;
    char text[MAX_PAGE_TEXTSIZE];
    char subtext[MAX_PAGE_TEXTSIZE];
    uint16_t tabPosition;

    uint32_t globalState = get_globalState();

    slowUpdate--;
    page = menu.pageMemoryForNavigation;
    switch(globalState)
    {
    	case StMSYS: 	if(actual_menu_content == MENU_SURFACE)
    					{
    						tMSystem_refresh(0, text, &tabPosition, subtext);
    						update_content_with_new_frame(page, text, tabPosition, subtext);
    					}
    		break;
    	case StMHARD:	tMHardware_refresh(0, text, &tabPosition, subtext);
        				update_content_with_new_frame(page, text, tabPosition, subtext);
        	break;
    	case StMOG:		if((actual_menu_content != MENU_SURFACE) && (slowUpdate == 0))
    					{
    						tMOG_refresh(0, text, &tabPosition, subtext);
    						update_content_with_new_frame(page, text, tabPosition, subtext);
    					}
    		break;
        case StMCG:		if((actual_menu_content != MENU_SURFACE) && (slowUpdate == 0))
        				{
        					tMCG_refresh(0, text, &tabPosition, subtext);
        					update_content_with_new_frame(page, text, tabPosition, subtext);
        				}
    		break;
    	default:
    		break;
    }
    if(slowUpdate == 0)
    {
    	slowUpdate = SLOW_UPDATE_CNT;
    }

    tMscreen.FBStartAdress = menu.StartAddressForPage[page];
    tHome_show_lost_connection_count(&tMscreen);
    /*
    SStateList idList;
    if(actual_menu_content == MENU_SURFACE)
    {
        page = menu.pageMemoryForNavigation;
        get_idSpecificStateList(StMSYS, &idList);
        if(page == idList.page)
        {
            tMSystem_refresh(0, text, &tabPosition, subtext);
            update_content_with_new_frame(page, text, tabPosition, subtext);
        }
    }
    */
}


/* new frame only! */
void updateSpecificMenu(uint32_t id)
{
    uint8_t  page;
    SStateList idList;

    char text[MAX_PAGE_TEXTSIZE];
    char subtext[MAX_PAGE_TEXTSIZE];
    uint16_t tabPosition;

    *subtext = 0;
    *text = 0;
    tabPosition = 400;

    get_idSpecificStateList(id, &idList);
    page = idList.page;

    switch(id)
    {
    case StMOG:
        tMOG_refresh(0, text, &tabPosition, subtext);
        update_content_with_new_frame(page, text, tabPosition, subtext);
        break;
    case StMCG:
        tMCG_refresh(0, text, &tabPosition, subtext);
        update_content_with_new_frame(page, text, tabPosition, subtext);
        break;
    case StMSP:
        tMSP_refresh(text, &tabPosition, subtext);
        update_content_with_new_frame(page, text, tabPosition, subtext);
        break;
    case StMSYS:
    	tMSystem_refresh(0, text, &tabPosition, NULL);
    	update_content_with_new_frame(page, text, tabPosition, subtext);
    	break;

    default:
        break;
    }
}


void updateMenu(void)
{
    uint8_t page, line;

    char text[MAX_PAGE_TEXTSIZE];
    char subtext[MAX_PAGE_TEXTSIZE];
    uint16_t tabPosition;

    *subtext = 0;
    *text = 0;
    tabPosition = 400;

    page = menu.pageMemoryForNavigation;
    line = menu.lineMemoryForNavigationForPage[page];

    switch(get_globalState())
    {
    case StMOG:
        tMOG_refresh(0, text, &tabPosition, subtext);
        update_content_with_new_frame(page, text, tabPosition, subtext);
        break;
    case StMCG:
        tMCG_refresh(0, text, &tabPosition, subtext);
        update_content_with_new_frame(page, text, tabPosition, subtext);
        break;
    case StMSP:
        tMSP_refresh(text, &tabPosition, subtext);
        update_content_with_new_frame(page, text, tabPosition, subtext);
        break;
    case StMXTRA:
        tMXtra_refresh(0, text, &tabPosition, subtext);
        update_content_with_new_frame(page, text, tabPosition, subtext);
        break;
    case StMDECO:
        if((line == 1) || (line == 3)) // dive mode or ppO2 limits (the later for correct MOD in gaslists)
        {
            tM_rebuild_pages();
            menu.lineMemoryForNavigationForPage[page] = line; // fix 160623
            GFX_SetFrameTop(menu.StartAddressForPage[page]);
        }
        else
        {
            tMDeco_refresh(line, text, &tabPosition, subtext);
            clean_line_actual_page();
            update_content_actual_page(text, tabPosition, subtext);
        }
        break;
    case StMDECOP:
        tMDecoParameters_refresh(line, text, &tabPosition, subtext);
        clean_line_actual_page();
        update_content_actual_page(text, tabPosition, subtext);
        break;
    case StMPLAN:
        tMPlanner_refresh(line, text, &tabPosition, subtext);
        clean_line_actual_page();
        update_content_actual_page(text, tabPosition, subtext);
        break;
    case StMHARD:
        tMHardware_refresh(line, text, &tabPosition, subtext);
        clean_line_actual_page();
        update_content_actual_page(text, tabPosition, subtext);
        break;
    case StMSYS:
        if((line == 2) || (line == 3) || (line == 6))
        {
            tM_rebuild_pages();
            menu.lineMemoryForNavigationForPage[page] = line; // fix 160623
            GFX_SetFrameTop(menu.StartAddressForPage[page]);
            menu.lineMemoryForNavigationForPage[page] = line;
        }
        else
        {
            tMSystem_refresh(line, text, &tabPosition, subtext);
            clean_line_actual_page();
            update_content_actual_page(text, tabPosition, subtext);
        }
        break;
    case StMCustom:
	        tMCustom_refresh(line, text, &tabPosition, subtext);
    	    clean_line_actual_page();
        	update_content_actual_page(text, tabPosition, subtext);
        break;
    default:
        break;
    }
}

void openMenu_first_page_with_OC_gas_update(void)
{
    menu.pageMemoryForNavigation = 1;
    for(int i=0;i<=MAXPAGES;i++)
        menu.lineMemoryForNavigationForPage[i] = 0;

    set_globalState(StMOG);
    updateMenu();
    openMenu(1);
}


void openMenu(uint8_t freshWithFlipPages)
{
    uint8_t page, line;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    findValidPosition(&page, &line);
    if((page == 0) || (line == 0))
        return;

    menu.pageMemoryForNavigation = page;
    /* new test for 3button design */
    if(freshWithFlipPages)
    {
        menu.lineMemoryForNavigationForPage[page] = 0;
        menu.modeFlipPages = 1;
    }
    else
    {
        menu.lineMemoryForNavigationForPage[page] = line;
        menu.modeFlipPages = 0;
    }

    set_globalState_Menu_Page(page);


    change_CLUT_entry(CLUT_MenuLineSelectedSides, 		(CLUT_MenuPageGasOC + page - 1));
    change_CLUT_entry(CLUT_MenuLineSelectedSeperator, (CLUT_MenuPageGasOC + page - 1));


    if(((page == 6) ||  (page == 8)) && (menu.pageCountNumber[page-1] == 0))
    {
        change_CLUT_entry(CLUT_MenuLineSelectedSides, 		(CLUT_MenuPageGasOC + page - 2));
        change_CLUT_entry(CLUT_MenuLineSelectedSeperator, (CLUT_MenuPageGasOC + page - 2));

    }

    GFX_SetFrameTop(menu.StartAddressForPage[page]);
    /* new test for 3button design */

    if(!pSettings->FlipDisplay)
    {
		if(freshWithFlipPages)
			GFX_SetFrameBottom(tMdesignSolo.FBStartAdress, 0, 25, 800, 390);
		else
			GFX_SetFrameBottom((tMdesignCursor.FBStartAdress) + 65*2*(line - 1), 0, 25, 800, 390);
    }
    else
    {
		if(freshWithFlipPages)
		{
			GFX_SetFrameBottom((tMdesignSolo.FBStartAdress), 0, 480-390-KEY_LABEL_HIGH, 800, 390); //- (25 * 2 * 800), 0, 25, 800, 390);
		}
		else
			GFX_SetFrameBottom((tMdesignCursor.FBStartAdress + (390 - 65 *(line)) *2), 0,480-390-KEY_LABEL_HIGH, 800, 390); //480-390-KEY_LABEL_HIGH + 65*2*(line - 1)
    }

}

static void block_diluent_handler(_Bool Unblock)
{
    SStateList list;
    static uint8_t	linesAvailableForPageDiluent = 0;
    get_idSpecificStateList(StMCG, &list);

    if(Unblock && linesAvailableForPageDiluent)
    {
        menu.linesAvailableForPage[list.page] = linesAvailableForPageDiluent;
    }
    else
    {
        linesAvailableForPageDiluent = menu.linesAvailableForPage[list.page];
        menu.linesAvailableForPage[list.page] = 0;
    }
}

void block_diluent_page(void)
{
    block_diluent_handler(0);
}


void unblock_diluent_page(void)
{
    block_diluent_handler(1);
}


static void nextPage(void)
{
    uint8_t page, line;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

    menu.pageMemoryForNavigation += 1;

    findValidPosition(&page, &line);
    menu.pageMemoryForNavigation = page;
    /* new test for 3button design */
    //menu.lineMemoryForNavigationForPage[page] = line;
    menu.lineMemoryForNavigationForPage[page] = 0;
    menu.modeFlipPages = 1;

    set_globalState_Menu_Page(page);

    change_CLUT_entry(CLUT_MenuLineSelectedSides, 		(CLUT_MenuPageGasOC + page - 1));
    change_CLUT_entry(CLUT_MenuLineSelectedSeperator, (CLUT_MenuPageGasOC + page - 1));

    GFX_SetFrameTop(menu.StartAddressForPage[page]);
    /* new test for 3button design */
    //GFX_SetFrameBottom((.FBStartAdress) + 65*2*(line - 1), 0, 25, 800, 390);
    if(!pSettings->FlipDisplay)
    {
    	GFX_SetFrameBottom(tMdesignSolo.FBStartAdress, 0, 25, 800, 390);
    }
    else
    {
    	GFX_SetFrameBottom(tMdesignSolo.FBStartAdress, 0, 65, 800, 390);
    }
}

void selectPage(uint32_t selection)
{
    uint8_t page;
    SStateList idList;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

 //   menu.pageMemoryForNavigation = selection;

 //   findValidPosition(&page, &line);
    if(selection > MAXPAGES)		/* selection via structure */
    {
    	get_idSpecificStateList(selection, &idList);
    	page = idList.page;
    }
    else
    {
    	page = selection;
    }

    if(menu.shadowPage[page])		/* backup base page in case a shadow was selected */
    {
    	menu.activeShadow = menu.pageMemoryForNavigation;
    }

    menu.pageMemoryForNavigation = page;
    /* new test for 3button design */
    //menu.lineMemoryForNavigationForPage[page] = line;
    menu.lineMemoryForNavigationForPage[page] = 0;
    menu.modeFlipPages = 1;

    set_globalState_Menu_Page(page);

    change_CLUT_entry(CLUT_MenuLineSelectedSides, 		(CLUT_MenuPageGasOC + page - 1));
    change_CLUT_entry(CLUT_MenuLineSelectedSeperator, (CLUT_MenuPageGasOC + page - 1));

    GFX_SetFrameTop(menu.StartAddressForPage[page]);
    /* new test for 3button design */
    //GFX_SetFrameBottom((.FBStartAdress) + 65*2*(line - 1), 0, 25, 800, 390);
    if(!pSettings->FlipDisplay)
    {
    	GFX_SetFrameBottom(tMdesignSolo.FBStartAdress, 0, 25, 800, 390);
    }
    else
    {
    	GFX_SetFrameBottom(tMdesignSolo.FBStartAdress, 0, 65, 800, 390);
    }
    nextLine();
}


static void nextLine(void)
{
    uint8_t page, line;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    page = menu.pageMemoryForNavigation;
    menu.lineMemoryForNavigationForPage[page] += 1;

    findValidPosition(&page, &line);
    menu.lineMemoryForNavigationForPage[page] = line;

    /* new test for 3button design */
    menu.modeFlipPages = 0;
    if(!pSettings->FlipDisplay)
    {
    	GFX_SetFrameBottom((tMdesignCursor.FBStartAdress) + 65*2*(line - 1), 0, 25, 800, 390);
    }
    else
    {
    	GFX_SetFrameBottom((tMdesignCursor.FBStartAdress)+ (390 - 65 *(line)) *2, 0, 480-390-KEY_LABEL_HIGH, 800, 390);
    }
}


static void stepBackMenu(void)
{
	if(menu.activeShadow)	/* restore base page */
	{
		selectPage(menu.activeShadow);
		menu.activeShadow = 0;
	}
    if(menu.modeFlipPages == 0)
    {
        menu.lineMemoryForNavigationForPage[menu.pageMemoryForNavigation] = 0;
        menu.modeFlipPages = 1;
        if(!settingsGetPointer()->FlipDisplay)
        {
        	GFX_SetFrameBottom(tMdesignSolo.FBStartAdress, 0, 25, 800, 390);
        }
		else
		{
			GFX_SetFrameBottom(tMdesignSolo.FBStartAdress, 0, 480-390-KEY_LABEL_HIGH, 800, 390);
		}
    }
    else
    {
        exitMenu();
    }
}


void exitMenu(void)
{
    set_globalState_tHome();
}


static void stepForwardMenu(void)
{
    if(menu.modeFlipPages == 1)
    {
        nextLine();
    }
    else
        gotoMenuEdit();
}

static void gotoMenuEdit(void)
{
    uint8_t line;

    line = menu.lineMemoryForNavigationForPage[menu.pageMemoryForNavigation];

    switch(get_globalState())
    {
    case StMOG:
        openEdit_GasOC(line);
        break;
    case StMCG:
        openEdit_GasCC(line);
        break;
    case StMSP:
        openEdit_Setpoint(line);
        break;
    case StMXTRA:
        openEdit_Xtra(line);
        break;
    case StMDECO:
        openEdit_Deco(line);
        break;
    case StMDECOP:
        openEdit_DecoParameter(line);
        break;
    case StMPLAN:
        openEdit_Planner(line);
        break;
    case StMHARD:
        openEdit_Hardware(line);
        break;
    case StMSYS:
        openEdit_System(line);
        break;
    case StMCustom:
    	openEdit_Custom(line);
    	break;
    default:
        break;
    }
}


void sendActionToMenu(uint8_t sendAction)
{
    switch(sendAction)
    {
    case ACTION_BUTTON_ENTER:
        stepForwardMenu();
        break;
    case ACTION_BUTTON_NEXT:
        if(menu.modeFlipPages)
            nextPage();
        else
            nextLine();
        break;
    case ACTION_TIMEOUT:
    case ACTION_MODE_CHANGE:
    case ACTION_BUTTON_BACK:
        /* new test for 3button design */
        stepBackMenu();
        break;
    default:
        break;
    case ACTION_IDLE_TICK:
    case ACTION_IDLE_SECOND:
        break;
    }
/* 	tMC_OC_Gas(StMOG1, pSettings); */
}

static void tMenu_write(uint8_t page, char *text, char *subtext)
{
    if(page > MAXPAGES)
        return;
    if(menu.linesAvailableForPage[page] == 0)
        return;

    tMscreen.FBStartAdress = menu.StartAddressForPage[page];
    GFX_write_string(&FontT48, &tMwindow, text,1);
    if((*subtext) && (menu.linesAvailableForPage[page] < 6))
    {
        GFX_write_string(&FontT42, &tMwindow, subtext, (menu.linesAvailableForPage[page] + 1));
    }
}


/* Private functions ---------------------------------------------------------*/

static void draw_tMdesignSubUnselected(uint32_t *ppDestination)
{
    union al88_u
    {
        uint8_t al8[2];
        uint16_t al88;
    };

    uint16_t* prunning = (uint16_t*)*ppDestination;

    union al88_u color_seperator;
    union al88_u color_unselected;
    int i;

    color_seperator.al8[0] = CLUT_MenuLineUnselectedSeperator;
    color_unselected.al8[0] = CLUT_MenuLineUnselected;

    color_seperator.al8[1] = 0xFF;
    color_unselected.al8[1] = 0xFF;

    *(__IO uint16_t*)prunning++ = color_seperator.al88;
    *(__IO uint16_t*)prunning++ = color_seperator.al88;

    for(i = 61; i > 0; i--)
    {
        *(__IO uint16_t*)prunning++ = color_unselected.al88;
    }

    *(__IO uint16_t*)prunning++ = color_seperator.al88;
    *(__IO uint16_t*)prunning++ = color_seperator.al88;

    *ppDestination = (uint32_t)prunning;
}


static void draw_tMdesignSubSelected(uint32_t *ppDestination)
{
    union al88_u
    {
        uint8_t al8[2];
        uint16_t al88;
    };

    union al88_u color_selected;
    union al88_u color_seperator;
    int i;

    color_selected.al8[0] = CLUT_MenuLineSelected;
    color_selected.al8[1] = 0xFF;

    color_seperator.al8[0] = CLUT_MenuLineSelectedSeperator;
    color_seperator.al8[1] = 0xFF;

    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;
    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;

    for(i = 61; i > 0; i--)
    {
        *(__IO uint16_t*)*ppDestination = color_selected.al88;
        *ppDestination += 2;
    }

    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;
    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;
}


static void draw_tMdesignSubSelectedBorder(uint32_t *ppDestination)
{
    union al88_u
    {
        uint8_t al8[2];
        uint16_t al88;
        };

    union al88_u color_selected_sides;

    int i;

    color_selected_sides.al8[0] = CLUT_MenuLineSelectedSides;
    color_selected_sides.al8[1] = 0xFF;

    for(i = 65; i > 0; i--)
    {
    *(__IO uint16_t*)*ppDestination = color_selected_sides.al88;
        *ppDestination += 2;
    }
}


static void draw_tMcursorDesign(void)
{
    int i,j;
    uint32_t pDestination;

    pDestination = tMdesignCursor.FBStartAdress;

    for(j = 801; j > 0; j--)
    {
        for(i = 5; i > 0; i--)
        {
            draw_tMdesignSubUnselected(&pDestination);
        }
        if((j > 787) || (j < 17))
            draw_tMdesignSubSelectedBorder(&pDestination);
        else
            draw_tMdesignSubSelected(&pDestination);
    }

/* Draw menu background boxes which are visible if nothing is selected */
    pDestination = tMdesignSolo.FBStartAdress;

    for(j = 801; j > 0; j--)
    {
        for(i = 6; i > 0; i--)
        {
            draw_tMdesignSubUnselected(&pDestination);
        }
    }
}


static void draw_tMheader(uint8_t page)
{
    union al88_u
    {
        uint8_t al8[2];
        uint16_t al88;
    };
    union al88_u color_top;
    int i,j, k, k4text;
    uint32_t pBackup;
    uint16_t* pDestination;
    uint8_t colorText;
    uint16_t positionText;
    uint8_t pageText;

    char text8max[MAXPAGES+1][8] =
    {   "",
        "OC",
        "CC",
        "SP",
        "DATA",
        "DECO",
        "",
        "SYS",
        "",
		"",
        "SIM"
    };

    _Bool spacing[MAXPAGES+1] =
    {   0,
        0, // behind OC
        0, // behind CC
        1, // behind SP
        1, // behind DATA
        0, // behind DECO1
        1, // behind DECO2
        0, // behind SYS1
        0, // behind SYS2
		1, // behind SYS3
        1, // behind SIM
        0
    };

    if(actual_menu_content == MENU_SURFACE)
    {
    	spacing[3] = 0;		/* Display extra menu directly after setpoint */
    	sprintf(text8max[4],"OP");
    }

    pBackup = tMscreen.FBStartAdress;
    tMscreen.FBStartAdress = menu.StartAddressForPage[page];
    pDestination = (uint16_t*) menu.StartAddressForPage[page];
    positionText = 10;
    pageText = page;

    gfx_write_page_number(&tMscreen ,menu.pageCountNumber[page],menu.pageCountTotal,0);

    while((text8max[pageText][0] == 0) && (pageText > 1))
    {
        pageText--;
    }

    for(k = 1; k <= MAXPAGES; k++)
    {
        if(menu.pageCountNumber[k] != 0)
        {
            k4text = k; // new hw 170522
            if((text8max[k][0] == 0) && (menu.pageCountNumber[k-1] == 0))
                k4text = k-1;

            color_top.al8[0] = CLUT_MenuPageGasOC + k - 1;
            if (k == page)
            {
                color_top.al8[1] = 0xFF;
            }
            else
            {
                color_top.al8[1] = 0x50;
            }

            if(k4text == pageText)
            {
                colorText = CLUT_Font020;
            }
            else
            {
                colorText = CLUT_Font021;
            }

            write_content_simple(&tMscreen,positionText,775,0,&FontT42,text8max[k4text],colorText);
            /*
            write_content_simple(&tMscreen,positionText,775,0,&FontT42,text8max[k],colorText);
            if((text8max[k][0] == 0) && (menu.pageCountNumber[k-1] == 0))
                write_content_simple(&tMscreen,positionText,775,0,&FontT42,text8max[k-1],colorText);
    */
/* Draw color bars */
            if(!settingsGetPointer()->FlipDisplay)
            {
				pDestination +=  5 * 480;

					for(j = 60; j > 0; j--)
					{
						pDestination += (390 + 26);

						for(i = 16; i > 0; i--)
						{
							*(__IO uint16_t*)pDestination++ = color_top.al88;
						}
						pDestination += 48;
					}

					pDestination += 5 * 480;
					positionText += 70;

					if(((k == 4) && (actual_menu_content != MENU_SURFACE)) || ((k == 6) && (menu.pageCountNumber[5] == 0)))
					{
						pDestination += 70 * 480;
						positionText += 70;
					}

					if(spacing[k])
					{
						pDestination += 35 * 480;
						positionText += 35;
					}
				}
			else
			{
				pDestination += (800 - 5)* 480;

				for(j = 60; j > 0; j--)
				{
					pDestination -= (390 + 26);

					for(i = 16; i > 0; i--)
					{
					*(__IO uint16_t*)pDestination-- = color_top.al88;
					}
					pDestination -= 48;
				}

				pDestination -= (800) * 480;
				positionText += 66;

				if(((k == 4) && (actual_menu_content != MENU_SURFACE)) || ((k == 6) && (menu.pageCountNumber[5] == 0)))
				{
					pDestination -= 70 * 480;
					positionText += 66;
				}

				if(spacing[k])
				{
					pDestination -= 35 * 480;
					positionText += 31;
				}
			}
        }
    }
    tMscreen.FBStartAdress = pBackup;
}


char *makeGrey(bool isGrey)
{
    return isGrey ? "\031" : "";
}
