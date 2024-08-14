///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Common/Inc/settings.h
/// \brief
/// \author Heinrichs Weikamp
/// \date   2018
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

// From Common/Inc:
#include "FirmwareData.h"

//#include "data_central.h"

#include "global_constants.h"
// From Common/Drivers/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rtc.h"

#include "configuration.h"


#define NUM_GASES 5
#define NUM_OFFSET_DILUENT 5
#define SPECIAL_GAS_ID 0
#define NO_GAS_ID 255

#define ERROR_ 0xFF

#define CCRMODE_FixedSetpoint 0
#define CCRMODE_Sensors 1
#define CCRMODE_Simulation 2

#define DIVEMODE_OC 0
#define DIVEMODE_CCR 1
#define DIVEMODE_Gauge 2
#define DIVEMODE_Apnea 3
#define DIVEMODE_PSCR 4

#define GF_MODE 1
#define VPM_MODE 2

#define VPM_FROM_FORTRAN 0
#define VPM_BACHELORWORK 1

#define BUEHLMANN_OSTC4	0
#define BUEHLMANN_hwOS	1

#define MMDDYY 0
#define DDMMYY 1
#define YYMMDD 2

#define PRESSURE_OFFSET_LIMIT_MBAR	50

#define MAX_COMPASS_COMP 		(2u)
#define MAX_VIEWPORT_MODE 		(0x7F)

#define MAX_SCRUBBER_TIME 		(999u)
#define MIN_SCRUBBER_TIME       -99
#define MIN_PPO2_SP_CBAR		(40u)

#define PSCR_MAX_O2_DROP		(15u)
#define PSCR_MIN_LUNG_RATIO		(5u)
#define PSCR_MAX_LUNG_RATIO		(20u)

#define UART_MAX_PROTOCOL		(2u)

#define FUTURE_SPARE_SIZE		(0u)		/* Applied for reuse of old, not used, scooter block (was 32 bytes)*/

typedef enum
{
	O2_SENSOR_SOURCE_OPTIC = 0,
	O2_SENSOR_SOURCE_ANALOG,
	O2_SENSOR_SOURCE_DIGITAL,
	O2_SENSOR_SOURCE_ANADIG,
#ifdef ENABLE_SENTINEL_MODE
	O2_SENSOR_SOURCE_SENTINEL,
#endif
	O2_SENSOR_SOURCE_MAX
} SensorSource;
#define MAX_O2_SENSOR_SOURCE 	(2u)

typedef enum
{
	SCRUB_TIMER_OFF = 0,
	SCRUB_TIMER_MINUTES,
	SCRUB_TIMER_PERCENT,
	SCRUB_TIMER_END
} ScrubTimerMode_e;

/*	2015 Jan 30, hw, deco and travel added for MenuEditGas
	* can be used for buehlmann, vpm, etc. later but be carefull
	*	with current implemenation */
typedef struct{
uint8_t active:1;
uint8_t first:1;
uint8_t deco:1;
uint8_t travel:1;
uint8_t off:1;
uint8_t decocalc:1;
uint8_t spare:2;
} gasubit8_t;

typedef union{
gasubit8_t ub;
uint8_t uw;
} gasbit8_Type;

typedef struct{
uint8_t standard:4;
uint8_t alternative:4;
} ubit2x4_t;

typedef union{
ubit2x4_t ub;
uint8_t uw;
} split2x4_Type;

typedef union{
uint8_t u8[4];
uint32_t u32;
} general32to8_Type;

typedef struct
{
	uint8_t oxygen_percentage;
	uint8_t helium_percentage;
	gasbit8_Type note;
	uint8_t depth_meter;
	uint8_t depth_meter_travel;
	uint8_t bottle_size_liter;
//	uint8_t bottle_wireless_status;
	uint16_t bottle_wireless_id;
} SGasLine;

typedef struct
{
	uint8_t setpoint_cbar;
	uint8_t depth_meter;
	gasbit8_Type note;
} SSetpointLine;


typedef struct
{
	uint16_t TimerMax;
	int16_t TimerCur;
	RTC_DateTypeDef lastDive;
} SScrubberData;

enum {
    SETPOINT_INDEX_CUSTOM = 0,
    SETPOINT_INDEX_AUTO_LOW,
    SETPOINT_INDEX_AUTO_HIGH,
    SETPOINT_INDEX_AUTO_DECO,
};

typedef struct
{
	uint8_t FirstCorrection;
	uint8_t Corrections;
} SSettingsStatus;

/* SSettings
	 * gas[0] and setpoint[0] are the special ones configurable during the dive
	 */
