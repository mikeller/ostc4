///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEdit.c
/// \brief  Main Template file for Menu Setting Modifications
/// \author heinrichs weikamp gmbh
/// \date   04-July-2014
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
#include <stdlib.h>

#include "tMenuEdit.h"

#include "externLogbookFlash.h"
#include "gfx_fonts.h"
#include "tHome.h"
#include "tInfoCompass.h"
#include "tInfoSensor.h"
#include "tInfoPreDive.h"
#include "tMenuEditHardware.h"
#include "tMenuEditPlanner.h"
#include "tMenuEditSystem.h"
#include "tMenuEditXtra.h"
#include "tMenuEditCustom.h"

/* Private types -------------------------------------------------------------*/
#define TEXTSIZE 16

typedef struct
{
    uint32_t pEventFunction;
    uint32_t callerID;
} SEventHandler;

typedef struct
{
    uint32_t pEventFunction;
    uint8_t functionParameter;
    uint8_t line;
} SBackMenuHandler;

typedef struct
{
    char orgText[32];
    char newText[32];
    uint32_t input[4];
    uint16_t coord[3];
    int8_t begin[4], size[4];
    tFont *fontUsed;
    uint32_t callerID;
    uint8_t maintype;
    uint8_t subtype;
} SEditIdent;

    typedef enum
{
  FIELD_NUMBERS = 0,
    FIELD_BUTTON,
    FIELD_SELECT,
    FIELD_SYMBOL,
    FIELD_TOGGLE,
    FIELD_ON_OFF,
    FIELD_UDIGIT,
    FIELD_2DIGIT,
    FIELD_3DIGIT,
    FIELD_SDIGIT,
    FIELD_FLOAT,
    FIELD_END
} SField;

/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	tMEscreen;
GFX_DrawCfgScreen	tMEcursor;
GFX_DrawCfgScreen	tMEcursorNew;

static uint32_t menuID;
static uint8_t menuColor;

static int8_t id = 0;
static int8_t actualId = 0;
static int8_t idLast = -1;
static SEditIdent ident[10];
static int8_t tME_stop = 0;

static int8_t evid = 0;
static int8_t actualevid = 0;
static int8_t evidLast = -1;
static SEventHandler event[10];

static SBackMenuHandler backmenu;

static int8_t block = 0;
static int8_t subBlockPosition = 0;

static _Bool EnterPressedBeforeButtonAction = 0;
static _Bool EnterPressed = 0;

static _Bool WriteSettings = 0;

/* Private function prototypes -----------------------------------------------*/
static void create_newText_for_Id(int8_t localId);
void clean_content_of_Id(int8_t localId);
static void write_content_of_Id(int8_t localId);

void draw_tMEdesign(void);
void set_cursorNew(uint8_t forThisIdentID);
void startMenuEditFieldSelect(void);
void create_newText_for_actual_Id(void);
void write_content_of_actual_Id(void);
void clean_content_of_actual_Id(void);
void write_content_without_Id(void);

void nextMenuEditFieldDigit(void);
void upMenuEditFieldDigit(void);
void downMenuEditFieldDigit(void);

void draw_tMEcursorNewDesign(void);

uint8_t split_Content_to_Digit_helper(uint8_t inContentAscii, uint8_t *outDigit100, uint8_t *outDigit10, uint8_t *outDigit1);

/* Exported functions --------------------------------------------------------*/

void tMenuEdit_init(void)
{
    tMEcursor.FBStartAdress = getFrame(7);
    tMEcursor.ImageHeight = 480;
    tMEcursor.ImageWidth = 800;
    tMEcursor.LayerIndex = 0;

    GFX_fill_buffer(tMEcursor.FBStartAdress, 0xFF, CLUT_MenuEditCursor);

    tMEcursorNew.FBStartAdress = getFrame(8);
    tMEcursorNew.ImageHeight = 390;
    tMEcursorNew.ImageWidth = 800;
    tMEcursorNew.LayerIndex = 0;

    draw_tMEcursorNewDesign();
}

void stop_cursor_fields(void)
{
    tME_stop = 1;
}

void resetMenuContentStructure()
{
    id = 0;
    idLast = -1;
    evid = 0;
    evidLast = -1;
}

void resetMenuEdit(uint8_t color)
{
    id = 0;
    actualId = 0;
    idLast = -1;
    evid = 0;
    actualevid = 0;
    evidLast = -1;
    tME_stop = 0;
    EnterPressed = 0;
    EnterPressedBeforeButtonAction = 0;

    setBackMenu(0,0,0);

    releaseFrame(9,tMEscreen.FBStartAdress);

    tMEscreen.FBStartAdress = getFrame(9);
    tMEscreen.ImageHeight = 480;
    tMEscreen.ImageWidth = 800;
    tMEscreen.LayerIndex = 1;

/*
    write_content_simple(&tMEscreen, 		   0,  38,      0, &Awe48,"x",CLUT_ButtonSymbols);
    write_content_simple(&tMEscreen,	800-46, 800,      0, &Awe48,"u",CLUT_ButtonSymbols);
    write_content_simple(&tMEscreen,			 0,  45, 480-45, &Awe48,"d",CLUT_ButtonSymbols);
    write_content_simple(&tMEscreen,	800-48, 800, 480-45, &Awe48,"e",CLUT_ButtonSymbols);
*/
    menuID = get_globalState();

    menuColor = color;

    /* set cursor to first line */
    uint8_t line = 1;
    GFX_SetFrameTop(tMEscreen.FBStartAdress);
    if(!settingsGetPointer()->FlipDisplay)
    {
    	GFX_SetFrameBottom((tMEcursorNew.FBStartAdress) + 65*2*(line - 1), 0, 25, 800, 390);
    }
    else
    {
    	GFX_SetFrameBottom((tMEcursorNew.FBStartAdress)+ (390 - 65 *(line)) *2, 0, 480-390-25, 800, 390);
    }
}


void tMenuEdit_refresh_live_content(void)
{
	uint32_t globState = get_globalState();
	void (*refreshFct)() = NULL;


	 switch(globState)
	 {
	 	 case (StMHARD3_O2_Sensor1):
	 	 case (StMHARD3_O2_Sensor1 & MaskFieldDigit):
	 	 case (StMHARD3_O2_Calibrate):
	 	 case (StMHARD3_Sensor_Detect):
	 	 case (StMHARD3_Sensor_Info):							refreshFct = refresh_O2Sensors;
	 	 	 break;
		 case (StMHARD2_Compass & MaskFieldDigit):
             refreshFct = refresh_CompassEdit;
			 break;
		 case (StMXTRA_CompassHeading & MaskFieldDigit):
             refreshFct = refresh_CompassHeading;
			 break;
	 	 case (StMSYS4_Info & MaskFieldDigit): refreshFct = &refresh_InformationPage;
	 	 	 break;
	 	 case (StMPLAN5_ExitResult & MaskFieldDigit): refreshFct = refresh_PlanResult;
	 		 break;
	 	 case (StMHARD5_Button1 & MaskFieldDigit): // will not be executed in EditFieldMode as global state is different
						refreshFct = refresh_ButtonValuesFromPIC;
	 	 	 break;
	 	 case StMSYS1_DateTime: refreshFct = refresh_DateTime;
		 	 break;
	 	 case (StMSYS3_Units & MaskFieldDigit): refreshFct = refresh_Design;
	 	 	 break;
	 	 case (StMCustom1_CViewTimeout & MaskFieldDigit):refreshFct = refresh_Customviews;
	 	 	 break;
	 	 case (StMCustom4_CViewSelection1 & MaskFieldDigit):
	 	 case (StMCustom3_CViewSelection1 & MaskFieldDigit):
	 	 case StMCustom3_CViewSelection2:
	 	 case StMCustom3_CViewSelection3:
	 	 case StMCustom3_CViewSelection4:
	 	 case StMCustom3_CViewSelection5:
	 	 case StMCustom3_CViewSelection6: refreshFct = CustomviewDivemode_refresh;
	 	 	 break;
#ifdef ENABLE_MOTION_CONTROL
	 	 case (StMCustom5_CViewPortCalib & MaskFieldDigit):
	 	 case StMCustom5_CViewPortLayout:
	 	 case StMCustom5_CViewPortAmbient: refreshFct = refresh_ViewPort;
	 		 break;
#endif
	 	 default:	 /* no menu has been updated */
	 		 break;
	 }

	 if(refreshFct != NULL)
	 {
			uint32_t rememberPage = tMEscreen.FBStartAdress;
			tMEscreen.FBStartAdress = getFrame(9);

			refreshFct();

	        GFX_SetFrameTop(tMEscreen.FBStartAdress);
	        releaseFrame(9,rememberPage);
	 }
}

