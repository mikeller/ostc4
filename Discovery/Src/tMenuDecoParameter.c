///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuDecoParameter.c
/// \brief  Deco Parameter
/// \author heinrichs weikamp gmbh
/// \date   02-Feb-2015
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
#include "tMenu.h"
#include "tStructure.h"
#include "tMenuDecoParameter.h"
#include "text_multilanguage.h"
#include "settings.h"
#include "data_central.h"
#include "unit.h"

#define MDP_TAB (380)

/* Exported functions --------------------------------------------------------*/

uint32_t tMDecoParameters_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    uint8_t decoType, VpmConsveratism, GFlow, GFhigh, aGFlow, aGFhigh;
    int8_t mode;
    uint8_t textPointer;
    uint8_t	lastStop;
    char decotypeTxtId;

    textPointer = 0;
    *tab = MDP_TAB;
    *subtext = 0;

    if(actual_menu_content != MENU_SURFACE)
    {
      const SDiveSettings * pDiveSettings;

        if(actual_menu_content == MENU_DIVE_REAL)
        {
            const SDiveState * pState = stateRealGetPointer();
            pDiveSettings = &pState->diveSettings;
        }
        else
        {
            const SDiveState * pState = stateSimGetPointer();
            pDiveSettings = &pState->diveSettings;
        }

        if(pDiveSettings->deco_type.ub.standard == VPM_MODE)
            mode = VPM_MODE;
        else
    if((pDiveSettings->gf_high == settingsGetPointer()->GF_high) && (pDiveSettings->gf_low == settingsGetPointer()->GF_low))
            mode = GF_MODE;
        else
            mode = -1;


        VpmConsveratism = pDiveSettings->vpm_conservatism;
        GFlow = settingsGetPointer()->GF_low;
        GFhigh = settingsGetPointer()->GF_high;
        aGFlow = settingsGetPointer()->aGF_low;
        aGFhigh = settingsGetPointer()->aGF_high;

        text[textPointer++] = '\006' - (mode == VPM_MODE);
        text[textPointer++] = ' ';
        textPointer += snprintf(&text[textPointer], 60,\
            "VPM"
            "\t"
            "+"
            "%u"
            "\n\r"
            , VpmConsveratism
        );

        text[textPointer++] = '\006' - (mode == GF_MODE);
        text[textPointer++] = ' ';
        textPointer += snprintf(&text[textPointer], 60,\
            "GF"
            "\016\016"
            "low/high"
            "\017"
            "\t"
            "%u"
            "/"
            "%u"
            "\n\r"
            , GFlow, GFhigh
        );

        text[textPointer++] = '\006' - (mode == -1);
        text[textPointer++] = ' ';
        textPointer += snprintf(&text[textPointer], 60,\
            "aGF"
            "\016\016"
            "low/high"
            "\017"
            "\t"
            "%u"
            "/"
            "%u"
            "\n\r"
            , aGFlow, aGFhigh
        );

        text[textPointer++] = 0;
        return StMDECOP;
    }

    SSettings *data = settingsGetPointer();

    decoType = settingsGetPointer()->deco_type.ub.standard;
    VpmConsveratism = data->VPM_conservatism.ub.standard;
    GFlow = data->GF_low;
    GFhigh = data->GF_high;
    aGFlow = data->aGF_low;
    aGFhigh = data->aGF_high;
    lastStop = data->last_stop_depth_meter;

    if((line == 0) || (line == 1))
    {
        if(decoType == VPM_MODE)
            decotypeTxtId = TXT_VPM;
        else
            decotypeTxtId = TXT_ZHL16GF;

        textPointer += snprintf(&text[textPointer], 60,\
            "%c"
            "\t          "
            "%c"
            , TXT_DecoAlgorithm
            , decotypeTxtId
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 2))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "VPM"
        	"\t          "
            "+"
            "%u"
            , VpmConsveratism
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 3))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "GF"
            "\016\016"
            "low/high"
            "\017"
        	"\t          "
            "%u"
            "/"
            "%u"
            , GFlow, GFhigh
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 4))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "aGF"
            "\016\016"
            "low/high"
            "\017"
            "\t          "
            "%u"
            "/"
            "%u"
            , aGFlow, aGFhigh
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 5))
    {
        textPointer += snprintf(&text[textPointer], 60,\
            "%c"
            "\t          "
            "%u"
            "\016\016"
            " %c%c"
            "\017"
            , TXT_LastDecostop
            , unit_depth_integer(lastStop)
            , unit_depth_char1()
            , unit_depth_char2()
        );
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;
    if((line == 0) || (line == 6))
    {
    	textPointer+= snprintf(&text[textPointer], 60, "%c%c\t          ",TXT_2BYTE,TXT2BYTE_VpmTable);
        if(settingsGetPointer()->VPM_conservatism.ub.alternative)
            text[textPointer++] = '\005';
        else
            text[textPointer++] = '\006';
        text[textPointer] = 0;
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    return StMDECOP;
}
