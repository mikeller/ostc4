///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tempstick.h
/// \brief
/// \author Heinrichs Weikamp
/// \date   2026
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2026 Heinrichs Weikamp gmbh
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

#ifndef TEMPSTICK_H
#define TEMPSTICK_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEMPSTICK_LOG_SIZE 300
#define TEMPSTICK_MAX_VALUE	254


void tempstick_Init(void);
void tempstick_LogData(uint8_t simulated);
uint16_t tempstick_GetDataLength(void);
uint16_t* tempstickLog_GetDataPointer(uint8_t sensorId);
uint8_t* tempstick_GetActiveCntPointer();

#endif /* TEMPSTICK_H */

