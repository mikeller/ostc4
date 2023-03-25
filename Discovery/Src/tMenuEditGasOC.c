///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditGasOC.c
/// \brief  Main Template file for editing Open Circuit Gas Settings
/// \author heinrichs weikamp gmbh
/// \date   09-July-2014
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
#include "tMenuEditGasOC.h"

#include "check_warning.h"
#include "gfx_fonts.h"
#include "tMenuEdit.h"
#include "unit.h"
#include "configuration.h"

/* Private types -------------------------------------------------------------*/
typedef struct
{
    uint8_t gasID;
    SGasLine * pGasLine;
    uint8_t mod;
    uint8_t ccr;
    uint8_t setpoint;
} SEditGasPage;


/* Private variables ---------------------------------------------------------*/
SEditGasPage editGasPage;

/* Private function prototypes -----------------------------------------------*/
void create_text_with_u8(char *text, const char *text1, uint8_t inputU8, const char *text2);

void openEdit_Gas(uint8_t line, uint8_t ccr);
void openEdit_GasType(void);

void openEdit_DiveGasSelect(uint8_t line, uint8_t ccr);
void openEdit_SpecialDiveGasMenu(uint8_t ccr);
void openEdit_DiveGasSelect_Subroutine(uint8_t line, uint8_t ccr);

    void tMEGas_check_switch_to_bailout(void);

/* Announced function prototypes -----------------------------------------------*/
uint8_t OnAction_Mix			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_GasType		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_ChangeDepth	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_SetToMOD		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_BottleSize		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

uint8_t OnAction_First			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Deco			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Travel			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Inactive		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Off			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

uint8_t OnAction_DM_Active		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DM_Mix			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

/*
uint8_t OnAction_DefaultMix		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_ToggleDepth	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_ToggleDefault  (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DefaultDepth	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
*/

/* Exported functions --------------------------------------------------------*/

void openEdit_GasCC(uint8_t line)
{
    if(actual_menu_content == MENU_SURFACE)
    {
		if(line == 6)
		{
			if(isLoopMode(settingsGetPointer()->dive_mode) || (stateUsed->diveSettings.ccrOption == 1))
			{
				selectPage(StMOG);
			}
		}
		else
		{
			openEdit_Gas(line , 1);
		}
    }
    else
    {
		if(line == 6)
		{
			openEdit_SpecialDiveGasMenu(1);
		}
		else
		{
			openEdit_DiveGasSelect(line, 1);
		}
    }
}


void openEdit_GasOC(uint8_t line)
{
    if(actual_menu_content == MENU_SURFACE)
    {
		if(line == 6)
		{
			if(isLoopMode(settingsGetPointer()->dive_mode) || (stateUsed->diveSettings.ccrOption == 1))
			{
				selectPage(StMCG);
			}
		}
		else
		{
			openEdit_Gas(line, 0);
		}
    }
    else
    if(line == 6)
    {
        openEdit_SpecialDiveGasMenu(0);
    }
    else
    {
        openEdit_DiveGasSelect(line, 0);
    }
}


/* dive mode */
void openEdit_DiveSelectBetterGas(bool doBailout)
{
    uint8_t gasId;
    if (doBailout) {
        gasId = actualBetterBailoutGasId();
    } else {
        gasId = actualBetterGasId();
    }

    uint8_t ccr = 0;
    if(gasId>5)
    {
        gasId -= 5;
        ccr = 1;
    }
    openEdit_DiveGasSelect_Subroutine(gasId,ccr);
    if(ccr)
        updateSpecificMenu(StMCG); // is this necessary? openEdit_DiveGasSelect_Subroutine has update. hw 151209
    else
        updateSpecificMenu(StMOG); // is this necessary? openEdit_DiveGasSelect_Subroutine has update. hw 151209
}


/* select gas in divemode */
void openEdit_DiveGasSelect(uint8_t line, uint8_t ccr)
{
	if(!ccr)
	{
		tMEGas_check_switch_to_bailout();
	}
	openEdit_DiveGasSelect_Subroutine(line, ccr);

    exitMenuEdit_to_Home_with_Menu_Update();
}


