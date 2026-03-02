///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditCvOption.c
/// \brief  Menu for configuration depended items
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
#include "tMenuEditCvOption.h"
#include "tMenuCvOptionText.h"
#include "tMenuEditHardware.h"
#include "tMenuEdit.h"

#include "gfx_fonts.h"
#include "ostc.h"
#include "tMenuEdit.h"
#include "tHome.h"

#include "cv_heartbeat.h"


static openFunc_t openFctPointerTable[MAXLINES];		/* function pointer for refresh */

/* Private function prototypes -----------------------------------------------*/
static void openEdit_Timer(void);
void openEdit_Compass(void);

/* Announced function prototypes -----------------------------------------------*/
uint8_t OnAction_Compass		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CompassDeclination(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Bearing		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_BearingClear	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_InertiaLevel	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_Timer(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

/* Exported functions --------------------------------------------------------*/


void tMCvOption_SetOpenFnct(uint8_t cvOptId, uint8_t index)
{
	if(index < MAXLINES)
	{
		switch(cvOptId)
		{
			case CVOPT_Compass:	openFctPointerTable[index] = openEdit_Compass;
				break;
			case CVOPT_Timer: openFctPointerTable[index] = openEdit_Timer;
				break;
			case CVOPT_END: openFctPointerTable[index] = NULL;
				break;
			case CVOPT_O2_Sensor: openFctPointerTable[index] = openEdit_SensorsO2;
				break;
			case CVOPT_CO2_Sensor: openFctPointerTable[index] = openEdit_SensorsCO2;
				break;
			case CVOPT_HUD: openFctPointerTable[index] = openEdit_SensorsHUD;
				break;
			default:
				break;
		}
	}
}


void openEdit_CvOption(uint8_t line)
{
    if(openFctPointerTable[line - 1] != NULL)
    {
    	openFctPointerTable[line  - 1]();
    }
}

/* Private functions ---------------------------------------------------------*/

static uint8_t OnAction_CompassDeclination(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t digitContentNew;
    switch (action) {
    case ACTION_BUTTON_ENTER:

        return digitContent;
    case ACTION_BUTTON_ENTER_FINAL:
        {
            int32_t compassDeclinationDeg;
            evaluateNewString(editId, (uint32_t *)&compassDeclinationDeg, NULL, NULL, NULL);

            if (compassDeclinationDeg > 99) {
                compassDeclinationDeg = 99;
            } else if (compassDeclinationDeg < -99) {
                compassDeclinationDeg = -99;
            }

            settings->compassDeclinationDeg = compassDeclinationDeg;

            tMenuEdit_newInput(editId, ((input_u)compassDeclinationDeg).uint32, 0, 0, 0);
        }

        break;
    case ACTION_BUTTON_NEXT:
        if (digitNumber == 0) {
            digitContentNew = togglePlusMinus(digitContent);
        } else {
            digitContentNew = digitContent + 1;
            if (digitContentNew > '9') {
                digitContentNew = '0';
            }
        }

        return digitContentNew;
    case ACTION_BUTTON_BACK:
        if (digitNumber == 0) {
            digitContentNew = togglePlusMinus(digitContent);
        } else {
            digitContentNew = digitContent - 1;
            if (digitContentNew < '0') {
                digitContentNew = '9';
            }
        }

        return digitContentNew;
    }

    return UNSPECIFIC_RETURN;
}


static void showCompassDeclination(SSettings *settings, bool isRefresh)
{
    char text[16];
    snprintf(text, 16, "%c%c:", TXT_2BYTE, TXT2BYTE_CompassDeclination);
    write_label_var(30, 800, ME_Y_LINE6, &FontT48, text);
    if (isRefresh) {
        tMenuEdit_refresh_field(StMOption_Compass_Declination);
    } else {
        write_field_sdigit(StMOption_Compass_Declination, 500, 800, ME_Y_LINE6, &FontT48, "\034###`", settings->compassDeclinationDeg, 0, 0, 0);
    }
}


void refresh_CompassEdit(void)
{
    SSettings *settings = settingsGetPointer();

    uint16_t heading;
    char text[32];
    uint8_t textIndex = 0;

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Compass;
    text[3] = 0;
    write_topline(text);

    if(settings->compassInertia)
    {
    	heading = (uint16_t)compass_getCompensated();
    }
    else
    {
    	heading = (uint16_t)stateUsed->lifeData.compass_heading;
    }
    snprintf(text,32,"\001%03i`",heading);
    write_label_var(   0, 800, ME_Y_LINE1, &FontT54, text);

    tMenuEdit_refresh_field(StMOption_Compass_SetCourse);
    tMenuEdit_refresh_field(StMOption_Compass_Calibrate);
    tMenuEdit_refresh_field(StMOption_Compass_ResetCourse);
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_CompassInertia;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    text[textIndex++] = '0' + settings->compassInertia;

    write_label_var(30, 800, ME_Y_LINE5,  &FontT48, text);

    showCompassDeclination(settings, true);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}


void openEdit_Compass(void)
{
    SSettings *settings = settingsGetPointer();

    char text[10];
    uint8_t textIndex = 0;


    set_globalState(StMOption_Compass);
    resetMenuEdit(CLUT_MenuPageHardware);

    text[textIndex++] = '\001';
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_Compass;
    text[textIndex++] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[2] = 0;

    text[1] = TXT2BYTE_SetBearing;
    write_field_button(StMOption_Compass_SetCourse,	 30, 800, ME_Y_LINE2,  &FontT48, text);

    text[1] = TXT2BYTE_ResetBearing;
    write_field_button(StMOption_Compass_ResetCourse, 30, 800, ME_Y_LINE3,  &FontT48, text);

    text[1] = TXT2BYTE_CompassCalib;
    write_field_button(StMOption_Compass_Calibrate,	 30, 800, ME_Y_LINE4,  &FontT48, text);

    text[1] = TXT2BYTE_CompassInertia;
    textIndex = 2;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    text[textIndex++] = '0' + settings->compassInertia;
    text[textIndex++] = 0;

    write_field_button(StMOption_Compass_Inertia, 30, 800, ME_Y_LINE5,  &FontT48, text);

    showCompassDeclination(settings, false);

    setEvent(StMOption_Compass_SetCourse,		(uint32_t)OnAction_Bearing);
    setEvent(StMOption_Compass_ResetCourse,	(uint32_t)OnAction_BearingClear);
    setEvent(StMOption_Compass_Calibrate,		(uint32_t)OnAction_Compass);
    setEvent(StMOption_Compass_Inertia,	(uint32_t)OnAction_InertiaLevel);
    setEvent(StMOption_Compass_Declination, (uint32_t)OnAction_CompassDeclination);

    tMenuEdit_select(StMOption_Compass_SetCourse);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}


uint8_t OnAction_Compass (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    calibrateCompass();
    return EXIT_TO_INFO_COMPASS;
}


uint8_t OnAction_Bearing	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    if((int16_t)stateUsed->lifeData.compass_heading != -1)
	{
		settingsGetPointer()->compassBearing = (int16_t)stateUsed->lifeData.compass_heading;
	}
	else
	{
		settingsGetPointer()->compassBearing = 0;
	}

    if(settingsGetPointer()->compassBearing == 0)
        settingsGetPointer()->compassBearing = 360;
    return UPDATE_AND_EXIT_TO_MENU;
}


uint8_t OnAction_BearingClear	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    settingsGetPointer()->compassBearing = 0;
    return UPDATE_AND_EXIT_TO_MENU;
}


