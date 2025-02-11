///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/show_logbook.c
/// \brief  show_logbook_logbook_show_log_page1 /
/// \author Heinrichs Weikamp gmbh
/// \date   07-July-2014
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

#include "base.h"
#include "logbook.h"
#include "gfx_colors.h"
#include "gfx_engine.h"
#include "gfx_fonts.h"
#include "show_logbook.h"
#include "unit.h"
#include "configuration.h"
#include "logbook_miniLive.h"
#include "text_multilanguage.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // for abs()

#define LOG_BORDER_OFFSET	(50u)		/* text offset from left / right display to text start/end */

/* Private variables ---------------------------------------------------------*/

static GFX_DrawCfgScreen	tLOGscreen;
static GFX_DrawCfgScreen	tLOGbackground;


static void print_gas_name(char* output,uint8_t lengh,uint8_t oxygen,uint8_t helium);
static int16_t get_colour(int16_t color);

static uint8_t active_log_page = 1;
static uint8_t active_log_offset = 0;

/* Overview */
static void show_logbook_logbook_show_log_page1(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards);
/* Temperature */
static void show_logbook_logbook_show_log_page2(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards);
/* Gas List */
static void show_logbook_logbook_show_log_page3(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards);
/* ppO2 */
static void show_logbook_logbook_show_log_page4(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards);

static inline uint32_t MaxU32LOG(uint32_t a, uint32_t b)
{
    return((a>b)?a:b);
}

/**
  ******************************************************************************
  * @brief   GFX write label. /  print coordinate system & depth graph
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    07-July-2014
  ******************************************************************************
    *
  * @param  hgfx:
  * @param  window: 			WindowGimpStyle
  * @param  mode: 			different modes depending witch page uses the function
  * @param  dataLength:
  * @param  depthdata:
  * @param  colordata: 			1
  * @retval None
  */
static void show_logbook_draw_depth_graph(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards, SWindowGimpStyle* window, short mode, uint16_t dataLength, uint16_t* depthdata, uint8_t * colordata, uint16_t * decostopdata)
{
    SLogbookHeader logbookHeader;
    SWindowGimpStyle wintemp = *window;
    SWindowGimpStyle winsmal;
    logbook_getHeader(StepBackwards, &logbookHeader);
    int divetime = logbookHeader.diveTimeMinutes;
    int maxDepth =  logbookHeader.maxDepth/100;

    int16_t saveBottom = wintemp.bottom;
    int16_t saveTop = 0 - wintemp.top;

    //*** Horisontal (depth) ***************************************************

    //--- calc depth lines and labels --
    int vscale = 0;
    int vstep = 0;

    vstep = maxDepth / 5;
    vscale = vstep * 5;
    if(vscale < maxDepth)
    {
        vstep += 1;
        vscale += 5;
    }
/*
    if(vscale <
    for(int i=1; i <= 20; i++)
    {
        vscale = i * 25;
        vstep = i * 5;
        if( vscale > maxDepth)
            break;
    }
*/
    //--- print depth labels ---
    winsmal.left = wintemp.left - 48;
    winsmal.top	= wintemp.top  - 3;
    winsmal.right = wintemp.left -1;
    winsmal.bottom = winsmal.top + 16;

    if(settingsGetPointer()->nonMetricalSystem)
    {
    	Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_GasSensor1,"[ft]");
    }
    else
    {
    	Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_GasSensor1,"[m]");
    }

 //   winsmal.left = wintemp.left - 48;
    char msg[10];
    float deltaline = ((float)(wintemp.bottom - wintemp.top))/5;
    for(int i = 1; i<=5; i++)
    {
        winsmal.top	= wintemp.top + deltaline * i - 14;
        winsmal.bottom = winsmal.top + 16;

    //    winsmal.right = wintemp.left - 2;
        snprintf(msg,10,"%i",unit_depth_integer(i * vstep));
        Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_GasSensor1,msg);
    }

    //vertical (Time) *******************************************************************
    //--- calc time lines and labels --
    int timestep = 0;
    int lines = 0;
    for(int i=1; i <= 60; i++)
    {
        timestep = i * 5;
        lines = divetime/timestep;
        if(lines < 7)
        {
            break;
        }
    }
    //*** print coordinate system grit ***
    int winwidth = wintemp.right - wintemp.left;
    float vdeltaline = ((float)(winwidth * timestep))/divetime;
    GFX_draw_Grid( &tLOGbackground,wintemp,  0, vdeltaline,  5,0,  CLUT_LogbookGrid);


    //--- print time labels ---
    winsmal.left = wintemp.left;
    winsmal.top	=  wintemp.top - 40;
    winsmal.right = winsmal.left + 60;
    winsmal.bottom = winsmal.top + 16;

    Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_GasSensor1,"min");
    for(int i = 1; i<=lines; i++)
    {
        winsmal.left= wintemp.left + vdeltaline * i - 15;
        winsmal.right = winsmal.left + 30;
        snprintf(msg,10,"%3i",i * timestep);
        Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_GasSensor1,msg);
    }
    winsmal.left = wintemp.left;// - 9;
    winsmal.top	=  wintemp.top - 40;
    winsmal.right = winsmal.left + 60;

    //--- print depth graph ---
    //adapt window
    int winhight = wintemp.bottom - wintemp.top;
    int newhight = (winhight * maxDepth)/vscale;
    wintemp.bottom = wintemp.top + newhight;
    //wintemp.fontcolor = LOGBOOK_GRAPH_DEPTH;

    int datamax = 0;
    for(int i=0;i<dataLength;i++)
    {
        if(depthdata[i]>datamax)
            datamax = depthdata[i];
    }

    if(decostopdata)
    {
        if(dataLength <= 1000)
        {
            uint8_t colortemp[1000];

            for(int i = 0; i<dataLength; i++)
            {
                if(decostopdata[i] > depthdata[i])
                {
                    colortemp[i] = CLUT_WarningRed;
                }
                else
                {
                    colortemp[i] = CLUT_NiceGreen;
                }
            }
            GFX_graph_print(hgfx,&wintemp,saveTop,1,0,datamax, decostopdata,dataLength, 0, colortemp);
        }
        else
            GFX_graph_print(hgfx,&wintemp,saveTop,1,0,datamax, decostopdata,dataLength, CLUT_NiceGreen, NULL);
    }

    if(settingsGetPointer()->FlipDisplay)
    {
		winsmal.right = 800 - wintemp.left;
		winsmal.left = 800 - wintemp.right;
		winsmal.bottom = wintemp.bottom;
		winsmal.top = wintemp.top;
    }
    else
    {
		winsmal.right = wintemp.right;
		winsmal.left = wintemp.left;
		winsmal.bottom = wintemp.bottom;
		winsmal.top = wintemp.top;
    }

    switch(mode)
    {
    case 0:
        GFX_graph_print(hgfx,&winsmal,0,1,0,datamax, depthdata,dataLength,CLUT_GasSensor1, NULL);
        break;
    case 1:
        GFX_graph_print(hgfx,&winsmal,saveBottom,1,0,datamax, depthdata,dataLength,CLUT_GasSensor0,colordata);
        break;
    case 2:
        if(*colordata)
            GFX_graph_print(hgfx,&winsmal,0,1,0,datamax, depthdata,dataLength,CLUT_GasSensor0,colordata);
        else
            GFX_graph_print(hgfx,&winsmal,0,1,0,datamax, depthdata,dataLength,CLUT_GasSensor1, NULL);
    }
}