void openEdit_DiveGasSelect_Subroutine(uint8_t line, uint8_t ccr)
{
    uint8_t setpoint;

    editGasPage.pGasLine = (SGasLine *)stateUsed->diveSettings.gas;

    if(ccr)
    {
        editGasPage.gasID = line + NUM_OFFSET_DILUENT;
        setpoint = stateUsed->lifeData.actualGas.setPoint_cbar;
    }
    else
    {
        editGasPage.gasID = line;
        setpoint = 0;
    }

#ifdef ENABLE_UNUSED_GAS_HIDING
    if(editGasPage.pGasLine[editGasPage.gasID].note.ub.off)		/* disable selection of switched off gases */
    {
    	return;
    }
#endif
    for(int i=0;i<(1+ (2*NUM_GASES));i++)
        editGasPage.pGasLine[i].note.ub.first = 0;

    editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 1;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.first = 1;
    setActualGas_DM(&stateUsedWrite->lifeData,editGasPage.gasID,setpoint);
}

/* extra gas and gas on/off option */
void openEdit_SpecialDiveGasMenu(uint8_t ccr)
{
    char text[32];
    uint8_t oxygen, helium, gasOffset, textpointer, lineCount, ptrGas;
    //SDiveState * pState;

    editGasPage.pGasLine = (SGasLine *)stateUsed->diveSettings.gas;
    if(ccr)
    {
        editGasPage.setpoint = stateUsed->lifeData.actualGas.setPoint_cbar;
    }
    editGasPage.ccr = ccr;

    set_globalState_Menu_Line(6);

    if(ccr)
        resetMenuEdit(CLUT_MenuPageGasCC);
    else
        resetMenuEdit(CLUT_MenuPageGasOC);

    if(ccr)
        gasOffset = NUM_OFFSET_DILUENT;
    else
        gasOffset = 0;


    text[0] = '\001';
    text[1] = TXT_2BYTE;
    if(ccr)
        text[2] = TXT2BYTE_SpecialDiveGasMenuCCR;
    else
        text[2] = TXT2BYTE_SpecialDiveGasMenu;
    text[3] = 0;
    write_topline(text);

    lineCount = 1;


    for(int i=1; i<=5; i++)
    {
        ptrGas = i + gasOffset;
        oxygen = stateUsed->diveSettings.gas[ptrGas].oxygen_percentage;
        helium = stateUsed->diveSettings.gas[ptrGas].helium_percentage;

        if(oxygen == 100)
            textpointer = snprintf(text,10,"Oxy");
        else
            if((oxygen == 21) && (helium == 0))
            textpointer = snprintf(text,10,"Air");
        else
            textpointer = snprintf(text,10,"%02i/%02i",oxygen,helium);

        if(editGasPage.pGasLine[ptrGas].note.ub.first)
            strcpy(&text[textpointer]," *");

        write_field_on_off(StMOG_DM_ActiveBase+i,	30, 500, ME_Y_LINE_BASE + (lineCount * ME_Y_LINE_STEP),  &FontT48, text, stateUsed->diveSettings.gas[ptrGas].note.ub.active);
        lineCount++;
    }

    if(!ccr)
    {
        if(stateUsed->diveSettings.gas[0].note.ub.first)
        {
            oxygen = stateUsed->diveSettings.gas[0].oxygen_percentage;
            helium = stateUsed->diveSettings.gas[0].helium_percentage;
            write_field_udigit(StMOG_DM_ExtraMix, 30, 800, ME_Y_LINE6,	&FontT48, "##/##  Extra Gas *", oxygen, helium, 0, 0);
        }
        else
        {
            write_field_udigit(StMOG_DM_ExtraMix, 30, 800, ME_Y_LINE6,	&FontT48, "##/##  Extra Gas", 0, 0, 0, 0);
        }
    }

    for(int i=1; i<=5; i++)
    {
        setEvent(StMOG_DM_ActiveBase + i,(uint32_t)OnAction_DM_Active);
    }

    if(!ccr)
        setEvent(StMOG_DM_ExtraMix,(uint32_t)OnAction_DM_Mix);
}


uint8_t OnAction_DM_Active		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t  gasOffset, newActive;

    if(editGasPage.ccr)
        gasOffset = NUM_OFFSET_DILUENT;
    else
        gasOffset = 0;

    editGasPage.gasID = editId + gasOffset - StMOG_DM_ActiveBase;

    if(editGasPage.pGasLine[editGasPage.gasID].note.ub.first)
        return UNSPECIFIC_RETURN;

    if(editGasPage.pGasLine[editGasPage.gasID].note.ub.active)
        newActive = 0;
    else
        newActive = 1;

    tMenuEdit_set_on_off(editId, newActive);
    editGasPage.pGasLine[editGasPage.gasID].note.ub.active = newActive;

    return UNSPECIFIC_RETURN;
}

