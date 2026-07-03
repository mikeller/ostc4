///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tMenuCvOption.h
/// \brief  Header file text line provider functions
/// \author heinrichs weikamp gmbh
/// \date   03-Feb-2026
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TMENU_CVOPTIONTEXT_H
#define TMENU_CVOPTIONTEXT_H

/* Includes ------------------------------------------------------------------*/
/** @addtogroup Template
	* @{
	*/
#include <stdint.h>

enum CVOPTIONS						/* the order defines the priority as well */
{
		CVOPT_Compass = 0,
		CVOPT_CompassStyle,
		CVOPT_HUD,
		CVOPT_O2_Sensor,
		CVOPT_CO2_Sensor,
		CVOPT_Pressure_Sensor,
		CVOPT_Tempstick,
		CVOPT_Cave,
		CVOPT_Timer,
		CVOPT_END
};

typedef uint8_t (*refreshFunc_t)(char *);

/* Exported variables --------------------------------------------------------*/



/* Exported functions --------------------------------------------------------*/

uint8_t tMCvOptText_BuildDynamicContentList();
refreshFunc_t* tMCvOptText_GetTable();
uint8_t tMCvOptText_GetTableItemCnt();

uint8_t tMCvOptText_refreshCompass(char* pText);
uint8_t tMCvOptText_refreshCompassStyle(char* pText);
uint8_t tMCvOptText_refreshTimer(char* pText);
uint8_t tMCvOptText_refreshO2(char* pText);
uint8_t tMCvOptText_refreshCO2(char* pText);
uint8_t tMCvOptText_refreshCave(char* pText);

#endif /* TMENU_CVOPTIONTEXT_H */
