///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tHome.c
/// \brief  Control for Surface and Dive Templates
/// \author heinrichs weikamp gmbh
/// \date   10-November-2014
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
#include "tHome.h"

#include "data_exchange_main.h" // for dataOutGetPointer()
#include "gfx_fonts.h"
#include "t3.h"
#include "t4_tetris.h"
#include "t5_gauge.h"
#include "t6_apnea.h"
#include "t7.h"
#include "tDebug.h"
#include "timer.h" // for timer_Stopwatch_Restart
#include "tMenu.h"
#include "tMenuEditGasOC.h" // for openEdit_DiveSelectBetterGas()
#include "tMenuEditSetpoint.h" // for openEdit_DiveSelectBetterSetpoint()
#include "simulation.h"
#include "motion.h"
#include "logbook_miniLive.h"

/* Private types -------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
_Bool warning_count_high_time = 0;
_Bool display_count_high_time = 0;

uint8_t errorsInSettings = 0;
/* Private variables ---------------------------------------------------------*/
static uint8_t warning_toogle_count;
static uint16_t display_toogle_count;
static uint16_t tHome_tick_count_cview;
static uint16_t tHome_tick_count_field;
static uint16_t tHome_tick_count_o2sens;

const uint8_t cv_changelist[] = {CVIEW_Compass, CVIEW_SummaryOfLeftCorner, CVIEW_Tissues, CVIEW_Profile, CVIEW_EADTime, CVIEW_Gaslist, CVIEW_noneOrDebug, CVIEW_Decolist,CVIEW_sensors,CVIEW_sensors_mV, CVIEW_END};
const uint8_t cv_changelist_BS[] = {CVIEW_T3_Decostop, CVIEW_sensors, CVIEW_Compass, CVIEW_T3_MaxDepth,CVIEW_T3_StopWatch, CVIEW_T3_TTS, CVIEW_T3_GasList, CVIEW_T3_ppO2andGas, CVIEW_noneOrDebug,
									CVIEW_T3_Navigation, CVIEW_T3_DepthData, CVIEW_T3_DecoTTS,
#ifdef ENABLE_T3_PROFILE_VIEW
									CVIEW_T3_Profile,
#endif
									CVIEW_T3_END};

/* Private function prototypes -----------------------------------------------*/

#define AUTORETURN_O2SENS		(200u)		/* return to sensor view after 20 seconds in case sensor is connected */


/* Exported functions --------------------------------------------------------*/

void set_globalState_tHome(void)
{
    if(stateUsed->mode == MODE_DIVE)
    {
    	if(settingsGetPointer()->extraDisplay == EXTRADISPLAY_BFACTIVE)
    	{
    		settingsGetPointer()->design = 3;
    	}
        set_globalState(StD);
    }
    else
        set_globalState(StS);
}


void tHome_init(void)
{
    t7_init(); // standard + surface
    t3_init(); // big font
    t4_init(); // game
    t5_init(); // gauge
    t6_init(); // apnea
}


void tHome_init_compass(void)
{
    init_t7_compass();
}


void tHome_refresh(void)
{
    SSettings* pSettings = settingsGetPointer();

    warning_toogle_count++;
    if(warning_toogle_count >= 2* pSettings->warning_blink_dsec)
        warning_toogle_count = 0;

    if(warning_toogle_count >= pSettings->warning_blink_dsec)
        warning_count_high_time = 1;
    else
        warning_count_high_time = 0;


    display_toogle_count++;
    if(display_toogle_count >= 2* pSettings->display_toogle_desc)
        display_toogle_count = 0;

    if(display_toogle_count >= pSettings->display_toogle_desc)
        display_count_high_time = 1;
    else
        display_count_high_time = 0;


    if(pSettings->design == 6)
        t6_refresh();
    else
    if(pSettings->design == 5)
        t5_refresh();
    else
    if(pSettings->design == 4)
        t4_refresh();
    else
    if(pSettings->design == 3)
        t3_refresh();
    else
    if(pSettings->design == 7)
        t7_refresh();
    else
    {
        pSettings->design = 7;
        t7_refresh();
    }
}


void tHome_sleepmode_fun(void)
{
    t7_refresh_sleepmode_fun();
}