/* only for OC */
uint8_t OnAction_DM_Mix(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newOxygen, newHelium;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newOxygen, &newHelium, 0, 0);

        if(newOxygen < 5)
            newOxygen = 5;
        if(newOxygen == 99)
            newOxygen = 100;
        if(newHelium > 95)
            newHelium = 95;
        if((newOxygen + newHelium) > 100)
            newOxygen = 100 - newHelium;

        //SDiveState * pDiveState = 0;

        for(int i=1;i<=(2*NUM_GASES);i++)
            editGasPage.pGasLine[i].note.ub.first = 0;

        tMEGas_check_switch_to_bailout();
        setActualGas_ExtraGas(&stateUsedWrite->lifeData, newOxygen, newHelium, 0);
        return EXIT_TO_HOME;
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

    return UNSPECIFIC_RETURN;
}


void tMEGas_check_switch_to_bailout(void)
{
    if(isLoopMode(stateUsed->diveSettings.diveMode))
    {
    	stateUsedWrite->diveSettings.diveMode = DIVEMODE_OC;
        block_diluent_page();
    }
}


/* surface mode */
void openEdit_Gas(uint8_t line, uint8_t ccr)
{
    uint8_t gasID, oxygen, helium, depthDeco, active, first, depthMOD, deco, travel, inactive, off;//, bottleSizeLiter;

    char text[32];
    char textMOD[32];
    uint8_t txtptr;

    if(line)
        set_globalState_Menu_Line(line);
    else
        set_globalState_Menu_Line(NUM_GASES + 1);

    if(ccr)
        resetMenuEdit(CLUT_MenuPageGasCC);
    else
        resetMenuEdit(CLUT_MenuPageGasOC);

    if(ccr)
        gasID = line + NUM_OFFSET_DILUENT;
    else
        gasID = line;

    editGasPage.ccr = ccr;
    editGasPage.gasID = gasID;
    editGasPage.mod = calc_MOD(gasID);

    SSettings *data = settingsGetPointer();
    editGasPage.pGasLine = data->gas;

    oxygen = editGasPage.pGasLine[gasID].oxygen_percentage;
    if(oxygen > 99)
        oxygen = 99;
    helium = editGasPage.pGasLine[gasID].helium_percentage;
    depthDeco = editGasPage.pGasLine[gasID].depth_meter;
    //depthTravel = editGasPage.pGasLine[gasID].depth_meter_travel;
    active = editGasPage.pGasLine[gasID].note.ub.active;
    first = editGasPage.pGasLine[gasID].note.ub.first;
    deco = editGasPage.pGasLine[gasID].note.ub.deco;
    travel = editGasPage.pGasLine[gasID].note.ub.travel;
    off = editGasPage.pGasLine[gasID].note.ub.off;
    //bottleSizeLiter = editGasPage.pGasLine[gasID].bottle_size_liter;

    if(active)
        inactive = 0;
    else
        inactive = 1;

    depthMOD = editGasPage.mod;

    int i = 0;
    if(gasID >= 10)
    {
        i = 1;
        strcpy(text, "\001" "Gas #10 X");
    }
    else
        strcpy(text, "\001" "Gas #0 X");

    if(ccr)
        text[8+i] = TXT_Diluent_Gas_Edit;
    else
        text[8+i] = TXT_OC_Gas_Edit;

    if(gasID >= 10)
        text[6+i] += gasID - 10;
    else
        text[6+i] += gasID;

    write_topline(text);
    if(actual_menu_content == MENU_SURFACE)
    {
        write_label_fix(  20, 800, ME_Y_LINE1, &FontT48, TXT_Mix);
        write_field_udigit(StMOG_Mix, 					210, 400, ME_Y_LINE1,	&FontT48, "##/##", (uint32_t)oxygen, (uint32_t)helium, 0, 0);


        text[1] = 0;
        if(off)
        	text[0] = TXT_Off;
        else
        if(inactive)
            text[0] = TXT_Inactive;
        else
        if(first)
            text[0] = TXT_First;
        else
        if(deco)
            text[0] = TXT_Deco;
        else
        if(travel)
            text[0] = TXT_Travel;
        else
        	text[0] = TXT_Inactive;

        write_field_button(StMOG_GasType,	20, 710, ME_Y_LINE2, &FontT48, text);


        if(deco)
        {
            text[0] = TXT_ChangeDepth;
            text[1] = ' ';
            text[2] = TXT_Deco;
            text[3] = 0;
            write_label_var(  20 ,800, ME_Y_LINE3, &FontT48, text);

            textMOD[0] = '#';
            textMOD[1] = '#';
            textMOD[2] = '#';
            textMOD[3] = unit_depth_char1();
            textMOD[4] = unit_depth_char2();
            textMOD[5] = 0;
            write_field_udigit(StMOG_ChangeDepth,		600, 710, ME_Y_LINE3, &FontT48,textMOD, (uint32_t)unit_depth_integer(depthDeco), 0, 0, 0);

            txtptr = 0;
            text[txtptr++] = TXT_2BYTE;
            text[txtptr++] = TXT2BYTE_SetToMOD;
            text[txtptr++] = 0;
            write_field_button(StMOG_SetToMOD,		20, 710, ME_Y_LINE4, &FontT48,text);
        }
        else
        {
            txtptr = 0;
            text[txtptr++] = '\031';
            text[txtptr++] = TXT_ChangeDepth;
            text[txtptr++] = ' ';
            text[txtptr++] = TXT_Deco;
            text[txtptr++] = 0;
            write_label_var(  20 ,800, ME_Y_LINE3, &FontT48, text);

            txtptr = 0;
            text[txtptr++] = '\031';
            text[txtptr++] = TXT_2BYTE;
            text[txtptr++] = TXT2BYTE_SetToMOD;
            text[txtptr++] = 0;
            write_label_var(  20 ,800, ME_Y_LINE4, &FontT48, text);
        }
/*
        txtptr = 0;
        text[txtptr++] = TXT_2BYTE;
        text[txtptr++] = TXT2BYTE_Bottle;
        text[txtptr++] = 0;
        write_label_var(  20 ,800, ME_Y_LINE5, &FontT48, text);
        write_field_2digit(StMOG_Bottle,		600, 710, ME_Y_LINE5, &FontT48,"## ltr", (uint32_t)bottleSizeLiter, 0, 0, 0);

*/
        stop_cursor_fields();

        textMOD[0] = '#';
        textMOD[1] = '#';
        textMOD[2] = '#';
        textMOD[3] = unit_depth_char1();
        textMOD[4] = unit_depth_char2();
        textMOD[5] = ' ';
        textMOD[6] = 'M';
        textMOD[7] = 'O';
        textMOD[8] = 'D';
        textMOD[9] = 0;

        write_field_udigit(StMOG_MOD,						401, 780, ME_Y_LINE1,	&FontT48, textMOD, (uint32_t)unit_depth_integer(depthMOD), 0, 0, 0);
//		write_field_udigit(StMOG_MOD,						401, 780, ME_Y_LINE1,	&FontT48, "###m MOD", (uint32_t)depthMOD, 0, 0, 0);

        setEvent(StMOG_Mix, 					(uint32_t)OnAction_Mix);
        setEvent(StMOG_GasType,				(uint32_t)OnAction_GasType);

        if(deco)
        {
            setEvent(StMOG_ChangeDepth,		(uint32_t)OnAction_ChangeDepth);
            setEvent(StMOG_SetToMOD,		(uint32_t)OnAction_SetToMOD);
        }
/*
        setEvent(StMOG_Bottle, 				(uint32_t)OnAction_BottleSize);
*/
        write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
    }
}

