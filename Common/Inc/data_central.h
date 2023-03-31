///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Common/Inc/data_central.h
/// \brief	Common Dadatypes Declarations
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

#ifndef DATA_CENTRAL_H
#define DATA_CENTRAL_H

#include <stdint.h>

#include "settings.h"
#include "stm32f4xx_hal.h"
#include "configuration.h"

#define BUEHLMANN_STRUCT_MAX_GASES 11
#define BUEHLMANN_STRUCT_MAX_ASCENDRATES 3
#define DECOINFO_STRUCT_MAX_STOPS 50

#define false  0
#define true 1

#define EXT_INTERFACE_SENSOR_CNT	(5u)		/* 5 sensors may be connected to the external interface */


/* Helper structs ------------------------------------------------------------*/

//struct SGas
//contains gasinfos of single gas for deco calculation
typedef struct
{
	uint8_t nitrogen_percentage;
	uint8_t helium_percentage;
	uint8_t setPoint_cbar;
	uint8_t change_during_ascent_depth_meter_otherwise_zero;
	uint8_t GasIdInSettings;
	uint8_t AppliedDiveMode;
	float pscr_factor;
} 	SGas;

typedef struct
{
	float use_from_depth_bar;
	float rate_bar_per_minute;
} 	SAscentrate;


typedef struct{
	uint32_t date_rtc_dr;
	uint32_t time_rtc_tr;
	int32_t value_int32;
} SDeviceLine;


typedef struct
{
		uint16_t ageInMilliSeconds;
		uint8_t numberOfBytes;
		uint8_t status;
		uint8_t data[12];
} 	SDataWireless;


typedef struct
{
		uint32_t CO2_ppm;
		uint16_t signalStrength;
} 	SCO2Sensor;

/* Main structs -------------------------------------------------------------*/


//struct SDecoinfo
//contains result of deco calculation
typedef struct
{
	unsigned short output_stop_length_seconds[DECOINFO_STRUCT_MAX_STOPS];
	int output_time_to_surface_seconds;
	int output_ndl_seconds;
	float output_ceiling_meter;
	float super_saturation;
	uint32_t tickstamp;
} 	SDecoinfo;


typedef struct
{
	//crushing pressure
	//in/out
	float  max_crushing_pressure_he[16];
	float  max_crushing_pressure_n2[16];
	//
	float run_time_start_of_deco_zone_save;
	float depth_start_of_deco_zone_save;
	float max_first_stop_depth_save;
	short decomode_vpm_plus_conservatism_last_dive;
	_Bool deco_zone_reached;
	//State variables for repetetive dives
	_Bool repetitive_variables_not_valid;
	float adjusted_crushing_pressure_he[16];
	float adjusted_crushing_pressure_n2[16];
	float adjusted_critical_radius_he[16];
	float adjusted_critical_radius_n2[16];
	float initial_allowable_gradient_he[16];
	float initial_allowable_gradient_n2[16];
	float max_actual_gradient[16];
} 	SVpm;

typedef struct
{
	_Bool repetitive_variables_not_valid;
	_Bool is_data_from_RTE_CPU;
	_Bool spare2;
	_Bool spare3;
	float adjusted_crushing_pressure_he[16];
	float adjusted_crushing_pressure_n2[16];
	float adjusted_critical_radius_he[16];
	float adjusted_critical_radius_n2[16];
	float initial_allowable_gradient_he[16];
	float initial_allowable_gradient_n2[16];
	float max_actual_gradient[16];
}	SVpmRepetitiveData;

//struct SDevice
//contains information about usage
typedef struct
{
	SDeviceLine batteryChargeCycles;
	SDeviceLine batteryChargeCompleteCycles;
	SDeviceLine temperatureMinimum;
	SDeviceLine temperatureMaximum;
	SDeviceLine depthMaximum;
	SDeviceLine diveCycles;
	SDeviceLine voltageMinimum;
	SDeviceLine hoursOfOperation;
	SDeviceLine diveAccident;
}		SDevice;

/*
typedef struct
{
	SDevice device;
	SVpmRepetitiveData vpm;
}		SDeviceState;
*/

