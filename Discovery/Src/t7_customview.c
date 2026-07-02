///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/t7.c
/// \brief  Main Template file for dive mode 7x
/// \author Heinrichs Weikamp gmbh
/// \date   23-April-2014
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
#include <stdlib.h>

#include "t7.h"
#include "t7_customview.h"
#include "text_multilanguage.h"
#include "settings.h"
#include "data_exchange_main.h"
#include "data_central.h"
#include "decom.h"
#include "gfx_fonts.h"
#include "gfx_colors.h"
#include "logbook_miniLive.h"
#include "math.h"
#include "simulation.h"
#include "timer.h"
#include "unit.h"
#include "configuration.h"
#include "cavemode.h"
#include "check_warning.h"
#include "tMenuGas.h"
#include "vpm.h"
#include "base.h"

extern GFX_DrawCfgScreen	t7screen;

extern GFX_DrawCfgWindow	t7l1;
extern GFX_DrawCfgWindow	t7r1;
extern GFX_DrawCfgWindow	t7cH, t7cC, t7cY0free;

extern _Bool warning_count_high_time;


enum gasListOptions
{
	GASLIST_CHANGEDEPTH		= 0,
	GASLIST_DEMAND,
	GASLIST_END
};


/* Imported function prototypes ---------------------------------------------*/
extern uint8_t write_gas(char *text, uint8_t oxygen, uint8_t helium);

/* Private function prototypes -----------------------------------------------*/

static void t7_logo_OSTC(void)
{
    SWindowGimpStyle windowGimp;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

    /* OSTC logo */
	if(!pSettings->FlipDisplay)
	{
		windowGimp.left = t7l1.WindowX1 + 32;
	}
	else
	{
		windowGimp.left = t7r1.WindowX1 + 32;
	}

    windowGimp.top = 40 + 32;
    GFX_draw_image_monochrome(&t7screen, windowGimp, &ImgOSTC, 0);
}

static uint8_t t7_customtextPrepare(char * text)
{
    uint8_t i, j, textptr, lineCount;
    char nextChar;
    uint8_t alignmentChanged = 0;

    textptr = 0;
    lineCount = 0;

    text[textptr++] = TXT_MINIMAL;

    j = 0;
    i = 0;
    do
    {
        j += i;
        i = 0;
        do
        {
            nextChar = settingsGetPointer()->customtext[i+j];
         	if((nextChar == '^') && (alignmentChanged == 0))		/* center */
           	{
           		text[textptr++] = '\001';
           		alignmentChanged = 1;
           		i++;
           	}else
           	if((nextChar == 180) && (alignmentChanged == 0))		/* '�' => Right */
           	{
           		text[textptr++] = '\002';
           		alignmentChanged = 1;
           		i++;
           	}else
           	{
           		i++;
           		if((!nextChar) || (nextChar =='\n')  || (nextChar =='\r'))
           		{
           			break;
           		}
           		text[textptr++] = nextChar;
           	}
        } while (i < 12);

        if(i == 12)		/* exit by limit => check for blanks at the end of the string */
        {
        	while((textptr - 1 > 0) && (text[textptr - 1] == 32))
			{
				textptr--;
			}
        }

        if(!nextChar)
            break;

        if(lineCount < 3)
        {
            text[textptr++] = '\n';
            text[textptr++] = '\r';
        }
        alignmentChanged = 0;
        lineCount++;
        for(uint8_t k=0;k<2;k++)
        {
            nextChar = settingsGetPointer()->customtext[i+j+k];
            if((nextChar =='\n')  || (nextChar =='\r'))
                i++;
            else
                break;
        }

    } while (lineCount < 4);

    text[textptr] = 0;
    return lineCount;
}
void t7_cv_debug(void)
{
    char text[256+50];
    uint8_t textpointer = 0;

    t7cY0free.WindowLineSpacing = 28 + 48 + 14;
    t7cY0free.WindowY0 = t7cH.WindowY0 - 5 - 2 * t7cY0free.WindowLineSpacing;
    t7cY0free.WindowNumberOfTextLines = 3;

#ifdef T7_DEBUG_RUNTIME
    textpointer += snprintf(&text[textpointer],50,"Main loop %ld\n\r",getMainLoopTime());
    textpointer += snprintf(&text[textpointer],50,"Grafic loop %ld\n\r",getGfxLoopTime());
    textpointer += snprintf(&text[textpointer],50,"Decoloop %ld\n\r",getDecoLoopTime());
    GFX_write_string(&FontT24, &t7cY0free, text, 1);
#else
    textpointer += snprintf(&text[textpointer],50,"Ambient [bar]\n\r");
    textpointer += snprintf(&text[textpointer],50,"Surface [bar] + salt\n\r");
    textpointer += snprintf(&text[textpointer],50,"ShallowCounter [s]\n\r");
    GFX_write_string(&FontT24, &t7cY0free, text, 1);

    t7cY0free.WindowY0 -= 52;
    snprintf(text,60,
        "%0.2f\n\r"
        "%0.2f       %u%%\n\r"
        "%u"
        ,stateUsed->lifeData.pressure_ambient_bar
        ,stateUsed->lifeData.pressure_surface_bar
        ,settingsGetPointer()->salinity
        ,stateUsed->lifeData.counterSecondsShallowDepth);
    GFX_write_string(&FontT42, &t7cY0free, text, 1);
#endif
}