/* surface mode */
void openEdit_GasType(void)
{
    uint8_t gasID, active, first, deco, travel, inactive, off;
    char text[32];


    if(editGasPage.ccr)
    {
        resetMenuEdit(CLUT_MenuPageGasCC);
        setBackMenu((uint32_t)openEdit_GasCC, editGasPage.gasID - NUM_OFFSET_DILUENT, 2);
    }
    else
    {
        resetMenuEdit(CLUT_MenuPageGasOC);
        setBackMenu((uint32_t)openEdit_GasOC, editGasPage.gasID, 2);
    }

    gasID = editGasPage.gasID;
    active = editGasPage.pGasLine[gasID].note.ub.active;
    first = editGasPage.pGasLine[gasID].note.ub.first;
    deco = editGasPage.pGasLine[gasID].note.ub.deco;
    travel = editGasPage.pGasLine[gasID].note.ub.travel;
    off = editGasPage.pGasLine[gasID].note.ub.off;

    if((active) || (off))
        inactive = 0;
    else
        inactive = 1;


    /* header */
    int i = 0;
    if(gasID >= 10)
    {
        i = 1;
        strcpy(text, "\001" "Gas #10 X");
    }
    else
        strcpy(text, "\001" "Gas #0 X");

    if(editGasPage.ccr)
        text[8+i] = TXT_Diluent_Gas_Edit;
    else
        text[8+i] = TXT_OC_Gas_Edit;

    if(gasID >= 10)
        text[6+i] += gasID - 10;
    else
        text[6+i] += gasID;
    write_topline(text);

    text[1] = 0;
    text[0] = TXT_First;
    write_field_on_off(StMOG_First,     30, 400, ME_Y_LINE1, &FontT48, text, first);

    text[0] = TXT_Deco;
    write_field_on_off(StMOG_Deco,      30, 400, ME_Y_LINE2, &FontT48, text, deco);

    text[0] = TXT_Travel;
    write_field_on_off(StMOG_Travel,    30, 400, ME_Y_LINE3, &FontT48, text, travel);

    text[0] = TXT_Inactive;
    write_field_on_off(StMOG_Inactive,  30, 400, ME_Y_LINE4, &FontT48, text, inactive);

#ifdef ENABLE_UNUSED_GAS_HIDING
    text[0] = TXT_Off;
    write_field_on_off(StMOG_Off,  30, 400, ME_Y_LINE5, &FontT48, text, off);
#endif

    setEvent(StMOG_First,               (uint32_t)OnAction_First);
    setEvent(StMOG_Deco,                (uint32_t)OnAction_Deco);
    setEvent(StMOG_Travel,				(uint32_t)OnAction_Travel);
    setEvent(StMOG_Inactive,			(uint32_t)OnAction_Inactive);
#ifdef ENABLE_UNUSED_GAS_HIDING
    setEvent(StMOG_Off,			(uint32_t)OnAction_Off);
#endif

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}