/* struct SLifeData
 * contains data all actual data (pressure, stuturation, etc. as received from second ship
 * and has actualGas to be send to Small CPU (second chip)
 * contains data calculated from actual data after receiption from Small CPU
 */
typedef struct
{
	/* from Small CPU */
	uint8_t extIf_sensor_data[32]; /* The external sensor may contain a 64 bit ID. It has been placed at the beginning of the structure to avoid problems in alignment */
	uint8_t extIf_sensor_map[EXT_INTERFACE_SENSOR_CNT];

	int32_t dive_time_seconds;
	int32_t dive_time_seconds_without_surface_time;
	uint32_t surface_time_seconds;
	float pressure_ambient_bar;
	float pressure_surface_bar;
	float tissue_nitrogen_bar[16];
	float tissue_helium_bar[16];
	float cns;
	float otu;
	uint16_t desaturation_time_minutes;
	uint16_t no_fly_time_minutes;
	float temperature_celsius;
	float compass_heading;
	float compass_roll;
	float compass_pitch;
	int16_t compass_DX_f;
	int16_t compass_DY_f;
	int16_t compass_DZ_f;
	uint16_t counterSecondsShallowDepth;
	float ascent_rate_meter_per_min;
	uint32_t timeBinaryFormat;
	uint32_t dateBinaryFormat;
	float battery_voltage;
	float battery_charge;
	uint16_t ambient_light_level;
	SDataWireless wireless_data[4];
	uint8_t buttonPICdata[4];
	SCO2Sensor CO2_data;
	uint8_t extIf_sensor_Id;

	/* by create DiveSettings() and by setActualGas()
	 * is send to Small CPU2 for nitrogen calculation
	 * includes setpoint information
	 */
	SGas actualGas;

	uint8_t lastDiluent_GasIdInSettings;
    float lastSetpointChangeDepthM;

	/* calculated by DataEX_copy_to_LifeData()
			bottle_bar array size is made like this to have multiples of 32bit
	 */
	float			ppO2;
	float			depth_meter;
	float			max_depth_meter;
	float			average_depth_meter;
	float			apnea_total_max_depth_meter;
	float			apnea_last_max_depth_meter;
	int32_t 	apnea_last_dive_time_seconds;
	uint16_t	bottle_bar[2 * NUM_GASES +1];
	uint16_t	bottle_bar_age_MilliSeconds[2 * NUM_GASES + 1];
	uint16_t	apnea_total_counter;

	/* control of DataEX_copy_to_LifeData()
	 */
	uint8_t boolResetAverageDepth;
	uint8_t bool_temp1;
	uint8_t bool_temp2;

/* from local sensor or direct HUD communication */
	 //pp O2 Sensor
	 float ppO2Sensor_bar[3];
	 float sensorVoltage_mV[3];
	 float HUD_battery_voltage_V;

/* for PSCR Mode */
	 float ppo2Simulated_bar;
} 	SLifeData;


typedef struct
{
	uint16_t tissue_nitrogen_desaturation_time_minutes[16];
	uint16_t tissue_helium_desaturation_time_minutes[16];
} 	SLifeData2;


typedef struct
{
	//warnings
	int8_t decoMissed;
	int8_t aGf;
	int8_t ascentRateHigh;
	int8_t ppO2Low;
	int8_t ppO2High;
	int8_t cnsHigh;
	int8_t slowWarning;
	int8_t lowBattery;
	int8_t numWarnings;
	int8_t sensorLinkLost;
	int8_t sensorOutOfBounds[3];
	int8_t betterGas;
	int8_t fallback;
	int8_t betterSetpoint;
	int8_t co2High;
#ifdef ENABLE_BOTTLE_SENSOR
	int8_t newPressure;
#endif
} SWarnings;


typedef struct
{
	//Events logbook only
	int16_t manualMarker;
	int16_t gasChange;
	int16_t info_GasChange;
	int16_t setpointChange;
	int16_t info_SetpointChange;
	int16_t manualGasSet;
	int16_t info_manualGasSetHe;
	int16_t info_manualGasSetO2;
	int16_t bailout;
	int16_t info_bailoutHe;
	int16_t info_bailoutO2;
} 	SEvents;



