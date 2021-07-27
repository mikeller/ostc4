///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/settings.c
/// \brief  settingsHelperButtonSens_translate_hwOS_values_to_percentage.
/// \author Heinrichs Weikamp gmbh
/// \date   6-March-2017
///
/// \details
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

#include <string.h>

#include "settings.h"
#include "firmwareEraseProgram.h" // for HARDWAREDATA_ADDRESS
#include "externLogbookFlash.h" // for SAMPLESTART and SAMPLESTOP
#include "text_multilanguage.h" // for LANGUAGE_END
#include "tHome.h" // for CVIEW_END
#include "motion.h"
#include "t7.h"
#include "data_central.h"

static uint8_t settingsWarning = 0;		/* Active if setting values have been corrected */
static SSettingsStatus SettingsStatus;  /* Structure containing number of corrections and first occurrence */

SSettings Settings;

const uint8_t RTErequiredHigh = 3;
const uint8_t RTErequiredLow = 2;

const uint8_t FONTrequiredHigh = 1;
const uint8_t FONTrequiredLow =	0;

uint8_t RTEactualHigh = 0;
uint8_t RTEactualLow = 0;

//  ===============================================================================
//  SFirmwareData FirmwareData
/// @brief	internal counter is for changes after last release
///					use date and info as well for this purpose
///
//  ===============================================================================

const SFirmwareData firmware_FirmwareData __attribute__( (section(".firmware_firmware_data")) ) =
{
    .versionFirst   = 1,
    .versionSecond 	= 6,
    .versionThird   = 5,
    .versionBeta    = 1,

    /* 4 bytes with trailing 0 */
    .signature = "mk",

    .release_year = 23,
    .release_month = 8,
    .release_day = 26,
    .release_sub = 0,

    /* max 48 with trailing 0 */
    //release_info ="12345678901234567890123456789012345678901"
    .release_info  ="gcc_2nd",

    /* for safety reasons and coming functions */
    .magic[0] = FIRMWARE_MAGIC_FIRST,
    .magic[1] = FIRMWARE_MAGIC_SECOND,
    .magic[2] = FIRMWARE_MAGIC_FIRMWARE, /* the magic byte */
    .magic[3] = FIRMWARE_MAGIC_END
};


/* always adjust check_and_correct_settings() accordingly
 * There might even be entries with fixed values that have no range
 */
const SSettings SettingsStandard = {
    .header = 0xFFFF0028,
    .warning_blink_dsec = 8 * 2,
    .lastDiveLogId = 0,
    .logFlashNextSampleStartAddress = SAMPLESTART,

    .gas[0].oxygen_percentage = 21,
    .gas[1].oxygen_percentage = 21,
    .gas[2].oxygen_percentage = 21,
    .gas[3].oxygen_percentage = 21,
    .gas[4].oxygen_percentage = 21,
    .gas[5].oxygen_percentage = 21,
    .gas[6].oxygen_percentage = 21,
    .gas[7].oxygen_percentage = 21,
    .gas[8].oxygen_percentage = 21,
    .gas[9].oxygen_percentage = 21,
    .gas[10].oxygen_percentage = 21,
    .gas[0].helium_percentage = 0,
    .gas[1].helium_percentage = 0,
    .gas[2].helium_percentage = 0,
    .gas[3].helium_percentage = 0,
    .gas[4].helium_percentage = 0,
    .gas[5].helium_percentage = 0,
    .gas[6].helium_percentage = 0,
    .gas[7].helium_percentage = 0,
    .gas[8].helium_percentage = 0,
    .gas[9].helium_percentage = 0,
    .gas[10].helium_percentage = 0,
    .gas[0].depth_meter = 0,
    .gas[1].depth_meter = 0,
    .gas[2].depth_meter = 0,
    .gas[3].depth_meter = 0,
    .gas[4].depth_meter = 0,
    .gas[5].depth_meter = 0,
    .gas[6].depth_meter = 0,
    .gas[7].depth_meter = 0,
    .gas[8].depth_meter = 0,
    .gas[9].depth_meter = 0,
    .gas[10].depth_meter = 0,
    .gas[0].depth_meter_travel = 0,
    .gas[1].depth_meter_travel = 0,
    .gas[2].depth_meter_travel = 0,
    .gas[3].depth_meter_travel = 0,
    .gas[4].depth_meter_travel = 0,
    .gas[5].depth_meter_travel = 0,
    .gas[6].depth_meter_travel = 0,
    .gas[7].depth_meter_travel = 0,
    .gas[8].depth_meter_travel = 0,
    .gas[9].depth_meter_travel = 0,
    .gas[10].depth_meter_travel = 0,
    .gas[0].note.uw = 0,
    .gas[1].note.ub.first = 1,
    .gas[1].note.ub.active = 1,
    .gas[1].note.ub.deco = 0,
    .gas[1].note.ub.travel = 0,
    .gas[2].note.uw = 0,
    .gas[3].note.uw = 0,
    .gas[4].note.uw = 0,
    .gas[5].note.uw = 0,
    .gas[6].note.ub.first = 1,
    .gas[6].note.ub.active = 1,
    .gas[6].note.ub.deco = 0,
    .gas[6].note.ub.travel = 0,
    .gas[7].note.uw = 0,
    .gas[8].note.uw = 0,
    .gas[9].note.uw = 0,
    .gas[10].note.uw = 0,
    .gas[0].bottle_size_liter = 0,
    .gas[1].bottle_size_liter = 0,
    .gas[2].bottle_size_liter = 0,
    .gas[3].bottle_size_liter = 0,
    .gas[4].bottle_size_liter = 0,
    .gas[5].bottle_size_liter = 0,
    .gas[6].bottle_size_liter = 0,
    .gas[7].bottle_size_liter = 0,
    .gas[8].bottle_size_liter = 0,
    .gas[9].bottle_size_liter = 0,
    .gas[10].bottle_size_liter = 0,
/*
    .gas[0].bottle_wireless_status = 0,
    .gas[1].bottle_wireless_status = 0,
    .gas[2].bottle_wireless_status = 0,
    .gas[3].bottle_wireless_status = 0,
    .gas[4].bottle_wireless_status = 0,
    .gas[5].bottle_wireless_status = 0,
    .gas[6].bottle_wireless_status = 0,
    .gas[7].bottle_wireless_status = 0,
    .gas[8].bottle_wireless_status = 0,
    .gas[9].bottle_wireless_status = 0,
    .gas[10].bottle_wireless_status = 0,
*/
    .gas[0].bottle_wireless_id = 0,
    .gas[1].bottle_wireless_id = 0,
    .gas[2].bottle_wireless_id = 0,
    .gas[3].bottle_wireless_id = 0,
    .gas[4].bottle_wireless_id = 0,
    .gas[5].bottle_wireless_id = 0,
    .gas[6].bottle_wireless_id = 0,
    .gas[7].bottle_wireless_id = 0,
    .gas[8].bottle_wireless_id = 0,
    .gas[9].bottle_wireless_id = 0,
    .gas[10].bottle_wireless_id = 0,
    .setpoint[0].setpoint_cbar = 100,
    .setpoint[1].setpoint_cbar = 70,
    .setpoint[2].setpoint_cbar = 90,
    .setpoint[3].setpoint_cbar = 100,
    .setpoint[4].setpoint_cbar = 120,
    .setpoint[5].setpoint_cbar = 140,
    .setpoint[0].depth_meter = 0,
    .setpoint[1].depth_meter = 0,
    .setpoint[2].depth_meter = 0,
    .setpoint[3].depth_meter = 0,
    .setpoint[4].depth_meter = 0,
    .setpoint[5].depth_meter = 0,
    .setpoint[0].note.uw = 0,
    .setpoint[1].note.ub.active = 1,
    .setpoint[1].note.ub.first = 1,
    .setpoint[2].note.ub.active = 1,
    .setpoint[2].note.ub.first = 0,
    .setpoint[3].note.ub.active = 1,
    .setpoint[3].note.ub.first = 0,
    .setpoint[4].note.ub.active = 1,
    .setpoint[4].note.ub.first = 0,
    .setpoint[5].note.ub.active = 1,
    .setpoint[5].note.ub.first = 0,

    .CCR_Mode = CCRMODE_Sensors,
    .dive_mode = DIVEMODE_OC,
    .deco_type.ub.standard = GF_MODE,
    .deco_type.ub.alternative = GF_MODE,
    .ppO2_max_deco = 160,
    .ppO2_max_std = 140,
    .ppO2_min = 15,
    .CNS_max = 90,
    .ascent_MeterPerMinute_max = 30,
    .ascent_MeterPerMinute_showGraph = 30,
    .future_TTS = 5,
    .GF_high = 85,
    .GF_low = 30,
    .aGF_high = 95,
    .aGF_low = 95,
    .VPM_conservatism.ub.standard = 2,
    .VPM_conservatism.ub.alternative = 0,
    .safetystopDuration = 1,
    .AtemMinutenVolumenLiter = 25,
    .ReserveFractionDenominator = 4,
    .salinity = 0,
    .last_stop_depth_meter = 3,
    .stop_increment_depth_meter = 3,
    .brightness = 1,
    .date_format = DDMMYY,
    .selected_language = 0, /* 0 = LANGUAGE_English */
    .customtext = " <your name>\n" " <address>",
    .timeoutSurfacemode = 120,
    .timeoutMenuSurface = 120,//240,
    .timeoutMenuDive = 120,//20,
    .timeoutMenuEdit = 120,//40,
    .timeoutInfo = 120,//60,
    .timeoutInfoCompass = 60,
    .design = 7,
    .timeoutDiveReachedZeroDepth = 300,
    .divetimeToCreateLogbook = 60,
    .serialHigh = 0,
    .serialLow = 2,
/*
    .firmwareVersion16to32bit.ub.first 		= 0,
    .firmwareVersion16to32bit.ub.second 	= 6,
    .firmwareVersion16to32bit.ub.third 		= 0,
    .firmwareVersion16to32bit.ub.betaFlag = 0,
*/
    .backup_localtime_rtc_tr = 0,
    .backup_localtime_rtc_dr = 0,
    .totalDiveCounter = 0,
    .personalDiveCount = 0,
    .showDebugInfo = 0,
    .ButtonResponsiveness[0] = DEFAULT_BUTTONRESPONSIVENESS_GUI, // new hw 170306
    .ButtonResponsiveness[1] = DEFAULT_BUTTONRESPONSIVENESS_GUI, // new hw 170306
    .ButtonResponsiveness[2] = DEFAULT_BUTTONRESPONSIVENESS_GUI, // new hw 170306
    .ButtonResponsiveness[3] = DEFAULT_BUTTONRESPONSIVENESS_GUI, // new hw 170306
    .nonMetricalSystem = 0,
    .fallbackToFixedSetpoint = 1,
    .bluetoothActive = 0,
    .safetystopDepth = 5,
    .updateSettingsAllowedFromHeader = 0xFFFF0002,
	.pscr_lung_ratio = 10,
	.pscr_o2_drop = 4,
	.co2_sensor_active = 0,
    .ppo2sensors_deactivated = 0,
    .tX_colorscheme  = 0,
    .tX_userselectedLeftLowerCornerPrimary = LLC_Temperature,
    .tX_userselectedLeftLowerCornerTimeout = 0,
    .tX_customViewPrimary = 1,
    .tX_customViewTimeout = 0,
    .timeoutEnterButtonSelectDive = 10,
    .logbookOffset = 0,
    .alwaysShowPPO2 = 0,
    .extraDisplay = EXTRADISPLAY_BIGFONT,
    .display_toogle_desc = 200,
    .offsetPressure_mbar = 0,
    .offsetTemperature_centigrad = 0,
    .gasConsumption_travel_l_min = 30,
    .gasConsumption_bottom_l_min = 30,
    .gasConsumption_deco_l_min = 20,
    .debugModeOnStart = 0,
    .compassBearing = 0,
    .lastKnownBatteryPercentage = 0,
    .buttonBalance[0] = 3,
    .buttonBalance[1] = 3,
    .buttonBalance[2] = 3,
    .firmwareVersion[0] = 0,//FirmwareData.firmwareVersion16to32bit.ub.first,
    .firmwareVersion[1] = 0,//FirmwareData.firmwareVersion16to32bit.ub.second,
    .firmwareVersion[2] = 0,//FirmwareData.firmwareVersion16to32bit.ub.third,
    .firmwareVersion[3] = 0,//FirmwareData.firmwareVersion16to32bit.ub.betaFlag,
    .timeoutSurfacemodeWithSensors = 600,
    .VPM_model = 0,
    .GF_model = 0,
    .FactoryButtonBase = DEFAULT_BUTTONRESPONSIVENESS_GUI,
    .FactoryButtonBalance[0] = 3,
    .FactoryButtonBalance[1] = 3,
    .FactoryButtonBalance[2] = 3,
	.FlipDisplay = 0,
	.cv_configuration = 0xFFFFFFFF,
	.MotionDetection = MOTION_DETECT_OFF,
	.cv_config_BigScreen = 0xFFFFFFFF,
	.compassInertia = 0,
	.tX_customViewPrimaryBF = CVIEW_T3_Decostop,
	.viewPortMode = 0,
	.viewRoll = 0,
	.viewPitch = 0,
	.viewYaw = 0,
	.ppo2sensors_source = 0,
	.ppo2sensors_calibCoeff[0] = 0.0,
	.ppo2sensors_calibCoeff[1] = 0.0,
	.ppo2sensors_calibCoeff[2] = 0.0,
	.amPMTime = 0,
	.autoSetpoint = 0,
	.scrubTimerMax_Obsolete = 0,
	.scrubTimerCur_Obsolete = 0,
	.scrubTimerMode = SCRUB_TIMER_OFF,
	.ext_sensor_map[0] = SENSOR_OPTIC,
	.ext_sensor_map[1] = SENSOR_OPTIC,
	.ext_sensor_map[2] = SENSOR_OPTIC,
	.ext_sensor_map[3] = SENSOR_NONE,
	.ext_sensor_map[4] = SENSOR_NONE,
	.ext_sensor_map[5] = SENSOR_NONE,
	.ext_sensor_map[6] = SENSOR_NONE,
	.ext_sensor_map[7] = SENSOR_NONE,
	.buttonLockActive = 0,
    .compassDeclinationDeg = 0,
    .delaySetpointLow = false,
    .timerDurationS = 180,
};

