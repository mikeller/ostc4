///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuXtra.c
/// \brief  Menu Xtra - Specials in Divemode like Reset Stopwatch
/// \author heinrichs weikamp gmbh
/// \date   02-Mar-2015
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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "tMenu.h"
#include "tStructure.h"
#include "tMenuXtra.h"
#include "text_multilanguage.h"
#include "data_central.h"
#include "simulation.h"
#include "configuration.h"

/* Exported functions --------------------------------------------------------*/

uint32_t tMXtra_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    uint8_t textPointer = 0;
    uint8_t CcrModusTxtId = 0;

    textPointer = 0;
    *tab = 500;
    *subtext = 0;

    SSettings *pSettings = settingsGetPointer();

    /* DIVE MODE */
    if(actual_menu_content != MENU_SURFACE)
    {
		if((line == 0) || (line == 1))
		{
			text[textPointer++] = TXT_2BYTE;
			text[textPointer++] = TXT2BYTE_ResetStopwatch;
		}
		strcpy(&text[textPointer],"\n\r");
		textPointer += 2;
	/*
		if((line == 0) || (line == 2))
		{
				text[textPointer++] = TXT_2BYTE;
				text[textPointer++] = TXT2BYTE_ResetAvgDepth;
		}
		strcpy(&text[textPointer],"\n\r");
		textPointer += 2;
	*/
		if((line == 0) || (line == 2))
		{
			text[textPointer++] = TXT_2BYTE;
			text[textPointer++] = TXT2BYTE_CompassHeading;
		}
		strcpy(&text[textPointer],"\n\r");
		textPointer += 2;

		if((line == 0) || (line == 3))
		{
			text[textPointer++] = TXT_2BYTE;
			text[textPointer++] = TXT2BYTE_SetMarker;
		}
		strcpy(&text[textPointer],"\n\r");
		textPointer += 2;

#ifdef ENABLE_MOTION_CONTROL
		if((line == 0) || (line == 4))
		{
			text[textPointer++] = TXT_2BYTE;
			text[textPointer++] = TXT2BYTE_CalibView;
		}
		strcpy(&text[textPointer],"\n\r");
		textPointer += 2;
#endif

		if(is_stateUsedSetToSim())
		{
			if((line == 0) || (line == 5))
			{
				text[textPointer++] = TXT_2BYTE;
				text[textPointer++] = TXT2BYTE_SimFollowDecoStops;
				text[textPointer++] = ' ';
				text[textPointer++] = ' ';
				if(simulation_get_heed_decostops())
					text[textPointer++] = '\005';
				else
					text[textPointer++] = '\006';
			}
			strcpy(&text[textPointer],"\n\r");
			textPointer += 2;
		}
		else
		{
			if((line == 0) || (line == 5))		/* end dive mode only used during real dives */
				{
					text[textPointer++] = TXT_2BYTE;
					text[textPointer++] = TXT2BYTE_EndDiveMode;
				}
			strcpy(&text[textPointer],"\n\r");
			textPointer += 2;
		}
    }
    else	/* Surface MODE */
    {

        if((line == 0) || (line == 1))
        {
           	switch(pSettings->CCR_Mode)
           	{
            		case CCRMODE_Sensors: CcrModusTxtId = TXT_Sensor;
            			break;
            		case CCRMODE_FixedSetpoint: CcrModusTxtId = TXT_FixedSP;
            			break;
            		case CCRMODE_Simulation: CcrModusTxtId = TXT_SimPpo2;
            			break;
            		default:	CcrModusTxtId = 'X';
            			break;
           	}
            textPointer += snprintf(&text[textPointer], 60,\
                    "%c"
                    "\t"
                    "%c"
                    , TXT_CCRmode
                    , CcrModusTxtId
                );

            strcpy(&text[textPointer],"\n\r");
            textPointer += 2;
        }

        if((line == 0) || (line == 2))
         {
             bool canDoFallback = pSettings->CCR_Mode == CCRMODE_Sensors;
             if (!canDoFallback) {
                 text[textPointer++] = '\031';
             }
             textPointer += snprintf(&text[textPointer], 60,\
                 "%c"
                 ,TXT_Fallback
             );

             text[textPointer++] = '\t';
             if(settingsGetPointer()->fallbackToFixedSetpoint && canDoFallback)
                 text[textPointer++] = '\005';
             else
                 text[textPointer++] = '\006';

             if (!canDoFallback) {
                 text[textPointer++] = '\020';
             }
             strcpy(&text[textPointer],"\n\r");
             textPointer += 2;
         }


        if((line == 0) || (line == 3))
         {
             textPointer += snprintf(&text[textPointer], 60,\
                 "%c"
                 ,TXT_ScrubTime
             );
             strcpy(&text[textPointer],"\n\r");
             textPointer += 2;
         }



#ifdef ENABLE_PSCR_MODE
        if(pSettings->dive_mode == DIVEMODE_PSCR)
        {
            if((line == 0) || (line == 4))
             {
                 textPointer += snprintf(&text[textPointer], 60,\
                             "%c"
                             ,TXT_PSClosedCircuit);
             }
        }
#endif
#ifdef ENABLE_CO2_SUPPORT
        if((line == 0) || (line == 4))
         {
             textPointer += snprintf(&text[textPointer], 60, "%c", TXT_CO2Sensor);
         }
#endif
    }
    return StMXTRA;
}