void tMenuEdit_writeSettingsToFlash(void)
{
    if(WriteSettings)
    {
    	reset_SettingWarning();
        GFX_logoAutoOff();
        ext_flash_write_settings(0);
        WriteSettings = 0;
    }
}

void helperLeaveMenuEditField(uint8_t idID)
{
    if(ident[idID].maintype == FIELD_NUMBERS)
    {
        change_CLUT_entry((CLUT_MenuEditField0 + idID), CLUT_MenuEditFieldRegular);
    }
}


void helperGotoMenuEditField(uint8_t idID)
{
/*
    if(ident[idID].maintype == FIELD_NUMBERS)
    {
        change_CLUT_entry((CLUT_MenuEditField0 + idID), CLUT_MenuEditFieldSelected);
    }
*/
    set_cursorNew(idID);
//	set_cursor(idID);
}


void exitMenuEdit_to_BackMenu(void)
{
    _Bool EnterPressedBackup = EnterPressed;

    if(backmenu.pEventFunction)
    {
        ((void (*)(uint8_t))(backmenu.pEventFunction))(backmenu.functionParameter);

        EnterPressed = EnterPressedBackup;
//		if(backmenu.line > 1)
//			helperGotoMenuEditField(backmenu.line);
    }
}


void exitMenuEdit_to_Menu_with_Menu_Update(void)
{
    EnterPressed = 1;
    exitMenuEdit(1);
}


void exitMenuEdit_to_Menu_with_Menu_Update_do_not_write_settings_for_this_only(void)
{
    EnterPressed = 1;
    exitMenuEdit(0);
}


void exitMenuEdit_to_Home_with_Menu_Update(void)
{
    EnterPressed = 1;
    exitMenuEdit(1);
    set_globalState_tHome();
}


void exitMenuEdit_to_InfoCompassCalibration(void)
{
    exitMenuEdit(1);
    openInfo_Compass();
}

void exitMenuEdit_to_InfoSensor(void)
{
	uint32_t globState = get_globalState();

    exitMenuEdit(1);
	switch(globState)
	{
	 	 case (StMHARD3_O2_Sensor3): openInfo_Sensor(2);
			break;
	 	 case (StMHARD3_O2_Sensor2): openInfo_Sensor(1);
	 	 	 break;
	 	 default:
	 	 case (StMHARD3_O2_Sensor1): openInfo_Sensor(0);
	 	 	 break;
	}
}


void exitMenuEdit_to_Home(void)
{
    exitMenuEdit(1);
    set_globalState_tHome();
}


void exitMenuEdit(uint8_t writeSettingsIfEnterPressed)
{
    openMenu(0);
    if(EnterPressed)
    {
        updateMenu();
        if((stateUsed->mode == MODE_SURFACE) && writeSettingsIfEnterPressed)
            WriteSettings = 1;
    }
    releaseFrame(9,tMEscreen.FBStartAdress);
}


void exitMenuEditBackMenuOption(void)
{
    if(backmenu.pEventFunction == 0)
        exitMenuEdit(1);
    else
        exitMenuEdit_to_BackMenu();
}


void startMenuEditFieldSelect(void)
{
    actualId = 0;
    helperGotoMenuEditField(actualId);
}


void nextMenuEditField(void)
{
    helperLeaveMenuEditField(actualId);

    if(actualId < idLast)
    	actualId++;
    else
    	actualId = 0;
    helperGotoMenuEditField(actualId);
}

/*
void previousMenuEditField(void)
{
    helperLeaveMenuEditField(id);
    if(id > 0)
        id--;
    else
        id = idLast;
    helperGotoMenuEditField(id);
}
*/

_Bool inc_subBlock_or_block_of_actual_id(void)
{
    if(event[actualevid].callerID != ident[actualId].callerID)
        return 0;

    if((ident[actualId].subtype != FIELD_3DIGIT) && (ident[actualId].subtype != FIELD_2DIGIT) && ((subBlockPosition + 1) < ident[actualId].size[block]))
    {
        subBlockPosition++;
        return 1;
    }

    if(((block + 1) < 4) && (ident[actualId].size[block+1] > 0))
    {
        block++;
        subBlockPosition = 0;
        return 1;
    }

    return 0;
}


uint8_t get_newContent_of_actual_id_block_and_subBlock(uint8_t action)
{
    uint8_t (*onActionFunc)(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t);
    uint8_t content;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return 0;

    onActionFunc = (uint8_t (*)(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t))(event[actualevid].pEventFunction);

    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_3DIGIT))
    {
        content  = 100 * (	ident[actualId].newText[ident[actualId].begin[block] + 0] - '0');
        content +=  10 * (	ident[actualId].newText[ident[actualId].begin[block] + 1] - '0');
        content += 				ident[actualId].newText[ident[actualId].begin[block] + 2];
    }
    else
    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_2DIGIT))
    {
        content = 10 * (ident[actualId].newText[ident[actualId].begin[block] + 0] - '0');
        content += ident[actualId].newText[ident[actualId].begin[block] + 1];
    }
    else
    if(ident[actualId].maintype == FIELD_NUMBERS)
        content = ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition];
    else
    if((ident[actualId].maintype == FIELD_ON_OFF) || (ident[actualId].maintype == FIELD_TOGGLE))
        content = ident[actualId].input[block];
    else
        content = 0; /* just a default for protection */

    return onActionFunc(ident[actualId].callerID, block, subBlockPosition, content, action);
}

void mark_digit_of_actual_id_with_this_block_and_subBlock(int8_t oldblock, int8_t oldsubblockpos)
{
    char oneCharText[2];
    uint16_t positionOffset;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    if(ident[actualId].maintype == FIELD_NUMBERS)
    {
        oneCharText[0] = ident[actualId].newText[ident[actualId].begin[oldblock] + oldsubblockpos];
        oneCharText[1] = 0;
        positionOffset = GFX_return_offset(ident[actualId].fontUsed, ident[actualId].newText, ident[actualId].begin[oldblock] + oldsubblockpos);
        write_content( ident[actualId].coord[0] + positionOffset, ident[actualId].coord[1], ident[actualId].coord[2], ident[actualId].fontUsed, oneCharText, CLUT_MenuEditFieldSelected);
    }
}


void mark_new_2digit_of_actual_id_block(void)
{
    char oneCharText[3];
    uint16_t positionOffset;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    if(ident[actualId].maintype == FIELD_NUMBERS)
    {
        oneCharText[0] = ident[actualId].newText[ident[actualId].begin[block] + 0];
        oneCharText[1] = ident[actualId].newText[ident[actualId].begin[block] + 1];
        oneCharText[2] = 0;
        positionOffset = GFX_return_offset(ident[actualId].fontUsed, ident[actualId].newText, ident[actualId].begin[block] + 0);
        write_content( ident[actualId].coord[0] + positionOffset, ident[actualId].coord[1], ident[actualId].coord[2], ident[actualId].fontUsed, oneCharText, CLUT_MenuEditDigit);
    }
}


