///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/gfx_colors.h
/// \brief  Header file of gfx_ccolors.h with all color RGBs defined
/// \author heinrichs weikamp gmbh
/// \date   02-May-2014
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
#ifndef GFX_COLORS_H
#define GFX_COLORS_H

/* Includes ------------------------------------------------------------------*/

#include "stdint.h"

/* Exported types ------------------------------------------------------------*/

typedef enum
{
	CLUT_Font020 = 0,
	CLUT_Font021,
	CLUT_Font022,
	CLUT_Font023,
	CLUT_WarningYellow,
	CLUT_WarningRed,
	CLUT_NiceGreen,
	CLUT_Font027, // DIVE Main
	CLUT_Font030, // Plugin Box Main
	CLUT_Font031, // Plugin Box Grey
	CLUT_Font032, // DIVE_LabelColor
	CLUT_DIVE_FieldSeperatorLines,
	CLUT_DIVE_pluginbox,
	CLUT_NiceBlue,
	CLUT_DIVE_SPARE22,
	CLUT_DiveMainLabel,
	CLUT_pluginboxSurface,
	CLUT_MenuLineUnselected,
	CLUT_MenuLineUnselectedSeperator,
	CLUT_MenuLineSelected,
	CLUT_MenuEditLineSelected,
	CLUT_MenuLineSelectedSides,
	CLUT_MenuLineSelectedSeperator,
	CLUT_MenuTopBackground,
	CLUT_Group0,
	CLUT_MenuEditCursor,
	CLUT_MenuEditInfo,
	CLUT_MenuEditActive,
	CLUT_MenuEditButtonColor1,
	CLUT_MenuEditFieldRegular,
	CLUT_MenuEditFieldSelected,
	CLUT_MenuEditField0,
	CLUT_MenuEditField1,
	CLUT_MenuEditField2,
	CLUT_MenuEditField3,
	CLUT_MenuEditField4,
	CLUT_MenuEditField5,
	CLUT_MenuEditField6,
	CLUT_MenuEditField7,
	CLUT_MenuEditField8,
	CLUT_MenuEditField9,
	CLUT_MenuEditDigit,
	CLUT_MenuPageGasOC,
	CLUT_MenuPageGasCC,
	CLUT_MenuPageGasSP,
	CLUT_MenuPageXtra,
	CLUT_MenuPageDeco,
	CLUT_MenuPageDecoParameter,
	CLUT_MenuPageHardware,
	CLUT_MenuPageSystem,
	CLUT_MenuPageCustomView,
	CLUT_MenuPageCvOption,
	CLUT_MenuPageDivePlanner,
	CLUT_ButtonSymbols,
	CLUT_InfoSurface,
	CLUT_InfoDive,
	CLUT_InfoCompass,
	CLUT_InfoCursor,
	CLUT_InfoInActive,
	CLUT_InfoActive,
	CLUT_InfoButtonColor1,
	CLUT_InfoFieldRegular,
	CLUT_InfoFieldSelected,
	CLUT_InfoField0,
	CLUT_InfoField1,
	CLUT_InfoField2,
	CLUT_InfoField3,
	CLUT_InfoField4,
	CLUT_InfoField5,
	CLUT_InfoField6,
	CLUT_InfoField7,
	CLUT_InfoField8,
	CLUT_InfoField9,
	CLUT_InfoPageLogbook,
	CLUT_LogbookGrid,
	CLUT_LogbookText,
	CLUT_LogbookTemperature,
	CLUT_GasSensor0,
	CLUT_GasSensor1,
	CLUT_GasSensor2,
	CLUT_GasSensor3,
	CLUT_GasSensor4,
	CLUT_GasSensor5,
	CLUT_GasSensor6,
	CLUT_GasSensor7,
	CLUT_GasSensor8,
	CLUT_GasSensor9,
	CLUT_GasSensor10,
	CLUT_GasSensor11,
	CLUT_GasSensor12,
	CLUT_GasSensor13,
	CLUT_GasSensor14,
	CLUT_GasSensor15,
	CLUT_CompassUserBackHeadingTick,
	CLUT_CompassSubTick,
	CLUT_CompassNorthTick,
	CLUT_CompassUserHeadingTick,
	CLUT_EverythingOkayGreen,
	CLUT_ButtonSurfaceScreen,
	CLUT_BatteryStandard,
	CLUT_BatteryCharging,
	CLUT_BatteryProblem,
	CLUT_Colorscheme0,
	CLUT_Colorscheme0x1,
	CLUT_Colorscheme0x2,
	CLUT_Colorscheme0x3,
	CLUT_Colorscheme0x4,
	CLUT_Colorscheme0x5,
	CLUT_Colorscheme0x6,
	CLUT_Colorscheme0x7,
	CLUT_Colorscheme1,
	CLUT_Colorscheme1x1,
	CLUT_Colorscheme1x2,
	CLUT_Colorscheme1x3,
	CLUT_Colorscheme1x4,
	CLUT_Colorscheme1x5,
	CLUT_Colorscheme1x6,
	CLUT_Colorscheme1x7,
	CLUT_Colorscheme2,
	CLUT_Colorscheme2x1,
	CLUT_Colorscheme2x2,
	CLUT_Colorscheme2x3,
	CLUT_Colorscheme2x4,
	CLUT_Colorscheme2x5,
	CLUT_Colorscheme2x6,
	CLUT_Colorscheme2x7,
	CLUT_Colorscheme3,
	CLUT_Colorscheme3x1,
	CLUT_Colorscheme3x2,
	CLUT_Colorscheme3x3,
	CLUT_Colorscheme3x4,
	CLUT_Colorscheme3x5,
	CLUT_Colorscheme3x6,
	CLUT_Colorscheme3x7,
	CLUT_END
} SCLUT_Content;


/* Exported variables --------------------------------------------------------*/

extern uint32_t ColorLUT[];

#endif /* GFX_COLORS_H */