void t7_cv_hello()
{
    char text[256+50];
    uint8_t textpointer = 0;

	int16_t shiftWindowY0;
    SSettingsStatus SettingsStatus;
    uint8_t lineCountCustomtext = 0;

	t7_logo_OSTC();
	t7cC.WindowLineSpacing = 53;
	t7cC.WindowNumberOfTextLines = 5;
	shiftWindowY0 = 18;

	 if(DataEX_check_RTE_version__needs_update() || font_update_required())
	 {
		 if(warning_count_high_time)
		 {
			 shiftWindowY0 += 20;
			 t7cC.WindowY0 -= shiftWindowY0;
			 textpointer = 0;
			 text[textpointer++] = TXT_2BYTE;
			 text[textpointer++] = TXT2BYTE_PleaseUpdate;
			 text[textpointer++] = '\n';
			 text[textpointer++] = '\r';
			 if(DataEX_check_RTE_version__needs_update())
			 {
				 text[textpointer++] = TXT_2BYTE;
				 text[textpointer++] = TXT2BYTE_RTE;
				 text[textpointer++] = '\n';
				 text[textpointer++] = '\r';
			 }
			 if(font_update_required())
			 {
				 text[textpointer++] = TXT_2BYTE;
				 text[textpointer++] = TXT2BYTE_Fonts;
			 }
			 text[textpointer++] = 0;
			 GFX_write_string_color(&FontT42,&t7cC,text,1, CLUT_WarningRed);
			 t7cC.WindowY0 += shiftWindowY0;
		 }
		 t7cC.WindowNumberOfTextLines = 3;
	 }
	 else if(isSettingsWarning())
		{
		 if(warning_count_high_time)
		 {
			get_CorrectionStatus(&SettingsStatus);
			 shiftWindowY0 += 20;
			 t7cC.WindowY0 -= shiftWindowY0;
			 textpointer = 0;
			 snprintf(text,255,"\001%c%c\n\r\001%d|%d",TXT_2BYTE,TXT2BYTE_CheckSettings,SettingsStatus.FirstCorrection,SettingsStatus.Corrections);
			 GFX_write_string_color(&FontT42,&t7cC,text,1, CLUT_WarningRed);
			 t7cC.WindowY0 += shiftWindowY0;
		 }
		 t7cC.WindowNumberOfTextLines = 1;
		}
	 else if(!isCompassCalibrated()) {
		 if(warning_count_high_time) {
			 shiftWindowY0 += 20;
			 t7cC.WindowY0 -= shiftWindowY0;
			 textpointer = 0;
			 text[textpointer++] = '\001';
			 text[textpointer++] = TXT_2BYTE;
			 text[textpointer++] = TXT2BYTE_Compass;
			 text[textpointer++] = '\n';
			 text[textpointer++] = '\r';
			 text[textpointer++] = '\001';
			 text[textpointer++] = TXT_2BYTE;
			 text[textpointer++] = TXT2BYTE_NotCalibrated;
			 text[textpointer++] = '\n';
			 text[textpointer++] = '\r';
			 text[textpointer++] = 0;
			 GFX_write_string_color(&FontT42, &t7cC,text, 1, CLUT_WarningRed);
			 t7cC.WindowY0 += shiftWindowY0;
		 }
		 t7cC.WindowNumberOfTextLines = 2;
		}
	 else // customtext
	 {
		 lineCountCustomtext = t7_customtextPrepare(text);
		 if(lineCountCustomtext <= 2)
			 shiftWindowY0 += 20+26; // nach unten
		 else
		 if(lineCountCustomtext <= 3)
			 shiftWindowY0 += 20; // nach unten
		 t7cC.WindowY0 -= shiftWindowY0;

		 GFX_write_string(&FontT42,&t7cC,text,1);
		 t7cC.WindowNumberOfTextLines = 3;
		 t7cC.WindowY0 += shiftWindowY0;
	 }
	 if(lineCountCustomtext <= 4)
	 {
		 snprintf(text,100,"\001#%0u V%01u.%01u.%01u",
		 settingsGetPointer()->serialLow + (256 * settingsGetPointer()->serialHigh),
		 firmwareDataGetPointer()->versionFirst,
		 firmwareDataGetPointer()->versionSecond,
		 firmwareDataGetPointer()->versionThird
		 );
		 GFX_write_string(&FontT24,&t7cC,text,0);
	 }
}


