///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditDecoParameter.c
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
#include "tMenuEditDecoParameter.h"

#include "gfx_fonts.h"
#include "tMenuEdit.h"
#include "unit.h" // last stop in meter and feet

#define MEDP_TAB (525)

/* Private function prototypes -----------------------------------------------*/
static void openEdit_DecoAlgorithm(void);
static void openEdit_DecoGF(void);
static void openEdit_DecoAltGF(void);
static void openEdit_DecoVPM(void);
static void openEdit_DecoLastStop(void);
static void openEdit_DecoVpmTable(void);
static void openEdit_DM_SwitchAlgorithm(uint8_t line);

//void openEdit_DecoGasUsage(void);

/* Announced function prototypes -----------------------------------------------*/
uint8_t OnAction_GF				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_VPM			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_AltGF			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_LastStop		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DM_ActiveGF	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DM_ActiveVPM	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DM_AltActiveGF	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
/* Exported functions --------------------------------------------------------*/

void openEdit_DecoParameter(uint8_t line)
{
    set_globalState_Menu_Line(line);
    resetMenuEdit(CLUT_MenuPageDecoParameter);

    if(actual_menu_content == MENU_SURFACE)
    {
        switch(line)
        {
        case 1:
        default:
            openEdit_DecoAlgorithm();
            break;
        case 2:
            openEdit_DecoVPM();
            break;
        case 3:
            openEdit_DecoGF();
            break;
        case 4:
            openEdit_DecoAltGF();
            break;
        case 5:
            openEdit_DecoLastStop();
            break;
        case 6:
        	openEdit_DecoVpmTable();
            break;
        }
    }
    else
        openEdit_DM_SwitchAlgorithm(line);
}

/* Private functions ---------------------------------------------------------*/

static void openEdit_DM_SwitchAlgorithm(uint8_t line)
{
    switch(line)
    {
    case 1:
    default:
        stateUsedWrite->diveSettings.deco_type.ub.standard = VPM_MODE;
        break;
    case 2:

    	stateUsedWrite->diveSettings.gf_high = settingsGetPointer()->GF_high;
    	stateUsedWrite->diveSettings.gf_low = settingsGetPointer()->GF_low;
    	stateUsedWrite->diveSettings.deco_type.ub.standard = GF_MODE;
        break;
    case 3:
    	stateUsedWrite->diveSettings.gf_high = settingsGetPointer()->aGF_high;
    	stateUsedWrite->diveSettings.gf_low = settingsGetPointer()->aGF_low;
    	stateUsedWrite->diveSettings.deco_type.ub.standard = GF_MODE;
        break;
    }
    exitMenuEdit_to_Home_with_Menu_Update();
}


static void openEdit_DecoAlgorithm(void)
{
    SSettings *pSettings = settingsGetPointer();

    if(pSettings->deco_type.ub.standard == VPM_MODE)
        pSettings->deco_type.ub.standard = GF_MODE;
    else
        pSettings->deco_type.ub.standard = VPM_MODE;

    exitEditWithUpdate();
}


static void openEdit_DecoVPM(void)
{
    uint8_t vpm;
    char text[32];
    SSettings *pSettings = settingsGetPointer();

    vpm = pSettings->VPM_conservatism.ub.standard;

    text[0] = '\001';
    text[1] = TXT_Decoparameters;
    text[2] = 0;
    write_topline(text);

    write_label_var(   20, 800, ME_Y_LINE2, &FontT48, "VPM");

    write_field_udigit(StMDECOP2_VPM,	MEDP_TAB, 800, ME_Y_LINE2, &FontT48, "+#", (uint32_t)vpm, 0, 0, 0);

    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECOP2_VPM, 					(uint32_t)OnAction_VPM);
    startEdit();
}


