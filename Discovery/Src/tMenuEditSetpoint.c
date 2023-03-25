///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuEditSetpoint.c
/// \brief
/// \author heinrichs weikamp gmbh
/// \date   19-Dec-2014
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

#include "tMenuEditSetpoint.h"

#include "check_warning.h"
#include "gfx_fonts.h"
#include "tMenuEdit.h"
#include "unit.h"

/* Private types -------------------------------------------------------------*/
typedef struct
{
    uint8_t spID;
    SSetpointLine * pSetpointLine;
} SEditSetpointPage;


/* Private variables ---------------------------------------------------------*/
static SEditSetpointPage editSetpointPage;

/* Private function prototypes -----------------------------------------------*/

/* Announced function prototypes -----------------------------------------------*/
static uint8_t OnAction_SP_Setpoint    (uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_SP_DM_Sensor1	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_SP_DM_Sensor2	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint8_t OnAction_SP_DM_Sensor3	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);

/* Exported functions --------------------------------------------------------*/

void checkSwitchToLoop(void)
{
    if(!isLoopMode(stateUsedWrite->diveSettings.diveMode)) {
        stateUsedWrite->diveSettings.diveMode = settingsGetPointer()->dive_mode;

        unblock_diluent_page();
    }
}


void openEdit_Setpoint(uint8_t line)
{
    uint8_t useSensorSubMenu = 0;
    char text[20];
    uint8_t sensorActive[3];

    /* dive mode */
    if(actual_menu_content != MENU_SURFACE)
    {
        uint8_t setpointCbar, actualGasID;
        setpointCbar = 100;

        // actualGasID
        if(!isLoopMode(stateUsedWrite->diveSettings.diveMode))
        {
            actualGasID = stateUsedWrite->lifeData.lastDiluent_GasIdInSettings;
            if((actualGasID <= NUM_OFFSET_DILUENT) || (actualGasID > NUM_GASES + NUM_OFFSET_DILUENT))
                actualGasID = NUM_OFFSET_DILUENT + 1;
        }
        else
            actualGasID = stateUsedWrite->lifeData.actualGas.GasIdInSettings;

        // setpointCbar, CCR_Mode and sensor menu
        if((line < 6) && (stateUsedWrite->diveSettings.diveMode != DIVEMODE_PSCR))		/* setpoints inactive in PSCR mode */
        {
            setpointCbar = stateUsedWrite->diveSettings.setpoint[line].setpoint_cbar;
            stateUsedWrite->diveSettings.CCR_Mode = CCRMODE_FixedSetpoint;

            // BetterSetpoint warning only once
            if(actualBetterSetpointId() == line)
            {
                uint8_t depth;
                depth = stateUsedWrite->diveSettings.setpoint[line].depth_meter;
                // BetterSetpoint warning only once -> clear active
                for(int i=0; i<=NUM_GASES; i++)
                {
                	stateUsedWrite->diveSettings.setpoint[i].note.ub.first = 0;
                    if(stateUsedWrite->diveSettings.setpoint[i].depth_meter <= depth)
                    	stateUsedWrite->diveSettings.setpoint[i].note.ub.active = 0;
                }
                stateUsedWrite->diveSettings.setpoint[line].note.ub.first = 1;
            }
        }
        else	/* menu item not pointing to setpoint selection => use sensor or ppo2 simulation */
        {
        	if((stateUsedWrite->diveSettings.diveMode == DIVEMODE_PSCR) && (line == 2))
			{
        		stateUsedWrite->diveSettings.CCR_Mode = CCRMODE_Simulation;
			}
			else	/* => use sensor */
			{
				if(stateUsedWrite->diveSettings.CCR_Mode != CCRMODE_Sensors)
				{
					/* setpoint_cbar will be written by updateSetpointStateUsed() in main.c loop */
					setpointCbar = 255;
					stateUsedWrite->diveSettings.CCR_Mode = CCRMODE_Sensors;
				}
				else
				{
					useSensorSubMenu = 1;
				}
			}
        }

        setActualGas_DM(&stateUsedWrite->lifeData,actualGasID,setpointCbar);

        checkSwitchToLoop();

        clear_warning_fallback();

        if(!useSensorSubMenu)
        {
            exitMenuEdit_to_Home();
        }
        else // entire sub menu during dive to select sensors active
        {
            set_globalState_Menu_Line(line);
            resetMenuEdit(CLUT_MenuPageGasSP);

            text[0] = '\001';
            text[1] = TXT_o2Sensors;
            text[2] = 0;
            write_topline(text);

            if(stateUsedWrite->diveSettings.ppo2sensors_deactivated & 1)
            {
            	snprintf (text,20,"Sensor 1");
            	sensorActive[0] = 0;
            }
            else
            {
            	snprintf (text,20,"Sensor 1    (%01.2f)", stateUsed->lifeData.ppO2Sensor_bar[0] );
            	sensorActive[0] = 1;
            }
            write_label_var(  96, 600, ME_Y_LINE1, &FontT48, text);
            if(stateUsedWrite->diveSettings.ppo2sensors_deactivated & 2)
            {
               	snprintf (text,20,"Sensor 2");
               	sensorActive[1] = 0;
            }
            else
            {
               	snprintf (text,20,"Sensor 2    (%01.2f)", stateUsed->lifeData.ppO2Sensor_bar[1] );
               	sensorActive[1] = 1;
            }
            write_label_var(  96, 600, ME_Y_LINE2, &FontT48, text);
            if(stateUsedWrite->diveSettings.ppo2sensors_deactivated & 4)
            {
               	snprintf (text,20,"Sensor 3");
               	sensorActive[2] = 0;
            }
            else
            {
              	snprintf (text,20,"Sensor 3    (%01.2f)", stateUsed->lifeData.ppO2Sensor_bar[2] );
              	sensorActive[2] = 1;
            }
            write_label_var(  96, 600, ME_Y_LINE3, &FontT48, text);

            write_field_on_off(StMSP_Sensor1,	 30, 95, ME_Y_LINE1,  &FontT48, "", sensorActive[0]);
            write_field_on_off(StMSP_Sensor2,	 30, 95, ME_Y_LINE2,  &FontT48, "", sensorActive[1]);
            write_field_on_off(StMSP_Sensor3,	 30, 95, ME_Y_LINE3,  &FontT48, "", sensorActive[2]);

            setEvent(StMSP_Sensor1, (uint32_t)OnAction_SP_DM_Sensor1);
            setEvent(StMSP_Sensor2, (uint32_t)OnAction_SP_DM_Sensor2);
            setEvent(StMSP_Sensor3, (uint32_t)OnAction_SP_DM_Sensor3);
        }
        return;
    }
    else
    {
        /* surface mode */
        uint8_t spId, setpoint_cbar, sp_high, depthDeco, first;
        // uint8_t active,
        char text[70];
        uint8_t textPointer;
        uint16_t y_line;

        if(line < 6)
        {
			set_globalState_Menu_Line(line);

			resetMenuEdit(CLUT_MenuPageGasSP);

			spId = line;
			editSetpointPage.spID = spId;
			SSettings *data = settingsGetPointer();
			editSetpointPage.pSetpointLine = data->setpoint;

			setpoint_cbar = editSetpointPage.pSetpointLine[spId].setpoint_cbar;
			depthDeco = editSetpointPage.pSetpointLine[spId].depth_meter;
			//active = editSetpointPage.pSetpointLine[spId].note.ub.active;
			first = editSetpointPage.pSetpointLine[spId].note.ub.first;

			sp_high = setpoint_cbar / 100;

			strcpy(text, "\001" "Setpoint #0 X");
			text[11] += spId;
			text[13] = TXT_Setpoint_Edit;
			write_topline(text);


			y_line = ME_Y_LINE_BASE + (line * ME_Y_LINE_STEP);

			textPointer = 0;
			text[textPointer++] = 'S';
			text[textPointer++] = 'P';
			text[textPointer++] = '0' + spId;
			text[textPointer++] = ' ';
			text[textPointer++] = ' ';

			if(first == 0)
				strcpy(&text[textPointer++],"\177");

			textPointer += snprintf(&text[textPointer], 60,\
				"* "
				"       "
				"\016\016"
				" bar"
				"\017"
				"\034"
				"   "
				"\016\016"
				" "
				"\017"
				"           "
				"\016\016"
				"meter"
				"\017"
				"\035"
				"\n\r"
			);
			write_label_var(  20, 800, y_line, &FontT48, text);

			write_field_udigit(StMSP_ppo2_setting,	160, 800, y_line, &FontT48, "#.##            ###", (uint32_t)sp_high, (uint32_t)(setpoint_cbar - (100 * sp_high)), depthDeco, 0);
			setEvent(StMSP_ppo2_setting,	(uint32_t)OnAction_SP_Setpoint);
			startEdit();
		}
        else
        {
            SSettings *pSettings = settingsGetPointer();

            if(pSettings->autoSetpoint == 0)
            {
                pSettings->autoSetpoint = 1;
            }
            else
            {
                pSettings->autoSetpoint = 0;
            }
            exitMenuEdit_to_Menu_with_Menu_Update_do_not_write_settings_for_this_only();
        }
    }
}