static void checkSetStateSim(SSettings *settings)
{
    if (is_stateUsedSetToSim()) {
        if (settings->design != 3) {
            set_globalState(StDSIM1);
        } else {
#ifdef ENABLE_T3_PPO_SIM
            if (stateUsed->diveSettings.ppo2sensors_deactivated & 0x01 == 0) {
                set_globalState(StDSIM1);
            } else if (stateUsed->diveSettings.ppo2sensors_deactivated & 0x02 == 0) {
                set_globalState(StDSIM3);
            } else if (stateUsed->diveSettings.ppo2sensors_deactivated & 0x04 == 0) {
                set_globalState(StDSIM5);
            } else {
                set_globalState(StD);
            }
#endif
        }
    } else {
        set_globalState(StD);
    }
}


static void checkSetStateCompassSim(SSettings *settings)
{
    if (settings->design == 7 && t7_isCompassShowing()) {
        set_globalState(StDBEAR);
    } else {
        checkSetStateSim(settings);
    }
}


void tHomeDiveMenuControl(uint8_t sendAction)
{
    SSettings *settings = settingsGetPointer();

    if(sendAction == ACTION_BUTTON_NEXT)
    {
        if(settingsGetPointer()->design == 4)
            return;

        switch(get_globalState())
        {
        	case StD:
        	if(settingsGetPointer()->design == 6)
            {
                if(is_stateUsedSetToSim())
                    set_globalState(StDSIM1);
                else
                    set_globalState(StDQUIT);
                break;
            }

            if(settingsGetPointer()->design == 5)
            {
                if(t5_getCustomView() == CVIEW_Compass)
                    set_globalState(StDBEAR);
                else
                    set_globalState(StDRAVG);
                break;
            }

            if(settingsGetPointer()->design == 3)
            {
            	switch(t3_getCustomView())
            	{
            		case CVIEW_T3_Navigation:
            		case CVIEW_Compass:		set_globalState(StDBEAR);
            			break;
            		case CVIEW_T3_StopWatch: set_globalState(StDRAVG);
            			break;
            		case CVIEW_T3_GasList: if(stateUsed->warnings.betterGas)
            								{
                        						set_globalState(StDMGAS);
            								}
            			break;
#ifdef ENABLE_T3_PPO_SIM
            		case CVIEW_sensors:		if(is_stateUsedSetToSim())
											{
												set_globalState(StDSIM1);
											}
						break;
#endif
#ifdef ENABLE_T3_PROFILE_VIEW
            		case CVIEW_T3_Profile: 	set_globalState(StDMARK);
#endif
            			break;
            		default:
            			set_globalState(StDMENU);
            			break;
            	}
            	break;
            }

            if(stateUsed->warnings.betterGas)
                set_globalState(StDMGAS);
            else
            if(stateUsed->warnings.betterSetpoint)
                set_globalState(StDMSPT);
            else
                set_globalState(StDMENU);
            break;

        case StDMGAS:
        	if(settingsGetPointer()->design == 3)
        	{
        		set_globalState(StD);
        	}
        	else
        	{
				if(stateUsed->warnings.betterSetpoint)
					set_globalState(StDMSPT);
				else
					set_globalState(StDMENU);
        	}
            break;

        case StDMSPT:
                set_globalState(StDMENU);
            break;

        case StDMENU:
            if (settings->design == 7 && isLoopMode(settings->dive_mode)) {
                set_globalState(StDBAILOUT);

                break;
            }

            checkSetStateCompassSim(settings);

            break;
        case StDBAILOUT:
            if (settingsGetPointer()->dive_mode == DIVEMODE_CCR && isLoopMode(stateUsed->diveSettings.diveMode) && findSwitchToSetpoint()) {
                set_globalState(StDSETPOINT);

                break;
            }

            checkSetStateCompassSim(settings);

            break;
        case StDSETPOINT:
            checkSetStateCompassSim(settings);

            break;
        case StDSIM1:
                set_globalState(StDSIM2);
            break;

        case StDSIM2:
           		if(settingsGetPointer()->design != 3)
            	{
            		set_globalState(StDSIM3);
            	}
            	else
            	{
#ifdef ENABLE_T3_PPO_SIM
            		if((stateUsed->diveSettings.ppo2sensors_deactivated & 0x02) == 0)
            		{
            			set_globalState(StDSIM3);
            		}
            		else if((stateUsed->diveSettings.ppo2sensors_deactivated & 0x04) == 0)
            		{
            			set_globalState(StDSIM5);
            		}
            		else
            		{
            			set_globalState(StD);
            		}
#endif
            	}
            break;

        case StDSIM3:
                set_globalState(StDSIM4);
            break;

        case StDSIM4:
           	if(settingsGetPointer()->design != 3)
            {
                set_globalState(StD);
            }
           	else
           	{
#ifdef ENABLE_T3_PPO_SIM
           		if((stateUsed->diveSettings.ppo2sensors_deactivated & 0x04) == 0)
           		{
           			set_globalState(StDSIM5);
           		}
           		else
           		{
           			set_globalState(StD);
           		}
#endif
           	}
            break;
#ifdef ENABLE_T3_PPO_SIM
        case StDSIM5:
                set_globalState(StDSIM6);
            break;

        case StDSIM6:
                set_globalState(StD);
            break;
#endif
        case StDBEAR:
            if (settingsGetPointer()->design == 7) {
                checkSetStateSim(settings);

                break;
            }

        	if(settingsGetPointer()->design == 5)
        	{
        		set_globalState(StDRAVG);
        	}

           	if(settingsGetPointer()->design == 3)
            {
           		if(t3_getCustomView() == CVIEW_T3_Navigation)
           		{
           			set_globalState(StDRAVG);
           		}
           		else
           		{
           			set_globalState(StD);
           		}
            }
            break;
        case StDRAVG:
        	if(settingsGetPointer()->design == 5)
        	{
				if(is_stateUsedSetToSim())
					set_globalState(StDSIM1);
				else
					set_globalState(StD);
				break;
        	}
        	else
        	{
        		set_globalState(StD);
        	}
        	break;
        case StDQUIT: // t6_apnea
            set_globalState(StD);
            break;

        case StDMARK:
        	if((settingsGetPointer()->design == 3) && (MiniLiveLogbook_getNextMarkerIndex(0) != 0))
            {
        		set_globalState(StDCHECK);
            }
        	else
        	{
        		set_globalState(StD);
        	}
        	break;

        default:
            set_globalState(StD);
        }
    }

    if(sendAction == ACTION_BUTTON_ENTER)
    {
        if(settingsGetPointer()->design == 4)
            return;

        switch(get_globalState())
        {
        case StDMGAS:
            openEdit_DiveSelectBetterGas(false);
            set_globalState(StD);
            break;
        case StDMSPT:
            openEdit_DiveSelectBetterSetpoint(false);
            set_globalState(StD);
            break;

        case StDMENU:
            openMenu_first_page_with_OC_gas_update();
            break;

        case StDSIM1:
        	if(settingsGetPointer()->design != 3)
        	{
                Sim_Quit();
        	}
        	else
        	{
        		Sim_IncreasePPO(0);
        	}
            break;

        case StDSIM2:
        	if(settingsGetPointer()->design != 3)
        	{
        		Sim_Ascend();
        	}
        	else
        	{
        		Sim_DecreasePPO(0);
        	}

            break;

        case StDSIM3:
        	if(settingsGetPointer()->design != 3)
        	{
                Sim_Descend();
        	}
        	else
        	{
        		Sim_IncreasePPO(1);
        	}

            break;

        case StDSIM4:
        	if(settingsGetPointer()->design != 3)
        	{
                Sim_Divetime();
        	}
        	else
        	{
        		Sim_DecreasePPO(1);
        	}

            break;
#ifdef ENABLE_T3_PPO_SIM
        case StDSIM5:
        		Sim_IncreasePPO(2);
			break;
        case StDSIM6:
        		Sim_DecreasePPO(2);
			break;
#endif
        case StDBAILOUT:
            if (isLoopMode(stateUsed->diveSettings.diveMode)) {
                tMEGas_check_switch_to_bailout();

                openEdit_DiveSelectBetterGas(true);
            } else {
                checkSwitchToLoop();

                openEdit_DiveSelectBetterSetpoint(true);
            }

            set_globalState(StD);

            break;
        case StDSETPOINT:
            checkSwitchSetpoint();

            set_globalState(StD);

            break;
        case StDBEAR: // t5_gauge, t7
            setCompassHeading((uint16_t)stateUsed->lifeData.compass_heading);
            set_globalState(StD);
            break;

        case StDRAVG: // t5_gauge
            timer_Stopwatch_Restart();
            set_globalState(StD);
            break;

        case StDQUIT: // t6_apnea
            set_globalState(StD); // used to end StDQUIT, is called before everything else because changes are made in the next lines
            if(is_stateUsedSetToSim())
                Sim_Quit();
            else
                dataOutGetPointer()->setEndDive = 1;
            break;
        case StDMARK:	stateUsedWrite->events.manualMarker = 1;
				        set_globalState(StD);
        	break;

        case StDCHECK:	MiniLiveLogbook_checkMarker();
        	break;
        default:
            break;
        }
    }
}


