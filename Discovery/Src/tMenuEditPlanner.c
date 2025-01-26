///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditPlanner.c
/// \brief  Menu Edit Planner Parameters
/// \author heinrichs weikamp gmbh
/// \date   08-Jan-2015
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
#include "tMenuEditPlanner.h"

#include "gfx_fonts.h"
#include "ostc.h"		// for MX_Bluetooth_PowerOff();
#include "simulation.h"
#include "tHome.h"	//for tHome_gas_writer()
#include "tMenuEdit.h"
#include "unit.h"

/* Private types -------------------------------------------------------------*/
uint8_t resultPage = 0;
int8_t first = 0;

SDecoinfo* pDecoinfo;

uint16_t tMplan_pGasConsumption[6] = {0,0,0,0,0,0};
SSimDataSummary tMplan_Summary = {0};

/* Importend function prototypes ---------------------------------------------*/
extern uint8_t write_gas(char *text, uint8_t oxygen, uint8_t helium);

/* Imported variables --------------------------------------------------------*/

extern uint16_t tMplan_depth_meter, tMplan_intervall_time_minutes, tMplan_dive_time_minutes, tMplan_depth_editor;

/* Private variables ---------------------------------------------------------*/
uint8_t gasChangeListDepthGasId[40];

/* Private function prototypes -----------------------------------------------*/
void openEdit_PlanInterval(void);
void openEdit_PlanDiveTime(void);
void openEdit_PlanMaxDepth(void);
void openEdit_PlanSettings(void);
void openEdit_PlanResult(void);

/* Announced function prototypes -----------------------------------------------*/
uint8_t OnAction_PlanInterval		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_PlanDiveTime		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_PlanMaxDepth		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_PlanResultExit	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_PlanSettings		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

/* Exported functions --------------------------------------------------------*/

void openEdit_Planner(uint8_t line)
{
    set_globalState_Menu_Line(line);
    resetMenuEdit(CLUT_MenuPageDivePlanner);
    resultPage = 0;

    switch(line)
    {
    case 1:
    default:
        if(settingsGetPointer()->bluetoothActive != 0)
        {
            settingsGetPointer()->bluetoothActive = 0;
            MX_Bluetooth_PowerOff();
        }

        disableTimer();

        simulation_start(tMplan_depth_meter, tMplan_dive_time_minutes);
        exitMenuEdit_to_Home();
        break;
    case 2:
        openEdit_PlanInterval();
        break;
    case 3:
        openEdit_PlanDiveTime();
        break;
    case 4:
        openEdit_PlanMaxDepth();
        break;
    case 5:
        openEdit_PlanResult();
        break;
    case 6:
        openEdit_PlanSettings();
        break;
}
}

/* Private functions ---------------------------------------------------------*/

void openEdit_PlanInterval(void)
{
    char text[32];

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Simulator;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_Intervall;
    text[2] = 0;
    write_label_var(  20, 550, ME_Y_LINE2, &FontT48, text);

    write_field_udigit(StMPLAN2_Interval, 400, 800, ME_Y_LINE2, &FontT48, "###\016\016min\017", (uint32_t)tMplan_intervall_time_minutes, 0, 0, 0);
    setEvent(StMPLAN2_Interval, (uint32_t)OnAction_PlanInterval);
    startEdit();
}


void openEdit_PlanDiveTime(void)
{
    char text[32];

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Simulator;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_SimDiveTime;
    text[2] = 0;
    write_label_var(  20, 550, ME_Y_LINE3, &FontT48, text);

    write_field_udigit(StMPLAN3_DiveTime, 400, 800, ME_Y_LINE3, &FontT48, "###\016\016min\017", (uint32_t)tMplan_dive_time_minutes, 0, 0, 0);
    setEvent(StMPLAN3_DiveTime, (uint32_t)OnAction_PlanDiveTime);
    startEdit();
}