uint8_t OnAction_GasType(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    openEdit_GasType();
    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_Mix(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newOxygen, newHelium;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newOxygen, &newHelium, 0, 0);

        if(newOxygen < 5)
            newOxygen = 5;
        if(newOxygen == 99)
            newOxygen = 100;
        if(newHelium > 95)
            newHelium = 95;
        if((newOxygen + newHelium) > 100)
            newOxygen = 100 - newHelium;

        editGasPage.pGasLine[editGasPage.gasID].oxygen_percentage = newOxygen;
        editGasPage.pGasLine[editGasPage.gasID].helium_percentage = newHelium;
        editGasPage.mod = calc_MOD(editGasPage.gasID);

        if(newOxygen == 100)
            newOxygen = 99;

        tMenuEdit_newInput(editId, newOxygen, newHelium, 0, 0);
        tMenuEdit_newInput(StMOG_MOD, (uint32_t)unit_depth_integer(editGasPage.mod), 0, 0, 0);

        return UPDATE_DIVESETTINGS;
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

    return UNSPECIFIC_RETURN;
}

/*
uint8_t OnAction_DefaultMix(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint32_t newOxygen, newHelium;
    uint8_t depthDEFAULT;
    char text[32];

    newOxygen = 21;
    newHelium = 0;

    editGasPage.pGasLine[editGasPage.gasID].oxygen_percentage = newOxygen;
    editGasPage.pGasLine[editGasPage.gasID].helium_percentage = newHelium;

    tMenuEdit_newInput(StMOG_Mix, newOxygen, newHelium, 0, 0);

    depthDEFAULT = calc_MOD(editGasPage.gasID);
    create_text_with_u8(text, "", depthDEFAULT, "m MOD");
    tMenuEdit_newButtonText(StMOG_DefaultDepth, text);

    return UPDATE_DIVESETTINGS;
}

uint8_t OnAction_ToggleDepth(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t depth;
    uint32_t newDefaultDepth;
    char text[32];

    if(editGasPage.depth_mode == 0)
    {
        editGasPage.depth_mode = 1;
        depth = editGasPage.pGasLine[editGasPage.gasID].depth_meter_travel;
        newDefaultDepth = calc_MinOD(editGasPage.gasID);
        create_text_with_u8(text, "", newDefaultDepth, "m Min");
    }
    else
    {
        editGasPage.depth_mode = 0;
        depth = editGasPage.pGasLine[editGasPage.gasID].depth_meter;
        newDefaultDepth = calc_MOD(editGasPage.gasID);
        create_text_with_u8(text, "", newDefaultDepth, "m MOD");
    }
    tMenuEdit_newInput(StMOG_Depth, (uint32_t)depth, 0, 0, 0);
    tMenuEdit_newButtonText(StMOG_DefaultDepth, text);

    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_Depth(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newDepth;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newDepth, 0, 0, 0);
        if(newDepth > 255)
            newDepth = 255;
        if(editGasPage.depth_mode == 0)
            editGasPage.pGasLine[editGasPage.gasID].depth_meter = newDepth;
        else
            editGasPage.pGasLine[editGasPage.gasID].depth_meter_travel = newDepth;
        tMenuEdit_newInput(editId, newDepth, 0, 0, 0);
        return UPDATE_DIVESETTINGS;
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

    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_DefaultDepth(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint32_t newDepth;

    if(editGasPage.depth_mode == 0)
    {
        newDepth = calc_MOD(editGasPage.gasID);
        editGasPage.pGasLine[editGasPage.gasID].depth_meter = newDepth;
    }
    else
    {
        newDepth = calc_MinOD(editGasPage.gasID);
        editGasPage.pGasLine[editGasPage.gasID].depth_meter_travel = newDepth;
    }

    tMenuEdit_newInput(StMOG_Depth, newDepth, 0, 0, 0);

    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_Active(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t active, first;

    first = editGasPage.pGasLine[editGasPage.gasID].note.ub.first;

    if(first)
        return UNSPECIFIC_RETURN;

    active = editGasPage.pGasLine[editGasPage.gasID].note.ub.active;

    if(active)
    {
        active = 0;
        editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 0;
    }
    else
    {
        active = 1;
        editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 1;
    }
    tMenuEdit_set_on_off(editId, active);

    return UPDATE_DIVESETTINGS;
}
*/


