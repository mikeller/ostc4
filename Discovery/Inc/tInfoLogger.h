///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tInfoSensor.h
/// \brief  Infopage content for visualisation of UART protocol flow
/// \author heinrichs weikamp gmbh
/// \date   17-07-2025
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
#ifndef TINFO_LOGGER_H
#define TINFO_LOGGER_H

#define LINE_HEADER_BYTES (3u)
#define MAX_CHAR_PER_LINE (60u)
#define MAX_LOGGER_LINES  (13u)

#define LOG_TX_LINE	(0u)
#define LOG_RX_LINE (1u)

#include "gfx_engine.h"

/* Exported functions --------------------------------------------------------*/
void openInfo_Logger();
void refreshInfo_Logger(GFX_DrawCfgScreen s);
void sendActionToInfoLogger(uint8_t sendAction);
void InfoLogger_writeLine(uint8_t* pLine,uint8_t lineLength,uint8_t direction);
uint8_t InfoLogger_isUpdated();

#endif /* TINFO_LOGGER_H */
