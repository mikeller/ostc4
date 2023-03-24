///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/tHome.h
/// \brief  Control for Surface and Dive Templates
/// \author heinrichs weikamp gmbh
/// \date   10-November-2014
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
#ifndef THOME_H
#define THOME_H

/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "base.h"
#include "gfx.h"
#include "gfx_colors.h"
#include "gfx_engine.h"
#include "text_multilanguage.h"
#include "settings.h"
#include "data_central.h"
#include "data_exchange.h"
#include "check_warning.h"
#include "configuration.h"

extern const uint16_t BigFontSeperationLeftRight; // in t3.c
extern const uint16_t BigFontSeperationTopBottom; // in t3.c

#define MAX_AGE_DECOINFO_MS (120000)

enum EXTRADISPLAYS
{
		EXTRADISPLAY_none = 0,
		EXTRADISPLAY_BIGFONT,
		EXTRADISPLAY_BFACTIVE,
#ifdef HAVEDECOGAME
		EXTRADISPLAY_DECOGAME,
#endif
		EXTRADISPLAY_END
};

enum CUSTOMVIEWS
{
		CVIEW_noneOrDebug = 0,
		CVIEW_sensors,
		CVIEW_Compass,
		CVIEW_Decolist,
		CVIEW_Tissues,
		CVIEW_Profile,
		CVIEW_EADTime,
		CVIEW_Gaslist,
		CVIEW_sensors_mV,
		CVIEW_Hello,
		CVIEW_CompassDebug,
		CVIEW_SummaryOfLeftCorner,
		CVIEW_Charger,
		CVIEW_CcrSummary,
		CVIEW_END,
		CVIEW_T3_Decostop,
		CVIEW_T3_TTS,
		CVIEW_T3_MaxDepth,
		CVIEW_T3_ppO2andGas,
		CVIEW_T3_StopWatch,
		CVIEW_T3_GasList,
		CVIEW_T3_Temperature,
		CVIEW_T3_ApnoeSurfaceInfo,
		CVIEW_T3_Navigation,
		CVIEW_T3_DepthData,
		CVIEW_T3_DecoTTS,
#ifdef ENABLE_T3_PROFILE_VIEW
		CVIEW_T3_Profile,
#endif
		CVIEW_T3_END
};

// for custom view switch on/off 161122 hw
extern const uint8_t cv_changelist[];
extern const uint8_t cv_changelist_BS[];

#define CHECK_BIT_THOME(var,pos) (((var)>>(pos)) & 1)

typedef struct
{
	uint16_t Hours;
	uint16_t Minutes;
	uint16_t Seconds;
	uint32_t Total;

}	SDivetime;


typedef struct
{
	uint8_t Days;
	uint8_t Hours;
	uint8_t Minutes;
	uint8_t Seconds;
	uint32_t Total;
}	SSurfacetime;


extern _Bool warning_count_high_time;
extern _Bool display_count_high_time;
extern uint8_t errorsInSettings;


void tHome_init(void);
void tHome_refresh(void);
void tHome_sleepmode_fun(void);
void set_globalState_tHome(void);
void tHome_change_field_button_pressed(void);
void tHome_change_customview_button_pressed(uint8_t action);

void tHome_findNextStop(const uint16_t *list, uint8_t *depthOut, uint16_t *lengthOut);
void tHomeDiveMenuControl(uint8_t sendAction);
void tHome_tick(void);

uint8_t tHome_gas_writer(uint8_t oxygen_percentage, uint8_t helium_percentage, char *text);
uint32_t tHome_DateCode(RTC_DateTypeDef *dateInput);

void tHome_init_compass(void);
uint8_t tHome_getNumberOfAvailableCVs(const uint8_t* pcv_list);

float t3_basics_lines_depth_and_divetime(GFX_DrawCfgScreen *tXscreen, GFX_DrawCfgWindow* tXl1, GFX_DrawCfgWindow* tXr1, uint8_t mode);
void t3_basics_battery_low_customview_extra(GFX_DrawCfgWindow* tXc1);
void t3_basics_show_customview_warnings(GFX_DrawCfgWindow* tXc1);
void t3_basics_refresh_customview(float depth, uint8_t tX_selection_customview, GFX_DrawCfgScreen *tXscreen, GFX_DrawCfgWindow* tXc1, GFX_DrawCfgWindow* tXc2, uint8_t mode);
void t3_basics_refresh_apnoeRight(float depth, uint8_t tX_selection_customview, GFX_DrawCfgScreen *tXscreen, GFX_DrawCfgWindow* tXc1, GFX_DrawCfgWindow* tXc2, uint8_t mode);
//void _findNextStop(const uint16_t *list, uint8_t *depthOut, uint16_t *lengthOut);
void t3_basics_colorscheme_mod(char *text);
void t3_basics_change_customview(uint8_t *tX_selection_customview, const uint8_t *tX_customviews, uint8_t action);

uint8_t tHome_show_lost_connection_count(GFX_DrawCfgScreen *ScreenToWriteOn);

#endif /* THOME_H */
