///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/text_multilanguage.h
/// \brief  Header file of TXT Multilanguage Support
/// \author heinrichs weikamp gmbh
/// \date   20-April-2014
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
#ifndef TEXT_MULTILINGUAGE_H
#define TEXT_MULTILINGUAGE_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

enum LANGUAGES
{
		LANGUAGE_English = 0,
		LANGUAGE_German,
		LANGUAGE_French,
		LANGUAGE_Italian,
		LANGUAGE_Espanol,
		LANGUAGE_END
};
#define NUMBER_OF_LANGUAGES LANGUAGE_END

typedef struct {
		 const uint8_t	code;
		 const uint8_t* text[NUMBER_OF_LANGUAGES];
} tText;
extern const tText text_array[];
extern const tText text_array2[];

/* Text codes ---------------------------------------------------------------*/
	enum TXT_MULTILANGUAGE
	{
		TXT_Language = '\x80',
		TXT_LanguageName,
		TXT_Depth,
		TXT_Divetime,
		TXT_MaxDepth,
		TXT_AvgDepth,
		TXT_Ceiling,
		TXT_ActualGradient,
		TXT_Stopwatch,
		TXT_Decostop,
		TXT_Nullzeit,
		TXT_ppO2,
		TXT_TTS,
		TXT_CNS,
		TXT_Temperature,
		TXT_FutureTTS,
		TXT_Gas,
		TXT_ChargeHour,
		TXT_Date,
		TXT_Format,
		TXT_Warning,
		TXT_Charging,
		TXT_o2Sensors,
		TXT_Brightness,
		TXT_Cave,
		TXT_Eco,
		TXT_Normal,
		TXT_Bright,
		TXT_Ultrabright,
		/* */
		TXT_OC_Gas_Edit,
		TXT_Diluent_Gas_Edit,
		TXT_Mix,
		TXT_First,
		TXT_Deco,
		TXT_Travel,
		TXT_Inactive,
		TXT_Off,
		TXT_ChangeDepth,
		TXT_Active,
		TXT_Default,
		TXT_Type,
		/* */
		TXT_Setpoint_Edit,
		/* */
		TXT_DecoAlgorithm,
		TXT_ZHL16GF,
		TXT_aGF,
		TXT_VPM,
		TXT_SafetyStop,
		TXT_low_high,
		TXT_ppO2Name,
		TXT_Minimum,
		TXT_Maximum,
		TXT_Minutes,
		TXT_Seconds,
		TXT_CCRmode,
		TXT_AtemGasVorrat,
		TXT_LiterproMinute,
		TXT_Reserve,
		TXT_Salinity,
		TXT_DiveMode,
		TXT_OpenCircuit,
		TXT_ClosedCircuit,
		TXT_Apnoe,
		TXT_Gauge,
		TXT_PSClosedCircuit,
		TXT_Sensor,
		TXT_FixedSP,
		TXT_Decoparameters,
		TXT_LastDecostop,
		TXT_Fallback,
		/* */
		TXT_DateAndTime,
		TXT_DateConfig,
		TXT_TimeConfig,
		TXT_Daylightsaving,
		/* */
		TXT_Logbook,
		TXT_LogbookEmpty,
		/* */
		TXT_Start_Calculation,
		/* */
		TXT_Information,
		/* */
		TXT_ScrubTime,
		TXT_ScrubTimeReset,
		TXT_ScrubTimeMode,
		TXT_Percent,
		TXT_PSCRO2Drop,
		TXT_PSCRLungRatio,
		TXT_SimPpo2,
		TXT_CO2Sensor,
		TXT_ButtonLock,
		/* */
		TXT_END,

		TXT_MINIMAL = '\xFE',
		TXT_2BYTE 	= '\xFF',
	};

