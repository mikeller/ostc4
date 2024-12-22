///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditSystem.c
/// \brief  Main Template file for Menu Edit System settings
/// \author heinrichs weikamp gmbh
/// \date   05-Aug-2014
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
#include "tMenuEditSystem.h"

#include "data_exchange_main.h"
#include "externLogbookFlash.h"
#include "gfx_fonts.h"
#include "ostc.h"
#include "settings.h" // for getLicence()
#include "tHome.h"  // for enum CUSTOMVIEWS and init_t7_compass()
#include "tMenu.h"
#include "tMenuEdit.h"
#include "tMenuSystem.h"
#include "tMenuEditCustom.h"
#include "motion.h"
#include "t7.h"


/*#define HAVE_DEBUG_VIEW */
static uint8_t infoPage = 0;

/* Private function prototypes -----------------------------------------------*/
void openEdit_DateTime(void);
void openEdit_DateFormat(void);
void openEdit_Language(void);
void openEdit_Design(void);
void openEdit_Information(void);
void openEdit_Reset(void);
void openEdit_Maintenance(void);
//void openEdit_ShowDebugInfo(void);
//void openEdit_Salinity(void);

/* Announced function prototypes -----------------------------------------------*/
uint8_t OnAction_Date					(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Time					(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_12HR				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Format				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DDMMYY				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_MMDDYY				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_YYMMDD				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DST					(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_UTC				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_SetGnss			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_English			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_German				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_French				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Italian			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Espanol			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
//uint8_t OnAction_Design_t7		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
//uint8_t OnAction_Design_t7ft	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
//uint8_t OnAction_Design_t3		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

uint8_t OnAction_Units				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Colorscheme	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_DebugInfo		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

uint8_t OnAction_Exit					(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Confirm			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Maintenance			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_RebootRTE				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_ResetDeco		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_ResetAll			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_ResetLogbook	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_RebootMainCPU		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_Nothing			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_LogbookOffset(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_SetFactoryDefaults(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_SetBatteryCharge(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_AdjustSurfacePressure		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
#ifdef ENABLE_ANALYSE_SAMPLES
uint8_t OnAction_RecoverSampleIdx(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
#endif
#ifdef SCREENTEST
uint8_t OnAction_ScreenTest		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
#endif
uint8_t OnAction_Information	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
/*
uint8_t OnAction_Salinity			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
uint8_t OnAction_TestCLog			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
*/

/* Exported functions --------------------------------------------------------*/

static uint8_t OnAction_Timer(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    uint8_t digitContentNew;
    switch (action) {
    case ACTION_BUTTON_ENTER:

        return digitContent;
    case ACTION_BUTTON_ENTER_FINAL:
        {
            uint32_t timerM;
            uint32_t timerS;
            evaluateNewString(editId, &timerM, &timerS, 0, 0);
            if (timerM > 9) {
                timerM = 9;
            }
            if (timerS > 59) {
                timerS = 59;
            }

            uint16_t timerDurationS = 60 * timerM + timerS;

            if (timerDurationS < 1) {
                timerDurationS = 1;
            }

            if (timerDurationS != settings->timerDurationS) {
                settings->timerDurationS = timerDurationS;

                disableTimer();

                tMenuEdit_newInput(editId, settings->timerDurationS / 60, settings->timerDurationS % 60, 0, 0);
            }

            return EXIT_TO_MENU;
        }
    case ACTION_BUTTON_NEXT:
        digitContentNew = digitContent + 1;
        if ((blockNumber == 1 && digitNumber == 0 && digitContentNew > '5') || digitContentNew > '9') {
            digitContentNew = '0';
        }

        return digitContentNew;
    case ACTION_BUTTON_BACK:
        digitContentNew = digitContent - 1;
        if (digitContentNew < '0') {
            if (blockNumber == 1 && digitNumber == 0) {
                digitContentNew = '5';
            } else {
                digitContentNew = '9';
            }
        }

        return digitContentNew;
    }

    return EXIT_TO_MENU;
}


static void openEdit_Timer(void)
{
    SSettings *settings = settingsGetPointer();

    char text[32];
    snprintf(text, 32, "\001%c%c", TXT_2BYTE, TXT2BYTE_Timer);
    write_topline(text);

    uint16_t yPos = ME_Y_LINE_BASE + get_globalState_Menu_Line() * ME_Y_LINE_STEP;
    snprintf(text, 32, "%c%c", TXT_2BYTE, TXT2BYTE_Timer);
    write_label_var(30, 299, yPos, &FontT48, text);
    write_field_udigit(StMSYS_Timer, 300, 392, yPos, &FontT48, "#:##", settings->timerDurationS / 60, settings->timerDurationS % 60, 0, 0);
    write_label_var(393, 800, yPos, &FontT48, "\016\016 [m:ss]\017");

    write_buttonTextline(TXT2BYTE_ButtonMinus, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonPlus);

    setEvent(StMSYS_Timer, (uint32_t)OnAction_Timer);
    startEdit();
}


void openEdit_System(uint8_t line)
{
    set_globalState_Menu_Line(line);
    resetMenuEdit(CLUT_MenuPageSystem);

    if(actual_menu_content == MENU_SURFACE)
    {
        switch(line)
        {
        case 1:
        default:
            openEdit_DateTime();
        break;
        case 2:
            openEdit_Timer();
        break;
        case 3:
            openEdit_Language();
        break;
        case 4:
            openEdit_Design();
        break;
        case 5:
            openEdit_Information();
        break;
        case 6:
            openEdit_Reset();
        break;
/*
        case 3:
            openEdit_DecoFutureTTS();
        break;
        case 4:
            openEdit_DecoLastStop();
        break;
*/
        }
    }
    else
    {
        openEdit_CustomviewDivemodeMenu(line);
    }

}

/* Private functions ---------------------------------------------------------*/


void refresh_DateTime()
{
    RTC_DateTypeDef Sdate;
    RTC_TimeTypeDef Stime;
    uint8_t day,month,year,hour,minute;
    char text[32];
    char formatStr[20];
    SSettings *pSettings;
    const SFirmwareData *pFirmwareInfo;
#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
    uint8_t localHours = 0;
    uint8_t localMinutes = 0;
#endif
    pFirmwareInfo = firmwareDataGetPointer();
    const SDiveState * pStateReal = stateRealGetPointer();

    pSettings = settingsGetPointer();

    translateDate(pStateReal->lifeData.dateBinaryFormat, &Sdate);
    translateTime(pStateReal->lifeData.timeBinaryFormat, &Stime);
    year = Sdate.Year;
    month = Sdate.Month;
    day = Sdate.Date;
    hour = Stime.Hours;
    minute= Stime.Minutes;

    if(year < pFirmwareInfo->release_year)
        year = pFirmwareInfo->release_year;

    if(month < 1)
        month = 1;

    if(day < 1)
        day = 1;

    getStringOfFormat_DDMMYY(formatStr, 20);

    text[0] = '\001';
    text[1] = TXT_DateAndTime;
    text[2] = 0;

    write_topline(text);

    write_label_fix(  20, 340, ME_Y_LINE1, &FontT42, TXT_TimeConfig);
    write_label_fix(  20, 340, ME_Y_LINE2, &FontT42, TXT_Format);
    write_label_fix(  20, 340, ME_Y_LINE3, &FontT42, TXT_DateConfig);
    write_label_fix(  20, 790, ME_Y_LINE4, &FontT42, TXT_Format);
#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
    write_label_var(  20, 340, ME_Y_LINE5, &FontT42, "GNSS");
    snprintf(text, 32, "%c%c", TXT_2BYTE, TXT2BYTE_TIMEZONE);
    write_label_var(  20, 340, ME_Y_LINE6, &FontT42, text);
#endif


    tMenuEdit_newInput(StMSYS1_Time, hour, minute, 0, 0);
    tMenuEdit_set_on_off(StMSYS1_12HR, pSettings->amPMTime);

    switch(pSettings->date_format)
    {
    	default:
    	case DDMMYY:  tMenuEdit_newInput(StMSYS1_Date, day, month, year, 0);
    		break;
    	case MMDDYY:  tMenuEdit_newInput(StMSYS1_Date, month, day, year, 0);
    		break;
    	case YYMMDD:  tMenuEdit_newInput(StMSYS1_Date, year, month, day, 0);
    		break;
    }
    tMenuEdit_newButtonText(StMSYS1_FORMAT, formatStr);

#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
    if(pStateReal->lifeData.gnssData.alive & GNSS_ALIVE_STATE_TIME)
    {
        convertUTCToLocal(pStateReal->lifeData.gnssData.DateTime.hour, pStateReal->lifeData.gnssData.DateTime.min, &localHours, &localMinutes);
        convertStringOfDate_DDMMYY(formatStr, 20, pStateReal->lifeData.gnssData.DateTime.day
        										, pStateReal->lifeData.gnssData.DateTime.month
    											, pStateReal->lifeData.gnssData.DateTime.year);
        snprintf(text, 32, "%02d:%02d - %s", localHours, localMinutes, formatStr);
        tMenuEdit_newButtonText(StMSYS1_GNSSDT, text);
    }
    else
    {
    	snprintf(text, 32, "--:--");
    	write_label_var(  320, 790, ME_Y_LINE5, &FontT42, text);
    }
    tMenuEdit_newInput(StMSYS1_ZONE, pSettings->timeZone.hours, pSettings->timeZone.minutes, 0, 0);
#endif
    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}
void openEdit_DateTime(void)
{
    RTC_DateTypeDef Sdate;
    RTC_TimeTypeDef Stime;
    uint8_t day,month,year,hour,minute;
    char text[32];
    char formatStr[20];
    SSettings *pSettings;
    const SFirmwareData *pFirmwareInfo;

    pFirmwareInfo = firmwareDataGetPointer();
    const SDiveState * pStateReal = stateRealGetPointer();

    pSettings = settingsGetPointer();

    set_globalState(StMSYS1_DateTime);
	resetMenuEdit(CLUT_MenuPageSystem);

    translateDate(pStateReal->lifeData.dateBinaryFormat, &Sdate);
    translateTime(pStateReal->lifeData.timeBinaryFormat, &Stime);
    year = Sdate.Year;
    month = Sdate.Month;
    day = Sdate.Date;
    hour = Stime.Hours;
    minute= Stime.Minutes;

    if(year < pFirmwareInfo->release_year)
        year = pFirmwareInfo->release_year;

    if(month < 1)
        month = 1;

    if(day < 1)
        day = 1;

    getStringOfFormat_DDMMYY(formatStr, 20);

    text[0] = '\001';
    text[1] = TXT_DateAndTime;
    text[2] = 0;

    write_topline(text);

    write_label_fix(  20, 340, ME_Y_LINE1, &FontT42, TXT_TimeConfig);
    write_label_fix(  20, 340, ME_Y_LINE2, &FontT42, TXT_Format);
    write_label_fix(  20, 340, ME_Y_LINE3, &FontT42, TXT_DateConfig);
    write_label_fix(  20, 790, ME_Y_LINE4, &FontT42, TXT_Format);
#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
    write_label_var(  20, 340, ME_Y_LINE5, &FontT42, "GNSS");
    snprintf(text, 32, "%c%c", TXT_2BYTE, TXT2BYTE_TIMEZONE);
    write_label_var(  20, 340, ME_Y_LINE6, &FontT42, text);
#endif

    write_field_2digit(StMSYS1_Time,		320, 780, ME_Y_LINE1,  &FontT48, "##:##", (uint32_t)hour, (uint32_t)minute, 0, 0);
    write_field_on_off(StMSYS1_12HR,		320, 790, ME_Y_LINE2,  &FontT48, "12 HR", pSettings->amPMTime);

    switch(pSettings->date_format)
    {
    	default:
    	case DDMMYY:  write_field_2digit(StMSYS1_Date,		320, 780, ME_Y_LINE3,  &FontT48, "##-##-20##", (uint32_t)day, (uint32_t)month, (uint32_t)year, 0);
    		break;
    	case MMDDYY:  write_field_2digit(StMSYS1_Date,		320, 780, ME_Y_LINE3,  &FontT48, "##-##-20##", (uint32_t)month, (uint32_t)day, (uint32_t)year, 0);
    		break;
    	case YYMMDD:  write_field_2digit(StMSYS1_Date,		320, 780, ME_Y_LINE3,  &FontT48, "20##-##-##", (uint32_t)year, (uint32_t)month, (uint32_t)day, 0);
    		break;
    }

    write_field_button(StMSYS1_FORMAT, 320, 790, ME_Y_LINE4,  &FontT48, formatStr);

#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
	snprintf(text, 32, "--:--");
	write_field_button(StMSYS1_GNSSDT, 320, 790, ME_Y_LINE5,  &FontT48, text);
    write_field_sdigit(StMSYS1_ZONE, 320, 780, ME_Y_LINE6,  &FontT48, "UTC: ###:###", pSettings->timeZone.hours, pSettings->timeZone.minutes,0,0);
#endif

    setEvent(StMSYS1_Date, 		(uint32_t)OnAction_Date);
    setEvent(StMSYS1_Time, 		(uint32_t)OnAction_Time);
    setEvent(StMSYS1_12HR,      (uint32_t)OnAction_12HR);
    setEvent(StMSYS1_FORMAT,	(uint32_t)OnAction_Format);
#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
   	setEvent(StMSYS1_GNSSDT, (uint32_t)OnAction_SetGnss);
	setEvent(StMSYS1_ZONE,		(uint32_t)OnAction_UTC);
#endif
    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}


uint8_t OnAction_Date(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newDay, newMonth, newYear;
    RTC_DateTypeDef sdatestructure;

    const SFirmwareData *pFirmwareInfo;
    pFirmwareInfo = firmwareDataGetPointer();

    uint8_t mapDMY[3];
    switch(settingsGetPointer()->date_format)
    {
    	default:
    	case DDMMYY: mapDMY[0] = 0;
    				 mapDMY[1] = 1;
    				 mapDMY[2] = 2;
    		break;
    	case MMDDYY: mapDMY[0] = 1;
		 	 	 	 mapDMY[1] = 0;
		 	 	 	 mapDMY[2] = 2;
    		break;
    	case YYMMDD: mapDMY[0] = 2;
 	 	 	 	 	 mapDMY[1] = 1;
 	 	 	 	 	 mapDMY[2] = 0;
    	break;
    }

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
    	switch(settingsGetPointer()->date_format)
    	    {
    	    	default:
    	    	case DDMMYY: evaluateNewString(editId, &newDay, &newMonth, &newYear, 0);
    	    		break;
    	    	case MMDDYY: evaluateNewString(editId, &newMonth, &newDay, &newYear, 0);
    	    		break;
    	    	case YYMMDD: evaluateNewString(editId, &newYear, &newMonth, &newDay, 0);
    	    		break;
    	    }

        if(newDay == 0)
            newDay = 1;
        if(newDay > 31)
            newDay = 31;
        if(newMonth == 0)
            newMonth = 1;
        if(newMonth > 12)
            newMonth = 12;
        if((newMonth == 2) && (newDay > 29))
            newDay = 29;
        if((newDay > 30) && ((newMonth == 4) ||(newMonth == 6) ||(newMonth == 9) ||(newMonth == 11)))
            newDay = 30;
        if(newYear < 17)
            newYear = 17;
        if(newYear > 99)
            newYear = 99;

        sdatestructure.Date = newDay;
        sdatestructure.Month = newMonth;
        sdatestructure.Year = newYear;
        setWeekday(&sdatestructure);

        setDate(sdatestructure);

        switch(settingsGetPointer()->date_format)
        {
           	default:
           	case DDMMYY: tMenuEdit_newInput(editId, newDay, newMonth, newYear, 0);
          		break;
           	case MMDDYY: tMenuEdit_newInput(editId, newMonth, newDay, newYear, 0);
           		break;
           	case YYMMDD: tMenuEdit_newInput(editId, newYear, newMonth, newDay, 0);
           		break;
        }

        return UNSPECIFIC_RETURN;
    }
    if(action == ACTION_BUTTON_NEXT)		/* clip values to a specific range e.g. 12 months */
    {
        digitContentNew = digitContent + 1;

        if((blockNumber == mapDMY[0]) && (digitContentNew > '0' + 31))
        {
            digitContentNew = '1';
        }
        if((blockNumber == mapDMY[1]) && (digitContentNew > '0' + 12))
        {
            digitContentNew = '1';
        }
        if((blockNumber == mapDMY[2]) && (digitContentNew > '0' + pFirmwareInfo->release_year + 10))
        {
            digitContentNew = '0' + pFirmwareInfo->release_year;
        }
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)		/* clip values to a specific range e.g. 12 months */
    {
        digitContentNew = digitContent - 1;
        if((blockNumber == mapDMY[0]) && (digitContentNew < '1'))
        {
            digitContentNew = '0' + 31;
        }
        if((blockNumber ==  mapDMY[1]) && (digitContentNew < '1'))
        {
            digitContentNew = '0' + 12;
        }
        if((blockNumber ==  mapDMY[2]) && (digitContentNew < '0' + pFirmwareInfo->release_year))
        {
            digitContentNew = '0' + pFirmwareInfo->release_year + 10;
        }
        return digitContentNew;
    }
/*
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if((blockNumber == 2) && (digitNumber == 0) && (digitContentNew > '2'))
            digitContentNew = '1';
        if((blockNumber == 0) && (digitNumber == 0) && (digitContentNew > '3'))
            digitContentNew = '0';
        if((blockNumber == 1) && (digitNumber == 0) && (digitContentNew > '1'))
            digitContentNew = '0';
        if(digitContentNew > '9')
            digitContentNew = '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if((blockNumber == 2) && (digitNumber == 0) && (digitContentNew < '1'))
            digitContentNew = '2';
        if((blockNumber == 0) && (digitNumber == 0) && (digitContentNew < '0'))
            digitContentNew = '3';
        if((blockNumber == 1) && (digitNumber == 0) && (digitContentNew < '0'))
            digitContentNew = '1';
        if(digitContentNew < '0')
            digitContentNew = '9';
        return digitContentNew;
    }
*/
    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_Time(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newHour, newMinute;
    RTC_TimeTypeDef stimestructure;

    if(action == ACTION_BUTTON_ENTER)
    {
        return digitContent;
    }
    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newHour, &newMinute, 0, 0);
        if(newHour > 23)
            newHour = 23;
        if(newMinute > 59)
            newMinute = 59;

        stimestructure.Hours = newHour;
        stimestructure.Minutes = newMinute;
        stimestructure.Seconds = 0;

        setTime(stimestructure);

        tMenuEdit_newInput(editId, newHour, newMinute, 0, 0);
        return UNSPECIFIC_RETURN;
    }
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if((blockNumber == 0) && (digitContentNew > '0' + 23))
            digitContentNew = '0';
        if((blockNumber == 1) && (digitContentNew > '0' + 59))
            digitContentNew = '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if((blockNumber == 0) && (digitContentNew < '0'))
            digitContentNew = '0' + 23;
        if((blockNumber == 1) && (digitContentNew < '0'))
            digitContentNew = '0' + 59;
        return digitContentNew;
    }
/*
    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if((blockNumber == 0) && (digitNumber == 0) && (digitContentNew > '2'))
            digitContentNew = '0';
        if((blockNumber == 1) && (digitNumber == 0) && (digitContentNew > '5'))
            digitContentNew = '0';
        if(digitContentNew > '9')
            digitContentNew = '0';
        return digitContentNew;
    }
    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if((blockNumber == 0) && (digitNumber == 0) && (digitContentNew < '0'))
            digitContentNew = '2';
        if((blockNumber == 1) && (digitNumber == 0) && (digitContentNew < '0'))
            digitContentNew = '5';
        if(digitContentNew < '0')
            digitContentNew = '9';
        return digitContentNew;
    }
*/
    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_12HR(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;

    pSettings = settingsGetPointer();
    pSettings->amPMTime = !(pSettings->amPMTime);

    tMenuEdit_set_on_off(editId, pSettings->amPMTime);

    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_SetGnss(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;
    uint8_t localHours = 0;
    uint8_t localMinutes = 0;

    const SDiveState * pStateReal = stateRealGetPointer();

    if(pStateReal->lifeData.gnssData.alive & GNSS_ALIVE_STATE_TIME)
    {
        convertUTCToLocal(pStateReal->lifeData.gnssData.DateTime.hour, pStateReal->lifeData.gnssData.DateTime.min, &localHours, &localMinutes);
        stimestructure.Hours = localHours;
        stimestructure.Minutes = localMinutes;
        stimestructure.Seconds = 0;
        setTime(stimestructure);

        sdatestructure.Date = pStateReal->lifeData.gnssData.DateTime.day;
        sdatestructure.Month = pStateReal->lifeData.gnssData.DateTime.month;
        sdatestructure.Year = pStateReal->lifeData.gnssData.DateTime.year;
        setWeekday(&sdatestructure);
        setDate(sdatestructure);
    }
    return UNSPECIFIC_RETURN;
}

void openEdit_DateFormat(void)
{
    char text[32];
    SSettings *pSettings;

    uint8_t ddmmyy = 0;
    uint8_t mmddyy= 0;
    uint8_t yymmdd = 0;

    pSettings = settingsGetPointer();


    set_globalState(StMSYS1_FORMAT);
	resetMenuEdit(CLUT_MenuPageSystem);
	setBackMenu((uint32_t)openEdit_DateTime,0,4);

    switch(pSettings->date_format)
    {
    	default:
    	case DDMMYY: ddmmyy = 1;
    		break;
    	case MMDDYY: mmddyy = 1;
    		break;
    	case YYMMDD: yymmdd = 1;
    	    break;
    };

    text[0] = '\001';
    text[1] = TXT_Format;
    text[2] = 0;

    write_topline(text);

    write_label_fix(  20, 790, ME_Y_LINE2, &FontT42, TXT_Format);

    write_field_on_off(StMSYS1_DDMMYY,  320, 790, ME_Y_LINE1,  &FontT48, "DDMMYY", ddmmyy);
    write_field_on_off(StMSYS1_MMDDYY,	320, 790, ME_Y_LINE2,  &FontT48, "MMDDYY", mmddyy);
    write_field_on_off(StMSYS1_YYMMDD,	320, 790, ME_Y_LINE3,  &FontT48, "YYMMDD", yymmdd);

    setEvent(StMSYS1_DDMMYY,	(uint32_t)OnAction_DDMMYY);
    setEvent(StMSYS1_MMDDYY,	(uint32_t)OnAction_MMDDYY);
    setEvent(StMSYS1_YYMMDD,	(uint32_t)OnAction_YYMMDD);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}

uint8_t OnAction_Format(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	openEdit_DateFormat();

    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_DDMMYY(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;

    pSettings = settingsGetPointer();
    pSettings->date_format = DDMMYY;

    tMenuEdit_set_on_off(editId, 1);
    tMenuEdit_set_on_off(StMSYS1_MMDDYY, 0);
    tMenuEdit_set_on_off(StMSYS1_YYMMDD, 0);

    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_MMDDYY(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;

    pSettings = settingsGetPointer();
    pSettings->date_format = MMDDYY;

    tMenuEdit_set_on_off(editId, 1);
    tMenuEdit_set_on_off(StMSYS1_DDMMYY, 0);
    tMenuEdit_set_on_off(StMSYS1_YYMMDD, 0);

    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_YYMMDD(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings;

    pSettings = settingsGetPointer();
    pSettings->date_format = YYMMDD;

    tMenuEdit_set_on_off(editId, 1);
    tMenuEdit_set_on_off(StMSYS1_MMDDYY, 0);
    tMenuEdit_set_on_off(StMSYS1_DDMMYY, 0);

    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_UTC(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *settings = settingsGetPointer();
    const SDiveState * pStateReal = stateRealGetPointer();
    int32_t utcHour;
    uint32_t utcMinutes;
    uint8_t digitContentNew;
    uint8_t localHours = 0;
    uint8_t localMinutes = 0;
    char text[32];
    char timeStr[20];

    switch (action) {
    case ACTION_BUTTON_ENTER:

        return digitContent;
    case ACTION_BUTTON_ENTER_FINAL:
        {

            evaluateNewString(editId, (uint32_t *)&utcHour, &utcMinutes, NULL, NULL);

            if (utcHour > 14) {
            	utcHour = 14;
            } else if (utcHour < -12) {
            	utcHour = -12;
            }

            if (utcMinutes % 15 != 0)
            {
            	utcMinutes = (utcMinutes / 15) * 15;
            }
            if(utcMinutes > 45)
            {
            	utcMinutes = 45;
            } else if (utcMinutes < 0) {
            	utcMinutes = 0;
            }
            settings->timeZone.hours = utcHour;
            settings->timeZone.minutes = utcMinutes;

            tMenuEdit_newInput(editId, ((input_u)utcHour).uint32, utcMinutes, 0, 0);
            convertUTCToLocal(pStateReal->lifeData.gnssData.DateTime.hour, pStateReal->lifeData.gnssData.DateTime.min, &localHours, &localMinutes);
            convertStringOfDate_DDMMYY(timeStr, 20, pStateReal->lifeData.gnssData.DateTime.day
            										, pStateReal->lifeData.gnssData.DateTime.month
        											, pStateReal->lifeData.gnssData.DateTime.year);
            snprintf(text, 32, "%2d:%2d - %s", localHours, localMinutes, timeStr);
            tMenuEdit_newButtonText(StMSYS1_GNSSDT, text);
        }

        break;
    case ACTION_BUTTON_NEXT:
        if ((blockNumber == 0) && (digitNumber == 0)) {
            digitContentNew = togglePlusMinus(digitContent);
        } else {
            digitContentNew = digitContent + 1;
            if (digitContentNew > '9') {
                digitContentNew = '0';
            }
        }

        return digitContentNew;
    case ACTION_BUTTON_BACK:
    	if ((blockNumber == 0) && (digitNumber == 0)) {
            digitContentNew = togglePlusMinus(digitContent);
        } else {
            digitContentNew = digitContent - 1;
            if (digitContentNew < '0') {
                digitContentNew = '9';
            }
        }
        return digitContentNew;
    }
    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_DST(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    RTC_TimeTypeDef stimestructure;
    uint8_t newDST;

    get_RTC_DateTime(0, &stimestructure);
    newDST = stimestructure.DayLightSaving;
    if(newDST)
        newDST = 0;
    else
        newDST = 1;
    stimestructure.DayLightSaving = newDST;
    set_RTC_DateTime(0, &stimestructure);

    tMenuEdit_set_on_off(editId, newDST);

    return UNSPECIFIC_RETURN;
}


void openEdit_Language(void)
{
    char text[32];
    uint8_t actualLanguage, active;
    SSettings *pSettings;

    pSettings = settingsGetPointer();
    actualLanguage = pSettings->selected_language;

    text[0] = '\001';
    text[1] = TXT_Language;
    text[2] = 0;
    write_topline(text);

    text[0] = TXT_LanguageName;
    text[1] = 0;

    pSettings->selected_language = LANGUAGE_English;
    if(	actualLanguage == pSettings->selected_language)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMSYS2_English,			 30, 500, ME_Y_LINE1,  &FontT48, text, active);

    pSettings->selected_language = LANGUAGE_German;
    if(	actualLanguage == pSettings->selected_language)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMSYS2_German,			 30, 800, ME_Y_LINE2,  &FontT48, text, active);

    pSettings->selected_language = LANGUAGE_French;
    if(	actualLanguage == pSettings->selected_language)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMSYS2_French,			 30, 800, ME_Y_LINE3,  &FontT48, text, active);


    pSettings->selected_language = LANGUAGE_Italian;
    if(	actualLanguage == pSettings->selected_language)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMSYS2_Italian,			 30, 800, ME_Y_LINE4,  &FontT48, text, active);


    pSettings->selected_language = LANGUAGE_Espanol;
    if(	actualLanguage == pSettings->selected_language)
        active = 1;
    else
        active = 0;
    write_field_on_off(StMSYS2_Espanol,			 30, 800, ME_Y_LINE5,  &FontT48, text, active);

    pSettings->selected_language = actualLanguage;

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);

    setEvent(StMSYS2_English, (uint32_t)OnAction_English);
    setEvent(StMSYS2_German, 	(uint32_t)OnAction_German);
    setEvent(StMSYS2_French,	(uint32_t)OnAction_French);
    setEvent(StMSYS2_Italian,	(uint32_t)OnAction_Italian);
    setEvent(StMSYS2_Espanol,	(uint32_t)OnAction_Espanol);
}


uint8_t OnAction_English			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    pSettings->selected_language = LANGUAGE_English;
    return EXIT_TO_MENU_WITH_LOGO;
}


uint8_t OnAction_German				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    pSettings->selected_language = LANGUAGE_German;
    return EXIT_TO_MENU_WITH_LOGO;
}


uint8_t OnAction_French				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    pSettings->selected_language = LANGUAGE_French;
    return EXIT_TO_MENU_WITH_LOGO;
}


uint8_t OnAction_Italian			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    pSettings->selected_language = LANGUAGE_Italian;
    return EXIT_TO_MENU_WITH_LOGO;
}


uint8_t OnAction_Espanol			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    pSettings->selected_language = LANGUAGE_Espanol;
    return EXIT_TO_MENU_WITH_LOGO;
}


void openEdit_Design(void)
{
    refresh_Design();

    write_field_button(StMSYS3_Units,			400, 700, ME_Y_LINE1,  &FontT48, "");
    write_field_button(StMSYS3_Colors,		400, 700, ME_Y_LINE2,  &FontT48, "");
#ifdef HAVE_DEBUG_VIEW
	write_field_button(StMSYS3_Debug,			400, 700, ME_Y_LINE3,  &FontT48, "");
#endif
    setEvent(StMSYS3_Units,		(uint32_t)OnAction_Units);
    setEvent(StMSYS3_Colors,	(uint32_t)OnAction_Colorscheme);
#ifdef HAVE_DEBUG_VIEW
	setEvent(StMSYS3_Debug,		(uint32_t)OnAction_DebugInfo);
#endif
}


void refresh_Design(void)
{
    char text[32];

    // header
    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Layout;
    text[3] = 0;
    write_topline(text);

    // units
    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_Units;
    text[2] = 0;
    write_label_var(  30, 200, ME_Y_LINE1, &FontT48, text);

    if(settingsGetPointer()->nonMetricalSystem)
    {
        text[1] = TXT2BYTE_Units_feet;
    }
    else
    {
        text[1] = TXT2BYTE_Units_metric;
    }
    write_label_var( 400, 700, ME_Y_LINE1, &FontT48, text);

    // colorscheme
    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_Farbschema;
    text[2] = 0;
    write_label_var(  30, 300, ME_Y_LINE2, &FontT48, text);

    text[0] = '0' + settingsGetPointer()->tX_colorscheme;
    text[1] = 0;
    write_label_var( 400, 700, ME_Y_LINE2, &FontT48, text);

#ifdef HAVE_DEBUG_VIEW
    // specials
    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_ShowDebug;
    text[2] = 0;
    write_label_var(  30, 700, ME_Y_LINE3, &FontT48, text);

    if(settingsGetPointer()->showDebugInfo)
        text[0] = '\005';
    else
        text[0] = '\006';
    text[1] = 0;
    write_label_var( 400, 700, ME_Y_LINE3, &FontT48, text);
#endif

    // design
    text[0] = TXT_Depth;
    text[1] = 0;
    write_content( 30, 700, ME_Y_LINE4, &FontT24, text, CLUT_Font031);
    write_content( 30, 700, ME_Y_LINE4 + 30 + 70, &FontT48, "___________", CLUT_DIVE_FieldSeperatorLines);
    write_content(280, 700, ME_Y_LINE4 + 30 + 70 - 3, &FontT48, "|", CLUT_DIVE_pluginbox);
    write_content(290, 700, ME_Y_LINE4 + 30 + 70 - 37, &FontT48, "_______________", CLUT_DIVE_pluginbox);
    write_content( 30, 700, ME_Y_LINE4 + 42, &FontT144, "24.7", CLUT_Font027);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}


uint8_t OnAction_Units(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    settingsGetPointer()->nonMetricalSystem = !(settingsGetPointer()->nonMetricalSystem);
    return EXIT_TO_MENU_WITH_LOGO;//UPDATE_DIVESETTINGS;
}


uint8_t OnAction_Colorscheme(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t newColorscheme;

    newColorscheme = settingsGetPointer()->tX_colorscheme + 1;

    if(newColorscheme > 3)
        newColorscheme = 0;

    settingsGetPointer()->tX_colorscheme = newColorscheme;
    GFX_use_colorscheme(newColorscheme);
    tHome_init_compass();
    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_DebugInfo(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    settingsGetPointer()->showDebugInfo = !(settingsGetPointer()->showDebugInfo);
    return UPDATE_DIVESETTINGS;
}




/*
uint8_t OnAction_Design_t7ft		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    if((pSettings->design == 7) && pSettings->nonMetricalSystem)
        return EXIT_TO_MENU;
    pSettings->design = 7;
    pSettings->nonMetricalSystem = 1;
    tMenuEdit_set_on_off(StMSYS3_t7,	 0);
    tMenuEdit_set_on_off(StMSYS3_t7ft, 1);
    tMenuEdit_set_on_off(StMSYS3_t3,	 0);
    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_Design_t7			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    if((pSettings->design == 7) && (pSettings->nonMetricalSystem == 0))
        return EXIT_TO_MENU;
    pSettings->design = 7;
    pSettings->nonMetricalSystem = 0;
    tMenuEdit_set_on_off(StMSYS3_t7,	 1);
    tMenuEdit_set_on_off(StMSYS3_t7ft, 0);
    tMenuEdit_set_on_off(StMSYS3_t3,	 0);
    return UPDATE_DIVESETTINGS;
}


uint8_t OnAction_Design_t3			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSettings *pSettings = settingsGetPointer();
    if(pSettings->design == 3)
        return EXIT_TO_MENU;
    pSettings->design = 3;
    pSettings->nonMetricalSystem = 0;
    tMenuEdit_set_on_off(StMSYS3_t7, 	0);
    tMenuEdit_set_on_off(StMSYS3_t7ft,0);
    tMenuEdit_set_on_off(StMSYS3_t3, 	1);
    return UPDATE_DIVESETTINGS;
}
*/



void openEdit_Information(void)
{
    char text[70];

    infoPage = 0;

    text[0] = '\001';
    text[1] = TXT_Information;
    text[2] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_ButtonNext;
    text[2] = 0;

    write_field_button(StMSYS4_Info, 30, 800, ME_Y_LINE6,  &FontT48, text);

    setEvent(StMSYS4_Info, (uint32_t)OnAction_Information);
    tMenuEdit_select(StMSYS4_Info);
}


uint8_t OnAction_Information	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    resetEnterPressedToStateBeforeButtonAction();

    infoPage++;
    if(infoPage > 3)
        return EXIT_TO_MENU;
    else
        return UNSPECIFIC_RETURN;
}


void refresh_InformationPage(void)
{
    char text_header[5];
    char text_button[5];
    char text_content[256];
    uint8_t date[3], year,month,day;

    RTC_DateTypeDef Sdate, Sdate2;
    float temperature1, temperature2, voltage, offsetTemperature;

    //RTC_TimeTypeDef Stime;

    /*
        SDeviceLine batteryChargeCycles;
        SDeviceLine batteryChargeCompleteCycles;
        SDeviceLine temperatureMinimum;
        SDeviceLine temperatureMaximum;
        SDeviceLine depthMaximum;
        SDeviceLine diveCycles;
        SDeviceLine voltageMinimum;
    */

    switch(infoPage)
    {
    case 0:
        text_header[0] = '\001';
        text_header[1] = TXT_Information;
        text_header[2] = 0;

        write_label_var(  20, 800, ME_Y_LINE1, &FontT42, "Dive Computer OSTC4");
        write_label_var(  20, 800, ME_Y_LINE2, &FontT42, "Design heinrichs/weikamp");

        Sdate.Year = firmwareDataGetPointer()->release_year;
        Sdate.Month = firmwareDataGetPointer()->release_month;
        Sdate.Date = firmwareDataGetPointer()->release_day;

        if(settingsGetPointer()->date_format == DDMMYY)
        {
            day = 0;
            month = 1;
            year = 2;
        }
        else
        if(settingsGetPointer()->date_format == MMDDYY)
        {
            day = 1;
            month = 0;
            year = 2;
        }
        else
        {
            day = 2;
            month = 1;
            year = 0;
        }
        date[day] = Sdate.Date;
        date[month] = Sdate.Month;
        date[year] = Sdate.Year;
        snprintf(text_content,40,"Firmware release date:  %02d.%02d.%02d",date[0],date[1],date[2]);
        write_label_var(  20, 800, ME_Y_LINE3, &FontT42, text_content);
        write_label_var(  20, 800, ME_Y_LINE4, &FontT42, "for more information");
        write_label_var(  20, 800, ME_Y_LINE5, &FontT42, "info@heinrichsweikamp.com");

        text_button[0] = TXT_2BYTE;
        text_button[1] = TXT2BYTE_ButtonNext;
        text_button[2] = 0;
        break;

    case 1:
        text_header[0] = '\001';
        text_header[1] = TXT_2BYTE;
        text_header[2] = TXT2BYTE_Usage_Battery;
        text_header[3] = 0;

        text_content[0] = TXT_2BYTE;
        text_content[1] = TXT2BYTE_ChargeCycles;
        text_content[2] = 0;
        write_label_var(  20, 800, ME_Y_LINE1, &FontT42, text_content);
        /*      snprintf(text_content,80,"%ld (%ld)",stateDeviceGetPointer()->batteryChargeCycles.value_int32,stateDeviceGetPointer()->batteryChargeCompleteCycles.value_int32);
       write_label_var(  20, 800, ME_Y_LINE2, &FontT42, text_content); */

        translateDate(stateDeviceGetPointer()->batteryChargeCycles.date_rtc_dr, &Sdate);
        translateDate(stateDeviceGetPointer()->batteryChargeCompleteCycles.date_rtc_dr, &Sdate2);
        snprintf(text_content,80,"%u.%u.20%02u (%u.%u.20%02u)",Sdate.Date,Sdate.Month,Sdate.Year, Sdate2.Date,Sdate2.Month,Sdate2.Year);
        write_label_var(  20, 800, ME_Y_LINE2, &FontT42, text_content);

        text_content[0] = TXT_2BYTE;
        text_content[1] = TXT2BYTE_LowestVoltage;
        text_content[2] = 0;
        write_label_var(  20, 800, ME_Y_LINE4, &FontT42, text_content);

        translateDate(stateDeviceGetPointer()->voltageMinimum.date_rtc_dr, &Sdate);
        voltage = ((float)stateDeviceGetPointer()->voltageMinimum.value_int32) / 1000;
        snprintf(text_content,80,"%0.3fV (%u.%u.20%02u)",voltage, Sdate.Date,Sdate.Month,Sdate.Year);
        write_label_var(  20, 800, ME_Y_LINE5, &FontT42, text_content);

        text_button[0] = TXT_2BYTE;
        text_button[1] = TXT2BYTE_ButtonNext;
        text_button[2] = 0;
    break;

    case 2:
        text_header[0] = '\001';
        text_header[1] = TXT_2BYTE;
        text_header[2] = TXT2BYTE_Usage_Dives;
        text_header[3] = 0;

        text_content[0] = TXT_2BYTE;
        text_content[1] = TXT2BYTE_NumberOfDives;
        text_content[2] = 0;
        write_label_var(  20, 800, ME_Y_LINE1, &FontT42, text_content);

        snprintf(text_content,80,"%ld (%ld)",stateDeviceGetPointer()->diveCycles.value_int32,(stateDeviceGetPointer()->depthMaximum.value_int32 - 1000) / 100);
        write_label_var(  20, 800, ME_Y_LINE2, &FontT42, text_content);

        translateDate(stateDeviceGetPointer()->diveCycles.date_rtc_dr, &Sdate);
        translateDate(stateDeviceGetPointer()->depthMaximum.date_rtc_dr, &Sdate2);
        snprintf(text_content,80,"%u.%u.20%02u (%u.%u.20%02u)",Sdate.Date,Sdate.Month,Sdate.Year, Sdate2.Date,Sdate2.Month,Sdate2.Year);
        write_label_var(  20, 800, ME_Y_LINE3, &FontT42, text_content);

        text_content[0] = TXT_2BYTE;
        text_content[1] = TXT2BYTE_HoursOfOperation;
        text_content[2] = 0;
        write_label_var(  20, 800, ME_Y_LINE4, &FontT42, text_content);

        snprintf(text_content,80,"%ld",(stateDeviceGetPointer()->hoursOfOperation.value_int32)/3600);
        write_label_var(  20, 800, ME_Y_LINE5, &FontT42, text_content);

        text_button[0] = TXT_2BYTE;
        text_button[1] = TXT2BYTE_ButtonNext;
        text_button[2] = 0;
    break;

    case 3:
        text_header[0] = '\001';
        text_header[1] = TXT_2BYTE;
        text_header[2] = TXT2BYTE_Usage_Environment;
        text_header[3] = 0;

        text_content[0] = TXT_2BYTE;
        text_content[1] = TXT2BYTE_AmbientTemperature;
        text_content[2] = 0;
        write_label_var(  20, 800, ME_Y_LINE1, &FontT42, text_content);

        temperature1 = ((float)stateDeviceGetPointer()->temperatureMinimum.value_int32) / 100;
        temperature2 = ((float)stateDeviceGetPointer()->temperatureMaximum.value_int32) / 100;
        snprintf(text_content,80,"%0.2f\140C  /  %0.2f\140C",temperature1,temperature2);
        write_label_var(  20, 800, ME_Y_LINE2, &FontT42, text_content);

        translateDate(stateDeviceGetPointer()->temperatureMinimum.date_rtc_dr, &Sdate);
        translateDate(stateDeviceGetPointer()->temperatureMaximum.date_rtc_dr, &Sdate2);
        snprintf(text_content,80,"(%u.%u.20%02u  /  %u.%u.20%02u)",Sdate.Date,Sdate.Month,Sdate.Year, Sdate2.Date,Sdate2.Month,Sdate2.Year);
        write_label_var(  20, 800, ME_Y_LINE3, &FontT42, text_content);

        text_content[0] = TXT_2BYTE;
        text_content[1] = TXT2BYTE_Korrekturwerte;
        text_content[2] = 0;
        write_label_var(  20, 800, ME_Y_LINE4, &FontT42, text_content);

        offsetTemperature = ((float)settingsGetPointer()->offsetTemperature_centigrad) / 10;
        snprintf(text_content,80,"%i %s  /  %0.2f\140C",settingsGetPointer()->offsetPressure_mbar, TEXT_PRESSURE_UNIT, offsetTemperature);
        write_label_var(  20, 800, ME_Y_LINE5, &FontT42, text_content);

        text_button[0] = TXT_2BYTE;
        text_button[1] = TXT2BYTE_Exit;
        text_button[2] = 0;
    break;
    }

    write_topline(text_header);
    tMenuEdit_newButtonText(StMSYS4_Info, text_button);
    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonNext,0);
}


void openEdit_Reset(void)
{
    char text[32];

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_ResetMenu;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_LogbookOffset;
    text[7] = 0;

    write_label_var(  30, 400, ME_Y_LINE1, &FontT48, text);

    write_field_udigit(StMSYS5_LogbookOffset,420, 800, ME_Y_LINE1, &FontT48, "####", settingsGetPointer()->logbookOffset,0,0,0);

    text[0] = TXT_2BYTE;
    text[2] = 0;

    text[1] = TXT2BYTE_ResetAll;
    write_field_button(StMSYS5_ResetAll,		30, 800, ME_Y_LINE2,  &FontT48, text);

    text[1] = TXT2BYTE_ResetDeco;
    write_field_button(StMSYS5_ResetDeco,		30, 800, ME_Y_LINE3,  &FontT48, text);

    text[1] = TXT2BYTE_Reboot;
    write_field_button(StMSYS5_Reboot,			30, 800, ME_Y_LINE4,  &FontT48, text);

    text[1] = TXT2BYTE_Maintenance;
    write_field_button(StMSYS5_Maintenance,	30, 800, ME_Y_LINE5,  &FontT48, text);

#ifndef RESETLOGBLOCK
    text[1] = TXT2BYTE_ResetLogbook;
    write_field_button(StMSYS5_ResetLogbook,30, 800, ME_Y_LINE6,  &FontT48, text);
#else
    text[0] = '\031';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_ResetLogbook;
    text[3] = 0;
    write_field_button(StMSYS5_ResetLogbook,30, 800, ME_Y_LINE6,  &FontT48, text);
    text[0] = TXT_2BYTE;
    text[2] = 0;
#endif

    setEvent(StMSYS5_LogbookOffset,	(uint32_t)OnAction_LogbookOffset);
    setEvent(StMSYS5_ResetAll, 			(uint32_t)OnAction_Confirm);
    setEvent(StMSYS5_ResetDeco, 		(uint32_t)OnAction_Confirm);
    setEvent(StMSYS5_Reboot, 				(uint32_t)OnAction_Confirm);
    setEvent(StMSYS5_Maintenance,		(uint32_t)OnAction_Maintenance);
#ifndef RESETLOGBLOCK
    setEvent(StMSYS5_ResetLogbook,	(uint32_t)OnAction_Confirm);
#else
    setEvent(StMSYS5_ResetLogbook,	(uint32_t)OnAction_Nothing);
#endif

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}


void openEdit_ResetConfirmation(uint32_t editIdOfCaller)
{
    char text[32];

    resetMenuEdit(CLUT_MenuPageSystem);

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_AreYouSure;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[2] = 0;
    text[1] = TXT2BYTE_Abort;

    write_field_button(StMSYS5_Exit,				30, 800, ME_Y_LINE1,  &FontT48, text);

    text[2] = 0;
    text[3] = 0;
    switch(editIdOfCaller)
    {
    case StMSYS5_Reboot:
    case StMSYS5_RebootRTE:
    case StMSYS5_RebootMainCPU:
        text[1] = TXT2BYTE_RebootMainCPU;
        write_field_button(StMSYS5_RebootMainCPU,	30, 800, ME_Y_LINE2,  &FontT48, text);
        text[1] = TXT2BYTE_RebootRTE;
        write_field_button(StMSYS5_RebootRTE,			30, 800, ME_Y_LINE3,  &FontT48, text);
        setEvent(StMSYS5_Exit, (uint32_t)OnAction_Exit);
        setEvent(StMSYS5_RebootMainCPU, (uint32_t)OnAction_RebootMainCPU);
        setEvent(StMSYS5_RebootRTE, 		(uint32_t)OnAction_RebootRTE);
        text[0] = '\025';
        text[1] = TXT_2BYTE;
        text[2] = TXT2BYTE_DecoDataLost;
        text[3] = 0;
        write_label_var(			30, 800, ME_Y_LINE4,  &FontT48, text);
        break;

    case StMSYS5_ResetDeco:
        text[1] = TXT2BYTE_ResetDeco;
        write_field_button(editIdOfCaller,			30, 800, ME_Y_LINE2,  &FontT48, text);
        setEvent(StMSYS5_Exit, (uint32_t)OnAction_Exit);
        setEvent(editIdOfCaller, (uint32_t)OnAction_ResetDeco);
        text[0] = '\025';
        text[1] = TXT_2BYTE;
        text[2] = TXT2BYTE_DecoDataLost;
        text[3] = 0;
        write_label_var(			30, 800, ME_Y_LINE4,  &FontT48, text);
        break;

    case StMSYS5_ResetAll:
        text[1] = TXT2BYTE_ResetAll;
        write_field_button(editIdOfCaller,			30, 800, ME_Y_LINE2,  &FontT48, text);
        setEvent(StMSYS5_Exit, (uint32_t)OnAction_Exit);
        setEvent(editIdOfCaller, (uint32_t)OnAction_ResetAll);
        break;

    case StMSYS5_ResetLogbook:
        text[1] = TXT2BYTE_ResetLogbook;
        write_field_button(editIdOfCaller,			30, 800, ME_Y_LINE2,  &FontT48, text);
        setEvent(StMSYS5_Exit, (uint32_t)OnAction_Exit);
        setEvent(editIdOfCaller, (uint32_t)OnAction_ResetLogbook);
        break;
    }

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}

void openEdit_Maintenance(void)
{
    char text[32];
    unsigned char index = 0;
    SSettings *pSettings = settingsGetPointer();
    SSensorDataDiveO2* pDiveO2Data = NULL;

    resetMenuEdit(CLUT_MenuPageSystem);

    text[0] = '\001';
    text[1] = TXT_2BYTE;
    text[2] = TXT2BYTE_Maintenance;
    text[3] = 0;
    write_topline(text);

    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_SetFactoryDefaults;
    text[2] = 0;
    write_field_button(StMSYS5_SetFactoryBC,			30, 800, ME_Y_LINE1,  &FontT48, text);


    if(stateRealGetPointer()->lifeData.battery_charge <= 0)
    {
        text[0] = TXT_2BYTE;
        text[1] = TXT2BYTE_SetBatteryCharge;
        text[2] = 0;
        snprintf(&text[2],10,": %u%%",pSettings->lastKnownBatteryPercentage);
        write_field_button(StMSYS5_SetBattCharge,			30, 800, ME_Y_LINE2,  &FontT48, text);
    }

    if((pSettings->ppo2sensors_source == O2_SENSOR_SOURCE_ANADIG) || (pSettings->ppo2sensors_source == O2_SENSOR_SOURCE_DIGITAL))
    {
    	for (index = 0; index < 3; index++)
    	{
    		if(pSettings->ext_sensor_map[index] == SENSOR_DIGO2M)
    		{
    			pDiveO2Data = (SSensorDataDiveO2*)stateRealGetPointer()->lifeData.extIf_sensor_data[index];
    			if(pDiveO2Data->pressure != 0)
    			{
					snprintf(text,32,"%c%c (%1.3lf => %1.3f)\016\016Bar",TXT_2BYTE,TXT2BYTE_AdjustAmbPressure,(float)(pDiveO2Data->pressure/1000000.0),
																stateRealGetPointer()->lifeData.pressure_surface_bar);

					write_field_button(StMSYS5_AdjustSurfPres,			30, 800, ME_Y_LINE4,  &FontT48, text);
    			}
     			break;
    		}
    	}
    }

#ifdef ENABLE_ANALYSE_SAMPLES
    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_SetSampleIndex;
    text[2] = 0;
    write_field_button(StMSYS5_SetSampleIndx,			30, 800, ME_Y_LINE4,  &FontT48, text);
#endif

    setEvent(StMSYS5_SetFactoryBC, (uint32_t)OnAction_SetFactoryDefaults);
    if(stateRealGetPointer()->lifeData.battery_charge <= 0)
    {
    	setEvent(StMSYS5_SetBattCharge, (uint32_t)OnAction_SetBatteryCharge);
    }
    if((pSettings->ppo2sensors_source == O2_SENSOR_SOURCE_ANADIG) || (pSettings->ppo2sensors_source == O2_SENSOR_SOURCE_DIGITAL))
    {
    	if(pDiveO2Data != NULL)
    	{
    		setEvent(StMSYS5_AdjustSurfPres, (uint32_t)OnAction_AdjustSurfacePressure);
    	}
    }
#ifdef ENABLE_ANALYSE_SAMPLES
    setEvent(StMSYS5_SetSampleIndx, (uint32_t)OnAction_RecoverSampleIdx);
#endif


    text[0] = TXT_2BYTE;
    text[1] = TXT2BYTE_WarnBatteryLow;
    text[2] = 0;
    snprintf(&text[2],10,": %01.2fV",stateRealGetPointer()->lifeData.battery_voltage);
    write_label_var(  30, 800, ME_Y_LINE5, &FontT42, text);

    snprintf(&text[0],30,"Code: %X",getLicence());
    write_label_var(  30, 800, ME_Y_LINE6, &FontT42, text);

    write_buttonTextline(TXT2BYTE_ButtonBack,TXT2BYTE_ButtonEnter,TXT2BYTE_ButtonNext);
}

uint8_t OnAction_LogbookOffset(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    uint8_t digitContentNew;
    uint32_t newOffset;

    if(action == ACTION_BUTTON_ENTER)
        return digitContent;

    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &newOffset, 0, 0, 0);
        if(newOffset > 9000)
            newOffset = 0;
        tMenuEdit_newInput(editId, newOffset, 0, 0, 0);
        settingsGetPointer()->logbookOffset = (uint16_t)newOffset;
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

uint8_t OnAction_Nothing			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_Exit					(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    return EXIT_TO_MENU;
}
uint8_t OnAction_Confirm				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    openEdit_ResetConfirmation(editId);
    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_Maintenance			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	openEdit_Maintenance();
    return UNSPECIFIC_RETURN;
}

