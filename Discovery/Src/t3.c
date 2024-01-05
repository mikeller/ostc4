///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/t3.c
/// \brief  Main Template file for dive mode special scree t3
/// \author Heinrichs Weikamp gmbh
/// \date   10-Nov-2014
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
#include <stdbool.h>

#include "t3.h"

#include "data_exchange_main.h"
#include "decom.h"
#include "gfx_fonts.h"
#include "math.h"
#include "tHome.h"
#include "timer.h"
#include "unit.h"
#include "motion.h"
#include "logbook_miniLive.h"
#include "tMenuEditCustom.h"


#define CV_PROFILE_WIDTH		(600U)

//* Imported function prototypes ---------------------------------------------*/
extern uint8_t write_gas(char *text, uint8_t oxygen, uint8_t helium);

/* Exported variables --------------------------------------------------------*/

const uint16_t BigFontSeperationLeftRight = 399;
const uint16_t BigFontSeperationTopBottom = 240;

/* Private variables ---------------------------------------------------------*/
GFX_DrawCfgScreen	t3screen;
GFX_DrawCfgWindow	t3l1;
GFX_DrawCfgWindow	t3r1;
GFX_DrawCfgWindow	t3c1;
GFX_DrawCfgWindow	t3c2;

uint8_t t3_selection_customview = CVIEW_noneOrDebug;

/* TEM HAS TO MOVE TO GLOBAL--------------------------------------------------*/

/* Private types -------------------------------------------------------------*/
#define TEXTSIZE 16

/* Private function prototypes -----------------------------------------------*/
void t3_refresh_divemode(void);

uint8_t t3_test_customview_warnings(void);
void t3_refresh_customview(float depth);
void t3_basics_compass(GFX_DrawCfgScreen *tXscreen, point_t center, uint16_t ActualHeading, uint16_t UserSetHeading);

/* Exported functions --------------------------------------------------------*/

void t3_init(void)
{
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    t3_selection_customview = cv_changelist_BS[0];

    t3screen.FBStartAdress = 0;
    t3screen.ImageHeight = 480;
    t3screen.ImageWidth = 800;
    t3screen.LayerIndex = 1;

    t3l1.Image = &t3screen;
    t3l1.WindowNumberOfTextLines = 2;
    t3l1.WindowLineSpacing = 19; // Abstand von Y0
    t3l1.WindowTab = 100;

    if(!pSettings->FlipDisplay)
    {
		t3l1.WindowX0 = 0;
		t3l1.WindowX1 = BigFontSeperationLeftRight - 5;
		t3l1.WindowY0 = BigFontSeperationTopBottom + 5;
		t3l1.WindowY1 = 479;
    }
    else
    {
		t3l1.WindowX0 = 800 - BigFontSeperationLeftRight + 5;
		t3l1.WindowX1 = 799;
		t3l1.WindowY0 = 0;
		t3l1.WindowY1 = 479 - BigFontSeperationTopBottom + 5 ;
    }

    t3r1.Image = &t3screen;
    t3r1.WindowNumberOfTextLines = t3l1.WindowNumberOfTextLines;
    t3r1.WindowLineSpacing = t3l1.WindowLineSpacing;
    t3r1.WindowTab = t3l1.WindowTab;
    if(!pSettings->FlipDisplay)
    {
		t3r1.WindowX0 = BigFontSeperationLeftRight + 5;
		t3r1.WindowX1 = 799;
    }
    else
    {
		t3r1.WindowX0 = 0;
		t3r1.WindowX1 = BigFontSeperationLeftRight - 5;
    }

    t3r1.WindowY0 = t3l1.WindowY0;
    t3r1.WindowY1 = t3l1.WindowY1;

    /* t3c1 is across the complete lower part of the display */
    t3c1.Image = &t3screen;
    t3c1.WindowNumberOfTextLines = 2;
    t3c1.WindowLineSpacing = 84 + 5; /* double font + spacing */
    t3c1.WindowX0 = 0;
    t3c1.WindowX1 = 799;
    if(!pSettings->FlipDisplay)
    {
    	t3c1.WindowY0 = 5;
    	t3c1.WindowY1 = BigFontSeperationTopBottom - 5;
    }
	else
	{
		t3c1.WindowY0 = 480 - BigFontSeperationTopBottom + 5;
		t3c1.WindowY1 = 479 - 5;
	}

    /* t3c2 is just showing the lower right part of the display */
    t3c2.Image = &t3screen;
    t3c2.WindowNumberOfTextLines = 3;
    t3c2.WindowLineSpacing = t3c1.WindowLineSpacing ;
    t3c2.WindowX0 = 370;
    t3c2.WindowX1 = 799;
    t3c2.WindowY0 = t3c1.WindowY0;
    t3c2.WindowY1 = t3c1.WindowY1;
    t3c2.WindowTab = 600;
}

void t3_select_customview(uint8_t selectedCustomview)
{
	if(selectedCustomview < CVIEW_T3_END)
	{
		t3_selection_customview = selectedCustomview;
	}
}

void t3_drawMarker(GFX_DrawCfgScreen *hgfx, const  SWindowGimpStyle *window, uint8_t *data, uint16_t datalength, uint8_t color)
{
	uint16_t line = 0;
	uint16_t dataIndex = 0;
	uint16_t lastDataIndex = 0;
	uint16_t windowWidth = 0;
	int16_t factor = 0;
	uint8_t setMarker = 0;

	 point_t start;
	 point_t stop;


	if( (window->bottom <= 479)
		&& (window->top <= 479)
		&& (window->right <= 799)
		&& (window->left <= 799)
		&& (window->right >= 0)
		&& (window->left >= 0)
		&& (window->bottom > window->top)
		&& (window->right > window->left))
	{
		windowWidth = window->right - window->left;

	    start.y = 479 - BigFontSeperationTopBottom - 5;
	   	stop.y =5;

		while((line <= windowWidth) && (dataIndex < datalength))
		{
			factor = (10 * line * (long)datalength)/windowWidth;
			dataIndex = factor/10;
		/* check if a marker is set in the intervall which is bypassed because of data reduction */
			setMarker = 0;
			while(lastDataIndex <= dataIndex)
			{
				lastDataIndex++;
				if(data[lastDataIndex] != 0)
				{
					setMarker = 1;
					break;
				}
			}
			lastDataIndex = dataIndex;
			int rest = factor - dataIndex*10;
			if(rest >= 5)
				dataIndex++;

			if((datalength - 1) < dataIndex)
				dataIndex = datalength-1;

			if((line > 0) && (setMarker))		/* draw marker line */
			{
				start.x = line;
				stop.x = line;
				GFX_draw_line(hgfx, start, stop, color);
			}
			line++;
			dataIndex++;
		}
	}
}

void t3_miniLiveLogProfile(void)
{
	static uint8_t wasDecoDive = 0;

    SWindowGimpStyle wintemp;
    uint16_t replayDataLength = 0;
    uint16_t liveDataLength = 0;
    uint16_t drawDataLength = 0;
    uint16_t* pReplayData;
    uint8_t* pReplayMarker;
    uint16_t max_depth = 10;
    char text[TEXTSIZE];
    point_t start, stop;
    uint16_t diveMinutes = 0;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

   	wintemp.top = 479 - BigFontSeperationTopBottom + 5;
   	wintemp.bottom = 479 - 5;

    if(!pSettings->FlipDisplay)
    {
        wintemp.left = t3c1.WindowX0;
        wintemp.right = t3c1.WindowX0 + CV_PROFILE_WIDTH;
    }
    else
    {
        wintemp.left = t3c1.WindowX1 - CV_PROFILE_WIDTH;;
        wintemp.right = t3c1.WindowX1;
    }

    start.x = CV_PROFILE_WIDTH + 2;
    stop.x = start.x;
    start.y = 479 - BigFontSeperationTopBottom - 5;
	stop.y =5;

   	GFX_draw_line(&t3screen, start, stop, CLUT_Font020);

   	if(getReplayOffset() != 0xFFFF)
   	{
		getReplayInfo(&pReplayData, &pReplayMarker, &replayDataLength, &max_depth, &diveMinutes);
   	}

   	if(max_depth < (uint16_t)(stateUsed->lifeData.max_depth_meter * 100))
	{
		max_depth = (uint16_t)(stateUsed->lifeData.max_depth_meter * 100);
	}

   	liveDataLength = getMiniLiveReplayLength();

   	if(replayDataLength > liveDataLength)
   	{
   		drawDataLength = replayDataLength;
   	}
   	else
   	{
   		drawDataLength = liveDataLength;
   	}

	if(drawDataLength < CV_PROFILE_WIDTH)
	{
		drawDataLength = CV_PROFILE_WIDTH;
	}

	if(diveMinutes != 0)
	{
		snprintf(text,TEXTSIZE,"\002%dmin",diveMinutes);
		GFX_write_string(&FontT42,&t3c1,text,1);
	}

    snprintf(text,TEXTSIZE,"\002%01.1fm", max_depth / 100.0);
    GFX_write_string(&FontT42,&t3c1,text,0);

    if(getMiniLiveReplayLength() < 10)		/* new dive startet => reset the visualization state for deco data */
    {
    	wasDecoDive = 0;
    }

    const SDecoinfo * pDecoinfo = getDecoInfo();
    if((pDecoinfo->output_time_to_surface_seconds) || (wasDecoDive))		/* draw deco data first => will be overlayed by all other informations */
    {
    	wasDecoDive = 1;
    	GFX_graph_print(&t3screen,&wintemp,wintemp.top * -1,1,0,max_depth, getMiniLiveDecoPointerToData(),drawDataLength, CLUT_NiceGreen, NULL);
    }

	if(replayDataLength != 0)
	{
		GFX_graph_print(&t3screen, &wintemp, 0,1,0, max_depth, pReplayData, drawDataLength, CLUT_Font031, NULL);
		if(pReplayMarker[0] != 0xFF)
		{
			t3_drawMarker(&t3screen, &wintemp, pReplayMarker, drawDataLength, CLUT_CompassUserHeadingTick);
		}
	}

    if(liveDataLength > 3)
    {
    	GFX_graph_print(&t3screen, &wintemp, 0,1,0, max_depth, getMiniLiveReplayPointerToData(), drawDataLength, CLUT_Font030, NULL);
    }
}