uint8_t OnAction_InertiaLevel	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	uint8_t newLevel = 0;

	newLevel = settingsGetPointer()->compassInertia + 1;
	if(newLevel > MAX_COMPASS_COMP)
	{
		newLevel = 0;
	}
	settingsGetPointer()->compassInertia = newLevel;
    return UPDATE_DIVESETTINGS;
}

static void openEdit_Timer(void)
{
    SSettings *settings = settingsGetPointer();

    char text[32];
    snprintf(text, 32, "\001%c%c", TXT_2BYTE, TXT2BYTE_Timer);
    write_topline(text);

    set_globalState(StMOption_Timer);
    resetMenuEdit(CLUT_MenuPageCvOption);

    uint16_t yPos = ME_Y_LINE_BASE + get_globalState_Menu_Line() * ME_Y_LINE_STEP;
    snprintf(text, 32, "%c%c", TXT_2BYTE, TXT2BYTE_Timer);
    write_label_var(30, 299, yPos, &FontT48, text);
    write_field_udigit(StMOption_Timer_Value, 300, 392, yPos, &FontT48, "#:##", settings->timerDurationS / 60, settings->timerDurationS % 60, 0, 0);
    write_label_var(393, 800, yPos, &FontT48, "\016\016 [m:ss]\017");

    write_buttonTextline(TXT2BYTE_ButtonMinus, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonPlus);

    setEvent(StMOption_Timer_Value, (uint32_t)OnAction_Timer);
    startEdit();
}
static uint8_t OnAction_Timer(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t digitContentNew;
    switch (action) {
    case ACTION_BUTTON_ENTER:

        return digitContent;
    case ACTION_BUTTON_ENTER_FINAL:
        {
            uint32_t timerM;
            uint32_t timerS;
            evaluateNewString(editId, &timerM, &timerS, 0, 0);
            if (timerM > 9) {
                timerM = 9;
            }
            if (timerS > 59) {
                timerS = 59;
            }

            uint16_t timerDurationS = 60 * timerM + timerS;

            if (timerDurationS < 1) {
                timerDurationS = 1;
            }

            if (timerDurationS != settings->timerDurationS) {
                settings->timerDurationS = timerDurationS;

                disableTimer();

                tMenuEdit_newInput(editId, settings->timerDurationS / 60, settings->timerDurationS % 60, 0, 0);
            }

            settings->cv_configuration |= (1 << CVIEW_Timer);

            return EXIT_TO_MENU;
        }
    case ACTION_BUTTON_NEXT:
        digitContentNew = digitContent + 1;
        if ((blockNumber == 1 && digitNumber == 0 && digitContentNew > '5') || digitContentNew > '9') {
            digitContentNew = '0';
        }

        return digitContentNew;
    case ACTION_BUTTON_BACK:
        digitContentNew = digitContent - 1;
        if (digitContentNew < '0') {
            if (blockNumber == 1 && digitNumber == 0) {
                digitContentNew = '5';
            } else {
                digitContentNew = '9';
            }
        }

        return digitContentNew;
    }

    return EXIT_TO_MENU;
}