void mark_new_3digit_of_actual_id_block(void)
{
    char oneCharText[4];
    uint16_t positionOffset;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    if(ident[actualId].maintype == FIELD_NUMBERS)
    {
        oneCharText[0] = ident[actualId].newText[ident[actualId].begin[block] + 0];
        oneCharText[1] = ident[actualId].newText[ident[actualId].begin[block] + 1];
        oneCharText[2] = ident[actualId].newText[ident[actualId].begin[block] + 2];
        oneCharText[3] = 0;
        positionOffset = GFX_return_offset(ident[actualId].fontUsed, ident[actualId].newText, ident[actualId].begin[block] + 0);
        write_content( ident[actualId].coord[0] + positionOffset, ident[actualId].coord[1], ident[actualId].coord[2], ident[actualId].fontUsed, oneCharText, CLUT_MenuEditDigit);
    }
}


void mark_new_digit_of_actual_id_block_and_subBlock(void)
{
    char oneCharText[2];
    uint16_t positionOffset;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    if(ident[actualId].maintype == FIELD_NUMBERS)
    {
        oneCharText[0] = ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition];
        oneCharText[1] = 0;
        positionOffset = GFX_return_offset(ident[actualId].fontUsed, ident[actualId].newText, ident[actualId].begin[block] + subBlockPosition);
        write_content( ident[actualId].coord[0] + positionOffset, ident[actualId].coord[1], ident[actualId].coord[2], ident[actualId].fontUsed, oneCharText, CLUT_MenuEditDigit);
    }
}


void enterMenuEditField(void)
{
    uint8_t newContent;
    uint8_t digit100;
    uint8_t digit10;
    uint8_t digit1;

    actualevid = 0;
    while((actualevid < evidLast) && (event[actualevid].callerID != ident[actualId].callerID))
    {
        actualevid++;
    }

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    set_globalState(event[actualevid].callerID);
    block = 0;
    subBlockPosition = 0;

    if(ident[actualId].maintype == FIELD_NUMBERS)
    {
        change_CLUT_entry(CLUT_MenuEditLineSelected, CLUT_MenuEditCursor);
        // old stuff? hw 150916, reactivated 150923, this shows which digit will be changed now as it marks the other grey/black
        // now fixed for button settings with newContent <= '0'+99 condition
        change_CLUT_entry((CLUT_MenuEditField0 + actualId), CLUT_MenuEditFieldSelected);
    }
    if(ident[actualId].maintype == FIELD_TOGGLE)
    {
        change_CLUT_entry(CLUT_MenuEditLineSelected, CLUT_MenuEditCursor);
    }


    newContent = get_newContent_of_actual_id_block_and_subBlock(ACTION_BUTTON_ENTER);

    if (((newContent == UPDATE_DIVESETTINGS) || (newContent == UPDATE_AND_EXIT_TO_HOME) || (newContent == UPDATE_AND_EXIT_TO_MENU)) && (actual_menu_content == MENU_SURFACE))
        createDiveSettings();

    if(newContent == EXIT_TO_MENU_WITH_LOGO)
    {
        GFX_logoAutoOff();
    }

    if((newContent == EXIT_TO_MENU) || (newContent == UPDATE_AND_EXIT_TO_MENU) || (newContent == EXIT_TO_MENU_WITH_LOGO))
    {
        if(backmenu.pEventFunction == 0)
            exitMenuEdit(1);
        else
            exitMenuEdit_to_BackMenu();
        return;
    }

    if((newContent == EXIT_TO_HOME) || (newContent == UPDATE_AND_EXIT_TO_HOME))
    {
        exitMenuEdit_to_Home();
        return;
    }

    if(newContent == EXIT_TO_INFO_COMPASS)
    {
        exitMenuEdit_to_InfoCompassCalibration();
        return;
    }

    if(newContent == EXIT_TO_INFO_SENSOR)
    {
        exitMenuEdit_to_InfoSensor();
        return;
    }

    switch(ident[actualId].maintype)
    {
    case FIELD_NUMBERS:
        write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

        switch (ident[actualId].subtype) {
        case FIELD_UDIGIT:
            if((newContent >= '0') && (newContent <= '9'))
                ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition] = newContent;

            mark_new_digit_of_actual_id_block_and_subBlock();

            break;
        case FIELD_3DIGIT:
            if((newContent >= '0') && (newContent <= '0'+200))
            {
                split_Content_to_Digit_helper( newContent, &digit100, &digit10, &digit1);
                ident[actualId].newText[ident[actualId].begin[block] + 0] = '0' + digit100;
                ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + digit10;
                ident[actualId].newText[ident[actualId].begin[block] + 2] = '0' + digit1;
                mark_new_3digit_of_actual_id_block();
            }

            break;
        case FIELD_2DIGIT:
            if((newContent >= '0') && (newContent <= '0'+99))
            {
                ident[actualId].newText[ident[actualId].begin[block]] = '0' + (newContent - '0')/10;
                ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + ((newContent - '0') - (10*((newContent - '0')/10)));
                mark_new_2digit_of_actual_id_block();
            }

            break;
        case FIELD_SDIGIT:
            if ((subBlockPosition == 0 && (newContent == '+' || newContent == '-')) || (subBlockPosition > 0 && newContent >= '0' && newContent <= '9')) {
                ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition] = newContent;
            }

            mark_new_digit_of_actual_id_block_and_subBlock();

            break;
        }
        break;
    case FIELD_BUTTON:
    case FIELD_ON_OFF:
        set_globalState(menuID);

        break;
    case FIELD_SYMBOL:
        ident[actualId].input[0] += 1;
        if(ident[actualId].input[0] >= ident[actualId].input[1])
            ident[actualId].input[0] = 0;
        ident[actualId].newText[0] = ident[actualId].orgText[ident[actualId].input[0]];
        write_content_of_actual_Id();
        set_globalState(menuID);
        break;
    case FIELD_SELECT:
        write_buttonTextline(TXT2BYTE_ButtonMinus, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonPlus);

        break;
    }
}


void exitMenuEditField(void)
{
    uint8_t newContent;

    set_globalState(menuID);

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    newContent = get_newContent_of_actual_id_block_and_subBlock(ACTION_TIMEOUT);

/*
    uint8_t (*onActionFunc)(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t);
    uint8_t newContent;

    onActionFunc = (uint8_t (*)(uint32_t, uint8_t, uint8_t, uint8_t, uint8_t))(event[evid].pEventFunction);

    newContent = onActionFunc(ident[id].callerID, 0, 0, 255, ACTION_BUTTON_BACK);
*/

    /* destroy changes of editing in newText */
    change_CLUT_entry(CLUT_MenuEditLineSelected, CLUT_MenuLineSelected);
    change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditFieldRegular);

    create_newText_for_actual_Id();
    write_content_of_actual_Id();

    if((newContent == EXIT_TO_MENU) || (newContent == UPDATE_AND_EXIT_TO_MENU))
    {
        exitMenuEdit(1);
        return;
    }

    if((newContent == EXIT_TO_HOME) || (newContent == UPDATE_AND_EXIT_TO_HOME))
    {
        exitMenuEdit_to_Home();
        return;
    }

}


static void startNextEdit(void)
{
    EnterPressed = 1;
    nextMenuEditField();
    enterMenuEditField();
}