void t3_refresh(void)
{
	static uint8_t last_mode = MODE_SURFACE;

    SStateList status;
    get_globalStateList(&status);

    if(stateUsed->mode != MODE_DIVE)
    {
        settingsGetPointer()->design = 7;
        return;
    }

    if(status.base != BaseHome)
        return;

    if(last_mode != MODE_DIVE)			/* Select custom view */
    {
    	if((settingsGetPointer()->tX_customViewTimeout == 0) && (settingsGetPointer()->showDebugInfo))
    	{
    		t3_selection_customview = CVIEW_noneOrDebug;
    	}
    	else
    	{
    		t3_selection_customview = settingsGetPointer()->tX_customViewPrimaryBF;
    	}
    	t3_change_customview(ACTION_END);
    }
    t3screen.FBStartAdress = getFrame(24);
    t3_refresh_divemode();
    GFX_SetFramesTopBottom(t3screen.FBStartAdress, 0,480);
    releaseAllFramesExcept(24,t3screen.FBStartAdress);
    last_mode = stateUsed->mode;
}

void t3_set_customview_to_primary(void)
{
    if(stateUsed->mode == MODE_DIVE)
    {
    	t3_selection_customview = settingsGetPointer()->tX_customViewPrimaryBF;
    	t3_change_customview(ACTION_END);
    }
}

/* Private functions ---------------------------------------------------------*/

float t3_basics_lines_depth_and_divetime(GFX_DrawCfgScreen *tXscreen, GFX_DrawCfgWindow* tXl1, GFX_DrawCfgWindow* tXr1, uint8_t mode)
{
    char text[256];
    uint8_t textPointer;
    uint8_t color;
    uint8_t depthChangeRate;
    uint8_t depthChangeAscent;
    point_t start, stop, startZeroLine;
    SDivetime Divetime = {0,0,0,0};

	SSettings* pSettings;
	pSettings = settingsGetPointer();

    start.x = 0;
    stop.x = 799;
    stop.y = start.y = BigFontSeperationTopBottom;
    if((viewInFocus()) && (!viewDetectionSuspended()))
    {
    	GFX_draw_line(tXscreen, start, stop, CLUT_Font023);
    }
    else
    {
    	GFX_draw_line(tXscreen, start, stop, CLUT_Font020);
    }


    start.y = BigFontSeperationTopBottom;
    stop.y = 479;

    stop.x = start.x = BigFontSeperationLeftRight;
    if((viewInFocus() && (!viewDetectionSuspended())))
    {
    	GFX_draw_line(tXscreen, start, stop, CLUT_Font023);
    }
    else
    {
    	GFX_draw_line(tXscreen, start, stop, CLUT_Font020);
    }

    /* depth */
    float depth = unit_depth_float(stateUsed->lifeData.depth_meter);

    if(depth <= 0.3f)
        depth = 0;

    if(settingsGetPointer()->nonMetricalSystem)
        snprintf(text,TEXTSIZE,"\032\f[feet]");
    else
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_Depth);
    GFX_write_string(&FontT42,tXl1,text,0);

    if(			((mode == DIVEMODE_Apnea) && ((stateUsed->lifeData.ascent_rate_meter_per_min > 4) || (stateUsed->lifeData.ascent_rate_meter_per_min < -4 )))
            || 	((mode != DIVEMODE_Apnea) && ((stateUsed->lifeData.ascent_rate_meter_per_min > 8) || (stateUsed->lifeData.ascent_rate_meter_per_min < -10)))
        )
    {
        snprintf(text,TEXTSIZE,"\f\002%.0f %c%c/min  "
            , unit_depth_float(stateUsed->lifeData.ascent_rate_meter_per_min)
            , unit_depth_char1()
            , unit_depth_char2()
        );
        GFX_write_string(&FontT42,tXl1,text,0);
    }

    if( depth < 100)
        snprintf(text,TEXTSIZE,"\020\003\016%01.1f",depth);
    else
        snprintf(text,TEXTSIZE,"\020\003\016%01.0f",depth);

    t3_basics_colorscheme_mod(text);
    GFX_write_string(&FontT105,tXl1,text,1);


    /* ascentrate graph */
    if(mode == DIVEMODE_Apnea)
    {
        /* ascentrate graph - apnea mode */
        if(stateUsed->lifeData.ascent_rate_meter_per_min > 0)
        {
            depthChangeAscent = 1;
            if(stateUsed->lifeData.ascent_rate_meter_per_min < 200)
                depthChangeRate = (uint8_t)stateUsed->lifeData.ascent_rate_meter_per_min;
            else
                depthChangeRate = 200;
        }
        else
        {
            depthChangeAscent = 0;
            if(stateUsed->lifeData.ascent_rate_meter_per_min > -200)
                depthChangeRate = (uint8_t)(0 - stateUsed->lifeData.ascent_rate_meter_per_min);
            else
                depthChangeRate = 200;
        }

        if(!pSettings->FlipDisplay)
        {
        	start.y = tXl1->WindowY0 - 1;
        }
        else
        {
        	start.y = tXl1->WindowY1 + 1;
        }
        startZeroLine.y = start.y;
        for(int i = 0; i<5;i++)
        {
            start.y += 40;
            stop.y = start.y;
            if(!pSettings->FlipDisplay)
            {
            	start.x = tXl1->WindowX1 - 1;
            }
            else
            {
            	start.x = tXr1->WindowX1 - 1;
            }
            stop.x = start.x - 17;

            if(depthChangeRate <= 6)
            {
                if(i == 2)
                {
                    startZeroLine.y = start.y;
                    stop.x = start.x - 34;
                }
            }
            else
            {
                if(((i == 1) && depthChangeAscent) || ((i == 3) && !depthChangeAscent))
                {
                    startZeroLine.y = start.y;
                    stop.x = start.x - 34;
                }
            }
            GFX_draw_line(tXscreen, start, stop, 0);
        }
        // new thick bar design Sept. 2015
        if((stateUsed->lifeData.ascent_rate_meter_per_min > 4) || (stateUsed->lifeData.ascent_rate_meter_per_min < -4))
        {
            start.y = startZeroLine.y;
            if(depthChangeAscent)
            {
                color = CLUT_EverythingOkayGreen;
                start.y += 7; // starte etwas weiter oben
                stop.y = start.y + (uint16_t)(depthChangeRate * 4) - 9; // - x; // wegen der Liniendicke
                if(stop.y > 475)
                    stop.y = 475;
            }
            else
            {
                color = CLUT_Font023;
                start.y -= 7;
                stop.y = start.y - (uint16_t)(depthChangeRate * 4) + 9;
                if(stop.y <= tXl1->WindowY0)
                    stop.y = tXl1->WindowY0 + 1;
            }
            if(!pSettings->FlipDisplay)
            {
            	start.x = tXl1->WindowX1 - 3 - 5;
            }
            else
            {
            	start.x = tXr1->WindowX1 - 3 - 5;
            }

            stop.x = start.x;
            GFX_draw_thick_line(12,tXscreen, start, stop, color);
        }
    }
    else
    {
        /* ascentrate graph -standard mode */
        if(stateUsed->lifeData.ascent_rate_meter_per_min > 0)
        {
        	 if(!pSettings->FlipDisplay)
        	 {
        		 start.y = tXl1->WindowY0 - 1;
        	 }
        	 else
        	 {
        		 start.y = tXl1->WindowY1 + 1;
        	 }

            for(int i = 0; i<4;i++)
            {
                start.y += 5*8;
                stop.y = start.y;
                if(!pSettings->FlipDisplay)
                {
                	start.x = tXl1->WindowX1 - 1;
                }
                else
                {
                	start.x = tXr1->WindowX1 - 1;
                }
                stop.x = start.x - 17;
                GFX_draw_line(tXscreen, start, stop, 0);
            }
            // new thick bar design Sept. 2015
            if(!pSettings->FlipDisplay)
            {
            	start.x = tXl1->WindowX1 - 3 - 5;
            }
            else
            {
            	start.x = tXr1->WindowX1 - 3 - 5;
            }

            stop.x = start.x;
            if(!pSettings->FlipDisplay)
            {
            	start.y = tXl1->WindowY0 - 1;
            }
            else
            {
            	start.y = tXl1->WindowY1 + 1;
            }

            stop.y = start.y + (uint16_t)(stateUsed->lifeData.ascent_rate_meter_per_min * 8);
            stop.y -= 3; // wegen der Liniendicke von 12 anstelle von 9
            if(stop.y >= 470)
                stop.y = 470;
            start.y += 7; // starte etwas weiter oben
            if(stateUsed->lifeData.ascent_rate_meter_per_min <= 10)
                color = CLUT_EverythingOkayGreen;
            else
            if(stateUsed->lifeData.ascent_rate_meter_per_min <= 15)
                color = CLUT_WarningYellow;
            else
                color = CLUT_WarningRed;

            GFX_draw_thick_line(12,tXscreen, start, stop, color);
        }
    }

    // divetime
    if(mode == DIVEMODE_Apnea)
    {
        if(stateUsed->lifeData.counterSecondsShallowDepth)
        {
            SDivetime SurfaceBreakTime = {0,0,0,0};

            SurfaceBreakTime.Total = stateUsed->lifeData.counterSecondsShallowDepth;
            SurfaceBreakTime.Minutes = SurfaceBreakTime.Total / 60;
            SurfaceBreakTime.Seconds = SurfaceBreakTime.Total - (SurfaceBreakTime.Minutes * 60);

            snprintf(text,TEXTSIZE,"\032\f\002%c%c", TXT_2BYTE,TXT2BYTE_ApneaSurface);
            GFX_write_string(&FontT42,tXr1,text,0);

            snprintf(text,TEXTSIZE,"\020\003\002\016%u:%02u",SurfaceBreakTime.Minutes, SurfaceBreakTime.Seconds);
        }
        else
        {
            Divetime.Total = stateUsed->lifeData.dive_time_seconds;
            Divetime.Minutes = Divetime.Total / 60;
            Divetime.Seconds = Divetime.Total - ( Divetime.Minutes * 60 );

            snprintf(text,TEXTSIZE,"\032\f\002%c",TXT_Divetime);
            GFX_write_string(&FontT42,tXr1,text,0);

            if(Divetime.Minutes < 100)
                snprintf(text,TEXTSIZE,"\020\003\002\016%u:%02u",Divetime.Minutes, Divetime.Seconds);
            else
                snprintf(text,TEXTSIZE,"\020\003\002\016%u'",Divetime.Minutes);
        }
        t3_basics_colorscheme_mod(text);
        GFX_write_string(&FontT105,tXr1,text,1);
    }
    else
    {
    	switch(get_globalState())
    	{
    		case StDMENU:   snprintf(text,TEXTSIZE,"\a\003\001%c%c", TXT_2BYTE, TXT2BYTE_DiveMenuQ);
    						GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;
    		case StDBEAR:   snprintf(text,TEXTSIZE,"\a\003\001%c%c", TXT_2BYTE, TXT2BYTE_DiveBearingQ);
            				GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
            	break;
    		case StDRAVG:	snprintf(text,TEXTSIZE,"\a\003\001%c%c", TXT_2BYTE, TXT2BYTE_DiveResetAvgQ);
                			GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
                break;

    		case StDMGAS:
    	        			textPointer = 0;
    						text[textPointer++] = '\a';
    						text[textPointer++] = '\001';
    						text[textPointer++] = ' ';
    	        			textPointer += tHome_gas_writer(stateUsed->diveSettings.gas[actualBetterGasId()].oxygen_percentage,stateUsed->diveSettings.gas[actualBetterGasId()].helium_percentage,&text[textPointer]);
    	        			text[textPointer++] = '?';
    	        			text[textPointer++] = ' ';
    	        			text[textPointer++] = 0;
    	        			GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    	        break;

    		case StDMARK:	snprintf(text,TEXTSIZE,"\a\003\001%c%c", TXT_2BYTE, TXT2BYTE_SetMarkerShort);
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;

    		case StDCHECK:	snprintf(text,TEXTSIZE,"\a\003\001%c%c", TXT_2BYTE, TXT2BYTE_CheckMarker);
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;

#ifdef ENABLE_T3_PPO_SIM
    		case StDSIM1:	snprintf(text,TEXTSIZE,"\a\003\001PPO S0 +");
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;
    		case StDSIM2:	snprintf(text,TEXTSIZE,"\a\003\001PPO S0 -");
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;
    		case StDSIM3:	snprintf(text,TEXTSIZE,"\a\003\001PPO S1 +");
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;
    		case StDSIM4:	snprintf(text,TEXTSIZE,"\a\003\001PPO S1 -");
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;
    		case StDSIM5:	snprintf(text,TEXTSIZE,"\a\003\001PPO S2 +");
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;
    		case StDSIM6:	snprintf(text,TEXTSIZE,"\a\003\001PPO S2 -");
							GFX_write_string_color(&FontT42,tXr1,text,1,CLUT_WarningYellow);
    			break;
#endif
    		default:		/* show divetime */

							Divetime.Total = stateUsed->lifeData.dive_time_seconds_without_surface_time;
							Divetime.Minutes = Divetime.Total / 60;
							Divetime.Seconds = Divetime.Total - ( Divetime.Minutes * 60 );

							snprintf(text,TEXTSIZE,"\032\f\002%c",TXT_Divetime);
							GFX_write_string(&FontT42,tXr1,text,0);

							if(Divetime.Minutes < 100)
								snprintf(text,TEXTSIZE,"\020\003\002\016%u:%02u",Divetime.Minutes, Divetime.Seconds);
							else
								snprintf(text,TEXTSIZE,"\020\003\002\016%u'",Divetime.Minutes);

						    t3_basics_colorscheme_mod(text);
						    GFX_write_string(&FontT105,tXr1,text,1);
				break;
    	}
    }

    return depth;
}


