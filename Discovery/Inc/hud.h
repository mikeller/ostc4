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

/* LED_STATUS:						defines LED operation	The left nibble defines the number of pulses and is calculated for a 2s update rate*/

#define HUD_LED_STATUS_OFF			(0x00)	/* OFF */
#define HUD_LED_PERMANENT			(0x01)	/* Permanent switched on */
#define HUD_LED_STATUS_2s2			(0x12)	/* PULSE (4 second period (2000ms ON, 2000ms OFF) */
#define HUD_LED_STATUS_1s1			(0x13)	/* PULSE (2 second period (1000ms ON, 1000ms OFF) */
#define HUD_LED_STATUS_05s05		(0x04)	/* PULSE (1 Second period  (500ms ON, 500ms OFF) */
#define HUD_LED_STATUS_025s025		(0x35)	/* PULSE (0,5 Second period  (250ms ON, 250ms OFF) */
#define HUD_LED_STATUS_0125s0125	(0x06)	/* PULSE (0,25 Second period  (125ms ON, 125ms OFF) */
#define HUD_LED_STATUS_0062s0062	(0x07)	/* PULSE (0,125 Second period  (62,5ms ON, 62,5ms OFF) */
#define HUD_LED_STATUS_1s2			(0x08)	/* PUSLE (3 second period (1000ms ON, 2000ms OFF) */
#define HUD_LED_STATUS_05s15		(0x09)	/* PUSLE (2 second period (500ms ON, 1500ms OFF) */
#define HUD_LED_STATUS_025s075		(0x0A)	/* PUSLE (1 second period (250ms ON, 750ms OFF) */
#define HUD_LED_STATUS_025s0375		(0x0B)	/* PUSLE (0,5 second period (250ms ON, 375ms OFF) */
#define HUD_LED_STATUS_0125s0187	(0x0C)	/* PUSLE (0.25 second period (125ms ON, 187,5ms OFF) */
#define HUD_LED_STATUS_0062s0093	(0x0D)	/* PUSLE (0.125 second period (62,5ms ON, 93,75ms OFF) */

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