void nextMenuEditFieldDigit(void)
{
    uint8_t action;
    uint8_t newContent;
    int8_t blockOld = 0;
    int8_t subBlockPositionOld = 0;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    blockOld = block;
    subBlockPositionOld = subBlockPosition;

    if(inc_subBlock_or_block_of_actual_id())
        action = ACTION_BUTTON_ENTER;
    else
        action = ACTION_BUTTON_ENTER_FINAL;

    newContent = get_newContent_of_actual_id_block_and_subBlock(action);

    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);

        change_CLUT_entry(CLUT_MenuEditLineSelected, CLUT_MenuLineSelected);
        for(int i = 0;i<=9;i++)
            change_CLUT_entry((CLUT_MenuEditField0 + i), CLUT_MenuEditFieldRegular);

        if(((newContent == UPDATE_DIVESETTINGS) || (newContent == UPDATE_AND_EXIT_TO_HOME) || (newContent == UPDATE_AND_EXIT_TO_MENU)) && (actual_menu_content == MENU_SURFACE))
        createDiveSettings();

        if((newContent == EXIT_TO_MENU) || (newContent == UPDATE_AND_EXIT_TO_MENU))
        {
            exitMenuEdit(1);
            return;
        }

        if((newContent == EXIT_TO_HOME) || (newContent == UPDATE_AND_EXIT_TO_HOME))
        {
            exitMenuEdit_to_Home();
            return;
        }

        if (newContent == EXIT_TO_NEXT_MENU) {
            startNextEdit();

            return;
        }
    }

    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_3DIGIT) && (action == ACTION_BUTTON_ENTER) &&(newContent >= '0') && (newContent <= '0' + 99))
    {
        ident[actualId].newText[ident[actualId].begin[block] + 0] = '0' + (newContent - '0')/100;
        ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + (newContent - '0')/10;
        ident[actualId].newText[ident[actualId].begin[block] + 2] = '0' + ((newContent - '0') - (10*((newContent - '0')/10)));
    }
    else
    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_2DIGIT) && (action == ACTION_BUTTON_ENTER) &&(newContent >= '0') && (newContent <= '0' + 99))
    {
        ident[actualId].newText[ident[actualId].begin[block] + 0] = '0' + (newContent - '0')/10;
        ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + ((newContent - '0') - (10*((newContent - '0')/10)));
    } else if (ident[actualId].maintype == FIELD_NUMBERS && ident[actualId].subtype == FIELD_SDIGIT && action == ACTION_BUTTON_ENTER && subBlockPosition == 0) {
        if (newContent == '+' || newContent == '-') {
            ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition] = newContent;
        }
    }
    else
    if((ident[actualId].maintype == FIELD_NUMBERS) && (action == ACTION_BUTTON_ENTER) && (newContent >= '0') && (newContent <= '9'))
        ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition] = newContent;

    if(action == ACTION_BUTTON_ENTER)
    {
        if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_3DIGIT))
        {
            mark_new_3digit_of_actual_id_block();
            mark_digit_of_actual_id_with_this_block_and_subBlock(blockOld,0);
            mark_digit_of_actual_id_with_this_block_and_subBlock(blockOld,1);
            mark_digit_of_actual_id_with_this_block_and_subBlock(blockOld,2);
        }
        else
        if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_2DIGIT))
        {
            mark_new_2digit_of_actual_id_block();
            mark_digit_of_actual_id_with_this_block_and_subBlock(blockOld,0);
            mark_digit_of_actual_id_with_this_block_and_subBlock(blockOld,1);
        }
        else
        {
            mark_new_digit_of_actual_id_block_and_subBlock();
            mark_digit_of_actual_id_with_this_block_and_subBlock(blockOld,subBlockPositionOld);
        }
    }
    else /* action == ACTION_BUTTON_ENTER_FINAL */
        set_globalState(menuID);
}

uint8_t split_Content_to_Digit_helper(uint8_t inContentAscii, uint8_t *outDigit100, uint8_t *outDigit10, uint8_t *outDigit1)
{
    uint8_t newContent, tempDigit, CopyContent;

    newContent = inContentAscii - '0';
    CopyContent = newContent;

    tempDigit = newContent / 100;
    newContent -= tempDigit * 100;
    if(outDigit100)
        *outDigit100 = tempDigit;

    tempDigit = newContent / 10;
    newContent -= tempDigit * 10;
    if(outDigit10)
        *outDigit10 = tempDigit;

    tempDigit = newContent;
    if(outDigit1)
        *outDigit1 = tempDigit;

    return CopyContent;
}


static void checkUpdateSDigit(uint8_t newContent)
{
    if ((subBlockPosition == 0 && (newContent == '+' || newContent == '-')) || (subBlockPosition > 0 && newContent >= '0' && newContent <= '9')) {
        ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition] = newContent;
    }
}


void upMenuEditFieldDigit(void)
{
    uint8_t newContent;
    uint8_t digit100;
    uint8_t digit10;
    uint8_t digit1;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    newContent = get_newContent_of_actual_id_block_and_subBlock(ACTION_BUTTON_NEXT);

    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_3DIGIT) &&(newContent >= '0') && (newContent <= '0' + 200))
    {
        split_Content_to_Digit_helper( newContent, &digit100, &digit10, &digit1);
        ident[actualId].newText[ident[actualId].begin[block] + 0] = '0' + digit100;
        ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + digit10;
        ident[actualId].newText[ident[actualId].begin[block] + 2] = '0' + digit1;
        mark_new_3digit_of_actual_id_block();
        return;
    }

    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_2DIGIT) &&(newContent >= '0') && (newContent <= '0' + 99))
    {
        ident[actualId].newText[ident[actualId].begin[block] + 0] = '0' + (newContent - '0')/10;
        ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + ((newContent - '0') - (10*((newContent - '0')/10)));
        mark_new_2digit_of_actual_id_block();
        return;
    }

    if (ident[actualId].maintype == FIELD_NUMBERS && ident[actualId].subtype == FIELD_SDIGIT) {
        checkUpdateSDigit(newContent);
    } else if (ident[actualId].maintype == FIELD_NUMBERS && newContent >= '0' && newContent <= '9') {
        ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition] = newContent;
    }

    mark_new_digit_of_actual_id_block_and_subBlock();
}


void downMenuEditFieldDigit(void)
{
    uint8_t newContent;
    uint8_t digit100;
    uint8_t digit10;
    uint8_t digit1;

    if(event[actualevid].callerID != ident[actualId].callerID)
        return;

    newContent = get_newContent_of_actual_id_block_and_subBlock(ACTION_BUTTON_BACK);

    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_3DIGIT) &&(newContent >= '0') && (newContent <= '0' + 200))
    {
        split_Content_to_Digit_helper( newContent, &digit100, &digit10, &digit1);
        ident[actualId].newText[ident[actualId].begin[block] + 0] = '0' + digit100;
        ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + digit10;
        ident[actualId].newText[ident[actualId].begin[block] + 2] = '0' + digit1;
        mark_new_3digit_of_actual_id_block();
        return;
    }

    if((ident[actualId].maintype == FIELD_NUMBERS) && (ident[actualId].subtype == FIELD_2DIGIT) &&(newContent >= '0') && (newContent <= '0' + 99))
    {
        ident[actualId].newText[ident[actualId].begin[block] + 0] = '0' + (newContent - '0')/10;
        ident[actualId].newText[ident[actualId].begin[block] + 1] = '0' + ((newContent - '0') - (10*((newContent - '0')/10)));
        mark_new_2digit_of_actual_id_block();
        return;
    }

    if (ident[actualId].maintype == FIELD_NUMBERS && ident[actualId].subtype == FIELD_SDIGIT) {
        checkUpdateSDigit(newContent);
    } else if (ident[actualId].maintype == FIELD_NUMBERS && newContent >= '0' && newContent <= '9') {
        ident[actualId].newText[ident[actualId].begin[block] + subBlockPosition] = newContent;
    }

    mark_new_digit_of_actual_id_block_and_subBlock();
}


