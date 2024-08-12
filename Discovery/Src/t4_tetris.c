///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/t4_tetris.c
/// \brief  Main Template file for dive mode special screen t4_tetris
/// \author Heinrichs Weikamp gmbh
/// \date   17-Feb-2016
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
#include "t4_tetris.h"

#include "data_exchange_main.h"
#include "decom.h"
#include "gfx_fonts.h"
#include "math.h"
#include "tHome.h"
#include "timer.h"
#include "unit.h"
#include "gfx_engine.h"

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	t4screen;
GFX_DrawCfgWindow	t4l1;
GFX_DrawCfgWindow	t4l2;
GFX_DrawCfgWindow	t4l3;

/* Private types -------------------------------------------------------------*/
#define TEXTSIZE 16

const uint16_t t4SeperationLeftRight = 250;
const uint16_t t4SeperationTopMid = 315;
const uint16_t t4SeperationMidBottom = 139;

/* Private function prototypes -----------------------------------------------*/
void t4_refresh_divemode(void);

/* Exported functions --------------------------------------------------------*/

void t4_init(void)
{
    t4screen.FBStartAdress = 0;
    t4screen.ImageHeight = 480;
    t4screen.ImageWidth = 800;
    t4screen.LayerIndex = 1;

    t4l1.Image = &t4screen;
    t4l1.WindowNumberOfTextLines = 2;
    t4l1.WindowLineSpacing = 19; // Abstand von Y0
    t4l1.WindowTab = 100;
    t4l1.WindowX0 = 0;
    t4l1.WindowX1 = t4SeperationLeftRight - 2;
    t4l1.WindowY1 = 479;
    t4l1.WindowY0 = t4SeperationTopMid + 3;

    t4l2.Image = t4l1.Image;
    t4l2.WindowNumberOfTextLines = t4l1.WindowNumberOfTextLines;
    t4l2.WindowLineSpacing = t4l1.WindowLineSpacing;
    t4l2.WindowTab = t4l1.WindowTab;
    t4l2.WindowX0 = t4l1.WindowX0;
    t4l2.WindowX1 = t4l1.WindowX1;
    t4l2.WindowY1 = t4SeperationTopMid - 2;
    t4l2.WindowY0 = t4SeperationMidBottom + 3;

    t4l3.Image = t4l1.Image;
    t4l3.WindowNumberOfTextLines = t4l1.WindowNumberOfTextLines;
    t4l3.WindowLineSpacing = t4l1.WindowLineSpacing;
    t4l3.WindowTab = t4l1.WindowTab;
    t4l3.WindowX0 = t4l1.WindowX0;
    t4l3.WindowX1 = t4l1.WindowX1;
    t4l3.WindowY1 = t4SeperationMidBottom - 2;
    t4l3.WindowY0 = 0;
}


void t4_refresh(void)
{
    SStateList status;
    get_globalStateList(&status);

    if(stateUsed->mode != MODE_DIVE)
    {
        settingsGetPointer()->design = 7;
        return;
    }

    if(status.base != BaseHome)
        return;

    t4screen.FBStartAdress = getFrame(25);
    t4_refresh_divemode();
    GFX_SetFramesTopBottom(t4screen.FBStartAdress, 0,480);
    releaseAllFramesExcept(25,t4screen.FBStartAdress);
}


/* Private functions ---------------------------------------------------------*/

void t4_refresh_divemode(void)
{
    char text[512];
//	uint8_t textpointer = 0;
//	uint8_t  customview_warnings = 0;

    point_t start, stop;

    start.x = 0;
    stop.x = t4SeperationLeftRight;
    stop.y = start.y = t4SeperationTopMid;
    GFX_draw_line(&t4screen, start, stop, CLUT_Font020);

    stop.y = start.y = t4SeperationMidBottom;
    GFX_draw_line(&t4screen, start, stop, CLUT_Font020);

    start.y = 0;
    stop.y = 479;
    stop.x = start.x = t4SeperationLeftRight;
    GFX_draw_line(&t4screen, start, stop, CLUT_Font020);


    // depth
    float depth = unit_depth_float(stateUsed->lifeData.depth_meter);

    if(depth <= 0.3f)
        depth = 0;

    snprintf(text,TEXTSIZE,"\032\f%c",TXT_Depth);
    GFX_write_string(&FontT24,&t4l1,text,0);

    if( depth < 100)
        snprintf(text,TEXTSIZE,"\020%01.1f",depth);
    else
        snprintf(text,TEXTSIZE,"\020%01.0f",depth);

    Gfx_colorsscheme_mod(text,0);
    GFX_write_string(&FontT144,&t4l1,text,1);

    // divetime
    SDivetime Divetime = {0,0,0, 0};

    Divetime.Total = stateUsed->lifeData.dive_time_seconds_without_surface_time;
    Divetime.Minutes = Divetime.Total / 60;
    Divetime.Seconds = Divetime.Total - ( Divetime.Minutes * 60 );

    SDivetime TimeoutTime = {0,0,0,0};
    TimeoutTime.Total = settingsGetPointer()->timeoutDiveReachedZeroDepth - stateUsed->lifeData.counterSecondsShallowDepth;
    if(TimeoutTime.Total > settingsGetPointer()->timeoutDiveReachedZeroDepth)
    {
        TimeoutTime.Total = 0;
    }
    TimeoutTime.Minutes = TimeoutTime.Total / 60;
    TimeoutTime.Seconds = TimeoutTime.Total - (TimeoutTime.Minutes * 60);

    if(stateUsed->lifeData.counterSecondsShallowDepth)
    {
        snprintf(text,TEXTSIZE,"\f\136 %u:%02u",TimeoutTime.Minutes, TimeoutTime.Seconds);
        GFX_write_string(&FontT42,&t4l2,text,0);
    }
    else
    {
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_Divetime);
        GFX_write_string(&FontT42,&t4l2,text,0);
    }

    if(Divetime.Minutes < 1000)
        snprintf(text,TEXTSIZE,"\020\016%u:%02u",Divetime.Minutes, Divetime.Seconds);
    else
        snprintf(text,TEXTSIZE,"\020\016%u'",Divetime.Minutes);
    Gfx_colorsscheme_mod(text,0);
    GFX_write_string(&FontT105,&t4l2,text,1);
}




