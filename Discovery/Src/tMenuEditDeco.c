///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditDeco.c
/// \brief  Main Template file for Menu Edit Deco Parameters
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
#include "tMenuEditDeco.h"

#include "gfx_fonts.h"
#include "tMenuEdit.h"
#include "unit.h"
#include "configuration.h"

/* Private variables ---------------------------------------------------------*/
static uint8_t lineSelected = 0;

/* Private function prototypes -----------------------------------------------*/

static void openEdit_DiveMode(void);
static void openEdit_ppO2max(void);
static void openEdit_SafetyStop(void);
static void openEdit_ExitTime(void);
static void openEdit_FutureTTS(void);
static void openEdit_Salinity(void);

/* Announced function prototypes -----------------------------------------------*/
static uint8_t OnAction_setMode (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_FutureTTS	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_ppO2Max	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_SafetyStop (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_SlowExitTime		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_Salinity	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
/* Exported functions --------------------------------------------------------*/

void openEdit_Deco(uint8_t line)
{
    set_globalState_Menu_Line(line);
    resetMenuEdit(CLUT_MenuPageDeco);

    lineSelected = line;

    switch(line)
    {
    case 1:
    default:
        openEdit_DiveMode();
        break;
    case 2:
        openEdit_ppO2max();
        break;
    case 3:
        openEdit_SafetyStop();
        break;
    case 4:
         openEdit_ExitTime();
         break;
    case 5:
        openEdit_FutureTTS();
        break;
    case 6:
        openEdit_Salinity();
        break;
    }
}

/* Private functions ---------------------------------------------------------*/


static void openEdit_DiveMode(void)
{
#define APNEAANDGAUGE

    char text[32];
    uint8_t lineOffset = 0;
    uint8_t actualDiveMode, active;
    SSettings *pSettings = settingsGetPointer();
    actualDiveMode = pSettings->dive_mode;

    text[0] = '\001';
    text[1] = TXT_DiveMode;
    text[2] = 0;
    write_topline(text);

    text[1] = 0;


#ifdef ENABLE_PSCR_MODE
    lineOffset = ME_Y_LINE_STEP;
#endif

    text[0] = TXT_OpenCircuit;
    if(actualDiveMode == DIVEMODE_OC)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMDECO1_OC,			 30, 500, ME_Y_LINE1,  &FontT48, text, active);

    text[0] = TXT_ClosedCircuit;
    if(actualDiveMode == DIVEMODE_CCR)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMDECO1_CC,			 30, 500, ME_Y_LINE2,  &FontT48, text, active);

#ifdef ENABLE_PSCR_MODE
    text[0] = TXT_PSClosedCircuit;
    if(actualDiveMode == DIVEMODE_PSCR)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMDECO1_PSCR,		 30, 500, ME_Y_LINE3,  &FontT48, text, active);
#endif
#ifdef APNEAANDGAUGE
    text[0] = TXT_Apnoe;
    if(actualDiveMode == DIVEMODE_Apnea)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMDECO1_Apnea,	 30, 500, ME_Y_LINE3 + lineOffset,  &FontT48, text, active);

    text[0] = TXT_Gauge;
    if(actualDiveMode == DIVEMODE_Gauge)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMDECO1_Gauge,	 30, 500, ME_Y_LINE4 + lineOffset,  &FontT48, text, active);
#endif

    setEvent(StMDECO1_OC, 			(uint32_t)OnAction_setMode);
    setEvent(StMDECO1_CC, 			(uint32_t)OnAction_setMode);
#ifdef ENABLE_PSCR_MODE
    setEvent(StMDECO1_PSCR,			(uint32_t)OnAction_setMode);
#endif

#ifdef APNEAANDGAUGE
    setEvent(StMDECO1_Apnea, 		(uint32_t)OnAction_setMode);
    setEvent(StMDECO1_Gauge, 		(uint32_t)OnAction_setMode);
#endif

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}


