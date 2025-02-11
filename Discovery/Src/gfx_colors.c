/**
  ******************************************************************************
  * @file    gfx_colors.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    02-May-2014
  * @brief   All colors used are defined here
  *
  @verbatim
  ==============================================================================
                        ##### How to use #####
  ==============================================================================
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT(c) 2014 heinrichs weikamp
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gfx_colors.h"

/* Exported variables --------------------------------------------------------*/

/*
	0x0000FF00,   green
	0x00FF0000,    red
	0x000000FF,   blue
	0x00FFFF00,   yellow
	0x0000FFFF,   cyan
	0x00000000,  black
	0x000092D0,  hw light blue
*/


uint32_t ColorLUT[] =
{
	// ARGB
	0x00FFFFFF, // \020 white
	0x00333333, // \021 dark grey
	0x0050FF50, // \022 units
	0x003060FF, // \023 blue
	0x00FFFF00, // \024 + CLUT_WarningYellow
	0x00FF0000, // \025 + CLUT_WarningRed
	0x0000FF00, // \026 + CLUT_NiceGreen
	0x00FFFFFF, // \027 DIVE_MainColorIfNotWhite\020
	0x00FFFFFF, // \030 DIVE_PluginBoxMainColor
	0x00AAAAAA, // \031 DIVE_PluginBoxGrey also used to display deactivation
	0x0050FF50, // \032 DIVE_LabelColor
	0x00FFFFFF, // CLUT_DIVE_FieldSeperatorLines
	0x0050FF50, // CLUT_DIVE_pluginbox // old pink 0x00FF20FF
	0x00FF00FF, // CLUT_NiceBlue descent graph (apnoe)
	0x00FFFFFF, // CLUT_DIVE_SPARE22 - UNUSED
	0x0000FF00, // CLUT_DiveMainLabel - mainly Debug mode
	0x00555555, // CLUT_pluginboxSurface
	0x00101010, // CLUT_MenuLineUnselected
	0x00000000, // CLUT_MenuLineUnselectedSeperator
	0x00000000, // CLUT_MenuLineSelected
	0x00000000, // CLUT_MenuEditLineSelected
	0x008F8F00, // CLUT_MenuLineSelectedSides
	0x008F8F00, // CLUT_MenuLineSelectedSeperator
	0x00000000, // CLUT_MenuTopBackground
	0x00FFFF00, // CLUT_Group0 - UNUSED
	0x00646464, // CLUT_MenuEditCursor,
	0x008C8C8C, // CLUT_MenuEditInfo,
	0x0068F10D, // CLUT_MenuEditActive, - UNUSED
	0x00FFFFFF, // CLUT_MenuEditButtonColor1,
	0x00FFFFFF, // CLUT_MenuEditFieldRegular,
	0x00000000, // CLUT_MenuEditFieldSelected,
	0x00000000, // CLUT_MenuEditField0,
	0x00FFFFFF, // CLUT_MenuEditField1,
	0x00FFFFFF, // CLUT_MenuEditField2,
	0x00FFFFFF, // CLUT_MenuEditField3,
	0x00FFFFFF, // CLUT_MenuEditField4,
	0x00FFFFFF, // CLUT_MenuEditField5,
	0x00FFFFFF, // CLUT_MenuEditField6,
	0x00FFFFFF, // CLUT_MenuEditField7,
	0x00FFFFFF, // CLUT_MenuEditField8,
	0x00FFFFFF, // CLUT_MenuEditField9,
	0x00FFFFFF, // CLUT_MenuEditDigit,
	0x0000AEFF,	// CLUT_MenuPageGasOC
	0x0000AE7D,	// CLUT_MenuPageGasCC
	0x00A7D744,	// CLUT_MenuPageGasSP
	0x00EEAA00, // CLUT_MenuPageXtra
	0x00FF0000, // CLUT_MenuPageDeco
	0x00FFC000,//0x00C4EACC, // CLUT_MenuPageDecoParameter
	0x00AC00ff,//0x00E5AE18, // CLUT_MenuPageHardware
	0x00ff00ff,//0x00E5AEFF, // CLUT_MenuPageSystem 
	0x00FF55FF,//0x00FFFF00, // CLUT_MenuPageCustomView
	0x00C4EACC, // CLUT_MenuPageDivePlanner
	0x00000000, // CLUT_MenuPage10 - UNUSED
	0x00FFFFFF, // CLUT_ButtonSymbols
	0x000092D0, // CLUT_InfoSurface
	0x000092D0, // CLUT_InfoDive - UNUSED
	0x000092D0, // CLUT_InfoCompass
	0x00A0A0A0, // CLUT_InfoCursor
	0x008C8C8C, // CLUT_InfoInActive,
	0x0068F10D, // CLUT_InfoActive,
	0x00FFFFFF, // CLUT_InfoButtonColor1,
	0x00FFFFFF, // CLUT_InfoFieldRegular,
	0x00000000, // CLUT_InfoFieldSelected, - UNUSED
	0x00FFFFFF, // CLUT_InfoField0
	0x00FFFFFF, // CLUT_InfoField1
	0x00FFFFFF, // CLUT_InfoField2
	0x00FFFFFF, // CLUT_InfoField3
	0x00FFFFFF, // CLUT_InfoField4
	0x00FFFFFF, // CLUT_InfoField5
	0x00FFFFFF, // CLUT_InfoField6
	0x00FFFFFF, // CLUT_InfoField7
	0x00FFFFFF, // CLUT_InfoField8
	0x00FFFFFF, // CLUT_InfoField9
	0x003060FF,	// CLUT_InfoPageLogbook
	0x00AAAAAA,	// CLUT_LogbookGrid,
	0x00FFFFFF, // CLUT_LogbookText
	0x00FF0000, // CLUT_LogbookTemperature
	0x00FF8000, // CLUT_GasSensor0
	0x00FFFFFF, // CLUT_GasSensor1
	0x0000FF00, // CLUT_GasSensor2
	0x00FF0000, // CLUT_GasSensor3
	0x000000FF, // CLUT_GasSensor4
	0x00FFFF00, // CLUT_GasSensor5
	0x0000FFFF, // CLUT_GasSensor6
	0x00FF00FF, // CLUT_GasSensor7
	0x00FFFFFF, // CLUT_GasSensor8
	0x00FFFFFF, // CLUT_GasSensor9
	0x00FFFFFF, // CLUT_GasSensor10
	0x00FFFFFF, // CLUT_GasSensor11
	0x00FFFFFF, // CLUT_GasSensor12
	0x00FFFFFF, // CLUT_GasSensor13
	0x00FFFFFF, // CLUT_GasSensor14
	0x00FFFFFF, // CLUT_GasSensor15
	0x00FF5050, // CLUT_CompassUserBackHeadingTick
	0x00888888, // CLUT_CompassSubTick
	0x00FFFFFF, // CLUT_CompassNorthTick
	0x0050FF50, // CLUT_CompassUserHeadingTick
	0x0000FF00, // CLUT_EverythingOkayGreen
	0x000092D0, // CLUT_ButtonSurfaceScreen
	0x00FFFFFF, // CLUT_BatteryStandard
	0x0000FF00, // CLUT_BatteryCharging
	0x00FF8000, // CLUT_BatteryProblem
	0x00FFFFFF, // \027 CLUT_MainColor ---------- colorscheme 0
	0x00FFFFFF, // \030 DIVE_PluginBoxMainColor	 	colorscheme 0
	0x00AAAAAA, // \031 DIVE_PluginBoxGrey				colorscheme 0
	0x0050FF50, // \032 DIVE_LabelColor 					colorscheme 0
	0x00FFFFFF, // CLUT_DIVE_FieldSeperatorLines	colorscheme 0
	0x0050FF50, // CLUT_DIVE_pluginbox						colorscheme 0
	0x00FFFFFF, // CLUT_DIVE_SPARE21							colorscheme 0
	0x00FFFFFF, // CLUT_DIVE_SPARE22							colorscheme 0
	0x00F29400, // CLUT_MainColor --------------- colorscheme 1
	0x00F29400, // \030 DIVE_PluginBoxMainColor	 	colorscheme 1
	0x00E2001A, // \031 DIVE_PluginBoxGrey				colorscheme 1
	0x00E2001A, // \032 DIVE_LabelColor 					colorscheme 1
	0x00F29400, // CLUT_DIVE_FieldSeperatorLines	colorscheme 1
	0x00E2001A, // CLUT_DIVE_pluginbox						colorscheme 1
	0x00FFFFFF, // CLUT_DIVE_SPARE21							colorscheme 1
	0x00FFFFFF, // CLUT_DIVE_SPARE22							colorscheme 1
	0x0000FF00, // \027 CLUT_MainColor ---------- colorscheme 2
	0x0000FF00, // \030 DIVE_PluginBoxMainColor	 	colorscheme 2
	0x0000948D, // \031 DIVE_PluginBoxGrey				colorscheme 2
	0x0000948D, // \032 DIVE_LabelColor 					colorscheme 2
	0x0000FF00, // CLUT_DIVE_FieldSeperatorLines	colorscheme 2
	0x0000948D, // CLUT_DIVE_pluginbox						colorscheme 2
	0x00FFFFFF, // CLUT_DIVE_SPARE21							colorscheme 2
	0x00FFFFFF, // CLUT_DIVE_SPARE22							colorscheme 2
	0x0033A1D6, // CLUT_MainColor --------------- colorscheme 3
	0x00FFFFFF, // \030 DIVE_PluginBoxMainColor	 	colorscheme 3
	0x00AAAAAA, // \031 DIVE_PluginBoxGrey				colorscheme 3
	0x000000FF, // \032 DIVE_LabelColor 					colorscheme 3
	0x0033A1D6, // CLUT_DIVE_FieldSeperatorLines	colorscheme 3
	0x000000FF, // CLUT_DIVE_pluginbox						colorscheme 3
	0x00FFFFFF, // CLUT_DIVE_SPARE21							colorscheme 3
	0x00FFFFFF, // CLUT_DIVE_SPARE22							colorscheme 3
};




/* Exported functions --------------------------------------------------------*/

