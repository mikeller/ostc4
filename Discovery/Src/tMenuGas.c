///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuGas.c
/// \brief  Main Template file for Menu Page Gas, OC only at the moment
/// \author heinrichs weikamp gmbh
/// \date   30-April-2014
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
#include "tMenu.h"
#include "tMenuGas.h"
#include "check_warning.h"
#include "decom.h"
#include "unit.h"
#include "configuration.h"

#define OCGAS_STANDARD (0)
#define OCGAS_BAILOUT_INACTIVE (1)
#define OCGAS_BAILOUT_ACTIVE (2)
#define CCGAS_STANDARD (3)

/* Private function prototypes -----------------------------------------------*/
uint8_t write_gas(char *text, uint8_t oxygen, uint8_t helium);
void tMG_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext, uint8_t start, uint8_t gasMode);

/* Exported functions --------------------------------------------------------*/

uint32_t tMOG_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    uint8_t gas_mode;

    if((actual_menu_content == MENU_SURFACE) || (stateUsed->diveSettings.ccrOption == 0))
    {
        gas_mode = OCGAS_STANDARD;
    }
    else
    {
        if (isLoopMode(stateUsed->diveSettings.diveMode))
            gas_mode = OCGAS_BAILOUT_INACTIVE;
        else
            gas_mode = OCGAS_BAILOUT_ACTIVE;
    }

    tMG_refresh(line, text, tab, subtext, 0, gas_mode);
    return StMOG;
}

uint32_t tMCG_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    tMG_refresh(line, text, tab, subtext, NUM_OFFSET_DILUENT, CCGAS_STANDARD);
    return StMCG;
}

