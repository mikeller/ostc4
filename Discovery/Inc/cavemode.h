///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/cavemode.h
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

#ifndef CAVEMODE_H
#define CAVEMODE_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>


typedef enum
{
		CAVEMODE_OFF = 0,
		CAVEMODE_RECORDING,
		CAVEMODE_RECORDING_PAUSE,
		CAVEMODE_RETURNING,
		CAVEMODE_RETURNING_PAUSE,
		CAVEMODE_END
} cavemodeState_t;

void caveMode_Init(void);
void caveMode_Update(SDiveState *pDiveState);
uint32_t caveMode_GetTTS(void);
void caveMode_SetActive(uint8_t activeRequest);
void caveMode_SetReturn(uint8_t returnRequest);
uint8_t caveMode_isReturning(void);
uint8_t caveMode_isActive(void);
uint8_t caveMode_isOff(void);
uint8_t caveMode_isLiveDive(void);
void caveMode_SyncToMarker(void);
void caveMode_NotifyCompression();
#endif /* CAVEMODE_H */

