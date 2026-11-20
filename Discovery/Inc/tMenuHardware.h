///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenuHardware.h
/// \brief  Header file of Menu Lines for Hardware related parts
/// \author heinrichs weikamp gmbh
/// \date   05-Aug-2014
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
#ifndef TMENU_HARDWARE_H
#define TMENU_HARDWARE_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "gfx_engine.h"

/* Exported functions --------------------------------------------------------*/

uint32_t tMHardware_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext);
uint8_t tMHardware_getAcquireFixLine(void);

#endif /* TMENU_HARDWARE_H */
