///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/tMenuCustom.c
/// \brief  Menu Custom - Provide access to custom view options
/// \author heinrichs weikamp gmbh
/// \date   25-Aug-2020
///
/// \details
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2020 Heinrichs Weikamp gmbh
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
#include "tHome.h"
#include "tStructure.h"
#include "tMenuCustom.h"
#include "text_multilanguage.h"
#include "data_central.h"
#include "motion.h"
#include "gfx_fonts.h"
#include "tInfo.h"
#include "motion.h"


/* Exported functions --------------------------------------------------------*/

uint32_t tMCustom_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext)
{
    uint8_t textPointer;

    textPointer = 0;
    *tab = 550;
    *subtext = 0;

    if((line == 0) || (line == 1))
    {
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_Customviews;
    }
    text[textPointer++] = '\n';
    text[textPointer++] = '\r';
    text[textPointer] = 0;

    if((line == 0) || (line == 2))
    {
       text[textPointer++] = TXT_2BYTE;
       text[textPointer++] = TXT2BYTE_ExtraDisplay;
       text[textPointer++] = ' ';
       text[textPointer++] = ' ';
       text[textPointer++] = TXT_2BYTE;

       switch(settingsGetPointer()->extraDisplay)
       {
		   /* BigFont */
		   case EXTRADISPLAY_BIGFONT:
			   text[textPointer++] = TXT2BYTE_ExtraBigFont;
			   break;
		   /* Start screen */
		   case EXTRADISPLAY_BFACTIVE:
			   text[textPointer++] = TXT2BYTE_ExtraActive;
			   break;
		   /* none */
		   case EXTRADISPLAY_none:
			   text[textPointer++] = TXT2BYTE_ExtraNone;
			   break;

		   default:
			   snprintf(&text[textPointer++],4,"%u",settingsGetPointer()->extraDisplay);
       break;
       }
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 3))
    {
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_SelectCustomviews;
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

    if((line == 0) || (line == 4))
    {
        text[textPointer++] = TXT_2BYTE;
        text[textPointer++] = TXT2BYTE_SelectBigFont;
    }
    strcpy(&text[textPointer],"\n\r");
    textPointer += 2;

#ifdef ENABLE_MOTION_CONTROL
    if((line == 0) || (line == 5))
    {
    /* MotionCtrl */
		text[textPointer++] = TXT_2BYTE;
		text[textPointer++] = TXT2BYTE_MotionCtrl;
    }

	strcpy(&text[textPointer],"\n\r");
	textPointer += 2;
#endif

#ifdef ENABLE_GPIO_V2
    if((line == 0) || (line == 5))
    {
    /* MotionCtrl */
		text[textPointer++] = TXT_2BYTE;
		text[textPointer++] = TXT2BYTE_BUZZER;
		text[textPointer++] = ' ';
		text[textPointer++] = TXT_Warning;
		text[textPointer++] = '\t';
	    if(settingsGetPointer()->warningBuzzer)
	            text[textPointer++] = '\005';
	        else
	            text[textPointer++] = '\006';
    }

	strcpy(&text[textPointer],"\n\r");
	textPointer += 2;
#endif

    return StMCustom;
}