uint8_t OnAction_First(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t first, i, gasOne;

    if(editGasPage.ccr)
        gasOne = 1 + NUM_OFFSET_DILUENT;
    else
        gasOne = 1;

    first = editGasPage.pGasLine[editGasPage.gasID].note.ub.first;

    if(!first)
    {
        for(i=gasOne;i<NUM_GASES + gasOne;i++)
        {
            if(editGasPage.pGasLine[i].note.ub.first)
            {
                editGasPage.pGasLine[i].note.ub.first = 0;
                editGasPage.pGasLine[i].note.ub.active = 0;
            }
        }
    }

    editGasPage.pGasLine[editGasPage.gasID].note.ub.first = 1;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 1;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.deco = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.travel = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.off = 0;
    editGasPage.pGasLine[editGasPage.gasID].depth_meter = 0;
    editGasPage.pGasLine[editGasPage.gasID].depth_meter_travel = 0;

    tMenuEdit_set_on_off(StMOG_First, 1);
    tMenuEdit_set_on_off(StMOG_Deco, 0);
    tMenuEdit_set_on_off(StMOG_Travel, 0);
    tMenuEdit_set_on_off(StMOG_Inactive, 0);
#ifdef ENABLE_UNUSED_GAS_HIDING
    tMenuEdit_set_on_off(StMOG_Off, 0);
#endif

    if(!first)
        return UPDATE_DIVESETTINGS;
    else
        return UPDATE_AND_EXIT_TO_MENU;
}


uint8_t OnAction_Deco(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t  first, deco, gasOne;

    if(editGasPage.ccr)
        gasOne = 1 + NUM_OFFSET_DILUENT;
    else
        gasOne = 1;

    first = editGasPage.pGasLine[editGasPage.gasID].note.ub.first;
    deco = editGasPage.pGasLine[editGasPage.gasID].note.ub.deco;

    if(first)
    {
        if(editGasPage.gasID == gasOne)
            return UNSPECIFIC_RETURN;
        else
        {
            editGasPage.pGasLine[gasOne].note.ub.first = 1;
            editGasPage.pGasLine[gasOne].note.ub.active = 1;
            editGasPage.pGasLine[gasOne].note.ub.deco = 0;
            editGasPage.pGasLine[gasOne].note.ub.travel = 0;
            editGasPage.pGasLine[gasOne].depth_meter = 0;
            editGasPage.pGasLine[gasOne].depth_meter_travel = 0;
        }
    }

    editGasPage.pGasLine[editGasPage.gasID].note.ub.first = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 1;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.deco = 1;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.travel = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.off = 0;
    editGasPage.pGasLine[editGasPage.gasID].depth_meter_travel = 0;
    editGasPage.mod = calc_MOD(editGasPage.gasID);
    editGasPage.pGasLine[editGasPage.gasID].depth_meter = editGasPage.mod;

/*
    if(deco)
    {
        editGasPage.pGasLine[editGasPage.gasID].depth_meter = editGasPage.mod;
        tMenuEdit_newInput(StMOG_ChangeDepth, editGasPage.pGasLine[editGasPage.gasID].depth_meter, 0, 0, 0);
    }
*/

    tMenuEdit_set_on_off(StMOG_First, 0);
    tMenuEdit_set_on_off(StMOG_Deco, 1);
    tMenuEdit_set_on_off(StMOG_Travel, 0);
    tMenuEdit_set_on_off(StMOG_Inactive, 0);
#ifdef ENABLE_UNUSED_GAS_HIDING
    tMenuEdit_set_on_off(StMOG_Off, 0);
#endif

    if(!deco)
        return UPDATE_DIVESETTINGS;
    else
        return UPDATE_AND_EXIT_TO_MENU;
}