void evaluateNewString(uint32_t editID, uint32_t *pNewValue1, uint32_t *pNewValue2, uint32_t *pNewValue3, uint32_t *pNewValue4)
{
    if(editID != ident[actualId].callerID)
        return;

    bool isSigned = ident[actualId].maintype == FIELD_NUMBERS && ident[actualId].subtype == FIELD_SDIGIT;

    uint8_t i, digitCount, digit;
    uint32_t sum[4], multiplier;

    for(i=0;i<4;i++)
        sum[i] = 0;

    i = 0;
    while( ident[actualId].size[i] && (i < 4))
    {
        multiplier = 1;
        for(digitCount = 1; digitCount < ident[actualId].size[i]; digitCount++)
            multiplier *= 10;

        bool isNegative = false;
        for(digitCount = 0; digitCount < ident[actualId].size[i]; digitCount++)
        {
            digit = ident[actualId].newText[ident[actualId].begin[i] + digitCount];

            if (isSigned && digitCount == 0) {
                if (digit == '-') {
                    isNegative = true;
                }
            } else {
                if(digit > '0')
                    digit -= '0';
                else
                    digit = 0;

                if(digit > 9)
                    digit = 9;

                sum[i] += digit * multiplier;
            }

            if(multiplier >= 10)
                multiplier /= 10;
            else
                multiplier = 0;
        }

        if (isSigned) {
            int32_t value = sum[i];
            if (isNegative) {
                value = -value;
            }
            sum[i] = ((input_u)value).uint32;
        }

        i++;
    }

    *pNewValue1 = sum[0];
    *pNewValue2 = sum[1];
    *pNewValue3 = sum[2];
    *pNewValue4 = sum[3];
}


uint8_t get_id_of(uint32_t editID)
{
    uint8_t temp_id;

    if(editID == ident[actualId].callerID)
        return actualId;
    else
    {
        temp_id = 0;
        while((temp_id < 9) && (editID != ident[temp_id].callerID))
            temp_id++;
        if(editID != ident[temp_id].callerID)
            temp_id = 255;
        return temp_id;
    }
}


void tMenuEdit_newButtonText(uint32_t editID, char *text)
{
    uint8_t backup_id, temp_id;

    temp_id = get_id_of(editID);
    if(temp_id == 255)
        return;

    backup_id = id;
    id = temp_id;

    strncpy(ident[id].newText, text, 32);
    ident[id].newText[31] = 0;

    clean_content_of_Id(id);
    write_content_of_Id(id);

    id = backup_id;
}


void tMenuEdit_set_on_off(uint32_t editID, uint32_t int1)
{
    uint8_t backup_id, temp_id;

    temp_id = get_id_of(editID);
    if(temp_id == 255)
        return;

    backup_id = id;
    id = temp_id;

    ident[id].input[0] = int1;

    if(int1)
        ident[id].newText[0] = '\005';
    else
        ident[id].newText[0] = '\006';

    clean_content_of_Id(id);
    write_content_of_Id(id);

    id = backup_id;
}

void tMenuEdit_select(uint32_t editID)
{
	uint8_t id_local = 0;
	id_local = get_id_of(editID);

	if(id_local <= idLast)
	{
		actualId = id_local;
		set_cursorNew(id_local);
	}
}

#if OLD_SELECTION
void tMenuEdit_select(uint32_t editID, uint32_t int1, uint32_t int2, uint32_t int3, uint32_t int4)
{
    if(int1 > 4)
        return;

    uint8_t backup_id, temp_id;

    temp_id = get_id_of(editID);
    if(temp_id == 255)
        return;

    backup_id = id;
    id = temp_id;

    ident[id].input[0] = int1;
    ident[id].input[1] = int1;
    ident[id].input[2] = int1;
    ident[id].input[3] = int1;

    create_newText_for_actual_Id();
    clean_content_of_actual_Id();
    write_content_of_actual_Id();

    id = backup_id;
}
#endif


void tMenuEdit_newInput(uint32_t editID, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4)
{
    uint8_t backup_id, temp_id;

    temp_id = get_id_of(editID);
    if(temp_id == 255)
        return;

    backup_id = id;
    id = temp_id;

    if(editID != ident[id].callerID)
    {
        temp_id = 0;
        while((temp_id < 9) && (editID != ident[temp_id].callerID))
            temp_id++;
        if(editID != ident[temp_id].callerID)
            return;
        id = temp_id;
    }
    ident[id].input[0] = int1;
    ident[id].input[1] = int2;
    ident[id].input[2] = int3;
    ident[id].input[3] = int4;

    create_newText_for_Id(id);
    if(id <= idLast)
        change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditFieldRegular);
    write_content_of_Id(id);

    id = backup_id;
}


void resetEnterPressedToStateBeforeButtonAction(void)
{
    EnterPressed = EnterPressedBeforeButtonAction;
}


void sendActionToMenuEdit(uint8_t sendAction)
{
    if(get_globalState() == menuID)
    {
        switch(sendAction)
        {
        case ACTION_BUTTON_ENTER:
            EnterPressedBeforeButtonAction = EnterPressed;
            EnterPressed = 1;
            enterMenuEditField();
            break;
        case ACTION_BUTTON_NEXT:
            nextMenuEditField();
//				previousMenuEditField();
            break;
        case ACTION_BUTTON_BACK:
            exitMenuEditBackMenuOption();
            break;
        case ACTION_TIMEOUT:
        case ACTION_MODE_CHANGE:
            exitMenuEdit(1);
            break;
        case ACTION_IDLE_TICK:
        case ACTION_IDLE_SECOND:
        default:
            break;

        }
    }
    else
    if(get_globalState() == event[actualevid].callerID)
    {
        switch(sendAction)
        {
        case ACTION_BUTTON_ENTER:
            nextMenuEditFieldDigit();
            break;
        case ACTION_BUTTON_NEXT:
            upMenuEditFieldDigit();
            break;
        case ACTION_BUTTON_BACK:
            downMenuEditFieldDigit();
            break;
        case ACTION_TIMEOUT:
        case ACTION_MODE_CHANGE:
            exitMenuEditField();
            break;
        case ACTION_IDLE_TICK:
        case ACTION_IDLE_SECOND:
            break;
        default:
            break;
        }
    }
    else
    {
        switch(sendAction)
        {
        case ACTION_BUTTON_ENTER:
            break;
        case ACTION_BUTTON_NEXT:
            break;
        case ACTION_BUTTON_BACK:
            break;
        case ACTION_TIMEOUT:
        case ACTION_MODE_CHANGE:
            exitMenuEdit(1);
            break;
        case ACTION_IDLE_TICK:
        case ACTION_IDLE_SECOND:
            break;
        default:
            break;
        }
    }
}


void create_newText_for_Id_and_field_select(int8_t localId)
{
    uint8_t i;

    i = 0;
    while( ident[localId].size[i] && (i < 4))
    {
        if(ident[localId].input[i])
            ident[localId].newText[ident[localId].begin[i]] = '\005';
        else
            ident[localId].newText[ident[localId].begin[i]] = '\006';
        i++;
    }
}

void create_newText_for_actual_Id_and_field_select(void)
{
	create_newText_for_Id_and_field_select(actualId);
}

