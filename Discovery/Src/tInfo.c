///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tInfo.c
/// \brief  Main Template file for Info menu page on left side
/// \author heinrichs weikamp gmbh
/// \date   11-Aug-2014
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
#include "tInfo.h"

#include "data_exchange.h"
#include "tDebug.h"
#include "gfx_fonts.h"
#include "tHome.h"
//#include "tInfoDive.h"
//#include "tInfoSurface.h"
#include "tInfoCompass.h"
#include "tInfoSensor.h"
#include "tMenu.h"
#include "tMenuEdit.h"

#include <string.h>

/* Private types -------------------------------------------------------------*/

typedef struct
{
    uint32_t pEventFunction;
    uint32_t callerID;
} SInfoEventHandler;

typedef struct
{
    char orgText[32];
    char newText[32];
    uint8_t input;
    char symbolCounter;
    int8_t begin[4], size[4];
    uint16_t coord[3];
    tFont *fontUsed;
    uint32_t callerID;
    uint8_t maintype;
    uint8_t subtype;
} SInfoIdent;

typedef enum
{
    FIELD_BUTTON = 1,
    FIELD_SELECT,
    FIELD_SYMBOL,
    FIELD_TOGGLE,
    FIELD_ON_OFF,
    FIELD_END
} SInfoField;

/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	tIscreen;
GFX_DrawCfgScreen	tIcursor;

uint8_t infoColor = CLUT_InfoSurface;

int8_t TIid = 0;
int8_t TIidLast = -1;
SInfoIdent TIident[10];

int8_t TIevid = 0;
int8_t TIevidLast = -1;
SInfoEventHandler TIevent[10];

/* Private function prototypes -----------------------------------------------*/
void tInfo_build_page(void);

void tI_set_cursor(uint8_t forThisIdentID);
void tI_startInfoFieldSelect(void);
void tInfo_write_content_of_actual_Id(void);
void tI_clean_content_of_actual_Id(void);
void tInfo_write_content_without_Id(void);

void tI_clean_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font);
void tInfo_write_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text, uint8_t color);

void tI_evaluateNewString       (uint32_t editID, uint32_t *pNewValue1, uint32_t *pNewValue2, uint32_t *pNewValue3, uint32_t *pNewValue4);

//void tI_tInfo_newInput        (uint32_t editID, uint32_t int1, uint32_t int2, uint32_t int3, uint32_t int4);
//void tI_tInfo_newButtonText   (uint32_t editID, char *text);

void tI_enterInfoField(void);
void tI_nextInfoField(void);

/* Announced function prototypes -----------------------------------------------*/
//uint8_t OnAction_ILoglist				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
//uint8_t OnAction_ISimulator			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

/* Exported functions --------------------------------------------------------*/

void tI_init(void)
{
    tIscreen.FBStartAdress = 0;
    tIscreen.ImageHeight = 480;
    tIscreen.ImageWidth = 800;
    tIscreen.LayerIndex = 1;

    tIcursor.FBStartAdress = getFrame(12);
    tIcursor.ImageHeight = 480;
    tIcursor.ImageWidth = 800;
    tIcursor.LayerIndex = 0;

    GFX_fill_buffer(tIcursor.FBStartAdress, 0xFF, CLUT_InfoCursor);
}


void openInfo(uint32_t modeToStart)
{
    if((modeToStart != StILOGLIST) && (modeToStart != StIDEBUG))
        return;

    TIid = 0;
    TIidLast = -1;
    TIevid = 0;
    TIevidLast = -1;

    if(tIscreen.FBStartAdress)
        releaseFrame(14,tIscreen.FBStartAdress);
    tIscreen.FBStartAdress = getFrame(14);

//	GFX_SetFramesTopBottom(tIscreen.FBStartAdress, tIcursor.FBStartAdress,480);
    GFX_SetFramesTopBottom(tIscreen.FBStartAdress, 0,480);
    infoColor = CLUT_InfoSurface;

    if(modeToStart == StIDEBUG)
    {
        tDebug_start();
    }
    else
    {
        openLog(0);
    }
//	openInfoLogLastDive();
}

/*
void openInfo(void)
{
    if((stateUsed->mode == MODE_DIVE) && (!is_stateUsedSetToSim()))
    {
        return;
    }

    TIid = 0;
    TIidLast = -1;
    TIevid = 0;
    TIevidLast = -1;

    if(tIscreen.FBStartAdress)
        releaseFrame(14,tIscreen.FBStartAdress);
    tIscreen.FBStartAdress = getFrame(14);

    GFX_SetFramesTopBottom(tIscreen.FBStartAdress, tIcursor.FBStartAdress,480);

    if(stateUsed->mode == MODE_DIVE)
    {
        infoColor = CLUT_InfoSurface;
        openInfo_Dive();
    }
    else
    {
        infoColor = CLUT_InfoDive;
        openInfo_Surface();
    }
}
*/