void tHome_findNextStop(const uint16_t *list, uint8_t *depthOutMeter, uint16_t *lengthOutSeconds)
{
    uint8_t ptr = DECOINFO_STRUCT_MAX_STOPS - 1;

    while(ptr && !list[ptr])
        ptr--;

    *lengthOutSeconds = list[ptr];
    if(!(*lengthOutSeconds))
    {
        *depthOutMeter = 0;
    }
    else
    if(ptr == 0)
    {
        *depthOutMeter = (uint8_t)((stateUsed->diveSettings.last_stop_depth_bar*10.0f) + 0.1f);
    }
    else
    {
        ptr -= 1;
        *depthOutMeter = (uint8_t)(((stateUsed->diveSettings.input_second_to_last_stop_depth_bar + (stateUsed->diveSettings.input_next_stop_increment_depth_bar * ptr))*10.0f) + 0.1f);
    }
}


void tHome_change_field_button_pressed(void)
{
    tHome_tick_count_field = 0;
    tHome_tick_count_o2sens = 0;
    if(settingsGetPointer()->design == 7)
        t7_change_field();
}


void tHome_change_customview_button_pressed(uint8_t action)
{
    tHome_tick_count_cview = 0;
    tHome_tick_count_o2sens = 0;

    if(settingsGetPointer()->design == 7)
        t7_change_customview(action);
    else
    if(settingsGetPointer()->design == 3)
        t3_change_customview(action);
    else
    if(settingsGetPointer()->design == 5)
        t5_change_customview(action);
    else
    if(settingsGetPointer()->design == 6)
        t6_change_customview(action);
}

