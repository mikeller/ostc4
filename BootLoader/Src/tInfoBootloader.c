///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   BootLoader/Src/tInfoBootloader.c
/// \brief  Write something on the screen in between steps
/// \author heinrichs weikamp gmbh
/// \date   08-May-2015
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
#include "tInfoBootloader.h"

#include "base_bootloader.h"
#include "gfx_colors.h"
#include "gfx_engine.h"
#include "gfx_fonts.h"
#include "ostc.h"

#include <string.h>
/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

	GFX_DrawCfgScreen	tIBscreen;
	GFX_DrawCfgWindow	tIBwindow;
	uint8_t line = 1;

	char textButtonLeft[30] = { 0 };
	char textButtonMid[31] = { 0 };
	char textButtonRight[31] = { 0 };

/* Private types -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

void tInfoBootloader_init(void)
{
	tIBscreen.FBStartAdress = 0;
	tIBscreen.ImageHeight = 480;
	tIBscreen.ImageWidth = 800;
	tIBscreen.LayerIndex = 1;

	tIBwindow.Image = &tIBscreen;
	tIBwindow.WindowNumberOfTextLines = 6;
	tIBwindow.WindowLineSpacing = 65;
	tIBwindow.WindowTab = 400;
	tIBwindow.WindowX0 = 20;
	tIBwindow.WindowX1 = 779;
	tIBwindow.WindowY0 = 0;
	tIBwindow.WindowY1 = 799;

	line = 1;
}


void tInfo_button_text(const char *text_left, const char *text_mid, const char *text_right)
{
	if(text_left)
		strncpy(textButtonLeft,text_left,30);
	if(text_mid)
	{
		textButtonMid[0] = '\001';
		strncpy(&textButtonMid[1],text_mid,30);
	}
	if(text_right)
	{
		textButtonRight[0] = '\002';
		strncpy(&textButtonRight[1],text_right,30);
	}
}


void tInfo_newpage(const char *text)
{
	uint32_t backup  = tIBscreen.FBStartAdress;

	tIBscreen.FBStartAdress = getFrame(18);
	line = 1;
	if(text)
		GFX_write_string(&FontT48min, &tIBwindow, text,line);
	line++;

	if(*textButtonLeft)
		write_content_simple(&tIBscreen, 0, 800, 480-24, &FontT24min,textButtonLeft,CLUT_ButtonSurfaceScreen);
	if(*textButtonMid)
		write_content_simple(&tIBscreen, 0, 800, 480-24, &FontT24min,textButtonMid,CLUT_ButtonSurfaceScreen);
	if(*textButtonRight)
		write_content_simple(&tIBscreen, 0, 800, 480-24, &FontT24min,textButtonRight,CLUT_ButtonSurfaceScreen);

	GFX_SetFrameTop(tIBscreen.FBStartAdress);
	GFX_change_LTDC();

	if(backup != 0)
			releaseFrame(18,backup);
}


void tInfo_write(const char *text)
{
	if((line > 6) || (tIBscreen.FBStartAdress == 0))
		tInfo_newpage(text);
	else
	{
		if(text)
			GFX_write_string(&FontT48min, &tIBwindow, text,line);
		line++;

	}
}

/* Private functions ---------------------------------------------------------*/