void openEdit_PlanMaxDepth(void)
{
    char text[32];
    tMplan_depth_editor = unit_depth_integer(tMplan_depth_meter);

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Simulator;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_SimMaxDepth;
    text[2] = 0;
    write_label_var(  20, 550, ME_Y_LINE4, &FontT48, text);

    if(settingsGetPointer()->nonMetricalSystem)
    {
    	write_field_udigit(StMPLAN4_MaxDepth, 400, 800, ME_Y_LINE4, &FontT48, "###\016\016ft\017", (uint32_t)tMplan_depth_editor, 0, 0, 0);
    }
    else
    {
    	write_field_udigit(StMPLAN4_MaxDepth, 400, 800, ME_Y_LINE4, &FontT48, "###\016\016m\017", (uint32_t)tMplan_depth_editor, 0, 0, 0);
    }

    setEvent(StMPLAN4_MaxDepth, (uint32_t)OnAction_PlanMaxDepth);
    startEdit();
}


uint8_t OnAction_PlanInterval	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint32_t newValue;

    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newValue, 0, 0, 0);
        tMplan_intervall_time_minutes = newValue;
        return EXIT_TO_MENU;
    }
    else
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContent++;
        if(digitContent > '9')
            digitContent = '0';
    }
    else
    if(action == ACTION_BUTTON_BACK)
    {
        digitContent--;
        if(digitContent < '0')
            digitContent = '9';
    }
    return digitContent;
}


uint8_t OnAction_PlanDiveTime	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint32_t newValue;

    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newValue, 0, 0, 0);
        tMplan_dive_time_minutes = newValue;
        return EXIT_TO_MENU;
    }
    else
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContent++;
        if((digitNumber == 0) && (digitContent > '3'))
            digitContent = '0';
        if(digitContent > '9')
            digitContent = '0';
    }
    else
    if(action == ACTION_BUTTON_BACK)
    {
        digitContent--;
        if((digitNumber == 0) && (digitContent < '0'))
            digitContent = '3';
        if(digitContent < '0')
            digitContent = '9';
    }
    return digitContent;
}


uint8_t OnAction_PlanMaxDepth	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint32_t newValue;

    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newValue, 0, 0, 0);
        if(settingsGetPointer()->nonMetricalSystem)
        {
        	tMplan_depth_editor = newValue * 10 / 33;
        }
        else
        {
        	tMplan_depth_editor = newValue;
        }
        tMplan_depth_meter = tMplan_depth_editor;
        return EXIT_TO_MENU;
    }
    else
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContent++;
        if((digitNumber == 0) && (digitContent > '3'))
            digitContent = '0';
        if(digitContent > '9')
            digitContent = '0';
    }
    else
    if(action == ACTION_BUTTON_BACK)
    {
        digitContent--;
        if((digitNumber == 0) && (digitContent < '0'))
            digitContent = '3';
        if(digitContent < '0')
            digitContent = '9';
    }
    return digitContent;
}


void openEdit_PlanSettings(void)
{
    uint8_t travel_lbar, deco_lbar;
    uint16_t y_line;
    char text[40];
    SSettings *pSettings = settingsGetPointer();

    travel_lbar = pSettings->gasConsumption_travel_l_min;
    deco_lbar = pSettings->gasConsumption_deco_l_min;

    y_line = ME_Y_LINE_BASE + (6 * ME_Y_LINE_STEP);

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_SimConsumption;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_SimConsumption;
    text[2] = 0;
    write_label_var(  20, 800, y_line, &FontT48, text);

    strcpy(text,
        "  "
        "\016\016"
        "  l/min"
        "\017"
        "  "
        "\016\016"
        " deco"
        "\017"
        "   "
        "\016\016"
        "   l/min"
        "\017"
    );
    write_label_var(  400, 800, y_line, &FontT48, text);

    write_field_udigit(StMPLAN4_Settings,		400, 800, y_line, &FontT48, "##            ##", (uint32_t)travel_lbar, (uint32_t)deco_lbar, 0, 0);
    // note : text max is 32 byte! -> ok and it does not like small fonts in between -> problem
    write_buttonTextline(TXT2BYTE_ButtonMinus,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonPlus);

    setEvent(StMPLAN4_Settings,			(uint32_t)OnAction_PlanSettings);
    startEdit();
}