//struct SDiveSettings
//contains settings necessary for deco calculation
typedef struct
{
	float last_stop_depth_bar;
	float input_next_stop_increment_depth_bar;
	float input_second_to_last_stop_depth_bar;
	float ascentRate_meterperminute;
	uint8_t diveMode; /* OC, CC, .. */
	uint8_t CCR_Mode;
	uint8_t future_TTS_minutes;

	/* If beginning of dive is CCR than ccrOption is set true
	 * true allows returning from bailout (OC) back to CCR
	 * true activates CC gas and setpoint pages in dive menu
	 */
	uint8_t ccrOption;
	uint8_t fallbackOption;
	uint8_t ppo2sensors_deactivated;

	split2x4_Type deco_type; /* GF or VPM for standard and alternative seperate */

	/* VPM conservatism, do not change during dive!!!
	 * do not change in between dives otherwise repetitve dive is not possible
	 */
	uint8_t vpm_conservatism;

	/* Bï¿½hlmann GF
	 * and a variable that is used by Buehlmann during the dive
	 * to remember the position of GF low during ascend
	 */
	uint8_t gf_high;
	uint8_t gf_low;

	/* copy of the Settings GasList and SetpintList
	 * that can be modified during the dive
	 * especially gases can be actived and deactivated
	 * gas[0] and setpoint[0] are the special ones configurable during the dive
	 */
	SGasLine gas[1 + (2*NUM_GASES)];
	SSetpointLine setpoint[1 + NUM_GASES];

	/* List of GasChanges in the actual order
	 * this is necessary for VPM and Buehlmann and will be created on start of each calculation
	 */
	SGas decogaslist[BUEHLMANN_STRUCT_MAX_GASES]; // unused filled with 0xFF

	/*
	 */
	float internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero;
	uint16_t compassHeading;

	uint8_t pscr_o2_drop;
	uint8_t pscr_lung_ratio;
 }  SDiveSettings;

enum CHARGE_STATUS{
		CHARGER_off = 0,
		CHARGER_running,
		CHARGER_complete,
		CHARGER_lostConnection
};

typedef struct
{
	SDiveSettings diveSettings;
	SLifeData lifeData;
	SVpm vpm;
	SEvents events;
	SWarnings warnings;
	SDecoinfo decolistVPM;
	SDecoinfo decolistFutureVPM;
	SDecoinfo decolistBuehlmann; //
	SDecoinfo decolistFutureBuehlmann;
	uint8_t mode; /* hw for sleep, surface, dive, .. */
	uint8_t chargeStatus;
	uint8_t data_old__lost_connection_to_slave;

	uint32_t pressure_uTick_new;
	uint32_t compass_uTick_new;

	uint32_t pressure_uTick_old;
	uint32_t compass_uTick_old;

	uint32_t pressure_uTick_local_new;
	uint32_t compass_uTick_local_new;

	uint8_t cnsHigh_at_the_end_of_dive;
	uint8_t decoMissed_at_the_end_of_dive;

	uint8_t sensorErrorsRTE;

	uint8_t lastKnownBatteryPercentage;
} 	SDiveState;


typedef struct{
uint8_t bit0:1;
uint8_t bit1:1;
uint8_t bit2:1;
uint8_t bit3:1;
uint8_t bit4:1;
uint8_t bit5:1;
uint8_t bit6:1;
uint8_t bit7:1;
} ubit8_t;

typedef union{
ubit8_t ub;
uint8_t uw;
} bit8_Type;

extern SDiveState stateSim;
extern SDiveState stateDeco;
extern uint8_t decoLock;
extern const SDiveState *stateUsed;
extern SDiveState *stateUsedWrite;

enum DECO_LOCK{
		DECO_CALC_running,
		DECO_CALC_ready,
		DECO_CALC_FINSHED_Buehlmann,
		DECO_CALC_FINSHED_vpm,
		DECO_CALC_FINSHED_FutureBuehlmann,
		DECO_CALC_FINSHED_Futurevpm,
		DECO_CALC_init_as_is_start_of_dive,
		DECO_CALC_undefined
};

