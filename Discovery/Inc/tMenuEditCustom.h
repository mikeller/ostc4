///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenuEditCustom.h
/// \brief  Header file for editing custom view Settings
/// \author heinrichs weikamp gmbh
/// \date   05-Aug-2014
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TMENU_EDIT_CUSTOM_H
#define TMENU_EDIT_CUSTOM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "gfx_engine.h"
#include "settings.h"
#include "data_central.h"

enum afDetectionState
{
	AF_VIEW_NOCHANGE		= 0,
	AF_VIEW_ACTIVATED,
	AF_VIEW_DEACTIVATED
};


void openEdit_Custom(uint8_t line);
void openEdit_CustomviewDivemode(const uint8_t* pcv_changelist);
void openEdit_CustomviewDivemodeMenu(uint8_t line);
void CustomviewDivemode_refresh();
void refresh_ViewPort(void);
uint8_t HandleAFCompass(void);

#endif /* TMENU_EDIT_CUSTOM_H */