uint8_t OnAction_PlanSettings		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint32_t newValueTravel, newValueDeco;

    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newValueTravel, &newValueDeco, 0, 0);
        if(newValueTravel < 5 )
            newValueTravel = 5;
        if(newValueTravel > 30 )
            newValueTravel = 30;
        if(newValueDeco < 5 )
            newValueDeco = 5;
        if(newValueDeco > 30 )
            newValueDeco = 30;

        settingsGetPointer()->gasConsumption_travel_l_min = (uint8_t)newValueTravel;
        settingsGetPointer()->gasConsumption_deco_l_min = (uint8_t)newValueDeco;

        return EXIT_TO_MENU;
    }
    else if(action == ACTION_BUTTON_NEXT)
    {
        digitContent++;
        if((digitNumber == 0) && (digitContent > '3'))
            digitContent = '0';
        if(digitContent > '9')
            digitContent = '0';
    }
    else if(action == ACTION_BUTTON_BACK)
    {
        digitContent--;
        if((digitNumber == 0) && (digitContent < '0'))
            digitContent = '3';
        if(digitContent < '0')
            digitContent = '9';
    }

    return digitContent;
}


void openEdit_PlanResult(void)
{
    char text[256];

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Calculating;
    text[3] = 0;

    write_topline(text);

    text[2] = TXT2BYTE_PleaseWait;
    write_label_var( 10, 790, ME_Y_LINE1, &FontT42, text);

    SSettings *pSettings = settingsGetPointer();
    uint8_t tMplan_gasConsumTravel = pSettings->gasConsumption_travel_l_min;
    uint8_t tMplan_gasConsumDeco = pSettings->gasConsumption_deco_l_min;

    resultPage = 0;
    pDecoinfo = simulation_decoplaner(tMplan_depth_meter, tMplan_intervall_time_minutes, tMplan_dive_time_minutes, gasChangeListDepthGasId);
    simulation_gas_consumption(tMplan_pGasConsumption, tMplan_depth_meter, tMplan_dive_time_minutes, pDecoinfo, tMplan_gasConsumTravel, tMplan_gasConsumDeco, gasChangeListDepthGasId);
    simulation_helper_change_points(&tMplan_Summary, tMplan_depth_meter, tMplan_dive_time_minutes, pDecoinfo, gasChangeListDepthGasId);

    first = 0;
    while((first < DECOINFO_STRUCT_MAX_STOPS-1) && pDecoinfo->output_stop_length_seconds[first+1])
        first++;
    resultPage = 1;

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_ButtonNext;
    text[2] = 0;
    write_field_button(StMPLAN5_ExitResult, 30, 800, ME_Y_LINE6,  &FontT48, text);
    setEvent(StMPLAN5_ExitResult, (uint32_t)OnAction_PlanResultExit);
    tMenuEdit_select(StMPLAN5_ExitResult);
}