/**
  ******************************************************************************
  * @brief  scaleAdapt
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    29-Nov-2016
  ******************************************************************************
    *
  * @param  ...
  * @retval *OutputStepOfScale, *OutputMaxValueOnScale, *OutputTop, *OutputBottom

    * fit to multiples of 1ï¿½C (data format is 1/10ï¿½C)
    */

static void scaleAdapt(	int InputTop, int InputBottom,
                                    int16_t *OutputMinValue, int16_t *OutputMaxValue, int *OutputTop, int *OutputBottom,
                                    uint16_t *OutputStepOfScale, int16_t *OutputMaxValueOnScale)
{
//	uint16_t oldScale;
    uint16_t newScale;
//	uint16_t diff_newScale;

//	int16_t oldMaxOnScale;
    int16_t newMaxOnScale;
//	int16_t diff_newMaxOnScale;
    _Bool negativeMaxValue = 0;

//	float oldRange;
    float newRange;

    float	sizeOfScreen;
//	float InputTopValue;
//	float InputBottomValue;
    float screenToRangeRatio;
    float diffOutMaxToMaxOnScale;
    float diffOutMinToMaxOnScale;
    int positonOutputMaxValue;
    int positonOutputMinValue;


// scale
//	oldScale = *OutputStepOfScale;
    newScale = *OutputStepOfScale + 9;
    newScale /= 10;
    newScale *= 10;
//	diff_newScale = newScale - *OutputStepOfScale;
//	oldRange = 5 * oldScale;
    newRange = 5 * newScale;
    *OutputStepOfScale = newScale;

    // MaxValueOnScale
//	oldMaxOnScale = *OutputMaxValueOnScale;
    if(OutputMaxValueOnScale < 0)
    {
        negativeMaxValue = 1;
        newMaxOnScale = 0 - *OutputMaxValueOnScale;
    }
    else
    {
        negativeMaxValue = 0;
        newMaxOnScale = *OutputMaxValueOnScale;
    }
    newMaxOnScale += 9;
    newMaxOnScale /= 10;
    newMaxOnScale *= 10;
    if(negativeMaxValue)
    {
//		diff_newMaxOnScale = newMaxOnScale + *OutputMaxValueOnScale;
        *OutputMaxValueOnScale = 0 - newMaxOnScale;
    }
    else
    {
//		diff_newMaxOnScale = newMaxOnScale - *OutputMaxValueOnScale;
        *OutputMaxValueOnScale = newMaxOnScale;
    }


    // new coordinates
    sizeOfScreen = 1 + InputBottom - InputTop;
//	InputTopValue = *OutputMaxValueOnScale;
//	InputBottomValue = InputTopValue + (6 * *OutputStepOfScale);

    screenToRangeRatio = sizeOfScreen / newRange;
    diffOutMaxToMaxOnScale = abs(*OutputMaxValueOnScale) - abs(*OutputMaxValue);
//	diffOutMinToMax = abs(*OutputMinValue - *OutputMaxValue);
    diffOutMinToMaxOnScale = abs(*OutputMaxValueOnScale - *OutputMinValue);

    positonOutputMaxValue = (int)(diffOutMaxToMaxOnScale * screenToRangeRatio);
    positonOutputMaxValue += *OutputTop;
    positonOutputMinValue = (int)(diffOutMinToMaxOnScale * screenToRangeRatio);
    positonOutputMinValue += *OutputTop;
//	positonOutputMinValue = (int)(diffOutMinToMax * screenToRangeRatio);
//	positonOutputMinValue += positonOutputMaxValue;
    *OutputTop = positonOutputMaxValue;
    *OutputBottom = positonOutputMinValue;
}