void t3_refresh_divemode(void)
{
    uint8_t  customview_warnings = 0;
    float depth_meter = 0.0;

    // everything like lines, depth, ascent graph and divetime
    depth_meter = t3_basics_lines_depth_and_divetime(&t3screen, &t3l1, &t3r1, 0); // 0 could be stateUsed->diveSettings.diveMode for CCR specials

    // customview
    if(stateUsed->warnings.numWarnings)
        customview_warnings = t3_test_customview_warnings();

    if(customview_warnings && warning_count_high_time)
        t3_basics_show_customview_warnings(&t3c1);
    else
        t3_refresh_customview(depth_meter);

    if(stateUsed->warnings.lowBattery)
        t3_basics_battery_low_customview_extra(&t3r1); //t3c1);
}


void t3_basics_battery_low_customview_extra(GFX_DrawCfgWindow* tXc1)
{
    char TextC1[256];

    TextC1[0] = ' ';//'\002';
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

    GFX_write_string(&Batt24,tXc1,TextC1,0);
}



void t3_refresh_customview(float depth)
{
    t3_basics_refresh_customview(depth, t3_selection_customview, &t3screen, &t3c1, &t3c2, stateUsedWrite->diveSettings.diveMode);
}


void t3_basics_refresh_apnoeRight(float depth, uint8_t tX_selection_customview, GFX_DrawCfgScreen *tXscreen, GFX_DrawCfgWindow* tXc1, GFX_DrawCfgWindow* tXc2, uint8_t mode)
{
    char text[30];
    uint16_t textpointer = 0;

    // CVIEW_T3_Temperature
    float temperature;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    SDivetime TotalDivetime = {0,0,0,0};
    SDivetime LastDivetime = {0,0,0,0};

    uint16_t tempWinX0;
    uint16_t tempWinX1;
    uint16_t tempWinY0;
    uint16_t tempWinY1;

    tempWinX0 = tXc1->WindowX0;
    tempWinX1 = tXc1->WindowX1;
    tempWinY0 = tXc1->WindowY0;
    tempWinY1 = tXc1->WindowY1;

    tXc1->WindowX0 = 440; // rechte Seite

    switch(tX_selection_customview)
    {
    case CVIEW_T3_Temperature:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_Temperature);
        GFX_write_string(&FontT42,tXc1,text,0);

        temperature = unit_temperature_float(stateUsed->lifeData.temperature_celsius);
        textpointer = snprintf(text,TEXTSIZE,"\020\003\016%01.0f\016\016\140",temperature); // "\016\016%01.1f `" + C or F
        if(settingsGetPointer()->nonMetricalSystem == 0)
            text[textpointer++] = 'C';
        else
            text[textpointer++] = 'F';
        text[textpointer++] = 0;
        t3_basics_colorscheme_mod(text);
        GFX_write_string(&FontT105,tXc1,text,1);
        break;

    case CVIEW_T3_ApnoeSurfaceInfo:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_Divetime);
        GFX_write_string(&FontT42,tXc1,text,0);

        TotalDivetime.Total = stateUsed->lifeData.dive_time_seconds_without_surface_time;
        TotalDivetime.Minutes = TotalDivetime.Total / 60;
        TotalDivetime.Seconds = TotalDivetime.Total - ( TotalDivetime.Minutes * 60 );

        LastDivetime.Total = stateUsed->lifeData.apnea_last_dive_time_seconds;
        LastDivetime.Minutes = LastDivetime.Total / 60;
        LastDivetime.Seconds = LastDivetime.Total - ( LastDivetime.Minutes * 60 );

   //     tXc1->WindowY0 = 100; // obere Zeile
        if(!pSettings->FlipDisplay)
        {
        	tXc1->WindowY0 = 100;
        }
        else
        {
        	tXc1->WindowY1 -= 100; /* jump to upper of two lines */
        }

        snprintf(text,TEXTSIZE,"\020\016%u:%02u",LastDivetime.Minutes, LastDivetime.Seconds);
        t3_basics_colorscheme_mod(text);
        GFX_write_string(&FontT105,tXc1,text,0);

        if(pSettings->FlipDisplay)
        {
            tXc1->WindowX0 = 0;

        }
        snprintf(text,TEXTSIZE,"\032\002%c%c",TXT_2BYTE, TXT2BYTE_ApneaLast);
        GFX_write_string(&FontT42,tXc1,text,0);

        if(!pSettings->FlipDisplay)
        {
        	tXc1->WindowY0 = tempWinY0;
        }
        else
        {
            tXc1->WindowX1 = tempWinX1;
        	tXc1->WindowY1 = tempWinY1; /* jump to upper of two lines */
        }

        snprintf(text,TEXTSIZE,"\020\016%u:%02u",TotalDivetime.Minutes, TotalDivetime.Seconds);
        t3_basics_colorscheme_mod(text);
        GFX_write_string(&FontT105,tXc1,text,0);

        snprintf(text,TEXTSIZE,"\032\002%c%c",TXT_2BYTE, TXT2BYTE_ApneaTotal);
        if(pSettings->FlipDisplay)
        {
            tXc1->WindowX0 = 0;

        }
        GFX_write_string(&FontT42,tXc1,text,0);
        break;
    }

    tXc1->WindowX0 = tempWinX0;
    tXc1->WindowX1 = tempWinX1;
    tXc1->WindowY0 = tempWinY0;

}