void refresh_PlanResult_helper(char *text, int start)
{
    uint8_t depthPrev, depthNext, depthLast, depthSecond, depthInc, depthChange, GasIdPrev, GasIdNextList[3], ListCount, oxygen_percentage, textptr;
    int lengthInMinutes;

    depthLast 		= (uint8_t)(stateUsed->diveSettings.last_stop_depth_bar * 10);
    depthSecond 	= (uint8_t)(stateUsed->diveSettings.input_second_to_last_stop_depth_bar * 10);
    depthInc 			= (uint8_t)(stateUsed->diveSettings.input_next_stop_increment_depth_bar * 10);


    if((start < 0) || (start >= DECOINFO_STRUCT_MAX_STOPS))
    {
        *text = 0;
        return;
    }

    if(start == 0)
    {
        depthNext = depthLast;
        depthPrev = depthSecond;
    }
    else if(start == 1)
    {
        depthNext = depthSecond;
        depthPrev = depthSecond + depthInc;
    }
    else
    {
        depthNext = depthSecond + (start - 1) * depthInc;
        depthPrev = depthNext + depthInc;
    }

    /* gas changes ? */
    GasIdPrev = 0;
    ListCount = 0;

    for(int i = 1; i < BUEHLMANN_STRUCT_MAX_GASES; i++)
    {
        if((stateSimGetPointer()->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero == 0)
#ifdef ENABLE_DECOCALC_OPTION
        	|| (stateSimGetPointer()->diveSettings.gas[stateSimGetPointer()->diveSettings.decogaslist[i].GasIdInSettings].note.ub.decocalc == 0)
#endif
        )
                break;
        depthChange = stateSimGetPointer()->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero;
        if(depthPrev <= depthChange)
        {
            GasIdPrev = i;
        }
        else
        {
                 break;
        }
    }

    for(int i = GasIdPrev + 1; i < BUEHLMANN_STRUCT_MAX_GASES; i++)
    {
            if((stateSimGetPointer()->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero == 0)
#ifdef ENABLE_DECOCALC_OPTION
            	|| (stateSimGetPointer()->diveSettings.gas[stateSimGetPointer()->diveSettings.decogaslist[i].GasIdInSettings].note.ub.decocalc == 0)
#endif
				)
                    break;
            depthChange = stateSimGetPointer()->diveSettings.decogaslist[i].change_during_ascent_depth_meter_otherwise_zero;
            if((depthChange < depthPrev) && (depthChange >= depthNext))
            {
                GasIdNextList[ListCount++] = i;
                if(ListCount > 3)
                    break;
            }
    }

    /* text output */
    if(pDecoinfo->output_stop_length_seconds[start])
    {
        textptr = snprintf(text,20,"\034%2u\016\016m\017 ",depthNext);
        lengthInMinutes = (pDecoinfo->output_stop_length_seconds[start]+59)/60;

        int i = 0;
        while((i<lengthInMinutes) && (i<15))
        {
            text[textptr++] = '_';
            i++;
        }
        for(int j=i;j<15;j++)
        {
            text[textptr++] = '\177';
            text[textptr++] = '_';
        }
        textptr += snprintf(&text[textptr],20," %3i'", lengthInMinutes);
    }
    else
        textptr = snprintf(text,20,"\031\034%2u\016\016m\017 ",depthNext);

    for(int i = 0; i < ListCount; i++)
    {
        if(stateSimGetPointer()->diveSettings.decogaslist[GasIdNextList[i]].setPoint_cbar != stateSimGetPointer()->diveSettings.decogaslist[GasIdPrev].setPoint_cbar)
            snprintf(&text[textptr],20," %01.2f ", ((float)(stateSimGetPointer()->diveSettings.decogaslist[GasIdNextList[i]].setPoint_cbar))/100);
        else
        {
            oxygen_percentage = 100;
            oxygen_percentage -= stateSimGetPointer()->diveSettings.decogaslist[GasIdNextList[i]].nitrogen_percentage;
            oxygen_percentage -= stateSimGetPointer()->diveSettings.decogaslist[GasIdNextList[i]].helium_percentage;

            text[textptr++] = ' ';
            textptr += tHome_gas_writer(oxygen_percentage, stateSimGetPointer()->diveSettings.decogaslist[GasIdNextList[i]].helium_percentage, &text[textptr]);
            text[textptr++] = ' ';
            text[textptr] = 0;
        }
        GasIdPrev = GasIdNextList[i];
    }
}