static void gasList_changeDepth()
{
    char text[30];
    uint16_t textpointer = 0;
    const SGasLine * pGasLine; // CVIEW_Gaslist
    uint8_t oxygen, helium; // CVIEW_Gaslist
    float fPpO2limitHigh, fPpO2limitLow, fPpO2ofGasAtThisDepth; // CVIEW_Gaslist

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_Gaslist);
    GFX_write_string(&FontT42,&t7cH,text,0);

    textpointer = 0;

    if(!pSettings->FlipDisplay)
    {
       	t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
    }
    else
    {
     	t7cY0free.WindowY1 = 400;
    }
    t7cY0free.WindowLineSpacing = 48+9;
    t7cY0free.WindowNumberOfTextLines = 5; // NUM_GASES == 5
    t7cY0free.WindowTab = 420;

    pGasLine = settingsGetPointer()->gas;
    if(actualLeftMaxDepth(stateUsed))
    {
        fPpO2limitHigh = (float)(settingsGetPointer()->ppO2_max_deco) / 100;
    }
    else
    {
    	fPpO2limitHigh = (float)(settingsGetPointer()->ppO2_max_std) / 100;
    }
    fPpO2limitLow = (float)(settingsGetPointer()->ppO2_min) / 100;
    for(int gasId=1;gasId<=NUM_GASES;gasId++)
    {
    	textpointer = 0;
#ifdef ENABLE_UNUSED_GAS_HIDING
        if(!pGasLine[gasId].note.ub.off)
        {
#endif
        fPpO2ofGasAtThisDepth = (stateUsed->lifeData.pressure_ambient_bar - WATER_VAPOUR_PRESSURE) * pGasLine[gasId].oxygen_percentage / 100;
        if(pGasLine[gasId].note.ub.active == 0)
        {
        	strcpy(&text[textpointer++],"\031");
        }
        else if(stateUsed->lifeData.actualGas.GasIdInSettings == gasId)	/* actual selected gas */
        {
        	strcpy(&text[textpointer++],"\030");
        }
        else if((fPpO2ofGasAtThisDepth > fPpO2limitHigh) || (fPpO2ofGasAtThisDepth < fPpO2limitLow))
        {
        	strcpy(&text[textpointer++],"\025");
        }
        else if(actualBetterGasId() == gasId)
        {
            strcpy(&text[textpointer++],"\026");			/* Highlight better gas */
        }
        else
        {
            strcpy(&text[textpointer++],"\023");
        }

        text[textpointer++] = ' ';
        oxygen = pGasLine[gasId].oxygen_percentage;
        helium = pGasLine[gasId].helium_percentage;
        textpointer += write_gas(&text[textpointer], oxygen, helium);
            // Wechseltiefe
        if(pGasLine[gasId].depth_meter)
        {
            textpointer += snprintf(&text[textpointer],10,"\t%u %c%c",unit_depth_integer(pGasLine[gasId].depth_meter), unit_depth_char1(), unit_depth_char2());
        }
        GFX_write_string(&FontT42, &t7cY0free, text, gasId);
#ifdef ENABLE_UNUSED_GAS_HIDING
     }
#endif
    }
}

