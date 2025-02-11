///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenu.h
/// \brief  Header file of Divemode with 7 windows plus plugin
/// \author heinrichs weikamp gmbh
/// \date   30-April-2014
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TMENU_H
#define TMENU_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "base.h"
#include "gfx.h"
#include "gfx_colors.h"
#include "gfx_engine.h"
#include "text_multilanguage.h"
#include "settings.h"
#include "data_central.h"
#include "data_exchange.h"

/* Exported types ------------------------------------------------------------*/
enum MENU_MODE
{
	MENU_SURFACE		= 0,
	MENU_DIVE_REAL	= 1,
	MENU_DIVE_SIM 	= 2,
	MENU_UNDEFINED	= 4
};

/* Exported constants --------------------------------------------------------*/

#define MAX_PAGE_TEXTSIZE 65 * 6

/* Exported variables --------------------------------------------------------*/

extern uint8_t actual_menu_content;

/* Exported functions --------------------------------------------------------*/

GFX_DrawCfgScreen * get_PointerMenuCursorScreen(void);
GFX_DrawCfgScreen * get_PointerMenuCursorDesignSoloScreen(void);

void nextline(char * text, uint8_t *textPointer);

void tM_init(void);
void openMenu(uint8_t freshWithFlipPages);
void openMenu_first_page_with_OC_gas_update(void);
void updateMenu(void);
void updateSpecificMenu(uint32_t id);
void sendActionToMenu(uint8_t sendAction);
void exitMenu(void);

void tM_check_content(void);
void tM_refresh_live_content(void);
void tM_rebuild_menu_after_tComm(void);

void tM_refresh(char *text, uint8_t *textPointer, uint8_t line, const char content[6]);
void tM_build_page(uint32_t id, char *text, uint16_t tab, char *subtext);

void block_diluent_page(void);
void unblock_diluent_page(void);

void selectPage(uint32_t selection);

uint8_t getLineMask(uint32_t lineId);
void resetLineMask(uint32_t lineId);
void disableLine(uint32_t lineId);
void enableLine(uint32_t lineId);
void clearDisabledMenuLines(void);

char *makeGrey(bool isGrey);
#endif /* TMENU_H */
