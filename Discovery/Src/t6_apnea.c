///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/t6_apnea.c
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
#include "t6_apnea.h"

#include "data_exchange_main.h"
#include "decom.h"
#include "gfx_fonts.h"
#include "math.h"
#include "tHome.h"
#include "simulation.h"
#include "timer.h"
#include "unit.h"

/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	t6screen;
GFX_DrawCfgWindow	t6l1;
GFX_DrawCfgWindow	t6r1;
GFX_DrawCfgWindow	t6c1;
GFX_DrawCfgWindow	t6c2;
GFX_DrawCfgWindow	t6c3; // for menu text

uint8_t t6_selection_customview = CVIEW_T3_noneOrDebug;

/* Imported function prototypes ---------------------------------------------*/

/* Private types -------------------------------------------------------------*/

#define CUSTOMBOX_LINE_LEFT (250)
#define CUSTOMBOX_LINE_RIGHT (549)
#define CUSTOMBOX_INSIDE_OFFSET (2)
#define CUSTOMBOX_OUTSIDE_OFFSET (2)

#define TEXTSIZE 16

const uint8_t t6_customviewsStandard[] =
{
	CVIEW_T3_noneOrDebug,
    CVIEW_T3_Temperature,
    CVIEW_T3_END
};

const uint8_t *t6_customviews = t6_customviewsStandard;
const uint8_t t6_customviewSurfaceMode = CVIEW_T3_ApnoeSurfaceInfo;

/* Private function prototypes -----------------------------------------------*/
void t6_refresh_divemode(void);
void t6_refresh_customview(float depth);

uint8_t t6_test_customview_warnings(void);

/* Exported functions --------------------------------------------------------*/

// for tHomeDiveMenuControl() in tHome.c and t6_refresh_customview
uint8_t t6_getCustomView(void)
{
    if(stateUsed->lifeData.counterSecondsShallowDepth)
        return t6_customviewSurfaceMode;
    else
        return t6_selection_customview;
}