uint8_t tHome_getNumberOfAvailableCVs(const uint8_t* pcv_list)
{
	uint8_t cnt = 0;

    while((pcv_list[cnt] != CVIEW_END) && (pcv_list[cnt] != CVIEW_T3_END))
    {
    	cnt++;
    	if (cnt > 100)	/* just in case an invalid list has been provided... */
    	{
    		break;
    	}
    }
    cnt--;		/* do not count end token */
    return cnt;
}

void tHome_tick(void)
{
    uint16_t field = settingsGetPointer()->tX_userselectedLeftLowerCornerTimeout;
    uint16_t cview = settingsGetPointer()->tX_customViewTimeout;

    if(field)
    {
        tHome_tick_count_field++;
        if(tHome_tick_count_field > (field * 10))
        {
            tHome_tick_count_field = 0;
            if(settingsGetPointer()->design == 7)
            {
                t7_set_field_to_primary();
            }
        }
    }

    if(cview)
    {
        tHome_tick_count_cview++;
        if(tHome_tick_count_cview > (cview *10))
        {
            tHome_tick_count_cview = 0;
            if(settingsGetPointer()->design == 7)
            {
                t7_set_customview_to_primary();
            }
            if(settingsGetPointer()->design == 3)
            {
                t3_set_customview_to_primary();
            }
        }
    }

    if((stateUsed->mode == MODE_SURFACE) && (stateUsed->diveSettings.ppo2sensors_deactivated != 0x07) && (stateUsed->diveSettings.ccrOption != 0) && (!t7_customview_disabled(CVIEW_sensors)))
    {
    	tHome_tick_count_o2sens++;
    	if(tHome_tick_count_o2sens > AUTORETURN_O2SENS)
    	{
    		tHome_tick_count_o2sens = 0;
    		t7_select_customview(CVIEW_sensors);
    	}
    }
}