void refresh_PlanResult(void)
{
    char text[256];
    uint8_t textpointer;
    uint16_t y_line;
    uint8_t oxygen_percentage;
    int now;

    if(resultPage < 0xF0)
    {
        textpointer = snprintf(text,256,"\001 %u' @ %um",tMplan_dive_time_minutes,tMplan_depth_meter);
        if(tMplan_intervall_time_minutes)
            snprintf(&text[textpointer],256-textpointer," in %u'",tMplan_intervall_time_minutes);
    }
    else if(resultPage == 0xFE)
    {
        textpointer = snprintf(text,30,"%c%c", TXT_2BYTE, TXT2BYTE_SimConsumption);
    }
    else
    {
        textpointer = snprintf(text,30,"%c%c", TXT_2BYTE, TXT2BYTE_SimSummary);
    }
    write_topline(text);

    switch (resultPage)
    {
    case 0:
        break;
    case 0xFF: // summary
        for(int j=0;j<4;j++)
        {
            y_line = ME_Y_LINE_BASE + ((j + 1) * ME_Y_LINE_STEP);

            // text
            textpointer = 0;
            *text = 0;
            text[textpointer] = 0;
            text[textpointer++] = TXT_2BYTE;
            text[textpointer++] = TXT2BYTE_SimDecTo + j; // see text_multilanguage.h
            text[textpointer] = 0;
            write_label_var(  10, 200, y_line, &FontT42, text);

            // depth
            textpointer = 0;
            *text = 0;
            switch(j)
            {
            case 0: // descent
            case 1: // level
                textpointer = snprintf(&text[textpointer],20,"%u\016\016 m\017",tMplan_depth_meter);
                break;
            case 2: // first stop
                textpointer = snprintf(&text[textpointer],20,"%u\016\016 m\017",tMplan_Summary.depthMeterFirstStop);
                break;
            default:
                break;
            }
            text[textpointer] = 0;
            write_label_var( 180, 400, y_line, &FontT42, text);

            // total time
            textpointer = 0;
            *text = 0;
            switch(j)
            {
            case 0: // descent
                textpointer = snprintf(&text[textpointer],20,"(%u')",tMplan_Summary.timeToBottom);
                break;
            case 1: // level
                textpointer = snprintf(&text[textpointer],20,"(%u')",tMplan_Summary.timeAtBottom);
                break;
            case 2: // first stop
                textpointer = snprintf(&text[textpointer],20,"(%u')",tMplan_Summary.timeToFirstStop);
                break;
            case 3: // surface
                textpointer = snprintf(&text[textpointer],20,"(%u')",tMplan_Summary.timeToSurface);
                break;
            default:
                break;
            }
            text[textpointer] = 0;
            write_label_var( 320, 500, y_line, &FontT42, text);

            // ascent or descent rate or ppO2@bottom
            textpointer = 0;
            *text = 0;
            switch(j)
            {
            case 0: // descent
                textpointer = snprintf(&text[textpointer],20,"-%u\016\016 m/min\017",tMplan_Summary.descentRateMeterPerMinute);
                break;
            case 1: // level
                textpointer = snprintf(&text[textpointer],20,"%1.2f\016\016 %c\017",tMplan_Summary.ppO2AtBottom, TXT_ppO2);
                break;
            case 2: // first stop
            case 3: // surface
                textpointer = snprintf(&text[textpointer],20,"%u\016\016 m/min\017",tMplan_Summary.ascentRateMeterPerMinute);
                break;
            default:
                break;
            }
            text[textpointer] = 0;
            write_label_var( 500, 800, y_line, &FontT42, text);
        }
        break;
    case 0xFE: // gas consumption
        for(int j=1;j<6;j++)
        {
            y_line = ME_Y_LINE_BASE + ((j + 0) * ME_Y_LINE_STEP);

            textpointer = 0;
            *text = 0;
            text[textpointer] = 0;

            if(tMplan_pGasConsumption[j] == 0)
                text[textpointer++] = '\031';

            textpointer += write_gas(&text[textpointer], settingsGetPointer()->gas[j].oxygen_percentage,   settingsGetPointer()->gas[j].helium_percentage );
            text[textpointer] = 0;
            write_label_var(  10, 390, y_line, &FontT42, text);

            textpointer = 0;
            *text = 0;
            text[textpointer] = 0;

            if(tMplan_pGasConsumption[j] == 0)
                text[textpointer++] = '\031';

            textpointer += snprintf(&text[textpointer],20,"\002%u\016\016 ltr\017",tMplan_pGasConsumption[j]);
            text[textpointer] = 0;
            write_label_var( 350, 560, y_line, &FontT42, text);
        }
        break;

    default:
        now = first -(5*(resultPage-1));
        for(int j=0;j<5;j++)
        {
            /* deco list */
            refresh_PlanResult_helper(text, now-j);
            y_line = ME_Y_LINE_BASE + ((j + 1) * ME_Y_LINE_STEP);
            if(*text != 0)
                write_label_var( 300, 790, y_line, &FontT42, text);

            /* common infos */
            textpointer = 0;
            *text = 0;
            switch (j)
            {
            case 0:
                if(stateUsed->diveSettings.deco_type.ub.standard == VPM_MODE)
                    textpointer += snprintf(&text[textpointer],20,"VPM +%u",stateUsed->diveSettings.vpm_conservatism);
                else
                    textpointer += snprintf(&text[textpointer],20,"GF %u/%u", stateUsed->diveSettings.gf_low, stateUsed->diveSettings.gf_high);
                break;
            case 1:
                if(isLoopMode(settingsGetPointer()->dive_mode))
                    text[textpointer++] = 'C';
                else
                    text[textpointer++] = 'O';
                text[textpointer++] = 'C';
                text[textpointer++] = ',';
                text[textpointer++] = ' ';
                oxygen_percentage = 100;
                oxygen_percentage -= stateSimGetPointer()->lifeData.actualGas.nitrogen_percentage;
                oxygen_percentage -= stateSimGetPointer()->lifeData.actualGas.helium_percentage;
                textpointer += tHome_gas_writer(oxygen_percentage, stateSimGetPointer()->lifeData.actualGas.helium_percentage, &text[textpointer]);
                break;
            case 2:
                textpointer += snprintf(&text[textpointer],20,"TTS: %u'", ((pDecoinfo->output_time_to_surface_seconds+59)/60));
            // alt: textpointer += snprintf(&text[textpointer],20,"TTS: %u'",tMplan_dive_time_minutes + ((pDecoinfo->output_time_to_surface_seconds+59)/60));
                break;
            case 3:
                textpointer += snprintf(&text[textpointer],20,"CNS:");
                break;
            case 4:
                textpointer += snprintf(&text[textpointer],20,"%.0f%%->%.0f%%",stateRealGetPointer()->lifeData.cns,stateSimGetPointer()->lifeData.cns);
                break;
            }
            text[textpointer] = 0;
            if(*text != 0)
                write_label_var(  10, 790, y_line, &FontT42, text);
        }
        break;
    };

    text[0] = TXT_2BYTE;
//	if(first < (resultPage * 5))
    if(resultPage == 0xFF)
        text[1] = TXT2BYTE_Exit;
    else
        text[1] = TXT2BYTE_ButtonNext;
    text[2] = 0;
    tMenuEdit_newButtonText(StMPLAN5_ExitResult, text);
    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonNext,0);
}

uint8_t OnAction_PlanResultExit	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    resetEnterPressedToStateBeforeButtonAction();
    if(resultPage == 0xFF) // last extra page
    {
        resultPage = 0;
        return EXIT_TO_MENU;
    }
    if(resultPage >= 0xF0)
    {
        resultPage++;
        return UNSPECIFIC_RETURN;
    }
    else if(first < (resultPage * 5))
    {
        resultPage = 0xFE;
        return UNSPECIFIC_RETURN;
    }
    else
    {
        resultPage++;
        return UNSPECIFIC_RETURN;
    }
}