/*
uint8_t OnAction_ILoglist	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    return 255;
}
*/

void tInfo_refresh(void)
{
    uint32_t oldIscreen;
    uint32_t globalState = get_globalState();
    oldIscreen = tIscreen.FBStartAdress;
    
    if(inDebugMode())
    {
	    tIscreen.FBStartAdress = getFrame(14);
    	infoColor = CLUT_InfoCompass;
    	tDebug_refresh();
    }
    else
    {
    	switch(globalState)
    	{
    		case StICOMPASS: 	tIscreen.FBStartAdress = getFrame(14);
    							infoColor = CLUT_InfoCompass;
    							refreshInfo_Compass(tIscreen);
    				break;
    		case StISENINFO: 	tIscreen.FBStartAdress = getFrame(14);
    							infoColor = CLUT_MenuPageHardware;
    							refreshInfo_Sensor(tIscreen);
    				break;
    		default:
    				break;
    	}
    }
    if(oldIscreen != tIscreen.FBStartAdress)
    {
    	GFX_SetFramesTopBottom(tIscreen.FBStartAdress, 0,480);
    	if(oldIscreen)
    	{
    	 	releaseFrame(14,oldIscreen);
    	}
    }
}


void exitInfo(void)
{
    set_globalState_tHome();
    releaseFrame(14,tIscreen.FBStartAdress);
    exitDebugMode();
}

void exitInfoToBack(void)
{
    releaseFrame(14,tIscreen.FBStartAdress);
    exitMenuEdit_to_BackMenu();
}