static uint8_t OnAction_setMode (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action) //(uint32_t newMode)
{
	uint32_t modeArray[] = {StMDECO1_OC, StMDECO1_CC, StMDECO1_Gauge, StMDECO1_Apnea  /* definition needs to follow order of DIVEMODE (settings.h) */
#ifdef ENABLE_PSCR_MODE
			, StMDECO1_PSCR
#endif
	};

	uint8_t index = 0;
	SSettings *pSettings = settingsGetPointer();
	uint8_t retVal = EXIT_TO_MENU;
	uint8_t lastMode = pSettings->dive_mode;


	setActualGasFirst(&stateRealGetPointerWrite()->lifeData);
	while(index < sizeof(modeArray) / 4)	/* calculate number of items out of array size (bytes) */
	{
		if(editId == modeArray[index])
		{
			if(pSettings->dive_mode != index)
			{
				tMenuEdit_set_on_off(modeArray[index], 1);
				pSettings->dive_mode = index;
				retVal = UPDATE_DIVESETTINGS;
			}
		}
		else
		{
			if(lastMode == index)		/* reset state of previous mode selection */
			{
				tMenuEdit_set_on_off(modeArray[index], 0);
			}
		}
		index++;
	}
	return retVal;
}





static void openEdit_SafetyStop(void)
{
    uint32_t safetystopDuration, safetystopDepth;
    char text[64];
    uint16_t y_line;

    safetystopDuration = settingsGetPointer()->safetystopDuration;
    safetystopDepth = settingsGetPointer()->safetystopDepth;

    y_line = ME_Y_LINE_BASE + (lineSelected * ME_Y_LINE_STEP);

    text[0] = '\001';
    text[1] = TXT_SafetyStop;
    text[2] = 0;
    write_topline(text);

    write_label_fix(   20, 800, y_line, &FontT48, TXT_SafetyStop);

    strcpy(text,"\016\016");
    text[2] = TXT_Minutes;
    if(settingsGetPointer()->nonMetricalSystem)
    {
        strcpy(&text[3],
            "\017"
            "  @       "
            "\016\016"
            " ft"
            "\017"
        );
    }
    else
    {
        strcpy(&text[3],
            "\017"
            "  @       "
            "\016\016"
            " m"
            "\017"
        );
    }
    write_label_var(  410, 800, y_line, &FontT48, text);

    if(settingsGetPointer()->nonMetricalSystem)
    {
        write_field_2digit(StMDECO4_SafetyStop,	 		350, 800, y_line, &FontT48, "##               ##", safetystopDuration, unit_depth_integer(safetystopDepth), 0, 0);
    }
    else
    {
        write_field_udigit(StMDECO4_SafetyStop,	 		370, 800, y_line, &FontT48, "#                 #", safetystopDuration, safetystopDepth, 0, 0);
    }

    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECO4_SafetyStop, 		(uint32_t)OnAction_SafetyStop);
    startEdit();
}


static void openEdit_ExitTime(void)
{
    uint32_t SlowExitTime;
    char text[64];
    uint16_t y_line;

    SlowExitTime = settingsGetPointer()->safetystopDuration;

    y_line = ME_Y_LINE_BASE + (lineSelected * ME_Y_LINE_STEP);

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_SlowExit;
    text[3] = 0;
    write_topline(text);

    write_label_var(   20, 800, y_line, &FontT48, &text[1]);

    strcpy(text,"\016\016");
    text[2] = TXT_Minutes;
    if(settingsGetPointer()->nonMetricalSystem)
    {
        sprintf(&text[3], "\017  ^      %u\016\016 ft\017", settingsGetPointer()->slowExitTime);
    }
    else
    {
        sprintf(&text[3],  "\017  ^      %u\016\016 m\017", settingsGetPointer()->slowExitTime);
    }
    write_label_var(  410, 800, y_line, &FontT48, text);

    write_field_udigit(StMDECO_SlowExit,	 		370, 800, y_line, &FontT48, "#", SlowExitTime, 0, 0, 0);
    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECO_SlowExit, 		(uint32_t)OnAction_SlowExitTime);
    startEdit();
}