/* Private function prototypes -----------------------------------------------*/
uint8_t checkValue(uint8_t value,uint8_t from, uint8_t to);

/* Functions -----------------------------------------------------------------*/


//  ===============================================================================
//	set_new_settings_missing_in_ext_flash
/// @brief	Add all the new setting variables of this version
///					or/and change what has changed in the meantime
///
///					Additionally update the serial number if written via bluetooth
///
//  ===============================================================================
SSettings* pSettings;
void set_new_settings_missing_in_ext_flash(void)
{
	uint32_t tmp = 0;

    // never delete this part setting the serial
    if(hardwareDataGetPointer()->secondarySerial != 0xFFFF)
    {
        settingsGetPointer()->serialHigh = (hardwareDataGetPointer()->secondarySerial / 256);
        settingsGetPointer()->serialLow  = (hardwareDataGetPointer()->secondarySerial & 0xFF);
    }
    else
    if(hardwareDataGetPointer()->primarySerial != 0xFFFF)
    {
        settingsGetPointer()->serialHigh = (hardwareDataGetPointer()->primarySerial / 256);
        settingsGetPointer()->serialLow  = (hardwareDataGetPointer()->primarySerial & 0xFF);
    }
    else
    {
        settingsGetPointer()->serialHigh = 0;
        settingsGetPointer()->serialLow  = 0;
    }

    settingsGetPointer()->firmwareVersion[0] = firmware_FirmwareData.versionFirst;
    settingsGetPointer()->firmwareVersion[1] = firmware_FirmwareData.versionSecond;
    settingsGetPointer()->firmwareVersion[2] = firmware_FirmwareData.versionThird;
    settingsGetPointer()->firmwareVersion[3] = firmware_FirmwareData.versionBeta;

    pSettings = settingsGetPointer();
    const SSettings* pStandard = settingsGetPointerStandard();

    /* Pointing to the old header data => set new data depending on what had been added since last version */
    switch(pSettings->header)
    {
    case 0xFFFF0000:
    case 0xFFFF0001:
    case 0xFFFF0002:
    case 0xFFFF0003:
    case 0xFFFF0004:
    case 0xFFFF0005:
        pSettings->ppo2sensors_deactivated          = pStandard->ppo2sensors_deactivated;
        pSettings->tX_colorscheme                   = pStandard->tX_colorscheme;
        pSettings->tX_userselectedLeftLowerCornerPrimary = pStandard->tX_userselectedLeftLowerCornerPrimary;
        pSettings->tX_userselectedLeftLowerCornerTimeout = pStandard->tX_userselectedLeftLowerCornerTimeout;
        pSettings->tX_customViewPrimary             = pStandard->tX_customViewPrimary;
        pSettings->tX_customViewTimeout             = pStandard->tX_customViewTimeout;
        // no break
    case 0xFFFF0006:
        pSettings->timeoutEnterButtonSelectDive     = pStandard->timeoutEnterButtonSelectDive;
        pSettings->ButtonResponsiveness[0]          = pStandard->ButtonResponsiveness[0];
        pSettings->ButtonResponsiveness[1]          = pStandard->ButtonResponsiveness[1];
        pSettings->ButtonResponsiveness[2]          = pStandard->ButtonResponsiveness[2];
        pSettings->ButtonResponsiveness[3]          = pStandard->ButtonResponsiveness[3];
        pSettings->timeoutSurfacemode               = pStandard->timeoutSurfacemode;
        pSettings->timeoutMenuSurface               = pStandard->timeoutMenuSurface;
        pSettings->timeoutMenuDive                  = pStandard->timeoutMenuDive;
        pSettings->timeoutMenuEdit                  = pStandard->timeoutMenuEdit;
        pSettings->timeoutInfo                      = pStandard->timeoutInfo;
        pSettings->timeoutInfoCompass               = pStandard->timeoutInfoCompass;
        pSettings->timeoutDiveReachedZeroDepth      = pStandard->timeoutDiveReachedZeroDepth;
        pSettings->divetimeToCreateLogbook          = pStandard->divetimeToCreateLogbook;
        pSettings->safetystopDuration               = pStandard->safetystopDuration; // change from on/off to minutes
        // no break
    case 0xFFFF0007:
    case 0xFFFF0008:
        pSettings->alwaysShowPPO2                   = pStandard->alwaysShowPPO2;
        pSettings->logbookOffset                    = pStandard->logbookOffset;
        // no break
    case 0xFFFF0009:
        pSettings->extraDisplay                     = pStandard->extraDisplay;
        // no break
    case 0xFFFF000A:
        pSettings->display_toogle_desc              = pStandard->display_toogle_desc;
        // no break
    case 0xFFFF000B:
        pSettings->offsetPressure_mbar              = pStandard->offsetPressure_mbar;
        pSettings->offsetTemperature_centigrad      = pStandard->offsetTemperature_centigrad;
        pSettings->gasConsumption_travel_l_min      = pStandard->gasConsumption_travel_l_min;
        pSettings->gasConsumption_bottom_l_min      = pStandard->gasConsumption_bottom_l_min;
        pSettings->gasConsumption_deco_l_min        = pStandard->gasConsumption_deco_l_min;
        // no break
    case 0xFFFF000C:
        memcpy(pSettings->customtext, " hwOS 4\n\r" " welcome\n\r", 60);
        // no break
    case 0xFFFF000D: // nothing to do from 0xFFFF000D to 0xFFFF000E, just about header :-)
    case 0xFFFF000E:
        pSettings->debugModeOnStart                 = 0;
        // no break
    case 0xFFFF000F:
        pSettings->compassBearing                   = 0;
        // no break
    case 0xFFFF0011:
        pSettings->lastKnownBatteryPercentage       = 0;
        // no break
    case 0xFFFF0012:
        pSettings->buttonBalance[0]                 = 3;
        pSettings->buttonBalance[1]                 = 3;
        pSettings->buttonBalance[2]                 = 3;
        // no break
    case 0xFFFF0013:
    case 0xFFFF0014:
        pSettings->timeoutSurfacemodeWithSensors    = pStandard->timeoutSurfacemodeWithSensors;
        // no break
    case 0xFFFF0015:
        pSettings->ButtonResponsiveness[3]          = pStandard->ButtonResponsiveness[3];
        settingsHelperButtonSens_keepPercentageValues(settingsHelperButtonSens_translate_hwOS_values_to_percentage(pSettings->ButtonResponsiveness[3]), pSettings->ButtonResponsiveness);
        pSettings->VPM_model = 0;
        pSettings->GF_model = 0;
        // no break
    case 0xFFFF0016:
        pSettings->FactoryButtonBase                = pStandard->FactoryButtonBase;
        pSettings->FactoryButtonBalance[0]          = pStandard->FactoryButtonBalance[0];
        pSettings->FactoryButtonBalance[1]          = pStandard->FactoryButtonBalance[1];
        pSettings->FactoryButtonBalance[2]          = pStandard->FactoryButtonBalance[2];
        // no break
    case 0xFFFF0017:
    	pSettings->FlipDisplay = 0;
    	// no break
    case 0xFFFF0018:
    	pSettings->cv_configuration = 0xFFFFFFFF;
    	// no break
    case 0xFFFF0019:
    	pSettings->MotionDetection = MOTION_DETECT_OFF;
    	// no break
    case 0xFFFF001A:
    	/* deactivate new views => to be activated by customer */
        pSettings->cv_config_BigScreen = 0xFFFFFFFF;
        pSettings->cv_config_BigScreen &= pSettings->cv_configuration ^= 1 << (CVIEW_T3_Navigation + LEGACY_T3_START_ID_PRE_TIMER);
        pSettings->cv_config_BigScreen &= pSettings->cv_configuration ^= 1 << (CVIEW_T3_DepthData + LEGACY_T3_START_ID_PRE_TIMER);
        // no break
    case 0xFFFF001B:
    	pSettings->compassInertia = 0; 			/* no inertia */
    	pSettings->tX_customViewPrimaryBF = CVIEW_T3_Decostop;
    	pSettings->cv_config_BigScreen &= pSettings->cv_configuration ^= 1 << (CVIEW_T3_DecoTTS + LEGACY_T3_START_ID_PRE_TIMER);
        // no break
    case 0xFFFF001C:
    	pSettings->viewPortMode = 0;
    	pSettings->viewRoll = 0;
    	pSettings->viewPitch = 0;
    	pSettings->viewYaw = 0;
    	// no break
    case 0xFFFF001D:
    	pSettings->ppo2sensors_source = O2_SENSOR_SOURCE_OPTIC;
    	pSettings->ppo2sensors_calibCoeff[0] = 0.0;
    	pSettings->ppo2sensors_calibCoeff[1] = 0.0;
    	pSettings->ppo2sensors_calibCoeff[2] = 0.0;
    	pSettings->amPMTime = 0;
    	// no break
    case 0xFFFF001E:
    	pSettings->autoSetpoint = 0;
    	pSettings->scrubTimerMax_Obsolete = 0;
    	pSettings->scrubTimerCur_Obsolete = 0;
    	pSettings->scrubTimerMode = SCRUB_TIMER_OFF;
    	// no break
    case 0xFFFF001F:
    	pSettings->pscr_lung_ratio = 10;
    	pSettings->pscr_o2_drop = 4;
    	// no break
    case 0xFFFF0020:
    	pSettings->co2_sensor_active = 0;
    	// no break;
    case 0xFFFF0021:
    	pSettings->ext_uart_protocol = 0;
    	// no break;
    case 0xFFFF0022:
    	pSettings->scubberActiveId = 0;
    	pSettings->scrubberData[0].TimerCur = pSettings->scrubTimerCur_Obsolete;
    	pSettings->scrubberData[0].TimerMax = pSettings->scrubTimerMax_Obsolete;
    	pSettings->scrubberData[0].lastDive.WeekDay = 0;
    	pSettings->scrubberData[0].lastDive.Month = 0;
    	pSettings->scrubberData[0].lastDive.Date = 0;
    	pSettings->scrubberData[0].lastDive.Year = 0;

    	pSettings->scrubberData[1].TimerCur = 0;
    	pSettings->scrubberData[1].TimerMax = 0;
    	pSettings->scrubberData[1].lastDive.WeekDay = 0;
    	pSettings->scrubberData[1].lastDive.Month = 0;
    	pSettings->scrubberData[1].lastDive.Date = 0;
    	pSettings->scrubberData[1].lastDive.Year = 0;
    	// no break;
    case 0xFFFF0023: pSettings->ext_sensor_map[0] = SENSOR_OPTIC;
    				 pSettings->ext_sensor_map[1] = SENSOR_OPTIC;
    				 pSettings->ext_sensor_map[2] = SENSOR_OPTIC;
    				 pSettings->ext_sensor_map[3] = SENSOR_NONE;
    				 pSettings->ext_sensor_map[4] = SENSOR_NONE;
    	// no break;
    case 0xFFFF0024: pSettings->buttonLockActive = 0;
    	// no break;
    case 0xFFFF0025:
        pSettings->compassDeclinationDeg = pStandard->compassDeclinationDeg;
        pSettings->delaySetpointLow = pStandard->delaySetpointLow;
        // Disable auto setpoint to avoid a configuration warning being triggered by the new auto setpoint validation
        // This ensures that users don't lose setpoint information if it is not in the right spot for the new configuration
        pSettings->autoSetpoint = false;

        pSettings->timerDurationS = pStandard->timerDurationS;

    	// no break;
    case 0xFFFF0026:
    	pSettings->ext_sensor_map[0] = pSettings->ext_sensor_map_Obsolete[0];
    	pSettings->ext_sensor_map[1] = pSettings->ext_sensor_map_Obsolete[1];
    	pSettings->ext_sensor_map[2] = pSettings->ext_sensor_map_Obsolete[2];
    	pSettings->ext_sensor_map[3] = pSettings->ext_sensor_map_Obsolete[3];
    	pSettings->ext_sensor_map[4] = pSettings->ext_sensor_map_Obsolete[4];
    	pSettings->ext_sensor_map[5] = SENSOR_NONE;
    	pSettings->ext_sensor_map[6] = SENSOR_NONE;
    	pSettings->ext_sensor_map[7] = SENSOR_NONE;

        // no break;
    case 0xFFFF0027:
    	tmp = pSettings->cv_config_BigScreen & 0x00000007;				/* position of first three view is keeps the same */
    	/* Identify from which data version was in use before the update */
    	if(pSettings->tX_customViewPrimaryBF == LEGACY_CV_END_POST_TIMER)	/* data version after introduction of timer (ID shift problem) */
    	{
    		pSettings->tX_customViewPrimaryBF = CVIEW_T3_Decostop;
    	}
    	else
    	{
    		if(pSettings->tX_customViewPrimaryBF >= 3)
    		{
    			pSettings->tX_customViewPrimaryBF -= LEGACY_T3_START_ID_PRE_TIMER - 3;
    		}
    	}
    	pSettings->cv_config_BigScreen = pSettings->cv_config_BigScreen >> (LEGACY_T3_START_ID_PRE_TIMER - 3);
    	pSettings->cv_config_BigScreen &= ~0x00000007;		/* just to be sure: clear lower three bits */
    	pSettings->cv_config_BigScreen |= tmp;

    	// no break;
    default:
        pSettings->header = pStandard->header;
        break; // no break before!!
    }
}