static uint8_t OnAction_SP_Setpoint(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    int8_t digitContentNew;
    uint32_t new_integer_part, new_fractional_part, new_cbar, newDepth;

    if(action == ACTION_BUTTON_ENTER)
        return digitContent;

    if(action == ACTION_BUTTON_ENTER_FINAL)
    {
        evaluateNewString(editId, &new_integer_part, &new_fractional_part, &newDepth, 0);

        new_cbar = (new_integer_part * 100) + new_fractional_part;

        if(new_cbar < MIN_PPO2_SP_CBAR)
            new_cbar = MIN_PPO2_SP_CBAR;

        if(new_cbar > 160)
            new_cbar = 160;

        new_integer_part = new_cbar / 100;
        new_fractional_part = new_cbar - (new_integer_part * 100);

        editSetpointPage.pSetpointLine[editSetpointPage.spID].setpoint_cbar = new_cbar;

        if(newDepth > 255)
            newDepth = 255;

        editSetpointPage.pSetpointLine[editSetpointPage.spID].depth_meter = newDepth;

        return UPDATE_AND_EXIT_TO_MENU;
    }

    if(action == ACTION_BUTTON_NEXT)
    {
        digitContentNew = digitContent + 1;
        if((blockNumber == 0) && (digitContentNew > '1'))
            digitContentNew = '0';
        if(digitContentNew > '9')
            digitContentNew = '0';
        return digitContentNew;
    }

    if(action == ACTION_BUTTON_BACK)
    {
        digitContentNew = digitContent - 1;
        if((blockNumber == 0) && (digitContentNew < '0'))
            digitContentNew = '1';
        if(digitContentNew < '0')
            digitContentNew = '9';
        return digitContentNew;
    }

    return EXIT_TO_MENU;
}

