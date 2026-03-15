///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/hud.h
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

#ifndef HUD_H
#define HUD_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "text_multilanguage.h"

#define NUM_OF_HUD_FCT				(4u)	/* number of different functions which may be displayed at HUD at once */
#define HUD_INFO_DATA_LENGTH		(24u)	/* expected number of received info data */
#define HUD_INFO_INFOSTR_LENGTH		(16u)


#define HUD_INFO_INFOSTR_OFFSET		(3u)	/* offset of info string */
#define HUD_INFO_VERSION_OFFSET		(19u)	/* offset of version byte */

enum hudFct
{
	HUD_FCT_NONE		= 0,	/* Slot will not be used */
	HUD_FCT_WARNING,			/* Show common warning indicator */
	HUD_FCT_PPO2SUM,			/* Show combined PPO2 status */
	HUD_FCT_PPO2_0,				/* Show individual PPO2 status (sensor 0) */
	HUD_FCT_PPO2_1,				/* Show individual PPO2 status (sensor 1) */
	HUD_FCT_PPO2_2,				/* Show individual PPO2 status (sensor 2) */
	HUD_FCT_ASCENT_SPEED,		/* Show indicator for ascent speed */
	HUD_FCT_DECO,				/* Show deco / depth indicator */
	HUD_FCT_END
};

void hud_Init(void);
void hud_GetString(uint8_t id, uint8_t* pText);
uint8_t hud_NextFct(uint8_t curFct, uint8_t fctId);
void hud_UpdateStatus(void);
uint8_t hud_IsActive(void);
uint8_t hud_GetAddress(void);

#endif /* HUD_H */

