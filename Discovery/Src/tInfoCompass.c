///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tInfoCompass.c
/// \brief  there is only compass_DX_f, compass_DY_f, compass_DZ_f output during this mode
/// \author heinrichs weikamp gmbh
/// \date   23-Feb-2015
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


#include "gfx_fonts.h"
#include "tHome.h"
#include "tInfo.h"
#include "tInfoCompass.h"

#include <string.h>
#include <math.h>

#define PI 3.14159265358979323846

#define PITCH 	(0)
#define ROLL	(1)
#define YAW 	(2)

/* Private variables ---------------------------------------------------------*/

static uint16_t tInfoCompassTimeout = 0;
static int16_t minMaxCompassDX[3][2] = { 0 };

static axisIndicator_t axis[3];
static uint8_t activeAxis = PITCH;
static uint32_t checkTick = 0;

/* Exported functions --------------------------------------------------------*/
void openInfo_Compass(void)
{
	uint16_t angle = 0;
	uint8_t axisIndex = 0;
    set_globalState(StICOMPASS);
    tInfoCompassTimeout = settingsGetPointer()->timeoutInfoCompass;
    tInfoCompassTimeout *= 10;

    for(axisIndex = 0; axisIndex < 3; axisIndex++)
    {
    	memset(axis[axisIndex].check,0, 360);
    }
    for(angle = 170; angle < 360; angle++)
    {
       	axis[PITCH].check[angle] = 1;	/* pitch only from -90 to +90 */
    }
    axis[YAW].coord.x = 100;
    axis[YAW].coord.y = 70;
    axis[PITCH].coord.x = 100;
    axis[PITCH].coord.y = 300;
    axis[ROLL].coord.x = 100;
    axis[ROLL].coord.y = 200;

    axis[YAW].eclipse.x = 50;
    axis[YAW].eclipse.y = 50;
    axis[PITCH].eclipse.x = 20;
    axis[PITCH].eclipse.y = 50;
    axis[ROLL].eclipse.x = 50;
    axis[ROLL].eclipse.y = 20;

    for(int i = 0; i<3;i ++)
    {
            minMaxCompassDX[i][0] = 999;
            minMaxCompassDX[i][1] = -999;
    }
    checkTick = HAL_GetTick();

    activeAxis = PITCH;
}