/**
  ******************************************************************************
  * @brief  scaleHelper
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    13-Oct-2016
  ******************************************************************************
    *
  * @param  hgfx:
  * @retval None

    * pixel  50 oben
    * pixel 439 unten
    * pixel 390 gesamt hï¿½he

    * for temperature, input is ï¿½C * 10
    */

static void scaleHelper(	uint16_t InputDataLength, int16_t *InputDataArray, int InputTop, int InputBottom,
                                    int16_t *OutputMinValue, int16_t *OutputMaxValue, int *OutputTop, int *OutputBottom,
                                    uint16_t *OutputStepOfScale, int16_t *OutputMaxValueOnScale)
{
    int32_t 	datamin = INT16_MAX; // 32 bit for delta calculation ( delta is unsigned -> value can be 2x INT16_MAX)
    int32_t 	datamax = INT16_MIN;
    uint16_t	deltaMinMax = 1;
//	uint16_t	deltaMinMaxUsed = 1;
//	uint16_t 	digits = 1;
//	uint16_t 	scaler = 1;
    uint32_t	step = 1;
    const int	sizeOfScreen = InputBottom - InputTop;
    float pixel2range = 1.0;

    // min, max, deltaMinMax, OutputMinValue, OutputMaxValue
    for(uint16_t i = 0; i < InputDataLength; i++)
    {
        if(InputDataArray[i] > datamax)
            datamax = InputDataArray[i];

        if(InputDataArray[i] < datamin)
            datamin = InputDataArray[i];
    }

    deltaMinMax = (uint16_t)(datamax - datamin);

    *OutputMinValue = (int16_t)datamin;
    *OutputMaxValue = (int16_t)datamax;

    // step
    step = deltaMinMax / 5;
    while(deltaMinMax > (step * 5))
    {
        step += 1;
    }
    pixel2range = ((float)sizeOfScreen) / (step * 5);

    *OutputStepOfScale = (uint16_t)step;
    *OutputMaxValueOnScale = *OutputMaxValue;
    *OutputTop = InputTop;
    *OutputBottom = ((int)(pixel2range * deltaMinMax)) + *OutputTop;
}

/**
  ******************************************************************************
  * @brief  show_logbook_logbook_show_log_page1 /
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    07-July-2014
  ******************************************************************************
    *
  * @param  hgfx:
  * @retval None
  */
static void show_logbook_logbook_show_log_page1(GFX_DrawCfgScreen *hgfx,uint8_t StepBackwards)
{
    SWindowGimpStyle wintemp;
    SWindowGimpStyle winsmal;
    wintemp.left = LOG_BORDER_OFFSET;
    wintemp.right = 799 - wintemp.left;
    wintemp.top = LOG_BORDER_OFFSET;
    wintemp.bottom = 479 - 40;
    char timeSuffix;
    uint8_t hoursToDisplay;

    SLogbookHeader logbookHeader;
    logbook_getHeader(StepBackwards ,&logbookHeader);

    uint16_t depthdata[1000] = { 0 };
    uint8_t  gasdata[1000] = { 0 };
    int16_t tempdata[1000] = { 0 };
    uint16_t tankdata[1000] = { 0 };

    SGnssCoord posCoord;
    posCoord.fLat = 0.0;
    posCoord.fLon = 0.0;

#ifdef ENABLE_BOTTLE_SENSOR
    uint16_t bottlePressureStart = 0;
    uint16_t bottlePressureEnd = 0;
    uint16_t loop = 0;
#endif

    uint16_t dataLength = 0;
    dataLength = logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, tempdata, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tankdata, &posCoord, NULL);

    //Print Date
    uint8_t year = logbookHeader.dateYear;
    uint8_t month = logbookHeader.dateMonth;
    uint8_t day =  logbookHeader.dateDay;
    char text[40];
    snprintf(text, 20, "20%02i-%02i-%02i", year, month, day);

    Gfx_write_label_var(hgfx, 30, 250,10, &FontT42,CLUT_GasSensor1,text);

#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
    if((posCoord.fLat != 0.0) || (posCoord.fLon != 0.0))
    {
    	snprintf(text, 20, "%2.4f - %2.4f", posCoord.fLat, posCoord.fLon );
    	Gfx_write_label_var(hgfx, 300, 500,10, &FontT42,CLUT_GasSensor1,text);
    }