void t6_init(void)
{
	SSettings* pSettings;
	pSettings = settingsGetPointer();
    t6_selection_customview = t6_customviewsStandard[0];

    t6screen.FBStartAdress = 0;
    t6screen.ImageHeight = 480;
    t6screen.ImageWidth = 800;
    t6screen.LayerIndex = 1;

    t6l1.Image = &t6screen;
    t6l1.WindowNumberOfTextLines = 2;
    t6l1.WindowLineSpacing = 19; // Abstand von Y0
    t6l1.WindowTab = 100;
    if(!pSettings->FlipDisplay)
    {
    	t6l1.WindowX0 = 0;
    	t6l1.WindowX1 = BigFontSeperationLeftRight - 5;
    	t6l1.WindowY0 = BigFontSeperationTopBottom + 5;
    	t6l1.WindowY1 = 479;
    }
    else
    {
    	t6l1.WindowX0 = 800 - BigFontSeperationLeftRight + 5;
    	t6l1.WindowX1 = 799;
    	t6l1.WindowY0 = 0;
    	t6l1.WindowY1 = 479 - BigFontSeperationTopBottom + 5 ;
    }

    t6r1.Image = &t6screen;
    t6r1.WindowNumberOfTextLines = t6l1.WindowNumberOfTextLines;
    t6r1.WindowLineSpacing = t6l1.WindowLineSpacing;
    t6r1.WindowTab = t6l1.WindowTab;
    if(!pSettings->FlipDisplay)
    {
    	t6r1.WindowX0 = BigFontSeperationLeftRight + 5;
    	t6r1.WindowX1 = 799;
    }
    else
    {
    	t6r1.WindowX0 = 0;
    	t6r1.WindowX1 = BigFontSeperationLeftRight - 5;
    }
    t6r1.WindowY0 = t6l1.WindowY0;
    t6r1.WindowY1 = t6l1.WindowY1;

    t6c1.Image = &t6screen;
    t6c1.WindowNumberOfTextLines = 2;
    t6c1.WindowLineSpacing = t6l1.WindowLineSpacing;
    t6c1.WindowX0 = 0;
    t6c1.WindowX1 = 799;
    if(!pSettings->FlipDisplay)
    {
    	t6c1.WindowY0 = 0;
    	t6c1.WindowY1 = BigFontSeperationTopBottom - 5;
    }
	else
	{
		t6c1.WindowY0 = 480 - BigFontSeperationTopBottom + 5;
		t6c1.WindowY1 = 479;
	   	t6c1.WindowX0 = 0;
	    t6c1.WindowX1 = BigFontSeperationLeftRight - 5;
	}

    t6c2.Image = &t6screen;
    t6c2.WindowNumberOfTextLines = 3;
    t6c2.WindowLineSpacing = 58;
    t6c2.WindowX0 = 370;
    t6c2.WindowX1 = 799;

    if(!pSettings->FlipDisplay)
    {
    	t6c2.WindowY0 = 0;
    	t6c2.WindowY1 = BigFontSeperationTopBottom - 5;
    }
	else
	{
		t6c2.WindowY0 = 480 - BigFontSeperationTopBottom + 5;
		t6c2.WindowY1 = 479;
	}

    t6c2.WindowTab = 600;

    t6c3.Image = &t6screen;
    t6c3.WindowNumberOfTextLines = 1;
    t6c3.WindowLineSpacing = 0; // Abstand von Y0
    t6c3.WindowTab = 100;
    t6c3.WindowX0 = CUSTOMBOX_LINE_LEFT + CUSTOMBOX_INSIDE_OFFSET;
    t6c3.WindowX1 = CUSTOMBOX_LINE_RIGHT - CUSTOMBOX_INSIDE_OFFSET;
    if(!pSettings->FlipDisplay)
    {
    	t6c3.WindowY0 = 0;
    	t6c3.WindowY1 = 69;
    }
    else
    {
    	t6c3.WindowY0 = 480 - 69;
    	t6c3.WindowY1 = 479;
    }
}


void t6_refresh(void)
{
    static uint8_t last_mode = MODE_SURFACE;

    SStateList status;
    get_globalStateList(&status);

    if(stateUsed->mode != MODE_DIVE)
    {
        last_mode = MODE_SURFACE;
        settingsGetPointer()->design = 7;
        if(t6screen.FBStartAdress)
        {
            releaseFrame(24,t6screen.FBStartAdress);
            t6screen.FBStartAdress = 0;
        }
        return;
    }

    if(status.base != BaseHome)
        return;

    t6screen.FBStartAdress = getFrame(24);

    if(last_mode != MODE_DIVE)
    {
        last_mode = MODE_DIVE;
        t6_selection_customview = *t6_customviews;
    }

    if(status.page == PageSurface)
        set_globalState(StD);

    t6_refresh_divemode();
    GFX_SetFramesTopBottom(t6screen.FBStartAdress, 0,480);
    releaseAllFramesExcept(24,t6screen.FBStartAdress);
}

/* Private functions ---------------------------------------------------------*/