uint8_t newFirmwareVersionCheckViaSettings(void)
{
    SSettings* pSettings = settingsGetPointer();

    if(pSettings->header < 0xFFFF0014) // for the versions without firmwareVersion[]
        return 1;

    if(pSettings->firmwareVersion[0] != firmware_FirmwareData.versionFirst)
        return 1;
    if(pSettings->firmwareVersion[1] != firmware_FirmwareData.versionSecond)
        return 1;
    if(pSettings->firmwareVersion[2] != firmware_FirmwareData.versionThird)
        return 1;
    if(pSettings->firmwareVersion[3] != firmware_FirmwareData.versionBeta)
        return 1;

    return 0;
}


void set_settings_button_to_factory_with_individual_buttonBalance(void)
{
    bool factoryBalanceSettingsValid = true;
    unsigned i = 0;
    while (factoryBalanceSettingsValid && i < 3) {
        if (Settings.FactoryButtonBalance[i] < 2 || Settings.FactoryButtonBalance[i] > 5) {
            factoryBalanceSettingsValid = false;
        }

        i++;
    }
    if (factoryBalanceSettingsValid) {
        for (i = 0; i < 3; i++) {
            Settings.buttonBalance[i] = Settings.FactoryButtonBalance[i];
        }
    }

    uint8_t buttonResponsiveness = Settings.FactoryButtonBase;
    if (buttonResponsiveness < 70 || buttonResponsiveness > 110) {
        buttonResponsiveness = SettingsStandard.ButtonResponsiveness[3];
    }

    settingsHelperButtonSens_keepPercentageValues(buttonResponsiveness, settingsGetPointer()->ButtonResponsiveness);
}


bool checkAndFixSetpointSettings(void)
{
    SSettings *settings = settingsGetPointer();

    bool fixed = false;
    if (settings->autoSetpoint) {
        SSetpointLine *setpointLow = &settings->setpoint[SETPOINT_INDEX_AUTO_LOW];
        if (setpointLow->setpoint_cbar >= 100) {
            setpointLow->setpoint_cbar = 70;

            fixed = true;
        }
        if (setpointLow->depth_meter == 0 || setpointLow->depth_meter > 100) {
            setpointLow->depth_meter = 2;

            fixed = true;
        }
        SSetpointLine *setpointHigh = &settings->setpoint[SETPOINT_INDEX_AUTO_HIGH];
        if (setpointHigh->setpoint_cbar <= setpointLow->setpoint_cbar) {
            setpointHigh->setpoint_cbar = 130;

            fixed = true;
        }
        if (setpointHigh->depth_meter <= setpointLow->depth_meter) {
            setpointHigh->depth_meter = setpointLow->depth_meter + 6;

            fixed = true;
        }

        SSetpointLine *setpointDeco = &settings->setpoint[SETPOINT_INDEX_AUTO_DECO];
        if (setpointDeco->note.ub.active) {
            if (setpointDeco->setpoint_cbar == setpointHigh->setpoint_cbar || setpointDeco->setpoint_cbar == setpointLow->setpoint_cbar) {
                setpointDeco->setpoint_cbar = 10 * (setpointHigh->setpoint_cbar / 10) - 10;
                if (setpointDeco->setpoint_cbar == setpointLow->setpoint_cbar) {
                    setpointDeco->setpoint_cbar += 5;
                }

                fixed = true;
            }
        }
    }

    return fixed;
}

static void setFirstCorrection(uint8_t Id)
{
	if(SettingsStatus.FirstCorrection == 0xFF)
	{
		SettingsStatus.FirstCorrection = Id;
	}
}

void get_CorrectionStatus(SSettingsStatus* Status)
{
	if(Status != NULL)
	{
		memcpy(Status, &SettingsStatus,2);
	}
}