void t3_basics_refresh_customview(float depth, uint8_t tX_selection_customview, GFX_DrawCfgScreen *tXscreen, GFX_DrawCfgWindow* tXc1, GFX_DrawCfgWindow* tXc2, uint8_t mode)
{
	static uint8_t last_customview = CVIEW_T3_END;

    char text[30];
    uint16_t textpointer = 0;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

    // CVIEW_T3_Decostop and CVIEW_T3_TTS
    const SDecoinfo * pDecoinfo = getDecoInfo();

    // CVIEW_T3_Decostop
    uint16_t 	nextstopLengthSeconds = 0;
    uint8_t 	nextstopDepthMeter = 0;
    SDivetime SafetyStopTime = {0,0,0,0};

    // CVIEW_T3_ppO2andGas
    uint8_t oxygen_percentage = 0;

    // CVIEW_T3_Temperature
    float temperature;

    // CVIEW_T3_GasList
    float fPpO2limitHigh, fPpO2limitLow, fPpO2ofGasAtThisDepth;
    const SGasLine * pGasLine;
    uint8_t oxygen, helium;
    uint8_t lineNumber;
    uint8_t gasPosIdx;

    /* compass position */
    point_t center;
    uint16_t heading;

    // CVIEW_T3_StopWatch
    SDivetime Stopwatch = {0,0,0,0};
    float fAverageDepth, fAverageDepthAbsolute;

#ifdef ENABLE_PSCR_MODE
    uint8_t showSimPPO2 = 1;
#endif
    uint16_t tempWinX0;
    uint16_t tempWinX1;
    uint16_t tempWinY0;
    uint16_t tempWinY1;
    uint16_t tempWinC2X0;
    uint16_t tempWinC2Y0;
    uint16_t tempWinC2X1;
    uint16_t tempWinC2Y1;
    uint16_t tempWinC2Tab;

    tempWinX0 = tXc1->WindowX0;
    tempWinY0 = tXc1->WindowY0;
    tempWinX1 = tXc1->WindowX1;
    tempWinY1 = tXc1->WindowY1;

    tempWinC2X0  = tXc2->WindowX0;
    tempWinC2Y0 = tXc2->WindowY0;
    tempWinC2X1  = tXc2->WindowX1;
    tempWinC2Y1 = tXc2->WindowY1;
    tempWinC2Tab = tXc2->WindowTab;

    if(settingsGetPointer()->compassInertia)
    {
    	heading = (uint16_t)compass_getCompensated();
    }
    else
    {
    	heading = (uint16_t)stateUsed->lifeData.compass_heading;
    }
	if((last_customview != tX_selection_customview)	&& (settingsGetPointer()->design == 3))	/* check if current selection is disabled and should be skipped */
	{
		if(t3_customview_disabled(tX_selection_customview))
		{
			tX_selection_customview = t3_change_customview(ACTION_BUTTON_ENTER);
		}
		last_customview = tX_selection_customview;
	}

    switch(tX_selection_customview)
    {
    case CVIEW_T3_ApnoeSurfaceInfo:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_MaxDepth);

        if(!pSettings->FlipDisplay)
        {
	        GFX_write_string(&FontT42,tXc1,text,0);
        	tXc1->WindowY0 = 100;
        }
        else
        {
	        GFX_write_string(&FontT42,tXc2,text,0);
        	tXc2->WindowY1 -= 100; /* jump to upper of two lines */
        }

        snprintf(text,TEXTSIZE,"\020\016%01.1f",unit_depth_float(stateUsed->lifeData.apnea_last_max_depth_meter));
        t3_basics_colorscheme_mod(text);
        
        if(!pSettings->FlipDisplay)
        {
	        GFX_write_string(&FontT105,tXc1,text,0);
        	tXc1->WindowY0 = tempWinY0;
        }
        else
        {
	        GFX_write_string(&FontT105,tXc2,text,0);
        	tXc2->WindowY1 = tempWinC2Y1; /* jump to upper of two lines */
        }


        snprintf(text,TEXTSIZE,"\020\016%01.1f",unit_depth_float(stateUsed->lifeData.apnea_total_max_depth_meter));
        t3_basics_colorscheme_mod(text);
        if(!pSettings->FlipDisplay)
        {
		    GFX_write_string(&FontT105,tXc1,text,0);
        }
        else
        {
	        GFX_write_string(&FontT105,tXc2,text,0);
        }
        break;

    case CVIEW_T3_StopWatch:

        Stopwatch.Total = timer_Stopwatch_GetTime();
        Stopwatch.Minutes = Stopwatch.Total / 60;
        Stopwatch.Seconds = Stopwatch.Total - ( Stopwatch.Minutes * 60 );
        fAverageDepth = timer_Stopwatch_GetAvarageDepth_Meter();
        fAverageDepthAbsolute = stateUsed->lifeData.average_depth_meter;

        snprintf(text,TEXTSIZE,"\032\f%c",TXT_AvgDepth);
        GFX_write_string(&FontT42,tXc1,text,0);
        snprintf(text,TEXTSIZE,"\030\003\016%01.1f",unit_depth_float(fAverageDepthAbsolute));
        GFX_write_string(&FontT105,tXc1,text,0);

        if(!pSettings->FlipDisplay)
        {
        	tXc1->WindowX0 = 480;
        }
        else
        {
        	tXc1->WindowX1 = 320;
        	tXc1->WindowY0 = t3c1.WindowY0; /* select customer window */
        }
//			snprintf(text,TEXTSIZE,"\032\f%c%c - %c",TXT_2BYTE, TXT2BYTE_Clock, TXT_AvgDepth);

        snprintf(text,TEXTSIZE,"\032\f%c", TXT_Stopwatch);
        GFX_write_string(&FontT42,tXc1,text,0);
        snprintf(text,TEXTSIZE,"\030\016%01.1f",unit_depth_float(fAverageDepth));
        GFX_write_string(&FontT105,tXc1,text,0);
        if(!pSettings->FlipDisplay)
        {
        	tXc1->WindowY0 = 100;
        }
        else
        {
        	tXc1->WindowY1 -= 100; /* jump to upper of two lines */
        }

        snprintf(text,TEXTSIZE,"\030%u:\016\016%02u",Stopwatch.Minutes, Stopwatch.Seconds);
        GFX_write_string(&FontT105,tXc1,text,0);

       break;

    case CVIEW_T3_GasList:
    	gasPosIdx = 0;
        snprintf(text,TEXTSIZE,"\032\f%c%c",TXT_2BYTE, TXT2BYTE_Gaslist);
        GFX_write_string(&FontT42,tXc1,text,0);

        textpointer = 0;
        tXc2->WindowX0 = 0;
        tXc2->WindowTab = 800/3; // /2

        if(pSettings->FlipDisplay)
        {
        	tXc2->WindowY1 = 0;
        }

        pGasLine = settingsGetPointer()->gas;
        if(actualLeftMaxDepth(stateUsed))
            fPpO2limitHigh = (float)(settingsGetPointer()->ppO2_max_deco) / 100;
        else
            fPpO2limitHigh = (float)(settingsGetPointer()->ppO2_max_std) / 100;
        fPpO2limitLow = (float)(settingsGetPointer()->ppO2_min) / 100;
        for(int gasId=1;gasId<=NUM_GASES;gasId++)
        {
#ifdef ENABLE_UNUSED_GAS_HIDING
            if(!pGasLine[gasId].note.ub.off)
            {
#endif
            textpointer = 0;
            text[textpointer++] = '\003';

            lineNumber = 1;

            switch(gasPosIdx)
            {
            	case 0:			lineNumber = 0;
            	case 1:
            		break;
            	case 4:			text[textpointer++] = '\001';   /* display centered */
            		break;
            	case 2:			lineNumber = 0;
            	case 3:			text[textpointer++] = '\002';	/* display right aligned */
            	default:
            		break;
            }
            gasPosIdx++;

            fPpO2ofGasAtThisDepth = (stateUsed->lifeData.pressure_ambient_bar - WATER_VAPOUR_PRESSURE) * pGasLine[gasId].oxygen_percentage / 100;
            if(pGasLine[gasId].note.ub.active == 0)
                strcpy(&text[textpointer++],"\031");
            else if(stateUsed->lifeData.actualGas.GasIdInSettings == gasId)	/* actual selected gas */
            {
            	strcpy(&text[textpointer++],"\030");
            }
            else if((fPpO2ofGasAtThisDepth > fPpO2limitHigh) || (fPpO2ofGasAtThisDepth < fPpO2limitLow))
                strcpy(&text[textpointer++],"\025");
            else if(actualBetterGasId() == gasId)
            {
            	strcpy(&text[textpointer++],"\026");	/* Highlight better gas */
            }
            else
                strcpy(&text[textpointer++],"\023");	/* Blue for travel or deco without special state */

            text[textpointer++] = ' ';
            oxygen = pGasLine[gasId].oxygen_percentage;
            helium = pGasLine[gasId].helium_percentage;
            textpointer += write_gas(&text[textpointer], oxygen, helium);

            if((pGasLine[gasId].depth_meter) && (gasPosIdx < 5))	/* do not show for potential last gas because of formating issues */
            {
                textpointer += snprintf(&text[textpointer],7,"\016\016%u%c%c",unit_depth_integer(pGasLine[gasId].depth_meter), unit_depth_char1(), unit_depth_char2());
            }
            text[textpointer++] = 0;
            GFX_write_string(&FontT42, tXc1, text, lineNumber);
#ifdef ENABLE_UNUSED_GAS_HIDING
            }
#endif
        }
        break;

    case CVIEW_T3_Temperature:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_Temperature);
        GFX_write_string(&FontT42,tXc1,text,0);

        temperature = unit_temperature_float(stateUsed->lifeData.temperature_celsius);
        textpointer = snprintf(text,TEXTSIZE,"\030\003\016%01.1f \140",temperature); // "\016\016%01.1f `" + C or F
        if(settingsGetPointer()->nonMetricalSystem == 0)
            text[textpointer++] = 'C';
        else
            text[textpointer++] = 'F';
        text[textpointer++] = 0;
        GFX_write_string(&FontT105,tXc1,text,0);
        break;

    case CVIEW_Compass:
        center.x = 600;
        center.y = 116;
        snprintf(text,TEXTSIZE,"\032\f%c%c",TXT_2BYTE, TXT2BYTE_Compass);
        GFX_write_string(&FontT42,tXc1,text,0);
        snprintf(text,100,"\030\003%03i`",heading);
        GFX_write_string(&FontT105,tXc1,text,0);
        t3_basics_compass(tXscreen, center, heading, stateUsed->diveSettings.compassHeading);
        break;