#endif


    // Print logbook number with offset
    if(settingsGetPointer()->logbookOffset)
    {
        int32_t logNumber;
        logNumber = settingsGetPointer()->logbookOffset - StepBackwards;
        if(logNumber < 0)
            logNumber = 0;
        else
        if(logNumber > 9999)
            logNumber = 9999;

        snprintf(text,20,"#%ld",logNumber);
        Gfx_write_label_var(hgfx, 300, 590,10, &FontT42,CLUT_GasSensor1,text);
    }

    //Print time
    uint8_t minute = logbookHeader.timeMinute;

    if (settingsGetPointer()->amPMTime)
    {
    	if (logbookHeader.timeHour > 11)
    	{
    		timeSuffix = 'P';
    	}
    	else
    	{
    		timeSuffix = 'A';
    	}

    	if (logbookHeader.timeHour % 12 == 0)
    	{
    		hoursToDisplay = 12;
    	}
    	else
    	{
    		hoursToDisplay = (logbookHeader.timeHour % 12);
    	}

    	snprintf(text,20,"\002%02i:%02i %cM",hoursToDisplay,minute,timeSuffix);
    }
    else
    {
    	hoursToDisplay = logbookHeader.timeHour;
    	snprintf(text,20,"\002%02i:%02i",hoursToDisplay,minute);
    }


    Gfx_write_label_var(hgfx, 600, wintemp.right,10, &FontT42,CLUT_GasSensor1,text);

    //Print Dive Mode (OC/CCR/...)
    switch(logbookHeader.diveMode)
    {
    case DIVEMODE_OC:
            snprintf(text,20,"%c",TXT_OpenCircuit);
            break;
    case DIVEMODE_CCR:
            snprintf(text,20,"%c",TXT_ClosedCircuit);
            break;
    case DIVEMODE_Gauge:
            snprintf(text,20,"%c",TXT_Gauge);
            break;
    case DIVEMODE_Apnea:
            snprintf(text,20,"%c",TXT_Apnoe);
            break;
    case DIVEMODE_PSCR:
            snprintf(text,20,"%c",TXT_PSClosedCircuit);
            break;
    }
    Gfx_write_label_var(hgfx, 30, 250,60, &FontT42,CLUT_GasSensor4,text);

    // Decomodel
    if(logbookHeader.diveMode <= DIVEMODE_CCR)
    {
        switch(logbookHeader.decoModel)
        {
        case GF_MODE:
                snprintf(text,20,"\002GF%u/%u",logbookHeader.gfLow_or_Vpm_conservatism,logbookHeader.gfHigh);
            break;
        case VPM_MODE:
                snprintf(text,20,"\002VPM +%u",logbookHeader.gfLow_or_Vpm_conservatism);
            break;
        default:
        		snprintf(text,20," ");		/* no information to be displayed */
        	break;
        }
        Gfx_write_label_var(hgfx, 500, wintemp.right,60, &FontT42,CLUT_GasSensor1,text);
    }

    //Write Dive Time
    int minutes = logbookHeader.diveTimeMinutes;
    int seconds = logbookHeader.diveTimeSeconds;
    int hours = minutes/60;
    minutes -= hours * 60;
    snprintf(text,20,"%02i:%02i:%02i",hours,minutes,seconds);
    Gfx_write_label_var(hgfx, 30, 250,360, &FontT42,CLUT_GasSensor1,text);
    Gfx_write_label_var(hgfx, 200, 250,360, &FontT42,CLUT_GasSensor4,"s");

    // Max Depth
    int maxdepth =logbookHeader.maxDepth/100;
    int maxdepth_dcm = logbookHeader.maxDepth/10 - maxdepth * 10;
    int top = 150;
    if(settingsGetPointer()->nonMetricalSystem)
    {
        float maxDepthFeet = 0;
        maxDepthFeet = unit_depth_float(((float)logbookHeader.maxDepth)/100);
        snprintf(text,20,"%.0f",maxDepthFeet);
    }
    else
    {
        snprintf(text,20,"%i.%i",maxdepth,maxdepth_dcm);
    }
    Gfx_write_label_var(hgfx, 30, 250,top, &FontT42,CLUT_GasSensor1,text);
    winsmal.left = 30;
    winsmal.top = top -3;
    winsmal.bottom = winsmal.top + FontT42.height;

    if(maxdepth < 10)
    {
        winsmal.left  = 137;
    }
    else if(maxdepth < 100)
    {
        winsmal.left  = 151;
    }
    else
    {
        winsmal.left  = 147;
    }
    winsmal.right = winsmal.left + 50;

    Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,top, &FontT24,CLUT_GasSensor4,"max");
    snprintf(text,3,"%c%c"
        , unit_depth_char1()
        , unit_depth_char2()
    );
    Gfx_write_label_var(hgfx, winsmal.left - 37, 250,top, &FontT42,CLUT_GasSensor4,text);

    // Average Depth
    int avrdepth =logbookHeader.averageDepth_mbar/100;
    int avrdepth_dcm = logbookHeader.averageDepth_mbar/10 - avrdepth * 10;
    top = 200;
    if(settingsGetPointer()->nonMetricalSystem)
    {
        float avgDepthFeet = 0;
        avgDepthFeet = unit_depth_float(((float)logbookHeader.averageDepth_mbar)/100);
        snprintf(text,20,"%.0f",avgDepthFeet);
    }
    else
    {
        snprintf(text,20,"%i.%i",avrdepth,avrdepth_dcm);
    }
    Gfx_write_label_var(hgfx, 30, 250,top, &FontT42,CLUT_GasSensor1,text);

    winsmal.left = 30;
    winsmal.top = top -3;
    winsmal.bottom = winsmal.top + FontT42.height;

/* put avg behind previous string */
    if(avrdepth < 10)
    {
        winsmal.left  = 137                               ;
    }
    else if(avrdepth < 100)
    {
        winsmal.left  = 151;
    }
    else
    {
        winsmal.left  = 147;
    }
    winsmal.right = winsmal.left + 50;

    Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_GasSensor4,"avg");
    snprintf(text,3,"%c%c"
        , unit_depth_char1()
        , unit_depth_char2()
    );
    Gfx_write_label_var(hgfx, winsmal.left - 37, 250,top, &FontT42,CLUT_GasSensor4,text);
    // Temperature
    top+= 50;
    float temp_Temperature;
    uint16_t start;
    temp_Temperature = ((float)logbookHeader.minTemp)/10;
    snprintf(text,20,"%.1f",unit_temperature_float(temp_Temperature));
    Gfx_write_label_var(hgfx, 30, 250,top, &FontT42,CLUT_GasSensor1,text);

    if(settingsGetPointer()->nonMetricalSystem)
        start = 121;
    else if((logbookHeader.minTemp >= 0) && (logbookHeader.minTemp < 10))
        start = 100;
    else if((logbookHeader.minTemp >= -10) && (logbookHeader.minTemp < 100))
        start = 114;
    else
        start = 121;

    text[0] = '\140';
    if(settingsGetPointer()->nonMetricalSystem)
        text[1] = 'F';
    else
        text[1] = 'C';
    text[2] = 0;

    Gfx_write_label_var(hgfx, start, 300,top, &FontT42,CLUT_GasSensor4,text);

    // CNS
    snprintf(text,20,"CNS: %i %%",logbookHeader.maxCNS);
    Gfx_write_label_var(hgfx, 30, 250,440, &FontT42,CLUT_GasSensor1,text);

