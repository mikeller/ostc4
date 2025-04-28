///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenuEditCvOption.h
/// \brief  Header file for editing Hardware Settings
/// \author heinrichs weikamp gmbh
/// \date   24-Apr-2025
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2025 Heinrichs Weikamp gmbh
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
#ifndef TMENU_EDIT_CVOPTION_H
#define TMENU_EDIT_CVOPTION_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "gfx_engine.h"
#include "global_constants.h"
#include "settings.h"
#include "data_central.h"

void openEdit_CvOption(uint8_t line);
void refresh_CompassEdit(void);
uint32_t tMCvOption_refresh(uint8_t line, char *text, uint16_t *tab, char *subtext);
void tMCvOption_checkLineStatus(void);

#endif /* TMENU_EDIT_CVOPTION_H */