void tMG_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext, uint8_t start, uint8_t gasMode)
{
    const SGasLine * pGasLine;

    uint8_t  gasId, oxygen, helium, depthUp, active, first, typeDeco;
    float fPpO2limitHigh = 0;
    float fPpO2ofGasAtThisDepth = 0;
    //uint8_t senderCode, depthDown,;
#ifdef ENABLE_ADVANCED_GAS
    uint16_t bar, ltr;
#endif
    uint16_t textPointer, mod;

    if(actual_menu_content == MENU_SURFACE)
    {
        pGasLine = settingsGetPointer()->gas;
    }
    else
    {
        pGasLine = stateUsed->diveSettings.gas;
        if(actualLeftMaxDepth(stateUsed))
            fPpO2limitHigh = (float)(settingsGetPointer()->ppO2_max_deco) / 100;
        else
            fPpO2limitHigh = (float)(settingsGetPointer()->ppO2_max_std) / 100;
    }
    textPointer = 0;
    *tab = 158;

/*
    if((line == 0) && (actual_menu_content == MENU_SURFACE))
    {
        strcpy(subtext,
        "\022"
//		"           Travel Deco   MOD     Bottle"
        "       First MOD Deco    Bottle"
        );
    }
*/

    gasId = start;
    for(int count=1;count<=NUM_GASES;count++)
    {
        gasId++;
        if(line && (line != count))
        {
                first = pGasLine[gasId].note.ub.first;
                if(first == 0)
                {
                    strcpy(&text[textPointer],
                        "\t"
                        "\177"
                        "*"
                        "\n\r"
                    );
                    textPointer += 5;
                }
                else
                {
                    strcpy(&text[textPointer],"\n\r");
                    textPointer += 2;
                }
        }
        else
        {
            oxygen = pGasLine[gasId].oxygen_percentage;
            helium = pGasLine[gasId].helium_percentage;
            depthUp = pGasLine[gasId].depth_meter;
            //depthDown = pGasLine[gasId].depth_meter_travel;
            //senderCode = pGasLine[gasId].note.ub.senderCode;
            active = pGasLine[gasId].note.ub.active;
            first = pGasLine[gasId].note.ub.first;
            typeDeco = pGasLine[gasId].note.ub.deco;
            mod = calc_MOD(gasId);
#ifdef ENABLE_ADVANCED_GAS
            ltr = pGasLine[gasId].bottle_size_liter;
            if(pGasLine[gasId].bottle_id_bar != 0)
            {
            	bar = pGasLine[gasId].bottle_id_bar;
            }
            else
            {
            	bar = stateUsed->lifeData.bottle_bar[gasId];  /* this would be sensor data... */
            }
#endif

#ifdef ENABLE_UNUSED_GAS_HIDING
            if(pGasLine[gasId].note.ub.off)
            {
            	strcpy(&text[textPointer++],"\021");
            }
            else
#endif
            if(active)
            {
                if(actual_menu_content == MENU_SURFACE)
                    strcpy(&text[textPointer++],"\020");
                else
                {
                    fPpO2ofGasAtThisDepth = (stateUsed->lifeData.pressure_ambient_bar - WATER_VAPOUR_PRESSURE) * pGasLine[gasId].oxygen_percentage / 100;
                    if(fPpO2ofGasAtThisDepth > fPpO2limitHigh)
                        strcpy(&text[textPointer++],"\025");
                    else
                        strcpy(&text[textPointer++],"\020");
                }
            }
            else
            {
                strcpy(&text[textPointer++],"\031");
            }
            textPointer += write_gas(&text[textPointer], oxygen, helium);

            strcpy(&text[textPointer++],"\t");

            if(gasMode == OCGAS_BAILOUT_INACTIVE)
            {
                textPointer += snprintf(&text[textPointer], 59,\
                    "\024 Bailout\031\034 %3u\016\016%c%c\017"
                	"\035",
					unit_depth_integer(mod),
                    unit_depth_char1(),
                    unit_depth_char2());
#ifdef ENABLE_ADVANCED_GAS
					if( ltr != 0)
					{
						textPointer += snprintf(&text[textPointer], 20,"\033 %2u\016\016ltr\017",ltr);
					}
					else
					{
						text[textPointer++] = '\033';
						text[textPointer++] = '\033';
						text[textPointer++] = '\033';
					}
                	if( bar != 0)
                	{
                		textPointer += snprintf(&text[textPointer], 20,"\033 %2u\016\016bar\017",bar);
                	}

#endif
                	textPointer += snprintf(&text[textPointer], 5,"\n\r");
            }
            else
            {
                if(first == 0)
                    strcpy(&text[textPointer++],"\177");

                /* color active / inactive for gas changes */
                char color[5] = {'\031','\031','\031','\031','\031'};
                if(active)
                {
                    /* mod */
                    color[0] = '\023';
                    /* deco */
                    if(typeDeco && depthUp)
                        color[1] = '\020';
                    /* ltr */
#ifdef ENABLE_ADVANCED_GAS
                    if(ltr)
                        color[2] = '\020';
#endif
                    /* bar */

                    if(mod < depthUp)
                    {
                        color[0] = '\025';
                        color[1] = '\025';
                    }
                    if(typeDeco && !depthUp)
                    {
                        color[1] = '\025';
                    }
                }

                /* output */
                /* MOD */
                textPointer += snprintf(&text[textPointer], 14,\
                    "*"
                    " "
                    "\034"
                    "%c"
                    "%3u"
                    "\016\016"
                    "%c%c"
                    "\017",
                    color[0],
                    unit_depth_integer(mod),
                    unit_depth_char1(),
                    unit_depth_char2()
                );

                /* deco */
                if(typeDeco)
                {
                    textPointer += snprintf(&text[textPointer], 14,\
                        "%c"
                        "%3u"
                        "\016\016"
                        "%c%c"
                        "\017",
                        color[1],
                        unit_depth_integer(depthUp),
                        unit_depth_char1(),
                        unit_depth_char2()
                    );
                }
                else
                {
                    text[textPointer++] = '\x7F';
                    text[textPointer++] = '\x7F';
                    text[textPointer++] = 3*25 + 20;
                }
                /* liter */
#ifdef ENABLE_ADVANCED_GAS
                if(ltr != 0)
                {
					textPointer += snprintf(&text[textPointer], 12,\
						"%c"
						"\033\033%2u"
						"\016\016"
						"ltr"
						"\017"
						, color[2],ltr
					);
                }
                else
                {
                	text[textPointer++] = '\033';
                	text[textPointer++] = '\033';
                	text[textPointer++] = '\033';
                }
                /* bar */
                if(bar != 0)
                {
					textPointer += snprintf(&text[textPointer], 12,\
						"%c"
						"\033%3u"
						"\016\016"
						"bar"
						"\017",
						color[3], bar);
                }
#endif
                text[textPointer++] = '\035';
                text[textPointer++] = '\n';
                text[textPointer++] = '\r';
                text[textPointer] = 0;
            }
        }
    }

    /* special gas number #6 in dive mode*/
    if(((line == 0) || (line == 6)))
    {
    	if(actual_menu_content != MENU_SURFACE)
    	{
			text[textPointer++] = '\020';
			text[textPointer++] = TXT_2BYTE;
			if(start == NUM_OFFSET_DILUENT)
				text[textPointer++] = TXT2BYTE_SpecialDiveGasMenuCCR;
			else
				text[textPointer++] = TXT2BYTE_SpecialDiveGasMenu;
			text[textPointer++] = '\n';
			text[textPointer++] = '\r';
			text[textPointer++] = 0;
    	}
    	else	/* switch to bailout selection in surface mode */
    	if((isLoopMode(settingsGetPointer()->dive_mode)) || (stateUsed->diveSettings.ccrOption == 1))
    	{
			text[textPointer++] = '\024';
			if(gasMode == CCGAS_STANDARD)
			{
				textPointer += snprintf(&text[textPointer], 14,"Bailout\n");
			}
			else
			{
				textPointer += snprintf(&text[textPointer], 14,"Diluent\n");
			}
    	}
    }
}


/* Private functions ---------------------------------------------------------*/

/* write_gas() is used in t7.c
 */
uint8_t write_gas(char *text, uint8_t oxygen, uint8_t helium)
{
    uint8_t length;

    if((oxygen == 21) && (helium == 0))
    {
        strcpy(text,"Air");
        length = 3;
    }
    else if(oxygen == 100)
    {
        strcpy(text,"Oxy");
        length = 3;
    }
    else if(helium == 0)
    {
        length = snprintf(text, 7,"NX%u",oxygen);
    }
    else if((oxygen + helium) == 100)
    {
        length = snprintf(text, 7,"HX%u",oxygen);
    }
    else
    {
        length = snprintf(text, 7,"%u/%u",oxygen,helium);
    }

    if(length > 6)
    {
        strcpy(text,"error");
        length = 5;
    }

    return length;
}