/* Text codes ---------------------------------------------------------------*/
/* don't use the chars before as those break if(text == '\n') etc. pp.*/
	enum TXT2BYTE_MULTILANGUAGE
	{
		TXT2BYTE_START = '\x1F',
		TXT2BYTE_ResetMenu,
		TXT2BYTE_LogbookOffset,
		TXT2BYTE_AreYouSure,
		TXT2BYTE_Abort,
		TXT2BYTE_RebootRTE,
		TXT2BYTE_ResetAll,
		TXT2BYTE_ResetDeco,
		TXT2BYTE_ResetLogbook,
		TXT2BYTE_RebootMainCPU,
		TXT2BYTE_Exit,
		/* */
		TXT2BYTE_ShowDebug,
		TXT2BYTE_PleaseUpdate,
		TXT2BYTE_RTE,
		TXT2BYTE_Fonts,
		/* */
		TXT2BYTE_ResetStopwatch,
		TXT2BYTE_SetMarker,
		TXT2BYTE_SetMarkerShort,
		TXT2BYTE_CheckMarker,
		TXT2BYTE_CompassHeading,
		TXT2BYTE_CalibView,
		TXT2BYTE_EndDiveMode,
		/* */
		TXT2BYTE_Simulator,
		TXT2BYTE_StartSimulator,
		TXT2BYTE_Intervall,
		TXT2BYTE_SimDiveTime,
		TXT2BYTE_SimMaxDepth,
		TXT2BYTE_SimTravelGas,
		TXT2BYTE_SimDecoGas,
		TXT2BYTE_SimConsumption,
		TXT2BYTE_SimSummary,
		TXT2BYTE_SimDecTo,
		TXT2BYTE_SimLevel,
		TXT2BYTE_SimAscTo,
		TXT2BYTE_SimSurface,
		TXT2BYTE_CalculateDeco,
		TXT2BYTE_Calculating,
		TXT2BYTE_PleaseWait,

		/* */
		TXT2BYTE_Decolist,
		/* */
		TXT2BYTE_Bluetooth,
		TXT2BYTE_ButtonSensitivity,
		TXT2BYTE_SpecialDiveGas,
		TXT2BYTE_SpecialDiveGasMenu,
		TXT2BYTE_SpecialDiveGasMenuCCR,
		TXT2BYTE_CompassCalib,
		TXT2BYTE_CompassInertia,
		TXT2BYTE_UseSensor,
		TXT2BYTE_AutomaticSP,
		/* */
		TXT2BYTE_WarnDecoMissed,
		TXT2BYTE_WarnPPO2Low,
		TXT2BYTE_WarnPPO2High,
		TXT2BYTE_WarnBatteryLow,
		TXT2BYTE_WarnSensorLinkLost,
		TXT2BYTE_WarnFallback,
		TXT2BYTE_WarnCnsHigh,
		TXT2BYTE_WarnCO2High,
		/* */
		TXT2BYTE_O2monitor,
		TXT2BYTE_O2voltage,
		TXT2BYTE_Tissues,
		TXT2BYTE_Nitrogen,
		TXT2BYTE_Helium,
		TXT2BYTE_CNS,
		TXT2BYTE_OTU,
		TXT2BYTE_Profile,
		TXT2BYTE_Compass,
		TXT2BYTE_SafetyStop2,
		TXT2BYTE_noFly,
		TXT2BYTE_Desaturation,
		TXT2BYTE_TimeSinceLastDive,
		TXT2BYTE_ButtonLogbook,
		TXT2BYTE_ButtonMenu,
		TXT2BYTE_ButtonView,
		TXT2BYTE_ButtonBack,
		TXT2BYTE_ButtonEnter,
		TXT2BYTE_ButtonNext,
		TXT2BYTE_ButtonMinus,
		TXT2BYTE_ButtonPlus,
		TXT2BYTE_SimFollowDecoStops,
		/* */
		TXT2BYTE_Usage_Battery,
		TXT2BYTE_Usage_Dives,
		TXT2BYTE_Usage_Environment,
		/* */
		TXT2BYTE_ChargeCycles,
		TXT2BYTE_LowestVoltage,
		TXT2BYTE_HoursOfOperation,
		TXT2BYTE_NumberOfDives,
		TXT2BYTE_AmbientTemperature,
		/* */
		TXT2BYTE_Bottle,
		/* */
		TXT2BYTE_Gaslist,
		TXT2BYTE_Clock,
		TXT2BYTE_Sunday,
		TXT2BYTE_Monday,
		TXT2BYTE_Tuesday,
		TXT2BYTE_Wednesday,
		TXT2BYTE_Thursday,
		TXT2BYTE_Friday,
		TXT2BYTE_Saturday,
		/* */
		TXT2BYTE_Layout,
		TXT2BYTE_Units,
		TXT2BYTE_Units_metric,
		TXT2BYTE_Units_feet,
		TXT2BYTE_Farbschema,
		TXT2BYTE_Customviews,
		TXT2BYTE_CViewTimeout,
		TXT2BYTE_CViewStandard,
		TXT2BYTE_CornerTimeout,
		TXT2BYTE_CornerStandard,
		TXT2BYTE_IndicateFrame,
		TXT2BYTE_BoostBacklight,
		TXT2BYTE_FocusSpotSize,

		TXT2BYTE_SetToMOD,
		/* */
		TXT2BYTE_HUDbattery,
		TXT2BYTE_SensorDetect,
		TXT2BYTE_O2Calib,
		TXT2BYTE_O2Interface,
		TXT2BYTE_O2IFOptic,
		TXT2BYTE_O2IFAnalog,
		TXT2BYTE_O2IFDigital,
		TXT2BYTE_LowerIsLess,
		TXT2BYTE_DiveMenuQ,
		TXT2BYTE_DiveQuitQ,
		TXT2BYTE_DiveBearingQ,
		TXT2BYTE_DiveResetAvgQ,
		/* */
		TXT2BYTE_ExtraDisplay,
		TXT2BYTE_ExtraBigFont,
		TXT2BYTE_ExtraBigFontV2,
		TXT2BYTE_ExtraDecoGame,
		TXT2BYTE_ExtraNone,
		TXT2BYTE_ExtraActive,
		/* */
		TXT2BYTE_MotionCtrl,
		TXT2BYTE_MoCtrlNone,
		TXT2BYTE_MoCtrlPitch,
		TXT2BYTE_MoCtrlSector,
		TXT2BYTE_MoCtrlScroll,
		/* */
		TXT2BYTE_DecoDataLost,
		TXT2BYTE_Info,
		TXT2BYTE_Korrekturwerte,
		TXT2BYTE_SetBearing,
		TXT2BYTE_ResetBearing,
		TXT2BYTE_Sensor,
		TXT2BYTE_Maintenance,
		TXT2BYTE_SetBatteryCharge,
		TXT2BYTE_SetFactoryDefaults,
		TXT2BYTE_ResetBluetooth,
		TXT2BYTE_SetSampleIndex,
		TXT2BYTE_Reboot,
		TXT2BYTE_ButtonLeft,
		TXT2BYTE_ButtonMitte,
		TXT2BYTE_ButtonRight,
		/* */
		TXT2BYTE_Summary,
		TXT2BYTE_DispNoneDbg,
		TXT2BYTE_ApneaLast,
		TXT2BYTE_ApneaTotal,
		TXT2BYTE_ApneaSurface,
		/* */
		TXT2BYTE_FLIPDISPLAY,
		TXT2BYTE_SelectCustomviews,
		TXT2BYTE_SelectBigFont,
		TXT2BYTE_MaxDepth,
		TXT2BYTE_Stopwatch,
		TXT2BYTE_TTS,
		TXT2BYTE_ppoNair,
		TXT2BYTE_Navigation,
		TXT2BYTE_DepthData,
		TXT2BYTE_DecoTTS,

		TXT2BYTE_Minimum,
		TXT2BYTE_Normal,
		TXT2BYTE_Maximum,

		TXT2BYTE_CheckSettings,
		TXT2BYTE_NotCalibrated,

        TXT2BYTE_CcrSummary,
        TXT2BYTE_Setpoint,
        TXT2BYTE_Scrubber,

		TXT2BYTE_END
};

#endif /* TEXT_MULTILINGUAGE_H */
