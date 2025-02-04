///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/t3.h
/// \brief  Header file of Divemode with 3 windows plus plugin
/// \author heinrichs weikamp gmbh
/// \date   10-Nov-2014
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
#ifndef T3_H
#define T3_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "gfx_engine.h"
#include "settings.h"

/* Exported functions --------------------------------------------------------*/
void t3_init(void);
void t3_refresh(void);
void t3_select_customview(uint8_t selectedCustomview);
uint8_t t3_change_customview(uint8_t action);
uint8_t t3_GetEnabled_customviews(void);
uint8_t t3_getCustomView(void);
void t3_set_customview_to_primary(void);
uint8_t t3_customview_disabled(uint8_t view);
void t3_handleAutofocus(void);

int printScrubberText(char *text, size_t size, const SScrubberData *scrubberData, SSettings *settings);

#endif /* T3_H */