void sendActionToInfo(uint8_t sendAction)
{
    if(inDebugMode())
    {
        tDebugControl(sendAction);
        return;
    }

    if(get_globalState() == StICOMPASS)
        return;

    switch(sendAction)
    {
    case ACTION_BUTTON_ENTER:
        tI_enterInfoField();
        break;
    case ACTION_BUTTON_NEXT:
        tI_nextInfoField();
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

/* Private functions ---------------------------------------------------------*/

void tInfo_build_page(void)
{
    tInfo_write_content_simple(  30, 340,  90, &FontT48, "Logbook", CLUT_Font020);

}

void tInfo_write_content_simple(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text, uint8_t color)
{
    GFX_DrawCfgWindow	hgfx;

    if(XrightGimpStyle > 799)
        XrightGimpStyle = 799;
    if(XleftGimpStyle >= XrightGimpStyle)
        XleftGimpStyle = 0;
    if(YtopGimpStyle > 479)
        YtopGimpStyle = 479;
    hgfx.Image = &tIscreen;
    hgfx.WindowNumberOfTextLines = 1;
    hgfx.WindowLineSpacing = 0;
    hgfx.WindowTab = 400;
    hgfx.WindowX0 = XleftGimpStyle;
    hgfx.WindowX1 = XrightGimpStyle;
    hgfx.WindowY1 = 479 - YtopGimpStyle;


    if(!settingsGetPointer()->FlipDisplay)
    {
    	hgfx.WindowY1 = 479 - YtopGimpStyle;
    	if(hgfx.WindowY1 < Font->height)
    	        hgfx.WindowY0 = 0;
    	    else
    	        hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
    }
    else
    {
    	hgfx.WindowY0 = YtopGimpStyle;
    	hgfx.WindowY1 = YtopGimpStyle + Font->height;
    }

    GFX_write_string_color(Font, &hgfx, text, 0, color);
}

/* Exported functions --------------------------------------------------------*/

void tI_startInfoFieldSelect(void)
{
    TIid = 0;
    tI_set_cursor(TIid);
}


void tI_nextInfoField(void)
{
    if(TIid < TIidLast)
        TIid++;
    else
        TIid = 0;
    tI_set_cursor(TIid);
}


void tI_previousInfoField(void)
{
    if(TIid > 0)
        TIid--;
    else
        TIid = TIidLast;
    tI_set_cursor(TIid);
}


uint8_t tI_get_newContent_of_actual_id_block_and_subBlock(uint8_t action)
{
    uint8_t (*onActionFunc)(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t);
    uint8_t content;

    if(TIevent[TIevid].callerID != TIident[TIid].callerID)
        return 0;

    onActionFunc = (uint8_t (*)(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t))(TIevent[TIevid].pEventFunction);

    if(TIident[TIid].maintype == FIELD_ON_OFF)
        content = TIident[TIid].input;
    else
        content = 0; /* just a default for protection */

    return onActionFunc(TIident[TIid].callerID, 0, 0, content, action);
}


void tI_enterInfoField(void)
{
    uint8_t newContent;

    TIevid = 0;
    while((TIevid < TIevidLast) && (TIevent[TIevid].callerID != TIident[TIid].callerID))
    {
        TIevid++;
    }

    if(TIevent[TIevid].callerID != TIident[TIid].callerID)
        return;

    newContent = tI_get_newContent_of_actual_id_block_and_subBlock(ACTION_BUTTON_ENTER);

    if(newContent == 255)
    {
        exitInfo();
        return;
    }

    switch(TIident[TIid].maintype)
    {
    case FIELD_BUTTON:
        break;
    case FIELD_ON_OFF:
        break;
    case FIELD_SYMBOL:
        TIident[TIid].input += 1;
        if(TIident[TIid].input >= TIident[TIid].symbolCounter)
            TIident[TIid].input = 0;
        TIident[TIid].newText[0] = TIident[TIid].orgText[TIident[TIid].input];
        tInfo_write_content_of_actual_Id();
        break;
    }
}


void tI_evaluateNewString(uint32_t editID, uint32_t *pNewValue1, uint32_t *pNewValue2, uint32_t *pNewValue3, uint32_t *pNewValue4)
{
    if(editID != TIident[TIid].callerID)
        return;

    uint8_t i, digitCount, digit;
    uint32_t sum[4], multiplier;

    for(i=0;i<4;i++)
        sum[i] = 0;

    i = 0;
    while( TIident[TIid].size[i] && (i < 4))
    {
        multiplier = 1;
        for(digitCount = 1; digitCount < TIident[TIid].size[i]; digitCount++)
            multiplier *= 10;

        for(digitCount = 0; digitCount < TIident[TIid].size[i]; digitCount++)
        {
            digit = TIident[TIid].newText[TIident[TIid].begin[i] + digitCount];

            if(digit > '0')
                digit -= '0';
            else
                digit = 0;

            if(digit > 9)
                digit = 9;

            sum[i] += digit * multiplier;

            if(multiplier >= 10)
                multiplier /= 10;
            else
                multiplier = 0;
        }
        i++;
    }

    *pNewValue1 = sum[0];
    *pNewValue2 = sum[1];
    *pNewValue3 = sum[2];
    *pNewValue4 = sum[3];
}


uint8_t tI_get_id_of(uint32_t editID)
{
    uint8_t temp_id;

    if(editID == TIident[TIid].callerID)
        return TIid;
    else
    {
        temp_id = 0;
        while((temp_id < 9) && (editID != TIident[temp_id].callerID))
            temp_id++;
        if(editID != TIident[temp_id].callerID)
            temp_id = 255;
        return temp_id;
    }
}


void tI_newButtonText(uint32_t editID, char *text)
{
    uint8_t backup_id, temp_id;

    temp_id = tI_get_id_of(editID);
    if(temp_id == 255)
        return;

    backup_id = TIid;
    TIid = temp_id;

    strncpy(TIident[TIid].newText, text, 32);
    TIident[TIid].newText[31] = 0;

    tI_clean_content_of_actual_Id();
    tInfo_write_content_of_actual_Id();

    TIid = backup_id;
}


void tInfo_set_on_off(uint32_t editID, uint8_t int1)
{
    uint8_t backup_id, temp_id;

    temp_id = tI_get_id_of(editID);
    if(temp_id == 255)
        return;

    backup_id = TIid;
    TIid = temp_id;

    TIident[TIid].input = int1;

    if(int1)
        change_CLUT_entry((CLUT_InfoField0 + TIid), CLUT_InfoActive);
    else
        change_CLUT_entry((CLUT_InfoField0 + TIid), CLUT_InfoInActive);

    tInfo_write_content_of_actual_Id();

    TIid = backup_id;
}


void tInfo_write_content_without_Id(void)
{
    tInfo_write_content( TIident[TIid].coord[0], TIident[TIid].coord[1], TIident[TIid].coord[2], TIident[TIid].fontUsed, TIident[TIid].newText, CLUT_InfoFieldRegular);
}


void tInfo_write_content_of_actual_Id(void)
{
    tInfo_write_content( TIident[TIid].coord[0], TIident[TIid].coord[1], TIident[TIid].coord[2], TIident[TIid].fontUsed, TIident[TIid].newText, (CLUT_InfoField0 + TIid));
}


void tI_clean_content_of_actual_Id(void)
{
    tI_clean_content( TIident[TIid].coord[0], TIident[TIid].coord[1], TIident[TIid].coord[2], TIident[TIid].fontUsed);
}


void tInfo_write_field_button(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text)
{
    if(TIidLast >= 9)
        return;

    TIident[TIid].maintype = FIELD_BUTTON;
    TIident[TIid].subtype  = FIELD_BUTTON;

    TIident[TIid].coord[0] = XleftGimpStyle;
    TIident[TIid].coord[1] = XrightGimpStyle;
    TIident[TIid].coord[2] = YtopGimpStyle;
    TIident[TIid].fontUsed = (tFont *)Font;
    TIident[TIid].callerID = editID;

    strncpy(TIident[TIid].orgText, text, 32);
    strncpy(TIident[TIid].newText, text, 32);
    TIident[TIid].orgText[31] = 0;
    TIident[TIid].newText[31] = 0;

    change_CLUT_entry((CLUT_InfoField0 + TIid), CLUT_InfoButtonColor1);

    if(editID == 0)
        tInfo_write_content_without_Id();
    else
    {
        tInfo_write_content_of_actual_Id();
        TIidLast = TIid;
        TIid++;
    }
}


void tInfo_write_field_symbol(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1)
{
    if(TIidLast >= 9)
        return;

    TIident[TIid].maintype = FIELD_SYMBOL;
    TIident[TIid].subtype  = FIELD_SYMBOL;

    TIident[TIid].coord[0] = XleftGimpStyle;
    TIident[TIid].coord[1] = XrightGimpStyle;
    TIident[TIid].coord[2] = YtopGimpStyle;
    TIident[TIid].fontUsed = (tFont *)Font;
    TIident[TIid].callerID = editID;

    strncpy(TIident[TIid].orgText, text, 32);
    strncpy(TIident[TIid].newText, text, 32);
    TIident[TIid].orgText[31] = 0;

    TIident[TIid].newText[0] = text[0];
    TIident[TIid].newText[1] = 0;

    TIident[TIid].input = int1;
    TIident[TIid].symbolCounter = strlen(TIident[TIid].orgText);

    change_CLUT_entry((CLUT_InfoField0 + TIid), CLUT_InfoButtonColor1);

    if(editID == 0)
        tInfo_write_content_without_Id();
    else
    {
        tInfo_write_content_of_actual_Id();
        TIidLast = TIid;
        TIid++;
    }
}


void tInfo_write_field_on_off(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1)
{
    if(TIidLast >= 9)
        return;

    TIident[TIid].maintype = FIELD_ON_OFF;
    TIident[TIid].subtype  = FIELD_ON_OFF;

    TIident[TIid].coord[0] = XleftGimpStyle;
    TIident[TIid].coord[1] = XrightGimpStyle;
    TIident[TIid].coord[2] = YtopGimpStyle;
    TIident[TIid].fontUsed = (tFont *)Font;
    TIident[TIid].callerID = editID;

    strncpy(TIident[TIid].orgText, text, 32);
    strncpy(TIident[TIid].newText, text, 32);
    TIident[TIid].orgText[31] = 0;
    TIident[TIid].newText[31] = 0;


    if(int1)
        change_CLUT_entry((CLUT_InfoField0 + TIid), CLUT_InfoActive);
    else
        change_CLUT_entry((CLUT_InfoField0 + TIid), CLUT_InfoInActive);

    if(editID == 0)
        tInfo_write_content_without_Id();
    else
    {
        tInfo_write_content_of_actual_Id();
        TIidLast = TIid;
        TIid++;
    }
}


void tInfo_setEvent(uint32_t inputEventID, uint32_t inputFunctionCall)
{
    if(TIevidLast >= 9)
        return;

    /* set cursor to first field */
    if(TIevidLast < 0)
    {
        tI_startInfoFieldSelect();
    }

    TIevent[TIevid].callerID = inputEventID;
    TIevent[TIevid].pEventFunction = inputFunctionCall;

    TIevidLast = TIevid;
    TIevid++;
}


void tI_set_cursor(uint8_t forThisIdentID)
{
    int16_t x0, x1, y0, y1;

    uint32_t xtra_left_right = 10;
    uint32_t xtra_top_bottom = 10;

    /* y geht von 0 bis 799 */
    /* x geht von 0 bis 479 */

    x0 = (int16_t)TIident[forThisIdentID].coord[0];
    x1 = (int16_t)TIident[forThisIdentID].coord[1];
    y0 = (int16_t)TIident[forThisIdentID].coord[2];
    y1 = y0 + (int16_t)TIident[forThisIdentID].fontUsed->height;

    if(((int16_t)TIident[forThisIdentID].fontUsed->height) > 70)
    {
         xtra_left_right = 10;
         xtra_top_bottom = 10;
    }
    else
    {
         xtra_left_right = 10;
         xtra_top_bottom = 0;
    }

    x0 -= xtra_left_right;
    x1 += xtra_left_right;
    y0 -= xtra_top_bottom;
    y1 += xtra_top_bottom;

    GFX_SetWindowLayer0(tIcursor.FBStartAdress, x0, x1, y0, y1);
}


void tInfo_write_label_var(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text)
{
    GFX_DrawCfgWindow	hgfx;

    if(XrightGimpStyle > 799)
        XrightGimpStyle = 799;
    if(XleftGimpStyle >= XrightGimpStyle)
        XleftGimpStyle = 0;
    if(YtopGimpStyle > 479)
        YtopGimpStyle = 479;
    hgfx.Image = &tIscreen;
    hgfx.WindowNumberOfTextLines = 1;
    hgfx.WindowLineSpacing = 0;
    hgfx.WindowTab = 0;
    hgfx.WindowX0 = XleftGimpStyle;
    hgfx.WindowX1 = XrightGimpStyle;
    hgfx.WindowY1 = 479 - YtopGimpStyle;
    if(hgfx.WindowY1 < Font->height)
        hgfx.WindowY0 = 0;
    else
        hgfx.WindowY0 = hgfx.WindowY1 - Font->height;

    GFX_write_label(Font, &hgfx, text, infoColor);
}


void tInfo_write_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text, uint8_t color)
{
    GFX_DrawCfgWindow	hgfx;

    if(XrightGimpStyle > 799)
        XrightGimpStyle = 799;
    if(XleftGimpStyle >= XrightGimpStyle)
        XleftGimpStyle = 0;
    if(YtopGimpStyle > 479)
        YtopGimpStyle = 479;
    hgfx.Image = &tIscreen;
    hgfx.WindowNumberOfTextLines = 1;
    hgfx.WindowLineSpacing = 0;
    hgfx.WindowTab = 0;
    hgfx.WindowX0 = XleftGimpStyle;
    hgfx.WindowX1 = XrightGimpStyle;
    hgfx.WindowY1 = 479 - YtopGimpStyle;
    if(hgfx.WindowY1 < Font->height)
        hgfx.WindowY0 = 0;
    else
        hgfx.WindowY0 = hgfx.WindowY1 - Font->height;

    GFX_write_label(Font, &hgfx, text, color);
}


void tInfo_write_label_fix(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char textId)
{
    char text[2];

    text[0] = textId;
    text[1] = 0;

    tInfo_write_label_var(XleftGimpStyle, XrightGimpStyle, YtopGimpStyle, Font, text);
}


void tI_clean_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font)
{
    GFX_DrawCfgWindow	hgfx;

    if(XrightGimpStyle > 799)
        XrightGimpStyle = 799;
    if(XleftGimpStyle >= XrightGimpStyle)
        XleftGimpStyle = 0;
    if(YtopGimpStyle > 479)
        YtopGimpStyle = 479;
    hgfx.Image = &tIscreen;
    hgfx.WindowX0 = XleftGimpStyle;
    hgfx.WindowX1 = XrightGimpStyle;
    hgfx.WindowY1 = 479 - YtopGimpStyle;
    if(hgfx.WindowY1 < Font->height)
        hgfx.WindowY0 = 0;
    else
        hgfx.WindowY0 = hgfx.WindowY1 - Font->height;

    GFX_clear_window_immediately(&hgfx);
}


void tInfo_write_buttonTextline(GFX_DrawCfgScreen *screenPtr, uint8_t left2ByteCode, char middle2ByteCode, char right2ByteCode)
{
    GFX_clean_area(&tIscreen, 0, 800, 480-24,480);

    char localtext[32];

    if(left2ByteCode)
    {
        localtext[0] = TXT_2BYTE;
        localtext[1] = left2ByteCode;
        localtext[2] = 0;
        write_content_simple(screenPtr, 0, 800, 480-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }

    if(middle2ByteCode)
    {
        localtext[0] = '\001';
        localtext[1] = TXT_2BYTE;
        localtext[2] = middle2ByteCode;
        localtext[3] = 0;
        write_content_simple(screenPtr, 0, 800, 480-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }

    if(right2ByteCode)
    {
        localtext[0] = '\002';
        localtext[1] = TXT_2BYTE;
        localtext[2] = right2ByteCode;
        localtext[3] = 0;
        write_content_simple(screenPtr, 0, 800, 480-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }
}
