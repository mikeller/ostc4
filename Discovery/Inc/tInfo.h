///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tInfo.h
/// \brief  Header file of Info Pages on left side
/// \author heinrichs weikamp gmbh
/// \date   11-Aug-2014
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
#ifndef TINFO_H
#define TINFO_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "base.h"
#include "gfx.h"
#include "gfx_colors.h"
#include "gfx_engine.h"
#include "text_multilanguage.h"
#include "tInfoLog.h"
#include "data_central.h"

/* Exported functions --------------------------------------------------------*/

void tI_init(void);
void openInfo(uint32_t modeToStart);
void tInfo_refresh(void);

void sendActionToInfo(uint8_t sendAction);

void tInfo_write_content_simple(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text, uint8_t color);

void tInfo_write_label_fix(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char textId);
void tInfo_write_label_var(uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle,  const tFont *Font, const char *text);

void tInfo_write_field_button(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text);
void tInfo_write_field_symbol(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1);
void tInfo_write_field_toggle(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1,  uint8_t int2);
void tInfo_write_field_on_off(uint32_t editID, uint16_t XleftGimpStyle, uint16_t XrightGimpStyle, uint16_t YtopGimpStyle, const tFont *Font, const char *text, uint8_t int1);

void tInfo_write_buttonTextline(GFX_DrawCfgScreen *screenPtr, uint8_t left2ByteCode, char middle2ByteCode, char right2ByteCode);
void tInfo_write_buttonTextline_simple(uint8_t left2ByteCode, char middle2ByteCode, char right2ByteCode);

void tInfo_setEvent(uint32_t inputEventID, uint32_t inputFunctionCall);

void tInfo_set_on_off(uint32_t editID, uint8_t int1);
void exitInfo(void);
void exitInfoToBack(void);

#endif /* TINFO_H */