#ifdef ENABLE_T3_PROFILE_VIEW
    case CVIEW_T3_Profile:
        snprintf(text,100,"\032\f\002%c%c",TXT_2BYTE,TXT2BYTE_Profile);
        GFX_write_string(&FontT42,tXc1,text,0);
        t3_miniLiveLogProfile();
    	break;
#endif
    case CVIEW_T3_DecoTTS:
    case CVIEW_T3_Decostop:
    default:
        // decostop
        if(pDecoinfo->output_time_to_surface_seconds)
        {
            tHome_findNextStop(pDecoinfo->output_stop_length_seconds, &nextstopDepthMeter, &nextstopLengthSeconds);
        }
        else
        {
            nextstopDepthMeter = 0;
            nextstopLengthSeconds = 0;
        }

        SafetyStopTime.Total = timer_Safetystop_GetCountDown();
        SafetyStopTime.Minutes = SafetyStopTime.Total / 60;
        SafetyStopTime.Seconds = SafetyStopTime.Total - (SafetyStopTime.Minutes * 60);

        if(nextstopDepthMeter)
        {
            snprintf(text,TEXTSIZE,"\032\f%c",TXT_Decostop);
            GFX_write_string(&FontT42,tXc1,text,0);

            textpointer = 0;
            snprintf(&text[textpointer],TEXTSIZE,"\020\003%u%c%c %u'"
			, unit_depth_integer(nextstopDepthMeter)
			, unit_depth_char1_T105()
			, unit_depth_char2_T105()
			, (nextstopLengthSeconds+59)/60);
			t3_basics_colorscheme_mod(text);
			GFX_write_string(&FontT105,tXc1,text,0);
        }
        else if(SafetyStopTime.Total && (depth > timer_Safetystop_GetDepthUpperLimit()))
        {
            textpointer = 0;
            snprintf(&text[textpointer],TEXTSIZE,"\032\f%c%c",TXT_2BYTE,TXT2BYTE_SafetyStop2);
            GFX_write_string(&FontT42,tXc1,text,0);

            textpointer = 0;
            snprintf(&text[textpointer],TEXTSIZE,"\020\003\016%u:%02u",SafetyStopTime.Minutes,SafetyStopTime.Seconds);
            t3_basics_colorscheme_mod(text);
            GFX_write_string(&FontT105,tXc1,text,0);
        }
        else if(pDecoinfo->output_ndl_seconds) // NDL
        {
            snprintf(text,TEXTSIZE,"\032\f%c",TXT_Nullzeit);
            GFX_write_string(&FontT42,tXc1,text,0);
            if(pDecoinfo->output_ndl_seconds < 1000 * 60)
                snprintf(text,TEXTSIZE,"\020\003%i'",pDecoinfo->output_ndl_seconds/60);
            else
                snprintf(text,TEXTSIZE,"\020\003%ih",pDecoinfo->output_ndl_seconds/3600);
            t3_basics_colorscheme_mod(text);
            GFX_write_string(&FontT105,tXc1,text,0);
        }

        if(tX_selection_customview == CVIEW_T3_DecoTTS)	/* add tts data on right side of screen */
        {
            if(pDecoinfo->output_time_to_surface_seconds)
            {
				snprintf(text,TEXTSIZE,"\002\032\f%c",TXT_TTS);
				GFX_write_string(&FontT42,tXc1,text,0);
				if(pDecoinfo->output_time_to_surface_seconds)
				{
					if(pDecoinfo->output_time_to_surface_seconds < 100 * 60)
						snprintf(text,TEXTSIZE,"\020\003\002%i'",(pDecoinfo->output_time_to_surface_seconds + 59)/ 60);
					else
						snprintf(text,TEXTSIZE,"\020\003\002%ih",(pDecoinfo->output_time_to_surface_seconds + 59)/ 3600);
					t3_basics_colorscheme_mod(text);
					GFX_write_string(&FontT105,tXc1,text,0);
				}
            }
            else if(pDecoinfo->super_saturation > 0.1)
            {
            	snprintf(text,TEXTSIZE,"\002\032\f%c",TXT_ActualGradient);
				GFX_write_string(&FontT42,tXc1,text,0);
                snprintf(text,TEXTSIZE,"\020\003\002%.0f\016\016%%\017",100 * pDecoinfo->super_saturation);
                t3_basics_colorscheme_mod(text);
                GFX_write_string(&FontT105,tXc1,text,0);
            }
        }
        break;

    case CVIEW_T3_sensors:
        snprintf(text,TEXTSIZE,"\032\f%c%c",TXT_2BYTE,TXT2BYTE_O2monitor);
        GFX_write_string(&FontT42,tXc1,text,0);

        for(int i=0;i<3;i++)
        {
            textpointer = 0;
            text[textpointer++] = '\030';
            if(i==1)
                text[textpointer++] = '\001';		/* center */
            else if(i==2)
                text[textpointer++] = '\002';		/* right  */

            if((stateUsed->diveSettings.ppo2sensors_deactivated & (1<<i)) || (stateUsed->lifeData.ppO2Sensor_bar[i] == 0.0))
            {
#ifdef ENABLE_PSCR_MODE
            	if((stateUsed->diveSettings.diveMode == DIVEMODE_PSCR) && (showSimPPO2) && (stateUsed->mode == MODE_DIVE))	/* display ppo2 sim in blue letters in case a slot is not used in the ppo2 custom view */
            	{
            		text[textpointer++] = '\023';
            		textpointer += snprintf(&text[textpointer],TEXTSIZE,"%.2f",stateUsed->lifeData.ppo2Simulated_bar);
            		showSimPPO2 = 0;
            	}
            	else
#endif
            	{
					text[textpointer++] = '\031';
					text[textpointer++] = ' ';
					text[textpointer++] = '-';
					text[textpointer++] = ' ';
					text[textpointer++] = 0;
            	}
            }
            else
            {
                if(stateUsed->warnings.sensorOutOfBounds[i])
                    text[textpointer++] = '\025';
                textpointer += snprintf(&text[textpointer],TEXTSIZE,"%.2f",stateUsed->lifeData.ppO2Sensor_bar[i]);
            }
            GFX_write_string(&FontT105,tXc1,text,0);

            if((pSettings->co2_sensor_active) && isLoopMode(pSettings->dive_mode))
            {
            	snprintf(text,TEXTSIZE,"\032\001\f%c",TXT_CO2Sensor);
                GFX_write_string(&FontT42,tXc1,text,0);
                textpointer = 0;
                if(stateUsed->lifeData.CO2_data.CO2_ppm < CO2_WARNING_LEVEL_PPM)
                {
                	text[textpointer++] = '\020';
                }
                else if(stateUsed->lifeData.CO2_data.CO2_ppm < CO2_ALARM_LEVEL_PPM)
                {
                	text[textpointer++] = '\024';	/* yellow */
                }
                else
                {
                	text[textpointer++] = '\025'; 	/* red */
                }
                snprintf(&text[textpointer],TEXTSIZE,"\001%5ld",stateUsed->lifeData.CO2_data.CO2_ppm);
                GFX_write_string(&FontT105,tXc1,text,1);
            }

            if((pSettings->scrubTimerMode != SCRUB_TIMER_OFF) && isLoopMode(pSettings->dive_mode))
            {
                 snprintf(text,TEXTSIZE,"\032\002\f%c",TXT_ScrubTime);
                 GFX_write_string(&FontT42,tXc1,text,0);

                textpointer = 0;
                text[textpointer++] = '\002';
                textpointer += printScrubberText(&text[textpointer], 10, pSettings);
                GFX_write_string(&FontT105,tXc1,text,1);
            }
        }
        break;

    case CVIEW_T3_MaxDepth:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_MaxDepth);
        if(pSettings->FlipDisplay)
        {
        	if(mode == DIVEMODE_Apnea)
        	{
        		GFX_write_string(&FontT42,tXc2,text,0);
        	}
        	else
        	{
        		GFX_write_string(&FontT42,tXc1,text,0);
        	}
        }
        else
        {
        	GFX_write_string(&FontT42,tXc1,text,0);
        }
        snprintf(text,TEXTSIZE,"\020\003\016%01.1f",unit_depth_float(stateUsed->lifeData.max_depth_meter));
        t3_basics_colorscheme_mod(text);
        if(pSettings->FlipDisplay)
        {
        	if(mode == DIVEMODE_Apnea)
        	{
        		GFX_write_string(&FontT105,tXc2,text,0);
        	}
        	else
        	{
        		GFX_write_string(&FontT105,tXc1,text,0);
        	}
        }
        else
        {
        	GFX_write_string(&FontT105,tXc1,text,0);
        }
        break;

    case CVIEW_T3_TTS:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_TTS);
        GFX_write_string(&FontT42,tXc1,text,0);
        if(pDecoinfo->output_time_to_surface_seconds)
        {
            if(pDecoinfo->output_time_to_surface_seconds < 1000 * 60)
                snprintf(text,TEXTSIZE,"\020\003\002%i'",(pDecoinfo->output_time_to_surface_seconds + 59)/ 60);
            else
                snprintf(text,TEXTSIZE,"\020\003\002%ih",(pDecoinfo->output_time_to_surface_seconds + 59)/ 3600);
            t3_basics_colorscheme_mod(text);
            GFX_write_string(&FontT105,tXc1,text,0);
        }
        break;

    case CVIEW_T3_ppO2andGas:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_ppO2);
        GFX_write_string(&FontT42,tXc1,text,0);
        snprintf(text,TEXTSIZE,"\020\003%01.2f",stateUsed->lifeData.ppO2);
        t3_basics_colorscheme_mod(text);
        GFX_write_string(&FontT105,tXc1,text,0);

        textpointer = 0;
        text[textpointer++] = '\020';
        text[textpointer++] = '\003';
        oxygen_percentage = 100;
        oxygen_percentage -= stateUsed->lifeData.actualGas.nitrogen_percentage;
        oxygen_percentage -= stateUsed->lifeData.actualGas.helium_percentage;
        text[textpointer++] = '\002';
        tHome_gas_writer(oxygen_percentage,stateUsed->lifeData.actualGas.helium_percentage,&text[textpointer]);
        //textpointer = snprintf(&text[textpointer],TEXTSIZE,"\020\002%02u/%02u",oxygen_percentage, stateUsed->lifeData.actualGas.helium_percentage);
        t3_basics_colorscheme_mod(text);
        GFX_write_string(&FontT48,tXc1,text,0);
        break;

    case CVIEW_T3_Navigation:
        Stopwatch.Total = timer_Stopwatch_GetTime();
        Stopwatch.Minutes = Stopwatch.Total / 60;
        Stopwatch.Seconds = Stopwatch.Total - ( Stopwatch.Minutes * 60 );
        fAverageDepth = timer_Stopwatch_GetAvarageDepth_Meter();

         if(!pSettings->FlipDisplay)
        {
        	tXc2->WindowX0 = 550;
        }
        else
        {
        	tXc2->WindowX1 = 800;
        	tXc2->WindowY0 = t3c2.WindowY0; /* select customer window */
        }

        snprintf(text,TEXTSIZE,"\032\002\f%c", TXT_Stopwatch);
        GFX_write_string(&FontT42,tXc1,text,0);
        snprintf(text,TEXTSIZE,"\030\002\016%01.1f",unit_depth_float(fAverageDepth));
        GFX_write_string(&FontT105,tXc1,text,0);
        if(!pSettings->FlipDisplay)
        {
        	tXc2->WindowY0 = 100;
        }
        else
        {
        	tXc2->WindowY1 -= 100; /* jump to upper of two lines */
        }

        snprintf(text,TEXTSIZE,"\030\002%u:\016\016%02u",Stopwatch.Minutes, Stopwatch.Seconds);
        GFX_write_string(&FontT105,tXc1,text,1);


        center.x = 400;
        center.y = 116;

        snprintf(text,TEXTSIZE,"\032\f%c%c",TXT_2BYTE, TXT2BYTE_Compass);
        GFX_write_string(&FontT42,tXc1,text,0);
        snprintf(text,100,"\030%03i`",heading);
        GFX_write_string(&FontT144,tXc1,text,0);
        t3_basics_compass(tXscreen, center, heading, stateUsed->diveSettings.compassHeading);

    	break;

    case CVIEW_T3_DepthData:
        snprintf(text,TEXTSIZE,"\032\f%c",TXT_MaxDepth);
        if(pSettings->FlipDisplay)
        {
        	if(mode == DIVEMODE_Apnea)
        	{
        		GFX_write_string(&FontT42,tXc2,text,0);
        	}
        	else
        	{
        		GFX_write_string(&FontT42,tXc1,text,0);
        	}
        }
        else
        {
        	GFX_write_string(&FontT42,tXc1,text,0);
        }
        snprintf(text,TEXTSIZE,"\020\003\016%01.1f",unit_depth_float(stateUsed->lifeData.max_depth_meter));
        t3_basics_colorscheme_mod(text);
        if(pSettings->FlipDisplay)
        {
        	if(mode == DIVEMODE_Apnea)
        	{
        		GFX_write_string(&FontT105,tXc2,text,0);
        	}
        	else
        	{
        		GFX_write_string(&FontT105,tXc1,text,0);
        	}
        }
        else
        {
        	GFX_write_string(&FontT105,tXc1,text,0);
        }
        fAverageDepthAbsolute = stateUsed->lifeData.average_depth_meter;
        snprintf(text,TEXTSIZE,"\032\002\f%c",TXT_AvgDepth);
        GFX_write_string(&FontT42,tXc1,text,0);

        snprintf(text,TEXTSIZE,"\020\003\002\016\%01.1f",unit_depth_float(fAverageDepthAbsolute));
        GFX_write_string(&FontT105,tXc1,text,0);
        break;
    }



    tXc1->WindowX0 = tempWinX0;
    tXc1->WindowY0 = tempWinY0;
    tXc1->WindowX1 = tempWinX1;
    tXc1->WindowY1 = tempWinY1;

    tXc2->WindowX0 = tempWinC2X0;
    tXc2->WindowY0 = tempWinC2Y0;
    tXc2->WindowX1 = tempWinC2X1;
    tXc2->WindowY1 = tempWinC2Y1;
    tXc2->WindowTab = tempWinC2Tab;
}