uint8_t OnAction_RebootRTE				(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    MX_SmallCPU_Reset_To_Standard();
    return EXIT_TO_MENU;
}

uint8_t OnAction_ResetDeco		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    clearDeco();
    return EXIT_TO_MENU;
}

uint8_t OnAction_ResetAll			(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    set_settings_to_Standard();
    check_and_correct_settings();

    return UPDATE_AND_EXIT_TO_HOME;
}

uint8_t OnAction_ResetLogbook	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    write_label_var( 430, 740, 350, &FontT42, "Wait");
    ext_flash_erase_logbook();

    SSettings * pSettings = settingsGetPointer();
    pSettings->lastDiveLogId = 255;
    pSettings->logFlashNextSampleStartAddress = 0;

    return EXIT_TO_MENU;
}

uint8_t OnAction_RebootMainCPU		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    settingsGetPointer()->showDebugInfo = 0;
    extern	uint8_t bootToBootloader;
    bootToBootloader = 1;
    return UNSPECIFIC_RETURN;
}


uint8_t OnAction_SetFactoryDefaults(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    settingsWriteFactoryDefaults(settingsGetPointer()->ButtonResponsiveness[3], settingsGetPointer()->buttonBalance);
    return EXIT_TO_MENU;
}

#ifdef ENABLE_ANALYSE_SAMPLES
uint8_t OnAction_RecoverSampleIdx(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	char text[32];
	uint8_t openSec;
	uint8_t retval = UNSPECIFIC_RETURN;
	openSec = ext_flash_AnalyseSampleBuffer();
    snprintf(&text[0],30,"OpenSec: %d",openSec);
    write_label_var(  30, 800, ME_Y_LINE6, &FontT42, text);

    if(openSec == 2)
    {
    	retval = UPDATE_DIVESETTINGS;
    }
    return retval;
}
#endif