// Surface Pressure
//		snprintf(text,20,"\001%i\016\016 mbar",logbookHeader.surfacePressure_mbar);
//		Gfx_write_label_var(hgfx,300,500,750, &FontT42,CLUT_GasSensor1,text);
//		snprintf(text,40,"%i\016\016 mbar\017  (%i\016\016 m\017)",logbookHeader.surfacePressure_mbar, unit_SeaLevelRelation_integer(logbookHeader.surfacePressure_mbar));
    snprintf(text,40,"%i\016\016 hPa\017",logbookHeader.surfacePressure_mbar);
    Gfx_write_label_var(hgfx,320,600,440, &FontT42,CLUT_GasSensor1,text);


/* show symbol in case log entry is marked for usage in profile custom view */
	snprintf(text,10,"\002>");
	if(StepBackwards == getReplayOffset())
	{
		 Gfx_write_label_var(hgfx,750,799,440, &FontT42,CLUT_GasSensor1,text);
	}
	else
	{
		 Gfx_write_label_var(hgfx,750,799,440, &FontT42,CLUT_MenuTopBackground,text);
    }


/* Show tank info */
#ifdef ENABLE_BOTTLE_SENSOR
    for(loop = 0; loop < dataLength; loop++)
    {
    	if((bottlePressureStart == 0) && (tankdata[loop] != 0))	/* find first pressure value */
    	{
    		bottlePressureStart = tankdata[loop];
    	}
    	if((tankdata[loop] != 0))								/* store last pressure value */
    	{
    		bottlePressureEnd = tankdata[loop];
    	}
    }
    if(bottlePressureStart != 0)
    {
    	snprintf(text,40,"\002%i | %i\016\016 Bar\017",bottlePressureStart,bottlePressureEnd);
        Gfx_write_label_var(hgfx,450,wintemp.right,440, &FontT42,CLUT_GasSensor1,text);
    }
#endif
    //--- print coordinate system & depth graph with gaschanges ---
    wintemp.left 	= 330;
    wintemp.top	=  160;
    wintemp.bottom -= 40;

    show_logbook_draw_depth_graph(hgfx, StepBackwards, &wintemp, 1, dataLength, depthdata, gasdata, NULL);
}


static void show_logbook_logbook_show_log_page2(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards)
{
    //*** Page2: Depth and Temperature ****

    SWindowGimpStyle wintemp;
    SWindowGimpStyle winsmal;
    wintemp.left = 50;
    wintemp.right = 799 - wintemp.left;
    wintemp.top = 50;
    wintemp.bottom = 479 - 40;

    SLogbookHeader logbookHeader;

    logbook_getHeader(StepBackwards,&logbookHeader);
    uint16_t dataLength = 0;
    uint16_t depthdata[1000];
    uint8_t  gasdata[1000];
    int16_t  tempdata[1000];
    uint16_t decoDepthdata[1000];
    uint16_t *pDecoDepthData = 0;

    dataLength = logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, tempdata, NULL, NULL, NULL, NULL, NULL, NULL, NULL, decoDepthdata, NULL, NULL, NULL);

        for(int i = 0; i<dataLength; i++)
        {
            if(decoDepthdata[i] >= 300)
            {
                pDecoDepthData = decoDepthdata;
                break;
            }
        }
    //--- print coordinate system & depth graph ---
    show_logbook_draw_depth_graph(hgfx, StepBackwards, &wintemp, 0, dataLength, depthdata, gasdata, pDecoDepthData);

    //*** Temperature *************************************************

    //--- print temperature labels ---
    // input maxtmpline, tmpstep, deltaline

    winsmal.left = wintemp.right +6;
    winsmal.top	= wintemp.top - 3;
    winsmal.right =  wintemp.right + 30;
    winsmal.bottom = winsmal.top + 16;

    if(settingsGetPointer()->nonMetricalSystem)
    {
    	Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_LogbookTemperature,"[F]");
    }
    else
    {
    	Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_LogbookTemperature,"[C]");
    }

    int16_t minVal = 0;
    int16_t maxVal = 0;
    int newTop = 0;
    int newBottom = 0;
    uint16_t step = 0;
    int16_t maxValTop = 0;
    int16_t tmp = 0;

    scaleHelper(dataLength, tempdata, wintemp.top, wintemp.bottom,
                            &minVal, &maxVal, &newTop, &newBottom,
                            &step, &maxValTop); // newTop is wintemp.top

    scaleAdapt(	wintemp.top, wintemp.bottom,
                            &minVal, &maxVal, &newTop, &newBottom,
                            &step, &maxValTop);

    // temperature in 1/10 ï¿½C
    int deltaline = (1 + wintemp.bottom - wintemp.top)/5;
    char msg[15];

    /* temperature is provided in centi scaling => convert */
    maxValTop /= 10;
    step /= 10;

    tmp = maxValTop;
    for(int i = 1; i<=5; i++)
    {
        tmp -= 	step;
        winsmal.top	= wintemp.top + deltaline * i - 14;
        winsmal.bottom = winsmal.top + 16;
        snprintf(msg,15,"%2i",unit_temperature_integer(tmp));
        Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,CLUT_LogbookTemperature,msg);
    }


    //--- print temperature graph ---
    // input tempdata[i], maxtmpline, mintmpline, maxTmp, minTmp, deltaline, wintemp.top, dataLength, datamax,

    //adapt window
    wintemp.bottom = newBottom;
    wintemp.top = newTop;
    GFX_graph_print(hgfx,&wintemp,0,1,maxVal,minVal, (uint16_t *)tempdata,dataLength,CLUT_LogbookTemperature, NULL);
}


