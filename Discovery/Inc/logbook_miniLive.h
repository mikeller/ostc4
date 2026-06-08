///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/logbook_miniLive.h
/// \brief  little logbook for during the dive
/// \author heinrichs weikamp gmbh
/// \date   13-March-2015
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
#ifndef LOGBOOK_MINI_LIVE_H
#define LOGBOOK_MINI_LIVE_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported functions --------------------------------------------------------*/

void updateMiniLiveLogbook( _Bool checkOncePerSecond);
uint16_t* getMiniLiveLogbookPointerToData(void);
uint16_t getMiniLiveLogbookActualDataLength(void);
uint16_t* getMiniLiveReplayPointerToData(uint8_t forceRetModData);
uint16_t* getMiniLiveDecoPointerToData(void);
uint16_t getMiniLiveReplayLength(void);
uint16_t getMiniLiveModLength(void);
uint8_t prepareReplayLog(uint8_t StepBackwards);
uint8_t getReplayInfo(uint16_t** pReplayData, uint8_t** pReplayMarker, uint16_t* DataLength, uint16_t* MaxDepth, uint16_t* diveMinutes);
uint16_t getReplayDataResolution(void);
uint16_t getReplayOffset(void);
uint16_t MiniLiveLogbook_getNextMarkerIndex(uint16_t curIndex);
void MiniLiveLogbook_checkMarker(void);
void MiniLiveLogbook_mirrowMiniLiveToReplayLog(void);
void MiniLiveLogbook_copyReplayToModLive(void);
void MiniLiveLogbook_resetModData(void);
void MiniLiveLogbook_releaseModData(void);
void MiniLiveLogbook_insertData(uint16_t targetIndex,uint16_t depth, uint16_t timeSec);

#endif /* LOGBOOK_MINI_LIVE_H */
