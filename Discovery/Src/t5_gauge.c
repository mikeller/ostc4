///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/t5_gauge.c
/// \brief  dive screen for Gauge mode
/// \author Heinrichs Weikamp gmbh
/// \date   1-Feb-2017
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
#include "t5_gauge.h"

#include "data_exchange_main.h"
#include "decom.h"
#include "gfx_fonts.h"
#include "math.h"
#include "tHome.h"
#include "simulation.h"
#include "timer.h"
#include "unit.h"


/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	t5screen;
GFX_DrawCfgWindow	t5l1;
GFX_DrawCfgWindow	t5r1;
GFX_DrawCfgWindow	t5c1;
GFX_DrawCfgWindow	t5c2;
GFX_DrawCfgWindow	t5c3; // for menu text

uint8_t t5_selection_customview = 0;

/* Importend function prototypes ---------------------------------------------*/
//extern uint8_t write_gas(char *text, uint8_t oxygen, uint8_t helium);

/* Private types -------------------------------------------------------------*/

#define CUSTOMBOX_LINE_LEFT (250)
#define CUSTOMBOX_LINE_RIGHT (549)
#define CUSTOMBOX_INSIDE_OFFSET (2)
#define CUSTOMBOX_OUTSIDE_OFFSET (2)

#define TEXTSIZE 16

const uint8_t t5_customviewsStandard[] =
{
	CVIEW_T3_sensors,
    CVIEW_T3_Compass,
    CVIEW_T3_MaxDepth,
    CVIEW_T3_StopWatch,
    CVIEW_T3_Temperature,
    CVIEW_T3_GasList,
    CVIEW_T3_Decostop,
    CVIEW_T3_END
};


const uint8_t *t5_customviews = t5_customviewsStandard;

/* Private function prototypes -----------------------------------------------*/
void t5_refresh_divemode(void);
void t5_refresh_customview(float depth);

uint8_t t5_test_customview_warnings(void);
//void t5_show_customview_warnings(void);
//void t5_compass(uint16_t ActualHeading, uint16_t UserSetHeading);

/* Exported functions --------------------------------------------------------*/

// for tHomeDiveMenuControl() in tHome.c
uint8_t t5_getCustomView(void)
{
    return t5_selection_customview;
}


void t5_init(void)
{
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    t5_selection_customview = t5_customviewsStandard[0];

    t5screen.FBStartAdress = 0;
    t5screen.ImageHeight = 480;
    t5screen.ImageWidth = 800;
    t5screen.LayerIndex = 1;

    t5l1.Image = &t5screen;
    t5l1.WindowNumberOfTextLines = 2;
    t5l1.WindowLineSpacing = 19; // Abstand von Y0
	t5l1.WindowTab = 100;
    if(!pSettings->FlipDisplay)
    {
		t5l1.WindowX0 = 0;
		t5l1.WindowX1 = BigFontSeperationLeftRight - 5;
		t5l1.WindowY0 = BigFontSeperationTopBottom + 5;
		t5l1.WindowY1 = 479;
    }
    else
    {
		t5l1.WindowX0 = 800 - BigFontSeperationLeftRight + 5;
		t5l1.WindowX1 = 799;
		t5l1.WindowY0 = 0;
		t5l1.WindowY1 = 479 - BigFontSeperationTopBottom + 5 ;
    }

    t5r1.Image = &t5screen;
    t5r1.WindowNumberOfTextLines = t5l1.WindowNumberOfTextLines;
    t5r1.WindowLineSpacing = t5l1.WindowLineSpacing;
    t5r1.WindowTab = t5l1.WindowTab;
    if(!pSettings->FlipDisplay)
    {
		t5r1.WindowX0 = BigFontSeperationLeftRight + 5;
		t5r1.WindowX1 = 799;
    }
    else
    {
		t5r1.WindowX0 = 0;
		t5r1.WindowX1 = BigFontSeperationLeftRight - 5;
    }
    t5r1.WindowY0 = t5l1.WindowY0;
    t5r1.WindowY1 = t5l1.WindowY1;

    t5c1.Image = &t5screen;
    t5c1.WindowNumberOfTextLines = 2;
    t5c1.WindowLineSpacing = 84 + 5;
    t5c1.WindowX0 = 0;
    t5c1.WindowX1 = 799;
    if(!pSettings->FlipDisplay)
    {
    	t5c1.WindowY0 = 0;
    	t5c1.WindowY1 = BigFontSeperationTopBottom - 5;
    }
	else
	{
		t5c1.WindowY0 = 480 - BigFontSeperationTopBottom + 5;
		t5c1.WindowY1 = 479;
	}

    t5c2.Image = &t5screen;
    t5c2.WindowNumberOfTextLines = 3;
    t5c2.WindowLineSpacing = 58;
    t5c2.WindowX0 = 370;
    t5c2.WindowX1 = 799;
    t5c2.WindowY0 = 0;
    t5c2.WindowY1 = BigFontSeperationTopBottom - 5;
    t5c2.WindowTab = 600;

    t5c3.Image = &t5screen;
    t5c3.WindowNumberOfTextLines = 1;
    t5c3.WindowLineSpacing = 0; // Abstand von Y0
    t5c3.WindowTab = 100;
    t5c3.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t5c3.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
    if(!pSettings->FlipDisplay)
    {
    	t5c3.WindowY0 = 0;
    	t5c3.WindowY1 = 69;
    }
    else
    {
    	t5c3.WindowY0 = 480 - 69;
    	t5c3.WindowY1 = 479;
    }
}