uint8_t t3_test_customview_warnings(void)
{
    uint8_t count = 0;

    count = 0;
    count += stateUsed->warnings.decoMissed;
    count += stateUsed->warnings.ppO2Low;
    count += stateUsed->warnings.ppO2High;
    //count += stateUsed->warnings.lowBattery;
    count += stateUsed->warnings.sensorLinkLost;
    count += stateUsed->warnings.fallback;

    return count;
}

//void t3_show_customview_warnings(GFX_DrawCfgScreen *tXscreen, GFX_DrawCfgWindow* tXl1, GFX_DrawCfgWindow* tXr1, uint8_t mode)
void t3_basics_show_customview_warnings(GFX_DrawCfgWindow* tXc1)
{
    char text[256], textMain[256];
    uint8_t textpointer, textpointerMain, lineFree, more;

    snprintf(text,TEXTSIZE,"\025\f%c",TXT_Warning);
    GFX_write_string(&FontT42,&t3c1,text,0);

    lineFree = 1;
    more = 0;

    textpointerMain = 0;
    textMain[textpointerMain++] = '\025';		/* red */
    textMain[textpointerMain++] = '\003';		/* doublesize */
    textMain[textpointerMain++] = TXT_2BYTE;	/* There is only one Main warning to be displayed */

    textpointer = 0;
    text[textpointer++] = '\025';				/* red */

    if(stateUsed->warnings.decoMissed)
    {
        if(lineFree)
        {
            textMain[textpointerMain++] = TXT2BYTE_WarnDecoMissed;
            lineFree--;
        }
        else
        {
        	text[textpointer++] = '\002';
            text[textpointer++] = TXT_2BYTE;
            text[textpointer++] = TXT2BYTE_WarnDecoMissed;
            text[textpointer++] = '\r';
            text[textpointer++] = '\n';
            more++;
        }
    }

    if(stateUsed->warnings.ppO2Low)
    {
        if(lineFree)
        {
            textMain[textpointerMain++] = TXT2BYTE_WarnPPO2Low;
            lineFree--;
        }
        else
        {
        	text[textpointer++] = '\002';
            text[textpointer++] = TXT_2BYTE;
            text[textpointer++] = TXT2BYTE_WarnPPO2Low;
            text[textpointer++] = '\r';
            text[textpointer++] = '\n';
            more++;
        }
    }

    if(stateUsed->warnings.ppO2High)
    {
        if(lineFree)
        {
            textMain[textpointerMain++] = TXT2BYTE_WarnPPO2High;
            lineFree--;
        }
        else
        {
        	text[textpointer++] = '\002';
            text[textpointer++] = TXT_2BYTE;
            text[textpointer++] = TXT2BYTE_WarnPPO2High;
            text[textpointer++] = '\r';
            text[textpointer++] = '\n';
            more++;
        }
    }

    if(stateUsed->warnings.fallback)
    {
        if(lineFree)
        {
            textMain[textpointerMain++] = TXT2BYTE_WarnFallback;
            lineFree--;
        }
        else
        {
        	text[textpointer++] = '\002';
            text[textpointer++] = TXT_2BYTE;
            text[textpointer++] = TXT2BYTE_WarnFallback;
            text[textpointer++] = '\r';
            text[textpointer++] = '\n';
            more++;
        }
    }

    if(stateUsed->warnings.sensorLinkLost)
    {
        if(lineFree)
        {
            textMain[textpointerMain++] = TXT2BYTE_WarnSensorLinkLost;
            lineFree--;
        }
        else
        {
        	text[textpointer++] = '\002';
            text[textpointer++] = TXT_2BYTE;
            text[textpointer++] = TXT2BYTE_WarnSensorLinkLost;
            text[textpointer++] = '\r';
            text[textpointer++] = '\n';
            more++;
        }
    }

    if(stateUsed->warnings.co2High)
    {
        if(lineFree)
        {
            textMain[textpointerMain++] = TXT2BYTE_WarnCO2High;
        }
        else
        {
        	text[textpointer++] = '\002';
            text[textpointer++] = TXT_2BYTE;
            text[textpointer++] = TXT2BYTE_WarnCO2High;
            text[textpointer++] = '\r';
            text[textpointer++] = '\n';
            more++;
        }
    }

    text[textpointer] = 0;
    textMain[textpointerMain] = 0;

    if(lineFree == 0)
    {
    	GFX_write_string(&FontT48,&t3c1,textMain,0);
    }
    if(more)
    {
        GFX_write_string(&FontT48,&t3c2,text,0);
    }
}