static uint8_t OnAction_SafetyStop		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newSafetystopDuration, newSafetystopDepth;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newSafetystopDuration, &newSafetystopDepth, 0, 0);

        if(settingsGetPointer()->nonMetricalSystem != 0) // new hw 170718
        {
            newSafetystopDepth += 2; // fï¿½r rundung
            newSafetystopDepth = (newSafetystopDepth * 3) / 10;
        }


        settingsGetPointer()->safetystopDuration = newSafetystopDuration;
        settingsGetPointer()->safetystopDepth = newSafetystopDepth;

        tMenuEdit_newInput(editId, newSafetystopDuration, newSafetystopDepth, 0, 0);
        return UPDATE_AND_EXIT_TO_MENU;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if(blockNumber == 0)
        {
            if(digitContentNew > '5')
                digitContentNew = '0';
        }
        else
        {
            if(settingsGetPointer()->nonMetricalSystem == 0)
            {
                if(digitContentNew > '6')
                    digitContentNew = '3';
            }
            else
            {
                if(digitContent < 13 + '0')
                    digitContentNew = 13 + '0';
                else if(digitContent < 16 + '0')
                    digitContentNew = 16 + '0';
                else if(digitContent < 20 + '0')
                    digitContentNew = 20 + '0';
                else
                    digitContentNew = 10 + '0';
            }
        }
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if(blockNumber == 0)
        {
            if(digitContentNew < '0')
                digitContentNew = '5';
        }
        else
        {
            if(settingsGetPointer()->nonMetricalSystem == 0)
            {
            if(digitContentNew < '3')
                digitContentNew = '6';
            }
            else
            {
                if(digitContent >= 20 + '0')
                    digitContentNew = 16 + '0';
                else if(digitContent >= 16 + '0')
                    digitContentNew = 13 + '0';
                else if(digitContent >= 13 + '0')
                    digitContentNew = 10 + '0';
                else
                    digitContentNew = 20 + '0';
            }
        }
        return digitContentNew;
    }
    return EXIT_TO_MENU;
}

static uint8_t OnAction_SlowExitTime		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newExitTime;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newExitTime, 0, 0, 0);

        settingsGetPointer()->slowExitTime = newExitTime;

        tMenuEdit_newInput(editId, newExitTime, 0, 0, 0);
        return UPDATE_AND_EXIT_TO_MENU;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if(blockNumber == 0)
        {
            if(digitContentNew > '9')
                digitContentNew = '0';
        }

        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if(blockNumber == 0)
        {
            if(digitContentNew < '0')
                digitContentNew = '9';
        }

        return digitContentNew;
    }
    return EXIT_TO_MENU;
}

static void openEdit_Salinity(void)
{
    char text[32];
    uint16_t y_line;

    text[0] = '\001';
    text[1] = TXT_Salinity;
    text[2] = 0;
    write_topline(text);

    y_line = ME_Y_LINE_BASE + (lineSelected * ME_Y_LINE_STEP);

    write_label_fix(   30, 800, y_line, &FontT48, TXT_Salinity);
    write_label_var(  400, 800, y_line, &FontT48, "\016\016 %\017");

    write_field_udigit(StMDECO6_SALINITY,	370, 800, y_line, &FontT48, "#", (uint32_t)settingsGetPointer()->salinity, 0, 0, 0);

    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECO6_SALINITY, 	(uint32_t)OnAction_Salinity);
    startEdit();
}


static uint8_t OnAction_Salinity(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;
    uint8_t digitContentNew;
    uint32_t salinity;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &salinity, 0, 0, 0);

        if(salinity >= 4)
            salinity = 4;

        pSettings = settingsGetPointer();

        pSettings->salinity = salinity;

        tMenuEdit_newInput(editId, salinity, 0, 0, 0);
        return UPDATE_AND_EXIT_TO_MENU;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if(digitContentNew > '4')
            digitContentNew = '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if(digitContentNew < '0')
            digitContentNew = '4';
        return digitContentNew;
    }

    return EXIT_TO_MENU;
}