static void gasList_demand()
{
    char text[200];
    uint16_t textpointer = 0;
    const SGasLine * pGasLine;
	SSettings* pSettings;
	pSettings = settingsGetPointer();
    uint8_t oxygen, helium;
    uint8_t line = 1;

    snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_GasDemand);
	GFX_write_string(&FontT42,&t7cH,text,0);
	// content
	textpointer = 0;
	if(!pSettings->FlipDisplay)
	{
		t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
	}
	else
	{
		t7cY0free.WindowY1 = 400;
	}
	t7cY0free.WindowLineSpacing = 48+9;
	t7cY0free.WindowNumberOfTextLines = 5; // NUM_GASES == 5
	t7cY0free.WindowTab = 420;
	pGasLine = settingsGetPointer()->gas;
	for(int gasId=1;gasId<=NUM_GASES;gasId++)
	{
		textpointer = 0;
			if(((pGasLine[gasId].note.ub.active) || (pGasLine[gasId].note.ub.deco)) && (pGasLine[gasId].bottle_size_liter != 0))
		{
			if(stateUsed->lifeData.caveGasNeed_Ltr[gasId] != 0)
			{
				if(stateUsed->lifeData.caveGasNeed_Ltr[gasId] > pGasLine[gasId].bottle_id_bar * pGasLine[gasId].bottle_size_liter)
				{
					text[textpointer++] = '\025';	/* more gas needed than available => red */
				}
				else if(stateUsed->lifeData.caveGasNeed_Ltr[gasId] > (pGasLine[gasId].bottle_id_bar * pGasLine[gasId].bottle_size_liter) * 0.7)
				{
					text[textpointer++] = '\024';	/* 70% warning => yellow */
				}
				else
				{
					text[textpointer++] = '\020';
				}
			}
	        text[textpointer++] = ' ';
			oxygen = pGasLine[gasId].oxygen_percentage;
			helium = pGasLine[gasId].helium_percentage;
			textpointer += write_gas(&text[textpointer], oxygen, helium);
			snprintf(&text[textpointer],100,"\002%ldBar",(stateUsed->lifeData.caveGasNeed_Ltr[gasId] / pGasLine[gasId].bottle_size_liter));
			GFX_write_string(&FontT42, &t7cY0free, text, line);
			line++;
		}
	}
}

void t7_cv_gasList()
{
	static uint32_t changeTick = 0;
	static uint8_t curOption = GASLIST_CHANGEDEPTH;
	uint8_t foundOption = 0;
	uint8_t nextOption = 0;

	if(time_elapsed_ms(changeTick, HAL_GetTick()) > 3000)
	{
		changeTick = HAL_GetTick();
		nextOption = curOption;
		do
		{
			nextOption++;
			if(nextOption == GASLIST_END)
			{
				nextOption = GASLIST_CHANGEDEPTH;
			}
			switch(nextOption)
			{
				case GASLIST_DEMAND: 	if(!caveMode_isOff())
										{
											foundOption = 1;
										}
					break;
				default:	foundOption = 1; /* no disable condition */
			}
		}
		while((nextOption != curOption) && (foundOption == 0));
		curOption = nextOption;
	}
	switch(curOption)
	{
		case GASLIST_CHANGEDEPTH:
		default: gasList_changeDepth();
			break;
		case GASLIST_DEMAND: gasList_demand();
			break;
	}
}