uint8_t t3_customview_disabled(uint8_t view)
{
	uint8_t i = 0;
	uint8_t cv_disabled = 0;
	const uint8_t *pcv_changelist;
    uint32_t cv_config = settingsGetPointer()->cv_config_BigScreen;

    pcv_changelist = cv_changelist_BS;

  	while(pcv_changelist[i] != CVIEW_T3_END)
    {
         if((view == pcv_changelist[i]) && !CHECK_BIT_THOME(cv_config, pcv_changelist[i]))
         {
        	 cv_disabled = 1;
               break;
         }
         i++;
    }

    if ((view == CVIEW_T3_sensors) &&
       	((stateUsed->diveSettings.ppo2sensors_deactivated == 0x07) || (stateUsed->diveSettings.ccrOption == 0) || stateUsed->warnings.fallback))
    {
      	cv_disabled = 1;
    }

    return cv_disabled;
}

uint8_t t3_change_customview(uint8_t action)
{

    t3_basics_change_customview(&t3_selection_customview, cv_changelist_BS, action);
    return t3_selection_customview;
}


void t3_basics_change_customview(uint8_t *tX_selection_customview,const uint8_t *tX_customviews, uint8_t action)
{
    uint8_t curViewIdx = 0xff;
    uint8_t index = 0;
    uint8_t indexOverrun = 0;
    uint8_t lastViewIdx = 0;
    uint8_t iterate = 0;									/* set to 1 if a view has to be skipped */
    uint8_t useFallback = 0;								/* is set if the current view is disabled */
    uint8_t fallbackSelection = CVIEW_noneOrDebug;			/* show "None" view per default */

    /* set pointer to currently selected view and count number of entries */
    while((tX_customviews[index] != CVIEW_T3_END))
    {
    	if (tX_customviews[index] == *tX_selection_customview)
    	{
    		curViewIdx = index;
    		break;
    	}
    	index++;
    }
    if(curViewIdx == 0xff)		/* called with unknown view */
    {
    	curViewIdx = 0;
    	*tX_selection_customview = CVIEW_noneOrDebug;		/* show "None" view per default */
    }
    lastViewIdx = index;
    index = curViewIdx;
    do
    {
    	iterate = 0;
    	switch(action)
    	{
    		case ACTION_BUTTON_ENTER:
    		case ACTION_PITCH_POS:

				if(tX_customviews[index] != CVIEW_T3_END)
				{
					index++;
				}
				if(tX_customviews[index] == CVIEW_T3_END)
				{
					index = 0;
					indexOverrun = 1;
				}
				break;
    		case ACTION_PITCH_NEG:
    			if(index == 0)
    			{
    				index = lastViewIdx - 1;
    				indexOverrun = 1;
    			}
    			else
    			{
    				index--;
    			}
    			break;
    		default:
    			break;
		}

		if((tX_customviews == cv_changelist_BS) && (t3_customview_disabled(tX_customviews[index])))
		{
			iterate = 1;
			if(*tX_selection_customview == tX_customviews[index])
			{
				useFallback = 1;		/* the provided view is disabled => use fallback */
			}
		}
		else	/* special case which are enabled but not to be displayed at the moment */
		{
			if(settingsGetPointer()->MotionDetection != MOTION_DETECT_SECTOR)		/* no hiding in case of active sector view option (fixed mapping would change during dive) */
			{
                const SDecoinfo * pDecoinfo = getDecoInfo();
				/* Skip TTS if value is 0 */
				if((tX_customviews[index] == CVIEW_T3_TTS) && (!pDecoinfo->output_time_to_surface_seconds))
				{
					if(*tX_selection_customview == tX_customviews[index])
					{
						useFallback = 1;		/* the provided view is disabled => use fallback */
					}
					iterate = 1;
					if(fallbackSelection == CVIEW_noneOrDebug)
					{
						fallbackSelection = CVIEW_T3_TTS;
					}
				}
				/* Skip Deco if NDL is not set */
				if((tX_customviews[index] == CVIEW_T3_Decostop) && ((!pDecoinfo->output_ndl_seconds) && (!pDecoinfo->output_time_to_surface_seconds) && (timer_Safetystop_GetCountDown() == 0)))
				{
					if(*tX_selection_customview == tX_customviews[index])
					{
						useFallback = 1;		/* the provided view is disabled => use fallback */
					}
					fallbackSelection = CVIEW_T3_Decostop;
					iterate = 1;
				}
			}
		}
	    if((iterate) && (action == ACTION_END))			/* ACTION_END is used to check the enable state of the provided view. If it is enable the function will return without change */
	    {
	    	action = ACTION_BUTTON_ENTER;
	    }
    }while ((iterate == 1) && (!((indexOverrun == 1) && (*tX_selection_customview == tX_customviews[index]))));		/* no other available view found => use fallback */

    if(*tX_selection_customview == tX_customviews[index])
	{
    	if(useFallback)
    	{
		   	*tX_selection_customview = fallbackSelection;		/* no active view found => keep actual view or change to fallback if actual view is deactivated */
    	}
	}
    else
    {
    	*tX_selection_customview = tX_customviews[index];
    }
}