uint8_t OnAction_Travel(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t first, travel, gasOne;

    if(editGasPage.ccr)
        gasOne = 1 + NUM_OFFSET_DILUENT;
    else
        gasOne = 1;

    first = editGasPage.pGasLine[editGasPage.gasID].note.ub.first;
    travel = editGasPage.pGasLine[editGasPage.gasID].note.ub.travel;

    if(first)
    {
        if(editGasPage.gasID == gasOne)
            return UNSPECIFIC_RETURN;
        else
        {
            editGasPage.pGasLine[gasOne].note.ub.first = 1;
            editGasPage.pGasLine[gasOne].note.ub.active = 1;
            editGasPage.pGasLine[gasOne].note.ub.deco = 0;
            editGasPage.pGasLine[gasOne].note.ub.travel = 0;
            editGasPage.pGasLine[gasOne].depth_meter = 0;
            editGasPage.pGasLine[gasOne].depth_meter_travel = 0;
        }
    }

    editGasPage.pGasLine[editGasPage.gasID].note.ub.first = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 1;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.deco = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.travel = 1;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.off = 0;
    editGasPage.pGasLine[editGasPage.gasID].depth_meter = 0;
    editGasPage.pGasLine[editGasPage.gasID].depth_meter_travel = 0;

    tMenuEdit_set_on_off(StMOG_First, 0);
    tMenuEdit_set_on_off(StMOG_Deco, 0);
    tMenuEdit_set_on_off(StMOG_Travel, 1);
    tMenuEdit_set_on_off(StMOG_Inactive, 0);
#ifdef ENABLE_UNUSED_GAS_HIDING
    tMenuEdit_set_on_off(StMOG_Off, 0);
#endif

    if(!travel)
        return UPDATE_DIVESETTINGS;
    else
        return UPDATE_AND_EXIT_TO_MENU;
}


uint8_t OnAction_Inactive(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t first, inactive, gasOne;

    if(editGasPage.ccr)
        gasOne = 1 + NUM_OFFSET_DILUENT;
    else
        gasOne = 1;

    first = editGasPage.pGasLine[editGasPage.gasID].note.ub.first;

    if(editGasPage.pGasLine[editGasPage.gasID].note.ub.active)
        inactive = 0;
    else
        inactive = 1;

    if(first)
    {
        if(editGasPage.gasID == gasOne)
            return UNSPECIFIC_RETURN;
        else
        {
            editGasPage.pGasLine[gasOne].note.ub.first = 1;
            editGasPage.pGasLine[gasOne].note.ub.active = 1;
            editGasPage.pGasLine[gasOne].note.ub.deco = 0;
            editGasPage.pGasLine[gasOne].note.ub.travel = 0;
            editGasPage.pGasLine[gasOne].depth_meter = 0;
            editGasPage.pGasLine[gasOne].depth_meter_travel = 0;
        }
    }

    editGasPage.pGasLine[editGasPage.gasID].note.ub.first = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.deco = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.travel = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.off = 0;

    tMenuEdit_set_on_off(StMOG_First, 0);
    tMenuEdit_set_on_off(StMOG_Deco, 0);
    tMenuEdit_set_on_off(StMOG_Travel, 0);
    tMenuEdit_set_on_off(StMOG_Inactive, 1);
#ifdef ENABLE_UNUSED_GAS_HIDING
    tMenuEdit_set_on_off(StMOG_Off, 0);
#endif
    if(!inactive)
        return UPDATE_DIVESETTINGS;
    else
        return UPDATE_AND_EXIT_TO_MENU;
}