void t7_cv_EADTime()
{
    char text[100];
    char tmpString[20];
    float depth, surface, fraction_nitrogen, fraction_helium, ead, end; // CVIEW_EADTime
    RTC_DateTypeDef Sdate;
    RTC_TimeTypeDef Stime;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_Info );
	GFX_write_string(&FontT42,&t7cH,text,0);

	t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
	if(pSettings->FlipDisplay)
	{
		t7cY0free.WindowY1 = 400;
	}
	t7cY0free.WindowLineSpacing = 48;
	t7cY0free.WindowNumberOfTextLines = 6;

	// time
	snprintf(text,100,"\032\001%c%c",TXT_2BYTE,TXT2BYTE_Clock );
	GFX_write_string(&FontT42, &t7cY0free, text, 1);

	translateDate(stateRealGetPointer()->lifeData.dateBinaryFormat, &Sdate);
	translateTime(stateRealGetPointer()->lifeData.timeBinaryFormat, &Stime);

	formatStringOfTime(tmpString,20,Stime,1,0);
	snprintf(text,100,"\030\001%s",tmpString);
	GFX_write_string(&FontT42, &t7cY0free, text, 2);

	// EAD / END
	// The equivalent air depth can be calculated for depths in metres as follows:
	// EAD = (Depth + 10) ï¿½ Fraction of N2 / 0.79 - 10   (wikipedia)
	// The equivalent narcotic depth can be calculated for depths in metres as follows:
	// END = (Depth + 10) ï¿½ (1 - Fraction of helium) - 10  (wikipedia)
	decom_get_inert_gases((float)stateUsed->lifeData.pressure_ambient_bar,&(stateUsed->lifeData.actualGas),&fraction_nitrogen,&fraction_helium);
	depth = stateUsed->lifeData.pressure_ambient_bar;
	surface = stateUsed->lifeData.pressure_surface_bar;
	ead = 10.f * ((depth * fraction_nitrogen/0.79f) - surface);
	end = 10.0f * ((depth * (1.f - fraction_helium)) - surface);
	if(ead < 0)
		ead = 0;
	if(end < 0)
		end = 0;

	snprintf(text,100,"\032\001EAD");
	GFX_write_string(&FontT42, &t7cY0free, text, 3);
	snprintf(text,100,"\030\001%01.1f %c%c"
	, unit_depth_float(ead)
	, unit_depth_char1()
	, unit_depth_char2()
	);
	GFX_write_string(&FontT42, &t7cY0free, text, 4);

	snprintf(text,100,"\032\001END");
	GFX_write_string(&FontT42, &t7cY0free, text, 5);
	snprintf(text,100,"\030\001%01.1f %c%c"
	, unit_depth_float(ead)
	, unit_depth_char1()
	, unit_depth_char2()
	);
	GFX_write_string(&FontT42, &t7cY0free, text, 6);
}


void t7_cv_Decolist()
{
    char text[100];
    uint16_t textpointer = 0;
	uint8_t decoPlanEntries = 6;
    int16_t start;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if(pSettings->VPM_conservatism.ub.alternative == 0)
	{
		text[0] = '\032';
	}
	else
	{
		switch(vpm_get_TableState())
		{
			case VPM_TABLE_MISSED: text[0] = '\025';
				break;
			case VPM_TABLE_WARNING: text[0] = '\024';
				break;
			case VPM_TABLE_ACTIVE:
			case VPM_TABLE_INIT:
			default:	text[0] = '\032';
				break;
		}
	}

	snprintf(&text[1],100,"\f\001 %c%c", TXT_2BYTE, TXT2BYTE_Decolist);
	GFX_write_string(&FontT42,&t7cH,text,0);

	uint8_t depthNext, depthLast, depthSecond, depthInc;

	depthLast 		= (uint8_t)(stateUsed->diveSettings.last_stop_depth_bar * 10);
	depthSecond 	= (uint8_t)(stateUsed->diveSettings.input_second_to_last_stop_depth_bar * 10);
	depthInc 			= (uint8_t)(stateUsed->diveSettings.input_next_stop_increment_depth_bar * 10);

	if(settingsGetPointer()->nonMetricalSystem)
	{
		depthLast		= (uint8_t)unit_depth_integer(depthLast);
		depthSecond	= (uint8_t)unit_depth_integer(depthSecond);
		depthInc 		= (uint8_t)unit_depth_integer(depthInc);
	}
	if(stateUsed->diveSettings.deco_type.ub.standard == VPM_MODE) /* show additional VPM data in last slot */
	{
		decoPlanEntries = 5;
	}

	const SDecoinfo * pDecoinfo = getDecoInfo();
	for(start=DECOINFO_STRUCT_MAX_STOPS-1; start>0; start--)
		if(pDecoinfo->output_stop_length_seconds[start]) break;
	start -= decoPlanEntries;
	if(start < 0) start = 0;

	textpointer = 0;
	for(int i=start;i<decoPlanEntries+start;i++)
	{
		if(i == 0)
			depthNext = depthLast;
		else
			depthNext = depthSecond + (( i - 1 )* depthInc);

		if(pDecoinfo->output_stop_length_seconds[i])
			textpointer += snprintf(&text[textpointer],20,"\030\034   %2u\016\016%c%c\017%3i'\n\r",depthNext, unit_depth_char1(), unit_depth_char2(), (pDecoinfo->output_stop_length_seconds[i]+59)/60);
		else
			textpointer += snprintf(&text[textpointer],20,"\031\034   %2u\016\016%c%c\017\n\r",depthNext, unit_depth_char1(), unit_depth_char2());
		if(textpointer > 200) break;
	}
	if(decoPlanEntries == 5) /* add VPM deco zone */
	{
		textpointer += snprintf(&text[textpointer],30,"\031\034   Zone %2u\016\016%c%c\017\n\r",vpm_get_decozone(), unit_depth_char1(), unit_depth_char2());
	}
	if(!pSettings->FlipDisplay)
	{
		t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
	}
	else
	{
		t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
		t7cY0free.WindowY1 = 400;
	}

	t7cY0free.WindowLineSpacing = 48;
	t7cY0free.WindowNumberOfTextLines = 6;
	GFX_write_string(&FontT42, &t7cY0free, text, 1);
}