static void openEdit_DecoGF(void)
{
    uint8_t gfLow,gfHigh;
    char text[32];
    SSettings *pSettings = settingsGetPointer();

    gfLow = pSettings->GF_low;
    gfHigh = pSettings->GF_high;

    text[0] = '\001';
    text[1] = TXT_Decoparameters;
    text[2] = 0;
    write_topline(text);

    write_label_var(   20, 800, ME_Y_LINE3, &FontT48, "GF\016\016low/high\017");

    write_field_udigit(StMDECOP3_GF, MEDP_TAB, 800, ME_Y_LINE3, &FontT48, "##/##", (uint32_t)gfLow, (uint32_t)gfHigh, 0, 0);

    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECOP3_GF, 					(uint32_t)OnAction_GF);
    startEdit();
}


static void openEdit_DecoAltGF(void)
{
    uint8_t aGfLow,aGfHigh;
    char text[32];
    SSettings *pSettings = settingsGetPointer();

    aGfLow = pSettings->aGF_low;
    aGfHigh = pSettings->aGF_high;

    text[0] = '\001';
    text[1] = TXT_Decoparameters;
    text[2] = 0;
    write_topline(text);

    write_label_var(   20, 800, ME_Y_LINE4, &FontT48, "aGF\016\016low/high\017");
    write_field_udigit(StMDECOP4_AltGF,		MEDP_TAB, 800, ME_Y_LINE4, &FontT48, "##/##", (uint32_t)aGfLow, (uint32_t)aGfHigh, 0, 0);

    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECOP4_AltGF, 					(uint32_t)OnAction_AltGF);
    startEdit();
}


static void openEdit_DecoLastStop(void)
{
    uint8_t lastStop;
    char text[32];
    SSettings *pSettings = settingsGetPointer();

    lastStop = pSettings->last_stop_depth_meter;

    text[0] = '\001';
    text[1] = TXT_LastDecostop;
    text[2] = 0;
    write_topline(text);

    write_label_fix(   20, 800, ME_Y_LINE5, &FontT48, TXT_LastDecostop);

    if(settingsGetPointer()->nonMetricalSystem)
    {
        write_label_var(  MEDP_TAB + 40, 800, ME_Y_LINE5, &FontT48, "\016\016  ft\017");
        write_field_2digit(StMDECOP5_LASTSTOP,	MEDP_TAB, 800, ME_Y_LINE5, &FontT48, "##", (uint32_t)unit_depth_integer(lastStop), 0, 0, 0);
    }
    else
    {
        write_label_var(  MEDP_TAB + 20, 800, ME_Y_LINE5, &FontT48, "\016\016 meter\017");
        write_field_udigit(StMDECOP5_LASTSTOP,	MEDP_TAB, 800, ME_Y_LINE5, &FontT48, "#", (uint32_t)lastStop, 0, 0, 0);
    }
    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMDECOP5_LASTSTOP, 	(uint32_t)OnAction_LastStop);
    startEdit();
}

static void openEdit_DecoVpmTable(void)
{
	SSettings *pSettings = settingsGetPointer();

	if(pSettings->VPM_conservatism.ub.alternative == 0)
	{
	   pSettings->VPM_conservatism.ub.alternative = 1;
	}
    else
    {
    	pSettings->VPM_conservatism.ub.alternative = 0;
    }
	exitEditWithUpdate();
}

uint8_t OnAction_VPM(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;
    uint8_t digitContentNew;
    uint32_t newConservatism;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newConservatism, 0, 0, 0);
        if(newConservatism > 5)
            newConservatism = 5;

        pSettings = settingsGetPointer();
        pSettings->VPM_conservatism.ub.standard = newConservatism;

        tMenuEdit_newInput(editId, newConservatism, 0, 0, 0);
        return UPDATE_AND_EXIT_TO_MENU;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if(digitContentNew > '5')
            digitContentNew = '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if(digitContentNew < '0')
            digitContentNew = '5';
        return digitContentNew;
    }

    return EXIT_TO_MENU;
}