void create_newText_for_Id(int8_t localId)
{
	bool isSigned = ident[localId].maintype == FIELD_NUMBERS && ident[localId].subtype == FIELD_SDIGIT;

	uint8_t i, digitCount;
	uint32_t remainder, digit, divider;
	i = 0;

    if(	ident[localId].maintype == FIELD_SELECT)
    {
        create_newText_for_Id_and_field_select(localId);
        return;
    }

	while( ident[localId].size[i] && (i < 4))
	{
		bool isNegative = false;
		if (isSigned) {
			int32_t value = ((input_u)ident[localId].input[i]).int32;
			if (value < 0) {
				isNegative = true;
			}
			remainder = abs(value);
		} else {
			remainder = ident[localId].input[i];
		}
		divider = 1;

		for(digitCount = 1; digitCount < ident[localId].size[i]; digitCount++)
			divider *= 10;

		for(digitCount = 0; digitCount < ident[localId].size[i]; digitCount++)
		{
			if (isSigned && digitCount == 0) {
				ident[localId].newText[ident[localId].begin[i] + digitCount] = isNegative ? '-' : '+';
			} else {
				digit = remainder	/ divider;
				remainder -= digit * divider;
				if(digit < 10)
					ident[localId].newText[ident[localId].begin[i] + digitCount] = digit + '0';
				else
					ident[localId].newText[ident[localId].begin[i] + digitCount] = 'x';
			}

			divider /= 10;
		}
		i++;
	}
}

void create_newText_for_actual_Id(void)
{
    create_newText_for_Id(actualId);
}


void write_content_without_Id(void)
{
    write_content( ident[id].coord[0], ident[id].coord[1], ident[id].coord[2], ident[id].fontUsed, ident[id].newText, CLUT_MenuEditFieldRegular);
}


void write_content_of_Id(int8_t localId)
{
    write_content( ident[localId].coord[0], ident[localId].coord[1], ident[localId].coord[2], ident[localId].fontUsed, ident[localId].newText, (CLUT_MenuEditField0 + localId));
}
void write_content_of_actual_Id(void)
{
	write_content_of_Id(actualId);
}

void clean_content_of_Id(int8_t localId)
{
    clean_content( ident[localId].coord[0], ident[localId].coord[1], ident[localId].coord[2], ident[localId].fontUsed);
}

void clean_content_of_actual_Id(void)
{
	clean_content_of_Id(actualId);
}

uint8_t togglePlusMinus(uint8_t input)
{
    if (input == '+') {
        return '-';
    } else {
        return '+';
    }
}

void write_field_udigit_and_2digit(uint8_t subtype, uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4)
{
    if(id >= 9)
        return;

    ident[id].maintype = FIELD_NUMBERS;
    ident[id].subtype  = subtype;

    ident[id].coord[0] = XleftGimpStyle;
    ident[id].coord[1] = XrightGimpStyle;
    ident[id].coord[2] = YtopGimpStyle;
    ident[id].fontUsed = (tFont *)Font;
    ident[id].callerID = editID;

    strncpy(ident[id].orgText, text, 32);
    strncpy(ident[id].newText, text, 32);
    ident[id].orgText[31] = 0;
    ident[id].newText[31] = 0;

    /* uint32_t has max 10 digits */

    int8_t beginTmp, sizeTmp;
    uint8_t i;

    ident[id].input[0] = int1;
    ident[id].input[1] = int2;
    ident[id].input[2] = int3;
    ident[id].input[3] = int4;

    for(i=0;i<4;i++)
        ident[id].size[i] = 0;

    beginTmp = 0;
    for(i=0;i<4;i++)
    {
        while((ident[id].orgText[beginTmp] != '#')&& ident[id].orgText[beginTmp])
            beginTmp++;

        if(ident[id].orgText[beginTmp] == '#')
        {
            sizeTmp = 1;
            while(ident[id].orgText[beginTmp + sizeTmp] == '#')
                sizeTmp++;

            ident[id].begin[i] = beginTmp;
            ident[id].size[i] = sizeTmp;
            beginTmp = ident[id].begin[i] + ident[id].size[i];
        }
        else
            break;
    }

    if(!tME_stop)
        change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditFieldRegular);
    else
        change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditInfo);

    create_newText_for_Id(id);

    if(editID == 0)
    	write_content_without_Id();
    else
    {
        write_content_of_Id(id);
        if(!tME_stop)
            idLast = id;
        id++;
    }
}

void write_field_udigit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4)
{
    write_field_udigit_and_2digit(FIELD_UDIGIT, editID,XleftGimpStyle,XrightGimpStyle,YtopGimpStyle,Font,text,int1,int2,int3,int4);
}

void write_field_2digit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4)
{
    write_field_udigit_and_2digit(FIELD_2DIGIT, editID,XleftGimpStyle,XrightGimpStyle,YtopGimpStyle,Font,text,int1,int2,int3,int4);
}

void write_field_3digit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4)
{
    write_field_udigit_and_2digit(FIELD_3DIGIT, editID,XleftGimpStyle,XrightGimpStyle,YtopGimpStyle,Font,text,int1,int2,int3,int4);
}


void write_field_sdigit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, int32_t int1, int32_t int2, int32_t int3, int32_t int4)
{
    write_field_udigit_and_2digit(FIELD_SDIGIT, editID, XleftGimpStyle, XrightGimpStyle, YtopGimpStyle, Font, text, ((input_u)int1).uint32, ((input_u)int2).uint32, ((input_u)int3).uint32, ((input_u)int4).uint32);
}


void write_field_select(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1,  uint8_t int2,  uint8_t int3,  uint8_t int4)
{
    if(id >= 9)
        return;

    int8_t beginTmp;

    ident[id].maintype = FIELD_SELECT;
    ident[id].subtype  = FIELD_SELECT;

    ident[id].coord[0] = XleftGimpStyle;
    ident[id].coord[1] = XrightGimpStyle;
    ident[id].coord[2] = YtopGimpStyle;
    ident[id].fontUsed = (tFont *)Font;
    ident[id].callerID = editID;

    strncpy(ident[id].orgText, text, 32);
    strncpy(ident[id].newText, text, 32);
    ident[id].orgText[31] = 0;
    ident[id].newText[31] = 0;

    ident[id].input[0] = int1;
    ident[id].input[1] = int2;
    ident[id].input[2] = int3;
    ident[id].input[3] = int4;

    for(int i=0;i<4;i++)
        ident[id].size[i] = 0;

    beginTmp = 0;
    for(int i=0;i<4;i++)
    {
        while((ident[id].orgText[beginTmp] != '#')&& ident[id].orgText[beginTmp])
            beginTmp++;

        if(ident[id].orgText[beginTmp] == '#')
        {

            ident[id].begin[i] = beginTmp;
            ident[id].size[i] = 1;
            beginTmp = ident[id].begin[i] + ident[id].size[i];
        }
        else
            break;
    }

    change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditFieldRegular);

    create_newText_for_Id(id);

    if(editID == 0)
    	write_content_without_Id();
    else
    {
        write_content_of_Id(id);
        if(!tME_stop)
            idLast = id;
        id++;
    }
}

void write_field_button(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text)
{
    if(id >= 9)
        return;

    ident[id].maintype = FIELD_BUTTON;
    ident[id].subtype  = FIELD_BUTTON;

    ident[id].coord[0] = XleftGimpStyle;
    ident[id].coord[1] = XrightGimpStyle;
    ident[id].coord[2] = YtopGimpStyle;
    ident[id].fontUsed = (tFont *)Font;
    ident[id].callerID = editID;

    strncpy(ident[id].orgText, text, 32);
    strncpy(ident[id].newText, text, 32);
    ident[id].orgText[31] = 0;
    ident[id].newText[31] = 0;

    change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditButtonColor1);

    if(editID == 0)
        write_content_without_Id();
    else
    {
        write_content_of_Id(id);
        if(!tME_stop)
            idLast = id;
        id++;
    }
}