void t7_cv_sensors()
{
    char text[100];
    uint16_t textpointer = 0;
	uint8_t local_ppo2sensors_deactivated = 0;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

#ifdef ENABLE_PSCR_MODE
	uint8_t showSimPPO2 = 1;
#endif

	if(stateUsed->mode == MODE_DIVE)	/* show sensors based on current dive settings */
	{
		local_ppo2sensors_deactivated = stateUsed->diveSettings.ppo2sensors_deactivated;
	}
	else
	{
		local_ppo2sensors_deactivated = pSettings->ppo2sensors_deactivated;
	}
	snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_O2monitor);
    GFX_write_string(&FontT42,&t7cH,text,0);
    textpointer = 0;
    text[textpointer++] = '\030'; // main color
    for(int i=0;i<3;i++)
    {
        if((local_ppo2sensors_deactivated & (1<<i)) || (stateUsed->lifeData.ppO2Sensor_bar[i] == 0.0))
        {
#ifdef ENABLE_PSCR_MODE
         	if((stateUsed->diveSettings.diveMode == DIVEMODE_PSCR) && (showSimPPO2) && (stateUsed->mode == MODE_DIVE))	/* display ppo2 sim in blue letters in case a slot is not used in the ppo2 custom view */
          	{
           		text[textpointer++] = '\023';
           		textpointer += snprintf(&text[textpointer],100,"\001%01.2f\n\r\030",stateUsed->lifeData.ppo2Simulated_bar);
           		showSimPPO2 = 0;
           	}
          	else
#endif
           	{
				text[textpointer++] = '\031'; // labelcolor
				text[textpointer++] = '\001';
				text[textpointer++] = '-';
				text[textpointer++] = '\n';
				text[textpointer++] = '\r';
				text[textpointer++] = '\030'; // main color
				text[textpointer] = 0;
           	}
        }
        else
        {
            if(stateUsed->warnings.sensorOutOfBounds[i])
                text[textpointer++] = '\025'; // Warning Red
            textpointer += snprintf(&text[textpointer],100,"\001%01.2f\n\r\030",stateUsed->lifeData.ppO2Sensor_bar[i]);
        }
    }
    t7cC.WindowLineSpacing = 95;
    t7cC.WindowNumberOfTextLines = 3;
    text[textpointer] = 0;
    if(pSettings->FlipDisplay)
    {
     	t7cC.WindowY1 -= 40;
    }
    GFX_write_string(&FontT105,&t7cC,text,1);
    if(pSettings->FlipDisplay)
    {
     	t7cC.WindowY1 += 40;
    }
}

