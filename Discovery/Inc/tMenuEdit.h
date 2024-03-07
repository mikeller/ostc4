///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenuEdit.h
/// \brief  Header file for Menu Setting Modifications
/// \author heinrichs weikamp gmbh
/// \date   04-July-2014
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
#ifndef TMENU_EDIT_H
#define TMENU_EDIT_H

/* Includes ------------------------------------------------------------------*/
#include "tMenu.h"

#define ME_Y_LINE1					(70)
#define ME_Y_LINE_STEP				(65)
#define ME_Y_LINE_BASE				(ME_Y_LINE1 - ME_Y_LINE_STEP)
#define ME_Y_LINE2					(ME_Y_LINE1 + (1 * ME_Y_LINE_STEP))
#define ME_Y_LINE3					(ME_Y_LINE1 + (2 * ME_Y_LINE_STEP))
#define ME_Y_LINE4					(ME_Y_LINE1 + (3 * ME_Y_LINE_STEP))
#define ME_Y_LINE5					(ME_Y_LINE1 + (4 * ME_Y_LINE_STEP))
#define ME_Y_LINE6					(ME_Y_LINE1 + (5 * ME_Y_LINE_STEP))

#define EXIT_TO_INFO_PREDIVE 		(245)
#define EXIT_TO_NEXT_MENU   		(246)
#define EXIT_TO_INFO_SENSOR 		(247)
#define EXIT_TO_INFO_COMPASS 		(248)
#define EXIT_TO_MENU_WITH_LOGO		(249)
#define EXIT_TO_HOME 				(250)
#define EXIT_TO_MENU 				(251)
#define UPDATE_DIVESETTINGS 		(252)
#define UPDATE_AND_EXIT_TO_HOME 	(253)
#define UPDATE_AND_EXIT_TO_MENU 	(255)
#define UNSPECIFIC_RETURN			(254)

typedef union {
    uint32_t uint32;
    int32_t int32;
} input_u;

void sendActionToMenuEdit(uint8_t sendAction);

void tMenuEdit_init(void);
void resetMenuContentStructure();
void resetMenuStructure(uint8_t color);
void resetMenuEdit(uint8_t color);
void tMenuEdit_refresh_live_content(void);
void tMenuEdit_refresh_field(uint32_t editID);
void tMenuEdit_select(uint32_t editID);

void evaluateNewString	(uint32_t editID, uint32_t *pNewValue1, uint32_t *pNewValue2, uint32_t *pNewValue3, uint32_t *pNewValue4);
void tMenuEdit_newInput	(uint32_t editID, uint32_t int1, uint32_t int2, uint32_t int3, uint32_t int4);
void tMenuEdit_newButtonText(uint32_t editID, char *text);
void tMenuEdit_set_on_off(uint32_t editID, uint32_t int1);

void write_label_fix(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char textId);
void write_label_var(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text);

void clean_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font);
void write_content(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text, uint8_t color);

void write_topline( char *text);
void write_buttonTextline( uint8_t left2ByteCode, char middle2ByteCode, char right2ByteCode);
void write_field_udigit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4);
void write_field_2digit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4);
void write_field_3digit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint32_t int1,  uint32_t int2,  uint32_t int3,  uint32_t int4);
void write_field_select(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1,  uint8_t int2,  uint8_t int3,  uint8_t int4);
void write_field_sdigit(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, int32_t int1, int32_t int2, int32_t int3, int32_t int4);
void write_field_button(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text);
void write_field_symbol(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1);
void write_field_toggle(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1,  uint8_t int2);
void write_field_on_off(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1);
void write_field_fpoint(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, float input);

void stop_cursor_fields(void);

void setEvent(uint32_t inputEventID, uint32_t inputFunctionCall);
void resetEnterPressedToStateBeforeButtonAction(void);

void setBackMenu(uint32_t inputFunctionCall, uint8_t functionCallParameter, uint8_t gotoMenuEditField);
void exitMenuEdit_to_BackMenu(void);
void exitMenuEdit(uint8_t writeSettingsIfEnterPressed);
void startEdit(void);
void exitEditWithUpdate(void);
void exitMenuEdit_to_Home(void);
void exitMenuEdit_to_Home_with_Menu_Update(void);
void exitMenuEdit_to_Menu_with_Menu_Update(void);
void exitMenuEdit_to_Menu_with_Menu_Update_do_not_write_settings_for_this_only(void);
void exitMenuEditField(void);
void tMenuEdit_writeSettingsToFlash(void);

GFX_DrawCfgScreen* getMenuEditScreen(void);
#endif /* TMENU_EDIT_H */
