///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tInfoCompass.h
/// \brief  Infopage content for compass calibration
/// \author heinrichs weikamp gmbh
/// \date   23-Feb-2015
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
#ifndef TINFO_COMPASS_H
#define TINFO_COMPASS_H

#include "gfx.h"

typedef struct
{
	point_t coord;
	point_t eclipse;
	uint8_t check[360];
} axisIndicator_t;

/* Exported functions --------------------------------------------------------*/
void openInfo_Compass(void);
void refreshInfo_Compass(GFX_DrawCfgScreen s);


#endif /* TINFO_COMPASS_H */