static void build_logbook_test(uint8_t page, uint8_t StepBackwards)
{
    uint32_t lastScreen,lastBackground;

    lastScreen = tLOGscreen.FBStartAdress;
    lastBackground = tLOGbackground.FBStartAdress;

    tLOGscreen.FBStartAdress = getFrame(16);
    tLOGscreen.ImageHeight = 480;
    tLOGscreen.ImageWidth = 800;
    tLOGscreen.LayerIndex = 1;

    tLOGbackground.FBStartAdress = getFrame(17);
    tLOGbackground.ImageHeight = 480;
    tLOGbackground.ImageWidth = 800;
    tLOGbackground.LayerIndex = 0;
    switch(page)
    {
    case 1:
        show_logbook_logbook_show_log_page1(&tLOGscreen,StepBackwards);
        break;
    case 2:
        show_logbook_logbook_show_log_page2(&tLOGscreen,StepBackwards);
        break;
    case 3:
        show_logbook_logbook_show_log_page3(&tLOGscreen,StepBackwards);
        break;
    case 4:
        show_logbook_logbook_show_log_page4(&tLOGscreen,StepBackwards);
        break;
    }

    releaseFrame(16,lastScreen);
    releaseFrame(17,lastBackground);
}


void show_logbook_test(_Bool firstPage, uint8_t StepBackwards)
{
    if(firstPage)
    {
    	active_log_page = 1;
    	active_log_offset = StepBackwards;
    }
    else
    {
    	active_log_page++;
        if(active_log_page > 4)
        {
           	active_log_page = 1;
        }
    }

    build_logbook_test(active_log_page,StepBackwards);
//	GFX_ResetLayer(TOP_LAYER);
//	GFX_ResetLayer(BACKGRD_LAYER);

    set_globalState(StILOGSHOW);
    GFX_SetFramesTopBottom(tLOGscreen.FBStartAdress, tLOGbackground.FBStartAdress,480);
}


void show_logbook_exit(void)
{
    releaseFrame(16,tLOGscreen.FBStartAdress);
    releaseFrame(17,tLOGbackground.FBStartAdress);
}


static void show_logbook_logbook_show_log_page3(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards)
{
    SWindowGimpStyle wintemp;
    SWindowGimpStyle winsmal;
    uint8_t gasWasUsed[NUM_GASES * 2];
    int16_t index = 0;
    uint8_t loopMode = 0;

    wintemp.left = 50;
    wintemp.right = 799 - wintemp.left;
    wintemp.top = 50;
    wintemp.bottom = 479 - 40;

    SLogbookHeader logbookHeader;

    logbook_getHeader(StepBackwards, &logbookHeader);
    uint16_t dataLength = 0;
    uint16_t depthdata[1000];
    uint8_t  gasdata[1000];
    dataLength = logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    char msg[15];
    char gas_name[15];
    int j = 0;

    loopMode = isLoopMode(logbookHeader.diveMode);

    /* check if gas was used, independent from its active state */
    for(index = 0; index < NUM_GASES * 2; index++)
    {
    	gasWasUsed[index] = 0;
    }
    for(index = 0; index < dataLength; index++)
    {
    	if(loopMode)
    	{
    		if(gasdata[index] < NUM_GASES)	/* the log entry starts with a ID in range 1..4 independend if diluent is used at start */
			{
				gasdata[index] += NUM_GASES;
			}
    		else
    		{
    			loopMode = 0;				/* after the first gas change, no matter if diluent or bailout, the correct ID will be stored */
    		}
    	}
    	if(gasdata[index] > 0)
    	{
    		gasWasUsed[gasdata[index]-1] = 1;		/* The ID stored in the samples is starting with 1 (array[0] = gasID1) */
    	}
    	else
    	{
    		gasWasUsed[0] = 1;
    	}
    }

    //--- print coordinate system & depth graph with gaschanges ---
    show_logbook_draw_depth_graph(hgfx, StepBackwards, &wintemp, 1, dataLength, depthdata, gasdata, NULL);

    //--- print gas list ---
    winsmal.left = wintemp.right - 190;
    winsmal.right =  winsmal.left + 150;

    loopMode = isLoopMode(logbookHeader.diveMode);
    for(index = (2 * NUM_GASES) -1; index >= 0; index--)
    {
    	if(gasWasUsed[index])
        {
            j++;
            if(j > 5)	/* limit number of gases displayed to 5 */
            {
            	break;
            }
            winsmal.top	= wintemp.bottom - 5 - j * 26 ;
            winsmal.bottom = winsmal.top + 21  ;
            uint8_t color = get_colour(index);

            if(loopMode)
            {
            	if(index < NUM_GASES)		/* Switch to Bailout is not covered by log gas list */
            	{
            		snprintf(gas_name,15,"Bailout");
            		snprintf(msg,15,"G%d: %s",index +1, gas_name);
            	}
            	else
            	{
            		print_gas_name(gas_name,15,logbookHeader.gasordil[index-NUM_GASES].oxygen_percentage,logbookHeader.gasordil[index-NUM_GASES].helium_percentage);
            		snprintf(msg,15,"D%d: %s",index +1 - NUM_GASES, gas_name);
            	}
            }
            else
            {
            	print_gas_name(gas_name,15,logbookHeader.gasordil[index].oxygen_percentage,logbookHeader.gasordil[index].helium_percentage);
            	snprintf(msg,15,"G%d: %s",index +1, gas_name);
            }
            Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,msg);
        }
    }

    //--- define buttons ---
    /*if(*ghost_char_logfile_oxydata)
        button_start_single_action(surf1_menu_logbook_current_page, surf1_menu_logbook_show_log_page4, surf1_menu_logbook_show_log_next);
    else
        button_start_single_action(surf1_menu_logbook_current_page, surf1_menu_logbook_show_log_page1, surf1_menu_logbook_show_log_next);
        */
}