typedef struct
{
	uint32_t header;
	uint8_t warning_blink_dsec;
	uint8_t lastDiveLogId;
	uint32_t logFlashNextSampleStartAddress;
	SGasLine gas[1 + (2*NUM_GASES)];
	SSetpointLine setpoint[1 + NUM_GASES];
	uint8_t CCR_Mode;
	uint8_t dive_mode;
	split2x4_Type deco_type;
	uint8_t ppO2_max_deco;
	uint8_t ppO2_max_std;
	uint8_t ppO2_min;
	uint8_t CNS_max;
	uint8_t ascent_MeterPerMinute_max;
	uint8_t ascent_MeterPerMinute_showGraph;
	uint8_t future_TTS;
	uint8_t GF_high;
	uint8_t GF_low;
	uint8_t aGF_high;
	uint8_t aGF_low;
	split2x4_Type VPM_conservatism;
	uint8_t safetystopDuration;
	uint8_t AtemMinutenVolumenLiter;
	uint8_t ReserveFractionDenominator;
	uint8_t salinity;
	uint8_t last_stop_depth_meter;
	uint8_t stop_increment_depth_meter;
	uint8_t brightness;
	uint8_t date_format;
	uint8_t selected_language;
	char customtext[60];
	uint16_t timeoutSurfacemode;
	uint8_t timeoutMenuSurface;
	uint8_t timeoutMenuDive;
	uint8_t timeoutMenuEdit;
	uint8_t timeoutInfo;
	uint8_t timeoutInfoCompass;
	uint8_t design;
	uint16_t timeoutDiveReachedZeroDepth;
	uint16_t divetimeToCreateLogbook;
	uint8_t  serialHigh;
	uint8_t  serialLow;
//	SUFirmware  firmwareVersion16to32bit;
	uint32_t backup_localtime_rtc_tr;
	uint32_t backup_localtime_rtc_dr;
	uint16_t totalDiveCounter;
	uint16_t personalDiveCount;
	uint8_t showDebugInfo;
	uint8_t ButtonResponsiveness[4];// changed content in 0xFFFF0016
	uint8_t nonMetricalSystem;
	uint8_t fallbackToFixedSetpoint;
	uint8_t bluetoothActive; /* will be set to zero on each startup at the moment */
	uint8_t safetystopDepth;
	uint32_t updateSettingsAllowedFromHeader;
	uint8_t pscr_lung_ratio;									/* redefined in 0xFFFF0020 */
	uint8_t pscr_o2_drop;										/* redefined in 0xFFFF0020 */
	uint8_t co2_sensor_active;									/* redefined in 0xFFFF0021 */
	uint8_t ext_uart_protocol;									/* redefined in 0xFFFF0022 */

	uint8_t scubberActiveId;									/* redefined in 0xFFFF0023 */
	SScrubberData scrubberData[2];
	uint8_t ext_sensor_map_Obsolete[5];
	uint8_t buttonLockActive;									/* redefined in 0xFFFF0025 */
	int8_t compassDeclinationDeg;
	uint8_t delaySetpointLow;
	uint16_t timerDurationS;										/* redefined in 0xFFFF0026 */
	uint8_t Future_SPARE[FUTURE_SPARE_SIZE];					/* redefined in 0xFFFF0020 (old scooter Block was 32 byte)*/
	// new in 0xFFFF0006
	uint8_t ppo2sensors_deactivated;
	uint8_t tX_colorscheme;
	uint8_t tX_userselectedLeftLowerCornerPrimary;
	uint8_t tX_userselectedLeftLowerCornerTimeout;
	uint8_t tX_customViewPrimary;
	uint8_t tX_customViewTimeout;
	uint8_t timeoutEnterButtonSelectDive;
	uint16_t logbookOffset;
	uint8_t alwaysShowPPO2;
	uint8_t extraDisplay;
	uint16_t display_toogle_desc;
	int8_t offsetPressure_mbar;
	int8_t offsetTemperature_centigrad;
	uint8_t gasConsumption_travel_l_min;
	uint8_t gasConsumption_bottom_l_min;
	uint8_t gasConsumption_deco_l_min;
	uint8_t debugModeOnStart;
	uint8_t IAmStolenPleaseKillMe;
	int16_t compassBearing;
	uint8_t lastKnownBatteryPercentage;
	uint8_t buttonBalance[3]; // 0 = right, 1 = center, 2 = left
	uint8_t firmwareVersion[4];
	uint16_t timeoutSurfacemodeWithSensors;
	// new in 0xFFFF0016
	uint8_t VPM_model;
	uint8_t	GF_model;
	// new in 0xFFFF0017
	uint8_t FactoryButtonBase;
	uint8_t FactoryButtonBalance[3];
	/* new in 0xFFFF0018 */
	uint8_t FlipDisplay;
	/* new in 0xFFFF0019 */
	uint32_t cv_configuration;
	/* new in 0xFFFF001A */
	uint8_t MotionDetection;
	/* new in 0xFFFF001B */
	uint32_t cv_config_BigScreen;
	/* new in 0xFFFF001C */
	uint8_t compassInertia;
	uint8_t tX_customViewPrimaryBF;
	/* new in 0xFFFF001D */
	uint8_t  viewPortMode;		/* 7-Reserve| 6..5 - Focus spot size | 4-Focusframe |  3-Reserve | 2..0-BacklightBoost */
	uint16_t viewRoll;
	uint16_t viewPitch;
	uint16_t viewYaw;
	/* new in 0xFFFF001E */
	uint8_t ppo2sensors_source;
	float   ppo2sensors_calibCoeff[3];
	uint8_t amPMTime;
	/* new in 0xFFFF001F */
	uint8_t autoSetpoint;
	uint16_t scrubTimerMax_Obsolete;	/* have been replaced with new scrubber data format */
	uint16_t scrubTimerCur_Obsolete;	/* have been replaced with new scrubber data format */
	uint8_t scrubTimerMode;
	uint8_t ext_sensor_map[8];		/* redefined in 0xFFFF0027 */
	uint8_t cvAutofocus;
} SSettings;