void t3_basics_colorscheme_mod(char *text)
{
    if((text[0] == '\020') && !GFX_is_colorschemeDiveStandard())
    {
        text[0] = '\027';
    }
}


point_t t3_compass_circle(uint8_t id, uint16_t degree, point_t center)
{
    float fCos, fSin;
    const float piMult =  ((2 * 3.14159) / 360);
//	const int radius[4] = {95,105,115,60};
    const int radius[4] = {85,95,105,90};
    static point_t forcenter = {.x = 900, .y = 500};	/* used to identify change of circle position */
    static point_t r[4][360] = { 0 };

    if((r[0][0].y == 0) || (forcenter.x != center.x) || (forcenter.y != center.y))	/* calculate values only once during first call or if center position changed */
    {
        for(int i=0;i<360;i++)
        {
            fCos = cos(i * piMult);
            fSin = sin(i * piMult);
            for(int j=0;j<4;j++)
            {
                r[j][i].x = center.x + (int)(fSin * radius[j]);
                r[j][i].y = center.y + (int)(fCos * radius[j]);
            }
        }
        forcenter.x = center.x;
        forcenter.y = center.y;
    }
    if(id > 3) id = 0;
    if(degree > 359) degree = 0;
    return r[id][degree];
}


void t3_basics_compass(GFX_DrawCfgScreen *tXscreen, point_t center, uint16_t ActualHeading, uint16_t UserSetHeading)
{
	uint8_t loop = 0;
    uint16_t LineHeading;

    static int32_t LastHeading = 0;
    int32_t newHeading = 0;
    int32_t diff = 0;
    int32_t diff2 = 0;

    int32_t diffAbs = 0;
    int32_t diffAbs2 = 0;

    newHeading = ActualHeading;

    diff = newHeading - LastHeading;

    if(newHeading < LastHeading)
        diff2 = newHeading + 360 - LastHeading;
    else
        diff2 = newHeading - 360 - LastHeading;

    diffAbs = diff;
    if(diffAbs < 0)
        diffAbs *= -1;

    diffAbs2 = diff2;
    if(diffAbs2 < 0)
        diffAbs2 *= -1;

    if(diffAbs <= diffAbs2)
        newHeading = LastHeading + (diff / 2);
    else
        newHeading = LastHeading + (diff2 / 2);

    if(newHeading < 0)
        newHeading += 360;
    else
    if(newHeading >= 360)
        newHeading -= 360;

    LastHeading = newHeading;
    ActualHeading = newHeading;

    if (ActualHeading < 90)
        ActualHeading += 360;

    while(ActualHeading > 359) ActualHeading -= 360;

    LineHeading = 360 - ActualHeading;

    GFX_draw_thick_line(9,tXscreen, t3_compass_circle(0,LineHeading, center),  t3_compass_circle(2,LineHeading, center), CLUT_Font030); // North
    LineHeading += 90;

    for (loop = 0; loop < 3; loop++)
    {
    	if(LineHeading > 359) LineHeading -= 360;
		GFX_draw_thick_line(9,tXscreen, t3_compass_circle(0,LineHeading, center),  t3_compass_circle(2,LineHeading, center), CLUT_Font031); // Main Ticks
		LineHeading += 90;
    }

    LineHeading = 360 - ActualHeading;
    LineHeading += 45;

    for (loop = 0; loop < 4; loop++)
    {
		if(LineHeading > 359) LineHeading -= 360;
		GFX_draw_thick_line(5,tXscreen, t3_compass_circle(1,LineHeading, center),  t3_compass_circle(2,LineHeading, center), CLUT_Font031); // Subtick
		LineHeading += 90;
    }

    LineHeading = 360 - ActualHeading;
    LineHeading += 22;
    for (loop = 0; loop < 8; loop++)
    {
       if(LineHeading > 359) LineHeading -= 360;
       GFX_draw_thick_line(3,tXscreen, t3_compass_circle(1,LineHeading, center),  t3_compass_circle(2,LineHeading, center), CLUT_Font031); // Subtick
       LineHeading += 45;
    }
    if(UserSetHeading)
    {
        LineHeading = UserSetHeading + 360 - ActualHeading;
        if(LineHeading > 359) LineHeading -= 360;
        GFX_draw_thick_line(9,tXscreen, t3_compass_circle(3,LineHeading, center),  t3_compass_circle(2,LineHeading, center), CLUT_CompassUserHeadingTick);

        // Rï¿½ckpeilung, User Back Heading
        LineHeading = UserSetHeading + 360 + 180 - ActualHeading;
        if(LineHeading > 359) LineHeading -= 360;
        if(LineHeading > 359) LineHeading -= 360;
        GFX_draw_thick_line(9,tXscreen, t3_compass_circle(3,LineHeading, center),  t3_compass_circle(2,LineHeading, center), CLUT_CompassUserBackHeadingTick);
    }

    GFX_draw_circle(tXscreen, center, 106, CLUT_Font030);
    GFX_draw_circle(tXscreen, center, 107, CLUT_Font030);
    GFX_draw_circle(tXscreen, center, 108, CLUT_Font030);
}

uint8_t t3_GetEnabled_customviews()
{
	uint8_t *pViews;
	uint8_t increment = 1;
    uint8_t enabledViewCnt = 0;

    pViews = (uint8_t*)cv_changelist_BS;
    while((*pViews != CVIEW_T3_END))
    {
    	increment = 1;
    /* check if view is enabled */
    	if(t3_customview_disabled(*pViews))
    	{
    		increment = 0;
    	}
    	pViews++;
    	enabledViewCnt += increment;
    }
    return enabledViewCnt;
}

uint8_t t3_getCustomView(void)
{
    return t3_selection_customview;
}

int printScrubberText(char *text, size_t size, SSettings *settings)
{
    int16_t currentTimerMinutes = settings->scrubberData[settings->scubberActiveId].TimerCur;
    char colour = '\020';
    if (currentTimerMinutes <= 0) {
        colour = '\025';
    } else if (currentTimerMinutes <= 30) {
        colour = '\024';
    }

    if (settings->scrubTimerMode == SCRUB_TIMER_MINUTES || currentTimerMinutes < 0) {
        return snprintf(text, size, "%c%3i'", colour, currentTimerMinutes);
    } else {
        return snprintf(text, size, "%c%u\016\016%%\017", colour, currentTimerMinutes * 100 / settingsGetPointer()->scrubberData[settings->scubberActiveId].TimerMax);
    }
}

uint8_t t3_HandleAFGaslist()
{
	static uint8_t debounce = 0;
	static uint8_t lastState = AF_VIEW_NOCHANGE;
	uint8_t detectionState = AF_VIEW_NOCHANGE;

	if((stateUsed->warnings.betterGas) /* switch if better gas is available or depending on ppo2 if in OC mode */
			|| ((stateUsed->diveSettings.diveMode == DIVEMODE_OC) && ((stateUsed->warnings.ppO2Low) || (stateUsed->warnings.ppO2High))))
	{
		if(debounce < 10)
		{
			debounce++;
		}
		else
		{
			detectionState = AF_VIEW_ACTIVATED;
		}
	}
	else
	{
		if(debounce > 0)
		{
			debounce--;
		}
		else
		{
			detectionState = AF_VIEW_DEACTIVATED;
		}
	}
	if(detectionState)	/* no state change => return 0 */
	{
		if((detectionState == lastState))
		{
			detectionState = AF_VIEW_NOCHANGE;
		}
		else
		{
			lastState = detectionState;
		}
	}

	return detectionState;
}

void t3_handleAutofocus(void)
{
	static uint8_t returnView = CVIEW_T3_END;

	if(stateUsed->diveSettings.activeAFViews & (1 << CVIEW_T3_Navigation))
	{
		switch(HandleAFCompass())
		{
			case AF_VIEW_ACTIVATED:	returnView = t3_selection_customview;
									t3_select_customview(CVIEW_T3_Navigation);

				break;
			case AF_VIEW_DEACTIVATED: if((returnView != CVIEW_T3_END) && (t3_selection_customview == CVIEW_T3_Navigation))
										{
											t3_select_customview(returnView);
											returnView = CVIEW_T3_END;
										}
				break;
			default:
				break;
		}
	}
	if(stateUsed->diveSettings.activeAFViews & (1 << CVIEW_T3_GasList))
	{
		switch(t3_HandleAFGaslist())
		{
			case AF_VIEW_ACTIVATED:	returnView = t3_selection_customview;
									t3_select_customview(CVIEW_T3_GasList);

				break;
			case AF_VIEW_DEACTIVATED: if((returnView != CVIEW_T3_END) && (t3_selection_customview == CVIEW_T3_GasList))
										{
											t3_select_customview(returnView);
											returnView = CVIEW_T3_END;
										}
				break;
			default:
				break;
		}
	}
}