uint32_t tHome_DateCode(RTC_DateTypeDef *dateInput)
{
    uint32_t answer = 0;

    answer = 0;
    answer += (dateInput->Year & 0x7F)<< 9;
    answer += (dateInput->Month & 0x0F)<< 5;
    answer += (dateInput->Date & 0x1F);

    return answer;
}


uint8_t tHome_gas_writer(uint8_t oxygen_percentage, uint8_t helium_percentage, char *text)
{
    if(oxygen_percentage == 100)
        return (uint8_t) snprintf(text,10,"Oxy");
    else if((oxygen_percentage == 21) && (!helium_percentage))
        return (uint8_t) snprintf(text,10,"Air");
    else if(!helium_percentage)
        return (uint8_t) snprintf(text,10,"NX%02i",oxygen_percentage);
    else if((oxygen_percentage + helium_percentage) == 100)
        return (uint8_t) snprintf(text,10,"HX%02i",oxygen_percentage);
    else
        return (uint8_t) snprintf(text,10,"%02i/%02i",oxygen_percentage,helium_percentage);
}

uint8_t tHome_show_lost_connection_count(GFX_DrawCfgScreen *ScreenToWriteOn)
{
	static uint8_t LastKnowRTEState = SPI_RX_STATE_INVALID;

	if(!SPI_MIN_ERROR_SHOW) return 0;
	if(DataEX_lost_connection_count()>=SPI_MIN_ERROR_SHOW && SPI_SHOW_SYNC_STATS){

    char text[64];

    SDataExchangeSlaveToMaster* dataIn=get_dataInPointer();
    SDataReceiveFromMaster* pDataOut = dataOutGetPointer();

    snprintf(text,32,"spi err:\002 %ld/%ld",DataEX_lost_connection_count(),get_num_SPI_CALLBACKS());
    Gfx_write_label_var(ScreenToWriteOn,  100,300, 0,&FontT24,CLUT_ButtonSymbols,text);

//    snprintf(text,32,"header:\002%X%X%X%X",dataIn->header.checkCode[0],dataIn->header.checkCode[1],dataIn->header.checkCode[2],dataIn->header.checkCode[3]);
//    Gfx_write_label_var(ScreenToWriteOn,  350,550, 0,&FontT24,CLUT_ButtonSymbols,text);

    //snprintf(text,32,"footer:\002%X%X%X%X",dataIn->footer.checkCode[0],dataIn->footer.checkCode[1],dataIn->footer.checkCode[2],dataIn->footer.checkCode[3]);

    /* data shifted => ignore received data */
    if((pDataOut->header.checkCode[SPI_HEADER_INDEX_RX_STATE] == SPI_RX_STATE_SHIFTED) || (pDataOut->header.checkCode[SPI_HEADER_INDEX_RX_STATE] == SPI_RX_STATE_OFFLINE))
    {
    	dataIn->header.checkCode[SPI_HEADER_INDEX_RX_STATE] = LastKnowRTEState;
    }
    else
    {
    	LastKnowRTEState =dataIn->header.checkCode[SPI_HEADER_INDEX_RX_STATE];
    }
    snprintf(text,32,"RX State M|R:\002%X|%X",pDataOut->header.checkCode[SPI_HEADER_INDEX_RX_STATE], dataIn->header.checkCode[SPI_HEADER_INDEX_RX_STATE] );
    Gfx_write_label_var(ScreenToWriteOn,  600,800, 0,&FontT24,CLUT_ButtonSymbols,text);
    }



//    snprintf(text,32,"cpt:\002%i",get_num_SPI_CALLBACKS());
//    Gfx_write_label_var(ScreenToWriteOn,  600,800, 90,&FontT24,CLUT_ButtonSymbols,text);

//    snprintf(text,10,"i2c:\002%i",get_DataEX_Error_place());
//    Gfx_write_label_var(ScreenToWriteOn,  600,800, 90,&FontT24,CLUT_ButtonSymbols,text);

    return DataEX_lost_connection_count();
}