typedef struct
{
	// 8 bytes
	uint16_t primarySerial;
	uint8_t primaryLicence;
	uint8_t revision8bit;
	uint8_t production_year;
	uint8_t production_month;
	uint8_t production_day;
	uint8_t production_bluetooth_name_set;

	// 44 bytes
	char production_info[44];

	// 8 bytes
	uint16_t secondarySerial;
	uint8_t secondaryLicence;
	uint8_t secondaryReason8bit;
	uint8_t secondary_year;
	uint8_t secondary_month;
	uint8_t secondary_day;
	uint8_t secondary_bluetooth_name_set;

	// 4 bytes
	char secondary_info[4];
} SHardwareData;

uint8_t writeData(uint8_t *);
uint8_t readData(uint8_t what,uint8_t *);
uint8_t readDataLimits__8and16BitValues_4and7BytesOutput(uint8_t what, uint8_t * data);

uint8_t getPPO2Max(void);
uint8_t getPPO2Min(void);
uint8_t getDiveMode(void);
uint8_t getCCRMode(void);
uint8_t getDecoType(void);
uint8_t getFutureTTS(void);

SSettings* settingsGetPointer(void);
const SSettings* settingsGetPointerStandard(void);
void set_settings_to_Standard(void);
void mod_settings_for_first_start_with_empty_ext_flash(void);
const SFirmwareData* firmwareDataGetPointer(void);
const SHardwareData* hardwareDataGetPointer(void);
uint8_t firmwareVersion_16bit_high(void);
uint8_t firmwareVersion_16bit_low(void);
void hardwareBatchCode(uint8_t *high, uint8_t *low);

uint8_t RTEminimum_required_high(void);
uint8_t RTEminimum_required_low(void);
uint8_t FONTminimum_required_high(void);
uint8_t FONTminimum_required_low(void);

void setActualRTEversion(uint8_t high, uint8_t low);
void getActualRTEandFONTversion(uint8_t *RTEhigh, uint8_t *RTElow, uint8_t *FONThigh, uint8_t *FONTlow);

void set_new_settings_missing_in_ext_flash(void);
uint8_t check_and_correct_settings(void);
uint8_t newFirmwareVersionCheckViaSettings(void);
void set_settings_button_to_factory_with_individual_buttonBalance(void);
uint8_t getLicence(void);
void firmwareGetDate(RTC_DateTypeDef *SdateOutput);

void settingsHelperButtonSens_original_translate_to_hwOS_values(const uint32_t inputValueRaw, uint8_t *outArray4Values);

uint8_t buttonBalanceTranslatorHexToArray(uint8_t hexValue, uint8_t* outputArray);
uint8_t buttonBalanceTranslateArrayOutHex(const uint8_t* inputArray);
void getButtonFactorDefaults(uint8_t* basePercentage, uint8_t* buttonBalanceArray);
void settingsWriteFactoryDefaults(uint8_t inputValueRaw, uint8_t *inputBalanceArray);

void settingsHelperButtonSens_keepPercentageValues(uint32_t inputValueRaw, uint8_t *outArray4Values);
uint8_t settingsHelperButtonSens_translate_percentage_to_hwOS_values(uint8_t inputValuePercentage);
uint8_t settingsHelperButtonSens_translate_hwOS_values_to_percentage(uint8_t inputValuePIC);

void get_CorrectionStatus(SSettingsStatus* Status);
void reset_SettingWarning();
uint8_t isSettingsWarning();

bool checkAndFixSetpointSettings(void);
#endif // SETTINGS_H