void t5_refresh(void)
{
    static uint8_t last_mode = MODE_SURFACE;

    SStateList status;
    get_globalStateList(&status);

    if(stateUsed->mode != MODE_DIVE)
    {
        last_mode = MODE_SURFACE;
        settingsGetPointer()->design = 7;
        if(t5screen.FBStartAdress)
        {
            releaseFrame(24,t5screen.FBStartAdress);
            t5screen.FBStartAdress = 0;
        }
        return;
    }

    if(status.base != BaseHome)
        return;

    t5screen.FBStartAdress = getFrame(24);

    if(last_mode != MODE_DIVE)
    {
        last_mode = MODE_DIVE;
        t5_selection_customview = *t5_customviews;
    }

    if(status.page == PageSurface)
        set_globalState(StD);

    t5_refresh_divemode();
    GFX_SetFramesTopBottom(t5screen.FBStartAdress, 0,480);
    releaseAllFramesExcept(24,t5screen.FBStartAdress);
}


/* Private functions ---------------------------------------------------------*/

void t5_refresh_divemode(void)
{
    char text[512];
    uint8_t  customview_warnings = 0;
    float depth_meter = 0.0;

    // everything like lines, depth, ascent graph and divetime
    depth_meter = t3_basics_lines_depth_and_divetime(&t5screen, &t5l1, &t5r1, DIVEMODE_Gauge);

    // customview
    if(stateUsed->warnings.numWarnings)
        customview_warnings = t5_test_customview_warnings();

    if(customview_warnings && warning_count_high_time)
        t3_basics_show_customview_warnings(&t5c1);
    else
        t5_refresh_customview(depth_meter);

    if(stateUsed->warnings.lowBattery)
        t3_basics_battery_low_customview_extra(&t5c1);


    /* Menu Selection (and gas mix) */
    if(get_globalState() == StDBEAR)
    {
        snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveBearingQ);
        GFX_write_string_color(&FontT48,&t5c3,text,0,CLUT_WarningYellow);
    }
    else
    if(get_globalState() == StDRAVG)
    {
        snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveResetAvgQ);
        GFX_write_string_color(&FontT48,&t5c3,text,0,CLUT_WarningYellow);
    }
//	else
//	if(get_globalState() == StDMENU)
//	{
//		snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveMenuQ);
//		GFX_write_string_color(&FontT48,&t5c3,text,0,CLUT_WarningYellow);
//	}
    else
    if(get_globalState() == StDSIM1)
    {
        snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveQuitQ);
        GFX_write_string_color(&FontT48,&t5c3,text,0,CLUT_WarningYellow);
    }
    else
    if(get_globalState() == StDSIM2)
    {
        if(settingsGetPointer()->nonMetricalSystem)
            snprintf(text,TEXTSIZE,"\a\001" " Sim:-3.33ft ");
        else
            snprintf(text,TEXTSIZE,"\a\001" " Sim:-1m ");
        GFX_write_string_color(&FontT48,&t5c3,text,0,CLUT_WarningYellow);
        snprintf(text,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t5l1,text,0,CLUT_WarningYellow);

    }
    else
    if(get_globalState() == StDSIM3)
    {
        if(settingsGetPointer()->nonMetricalSystem)
            snprintf(text,TEXTSIZE,"\a\001" " Sim:+3.33ft ");
        else
            snprintf(text,TEXTSIZE,"\a\001" " Sim:+1m ");
        GFX_write_string_color(&FontT48,&t5c3,text,0,CLUT_WarningYellow);
        snprintf(text,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t5l1,text,0,CLUT_WarningYellow);
    }
    else
    if(get_globalState() == StDSIM4)
    {
        snprintf(text,TEXTSIZE,"\a\001" " Sim:+5' ");
        GFX_write_string_color(&FontT48,&t5c3,text,0,CLUT_WarningYellow);
        snprintf(text,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t5l1,text,0,CLUT_WarningYellow);
    }
    else
    {
        // keep empty
    }
}


void t5_change_customview(uint8_t action)
{
    t3_basics_change_customview(&t5_selection_customview, t5_customviews, action);
}


void t5_refresh_customview(float depth)
{
 //   if((t5_selection_customview == CVIEW_sensors) &&(stateUsed->diveSettings.ccrOption == 0))
 //       t5_change_customview();

    t3_basics_refresh_customview(depth, t5_selection_customview, &t5screen, &t5c1, &t5c2, DIVEMODE_Gauge);
}


uint8_t t5_test_customview_warnings(void)
{
    uint8_t count = 0;

    count = 0;
    return count;
}