uint8_t OnAction_SetBatteryCharge(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    setBatteryPercentage(settingsGetPointer()->lastKnownBatteryPercentage);
//	setBatteryPercentage(100);
    return EXIT_TO_MENU;
}

uint8_t OnAction_AdjustSurfacePressure		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    SSensorDataDiveO2* pDiveO2Data;
    const SDiveState* pDiveState = stateRealGetPointer();
    SSettings* pSettings = settingsGetPointer();
    uint8_t index = 0;
    int8_t newOffset = 0;

    char text[32];

    float orgpressure_surface;

    for (index = 0; index < 3; index++)
    {
    	if(settingsGetPointer()->ext_sensor_map[index] == SENSOR_DIGO2M)
    	{
    		pDiveO2Data = (SSensorDataDiveO2*)stateRealGetPointer()->lifeData.extIf_sensor_data[index];
    		orgpressure_surface = pDiveState->lifeData.pressure_surface_bar - (settingsGetPointer()->offsetPressure_mbar / 1000.0);
    		newOffset = ((pDiveO2Data->pressure/1000) - (orgpressure_surface * 1000));

    		if((pDiveState->lifeData.pressure_surface_bar * 1000 + newOffset) != (pDiveO2Data->pressure/1000)) /* there might be a rounding difference => compensate */
			{
    			newOffset += (pDiveO2Data->pressure/1000) - (pDiveState->lifeData.pressure_surface_bar * 1000 + newOffset);
			}

    		pSettings->offsetPressure_mbar = newOffset;
    		snprintf(text,32,"%c%c (%1.3lf => %1.3f)\016\016Bar",TXT_2BYTE,TXT2BYTE_AdjustAmbPressure,(float)(pDiveO2Data->pressure/1000000.0),
    		        													pDiveState->lifeData.pressure_surface_bar + pSettings->offsetPressure_mbar / 1000.0);
    		tMenuEdit_newButtonText(StMSYS5_AdjustSurfPres,text);
    		break;
    	}
    }

    return UNSPECIFIC_RETURN;
}