void getElipsePoint(uint16_t elipseX , int16_t elipseY, int16_t degree, int16_t *x, int16_t *y)
{
    double rad = degree * (PI / 180.0);
    *x = (int16_t) (elipseX * cos(rad));
    *y = (int16_t) (elipseY * sin(rad));
}
//  ===============================================================================
//	refreshInfo_Compass
/// @brief	there is only compass_DX_f, compass_DY_f, compass_DZ_f output during this mode
///					the accel is not called during this process
//  ===============================================================================
void refreshInfo_Compass(GFX_DrawCfgScreen s)
{
	static int16_t cursorAngle = 0;

	int16_t angle = 0.0;
	int16_t drawX = 0;
	int16_t drawY = 0;
	uint8_t color = 0;
	point_t start;
	point_t stop;
	point_t center;

	int8_t offset = 0;
	uint8_t axisIndex = 0;
	int16_t index = 0;
	uint8_t textIndex = 0;

	tHome_show_lost_connection_count(&s);
    tInfoCompassTimeout--;
    if(tInfoCompassTimeout == 0)
    {
        exitInfo();
        return;
    }

    char text[80];

    int16_t compassValues[3];

    compassValues[0] = stateUsed->lifeData.compass_DX_f;
    compassValues[1] = stateUsed->lifeData.compass_DY_f;
    compassValues[2] = stateUsed->lifeData.compass_DZ_f;

   /* draw indicator */
    for(axisIndex = 0; axisIndex < 3; axisIndex++)
    {
    	switch(axisIndex)
    	{
    		default:
    		case YAW:	index = (uint16_t)(stateUsed->lifeData.compass_heading);
    					textIndex = snprintf(text,80,"yaw %.1f", stateUsed->lifeData.compass_heading);

    					if((tInfoCompassTimeout < 50) && (activeAxis == YAW))
    					{
    						snprintf(&text[textIndex],80,"\023\t(%i, %i)", minMaxCompassDX[YAW][0], minMaxCompassDX[YAW][1]);
    					}

    					start.x = axis[axisIndex].coord.x - 1;
    		    	    start.y = axis[axisIndex].coord.y;
    		    	    stop.x = axis[axisIndex].coord.x + 1;
    		    	    stop.y = axis[axisIndex].coord.y;
    			break;
    		case PITCH:	index = (uint16_t)(stateUsed->lifeData.compass_pitch + 90);
    					textIndex = snprintf(text,80,"pitch %.1f", stateUsed->lifeData.compass_pitch);
    					if((tInfoCompassTimeout < 50) && (activeAxis == YAW))
    					{
    						snprintf(&text[textIndex],80,"\023\t(%i, %i)", minMaxCompassDX[PITCH][0], minMaxCompassDX[PITCH][1]);
    					}
    					start.x = axis[axisIndex].coord.x - 30;
    					start.y = axis[axisIndex].coord.y;
    					stop.x = axis[axisIndex].coord.x + 30;
    					stop.y = axis[axisIndex].coord.y;

    			break;
    		case ROLL: index = (uint16_t)(stateUsed->lifeData.compass_roll + 180);
    					textIndex = snprintf(text,80,"roll %.1f", stateUsed->lifeData.compass_roll);
    					if((tInfoCompassTimeout < 50) && (activeAxis == YAW))
    					{
    						snprintf(&text[textIndex],80,"\023\t(%i, %i)", minMaxCompassDX[ROLL][0], minMaxCompassDX[ROLL][1]);
    					}
    					start.x = axis[axisIndex].coord.x;
    					start.y = axis[axisIndex].coord.y - 30;
    					stop.x = axis[axisIndex].coord.x;
    					stop.y = axis[axisIndex].coord.y +30;
    			break;
    	}
		if(settingsGetPointer()->FlipDisplay)
		{
			start.x = 800 - start.x;
			stop.x = 800 - stop.x;
			start.y = 480 - start.y;
			stop.y = 480 - stop.y;
		}
    	if(activeAxis == axisIndex)			/* only check one axis at a time. reason: yaw will be unstable at the beginning of calibration */
    	{
    		for(offset = -6; offset <= 6; offset++)	/* it is hard to hit every single angle and the resolution is not needed */
    		{
				if(( (index + offset) >= 0) && (index + offset < 360))
				{
					axis[axisIndex].check[index + offset] = 1;							/* => check surrounding angles as well */
				}
    		}
    	}
    	if(axisIndex == activeAxis)
    	{
    		color = CLUT_InfoCompass;
    		getElipsePoint(axis[axisIndex].eclipse.x,axis[axisIndex].eclipse.y,cursorAngle, &drawX, &drawY);
    		center.x = axis[axisIndex].coord.x + drawX;
    		center.y = axis[axisIndex].coord.y + drawY;

    		t_Info_draw_circle(center, 4, CLUT_Font020);
    		cursorAngle += 15;
    		if(cursorAngle >= 360)
    		{
    			cursorAngle = 0;
    		}
    	}
    	else
    	{
    		color = CLUT_Font021;
    	}
    	tInfo_write_content_simple(  200, 600, 480 - axis[axisIndex].coord.y - 35 , &FontT42, text, color);
  	    tInfo_draw_colorline(start, stop, CLUT_Font020);
  	    for(angle = 0; angle < 360; angle = angle + 3)
  	    {
  	    	if(axis[axisIndex].check[angle])
  	    	{
  	    		color = CLUT_NiceGreen;
  	    	}
  	    	else
  	    	{
  	    		color = CLUT_WarningRed;
  	    	}
  	    	getElipsePoint(axis[axisIndex].eclipse.x,axis[axisIndex].eclipse.y,angle, &drawX, &drawY);
  	    	tInfo_drawPixel(axis[axisIndex].coord.x + drawX, axis[axisIndex].coord.y + drawY,color);
  	    	if((axisIndex == PITCH) && (angle == 180))		/* pitch only from -90 to +90 */
  	    	{
  	    		break;
  	    	}
  	    }

    }

    for(int i = 0; i<3;i ++)
    {
        // do not accept zero
        if(minMaxCompassDX[i][0] == 0)
            minMaxCompassDX[i][0] = compassValues[i];

        // do not accept zero
        if(minMaxCompassDX[i][1] == 0)
            minMaxCompassDX[i][1] = compassValues[i];

        if(compassValues[i] < minMaxCompassDX[i][0])
            minMaxCompassDX[i][0] = compassValues[i];

        if(compassValues[i] > minMaxCompassDX[i][1])
            minMaxCompassDX[i][1] = compassValues[i];
    }

    snprintf(text,80,"Time left: %u s",(tInfoCompassTimeout+9)/10);
    tInfo_write_content_simple(  20,800,  25, &FontT42, text, CLUT_InfoCompass);


    if(time_elapsed_ms(checkTick, HAL_GetTick() > 1000))
    {
    	index = 0;
    	 for(angle = 0; angle < 360; angle++)
    	 {
    		 if(axis[activeAxis].check[angle])
    		 {
    			 index++;
    		 }
    	 }
    	 if(index > 350)
    	 {
    		 if(activeAxis < 2)
    		 {
    			 activeAxis++;
    		 }
    		 else
    		 {
    			 if(tInfoCompassTimeout > 50)
    			 {
    				 tInfoCompassTimeout = 50;	/* reduce exit time to five seconds */
    			 }
    		 }
    	 }


    	checkTick = HAL_GetTick();
    }
}