uint8_t OnAction_Off			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t first, off, gasOne;

    if(editGasPage.ccr)
        gasOne = 1 + NUM_OFFSET_DILUENT;
    else
        gasOne = 1;

    first = editGasPage.pGasLine[editGasPage.gasID].note.ub.first;
    off =editGasPage.pGasLine[editGasPage.gasID].note.ub.off;

    if(first)
    {
        if(editGasPage.gasID == gasOne)
            return UNSPECIFIC_RETURN;
        else
        {
            editGasPage.pGasLine[gasOne].note.ub.first = 1;
            editGasPage.pGasLine[gasOne].note.ub.active = 1;
            editGasPage.pGasLine[gasOne].note.ub.deco = 0;
            editGasPage.pGasLine[gasOne].note.ub.travel = 0;
            editGasPage.pGasLine[gasOne].depth_meter = 0;
            editGasPage.pGasLine[gasOne].depth_meter_travel = 0;
        }
    }

    editGasPage.pGasLine[editGasPage.gasID].note.ub.first = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.active = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.deco = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.travel = 0;
    editGasPage.pGasLine[editGasPage.gasID].note.ub.off = 1;

    tMenuEdit_set_on_off(StMOG_First, 0);
    tMenuEdit_set_on_off(StMOG_Deco, 0);
    tMenuEdit_set_on_off(StMOG_Travel, 0);
    tMenuEdit_set_on_off(StMOG_Inactive, 0);
#ifdef ENABLE_UNUSED_GAS_HIDING
    tMenuEdit_set_on_off(StMOG_Off, 1);
#endif
    if(!off)
        return UPDATE_DIVESETTINGS;
    else
        return UPDATE_AND_EXIT_TO_MENU;
}

uint8_t OnAction_SetToMOD	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t newChangeDepth = editGasPage.mod;

    editGasPage.pGasLine[editGasPage.gasID].depth_meter = newChangeDepth;
    tMenuEdit_newInput(StMOG_ChangeDepth, unit_depth_integer(newChangeDepth), 0, 0, 0);

    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_ChangeDepth(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newDepth;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newDepth, 0, 0, 0);
        if(settingsGetPointer()->nonMetricalSystem != 0) // new hw 170703
        {
            newDepth += 2; // fï¿½r rundung
            newDepth = (newDepth * 3) / 10;
        }
        if(newDepth > 255)
            newDepth = 255;
        editGasPage.pGasLine[editGasPage.gasID].depth_meter = newDepth;
        tMenuEdit_newInput(editId, unit_depth_integer(newDepth), 0, 0, 0);
        return UPDATE_DIVESETTINGS;
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

    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_BottleSize		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    int8_t digitContentNew;
    uint32_t newBottleSize;
//	const uint8_t validSize = {4,7,8,10,12,15,16,17,20,24,30,40,255};
    const uint8_t validSize[] = {40,30,24,20,17,16,15,12,10,8,7,4,0};

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newBottleSize, 0, 0, 0);

        if(newBottleSize > validSize[0])
            newBottleSize = validSize[0];

        editGasPage.pGasLine[editGasPage.gasID].bottle_size_liter = newBottleSize;

        tMenuEdit_newInput(editId, newBottleSize, 0, 0, 0);
        return UPDATE_DIVESETTINGS;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        // new here for ease of comparison
        digitContentNew = digitContent - '0';

        if(digitContentNew > 0)
        {
            int i = 0;
            while(digitContentNew < validSize[i])
            {
                i++;
            }
            if(i == 0)
                digitContentNew = 0; // off
            else
                digitContentNew = validSize[i-1];
        }
        else
        {
            int i = 0;
            while(validSize[i] != 0)
                i++;
            digitContentNew = validSize[i-1]; // smallest tank
        }
        digitContentNew += '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        // new here for ease of comparison
        digitContentNew = digitContent - '0';
        if(digitContentNew > 0)
        {
            int i = 0;
            while(digitContentNew < validSize[i])
            {
                i++;
            }
            if(validSize[i] == 0)
                digitContentNew = 0; // off
            else if(validSize[i+1] == 0)
                digitContentNew = 0; // off
            else
                digitContentNew = validSize[i+1];
        }
        else
        {
            digitContentNew = validSize[0];
        }
        digitContentNew += '0';

        return digitContentNew;
    }
    return EXIT_TO_MENU;
}


/* Private functions ---------------------------------------------------------*/


void	create_text_with_u8(char *text, const char *text1, uint8_t inputU8, const char *text2)
{
    uint8_t digit1, digit2, digit3, count;

    count = 0;

    if(*text1)
    {
        strcpy(&text[count], text1);
        count = strlen(text1);
    }

    digit1 = inputU8 / 100;
    inputU8 -=  digit1 * 100;
    digit2 = inputU8 / 10;
    inputU8 -=  digit2 * 10;
    digit3 = inputU8;
    if(digit1)
        text[count++] = '0' + digit1;
    if(count || digit2)
        text[count++] = '0' + digit2;
    text[count++] = '0' + digit3;

    if(*text2)
    {
        strcpy(&text[count], text2);
    }
}
