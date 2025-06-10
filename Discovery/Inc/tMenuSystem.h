///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenuSystem.h
/// \brief  Header file of Menu Lines for System settings
/// \author heinrichs weikamp gmbh
/// \date   31-July-2014
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
#ifndef TMENU_SYSTEM_H
#define TMENU_SYSTEM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "gfx_engine.h"

/** @addtogroup Template
	* @{
	*/

void set_CustomsviewsSubpage(uint8_t page);

/* Exported variables --------------------------------------------------------*/


/* Exported functions --------------------------------------------------------*/

uint32_t tMSystem_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext);

#endif /* TMENU_SYSTEM_H */
