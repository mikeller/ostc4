///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tInfoSensor.h
/// \brief  Infopage content for connected smart sensors
/// \author heinrichs weikamp gmbh
/// \date   17-11-2022
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2022 Heinrichs Weikamp gmbh
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
#ifndef TINFO_PREDIVE_H
#define TINFO_PREDIVE_H

/* Exported functions --------------------------------------------------------*/
void openInfo_PreDive();
void refreshInfo_PreDive(GFX_DrawCfgScreen s);
void sendActionToInfoPreDive(uint8_t sendAction);


#endif /* TINFO_COMPASS_H */