static void openEdit_ppO2max(void)
{
    uint8_t maxL_std, maxL_deco;
    uint16_t y_line;
    char text[32];
    SSettings *pSettings = settingsGetPointer();

    maxL_std = pSettings->ppO2_max_std - 100;
    maxL_deco = pSettings->ppO2_max_deco - 100;

    y_line = ME_Y_LINE_BASE + (lineSelected * ME_Y_LINE_STEP);

    text[0] = '\001';
    text[1] = TXT_ppO2Name;
    text[2] = 0;
    write_topline(text);

    strcpy(text,"ppO2\016\016max\017");
    write_label_var(   20, 800, y_line, &FontT48, text);
    strcpy(text,
        "\016\016"
        " bar "
        "  deco "
        "\017"
        "      "
        "\016\016"
        " bar"
        "\017"
    );
    write_label_var(  460, 800, y_line, &FontT48, text);

//	write_field_udigit(StMDECO4_PPO2Max,		410, 800, y_line, &FontT48, "##              ##", (uint32_t)maxL_std, (uint32_t)maxL_deco, 0, 0);
    write_field_udigit(StMDECO3_PPO2Max,		370, 800, y_line, &FontT48, "1.##           1.##", (uint32_t)maxL_std, (uint32_t)maxL_deco, 0, 0);

    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECO3_PPO2Max,			(uint32_t)OnAction_ppO2Max);
    startEdit();
}


static uint8_t OnAction_ppO2Max(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;
    uint8_t digitContentNew;
    uint32_t newPPO2LStd, newPPO2LDeco;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newPPO2LStd, &newPPO2LDeco, 0, 0);

        if(newPPO2LStd > 90)
            newPPO2LStd = 90;

        if(newPPO2LDeco > 90)
            newPPO2LDeco = 90;

        pSettings = settingsGetPointer();
        pSettings->ppO2_max_std = 100 + newPPO2LStd;
        pSettings->ppO2_max_deco = 100 + newPPO2LDeco;

        tMenuEdit_newInput(editId, newPPO2LStd, newPPO2LDeco, 0, 0);
        return UPDATE_AND_EXIT_TO_MENU;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if(digitContentNew > '9')
            digitContentNew = '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if(digitContentNew < '0')
            digitContentNew = '9';
        return digitContentNew;
    }
    return EXIT_TO_MENU;
}


static void openEdit_FutureTTS(void)
{
    uint8_t futureTTS;
    uint16_t y_line;

    char text[32];
    SSettings *pSettings = settingsGetPointer();
    futureTTS = pSettings->future_TTS;

    y_line = ME_Y_LINE_BASE + (lineSelected * ME_Y_LINE_STEP);

    text[0] = '\001';
    text[1] = TXT_FutureTTS;
    text[2] = 0;
    write_topline(text);

    strcpy(text,"\016\016");
    text[2] = TXT_Minutes;
    text[3] = 0;
    write_label_fix(   20, 800, y_line, &FontT48, TXT_FutureTTS);
    write_label_var(  435, 800, y_line, &FontT48, text);
    write_field_2digit(StMDECO5_FUTURE,	 		370, 500, y_line, &FontT48, "##", (uint32_t)futureTTS, 0, 0, 0);

    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECO5_FUTURE, 		(uint32_t)OnAction_FutureTTS);
    startEdit();
}


static uint8_t OnAction_FutureTTS(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;
    int8_t digitContentNew;
    uint32_t newFutureTTS;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newFutureTTS, 0, 0, 0);

        if(newFutureTTS > 15)
            newFutureTTS = 15;

        pSettings = settingsGetPointer();
        pSettings->future_TTS = newFutureTTS;

        tMenuEdit_newInput(editId, newFutureTTS, 0, 0, 0);
        return UPDATE_AND_EXIT_TO_MENU;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if(digitContentNew > '0'+ 15)
            digitContentNew = '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if(digitContentNew < '0')
            digitContentNew = '0' + 15;
        return digitContentNew;
    }
    return EXIT_TO_MENU;
}