void write_field_symbol(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1)
{
    if(id >= 9)
        return;

    ident[id].maintype = FIELD_SYMBOL;
    ident[id].subtype  = FIELD_SYMBOL;

    ident[id].coord[0] = XleftGimpStyle;
    ident[id].coord[1] = XrightGimpStyle;
    ident[id].coord[2] = YtopGimpStyle;
    ident[id].fontUsed = (tFont *)Font;
    ident[id].callerID = editID;

    strncpy(ident[id].orgText, text, 32);
    strncpy(ident[id].newText, text, 32);
    ident[id].orgText[31] = 0;

    ident[id].newText[0] = text[0];
    ident[id].newText[1] = 0;

    ident[id].input[0] = int1;
    ident[id].input[1] = strlen(ident[id].orgText);

    change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditButtonColor1);

    if(editID == 0)
        write_content_without_Id();
    else
    {
        write_content_of_Id(id);
        if(!tME_stop)
            idLast = id;
        id++;
    }
}
void write_field_toggle(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1,  uint8_t int2)
{
    if(id >= 9) return;
    ident[id].maintype = FIELD_TOGGLE;
	ident[id].subtype  = FIELD_TOGGLE;
	ident[id].coord[0] = XleftGimpStyle;
	ident[id].coord[1] = XrightGimpStyle;
	ident[id].coord[2] = YtopGimpStyle;
	ident[id].fontUsed = (tFont *)Font;
	ident[id].callerID = editID;
	strncpy(ident[id].orgText, text, 32);
	strncpy(ident[id].newText, text, 32);
	ident[id].orgText[31] = 0;
	ident[id].newText[31] = 0;

	change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditButtonColor1);

	if(editID == 0) write_content_without_Id();
	  else
	{
	      write_content_of_Id(id);
	      if(!tME_stop) idLast = id;
	        id++;
	}
}



/* was build for field_on_off
 * to be tested for other purposes first
 */
void tMenuEdit_refresh_field(uint32_t editID)
{
    uint8_t temp_id;

    temp_id = get_id_of(editID);
    if(temp_id == 255)
        return;

    clean_content( ident[temp_id].coord[0], ident[temp_id].coord[1], ident[temp_id].coord[2], ident[temp_id].fontUsed);
    write_content( ident[temp_id].coord[0], ident[temp_id].coord[1], ident[temp_id].coord[2], ident[temp_id].fontUsed, ident[temp_id].newText, (CLUT_MenuEditField0 + temp_id));
}


void write_field_on_off(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1)
{
    if(id >= 9)
        return;

    ident[id].maintype = FIELD_ON_OFF;
    ident[id].subtype  = FIELD_ON_OFF;

    ident[id].coord[0] = XleftGimpStyle;
    ident[id].coord[1] = XrightGimpStyle;
    ident[id].coord[2] = YtopGimpStyle;
    ident[id].fontUsed = (tFont *)Font;
    ident[id].callerID = editID;

    if(int1)
        ident[id].orgText[0] = '\005';
    else
        ident[id].orgText[0] = '\006';

    ident[id].orgText[1] = ' ';

    strncpy(&ident[id].orgText[2], text, 30);
    strncpy(ident[id].newText, ident[id].orgText, 32);
    ident[id].orgText[31] = 0;
    ident[id].newText[31] = 0;

    if(!tME_stop)
        change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditFieldRegular);
    else
        change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditInfo);

    if(editID == 0)
        write_content_without_Id();
    else
    {
        write_content_of_Id(id);
        if(!tME_stop)
            idLast = id;
        id++;
    }
}


void write_field_fpoint(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, float input)
{
    if(id >= 9)
        return;

    ident[id].maintype = FIELD_NUMBERS;
    ident[id].subtype  = FIELD_FLOAT;

    ident[id].coord[0] = XleftGimpStyle;
    ident[id].coord[1] = XrightGimpStyle;
    ident[id].coord[2] = YtopGimpStyle;
    ident[id].fontUsed = (tFont *)Font;
    ident[id].callerID = editID;

    strncpy(ident[id].orgText, text, 32);
    strncpy(ident[id].newText, text, 32);
    ident[id].orgText[31] = 0;
    ident[id].newText[31] = 0;

    change_CLUT_entry((CLUT_MenuEditField0 + id), CLUT_MenuEditFieldRegular);

    if(editID == 0)
        write_content_without_Id();
    else
    {
        write_content_of_Id(id);
        if(!tME_stop)
            idLast = id;
        id++;
    }
}


void setBackMenu(uint32_t inputFunctionCall, uint8_t functionCallParameter, uint8_t gotoMenuEditField)
{
    backmenu.pEventFunction = inputFunctionCall;
    backmenu.functionParameter = functionCallParameter;
    backmenu.line = gotoMenuEditField;
}


void setEvent(uint32_t inputEventID, uint32_t inputFunctionCall)
{
    if(evidLast >= 9)
        return;

#if 0
    /* set cursor to first field */
    if(evidLast < 0)
    {
        startMenuEditFieldSelect();
    }
#endif
    event[evid].callerID = inputEventID;
    event[evid].pEventFunction = inputFunctionCall;

    evidLast = evid;
    evid++;
}

void startEdit(void)
{
    EnterPressed = 1;
    helperGotoMenuEditField(0);
    enterMenuEditField();
}


void exitEditWithUpdate(void)
{
    createDiveSettings();
    EnterPressed = 1;
    exitMenuEdit(1);
}

/*
void set_cursor(uint8_t forThisIdentID)
{
    int16_t x0, x1, y0, y1;

    uint32_t xtra_left_right = 10;
    uint32_t xtra_top_bottom = 10;

    // y geht von 0 bis 799
    // x geht von 0 bis 479

    x0 = (int16_t)ident[forThisIdentID].coord[0];
    x1 = (int16_t)ident[forThisIdentID].coord[1];
    y0 = (int16_t)ident[forThisIdentID].coord[2];
    y1 = y0 + (int16_t)ident[forThisIdentID].fontUsed->height;

    if(((int16_t)ident[forThisIdentID].fontUsed->height) > 70)
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

    GFX_SetWindowLayer0(tMEcursor.FBStartAdress, x0, x1, y0, y1);
}
*/

void set_cursorNew(uint8_t forThisIdentID)
{
    int16_t y0;
    uint8_t lineMinusOne;

   if(!settingsGetPointer()->FlipDisplay)
   {
	   y0 = (int16_t)ident[forThisIdentID].coord[2];
	   y0 -= ME_Y_LINE1;
	}
	else
	{
    	y0 = 390 + 25 - (int16_t)ident[forThisIdentID].coord[2];
	}

    y0 /= ME_Y_LINE_STEP;
    if((y0 >= 0) && (y0 <=6)) 
        lineMinusOne = y0;
    else
        lineMinusOne = 0;

    if(!settingsGetPointer()->FlipDisplay)
    {
    	GFX_SetFrameBottom((tMEcursorNew.FBStartAdress) + 65*2*(lineMinusOne), 0, 25, 800, 390);
    }
    else
    {
    	GFX_SetFrameBottom((tMEcursorNew.FBStartAdress)+ (390 - 65 *(6-lineMinusOne)) *2, 0, 480-390-25, 800, 390);
    }
}


void write_topline( char *text)
{
    GFX_DrawCfgWindow	hgfx;
    const tFont *Font = &FontT48;

    hgfx.Image = &tMEscreen;
    hgfx.WindowNumberOfTextLines = 1;
    hgfx.WindowLineSpacing = 0;
    hgfx.WindowTab = 0;
    hgfx.WindowX0 = 20;
    hgfx.WindowX1 = 779;
    if(!settingsGetPointer()->FlipDisplay)
    {
		hgfx.WindowY1 = 479;
		hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
    }
	else
	{
		hgfx.WindowY0 = 0;
		hgfx.WindowY1 = hgfx.WindowY0 + Font->height;
	}
    GFX_write_label(Font, &hgfx, text, menuColor);
}