#ifdef SCREENTEST
uint8_t OnAction_ScreenTest		(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    static uint8_t FrameCount = 1; // 0 is invisible frame
    char text[5];
    GFX_DrawCfgScreen	tTestScreen;
    tTestScreen.FBStartAdress = 0;
    tTestScreen.ImageHeight = 480;
    tTestScreen.ImageWidth = 800;
    tTestScreen.LayerIndex = 1;

    set_globalState(StMSYS5_ScreenTest);
    tTestScreen.FBStartAdress = getFrameByNumber(FrameCount);
    if(tTestScreen.FBStartAdress == 0)
    {
        extern	uint8_t bootToBootloader;
        bootToBootloader = 1;
    }
    GFX_fill_buffer(tTestScreen.FBStartAdress, 0xFF, FrameCount);
    snprintf(text,5,"%u",FrameCount);
    Gfx_write_label_var(&tTestScreen,  10,100,10,&FontT48,CLUT_Font027,text);
    GFX_SetFramesTopBottom(tTestScreen.FBStartAdress, NULL,480);
    FrameCount++;
}
#endif
/*
uint8_t OnAction_TestCLog	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    write_label_var( 430, 740, 350, &FontT42, "Wait");

    test_log_only(20, 5);
    test_log_only(30, 10);
    ext_flash_write_settings();
    return EXIT_TO_MENU;
}
*/