static uint8_t check_data_array_empty(uint16_t* pdata)
{
	uint8_t ret = 0;
	uint8_t index = 0;
	uint8_t emptyCnt = 0;

	for (index=0; index < 10; index++)	/* read the first 10 data points. If all are 0 then the array is rated as empty */
	{
		if(*(pdata+index) == 0)
		{
			emptyCnt++;
		}
	}
	if(emptyCnt == 10)
	{
		ret = 1;
	}
	return ret;
}

static void show_logbook_logbook_show_log_page4(GFX_DrawCfgScreen *hgfx, uint8_t StepBackwards)
{ SWindowGimpStyle wintemp;
    SWindowGimpStyle winsmal;
    wintemp.left = 50;
    wintemp.right = 799 - wintemp.left;
    wintemp.top = 50;
    wintemp.bottom = 479 - 40;
    uint8_t color = 0;
    SLogbookHeader logbookHeader;

    logbook_getHeader(StepBackwards, &logbookHeader);
    uint16_t dataLength = 0;
    uint16_t depthdata[1000];
    uint8_t  gasdata[1000];
    uint16_t ppO2data[1000];
    uint16_t sensor2[1000];
    uint16_t sensor3[1000];
    uint16_t *setpoint = ppO2data;
    uint16_t *sensor1 = ppO2data;
    uint8_t  sensorDataAvailable[] = {0,0,0};


        if(!isLoopMode(logbookHeader.diveMode))
            dataLength = logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, NULL, ppO2data, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        else
        {
        	switch(logbookHeader.CCRmode)
        	{
        		case CCRMODE_FixedSetpoint:
        		default:				dataLength = logbook_readSampleData(StepBackwards, 1000, depthdata, gasdata, NULL, NULL, setpoint, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        				break;
        		case CCRMODE_Sensors:	dataLength = logbook_readSampleData(StepBackwards, 1000, depthdata, gasdata, NULL, NULL, NULL, sensor1, sensor2, sensor3, NULL, NULL, NULL, NULL, NULL, NULL);
        								if(!check_data_array_empty(sensor1))
        								{
        									sensorDataAvailable[0] = 1;
        								}
        								if(!check_data_array_empty(sensor2))
        								{
        								    sensorDataAvailable[1] = 1;
        								}
        								if(!check_data_array_empty(sensor3))
        								{
        								    sensorDataAvailable[2] = 1;
        								}
        								if((logbookHeader.diveMode == DIVEMODE_PSCR) && (sensorDataAvailable[0] + sensorDataAvailable[1] + sensorDataAvailable[2] != 3)) /*insert sim data if not all three sensors are in use*/
        								{
        									if(sensorDataAvailable[0] == 0)
        									{
        										logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, NULL, sensor1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        										sensorDataAvailable[0] = 1;
        									}
        									else if(sensorDataAvailable[1] == 0)
        									{
        										logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, NULL, sensor2, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        										sensorDataAvailable[1] = 1;
        									}
        									else if(sensorDataAvailable[2] == 0)
        									{
        										logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, NULL, sensor3, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        										sensorDataAvailable[2] = 1;
        									}
        								}
        		    	break;
        		case CCRMODE_Simulation: dataLength = logbook_readSampleData(StepBackwards, 1000, depthdata,gasdata, NULL, ppO2data, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        				break;
        	}
        }


    //--- print coordinate system & depth graph with bailout---
    show_logbook_draw_depth_graph(hgfx, StepBackwards, &wintemp, 0, dataLength, depthdata, gasdata, NULL);



    //*** Desciption at bottom of page ***************************
    winsmal.top	= wintemp.bottom +2 ;
    winsmal.bottom = winsmal.top + 16;


    /*if(strcmp( (char*)ghost_char_logfile_text_oc_ccr,"ccr/bailout") == 0)
    {
        winsmal.left = wintemp.left + 2;
        winsmal.right =  winsmal.left + 55;

        oled_write(OVERLAY, &winsmal,"CCR -",false,true);

        winsmal.left = winsmal.right;
        winsmal.right =  winsmal.left + 90;
        //winsmal.fontcolor = oled_get_colour(15);
        oled_write(OVERLAY, &winsmal,"bailout",false,true);
    }
    else*/
    {
        winsmal.left = wintemp.left + 2;
        winsmal.right =  winsmal.left + 55;
         color = CLUT_GasSensor1;//LOGBOOK_GRAPH_DEPTH;

         Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,"depth");

    }
    winsmal.left = 799 - 67;//wintemp.right -67;
    winsmal.right = winsmal.left;// + 45;

    color = CLUT_LogbookTemperature;

    if(!isLoopMode(logbookHeader.diveMode))
    {
    	Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,"\002PP O2");
    }
    else
    {
    	switch(logbookHeader.CCRmode)
    	{
    		case CCRMODE_FixedSetpoint:
    		default:				Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,"\002SETPOINT");
    				break;
    		case CCRMODE_Sensors:	Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,"\002SENSORS");
    		    	break;
    		case CCRMODE_Simulation: Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,"\002SIM PPO2");
    				break;
    	}
    }

    //*** PP O2 ****************************************************
    //calc lines and labels
    int datamax = 0;
    int datamin = 10000;
    for(int i=1;i<dataLength;i++)
    {
        if(ppO2data[i]>datamax)
            datamax = ppO2data[i];
        if(ppO2data[i]<datamin)
            datamin = ppO2data[i];
    }
    if(isLoopMode(logbookHeader.diveMode) && (logbookHeader.CCRmode == CCRMODE_Sensors))
    {
        for(int i=1;i<dataLength;i++)
        {
            if(sensor2[i]>datamax)
                datamax = sensor2[i];
            if(sensor2[i]<datamin)
                datamin = sensor2[i];
            if(sensor3[i]>datamax)
                datamax = sensor3[i];
            if(sensor3[i]<datamin)
                datamin = sensor3[i];
        }
    }
    float maxoxy = ((float)datamax)/100;
    float minoxy =  ((float)datamin)/100;
    float oxystep = 0.5;
    float maxoxyline = 2.5;

    //--- print PP O2 labels ----
    winsmal.left = wintemp.right + 2;
    winsmal.top	= wintemp.top ;
    winsmal.right =  wintemp.right + 30;
    winsmal.bottom = winsmal.top + 16;
    //winsmal.font = ft_tiny + ft_SLIM;
    color = CLUT_LogbookTemperature;// = LOGBOOK_GRAPH_TEMP;

    Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,"bar");

    int deltaline = (wintemp.bottom - wintemp.top)/5;
    char msg[4];
    float oxy = maxoxyline;
    for(int i = 1; i<=5; i++)
    {
        oxy -= 	oxystep;
        if(oxy < 0)
            break;
        winsmal.top	= wintemp.top + deltaline * i - 8;
        winsmal.bottom = winsmal.top + 16;
        snprintf(msg,4,"%1.1f",oxy);
        Gfx_write_label_var(hgfx, winsmal.left, winsmal.right,winsmal.top, &FontT24,color,msg);
        //oled_write(OVERLAY, &winsmal,msg,false,true);
    }

    //--- print PP O2 graph ---
    //Adapt window
    float ftmp = ((maxoxyline - minoxy) * deltaline) /oxystep + wintemp.top;
    wintemp.bottom = ftmp;
    if((ftmp - (int)ftmp) >= 0.5f)
        wintemp.bottom++;

    ftmp = wintemp.top + ((maxoxyline - maxoxy) * deltaline) /oxystep;
    wintemp.top = ftmp;
    if((ftmp - (int)ftmp) >= 0.5f)
        wintemp.top++;
    wintemp.top = MaxU32LOG(wintemp.top ,0);
    if(wintemp.top < wintemp.bottom)
    {
    	if(isLoopMode(logbookHeader.diveMode) && (logbookHeader.CCRmode == CCRMODE_Sensors))
        {
            if(sensorDataAvailable[0])
            {
            	GFX_graph_print(hgfx,&wintemp,0,1,datamax,datamin, ppO2data,dataLength,CLUT_GasSensor2, NULL);
            }
            if(sensorDataAvailable[1])
            {
            	GFX_graph_print(hgfx,&wintemp,0,1,datamax,datamin, sensor2,dataLength,CLUT_GasSensor3, NULL);
            }
            if(sensorDataAvailable[2])
            {
            	GFX_graph_print(hgfx,&wintemp,0,1,datamax,datamin, sensor3,dataLength,CLUT_GasSensor4, NULL);
            }
        }
        else
            GFX_graph_print(hgfx,&wintemp,0,1,datamax,datamin, ppO2data,dataLength,CLUT_LogbookTemperature, NULL);
    }
    else
    {
        point_t startPoint, stopPoint;
        startPoint.x = wintemp.left;
        stopPoint.x = wintemp.right;
        stopPoint.y = startPoint.y = 479 - wintemp.top;
        GFX_draw_colorline(hgfx, startPoint, stopPoint, CLUT_LogbookTemperature);
    }

    //--- define buttons ---
    //button_start_single_action(surf1_menu_logbook_current_page, surf1_menu_logbook_show_log_page1, surf1_menu_logbook_show_log_next);
}

static void print_gas_name(char* output,uint8_t length,uint8_t oxygen,uint8_t helium)
{
    if(helium == 0)
    {
        if(oxygen == 21)
            snprintf(output, length, "Air");
        else if(oxygen == 100)
            snprintf(output, length, "Oxy");
        else
            snprintf(output, length, "NX%i",oxygen);
    }
    else
        {
                if((oxygen + helium) == 100)
            snprintf(output, length, "HX%i",oxygen);
                else
                        snprintf(output, length, "TMX%i/%i", oxygen, helium);
        }

}

static int16_t get_colour(int16_t color)
{
     return CLUT_GasSensor1 + color;
}

uint8_t getActiveLogPage()
{
	return active_log_page;
}

void updateReplayIncdicator(GFX_DrawCfgScreen *hgfx)
{
    build_logbook_test(1,active_log_offset);
    GFX_SetFramesTopBottom(tLOGscreen.FBStartAdress, tLOGbackground.FBStartAdress,480);
}