uint8_t OnAction_GF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;
    uint8_t digitContentNew;
    uint32_t newGFlow, newGFhigh;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newGFlow, &newGFhigh, 0, 0);
        if(newGFlow < 10)
            newGFlow = 10;
        if(newGFhigh < 45)
            newGFhigh = 45;
        if(newGFlow > 99)
            newGFlow = 99;
        if(newGFhigh > 99)
            newGFhigh = 99;

        pSettings = settingsGetPointer();
        pSettings->GF_low = newGFlow;
        pSettings->GF_high= newGFhigh;

        tMenuEdit_newInput(editId, newGFlow, newGFhigh, 0, 0);
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

uint8_t OnAction_AltGF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;
    uint8_t digitContentNew;
    uint32_t newGFlow, newGFhigh;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newGFlow, &newGFhigh, 0, 0);
        if(newGFlow < 10)
            newGFlow = 10;
        if(newGFhigh < 45)
            newGFhigh = 45;
        if(newGFlow > 99)
            newGFlow = 99;
        if(newGFhigh > 99)
            newGFhigh = 99;

        pSettings = settingsGetPointer();
        pSettings->aGF_low = newGFlow;
        pSettings->aGF_high= newGFhigh;

        tMenuEdit_newInput(editId, newGFlow, newGFhigh, 0, 0);
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


uint8_t OnAction_LastStop(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;
    uint8_t digitContentNew;
    uint32_t lastStop;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &lastStop, 0, 0, 0);

        if(settingsGetPointer()->nonMetricalSystem != 0) // new hw 170718
        {
            lastStop += 2; // fï¿½r rundung
            lastStop = (lastStop * 3) / 10;
        }

        pSettings = settingsGetPointer();

        pSettings->last_stop_depth_meter = lastStop;

        tMenuEdit_newInput(editId, lastStop, 0, 0, 0);
        return UPDATE_AND_EXIT_TO_MENU;
    }

    if(settingsGetPointer()->nonMetricalSystem)
    {
        // 10, 13, 17, 20
        if(action == ACTION_BUTTON_NEXT)
        {
            if(digitContent < 13 + '0')
                digitContentNew = 13 + '0';
            else if(digitContent < 16 + '0')
                digitContentNew = 16 + '0';
            else if(digitContent < 20 + '0')
                digitContentNew = 20 + '0';
            else
                digitContentNew = 10 + '0';


            return digitContentNew;
        }
        if(action == ACTION_BUTTON_BACK)
        {
            if(digitContent >= 20 + '0')
                digitContentNew = 16 + '0';
            else if(digitContent >= 16 + '0')
                digitContentNew = 13 + '0';
            else if(digitContent >= 13 + '0')
                digitContentNew = 10 + '0';
            else
                digitContentNew = 20 + '0';

            return digitContentNew;
        }
    }
    else
    {
        if(action == ACTION_BUTTON_NEXT)
        {
            digitContentNew = digitContent + 1;
            if(digitContentNew > '6')
                digitContentNew = '3';
            return digitContentNew;
        }
        if(action == ACTION_BUTTON_BACK)
        {
            digitContentNew = digitContent - 1;
            if(digitContentNew < '3')
                digitContentNew = '6';
            return digitContentNew;
        }
    }

    return EXIT_TO_MENU;
}

uint8_t OnAction_DM_ActiveVPM(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    stateUsedWrite->diveSettings.deco_type.ub.standard = VPM_MODE;
    return EXIT_TO_HOME;
}


uint8_t OnAction_DM_ActiveGF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();

    stateUsedWrite->diveSettings.gf_high = pSettings->GF_high;
    stateUsedWrite->diveSettings.gf_low = pSettings->GF_low;
    stateUsedWrite->diveSettings.deco_type.ub.standard = GF_MODE;
    return EXIT_TO_HOME;
}


uint8_t OnAction_DM_AltActiveGF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	SSettings *pSettings = settingsGetPointer();

    stateUsedWrite->diveSettings.gf_high = pSettings->aGF_high;
    stateUsedWrite->diveSettings.gf_low = pSettings->aGF_low;
    stateUsedWrite->diveSettings.deco_type.ub.standard = GF_MODE;
    return EXIT_TO_HOME;
}