void openEdit_DiveSelectBetterSetpoint(bool useLastDiluent)
{
    uint8_t spId;
    uint8_t depth;

    if(stateUsedWrite->diveSettings.diveMode != DIVEMODE_PSCR)		/* no setpoints in PSCR mode */
    {
		spId = actualBetterSetpointId();

		depth = stateUsedWrite->diveSettings.setpoint[spId].depth_meter;

		// BetterSetpoint warning only once -> clear active
		for(int i=0; i<=NUM_GASES; i++)
		{
			stateUsedWrite->diveSettings.setpoint[i].note.ub.first = 0;
			if(stateUsedWrite->diveSettings.setpoint[i].depth_meter <= depth)
				stateUsedWrite->diveSettings.setpoint[i].note.ub.active = 0;
		}

		// new setpoint
		stateUsedWrite->diveSettings.setpoint[spId].note.ub.first = 1;

        uint8_t gasId;
        if (useLastDiluent) {
            gasId = stateUsed->lifeData.lastDiluent_GasIdInSettings;
        } else {
            gasId = stateUsed->lifeData.actualGas.GasIdInSettings;
        }

		// change in lifeData
		setActualGas_DM(&stateUsedWrite->lifeData, gasId, stateUsedWrite->diveSettings.setpoint[spId].setpoint_cbar);
    }
}


static uint8_t OnAction_SP_DM_Sensor1	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    if(stateUsedWrite->diveSettings.ppo2sensors_deactivated & 1)
    {
    	stateUsedWrite->diveSettings.ppo2sensors_deactivated &= 4+2;
        tMenuEdit_set_on_off(editId, 1);
    }
    else
    {
    	stateUsedWrite->diveSettings.ppo2sensors_deactivated |= 1;
        tMenuEdit_set_on_off(editId, 0);
    }

    return UNSPECIFIC_RETURN;
}

static uint8_t OnAction_SP_DM_Sensor2	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    if(stateUsedWrite->diveSettings.ppo2sensors_deactivated & 2)
    {
    	stateUsedWrite->diveSettings.ppo2sensors_deactivated &= 4+1;
        tMenuEdit_set_on_off(editId, 1);
    }
    else
    {
    	stateUsedWrite->diveSettings.ppo2sensors_deactivated |= 2;
        tMenuEdit_set_on_off(editId, 0);
    }

    return UNSPECIFIC_RETURN;
}

static uint8_t OnAction_SP_DM_Sensor3	(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
    if(stateUsedWrite->diveSettings.ppo2sensors_deactivated & 4)
    {
    	stateUsedWrite->diveSettings.ppo2sensors_deactivated &= 2+1;
        tMenuEdit_set_on_off(editId, 1);
    }
    else
    {
    	stateUsedWrite->diveSettings.ppo2sensors_deactivated |= 4;
        tMenuEdit_set_on_off(editId, 0);
    }
    return UNSPECIFIC_RETURN;
}