uint8_t check_and_correct_settings(void)
{
    uint32_t corrections = 0;
    uint8_t firstGasFoundOC = 0;
    uint8_t firstGasFoundCCR = 0;
    uint8_t parameterId = 0;


    settingsWarning = 0; /* reset warning indicator */
    SettingsStatus.FirstCorrection = 0xFF;

/*	uint32_t header;
 */

/*	uint8_t warning_blink_dsec; 1/10 seconds
 */
        if((Settings.warning_blink_dsec < 1) || (Settings.warning_blink_dsec > 100))
        {
            Settings.warning_blink_dsec = SettingsStandard.warning_blink_dsec;
            corrections++;
            setFirstCorrection(parameterId);
        }
        parameterId++;
/*	uint8_t lastDiveLogId;
 */

/*	uint32_t logFlashNextSampleStartAddress;
 */
        if((Settings.logFlashNextSampleStartAddress < SAMPLESTART) || (Settings.logFlashNextSampleStartAddress > SAMPLESTOP))
        {
            Settings.logFlashNextSampleStartAddress = SAMPLESTART;
            corrections++;
            setFirstCorrection(parameterId);
        }
        parameterId++;

/*	uint8_t dive_mode; has to before the gases
 */
    if(	(Settings.dive_mode != DIVEMODE_OC) 		&&
            (Settings.dive_mode != DIVEMODE_CCR)  	&&
            (Settings.dive_mode != DIVEMODE_Gauge)	&&
            (Settings.dive_mode != DIVEMODE_Apnea)	&&
			(Settings.dive_mode != DIVEMODE_PSCR))
    {
        Settings.dive_mode = DIVEMODE_OC;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;

/*	SGasLine gas[1 + (2*NUM_GASES)];
 */
    for(int i=1; i<=2*NUM_GASES;i++)
    {
        if(Settings.gas[i].oxygen_percentage < 4)
        {
            Settings.gas[i].oxygen_percentage = 4;
            corrections++;
            setFirstCorrection(parameterId);
        }
        if(Settings.gas[i].oxygen_percentage > 100)
        {
            Settings.gas[i].oxygen_percentage = 100;
            corrections++;
            setFirstCorrection(parameterId);
        }
        if((Settings.gas[i].oxygen_percentage + Settings.gas[i].helium_percentage) > 100)
        {
            Settings.gas[i].helium_percentage = 100 - Settings.gas[i].oxygen_percentage;
            corrections++;
            setFirstCorrection(parameterId);
        }
        if(Settings.gas[i].note.ub.deco)
        {
            if(Settings.gas[i].note.ub.active != 1)
            {
                Settings.gas[i].note.ub.active = 1;
                corrections++;
                setFirstCorrection(parameterId);
            }
            if(Settings.gas[i].note.ub.travel == 1)
            {
                Settings.gas[i].note.ub.travel = 0;
                corrections++;
                setFirstCorrection(parameterId);
            }
        }
        if(Settings.gas[i].note.ub.travel)
        {
            if(Settings.gas[i].note.ub.active != 1)
            {
                Settings.gas[i].note.ub.active = 1;
                corrections++;
                setFirstCorrection(parameterId);
            }
            if(Settings.gas[i].note.ub.deco == 1)
            {
                Settings.gas[i].note.ub.deco = 0;
                corrections++;
                setFirstCorrection(parameterId);
            }
        }
        if(Settings.gas[i].note.ub.first)
        {
            if(Settings.gas[i].note.ub.active != 1)
            {
                Settings.gas[i].note.ub.active = 1;
                corrections++;
                setFirstCorrection(parameterId);
            }
            if(Settings.gas[i].note.ub.travel == 1)
            {
                Settings.gas[i].note.ub.travel = 0;
                corrections++;
                setFirstCorrection(parameterId);
            }
            if(Settings.gas[i].note.ub.deco == 1)
            {
                Settings.gas[i].note.ub.deco = 0;
                corrections++;
                setFirstCorrection(parameterId);
            }
            if((i<=NUM_GASES) && (!firstGasFoundOC))
                firstGasFoundOC = 1;
            else
            if((i>NUM_GASES) && (!firstGasFoundCCR))
                firstGasFoundCCR = 1;
            else
                Settings.gas[i].note.ub.first = 0;
        }
        if(Settings.gas[i].bottle_size_liter > 40)
        {
            Settings.gas[i].bottle_size_liter = 40;
            corrections++;
            setFirstCorrection(parameterId);
        }
        if(Settings.gas[i].depth_meter > 250)
        {
            Settings.gas[i].depth_meter = 250;
            corrections++;
            setFirstCorrection(parameterId);
        }
        if(Settings.gas[i].depth_meter_travel > 250)
        {
            Settings.gas[i].depth_meter_travel = 250;
            corrections++;
            setFirstCorrection(parameterId);
        }
        /*if(Settings.gas[i].note.ub.senderCode)
        {
        }
        if(Settings.gas[i].bottle_wireless_id)
        {
        }
        */
    } // for(int i=1; i<=2*NUM_GASES;i++)
    parameterId++;
    if(!firstGasFoundOC)
    {
        Settings.gas[1].note.ub.active = 1;
        Settings.gas[1].note.ub.first = 1;
        Settings.gas[1].note.ub.travel = 0;
        Settings.gas[1].note.ub.deco = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(!firstGasFoundCCR)
    {
        Settings.gas[1 + NUM_GASES].note.ub.active = 1;
        Settings.gas[1 + NUM_GASES].note.ub.first = 1;
        Settings.gas[1 + NUM_GASES].note.ub.travel = 0;
        Settings.gas[1 + NUM_GASES].note.ub.deco = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	SSetpointLine setpoint[1 + NUM_GASES];
 */
    for(int i=1; i<=NUM_GASES;i++)
    {
        if(Settings.setpoint[i].setpoint_cbar < MIN_PPO2_SP_CBAR)
        {
            Settings.setpoint[i].setpoint_cbar = MIN_PPO2_SP_CBAR;
            corrections++;
            setFirstCorrection(parameterId);
        }
        if(Settings.setpoint[i].setpoint_cbar > 160)
        {
            Settings.setpoint[i].setpoint_cbar = 160;
            corrections++;
            setFirstCorrection(parameterId);
        }
        if(Settings.setpoint[i].depth_meter > 250)
        {
            Settings.setpoint[i].depth_meter = 250;
            corrections++;
            setFirstCorrection(parameterId);
        }
    }	// for(int i=1; i<=NUM_GASES;i++)
    parameterId++;
    if (checkAndFixSetpointSettings()) {
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t CCR_Mode;
 */
    if(	(Settings.CCR_Mode != CCRMODE_Sensors) &&
    		(Settings.CCR_Mode != CCRMODE_Simulation) &&
            (Settings.CCR_Mode != CCRMODE_FixedSetpoint))
    {
        Settings.CCR_Mode = CCRMODE_FixedSetpoint;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	split2x4_Type deco_type;
 */
    if(	(Settings.deco_type.ub.standard != GF_MODE) &&
            (Settings.deco_type.ub.standard != VPM_MODE))
    {
        Settings.deco_type.ub.standard = VPM_MODE;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.deco_type.ub.alternative != GF_MODE)
    {
        Settings.deco_type.ub.alternative = GF_MODE;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t ppO2_max_deco;
 */
    if(Settings.ppO2_max_deco > 190)
    {
        Settings.ppO2_max_deco = 190;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.ppO2_max_deco < 100)
    {
        Settings.ppO2_max_deco = 100;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;

/*	uint8_t ppO2_max_std;
 */
    if(Settings.ppO2_max_std > 190)
    {
        Settings.ppO2_max_std = 190;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.ppO2_max_std < 100)
    {
        Settings.ppO2_max_std = 100;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t ppO2_min;
 */
    if(Settings.ppO2_min != 15)
    {
        Settings.ppO2_min = 15;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t CNS_max;
 */
    if(Settings.CNS_max != 90)
    {
        Settings.CNS_max = 90;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*  uint8_t ascent_MeterPerMinute_max;
 */
    if(Settings.ascent_MeterPerMinute_max != 30)
    {
        Settings.ascent_MeterPerMinute_max = 30;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t ascent_MeterPerMinute_showGraph;
 */
    if(Settings.ascent_MeterPerMinute_showGraph != 30)
    {
        Settings.ascent_MeterPerMinute_showGraph = 30;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t future_TTS;
 */
    if(Settings.future_TTS > 15)
    {
        Settings.future_TTS = 15;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t GF_high;
 */
    if(Settings.GF_high > 99)
    {
        Settings.GF_high = 99;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.GF_high < 45)
    {
        Settings.GF_high = 45;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t GF_low;
 */
    if(Settings.GF_low > 99)
    {
        Settings.GF_low = 99;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.GF_low < 10)
    {
        Settings.GF_low = 10;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.GF_low > Settings.GF_high)
    {
        Settings.GF_low = Settings.GF_high;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t aGF_high;
 */
    if(Settings.aGF_high > 99)
    {
        Settings.aGF_high = 99;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.aGF_high < 45)
    {
        Settings.aGF_high = 45;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t aGF_low;
 */
    if(Settings.aGF_low > 99)
    {
        Settings.aGF_low = 99;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.aGF_low < 10)
    {
        Settings.aGF_low = 10;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.aGF_low > Settings.aGF_high)
    {
        Settings.aGF_low = Settings.aGF_high;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	split2x4_Type VPM_conservatism;
 */
    if(Settings.VPM_conservatism.ub.standard > 5)
    {
        Settings.VPM_conservatism.ub.standard = 5;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.VPM_conservatism.ub.alternative > 5)
    {
        Settings.VPM_conservatism.ub.alternative = 5;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t safetystopDuration;
 */
    if(Settings.safetystopDuration > 5)
    {
        Settings.safetystopDuration = 5;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t AtemMinutenVolumenLiter;
 */
    if(Settings.AtemMinutenVolumenLiter != 25)
    {
        Settings.AtemMinutenVolumenLiter = 25;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t ReserveFractionDenominator;
 */
    if(Settings.ReserveFractionDenominator != 4)
    {
        Settings.ReserveFractionDenominator = 4;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t salinity;
 */
    if(Settings.salinity > 4)
    {
        Settings.salinity = 4;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t last_stop_depth_meter;
 */
    if(Settings.last_stop_depth_meter > 9)
    {
        Settings.last_stop_depth_meter = 9;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.last_stop_depth_meter < 3)
    {
        Settings.last_stop_depth_meter = 3;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t stop_increment_depth_meter;
 */
    if(Settings.stop_increment_depth_meter != 3)
    {
        Settings.stop_increment_depth_meter = 3;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t brightness;
 */
    if(Settings.brightness > 4)
    {
        Settings.brightness = 4;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t date_format;
 */
    if(	(Settings.date_format != DDMMYY) &&
            (Settings.date_format != MMDDYY) &&
            (Settings.date_format != YYMMDD))
    {
        Settings.date_format = DDMMYY;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t selected_language;
 */
    if(Settings.selected_language >= LANGUAGE_END)
    {
        Settings.selected_language = LANGUAGE_English;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	char customtext[60];
 */
    if(Settings.customtext[59] != 0)
    {
        Settings.customtext[59] = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint16_t timeoutSurfacemode;
 */
    if(	(Settings.timeoutSurfacemode != 20) &&  // Quick Sleep Option
            (Settings.timeoutSurfacemode != 120))
    {
        Settings.timeoutSurfacemode = 120;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t timeoutMenuSurface;
 */
    if(Settings.timeoutMenuSurface != 120)
    {
        Settings.timeoutMenuSurface = 120;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t timeoutMenuDive;
 */
    if(Settings.timeoutMenuDive != 120)
    {
        Settings.timeoutMenuDive = 120;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t timeoutMenuEdit;
 */
    if(Settings.timeoutMenuEdit != 120)
    {
        Settings.timeoutMenuEdit = 120;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t timeoutInfo;
 */
    if(Settings.timeoutInfo != 120)
    {
        Settings.timeoutInfo = 120;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t timeoutInfoCompass;
 */
    if(Settings.timeoutInfoCompass != 60)
    {
        Settings.timeoutInfoCompass = 60;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t design;
 */
    if(Settings.design != 7)
    {
        Settings.design = 7;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint16_t timeoutDiveReachedZeroDepth;
 */
    if(Settings.timeoutDiveReachedZeroDepth != 300)
    {
        Settings.timeoutDiveReachedZeroDepth = 300;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint16_t divetimeToCreateLogbook;
 */
    if(Settings.divetimeToCreateLogbook != 60)
    {
        Settings.divetimeToCreateLogbook = 60;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t serialHigh;
 */

/*	uint8_t serialLow;
 */

/*	SUFirmware firmwareVersion16to32bit;
 */

/*	uint32_t backup_localtime_rtc_tr;
 */

/*	uint32_t backup_localtime_rtc_dr;
 */

/*	uint16_t totalDiveCounter;
 */

/*	uint16_t personalDiveCount;
 */

/*	uint8_t showDebugInfo;
 */

#ifdef HAVE_DEBUG_VIEW
    if(Settings.showDebugInfo > 1)
    {
        Settings.showDebugInfo = 0;
        corrections++;
    }
#else
    Settings.showDebugInfo = 0;
#endif

/*	uint8_t ButtonResponsiveness[4];
 */
    // Base value, index 3
    if(Settings.ButtonResponsiveness[3] < MIN_BUTTONRESPONSIVENESS_GUI)
    {
        Settings.ButtonResponsiveness[3] = MIN_BUTTONRESPONSIVENESS_GUI;
        corrections++;
        setFirstCorrection(parameterId);
    }
    else
    if(Settings.ButtonResponsiveness[3] > MAX_BUTTONRESPONSIVENESS_GUI)
    {
        Settings.ButtonResponsiveness[3] = MAX_BUTTONRESPONSIVENESS_GUI;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    // flex values 0, 1, 2
    for(int i=0; i<3;i++)
    {
        if(Settings.ButtonResponsiveness[i] < MIN_BUTTONRESPONSIVENESS) // 50-10  //Fix for broken buttons. :)
        {
            Settings.ButtonResponsiveness[i] = MIN_BUTTONRESPONSIVENESS;
            corrections++;
            setFirstCorrection(parameterId);
        }
        else
        if(Settings.ButtonResponsiveness[i] > MAX_BUTTONRESPONSIVENESS) // 110+20
        {
            Settings.ButtonResponsiveness[i] = MAX_BUTTONRESPONSIVENESS;
            corrections++;
            setFirstCorrection(parameterId);
        }
    }
    parameterId++;
/*	uint8_t buttonBalance[3];
 */
    for(int i=0; i<3;i++)
    {
        if(Settings.buttonBalance[i] < 2) // 2 = -10
        {
            Settings.buttonBalance[i] = 2;
            corrections++;
            setFirstCorrection(parameterId);
        }
        else
        if(Settings.buttonBalance[i] > 5) // 3 = 0, 4 = +10, 5 = +20
        {
            Settings.buttonBalance[i] = 5;
            corrections++;
            setFirstCorrection(parameterId);
        }
    }
    parameterId++;
/*	uint8_t nonMetricalSystem;
 */
    if(Settings.nonMetricalSystem > 1)
    {
        Settings.nonMetricalSystem = 1;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t fallbackToFixedSetpoint;
 */
    if(Settings.fallbackToFixedSetpoint > 1)
    {
        Settings.fallbackToFixedSetpoint = 1;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t bluetoothActive;
 */
    if(Settings.bluetoothActive > 1)
    {
        Settings.bluetoothActive = 1;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t safetystopDepth;
 */
    if(Settings.safetystopDepth > 6)
    {
        Settings.safetystopDepth = 6;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.safetystopDepth < 3)
    {
        Settings.safetystopDepth = 3;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint32_t updateSettingsAllowedFromHeader;
 */

/*	uint8_t ppo2sensors_deactivated;
 */
    if(Settings.ppo2sensors_deactivated > (1+2+4))
    {
        Settings.ppo2sensors_deactivated = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t tX_colorscheme;
 */
    if(Settings.tX_colorscheme > 3)
    {
        Settings.tX_colorscheme = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t tX_userselectedLeftLowerCornerPrimary;
 */
    if(Settings.tX_userselectedLeftLowerCornerPrimary >= LLC_END)
    {
        Settings.tX_userselectedLeftLowerCornerPrimary = LLC_Temperature;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t tX_userselectedLeftLowerCornerTimeout;
 */
    if(Settings.tX_userselectedLeftLowerCornerTimeout > 60)
    {
        Settings.tX_userselectedLeftLowerCornerTimeout = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t tX_customViewPrimary;
 */
    if(Settings.tX_customViewPrimary >= CVIEW_END)
    {
        Settings.tX_customViewPrimary = 1;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t tX_customViewTimeout;
 */
    if(Settings.tX_customViewTimeout > 60)
    {
        Settings.tX_customViewTimeout = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t timeoutEnterButtonSelectDive;
 */
    if(Settings.timeoutEnterButtonSelectDive != 10)
    {
        Settings.timeoutEnterButtonSelectDive = 10;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t logbookOffset;
 */
    if(Settings.logbookOffset > 9000)
    {
        Settings.logbookOffset = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t alwaysShowPPO2;
 */
    if(Settings.alwaysShowPPO2 > 1)
    {
        Settings.alwaysShowPPO2 = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t extraDisplay;
 */
    if(Settings.extraDisplay >= EXTRADISPLAY_END)
    {
        Settings.extraDisplay = EXTRADISPLAY_BIGFONT;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	int8_t offsetPressure_mbar;
 */
    if((Settings.offsetPressure_mbar > PRESSURE_OFFSET_LIMIT_MBAR) ||
         (Settings.offsetPressure_mbar < -1 * PRESSURE_OFFSET_LIMIT_MBAR))
    {
        Settings.offsetPressure_mbar = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	int8_t offsetTemperature_centigrad;
 */
    if((Settings.offsetTemperature_centigrad > 20) ||
         (Settings.offsetTemperature_centigrad < -20))
    {
        Settings.offsetTemperature_centigrad = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t gasConsumption_travel_l_min;
 */
    if((Settings.gasConsumption_travel_l_min < 5) ||
         (Settings.gasConsumption_travel_l_min > 50))
    {
        Settings.gasConsumption_travel_l_min = 20;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t gasConsumption_bottom_l_min;
 */
    if((Settings.gasConsumption_bottom_l_min < 5) ||
         (Settings.gasConsumption_bottom_l_min > 50))
    {
        Settings.gasConsumption_bottom_l_min = 20;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t gasConsumption_deco_l_min;
 */
    if((Settings.gasConsumption_deco_l_min < 5) ||
         (Settings.gasConsumption_deco_l_min > 50))
    {
        Settings.gasConsumption_deco_l_min = 20;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t showDebugInfo;
 */
#ifdef BOOT16
        Settings.showDebugInfo = 0;
#else
        if(Settings.showDebugInfo > 1)
            Settings.showDebugInfo = 0;

#endif

/*	uint8_t selected_language;
 */
#ifdef BOOT16
        if(Settings.selected_language > 1)
            Settings.selected_language = 0;
#else
        if(Settings.selected_language > 4)
            Settings.selected_language = 0;
#endif


/*	uint8_t display_toogle_desc; 1/10 seconds
 */
        if((Settings.display_toogle_desc < 20) || (Settings.display_toogle_desc > 600))
        {
            Settings.display_toogle_desc = SettingsStandard.display_toogle_desc;
            corrections++;
            setFirstCorrection(parameterId);
        }
        parameterId++;
/*	uint8_t debugModeOnStart;
 */
    if(Settings.debugModeOnStart > 1)
    {
        Settings.debugModeOnStart = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;

/*	uint8_t IAmStolenPleaseKillMe;
 */

    if(hardwareDataGetPointer()->primarySerial == 90)
        Settings.IAmStolenPleaseKillMe++;
    else
        Settings.IAmStolenPleaseKillMe = 0;


/*	uint8_t debugModeOnStart;
 */
    if(Settings.compassBearing > 360)
    {
        Settings.compassBearing = 0;
        corrections++;
        setFirstCorrection(parameterId);
    }

    parameterId++;
/*	uint8_t lastKnownBatteryPercentage;
 */
    if(Settings.lastKnownBatteryPercentage > 100)
    {
        Settings.lastKnownBatteryPercentage = 100;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t VPM_model
 */
    if((Settings.VPM_model !=  VPM_FROM_FORTRAN) && (Settings.VPM_model !=  VPM_BACHELORWORK))
    {
        Settings.VPM_model = VPM_FROM_FORTRAN;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
/*	uint8_t Buehlmann_model
 */
    if((Settings.GF_model !=  BUEHLMANN_OSTC4) && (Settings.GF_model !=  BUEHLMANN_hwOS))
    {
        Settings.GF_model = BUEHLMANN_OSTC4;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.FlipDisplay > 1) /* only boolean values allowed */
   	{
    	Settings.FlipDisplay = 0;
	    corrections++;
	    setFirstCorrection(parameterId);
   	}
    parameterId++;
#ifdef ENABLE_MOTION_CONTROL
    if(Settings.MotionDetection >= MOTION_DETECT_END)
   	{
    	Settings.MotionDetection = MOTION_DETECT_OFF;
	    corrections++;
	    setFirstCorrection(parameterId);
   	}
#else
    Settings.MotionDetection = MOTION_DETECT_OFF;
    Settings.viewPortMode = 0;
    Settings.viewRoll = 0.0;
    Settings.viewPitch = 0.0;
    Settings.viewYaw = 0.0;
#endif
    parameterId++;
    if(Settings.compassInertia > MAX_COMPASS_COMP)
   	{
    	Settings.compassInertia = 0;
	    corrections++;
	    setFirstCorrection(parameterId);
   	}
    parameterId++;
    if(Settings.tX_customViewPrimaryBF > CVIEW_T3_END)
    {
    	Settings.tX_customViewPrimaryBF = CVIEW_T3_Decostop;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.viewPortMode > MAX_VIEWPORT_MODE)
    {
    	Settings.viewPortMode = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.ppo2sensors_source >= O2_SENSOR_SOURCE_MAX)
    {
    	Settings.ppo2sensors_source = O2_SENSOR_SOURCE_OPTIC;
    	Settings.ppo2sensors_calibCoeff[0] = 0.0;
    	Settings.ppo2sensors_calibCoeff[1] = 0.0;
    	Settings.ppo2sensors_calibCoeff[2] = 0.0;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.amPMTime > 1) /* only boolean values allowed */
    {
    	Settings.amPMTime = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.autoSetpoint > 1) /* only boolean values allowed */
    {
    	Settings.autoSetpoint = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.scubberActiveId > 1)
    {
    	Settings.scubberActiveId = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if((Settings.scrubberData[0].TimerMax > MAX_SCRUBBER_TIME) || Settings.scrubberData[0].TimerCur < MIN_SCRUBBER_TIME || Settings.scrubberData[0].TimerCur > (int16_t)MAX_SCRUBBER_TIME)
    {
    	Settings.scrubberData[0].TimerMax = 0;
    	Settings.scrubberData[0].TimerCur = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if((Settings.scrubberData[1].TimerMax > MAX_SCRUBBER_TIME) || Settings.scrubberData[1].TimerCur < MIN_SCRUBBER_TIME || Settings.scrubberData[1].TimerCur > (int16_t)MAX_SCRUBBER_TIME)
    {
    	Settings.scrubberData[1].TimerMax = 0;
    	Settings.scrubberData[1].TimerCur = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.scrubTimerMode > SCRUB_TIMER_END)
    {
    	Settings.scrubTimerMode = SCRUB_TIMER_OFF;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if((Settings.pscr_lung_ratio > PSCR_MAX_LUNG_RATIO) || (Settings.pscr_lung_ratio < PSCR_MIN_LUNG_RATIO))
    {
    	Settings.pscr_lung_ratio = 10;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.pscr_o2_drop > PSCR_MAX_O2_DROP)
    {
    	Settings.pscr_o2_drop = 4;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.co2_sensor_active > 1)
    {
    	Settings.co2_sensor_active = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.ext_uart_protocol > UART_MAX_PROTOCOL)
    {
    	Settings.ext_uart_protocol = 0;
    	corrections++;
    	setFirstCorrection(parameterId);
    }
    parameterId++;
    if((Settings.ext_sensor_map[0] >= SENSOR_END)
    		|| (Settings.ext_sensor_map[1] >= SENSOR_END)
			|| (Settings.ext_sensor_map[2] >= SENSOR_END)
			|| (Settings.ext_sensor_map[3] >= SENSOR_END)
			|| (Settings.ext_sensor_map[4] >= SENSOR_END)
			|| (Settings.ext_sensor_map[5] >= SENSOR_END)
			|| (Settings.ext_sensor_map[6] >= SENSOR_END)
			|| (Settings.ext_sensor_map[7] >= SENSOR_END))
    {
    	Settings.ext_sensor_map[0] = SENSOR_OPTIC;
    	Settings.ext_sensor_map[1] = SENSOR_OPTIC;
    	Settings.ext_sensor_map[2] = SENSOR_OPTIC;
    	Settings.ext_sensor_map[3] = SENSOR_NONE;
    	Settings.ext_sensor_map[4] = SENSOR_NONE;
    	Settings.ext_sensor_map[5] = SENSOR_NONE;
    	Settings.ext_sensor_map[6] = SENSOR_NONE;
    	Settings.ext_sensor_map[7] = SENSOR_NONE;
       	corrections++;
       	setFirstCorrection(parameterId);
    }
    parameterId++;
    if(Settings.buttonLockActive > 1)
    {
        Settings.buttonLockActive = 1;
        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if (Settings.compassDeclinationDeg > 99) {
        Settings.compassDeclinationDeg = 99;

        corrections++;
        setFirstCorrection(parameterId);
    } else if (Settings.compassDeclinationDeg < -99) {
        Settings.compassDeclinationDeg = -99;

        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if (Settings.timerDurationS > 599) {
        Settings.timerDurationS = 599;

        corrections++;
        setFirstCorrection(parameterId);
    } else if (Settings.timerDurationS < 1) {
        Settings.timerDurationS = 1;

        corrections++;
        setFirstCorrection(parameterId);
    }
    parameterId++;
    if(corrections)
    {
    	settingsWarning = 1;
    }
    else

    if(corrections > 255)
    {
    	corrections = 255;
    }

    SettingsStatus.Corrections = corrections;
    return (uint8_t)corrections;
}


/* always at 0x8080000, do not move -> bootloader access */
const SFirmwareData* firmwareDataGetPointer(void)
{
    return &firmware_FirmwareData;
}


#ifndef SPECIALPROGRAMM
const SHardwareData* hardwareDataGetPointer(void)
{
    return (SHardwareData *)HARDWAREDATA_ADDRESS;
}
#endif

const SSettings* settingsGetPointerStandard(void)
{
    return &SettingsStandard;
}


void hardwareBatchCode(uint8_t *high, uint8_t *low)
{
    if(high)
    {
        *high = (uint8_t)((hardwareDataGetPointer()->production_year - 16) * 16);
        *high += hardwareDataGetPointer()->production_month;
        if(low)
        {
            *low = (uint8_t)(hardwareDataGetPointer()->production_day * 8);
        }
    }
}


uint8_t firmwareVersion_16bit_high(void)
{
    return ((firmware_FirmwareData.versionFirst & 0x1F)  << 3)	+ ((firmware_FirmwareData.versionSecond & 0x1C) >> 2);
}

uint8_t firmwareVersion_16bit_low(void)
{
    return ((firmware_FirmwareData.versionSecond & 0x03)  << 6)	+ ((firmware_FirmwareData.versionThird & 0x1F) << 1) + (firmware_FirmwareData.versionBeta & 0x01);
}

inline SSettings* settingsGetPointer(void)
{
    return &Settings;
}


//  ===============================================================================
//  set_settings_to_Standard
/// @brief	This function overwrites the current settings of the system
///					with the EXCEPTION of the personalDiveCount
///
///					It additionally calls set_new_settings_missing_in_ext_flash() and
///					check_and_correct_settings(), even so this shouldn't be necessary.
///					It is called on every start and from Reset All.
///
///					160622 added lastDiveLogIdBackup
///
//  ===============================================================================
void set_settings_to_Standard(void)
{
    SSettings *pSettings;
    const SSettings *pSettingsStandard;
    uint16_t personalDiveCountBackup;
    uint8_t lastDiveLogIdBackup;
    pSettings = settingsGetPointer();
    pSettingsStandard = settingsGetPointerStandard();

    personalDiveCountBackup = pSettings->personalDiveCount;
    lastDiveLogIdBackup = pSettings->lastDiveLogId;
    memcpy(pSettings,pSettingsStandard,sizeof(*pSettings));
    pSettings->personalDiveCount = personalDiveCountBackup;
    pSettings->lastDiveLogId = lastDiveLogIdBackup;

    pSettings->firmwareVersion[0] = firmware_FirmwareData.versionFirst;
    pSettings->firmwareVersion[1] = firmware_FirmwareData.versionSecond;
    pSettings->firmwareVersion[2] = firmware_FirmwareData.versionThird;
    pSettings->firmwareVersion[3] = firmware_FirmwareData.versionBeta;

    set_new_settings_missing_in_ext_flash();
    check_and_correct_settings();
    // has to be called too: createDiveSettings();
}


//  ===============================================================================
//  mod_settings_for_first_start_with_empty_ext_flash
/// @brief	This function overwrites some settings of the system
///					It is called on every start.
///					Those settings will be overwriten by ext_flash_read_settings()
///					Will be kept if ext_flash_read_settings() is invalid because
///					it is still empty.
///
//  ===============================================================================
void mod_settings_for_first_start_with_empty_ext_flash(void)
{
    settingsGetPointer()->debugModeOnStart = 1; //
}



//  ===============================================================================
//  hwOS4_to_hwOS_GasType
/// @brief	Helper for get gas / diluent
///
//  ===============================================================================
uint8_t hwOS4_to_hwOS_GasType(uint8_t inOSTC4style)
{
    switch(inOSTC4style)
    {
        case (1+2): // first
        case (2): 	// first
            return 1; // hwOS style first
        case (1+4): // deco
        case (4): 	// deco
            return 3; // hwOS style deco
        case (1+8): // travel
        case (8): 	// travel
            return 2; // hwOS style travel
        default:
            return 0; // hwOS style Disabled
    }
}


//  ===============================================================================
//  hwOS_to_hwOS4_GasType
/// @brief	Helper for set gas / diluent
///
//  ===============================================================================
uint8_t hwOS_to_hwOS4_GasType(uint8_t inOSTC4style)
{
    switch(inOSTC4style)
    {
        case (1): 		// first
            return 1+2; // hwOS4 style first
        case (2): 		// travel (normal for diluent)
            return 1+8; // hwOS4 style travel
        case (3): 		// deco
            return 1+4; // hwOS4 style deco
        default:
            return 0; 	// hwOS4 style inactive
    }
}



//  ===============================================================================
//  setGas
/// @brief	This function overwrites one gas, including mode and deco depth,
///					it returns 0x4D prompt which is not used by writeData() that calls it.
///
/// @param	i the gas id from 1 to 5 for OC and 6 to 10 for CCR, 0 is the extra gas
/// @param	*data 5 bytes with the first the command to call setGas and the four
///					following bytes to define oxygen, helium, mode and deco depth
///
///	@return	0x4D (prompt that is not used)
//  ===============================================================================
uint8_t setGas(int  i,uint8_t * data)
{
        if(!checkValue(data[1],4,100))
            return ERROR_;
        if(!checkValue(data[4],0,250))
            return ERROR_;

        Settings.gas[i].oxygen_percentage = data[1];
        Settings.gas[i].helium_percentage  = data[2];
        Settings.gas[i].note.uw  = hwOS_to_hwOS4_GasType(data[3]);
        Settings.gas[i].depth_meter  = data[4];
        return 0x4d;
}


uint8_t getGas(int  i,uint8_t * data)
{
    data[0] = Settings.gas[i].oxygen_percentage;
    data[1] = Settings.gas[i].helium_percentage;
    data[2] = hwOS4_to_hwOS_GasType(Settings.gas[i].note.uw);
    data[3] = Settings.gas[i].depth_meter;
    return 0x4d;
}

uint8_t setDiluent(int  i,uint8_t * data)
{
        if(!checkValue(data[1],4,100))
            return ERROR_;
        if(!checkValue(data[4],0,250))
            return ERROR_;

    Settings.gas[NUM_OFFSET_DILUENT + i].oxygen_percentage = data[1];
    Settings.gas[NUM_OFFSET_DILUENT + i].helium_percentage = data[2];
    Settings.gas[NUM_OFFSET_DILUENT + i].note.uw = hwOS_to_hwOS4_GasType(data[3]);
    Settings.gas[NUM_OFFSET_DILUENT + i].depth_meter = data[4];
    return 0x4d;
}

uint8_t getDiluent(int  i,uint8_t * data)
{
    data[0] = Settings.gas[NUM_OFFSET_DILUENT + i].oxygen_percentage;
    data[1] = Settings.gas[NUM_OFFSET_DILUENT + i].helium_percentage;
    data[2] = hwOS4_to_hwOS_GasType(Settings.gas[NUM_OFFSET_DILUENT + i].note.uw);
    data[3] = Settings.gas[NUM_OFFSET_DILUENT + i].depth_meter;
    return 0x4d;
}

uint8_t setSetpoint(int  i,uint8_t * data)
{
        if(!checkValue(data[1],50,160))
            return ERROR_;
        if(!checkValue(data[2],0,250))
            return ERROR_;

    Settings.setpoint[i].setpoint_cbar = data[1];
    Settings.setpoint[i].depth_meter = data[2];
    return 0x4d;
}

uint8_t getSetpoint(int  i,uint8_t * data)
{
    data[0] = Settings.setpoint[i].setpoint_cbar;
    data[1] = Settings.setpoint[i].depth_meter;
    return 0x4d;
}

uint8_t checkValue(uint8_t value,uint8_t from, uint8_t to)
{
    if(value >= from && value <= to)
        return 1;
    return 0;
}

uint8_t writeData(uint8_t * data)
{
        uint32_t newSensitivity;
        uint16_t newDuration, newOffset;
        uint8_t newStopDepth;

    switch(data[0])
    {
    case 0x10:
        return setGas(1,data);
    case 0x11:
        return setGas(2,data);
    case 0x12:
        return setGas(3,data);
    case 0x13:
        return setGas(4,data);
    case 0x14:
        return setGas(5,data);
    case 0x15:
        return setDiluent(1,data);
    case 0x16:
        return setDiluent(2,data);
    case 0x17:
        return setDiluent(3,data);
    case 0x18:
        return setDiluent(4,data);
    case 0x19:
        return setDiluent(5,data);
    case 0x1A:
        return setSetpoint(1,data);
    case 0x1B:
        return setSetpoint(2,data);
    case 0x1C:
        return setSetpoint(3,data);
    case 0x1D:
        return setSetpoint(4,data);
    case 0x1E:
        return setSetpoint(5,data);
    case 0x1F:
        if(!checkValue(data[2],0,1))
            return ERROR_;
        Settings.CCR_Mode = data[1];
        break;
     case 0x20:
         if(!checkValue(data[1],0,3))
            return ERROR_;
        Settings.dive_mode = data[1];
        break;
      case 0x21:
        if(!checkValue(data[1],1,2))
            return ERROR_;
        Settings.deco_type.ub.standard = data[1] & 0x0F;
        //Settings.deco_type.ub.alternative = (data[1] & 0xF0) >> 4;
        break;
      case 0x22:
        if(!checkValue(data[1],100,190))
           return ERROR_;
        Settings.ppO2_max_std = data[1];
        break;
      case 0x23:
        if(!checkValue(data[1],15,15))
           return ERROR_;
        Settings.ppO2_min = data[1];
        break;
     case 0x24:
        if(!checkValue(data[1],0,15))
           return ERROR_;
        Settings.future_TTS = data[1];
        break;
     case 0x25:
        if(!checkValue(data[1],10,99))
            return ERROR_;
        Settings.GF_low = data[1];
        break;
     case 0x26:
        if(!checkValue(data[1],45,99))
            return ERROR_;
        Settings.GF_high = data[1];
        break;
    case 0x27:
        if(!checkValue(data[1],10,99))
            return ERROR_;
        Settings.aGF_low = data[1];
        break;
     case 0x28:
        if(!checkValue(data[1],45,99))
            return ERROR_;
        Settings.aGF_high = data[1];
        break;
    case 0x29:
        if(!checkValue(data[1],0,5))
            return ERROR_;
        Settings.VPM_conservatism.ub.standard = data[1];
        break;
    case 0x2A:
    case 0x2B:
                return ERROR_;
        case 0x2C:
        if(!checkValue(data[1],3,9))
            return ERROR_;
        Settings.last_stop_depth_meter = data[1];
        break;
        case 0x2D:
        if(!checkValue(data[1],0,4))
            return ERROR_;
        Settings.brightness = data[1];
        break;
        case 0x2E:
        if(!checkValue(data[1],0,1))
            return ERROR_;
        Settings.nonMetricalSystem = data[1];
        break;
    case 0x2F:
                return ERROR_;
        case 0x30:
        if(!checkValue(data[1],0,4))
            return ERROR_;
        Settings.salinity = data[1];
        break;
        case 0x31:
        if(!checkValue(data[1],0,3))
            return ERROR_;
        Settings.tX_colorscheme = data[1];
                GFX_use_colorscheme(Settings.tX_colorscheme);
        break;
        case 0x32:
        if(!checkValue(data[1],0,4))
            return ERROR_;
        Settings.selected_language = data[1];
        break;
        case 0x33:
        if(!checkValue(data[1],0,2))
            return ERROR_;
        Settings.date_format = data[1];
        break;
        case 0x34:
                return ERROR_;
        case 0x35:
                if(data[1] & 0x80)
                {
                    data[1] = ~(data[1]);
                    if(!checkValue(data[1],0,PRESSURE_OFFSET_LIMIT_MBAR))
                            return ERROR_;
                    Settings.offsetPressure_mbar = 0 - data[1];
                }
                else
                {
                    if(!checkValue(data[1],0,PRESSURE_OFFSET_LIMIT_MBAR))
                            return ERROR_;
                    Settings.offsetPressure_mbar = data[1];
                }
                break;
        case 0x36:
        if(!checkValue(data[1],0,1))
            return ERROR_;
                if(data[1])
                    Settings.safetystopDuration = settingsGetPointerStandard()->safetystopDuration;
                else
                    Settings.safetystopDuration = 0;
        break;
        case 0x37:
                return ERROR_;
        case 0x38:
        if(!checkValue(data[1],0,1))
            return ERROR_;
        Settings.fallbackToFixedSetpoint = data[1];
        break;
        case 0x39:
                return ERROR_;
        case 0x3A:
        if(!checkValue(data[1],70,110))
            return ERROR_;
                newSensitivity = data[1];
                settingsHelperButtonSens_keepPercentageValues(newSensitivity, settingsGetPointer()->ButtonResponsiveness);
                setButtonResponsiveness(Settings.ButtonResponsiveness);
        break;
        case 0x3B:
                // value between 0 and 127
                if(buttonBalanceTranslatorHexToArray(data[1], settingsGetPointer()->buttonBalance))
                {
                    settingsHelperButtonSens_keepPercentageValues(settingsGetPointer()->ButtonResponsiveness[3], settingsGetPointer()->ButtonResponsiveness);
                }
                else // value >= 128 (bit 7 set) factory reset
                {
                    getButtonFactorDefaults(&settingsGetPointer()->ButtonResponsiveness[3], settingsGetPointer()->buttonBalance);
                    settingsHelperButtonSens_keepPercentageValues(settingsGetPointerStandard()->ButtonResponsiveness[3], settingsGetPointer()->ButtonResponsiveness);
                }
                // valid for both:
                setButtonResponsiveness(Settings.ButtonResponsiveness);
                break;
        case 0x3C:
        if(!checkValue(data[1],5,50))
            return ERROR_;
        Settings.gasConsumption_bottom_l_min = data[1];
        break;
        case 0x3D:
        if(!checkValue(data[1],5,50))
            return ERROR_;
        Settings.gasConsumption_deco_l_min = data[1];
        break;
        case 0x3E:
        if(!checkValue(data[1],5,50))
            return ERROR_;
        Settings.gasConsumption_travel_l_min = data[1];
        break;
        case 0x3F:
        case 0x40:
                return ERROR_;
        case 0x41:
        if(!checkValue(data[1],0,1))
            return ERROR_;
        Settings.alwaysShowPPO2 = data[1];
        break;
        case 0x42:
                if(data[1] & 0x80)
                {
                    data[1] = ~(data[1]);
                    if(!checkValue(data[1],0,20))
                            return ERROR_;
                    Settings.offsetTemperature_centigrad = 0 - data[1];
                }
                else
                {
                    if(!checkValue(data[1],0,20))
                            return ERROR_;
                    Settings.offsetTemperature_centigrad = data[1];
                }
                break;
        case 0x43:
        if(!checkValue(data[1],60,255))
            return ERROR_;
                newDuration = (uint16_t)data[1] + 59;
                newDuration /= 60;
        Settings.safetystopDuration = (uint8_t)newDuration;
        break;
        case 0x44:
        if(!checkValue(data[1],21,61))
            return ERROR_;
                newStopDepth = data[1] + 9;
                if(newStopDepth > 60)
                    newStopDepth = 60;
                newStopDepth /= 10;
        Settings.safetystopDepth = newStopDepth;
        break;
        case 0x45:
        case 0x46:
                return ERROR_;
        case 0x47:
                newOffset = data[2] * 256;
                newOffset += data[1];
                if(newOffset > 9000)
            return ERROR_;
                Settings.logbookOffset = newOffset;
                break;
        case 0x70:
        if(!checkValue(data[1],0,1))
            return ERROR_;
        Settings.showDebugInfo = data[1];
        break;
        case 0x71:
        if(!checkValue(data[1],0,(EXTRADISPLAY_END - 1)))
            return ERROR_;
        Settings.extraDisplay = data[1];
        break;
        case 0x72:
        if(!checkValue(data[1],0,8))
            return ERROR_;
        Settings.tX_customViewPrimary = data[1];
        break;
        case 0x73:
        if(!checkValue(data[1],0,20))
            return ERROR_;
        Settings.tX_customViewTimeout = data[1];
        break;
        case 0x74:
        if(!checkValue(data[1],1,7))
            return ERROR_;
        Settings.tX_userselectedLeftLowerCornerPrimary = data[1];
        break;
        case 0x75:
        if(!checkValue(data[1],0,20))
            return ERROR_;
        Settings.tX_userselectedLeftLowerCornerTimeout = data[1];
        break;
    }
    return 0;
}


uint8_t readDataLimits__8and16BitValues_4and7BytesOutput(uint8_t what, uint8_t * data)
{
    enum JeanDoParameterType {
        PARAM_UNKNOWN = 0,
        PARAM_INT15 = 1,
        PARAM_INT8,
        PARAM_DECI,
        PARAM_CENTI,
        PARAM_MILI,
        PARAM_PERCENT,
        PARAM_SEC,
        PARAM_COLOR,
        PARAM_BOOL,
        PARAM_ENUM,
        PARAM_SIGNED = 128,
        PARAM_SDECI = PARAM_SIGNED|PARAM_DECI,
        PARAM_SSEC = PARAM_SIGNED|PARAM_SEC,
        PARAM_SINT = PARAM_SIGNED|PARAM_INT8
    };

//		uint32_t buttonSensitivity;
        uint16_t newDuration;

        uint8_t datacounter = 0;

    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
        datacounter = 0;

    switch(what)
    {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 4;
        data[datacounter++] = settingsGetPointerStandard()->gas[1].oxygen_percentage;
        data[datacounter++] = 100;
        break;

    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 4;
        data[datacounter++] = settingsGetPointerStandard()->gas[1].oxygen_percentage;
        data[datacounter++] = 100;
        break;

    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        data[datacounter++] = PARAM_CENTI;
        data[datacounter++] = 50;
        data[datacounter++] = settingsGetPointerStandard()->setpoint[1].setpoint_cbar;
        data[datacounter++] = 160;
        break;

    case 0x1F:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->CCR_Mode;
        data[datacounter++] = 1;
        break;

    case 0x20:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->dive_mode;
        data[datacounter++] = 3;
        break;

    case 0x21:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 1;
        data[datacounter++] = settingsGetPointerStandard()->deco_type.ub.standard;
        data[datacounter++] = 2;
        break;

    case 0x22:
        data[datacounter++] = PARAM_CENTI;
        data[datacounter++] = 100;
        data[datacounter++] = settingsGetPointerStandard()->ppO2_max_std;
        data[datacounter++] = 190;
        break;

    case 0x23:
        data[datacounter++] = PARAM_CENTI;
        data[datacounter++] = 15;
        data[datacounter++] = settingsGetPointerStandard()->ppO2_min;
        data[datacounter++] = 15;
        break;

    case 0x24:
        data[datacounter++] = PARAM_INT8; // minutes
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->future_TTS;
        data[datacounter++] = 15;
        break;

    case 0x25:
        data[datacounter++] = PARAM_PERCENT;
        data[datacounter++] = 10;
        data[datacounter++] = settingsGetPointerStandard()->GF_low;
        data[datacounter++] = 99;
        break;

    case 0x26:
        data[datacounter++] = PARAM_PERCENT;
        data[datacounter++] = 45;
        data[datacounter++] = settingsGetPointerStandard()->GF_high;
        data[datacounter++] = 99;
        break;

    case 0x27:
        data[datacounter++] = PARAM_PERCENT;
        data[datacounter++] = 10;
        data[datacounter++] = settingsGetPointerStandard()->aGF_low;
        data[datacounter++] = 99;
        break;

    case 0x28:
        data[datacounter++] = PARAM_PERCENT;
        data[datacounter++] = 45;
        data[datacounter++] = settingsGetPointerStandard()->aGF_high;
        data[datacounter++] = 99;
        break;

    case 0x29:
        data[datacounter++] = PARAM_INT8; // conservatism +0 .. +5
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->VPM_conservatism.ub.standard;
        data[datacounter++] = 5;
        break;

    case 0x2A:
    case 0x2B:
        data[datacounter++] = PARAM_PERCENT;
        data[datacounter++] = 100;
        data[datacounter++] = 100;// saturation, desaturation, settingsGetPointerStandard()->;
        data[datacounter++] = 100;
        break;

        case 0x2C:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 3;
        data[datacounter++] = settingsGetPointerStandard()->last_stop_depth_meter;
        data[datacounter++] = 9;
        break;

        case 0x2D:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->brightness;
        data[datacounter++] = 4;
        break;

    case 0x2E:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->nonMetricalSystem;
        data[datacounter++] = 1;
        break;

    case 0x2F:
        data[datacounter++] = PARAM_INT8; // Sampling rate logbook
        data[datacounter++] = 2;
        data[datacounter++] = 2;
        data[datacounter++] = 2;
        break;

    case 0x30:
        data[datacounter++] = PARAM_PERCENT;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->salinity;
        data[datacounter++] = 4;
        break;

    case 0x31:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->tX_colorscheme;
        data[datacounter++] = 3;
        break;

    case 0x32:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->selected_language;
        data[datacounter++] = 1;
        break;

    case 0x33:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->date_format;
        data[datacounter++] = 2;
        break;

    case 0x34:
        data[datacounter++] = PARAM_UNKNOWN ;
        data[datacounter++] = 0;
        data[datacounter++] = 0; // compass gain, is unknown,, settingsGetPointerStandard()->;
        data[datacounter++] = 0;
        break;

    case 0x35:
        data[datacounter++] =  PARAM_SINT;
        data[datacounter++] = (uint8_t)(256 - PRESSURE_OFFSET_LIMIT_MBAR); // == -20
        if(settingsGetPointerStandard()->offsetPressure_mbar < 0)
            data[datacounter++] = (uint8_t)(127 - settingsGetPointerStandard()->offsetPressure_mbar);
        else
            data[datacounter++] = settingsGetPointerStandard()->offsetPressure_mbar;
        data[datacounter++] = PRESSURE_OFFSET_LIMIT_MBAR;
        break;

    case 0x36:
        data[datacounter++] = PARAM_BOOL;
        data[datacounter++] = 0;
        if(settingsGetPointerStandard()->safetystopDuration)
            data[datacounter++] = 1;
        else
            data[datacounter++] = 0;
        data[datacounter++] = 1;
        break;

    case 0x37:
        data[datacounter++] = PARAM_UNKNOWN ;
        data[datacounter++] = 0;
        data[datacounter++] = 0; // Set calibration gas, not possible with optical
        data[datacounter++] = 0;
        break;

    case 0x38:
        data[datacounter++] = PARAM_BOOL;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->fallbackToFixedSetpoint;
        data[datacounter++] = 1;
        break;

    case 0x39:
        data[datacounter++] = PARAM_BOOL;
        data[datacounter++] = 0;
        data[datacounter++] = 0; // flipscreen, not yet :-) settingsGetPointerStandard()->;
        data[datacounter++] = 0;
        break;

    case 0x3A:
        data[datacounter++] = PARAM_PERCENT;
        data[datacounter++] = 70;
        data[datacounter++] = settingsGetPointerStandard()->ButtonResponsiveness[3];
        data[datacounter++] = 110;
        break;

    case 0x3B:
        data[datacounter++] = PARAM_UNKNOWN;
        data[datacounter++] = 0;
        data[datacounter++] = buttonBalanceTranslateArrayOutHex(settingsGetPointerStandard()->buttonBalance);
        data[datacounter++] = 128;
        break;

    case 0x3C:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 5;
        data[datacounter++] = settingsGetPointerStandard()->gasConsumption_bottom_l_min;
        data[datacounter++] = 50;
        break;

    case 0x3D:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 5;
        data[datacounter++] = settingsGetPointerStandard()->gasConsumption_deco_l_min;
        data[datacounter++] = 50;
        break;

    case 0x3E:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 5;
        data[datacounter++] = settingsGetPointerStandard()->gasConsumption_travel_l_min;
        data[datacounter++] = 50;
        break;

    case 0x3F:
        data[datacounter++] = PARAM_UNKNOWN;
        data[datacounter++] = 0;
        data[datacounter++] = 0; // Dynamic ascend rate,  not yet :-) settingsGetPointerStandard()->;
        data[datacounter++] = 0;
        break;

    case 0x40:
        data[datacounter++] = PARAM_BOOL;
        data[datacounter++] = 1;
        data[datacounter++] = 1; // Graphical speed indicator;
        data[datacounter++] = 1;
        break;

    case 0x41:
        data[datacounter++] = PARAM_BOOL;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->alwaysShowPPO2;
        data[datacounter++] = 1;
        break;

    case 0x42:
        data[datacounter++] = PARAM_SIGNED|PARAM_CENTI;
        data[datacounter++] = (uint8_t)(256 - 20); // == -20
        if(settingsGetPointerStandard()->offsetTemperature_centigrad < 0)
            data[datacounter++] = (uint8_t)(127 - settingsGetPointerStandard()->offsetTemperature_centigrad);
        else
            data[datacounter++] = settingsGetPointerStandard()->offsetTemperature_centigrad;
        data[datacounter++] = 20;
        break;

    case 0x43:
        newDuration = settingsGetPointerStandard()->safetystopDuration;
        newDuration *= 60;
        if(newDuration > 255)
            newDuration = 255;
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 60; // coud be 1 minute instead
        data[datacounter++] = (uint8_t)newDuration;
        data[datacounter++] = 255; // could be 5 minutes instead
        break;

    case 0x44:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 30; // coud be 3 meter instead
        data[datacounter++] = settingsGetPointerStandard()->safetystopDepth * 10;
        data[datacounter++] = 60; // could be 6 meter instead
        break;

    case 0x45:
    case 0x46:
        data[datacounter++] = PARAM_UNKNOWN;
        data[datacounter++] = 0;
        data[datacounter++] = 0; // SafetyStop End Depth and SafetyStop Reset Depth
        data[datacounter++] = 0;
        break;

    case 0x47:
        data[datacounter++] = PARAM_INT15;
        data[datacounter++] = 0;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->logbookOffset & 0xFF;
        data[datacounter++] = settingsGetPointerStandard()->logbookOffset / 0xFF;
        data[datacounter++] = 9000 & 0xFF;
        data[datacounter++] = 9000 / 0xFF;
        break;

    case 0x70:
        data[datacounter++] = PARAM_BOOL;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->showDebugInfo;
        data[datacounter++] = 1;
        break;

    case 0x71:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->extraDisplay;
        data[datacounter++] = (EXTRADISPLAY_END - 1);
        break;

    case 0x72:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->tX_customViewPrimary;
        data[datacounter++] = 8;
        break;

    case 0x73:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->tX_customViewTimeout;
        data[datacounter++] = 60;
        break;

    case 0x74:
        data[datacounter++] = PARAM_ENUM;
        data[datacounter++] = 1;
        data[datacounter++] = settingsGetPointerStandard()->tX_userselectedLeftLowerCornerPrimary;
        data[datacounter++] = 7;
        break;

    case 0x75:
        data[datacounter++] = PARAM_INT8;
        data[datacounter++] = 0;
        data[datacounter++] = settingsGetPointerStandard()->tX_userselectedLeftLowerCornerTimeout;
        data[datacounter++] = 60;
        break;
    }

    if(datacounter == 0)
    {
        data[datacounter++] = PARAM_UNKNOWN;
        data[datacounter++] = 0;
        data[datacounter++] = 0; // SafetyStop End Depth and SafetyStop Reset Depth
        data[datacounter++] = 0;
    }

    return datacounter;
}


uint8_t readData(uint8_t what, uint8_t * data)
{
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    switch(what)
    {
    case 0x10:
        return getGas(1,data);
    case 0x11:
        return getGas(2,data);
    case 0x12:
        return getGas(3,data);
    case 0x13:
        return getGas(4,data);
    case 0x14:
        return getGas(5,data);
    case 0x15:
        return getDiluent(1,data);
    case 0x16:
        return getDiluent(2,data);
    case 0x17:
        return getDiluent(3,data);
    case 0x18:
        return getDiluent(4,data);
    case 0x19:
        return getDiluent(5,data);
    case 0x1A:
        return getSetpoint(1,data);
    case 0x1B:
        return getSetpoint(2,data);
    case 0x1C:
        return getSetpoint(3,data);
    case 0x1D:
        return getSetpoint(4,data);
    case 0x1E:
        return getSetpoint(5,data);
    case 0x1F:
        data[0] = Settings.CCR_Mode;
        break;
     case 0x20:
        data[0] = Settings.dive_mode;
        break;
      case 0x21:
        data[0] = Settings.deco_type.ub.standard;
        break;
      case 0x22:
        data[0] = Settings.ppO2_max_std;
        break;
      case 0x23:
        data[0] = Settings.ppO2_min;
        break;
     case 0x24:
        data[0] = Settings.future_TTS;
        break;
     case 0x25:
        data[0] = Settings.GF_low;
        break;
     case 0x26:
        data[0] = Settings.GF_high;
        break;
    case 0x27:
        data[0] = Settings.aGF_low;
        break;
     case 0x28:
        data[0] = Settings.aGF_high;
        break;
    case 0x29:
        data[0] = Settings.VPM_conservatism.ub.standard;
                break;
    case 0x2A:
    case 0x2B:
        data[0] = 100;
        break;
     case 0x2C:
        data[0] = Settings.last_stop_depth_meter;
        break;
     case 0x2D:
        data[0] = Settings.brightness;
        break;
     case 0x2E:
        data[0] = Settings.nonMetricalSystem;
        break;
     case 0x2F:
        data[0] = 0; // 0 == 2 sec sampling rate
        break;
     case 0x30:
        data[0] = Settings.salinity;
        break;
     case 0x31:
        data[0] = Settings.tX_colorscheme;
        break;
     case 0x32:
        data[0] = Settings.selected_language;
        break;
     case 0x33:
        data[0] = Settings.date_format;
        break;
     case 0x34:
        data[0] = 7; // gain should be always 7 as far as I understand the code in RTE
        break;
     case 0x35:
        data[0] = Settings.offsetPressure_mbar;
        break;
     case 0x36:
        if(Settings.safetystopDepth)
            data[0] = 1;
        else
            data[0] = 0;
        break;
     case 0x37:
        data[0] = 0; // calibration gas %O2 -> 0 no gas :-)
        break;
     case 0x38:
        data[0] = Settings.fallbackToFixedSetpoint;
        break;
     case 0x39:
        data[0] = 0; // flip screen
        break;
     case 0x3A:
        data[0] = Settings.ButtonResponsiveness[3];
        break;
     case 0x3B:
        data[0] = buttonBalanceTranslateArrayOutHex(settingsGetPointer()->buttonBalance);
        break;
     case 0x3C:
        data[0] = Settings.gasConsumption_bottom_l_min;
        break;
     case 0x3D:
        data[0] = Settings.gasConsumption_deco_l_min;
        break;
     case 0x3E:
        data[0] = Settings.gasConsumption_travel_l_min;
        break;
     case 0x3F:
        data[0] = 0; // fixed ascend rate 10 m/min
        break;
     case 0x40:
        data[0] = 1; // graphical speed indicator
        break;
     case 0x41:
        data[0] = Settings.alwaysShowPPO2;
        break;
     case 0x42:
        data[0] = Settings.offsetTemperature_centigrad;
        break;
     case 0x43:
        if(Settings.safetystopDuration > 4)
            data[0] = 255; // seconds
        else
            data[0] = 60 * Settings.safetystopDuration;
        break;
     case 0x44:
        data[0] = Settings.safetystopDepth * 10; // cbar instead of meter
        break;
     case 0x45:
        if(Settings.safetystopDepth == 3)
            data[0] = 20; // cbar
        else
            data[0] = 30; // cbar
        break;
     case 0x46:
        data[0] = 10; // reset at 10 meter as far as I understood
        break;
     case 0x47:
        data[0] = Settings.logbookOffset & 0xFF;
        data[1] = Settings.logbookOffset / 0xFF;
        break;
     case 0x70:
        data[0] = Settings.showDebugInfo;
        break;
     case 0x71:
        data[0] = Settings.extraDisplay;
        break;
     case 0x72:
        data[0] = Settings.tX_customViewPrimary;
        break;
     case 0x73:
        data[0] = Settings.tX_customViewTimeout;
        break;
     case 0x74:
        data[0] = Settings.tX_userselectedLeftLowerCornerPrimary;
        break;
     case 0x75:
        data[0] = Settings.tX_userselectedLeftLowerCornerTimeout;
        break;
        }
    return 0x4D;
}


uint8_t RTEminimum_required_high(void)
{
    return RTErequiredHigh;
}
uint8_t RTEminimum_required_low(void)
{
    return RTErequiredLow;
}

uint8_t FONTminimum_required_high(void)
{
    return FONTrequiredHigh;
}
uint8_t FONTminimum_required_low(void)
{
    return FONTrequiredLow;
}


void setActualRTEversion(uint8_t high, uint8_t low)
{
    RTEactualHigh = high;
    RTEactualLow = low;
}


void getActualRTEandFONTversion(uint8_t *RTEhigh, uint8_t *RTElow, uint8_t *FONThigh, uint8_t *FONTlow)
{
    if(RTEhigh && RTElow)
    {
        *RTEhigh = RTEactualHigh;
        *RTElow = RTEactualLow;
    }
    if(FONThigh && FONTlow)
    {
        *FONThigh = *(uint8_t *)0x08132000;
        *FONTlow = *(uint8_t *)0x08132001;
    }
}


uint8_t getLicence(void)
{
    return hardwareDataGetPointer()->primaryLicence;
}


void firmwareGetDate(RTC_DateTypeDef *SdateOutput)
{
    SdateOutput->Year = firmwareDataGetPointer()->release_year;
    SdateOutput->Month = firmwareDataGetPointer()->release_month;
    SdateOutput->Date = firmwareDataGetPointer()->release_day;
}


// this should use device specific values stored in OTPROG ROM soon
void getButtonFactorDefaults(uint8_t* basePercentage, uint8_t* buttonBalanceArray)
{
    *basePercentage = settingsGetPointerStandard()->ButtonResponsiveness[3];

    for(int i=0;i<3;i++)
    {
        buttonBalanceArray[i] = settingsGetPointerStandard()->buttonBalance[i];
    }
}


uint8_t buttonBalanceTranslatorHexToArray(uint8_t hexValue, uint8_t* outputArray)
{
    if(hexValue > 127)
        return 0;
    // internal order: 0 = right, 1 = center, 2 = left
    // external order: Factory,left,center,right
    outputArray[0] = 2 + (hexValue & 0x03);
    hexValue /= 4;
    outputArray[1] = 2 + (hexValue & 0x03);
    hexValue /= 4;
    outputArray[2] =  2 + (hexValue & 0x03);

    return 1;
}


uint8_t buttonBalanceTranslateArrayOutHex(const uint8_t* inputArray)
{
    uint8_t hexValue = 0;

    if(inputArray[2] > 2)
    {
        hexValue += inputArray[2] - 2;
    }
    hexValue *= 4;

    if(inputArray[1] > 2)
    {
        hexValue += inputArray[1] - 2;
    }
    hexValue *= 4;
    if(inputArray[0] > 2)
    {
        hexValue += inputArray[0] - 2;
    }
    return hexValue;
}

void settingsWriteFactoryDefaults(uint8_t inputValueRaw, uint8_t *inputBalanceArray)
{
    if((inputValueRaw >= 70) && (inputValueRaw <= 110))
    {
        Settings.FactoryButtonBase = inputValueRaw;
    }
    for(int i=0;i<3;i++)
    {
        if((inputBalanceArray[i] >= 2) && (inputBalanceArray[i] <= 5))
        {
            Settings.FactoryButtonBalance[i] = inputBalanceArray[i];
        }
    }
}


/**
  ******************************************************************************
  * @brief   settingsHelperButtonSens. /  make 32 bit input to three buttons + storage value in [3]
  * @author  heinrichs weikamp gmbh
  * @version V 01
  * @date    19-Sept-2016
  ******************************************************************************
    *
  * @param  inputValueRaw:
  * @param  outArray4Values: [0] is right, [1] is center, [2] is left, [3] is original value with zero balance
  * @retval None
  */
void settingsHelperButtonSens_keepPercentageValues(uint32_t inputValueRaw, uint8_t *outArray4Values)
{
    uint32_t newSensitivity;

    if(inputValueRaw > MAX_BUTTONRESPONSIVENESS)
    {
            inputValueRaw = MAX_BUTTONRESPONSIVENESS;
    }
    else
    if(inputValueRaw < MIN_BUTTONRESPONSIVENESS)
    {
            inputValueRaw = MIN_BUTTONRESPONSIVENESS;
    }

    // the unbalanced value
    outArray4Values[3] = inputValueRaw;

    // the balanced values
    for(int i=0;i<3;i++)
    {
        newSensitivity = inputValueRaw;
        switch(settingsGetPointer()->buttonBalance[i])
        {
        case 1: // should not be an option hw 170508
            newSensitivity -= 20;
            break;
        case 2:
            newSensitivity -= 10;
            break;
        default:
            break;
        case 4:
            newSensitivity += 10;
            break;
        case 5:
            newSensitivity += 20;
            break;
        }

        if(newSensitivity > MAX_BUTTONRESPONSIVENESS)
        {
                newSensitivity = MAX_BUTTONRESPONSIVENESS;
        }
        outArray4Values[i] = newSensitivity;
    }
}


/**
  ******************************************************************************
  * @brief   settingsHelperButtonSens_translate_to_hwOS_values. /  make 32 bit input to three buttons + storage value in [3]
  * @author  heinrichs weikamp gmbh
  * @version V 01
  * @date    19-Sept-2016
  ******************************************************************************
    *
  * @param  inputValueRaw:
  * @param  outArray4Values: [0] is right, [1] is center, [2] is left, [3] is original value with zero balance
  * @retval None
  */
void settingsHelperButtonSens_original_translate_to_hwOS_values(const uint32_t inputValueRaw, uint8_t *outArray4Values)
{
    uint32_t newSensitivity;

    for(int i=0;i<3;i++)
    {
        newSensitivity = inputValueRaw;
        switch(settingsGetPointer()->buttonBalance[i])
        {
        case 1:
            newSensitivity -= 20;
            break;
        case 2:
            newSensitivity -= 10;
            break;
        default:
            break;
        case 4:
            newSensitivity += 10;
            break;
        case 5:
            newSensitivity += 20;
            break;
        }

        if(newSensitivity > 100)
        {
            if(newSensitivity <= 105)
                newSensitivity = 10;
            else
                newSensitivity = 7;
        }
        else
        {
            newSensitivity *= 24;
            newSensitivity = 2400 - newSensitivity;
            newSensitivity /= 10;

            newSensitivity += 15;
            if(newSensitivity > 255)
                newSensitivity = 255;
        }
        outArray4Values[i] = newSensitivity;
    }

    // the unbalanced value
    newSensitivity = inputValueRaw;
    if(newSensitivity > 100)
    {
        if(newSensitivity <= 105)
            newSensitivity = 10;
        else
            newSensitivity = 7;
    }
    else
    {
        newSensitivity *= 24;
        newSensitivity = 2400 - newSensitivity;
        newSensitivity /= 10;

        newSensitivity += 15;
        if(newSensitivity > 255)
            newSensitivity = 255;
    }
    outArray4Values[3] = newSensitivity;
}


/**
  ******************************************************************************
  * @brief   settingsHelperButtonSens_translate_percentage_to_hwOS_values.
  * @author  heinrichs weikamp gmbh
  * @version V 01
  * @date    6-March-2017
  ******************************************************************************
    *
  * @param  inputValuePercentage with buttonBalance included
  * @retval PIC compatible value
  */
uint8_t settingsHelperButtonSens_translate_percentage_to_hwOS_values(uint8_t inputValuePercentage)
{
    uint32_t newSensitivity = inputValuePercentage;

    if(newSensitivity > 100)
    {
        if(newSensitivity <= 105)
            newSensitivity = 10;
        else
            newSensitivity = 7;
    }
    else
    {
        newSensitivity *= 24;
        newSensitivity = 2400 - newSensitivity;
        newSensitivity /= 10;

        newSensitivity += 15;
        if(newSensitivity > 255)
            newSensitivity = 255;
    }
    return (uint8_t)newSensitivity;
}


/**
  ******************************************************************************
  * @brief   settingsHelperButtonSens_translate_hwOS_values_to_percentage.
  * @author  heinrichs weikamp gmbh
  * @version V 01
  * @date    6-March-2017
  ******************************************************************************
    *
  * @param	PIC compatible value
  * @retval	Percentage
  */
uint8_t settingsHelperButtonSens_translate_hwOS_values_to_percentage(uint8_t inputValuePIC)
{
    if(inputValuePIC >= 15)
    {
        return(uint8_t)((25500 - (inputValuePIC)*100) / 240);
    }
    else
    {
        if(inputValuePIC >= 10)
        {
            return 105;
        }
        else
        {
            return 110;
        }
    }
}

void reset_SettingWarning()
{
	settingsWarning = 0;
}
inline uint8_t isSettingsWarning()
{
	return settingsWarning;
}