void t6_refresh_divemode(void)
{
    char text[512];
    uint8_t  customview_warnings = 0;
    float depth_meter = 0.0;

    // everything like lines, depth, ascent graph and divetime or counterSecondsShallowDepth
    depth_meter = t3_basics_lines_depth_and_divetime(&t6screen, &t6l1, &t6r1, DIVEMODE_Apnea);


    // customview
    if(stateUsed->warnings.numWarnings)
        customview_warnings = t6_test_customview_warnings();

    if(customview_warnings && warning_count_high_time)
        t3_basics_show_customview_warnings(&t6c1);
    else
        t6_refresh_customview(depth_meter);

    if(stateUsed->warnings.lowBattery)
        t3_basics_battery_low_customview_extra(&t6c1);


    /* Menu Selection (and gas mix) */
    if(get_globalState() == StDBEAR)
    {
        snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveBearingQ);
        GFX_write_string_color(&FontT48,&t6c3,text,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDRAVG)
    {
        snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveResetAvgQ);
        GFX_write_string_color(&FontT48,&t6c3,text,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDQUIT)
    {
        snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveQuitQ);
        GFX_write_string_color(&FontT48,&t6c3,text,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDSIM1)
    {
        snprintf(text,TEXTSIZE,"\a\001%c%c", TXT_2BYTE, TXT2BYTE_DiveQuitQ);
        GFX_write_string_color(&FontT48,&t6c3,text,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDSIM2)
    {
        if(settingsGetPointer()->nonMetricalSystem)
            snprintf(text,TEXTSIZE,"\a\001" " Sim:-3.33ft ");
        else
            snprintf(text,TEXTSIZE,"\a\001" " Sim:-1m ");
        GFX_write_string_color(&FontT48,&t6c3,text,0,CLUT_WarningYellow);
        snprintf(text,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t6l1,text,0,CLUT_WarningYellow);

    }
    else if(get_globalState() == StDSIM3)
    {
        if(settingsGetPointer()->nonMetricalSystem)
            snprintf(text,TEXTSIZE,"\a\001" " Sim:+3.33ft ");
        else
            snprintf(text,TEXTSIZE,"\a\001" " Sim:+1m ");
        GFX_write_string_color(&FontT48,&t6c3,text,0,CLUT_WarningYellow);
        snprintf(text,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t6l1,text,0,CLUT_WarningYellow);
    }
    else if(get_globalState() == StDSIM4)
    {
        snprintf(text,TEXTSIZE,"\a\001" " Sim:+5' ");
        GFX_write_string_color(&FontT48,&t6c3,text,0,CLUT_WarningYellow);
        snprintf(text,TEXTSIZE,"\a\f %u %c%c"
            , unit_depth_integer(simulation_get_aim_depth())
            , unit_depth_char1()
            , unit_depth_char2()
            );
        GFX_write_string_color(&FontT42,&t6l1,text,0,CLUT_WarningYellow);
    }
    else
    {
        // keep empty
    }
}


void t6_battery_low_customview_extra(void)
{
    char TextC1[256];

    TextC1[0] = '\002';
    TextC1[1] = '\f';
    TextC1[2] = '\025';
    TextC1[3] = '3';
    TextC1[4] = '1';
    TextC1[5] = '1';
    TextC1[6] = '1';
    TextC1[7] = '1';
    TextC1[8] = '1';
    TextC1[9] = '1';
    TextC1[10] = '1';
    TextC1[11] = '1';
    TextC1[12] = '1';
    TextC1[13] = '1';
    TextC1[14] = '0';
    TextC1[15] = 0;

    if(!warning_count_high_time)
        TextC1[4] = '2';

    GFX_write_string(&Batt24,&t6c1,TextC1,0);
}



void t6_change_customview(uint8_t action)
{
    t3_basics_change_customview(&t6_selection_customview, t6_customviews, action);
}


void t6_refresh_customview(float depth)
{
    uint8_t customViewLeftSide = CVIEW_T3_MaxDepth;

    if(t6_getCustomView() == CVIEW_T3_ApnoeSurfaceInfo)
        customViewLeftSide = CVIEW_T3_ApnoeSurfaceInfo;

    t3_basics_refresh_customview(depth, customViewLeftSide,  &t6screen, &t6c1, &t6c2, DIVEMODE_Apnea);
    t3_basics_refresh_apnoeRight(depth, t6_getCustomView(), &t6screen, &t6c1, &t6c2, DIVEMODE_Apnea);
}


uint8_t t6_test_customview_warnings(void)
{
    uint8_t count = 0;

    count = 0;
    return count;
}
