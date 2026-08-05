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
static void openEdit_Cave(void);
/* openEdit_CompassStyle is non-static (declared in tMenuEditCvOption.h) so the
   dive-mode Xtra menu can open the Compass Style page during a dive. */

/* Announced function prototypes -----------------------------------------------*/
uint8_t OnAction_Compass		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CompassDeclination(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Bearing		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_BearingClear	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_InertiaLevel	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_Timer(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CaveAutoStart(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CaveSwapMode(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CompassScaleVariant(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CompassSecondaryLabels(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CompassCourseTolerance(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CompassMinorTicks(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_CompassMountTilt(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);


/* Exported functions --------------------------------------------------------*/


void tMCvOption_SetOpenFnct(uint8_t cvOptId, uint8_t index)
{
	if(index < MAXLINES)
	{
		switch(cvOptId)
		{
			case CVOPT_Compass:	openFctPointerTable[index] = openEdit_Compass;
				break;
			case CVOPT_CompassStyle: openFctPointerTable[index] = openEdit_CompassStyle;
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
			case CVOPT_Cave: openFctPointerTable[index] = openEdit_Cave;
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

static void openEdit_Cave(void)
{
    SSettings *settings = settingsGetPointer();

    char text[10];
    uint8_t textIndex = 0;

    set_globalState(StMCustom3_CViewSelection1);
    resetMenuEdit(CLUT_MenuPageHardware);

    text[textIndex++] = '\001';
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_CaveMode;
    text[textIndex++] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[2] = 0;

    text[1] = TXT2BYTE_AutoStart;
    write_field_on_off(StMCustom3_CViewSelection1,	30, 800, ME_Y_LINE1,  &FontT48, text, settings->caveModeAutoStart);

    text[1] = TXT2BYTE_SwapMode;
    write_field_on_off(StMCustom3_CViewSelection2,	30, 800, ME_Y_LINE2,  &FontT48, text, settings->caveModeSwapMode);


    setEvent(StMCustom3_CViewSelection1,		(uint32_t)OnAction_CaveAutoStart);
    setEvent(StMCustom3_CViewSelection2,	(uint32_t)OnAction_CaveSwapMode);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
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

static uint8_t OnAction_CaveAutoStart(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	SSettings *pSettings = settingsGetPointer();
	if(pSettings->caveModeAutoStart)
	{
		pSettings->caveModeAutoStart = 0;
	}
	else
	{
		pSettings->caveModeAutoStart = 1;
	}
	tMenuEdit_set_on_off(editId, pSettings->caveModeAutoStart);
	return UNSPECIFIC_RETURN;
}
static uint8_t OnAction_CaveSwapMode(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	SSettings *pSettings = settingsGetPointer();
	if(pSettings->caveModeSwapMode)
	{
		pSettings->caveModeSwapMode = 0;
	}
	else
	{
		pSettings->caveModeSwapMode = 1;
	}
	tMenuEdit_set_on_off(editId, pSettings->caveModeSwapMode);
	return UNSPECIFIC_RETURN;
}



/* Append the scale-variant value label as plain ASCII into dst, returning the
   number of chars written. Inlined instead of using two TXT2BYTE tokens: the
   OSTC 2-byte text space is one byte wide and the compass menu would otherwise
   push TXT2BYTE_END past 255. The value is a short glyph string, not prose. */
static uint8_t compassScaleVariantStr(uint8_t variant, char *dst)
{
    const char *s = (variant == 0) ? "30`" : "8pt";   /* "30`" = 30 deg (`=deg glyph) / "8pt" */
    uint8_t n = 0;
    while(s[n]) { dst[n] = s[n]; n++; }
    return n;
}

/* Helper: map compassMountTilt to its display TXT2BYTE code */
static uint8_t compassMountTiltCode(uint8_t tilt)
{
    if(tilt == 1)
        return TXT2BYTE_TiltLeft;
    if(tilt == 2)
        return TXT2BYTE_TiltRight;
    return TXT2BYTE_TiltNone;
}


void refresh_CompassStyle(void)
{
    SSettings *settings = settingsGetPointer();
    char text[32];
    uint8_t textIndex;

    /* page title */
    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_CompassStyle;
    text[3] = 0;
    write_topline(text);

    /* Scale row: label + current value name */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_CompassScale;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    textIndex += compassScaleVariantStr(settings->compassScaleVariant, &text[textIndex]);
    text[textIndex++] = 0;
    write_label_var(30, 800, ME_Y_LINE1, &FontT48, text);

    /* SecondaryLabels and MinorTicks: refresh the on/off indicator */
    tMenuEdit_refresh_field(StMOption_CompassStyle_Secondary);
    tMenuEdit_refresh_field(StMOption_CompassStyle_MinorTicks);

    /* Course tolerance row: label + current value */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_CourseTol;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    textIndex += snprintf(&text[textIndex], 5, "%u`", settings->compassCourseTolerance);
    write_label_var(30, 800, ME_Y_LINE4, &FontT48, text);

    /* Wrist offset row: label + current value name */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_MountTilt;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = compassMountTiltCode(settings->compassMountTilt);
    text[textIndex++] = 0;
    write_label_var(30, 800, ME_Y_LINE5, &FontT48, text);

    write_buttonTextline(TXT2BYTE_ButtonBack, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonNext);
}


void openEdit_CompassStyle(void)
{
    SSettings *settings = settingsGetPointer();
    char text[32];
    uint8_t textIndex;

    set_globalState(StMOption_CompassStyle);
    resetMenuEdit(CLUT_MenuPageHardware);

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_CompassStyle;
    text[3] = 0;
    write_topline(text);

    /* LINE1: Scale cycler */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_CompassScale;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    textIndex += compassScaleVariantStr(settings->compassScaleVariant, &text[textIndex]);
    text[textIndex++] = 0;
    write_field_button(StMOption_CompassStyle_Scale, 30, 800, ME_Y_LINE1, &FontT48, text);

    /* LINE2: Minor labels on/off */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_SecLabels;
    text[textIndex++] = 0;
    write_field_on_off(StMOption_CompassStyle_Secondary, 30, 800, ME_Y_LINE2, &FontT48, text, settings->compassSecondaryLabels);

    /* LINE3: Minor ticks on/off */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_MinorTicks;
    text[textIndex++] = 0;
    write_field_on_off(StMOption_CompassStyle_MinorTicks, 30, 800, ME_Y_LINE3, &FontT48, text, settings->compassMinorTicks);

    /* LINE4: Course tolerance +/- */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_CourseTol;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    textIndex += snprintf(&text[textIndex], 5, "%u`", settings->compassCourseTolerance);
    write_field_button(StMOption_CompassStyle_CourseTol, 30, 800, ME_Y_LINE4, &FontT48, text);

    /* LINE5: Wrist offset cycler */
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_MountTilt;
    text[textIndex++] = ':';
    text[textIndex++] = ' ';
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = compassMountTiltCode(settings->compassMountTilt);
    text[textIndex++] = 0;
    write_field_button(StMOption_CompassStyle_MountTilt, 30, 800, ME_Y_LINE5, &FontT48, text);

    setEvent(StMOption_CompassStyle_Scale,     (uint32_t)OnAction_CompassScaleVariant);
    setEvent(StMOption_CompassStyle_Secondary, (uint32_t)OnAction_CompassSecondaryLabels);
    setEvent(StMOption_CompassStyle_CourseTol, (uint32_t)OnAction_CompassCourseTolerance);
    setEvent(StMOption_CompassStyle_MinorTicks,(uint32_t)OnAction_CompassMinorTicks);
    setEvent(StMOption_CompassStyle_MountTilt, (uint32_t)OnAction_CompassMountTilt);

    tMenuEdit_select(StMOption_CompassStyle_Scale);

    write_buttonTextline(TXT2BYTE_ButtonBack, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonNext);
}


static uint8_t OnAction_CompassScaleVariant(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t newVariant;

    newVariant = settings->compassScaleVariant + 1;
    if(newVariant > 1)
        newVariant = 0;
    settings->compassScaleVariant = newVariant;
    return UPDATE_DIVESETTINGS;
}


static uint8_t OnAction_CompassSecondaryLabels(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t newVal;

    newVal = settings->compassSecondaryLabels ? 0u : 1u;
    settings->compassSecondaryLabels = newVal;
    tMenuEdit_set_on_off(editId, newVal);
    return UPDATE_DIVESETTINGS;
}


#define MIN_COURSE_TOLERANCE (2u)
#define MAX_COURSE_TOLERANCE (15u)

static uint8_t OnAction_CompassCourseTolerance(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t newTol;

    if(action == ACTION_BUTTON_NEXT)
    {
        newTol = settings->compassCourseTolerance + 1;
        if(newTol > MAX_COURSE_TOLERANCE)
            newTol = MIN_COURSE_TOLERANCE;
    }
    else if(action == ACTION_BUTTON_BACK)
    {
        if(settings->compassCourseTolerance <= MIN_COURSE_TOLERANCE)
            newTol = MAX_COURSE_TOLERANCE;
        else
            newTol = settings->compassCourseTolerance - 1;
    }
    else
    {
        /* ACTION_BUTTON_ENTER: cycle forward */
        newTol = settings->compassCourseTolerance + 1;
        if(newTol > MAX_COURSE_TOLERANCE)
            newTol = MIN_COURSE_TOLERANCE;
    }
    settings->compassCourseTolerance = newTol;
    return UPDATE_DIVESETTINGS;
}


static uint8_t OnAction_CompassMinorTicks(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t newVal;

    newVal = settings->compassMinorTicks ? 0u : 1u;
    settings->compassMinorTicks = newVal;
    tMenuEdit_set_on_off(editId, newVal);
    return UPDATE_DIVESETTINGS;
}


static uint8_t OnAction_CompassMountTilt(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t newTilt;

    newTilt = settings->compassMountTilt + 1;
    if(newTilt > 2u)
        newTilt = 0;
    settings->compassMountTilt = newTilt;
    return UPDATE_DIVESETTINGS;
}