typedef enum
{
	 SENSOR_NONE,
	 SENSOR_SEARCH,
	 SENSOR_OPTIC,
	 SENSOR_ANALOG,
	 SENSOR_DIGO2,
	 SENSOR_SENTINEL,
	 SENSOR_TYPE_O2_END,
	 SENSOR_CO2,
	 SENSOR_END
} externalInterfaceSensorType;

#define DVO2_FATAL_INTENSITY_LOW	(0x00000001)
#define DVO2_FATAL_AMBIENT_HIGH		(0x00000002)
#define DVO2_FATAL_REFINT_LOW		(0x00000004)
#define DVO2_FATAL_REFAMB_HIGH		(0x00000008)
#define DVO2_FATAL_TEMPERATURE		(0x00000010)

#define DVO2_FATAL_ERROR	(DVO2_FATAL_INTENSITY_LOW | DVO2_FATAL_AMBIENT_HIGH | DVO2_FATAL_REFINT_LOW | DVO2_FATAL_REFAMB_HIGH | DVO2_FATAL_TEMPERATURE)

uint32_t time_elapsed_ms(uint32_t ticksstart,uint32_t ticksnow);

void set_stateUsedToSim(void);
void set_stateUsedToReal(void);

_Bool is_stateUsedSetToSim(void);

const SDiveState * stateRealGetPointer(void);
const SDiveState * stateSimGetPointer(void);
SDiveState * stateRealGetPointerWrite(void);
SDiveState * stateSimGetPointerWrite(void);
const SDevice * stateDeviceGetPointer(void);
SDevice * stateDeviceGetPointerWrite(void);
const SVpmRepetitiveData * stateVpmRepetitiveDataGetPointer(void);
SVpmRepetitiveData * stateVpmRepetitiveDataGetPointerWrite(void);

void UpdateLifeDataTest(SDiveState * diveState);

void setSimulationValues(int _ascent_rate_meter_per_min, int _descent_rate_meter_per_min, int _max_depth, int _bottom_time );

void createDiveSettings(void);
void copyDiveSettingsToSim(void);
void copyVpmRepetetiveDataToSim(void);
//void fallbackToFixedSetpoints(SLifeData *lifeData);
void setActualGasFirst(SLifeData *lifeData);
void setActualGasAir(SLifeData *lifeData);
void setActualGas(SLifeData *lifeData, uint8_t gasId, uint8_t setpoint_cbar);
void setActualGas_ExtraGas(SLifeData *lifeData, uint8_t oxygen, uint8_t helium, uint8_t setpoint_cbar);
void setActualGas_DM(SLifeData *lifeData, uint8_t gasId, uint8_t setpoint_cbar);
void setWeekday(RTC_DateTypeDef *sDate);
void setDate(RTC_DateTypeDef Sdate);
void setTime(RTC_TimeTypeDef Stime);
void setBatteryPercentage(uint8_t newChargePercentage);
void setButtonResponsiveness(uint8_t *ButtonSensitivyList);
void calibrateCompass(void);
void compass_Inertia(float newHeading);
float compass_getCompensated();
void clearDeco(void);
void translateDate(uint32_t datetmpreg, RTC_DateTypeDef *sDate);
void translateTime(uint32_t tmpreg, RTC_TimeTypeDef *sTime);
void updateSetpointStateUsed(void);

uint8_t calc_MOD(uint8_t gasId);
uint8_t calc_MinOD(uint8_t gasId);
//float calc_ppO2(float input_ambient_pressure_bar, SGas* pGas);
int current_second(void);
_Bool vpm_crush(SDiveState* pDiveState);
_Bool deco_zone_reached(void);
void resetEvents(const SDiveState *pStateUsed);

uint32_t crc32c_checksum(uint8_t* message, uint16_t length, uint8_t* message2, uint16_t length2);
uint32_t	CRC_CalcBlockCRC(uint32_t *buffer, uint32_t words);
uint32_t	CRC_CalcBlockCRC_moreThan768000(uint32_t *buffer1, uint32_t *buffer2, uint32_t words);

_Bool is_ambient_pressure_close_to_surface(SLifeData *lifeData);

uint8_t isLoopMode(uint8_t Mode);

#endif // DATA_CENTRAL_H