void t7_cv_sensors_mV()
{
    char text[100];
    uint16_t textpointer = 0;
	uint8_t local_ppo2sensors_deactivated = 0;

	SSettings* pSettings;
	pSettings = settingsGetPointer();

	snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_O2voltage);

    GFX_write_string(&FontT42,&t7cH,text,0);
    textpointer = 0;
    text[textpointer++] = '\030';
    for(int i=0;i<3;i++)
    {
    	if(local_ppo2sensors_deactivated & (1<<i))
        {
    		text[textpointer++] = '\031';
            text[textpointer++] = '\001';
            text[textpointer++] = '-';
            text[textpointer++] = '\n';
            text[textpointer++] = '\r';
            text[textpointer++] = '\030';
            text[textpointer] = 0;
         }
         else
         {
            if(stateUsed->warnings.sensorOutOfBounds[i])
                 text[textpointer++] = '\025';
            textpointer += snprintf(&text[textpointer],100,"\001%01.1f mV\n\r\030",(stateUsed->lifeData.sensorVoltage_mV[i]));
         }
    }
    t7cC.WindowLineSpacing = 95;
    t7cC.WindowNumberOfTextLines = 3;
    text[textpointer] = 0;
    if(pSettings->FlipDisplay)
    {
     	t7cC.WindowY1 -= 40;
    }
    GFX_write_string(&FontT48,&t7cC,text,1);
    if(pSettings->FlipDisplay)
    {
     	t7cC.WindowY1 += 40;
    }
}

#ifdef ENABLE_CAVEMODE
void t7_cv_Cave(void)
{
    char text[100];
	SSettings* pSettings;
	pSettings = settingsGetPointer();
    const SGasLine * pGasLine;
	uint8_t txtIndex = 0;
	uint8_t	gasId = 0;
	uint8_t textpointer = 0;
	uint8_t line = 0;
	uint8_t oxygen, helium;

	snprintf(text,100,"\032\f\001%c%c",TXT_2BYTE,TXT2BYTE_CaveMode);
	GFX_write_string(&FontT42,&t7cH,text,0);

	if(!pSettings->FlipDisplay)
	{
		t7cY0free.WindowY0 = t7cC.WindowY0 - 10;
	}
	else
	{
		t7cY0free.WindowY1 = 400;
	}
	t7cY0free.WindowLineSpacing = 48+9;
	t7cY0free.WindowNumberOfTextLines = 5; // NUM_GASES == 5
	t7cY0free.WindowTab = 420;

    if(caveMode_isOff())
    {
    	snprintf(text,50,"\030\001%c",TXT_Off);
    	GFX_write_string(&FontT48,&t7cY0free,text,0);
    }
    else
    {
    	txtIndex = snprintf(text,50,"\030\001");
    	if(caveMode_isReturning())
    	{
    		text[txtIndex++] = 'Z';	/* return arrow */
    	}
    	else
    	{
    		text[txtIndex++] = 'r';	/* forward arrow */
    	}
    	if(!caveMode_isActive())
    	{
    		text[txtIndex++] = ' ';
    		text[txtIndex++] = 'p';	/* pause / standby */
    	}
    	text[txtIndex] = 0;
    }
    line = 2;
    GFX_write_string(&Awe48,&t7cY0free,text,1);
    pGasLine = settingsGetPointer()->gas;
	for(gasId=1;gasId<=NUM_GASES;gasId++)
	{
		textpointer = 0;
		if(((pGasLine[gasId].note.ub.active) || (pGasLine[gasId].note.ub.deco)) && (pGasLine[gasId].bottle_size_liter != 0))
		{
			if((stateUsed->lifeData.caveGasNeed_Ltr[gasId] != 0)
					&& (stateUsed->lifeData.caveGasNeed_Ltr[gasId] > pGasLine[gasId].bottle_id_bar * pGasLine[gasId].bottle_size_liter))
			{
				text[textpointer++] = '\025';
				text[textpointer++] = ' ';
				oxygen = pGasLine[gasId].oxygen_percentage;
				helium = pGasLine[gasId].helium_percentage;
				textpointer += write_gas(&text[textpointer], oxygen, helium);
				snprintf(&text[textpointer],100,"\002%ldBar",(stateUsed->lifeData.caveGasNeed_Ltr[gasId] / pGasLine[gasId].bottle_size_liter));
				GFX_write_string(&FontT42, &t7cY0free, text, line);
				line++;
				if(line == 6)
				{
					break;
				}
			}
		}
	}
	snprintf(text,100," TTS:\002%ld min ",(uint32_t)(caveMode_GetTTS() / 60));
	GFX_write_string(&FontT42, &t7cY0free, text, 6);

}
#endif


