///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditXtra.c
/// \brief  Menu Edit Xtra - Specials in Divemode like Reset Stopwatch
/// \author heinrichs weikamp gmbh
/// \date   02-Mar-2015
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
#include "tMenuEditCustom.h"

#include "gfx_fonts.h"
#include "simulation.h"
#include "timer.h"
#include "tMenuEdit.h"
#include "tHome.h"  // for enum CUSTOMVIEWS and init_t7_compass()
#include "t3.h"
#include "t7.h"
#include "data_exchange_main.h"
#include "motion.h"
#include "tMenu.h"
#include "tMenuSystem.h"
#include <math.h>


#define CV_PER_PAGE  		(5u)	/* number of cv selections shown at one page */
#define MAX_BACKLIGHT_BOOST (2u)	/* max number of backlight levels which may be increased during focus state */

#define MAX_FOCUS_LIMITER	(2u)	/* max number for reducing the spot used for focus detection */

static uint8_t customviewsSubpage = 0;
static uint8_t customviewsSubpageMax = 0;	/* number of pages needed to display all selectable views */
static const uint8_t*	pcv_curchangelist;
/* Private function prototypes -----------------------------------------------*/
void openEdit_Customview(void);
void openEdit_BigScreen(void);
void openEdit_MotionCtrl(void);
void openEdit_ViewPort(void);
void refresh_Customviews(void);
void setMenuContentStructure();
char customview_TXT2BYTE_helper(uint8_t customViewId);
char customviewBF_TXT2BYTE_helper(uint8_t customViewId);
/* Announced function prototypes -----------------------------------------------*/
uint8_t OnAction_CViewTimeout  (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewStandard (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewStandardBF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewAutofocusBF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CornerTimeout (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CornerStandard(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewPortCalib(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewPortLayout(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewPortAmbient(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewPortControl(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_CViewPortSpot(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
/* Exported functions --------------------------------------------------------*/




void refresh_Customviews(void)
{
    char text[32];
    uint8_t textpointer = 0;

    // header
    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Customviews;
    text[3] = 0;
    write_topline(text);

    // custom view center  return
    textpointer = 0;
    text[textpointer++] = TXT_2BYTE;
    text[textpointer++] = TXT2BYTE_CViewTimeout;
    textpointer += snprintf(&text[textpointer],11,"  %02u\016\016 %c\017",settingsGetPointer()->tX_customViewTimeout,TXT_Seconds);
    write_label_var(  30, 700, ME_Y_LINE1, &FontT48, text);

    // custom view center primary
    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_CViewStandard;
    text[2] = ' ';
    text[3] = ' ';

    text[4] = TXT_2BYTE;
    text[5] = customview_TXT2BYTE_helper(settingsGetPointer()->tX_customViewPrimary);
    text[6] = 0;
    write_label_var(  30, 700, ME_Y_LINE2, &FontT48, text);

    // custom view big font
    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_ExtraDisplay;
    text[2] = ' ';
    text[3] = ' ';
    text[4] = TXT_2BYTE;
    text[5] = customviewBF_TXT2BYTE_helper(settingsGetPointer()->tX_customViewPrimaryBF);
    text[6] = 0;
    write_label_var(  30, 700, ME_Y_LINE3, &FontT48, text);

    /* Bigfont autofocus selector */
    textpointer = 0;
    text[textpointer++] = TXT_2BYTE;
    text[textpointer++] = TXT2BYTE_ExtraDisplay;
    textpointer += snprintf(&text[textpointer],20,"  %c%c",TXT_2BYTE, TXT2BYTE_Autofocus);
    write_label_var(  30, 700, ME_Y_LINE4, &FontT48, text);

    tMenuEdit_refresh_field(StMCustom1_CViewAutoFocusBF);


    // field corner  return
    textpointer = 0;
    text[textpointer++] = TXT_2BYTE;
    text[textpointer++] = TXT2BYTE_CornerTimeout;
    textpointer += snprintf(&text[textpointer],11,"  %02u\016\016 %c\017",settingsGetPointer()->tX_userselectedLeftLowerCornerTimeout,TXT_Seconds);
    write_label_var(  30, 700, ME_Y_LINE5, &FontT48, text);

    // field corner  primary
    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_CornerStandard;
    text[2] = ' ';
    text[3] = ' ';
    switch(settingsGetPointer()->tX_userselectedLeftLowerCornerPrimary)
    {
    /* Temperature */
    case LLC_Temperature:
        text[4] = TXT_Temperature;
        break;
    /* Average Depth */
    case LLC_AverageDepth:
        text[4] = TXT_AvgDepth;
        break;
    /* ppO2 */
    case LLC_ppO2:
        text[4] = TXT_ppO2;
        break;
    /* Stop Uhr */
    case LLC_Stopwatch:
        text[4] = TXT_Stopwatch;
        break;
    /* Ceiling */
    case LLC_Ceiling:
        text[4] = TXT_Ceiling;
        break;
    /* Future TTS */
    case LLC_FutureTTS:
        text[4] = TXT_FutureTTS;
        break;
    /* CNS */
    case LLC_CNS:
        text[4] = TXT_CNS;
        break;
    case LLC_GF:
    	text[4] = TXT_ActualGradient;
    	break;
    case LLC_ScrubberTime:
    	text[4] = TXT_ScrubTime;
    	break;

#ifdef ENABLE_BOTTLE_SENSOR
    case LCC_BottleBar:
    	text[4] = TXT_AtemGasVorrat;
    	    	break;
#endif

#ifdef ENABLE_PSCR_MODE
    case LCC_SimPpo2:
    	text[4] = TXT_SimPpo2;
    	    	break;
#endif
#ifdef ENABLE_CO2_SUPPORT
    case LCC_CO2:
        text[4] = TXT_CO2Sensor;
       	    	break;
#endif
    case LLC_Compass:
        text[4] = TXT_2BYTE;
        text[5] = TXT2BYTE_Compass;

        break;
    /* none */
    case LLC_Empty:
        text[4] = '-';
        break;
    default:
    	 text[4] = 'X';
    break;
    }
    if (text[4] != TXT_2BYTE) {
        text[5] = 0;
    } else {
        text[6] = 0;
    }

    write_label_var(  30, 700, ME_Y_LINE6, &FontT48, text);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}

void refresh_ViewPort(void)
{
    char text[32];
    uint8_t textIndex = 0;
    float distance = 0.0;
    SSettings* pSettings = settingsGetPointer();
    GFX_DrawCfgScreen* pdrawScreen;
    point_t lowerleft = {0,0};
    point_t upperright = {799,479};
    float localLimit = 0.1;
    uint8_t	spotSize = (pSettings->viewPortMode >> 5) & 0x03;

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_MotionCtrl;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_CalibView;
    text[2] = 0;
    write_label_var(   30, 700, ME_Y_LINE2, &FontT48, text);


    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_FocusSpotSize;
    text[textIndex++] = 0;
	write_label_var(  30, 340, ME_Y_LINE3, &FontT48, text);

    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    switch(spotSize)
    {
    	case 2:			text[textIndex++] = TXT2BYTE_Minimum;		break;
    	case 1:			text[textIndex++] = TXT2BYTE_Normal;		break;
    	default:
    	case 0:			text[textIndex++] = TXT2BYTE_Maximum;		break;
    }
    text[textIndex++] = 0;
    write_label_var(   500, 800, ME_Y_LINE3, &FontT48, text);

    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_IndicateFrame;
    text[textIndex++] = 0;
    write_label_var(  30, 340, ME_Y_LINE4, &FontT48, text);
    textIndex = 0;
    text[textIndex++] = '\006' - ((settingsGetPointer()->viewPortMode >> 4) & 1);
    text[textIndex++] = 0;
    write_label_var(   500, 800, ME_Y_LINE4, &FontT48, text);
    textIndex = 0;
    text[textIndex++] = TXT_2BYTE;
    text[textIndex++] = TXT2BYTE_BoostBacklight;
    text[textIndex++] = 0;
    write_label_var(  30, 340, ME_Y_LINE5, &FontT48, text);
    textIndex = 0;
    snprintf(&text[textIndex],32,"%d",(settingsGetPointer()->viewPortMode & 0x3));
    write_label_var(   500, 800, ME_Y_LINE5, &FontT48, text);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);

    /* test if we are in focus */
    if((pSettings->viewPitch != 0.0) || (pSettings->viewRoll != 0.0) || (pSettings->viewYaw != 0.0))
    {
    	distance = checkViewport(stateUsed->lifeData.compass_roll, stateUsed->lifeData.compass_pitch, stateUsed->lifeData.compass_heading, MOTION_ENABLE_ALL);

    	/* show "bar graph" indicating the distance to the center point */
        textIndex = 0;
        text[textIndex++] = '\001';

    	if(fabs(distance) < localLimit)
    	{
    		text[textIndex++] = '+';
    	}
    	else
    	{
    	    while (localLimit < 0.6)
    	    {
    	    	localLimit += 0.1;
    	    	text[textIndex++] = '-';
    	    	text[textIndex++] = '-';
    	    	if(fabs(distance) < localLimit)
    	    	{
    	    		break;
    	    	}
    	    }
    	}
    	text[textIndex] = 0;
    	write_label_var(   30, 700, ME_Y_LINE1, &FontT48, text);

/* MotionCtrl */
        textIndex = 0;

    	text[textIndex++] = TXT_2BYTE;
   		text[textIndex++] = TXT2BYTE_MotionCtrl;
   		text[textIndex++] = 0;
   		write_label_var(  30, 340, ME_Y_LINE6, &FontT48, text);
        textIndex = 0;
   		text[textIndex++] = TXT_2BYTE;
   		switch(settingsGetPointer()->MotionDetection)
   		{
   			case MOTION_DETECT_OFF:
   				text[textIndex++] = TXT2BYTE_MoCtrlNone;
   				break;
   			case MOTION_DETECT_MOVE:
   				text[textIndex++] = TXT2BYTE_MoCtrlPitch;
   				break;
   			case MOTION_DETECT_SECTOR:
   				text[textIndex++] = TXT2BYTE_MoCtrlSector;
   				break;
   			case MOTION_DETECT_SCROLL:
   				text[textIndex++] = TXT2BYTE_MoCtrlScroll;
   						break;
   			default:
   					textIndex += snprintf(&text[4],4,"%u",settingsGetPointer()->MotionDetection);
    			break;
        }
    	text[textIndex] = 0;
    	write_label_var(500, 800, ME_Y_LINE6, &FontT48, text);


		if(viewInFocus())
		{
			set_Backlight_Boost(settingsGetPointer()->viewPortMode & 0x03);
		    if((pSettings->viewPortMode >> 4) & 0x01)
		    {
		    	pdrawScreen = getMenuEditScreen();
		    	GFX_draw_box(pdrawScreen,lowerleft,upperright,0,CLUT_NiceGreen);
		    }
		}
		else
		{
			set_Backlight_Boost(0);
		    if((pSettings->viewPortMode >> 4) & 0x01)
		    {
		    	pdrawScreen = getMenuEditScreen();
		    	GFX_draw_box(pdrawScreen,lowerleft,upperright,0,CLUT_MenuTopBackground);
		    }
		}
		resetFocusState();	/* no other instance shall be impacted by the local detection */
    }
}

void openEdit_Custom(uint8_t line)
{
    set_globalState_Menu_Line(line);

    switch(line)
    {
    	case 1:
    	default:    openEdit_Customview();
    		break;
    	case 2: 	openEdit_BigScreen();
    		break;
    	case 3:		openEdit_CustomviewDivemode(cv_changelist);
    		break;
    	case 4:		openEdit_CustomviewDivemode(cv_changelist_BS);
    		break;
    	case 5:		openEdit_ViewPort();
    		break;
    }
}

/* Private functions ---------------------------------------------------------*/
void openEdit_Customview(void)
{
	resetMenuEdit(CLUT_MenuPageCustomView);
    refresh_Customviews();

    write_field_button(StMCustom1_CViewTimeout,		400, 700, ME_Y_LINE1,  &FontT48, "");
    write_field_button(StMCustom1_CViewStandard,	400, 700, ME_Y_LINE2,  &FontT48, "");
    write_field_button(StMCustom1_CViewStandardBF,	400, 700, ME_Y_LINE3,  &FontT48, "");
    write_field_on_off(StMCustom1_CViewAutoFocusBF,	650, 700, ME_Y_LINE4,  &FontT48, "", settingsGetPointer()->cvAutofocus);
    write_field_button(StMCustom1_CornerTimeout,	400, 700, ME_Y_LINE5,  &FontT48, "");
    write_field_button(StMCustom1_CornerStandard,	400, 700, ME_Y_LINE6,  &FontT48, "");

    setEvent(StMCustom1_CViewTimeout,		(uint32_t)OnAction_CViewTimeout);
    setEvent(StMCustom1_CViewStandard,		(uint32_t)OnAction_CViewStandard);
    setEvent(StMCustom1_CViewStandardBF,	(uint32_t)OnAction_CViewStandardBF);
    setEvent(StMCustom1_CViewAutoFocusBF,	(uint32_t)OnAction_CViewAutofocusBF);
    setEvent(StMCustom1_CornerTimeout,		(uint32_t)OnAction_CornerTimeout);
    setEvent(StMCustom1_CornerStandard,		(uint32_t)OnAction_CornerStandard);
}

void openEdit_BigScreen(void)
{
	uint8_t newValue = 0;
    SSettings *pSettings = settingsGetPointer();

    newValue = pSettings->extraDisplay + 1;

    if(newValue >= EXTRADISPLAY_END)
    {
    	newValue = EXTRADISPLAY_none;
    }
    pSettings->extraDisplay = newValue;

    exitMenuEdit_to_Menu_with_Menu_Update_do_not_write_settings_for_this_only();
}

void openEdit_MotionCtrl(void)
{
	uint8_t newValue = 0;
    SSettings *pSettings = settingsGetPointer();

     switch(pSettings->MotionDetection)
     {
     case MOTION_DETECT_OFF:
         newValue = MOTION_DETECT_MOVE;
         break;
     case MOTION_DETECT_MOVE:
         newValue = MOTION_DETECT_SECTOR;
         break;
     case MOTION_DETECT_SECTOR:
         newValue = MOTION_DETECT_SCROLL;
         break;
     case MOTION_DETECT_SCROLL:
     	newValue = MOTION_DETECT_OFF;
     	break;
     default:
         newValue = MOTION_DETECT_OFF;
         break;
     }
     pSettings->MotionDetection = newValue;
     InitMotionDetection();

     exitMenuEdit_to_Menu_with_Menu_Update_do_not_write_settings_for_this_only();
}



void openEdit_ViewPort(void)
{
	resetMenuEdit(CLUT_MenuPageCustomView);
    refresh_ViewPort();

    write_field_button(StMCustom5_CViewPortCalib,	400, 700, ME_Y_LINE2,  &FontT48, "");
    write_field_button(StMCustom5_CViewPortSpotSize,400, 700, ME_Y_LINE3,  &FontT48, "");
    write_field_button(StMCustom5_CViewPortLayout,	400, 700, ME_Y_LINE4,  &FontT48, "");
    write_field_button(StMCustom5_CViewPortAmbient,	400, 700, ME_Y_LINE5,  &FontT48, "");
    write_field_button(StMCustom5_CViewPortControl,	400, 700, ME_Y_LINE6,  &FontT48, "");

    setEvent(StMCustom5_CViewPortCalib,		(uint32_t)OnAction_CViewPortCalib);
    setEvent(StMCustom5_CViewPortSpotSize,	(uint32_t)OnAction_CViewPortSpot);
    setEvent(StMCustom5_CViewPortLayout,	(uint32_t)OnAction_CViewPortLayout);
    setEvent(StMCustom5_CViewPortAmbient,	(uint32_t)OnAction_CViewPortAmbient);
    setEvent(StMCustom5_CViewPortControl,	(uint32_t)OnAction_CViewPortControl);
}


char customview_TXT2BYTE_helper(uint8_t customViewId)
{
    char text = 0;

    switch(customViewId)
    {
    case CVIEW_sensors:
        text = TXT2BYTE_O2monitor;
        break;
    case CVIEW_sensors_mV:
        text = TXT2BYTE_O2voltage;
        break;
    case CVIEW_Compass:
        text = TXT2BYTE_Compass;
        break;
    case CVIEW_Decolist:
        text = TXT2BYTE_Decolist;
        break;
    case CVIEW_Tissues:
        text = TXT2BYTE_Tissues;
        break;
    case CVIEW_Profile:
        text = TXT2BYTE_Profile;
        break;
    case CVIEW_Gaslist:
        text = TXT2BYTE_Gaslist;
        break;
    case CVIEW_EADTime:
        text = TXT2BYTE_Info;
        break;
    case CVIEW_SummaryOfLeftCorner:
        text = TXT2BYTE_Summary;
        break;
    case CVIEW_Timer:
        text = TXT2BYTE_Timer;
        break;
    case CVIEW_Position:
        text = TXT2BYTE_Position;
        break;
    case CVIEW_noneOrDebug:
    	text = TXT2BYTE_DispNoneDbg;
    	break;
    default:
        break;
    }
    return text;
}
char customviewBF_TXT2BYTE_helper(uint8_t customViewId)
{
    char text = 0;

    switch(customViewId)
    {
    case CVIEW_T3_noneOrDebug:
    	text = TXT2BYTE_DispNoneDbg;
        break;
    case CVIEW_T3_sensors:
        text = TXT2BYTE_O2monitor;
        break;
    case CVIEW_T3_Compass:
        text = TXT2BYTE_Compass;
        break;
    case CVIEW_T3_Decostop:
        text = TXT2BYTE_Decolist;
        break;
#ifdef ENABLE_T3_PROFILE_VIEW
    case CVIEW_T3_Profile:
        text = TXT2BYTE_Profile;
        break;
#endif
    case CVIEW_T3_Temperature:
    	text = TXT2BYTE_AmbientTemperature;
    	break;
    case CVIEW_T3_GasList:
        text = TXT2BYTE_Gaslist;
        break;
    case CVIEW_T3_MaxDepth:
    	text = TXT2BYTE_MaxDepth;
    	break;
    case CVIEW_T3_StopWatch:
    	text = TXT2BYTE_Stopwatch;
    	break;
    case CVIEW_T3_TTS:
    	text = TXT2BYTE_TTS;
    	break;
    case CVIEW_T3_ppO2andGas:
    	text = TXT2BYTE_ppoNair;
    	break;
    case CVIEW_T3_Navigation:
    	text = TXT2BYTE_Navigation;
    	break;
    case CVIEW_T3_DepthData:
    	text = TXT2BYTE_DepthData;
    	break;
    case CVIEW_T3_DecoTTS:
    	text = TXT2BYTE_DecoTTS;
    	break;
    default:
        break;
    }
    return text;
}

uint8_t OnAction_CViewTimeout(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t value;
    value = settingsGetPointer()->tX_customViewTimeout;

    if(value < 5)
        value = 5;
    else if(value < 10)
        value = 10;
    else if(value < 15)
        value = 15;
    else if(value < 20)
        value = 20;
    else if(value < 30)
        value = 30;
    else if(value < 45)
        value = 45;
    else if(value < 60)
        value = 60;
    else
        value = 0;

    settingsGetPointer()->tX_customViewTimeout = value;
    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_CViewStandard(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t newValue;
    switch(settingsGetPointer()->tX_customViewPrimary)
    {
    case CVIEW_sensors:
        newValue = CVIEW_sensors_mV;
        break;
    case CVIEW_sensors_mV:
        newValue = CVIEW_Compass;
        break;
    case CVIEW_Compass:
        newValue = CVIEW_Decolist;
        break;
    case CVIEW_Decolist:
        newValue = CVIEW_Tissues;
        break;
    case CVIEW_Tissues:
        newValue = CVIEW_Profile;
        break;
    case CVIEW_Profile:
        newValue = CVIEW_Gaslist;
        break;
    case CVIEW_Gaslist:
        newValue = CVIEW_EADTime;
        break;
    case CVIEW_EADTime:
        newValue = CVIEW_SummaryOfLeftCorner;
        break;
    case CVIEW_SummaryOfLeftCorner:
        newValue = CVIEW_Timer;
        break;
    case CVIEW_Timer:
        newValue = CVIEW_noneOrDebug;
        break;
    case CVIEW_noneOrDebug:
    default:
         newValue = CVIEW_sensors;
        break;
    }
    settingsGetPointer()->tX_customViewPrimary = newValue;
    return UPDATE_DIVESETTINGS;
}

uint8_t OnAction_CViewStandardBF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	uint8_t index = 0;
    uint8_t newValue;

	/* list contains all views which may be selected => get index of current setting */
	while((settingsGetPointer()->tX_customViewPrimaryBF != cv_changelist_BS[index])	 && (cv_changelist_BS[index] != CVIEW_T3_END))
	{
		index++;
	}
	if((cv_changelist_BS[index] == CVIEW_T3_END) || (cv_changelist_BS[index+1] == CVIEW_T3_END)) 	/* invalid or last setting */
	{
		newValue = cv_changelist_BS[0];
	}
	else
	{
		newValue = cv_changelist_BS[index + 1];
	}

    settingsGetPointer()->tX_customViewPrimaryBF = newValue;
    return UPDATE_DIVESETTINGS;
}

uint8_t OnAction_CViewAutofocusBF(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	SSettings *pSettings = settingsGetPointer();
	if(pSettings->cvAutofocus)
	{
		pSettings->cvAutofocus = 0;
	}
	else
	{
		pSettings->cvAutofocus = 1;
	}
	tMenuEdit_set_on_off(editId, pSettings->cvAutofocus);
	return UPDATE_DIVESETTINGS;
}

uint8_t OnAction_CornerTimeout(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t value;
    value = settingsGetPointer()->tX_userselectedLeftLowerCornerTimeout;

    if(value < 5)
        value = 5;
    else  if(value < 10)
        value = 10;
    else if(value < 15)
        value = 15;
    else if(value < 20)
        value = 20;
    else if(value < 30)
        value = 30;
    else if(value < 45)
        value = 45;
    else if(value < 60)
        value = 60;
    else
        value = 0;

    settingsGetPointer()->tX_userselectedLeftLowerCornerTimeout = value;
    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_CornerStandard(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t value;
    value = settingsGetPointer()->tX_userselectedLeftLowerCornerPrimary;

    value += 1;

    if(value >= LLC_END)
        value = 0;

    settingsGetPointer()->tX_userselectedLeftLowerCornerPrimary = value;
    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_Customview_Toggle(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{

	uint8_t line = 0;
	SSettings *pSettings = settingsGetPointer();

	switch(editId)
	{
		case StMCustom3_CViewSelection1:	line = 1;
			break;
		case StMCustom3_CViewSelection2:	line = 2;
			break;
		case StMCustom3_CViewSelection3:	line = 3;
			break;
		case StMCustom3_CViewSelection4:	line = 4;
			break;
		case StMCustom3_CViewSelection5:	line = 5;
			break;

		default:
			break;
	}
	if(pcv_curchangelist == cv_changelist)
	{
		pSettings->cv_configuration ^= 1 << (pcv_curchangelist[customviewsSubpage * 5 + line-1]);
		if(t7_GetEnabled_customviews() == 0)
		{
			pSettings->cv_configuration ^= (1 << CVIEW_noneOrDebug);
		}
	}
	else
	{
		pSettings->cv_config_BigScreen ^= 1 << (pcv_curchangelist[customviewsSubpage * 5 + line-1]);
		if(t3_GetEnabled_customviews() == 0)
		{
			pSettings->cv_config_BigScreen ^= (1 << CVIEW_noneOrDebug);
		}
	}
    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_Customview_NextPage(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	resetMenuContentStructure();
	customviewsSubpage++;

	if(customviewsSubpage == customviewsSubpageMax)
	{
		customviewsSubpage = 0;
		setMenuContentStructure();
		tMenuEdit_select(StMCustom3_CViewSelection6);
	}
	else
	{
		setMenuContentStructure();
	}

	CustomviewDivemode_refresh();
    return UPDATE_DIVESETTINGS;
}

uint8_t OnAction_CViewPortCalib(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	float heading;
	if(settingsGetPointer()->compassInertia)
	{
	  	heading = (uint16_t)compass_getCompensated();
	}
	else
	{
	  	heading = (uint16_t)stateUsed->lifeData.compass_heading;
	}
	calibrateViewport(stateUsed->lifeData.compass_roll,stateUsed->lifeData.compass_pitch,heading);

	return UPDATE_DIVESETTINGS;
}
uint8_t OnAction_CViewPortLayout(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	SSettings* pSettings = settingsGetPointer();
	if((pSettings->viewPortMode & 0x10) != 0)
	{
		pSettings->viewPortMode &= 0xEF;	/* 1110 1111 */
	}
	else
	{
		pSettings->viewPortMode |= 0x10;
	}

	return UPDATE_DIVESETTINGS;
}
uint8_t OnAction_CViewPortAmbient(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	SSettings* pSettings = settingsGetPointer();
	if(((pSettings->viewPortMode +1) & 0x03) <= MAX_BACKLIGHT_BOOST)
	{
		pSettings->viewPortMode++;
	}
	else
	{
		pSettings->viewPortMode &= 0xFC;
	}

	return UPDATE_DIVESETTINGS;
}

uint8_t OnAction_CViewPortControl(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	uint8_t newValue = 0;
    SSettings *pSettings = settingsGetPointer();

    switch(pSettings->MotionDetection)
    {
    	case MOTION_DETECT_OFF:
    			newValue = MOTION_DETECT_MOVE;
    		break;
    	case MOTION_DETECT_MOVE:
    			newValue = MOTION_DETECT_SECTOR;
    		break;
    	case MOTION_DETECT_SECTOR:
    			newValue = MOTION_DETECT_SCROLL;
    		break;
    	case MOTION_DETECT_SCROLL:
    			newValue = MOTION_DETECT_OFF;
    		break;
    	default:
    			newValue = MOTION_DETECT_OFF;
    		break;
     }
     pSettings->MotionDetection = newValue;
     return UPDATE_DIVESETTINGS;
}

uint8_t OnAction_CViewPortSpot(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	uint8_t newValue = 0;
    SSettings *pSettings = settingsGetPointer();

    newValue = ((pSettings->viewPortMode >> 5) + 1) & 0x03;
    if(newValue > MAX_FOCUS_LIMITER)
    {
    	newValue = 0;
    }
    pSettings->viewPortMode &= 0x9F;		/* 1001 1111 */
    pSettings->viewPortMode |= (newValue << 5);
    return UPDATE_DIVESETTINGS;
}

void setMenuContentStructure()
{
	char text[MAX_PAGE_TEXTSIZE];
	uint8_t textPointer = 0;
	uint32_t id;

    uint8_t i;
    uint8_t endID = CVIEW_END;
	SSettings *pSettings = settingsGetPointer();

    if(pcv_curchangelist == cv_changelist_BS)
    {
    	endID = CVIEW_T3_END;
    }

    for(i=0; i<5;i++)		/* fill maximum 5 items and leave last one for sub page selection */
    {
   	textPointer = 0;
    	id = pcv_curchangelist[customviewsSubpage * 5 + i];
    	if(id == endID) /* last list item? */
    	{
    		break;
    	}
    	else
    	{
    			if(pcv_curchangelist == cv_changelist)
    			{
    				text[textPointer++] = '\006' - CHECK_BIT_THOME(pSettings->cv_configuration,id);
    			}
    			else
    			{
    				text[textPointer++] = '\006' - CHECK_BIT_THOME(pSettings->cv_config_BigScreen,id);
    			}
				text[textPointer++] = ' ';
				textPointer += snprintf(&text[textPointer], 60,	"%c%c\n\r",	TXT_2BYTE, customview_TXT2BYTE_helper(id));

				switch(i)
				{
					case 0: 	write_field_button(StMCustom3_CViewSelection1,	30, 800, ME_Y_LINE1,  &FontT48, "");
						break;
					case 1: 	write_field_button(StMCustom3_CViewSelection2,	30, 800, ME_Y_LINE2,  &FontT48, "");
						break;
					case 2: 	write_field_button(StMCustom3_CViewSelection3,	30, 800, ME_Y_LINE3,  &FontT48, "");
						break;
					case 3: 	write_field_button(StMCustom3_CViewSelection4,	30, 800, ME_Y_LINE4,  &FontT48, "");
						break;
					case 4: 	write_field_button(StMCustom3_CViewSelection5,	30, 800, ME_Y_LINE5,  &FontT48, "");
						break;
					default:
						break;
				}
    	}
    }
    for(;i<5;i++)	/* clear empty lines in case menu shows less than 5 entries */
    {
		switch(i)
		{
			case 0: 	write_label_var( 30, 800, ME_Y_LINE1, &FontT48, "");
				break;
			case 1:		write_label_var( 30, 800, ME_Y_LINE2, &FontT48, "");
				break;
			case 2: 	write_label_var( 30, 800, ME_Y_LINE3, &FontT48, "");
				break;
			case 3: 	write_label_var( 30, 800, ME_Y_LINE4, &FontT48, "");
				break;
			case 4: 	write_label_var( 30, 800, ME_Y_LINE5, &FontT48, "");
				break;
			default:
				break;
		};
    }

    if(customviewsSubpageMax != 1)
    {
        textPointer = 0;
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_ButtonNext;
        text[textPointer] = 0;
   	 write_field_button(StMCustom3_CViewSelection6,	30, 800, ME_Y_LINE6,  &FontT48, text);
    }

    /* because of the ID handling inside of the functions, all buttons needs to be assigned before the events may be set => have the same loop twice */
    for(i=0; i<5;i++)		/* fill maximum 5 items and leave last one for sub page selection */
    {
    	id = pcv_curchangelist[customviewsSubpage * 5 + i];
    	if(id == endID)		/* last list item? */
    	{
    		break;
    	}
    	else
    	{
				switch(i)
				{
					case 0: 	setEvent(StMCustom3_CViewSelection1, 				(uint32_t)OnAction_Customview_Toggle);
						break;
					case 1: 	setEvent(StMCustom3_CViewSelection2, 				(uint32_t)OnAction_Customview_Toggle);
						break;
					case 2: 	setEvent(StMCustom3_CViewSelection3, 				(uint32_t)OnAction_Customview_Toggle);
						break;
					case 3: 	setEvent(StMCustom3_CViewSelection4, 				(uint32_t)OnAction_Customview_Toggle);
						break;
					case 4: 	setEvent(StMCustom3_CViewSelection5, 				(uint32_t)OnAction_Customview_Toggle);
						break;

					default:
						break;
				}

    	}
    }
    if(customviewsSubpageMax != 1)
    {
   	 setEvent(StMCustom3_CViewSelection6,(uint32_t)OnAction_Customview_NextPage);
    }
}
void openEdit_CustomviewDivemode(const uint8_t* pcv_changelist)
{
    resetMenuEdit(CLUT_MenuPageCustomView);
	customviewsSubpageMax = (tHome_getNumberOfAvailableCVs(pcv_changelist) / CV_PER_PAGE) + 1;

	if(pcv_curchangelist != pcv_changelist)		/* new selection base? => reset page index */
	{
		customviewsSubpage = 0;
	}
	pcv_curchangelist = pcv_changelist;

	setMenuContentStructure();

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}

void openEdit_CustomviewDivemodeMenu(uint8_t line)
{
	static uint8_t customviewsSubpage = 0;
	SSettings *pSettings = settingsGetPointer();

	customviewsSubpageMax = (tHome_getNumberOfAvailableCVs(cv_changelist) / CV_PER_PAGE) + 1;

	if((line == 6) || (cv_changelist[customviewsSubpage * 5 + line-1] == CVIEW_END))		/* select next set of views */
	{
		customviewsSubpage++;
		if(customviewsSubpage == customviewsSubpageMax)
		{
			customviewsSubpage = 0;
		}
		set_CustomsviewsSubpage(customviewsSubpage);
		/* rebuild the selection page with the next set of customer views */
		updateSpecificMenu(StMSYS);
		openMenu(0);
	}
	else
	{
		pSettings->cv_configuration ^= 1 << (cv_changelist[customviewsSubpage * 5 + line-1]);
		if(t7_GetEnabled_customviews() == 0)
		{
			pSettings->cv_configuration ^= (1 << CVIEW_noneOrDebug);
		}
		InitMotionDetection(); /* consider new view setup for view selection by motion */
		exitMenuEdit_to_Menu_with_Menu_Update();
	}
}

uint8_t OnAction_ExtraDisplay	 (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t newValue;

    newValue = settingsGetPointer()->extraDisplay + 1;

    if(newValue >= EXTRADISPLAY_END)
    {
    	newValue = EXTRADISPLAY_none;
    }
    settingsGetPointer()->extraDisplay = newValue;
    return UNSPECIFIC_RETURN;
}

void CustomviewDivemode_refresh()
{
	SSettings *pSettings = settingsGetPointer();
	char text[MAX_PAGE_TEXTSIZE];
	uint8_t textPointer = 0;
	uint32_t id;

    uint8_t i;
    uint8_t endID = CVIEW_END;

    if(pcv_curchangelist == cv_changelist_BS)
    {
    	endID = CVIEW_T3_END;
    }

    text[textPointer++] = '\001';
    text[textPointer++] = TXT_2BYTE;
    text[textPointer++] = TXT2BYTE_SelectCustomviews;
    text[textPointer++] = ' ';
    text[textPointer++] = '1' + customviewsSubpage;
    text[textPointer++] = 0;
    write_topline(text);


     for(i=0; i<5;i++)		/* fill maximum 5 items and leave last one for sub page selection */
     {
    	textPointer = 0;
     	id = pcv_curchangelist[customviewsSubpage * 5 + i];
     	if(id == endID)	/* last list item? */
     	{
     		break;
     	}
     	else
     	{
 			if(pcv_curchangelist == cv_changelist)
 			{
 				text[textPointer++] = '\006' - CHECK_BIT_THOME(pSettings->cv_configuration,id);
 				text[textPointer++] = ' ';
 				textPointer += snprintf(&text[textPointer], 60,	"%c%c\n\r",	TXT_2BYTE, customview_TXT2BYTE_helper(id));
 			}
 			else
 			{
 				text[textPointer++] = '\006' - CHECK_BIT_THOME(pSettings->cv_config_BigScreen,id);
 				text[textPointer++] = ' ';
 				textPointer += snprintf(&text[textPointer], 60,	"%c%c\n\r",	TXT_2BYTE, customviewBF_TXT2BYTE_helper(id));
 			}

			switch(i)
			{
				case 0: 	write_label_var( 30, 800, ME_Y_LINE1, &FontT48, text);
					break;
				case 1:		write_label_var( 30, 800, ME_Y_LINE2, &FontT48, text);
					break;
				case 2: 	write_label_var( 30, 800, ME_Y_LINE3, &FontT48, text);
					break;
				case 3: 	write_label_var( 30, 800, ME_Y_LINE4, &FontT48, text);
					break;
				case 4: 	write_label_var( 30, 800, ME_Y_LINE5, &FontT48, text);
					break;
				default:
					break;
			}
     	}
     }
     if(customviewsSubpageMax != 1)
     {
         textPointer = 0;
         text[textPointer++] = TXT_2BYTE;
         text[textPointer++] = TXT2BYTE_ButtonNext;
         text[textPointer] = 0;
         write_label_var( 30, 800, ME_Y_LINE6, &FontT48, text);
     }
     write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}