void write_buttonTextline( uint8_t left2ByteCode, char middle2ByteCode, char right2ByteCode)
{

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(!pSettings->FlipDisplay)
	{
		GFX_clean_area(&tMEscreen, 0, 800, 479-24,480);
	}
	else
	{
		GFX_clean_area(&tMEscreen, 0, 800, 0, 24);
	}

    char localtext[32];

    if(left2ByteCode)
    {
        localtext[0] = TXT_2BYTE;
        localtext[1] = left2ByteCode;
        localtext[2] = 0;

        write_content_simple(&tMEscreen, 0, 800, 479-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }

    if(middle2ByteCode)
    {
        localtext[0] = '\001';
        localtext[1] = TXT_2BYTE;
        localtext[2] = middle2ByteCode;
        localtext[3] = 0;

       	write_content_simple(&tMEscreen, 0, 800, 479-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }

    if(right2ByteCode)
    {
        localtext[0] = '\002';
        localtext[1] = TXT_2BYTE;
        localtext[2] = right2ByteCode;
        localtext[3] = 0;

        write_content_simple(&tMEscreen, 0, 800, 479-24, &FontT24,localtext,CLUT_ButtonSurfaceScreen);
    }
}



void write_label_var(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text)
{
    GFX_DrawCfgWindow	hgfx;

    if(XrightGimpStyle > 799)
        XrightGimpStyle = 799;
    if(XleftGimpStyle >= XrightGimpStyle)
        XleftGimpStyle = 0;
    if(YtopGimpStyle > 479)
        YtopGimpStyle = 479;
    hgfx.Image = &tMEscreen;
    hgfx.WindowNumberOfTextLines = 1;
    hgfx.WindowLineSpacing = 0;
    hgfx.WindowTab = 0;
    if(!settingsGetPointer()->FlipDisplay)
    {
		hgfx.WindowX0 = XleftGimpStyle;
		hgfx.WindowX1 = XrightGimpStyle;
		hgfx.WindowY1 = 479 - YtopGimpStyle;
		if(hgfx.WindowY1 < Font->height)
			hgfx.WindowY0 = 0;
		else
			hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
    }
    else
    {
		hgfx.WindowX0 = 800 - XrightGimpStyle;
		hgfx.WindowX1 = 800 - XleftGimpStyle;
		hgfx.WindowY0 = YtopGimpStyle;
		if(hgfx.WindowY0 < Font->height)
			hgfx.WindowY1 = 0;
		else
			hgfx.WindowY1 = hgfx.WindowY0 + Font->height;
    }
    GFX_write_label(Font, &hgfx, text, 0);/*menuColor);*/
}


void write_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text, uint8_t color)
{
    GFX_DrawCfgWindow	hgfx;

    if(XrightGimpStyle > 799)
        XrightGimpStyle = 799;
    if(XleftGimpStyle >= XrightGimpStyle)
        XleftGimpStyle = 0;
    if(YtopGimpStyle > 479)
        YtopGimpStyle = 479;
    hgfx.Image = &tMEscreen;
    hgfx.WindowNumberOfTextLines = 1;
    hgfx.WindowLineSpacing = 0;
    hgfx.WindowTab = 0;

    if(!settingsGetPointer()->FlipDisplay)
    {
		hgfx.WindowX0 = XleftGimpStyle;
		hgfx.WindowX1 = XrightGimpStyle;
		hgfx.WindowY1 = 479 - YtopGimpStyle;
		if(hgfx.WindowY1 < Font->height)
			hgfx.WindowY0 = 0;
		else
			hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
    }
    else
    {
		hgfx.WindowX0 = 800 - XrightGimpStyle;
		hgfx.WindowX1 = 800 - XleftGimpStyle;
		hgfx.WindowY0 = YtopGimpStyle;
		if(hgfx.WindowY0 < Font->height)
			hgfx.WindowY1 = 0;
		else
			hgfx.WindowY1 = hgfx.WindowY0 + Font->height;
    }
    GFX_write_label(Font, &hgfx, text, color);
}


void write_label_fix(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char textId)
{
    char text[2];

    text[0] = textId;
    text[1] = 0;

    write_label_var(XleftGimpStyle, XrightGimpStyle, YtopGimpStyle, Font, text);
}


void clean_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font)
{
    GFX_DrawCfgWindow	hgfx;

    if(XrightGimpStyle > 799)
        XrightGimpStyle = 799;
    if(XleftGimpStyle >= XrightGimpStyle)
        XleftGimpStyle = 0;
    if(YtopGimpStyle > 479)
        YtopGimpStyle = 479;
    hgfx.Image = &tMEscreen;
    if(!settingsGetPointer()->FlipDisplay)
    {
    hgfx.WindowX0 = XleftGimpStyle;
    hgfx.WindowX1 = XrightGimpStyle;
    hgfx.WindowY1 = 479 - YtopGimpStyle;
    if(hgfx.WindowY1 < Font->height)
        hgfx.WindowY0 = 0;
    else
        hgfx.WindowY0 = hgfx.WindowY1 - Font->height;
    }
    else
    {
		hgfx.WindowX0 = 800 - XrightGimpStyle;
		hgfx.WindowX1 = 800 - XleftGimpStyle;
		hgfx.WindowY0 = YtopGimpStyle;
		if(hgfx.WindowY0 < Font->height)
			hgfx.WindowY1 = 0;
		else
			hgfx.WindowY1 = hgfx.WindowY0 + Font->height;
    }
    GFX_clear_window_immediately(&hgfx);
}


/* Private functions ---------------------------------------------------------*/

void draw_tMEdesign(void)
{
    GFX_draw_header(&tMEscreen,menuColor);
}

void draw_tMEdesignSubUnselected(uint32_t *ppDestination)
{
    union al88_u
    {
        uint8_t al8[2];
        uint16_t al88;
    };

    union al88_u color_seperator;
    union al88_u color_unselected;
    int i;

    color_seperator.al8[0] = CLUT_MenuLineUnselectedSeperator;
    color_unselected.al8[0] = CLUT_MenuLineUnselected;

    color_seperator.al8[1] = 0xFF;
    color_unselected.al8[1] = 0xFF;

    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;
    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;

    for(i = 61; i > 0; i--)
    {
        *(__IO uint16_t*)*ppDestination = color_unselected.al88;
        *ppDestination += 2;
    }

    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;
    *(__IO uint16_t*)*ppDestination = color_seperator.al88;
        *ppDestination += 2;
}


void draw_tMEdesignSubSelected(uint32_t *ppDestination)
{
    union al88_u
    {
        uint8_t al8[2];
        uint16_t al88;
    };

    union al88_u color_selected;
    union al88_u color_seperator;
    int i;

    color_selected.al8[0] = CLUT_MenuEditLineSelected;
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


void draw_tMEdesignSubSelectedBorder(uint32_t *ppDestination)
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


void draw_tMEcursorNewDesign(void)
{
    int i,j;
    uint32_t pDestination;

    pDestination = tMEcursorNew.FBStartAdress;

    for(j = 801; j > 0; j--)
    {
        for(i = 5; i > 0; i--)
        {
            draw_tMEdesignSubUnselected(&pDestination);
        }
        if((j > 787) || (j < 17))
            draw_tMEdesignSubSelectedBorder(&pDestination);
        else
            draw_tMEdesignSubSelected(&pDestination);
    }
}

GFX_DrawCfgScreen* getMenuEditScreen()
{
	return &tMEscreen;
}


