///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenuEditSetpoint.h
/// \brief
/// \author heinrichs weikamp gmbh
/// \date   19-Dec-2014
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
#ifndef TMENU_EDIT_SETPOINT_H
#define TMENU_EDIT_SETPOINT_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

void openEdit_Setpoint(uint8_t line);
void openEdit_DiveSelectBetterSetpoint(bool useLastDiluent);

void checkSwitchToLoop(void);
bool findSwitchToSetpoint(void);
uint8_t getSwitchToSetpointCbar(void);
void checkSwitchSetpoint(void);
#endif /* TMENU_EDIT_SETPOINT_H */
