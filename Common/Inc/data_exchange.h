///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Common/Inc/data_exchange.h
/// \brief	Data exchange between RTE and Discovery processors.
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

#ifndef DATA_EXCHANGE_H
#define DATA_EXCHANGE_H

#include "data_central.h"
#include "settings.h"
#include "stm32f4xx_hal.h"

/* Command definitions for control of external interface */
/* 1st nibble binary on/off states */
/* 2nd nibble UART protocol selection */
/* 3rd nibble reserve */
/* 4th nibble command channel */
#define EXT_INTERFACE_33V_ON		(0x8000u)	/* Bit set to enable 3.3V power interface */
#define EXT_INTERFACE_ADC_ON		(0x4000u)	/* Bit set to enable ADC conversion */
#define EXT_INTERFACE_BUZZER_ON		(0x2000u)	/* Bit set to enable the buzzer */

/* Command subset */
#define EXT_INTERFACE_AUTODETECT 	(0x0001u)	/* Start auto detection of connected sensors	*/
#define EXT_INTERFACE_COPY_SENSORMAP (0x0002u)	/* Use the sensor map provided by master for internal operations */
#define EXT_INTERFACE_CO2_CALIB 	(0x0010u)	/* Request calibration of CO2Sensor */
#define EXT_INTERFACE_O2_INDICATE	(0x0020u)	/* Request LED to blink*/

#define DATA_BUFFER_ADC				(0x01u)
#define DATA_BUFFER_CO2				(0x02u)

#define EXTIF_SENSOR_INFO_SIZE		(32u)		/* size of data array reserved for extended sensor data from external interface */

#define CO2_WARNING_LEVEL_PPM		(2000u)	    /* Early warning to indicate unexpected high co2 concentration (yellow) */
#define CO2_ALARM_LEVEL_PPM			(5000u)		/* starting by this level CO2 has a negative impact on health (long exposure) */

#define GNSS_ALIVE_STATE_ALIVE		(0x01u)		/* Communication to module active */
#define GNSS_ALIVE_STATE_TIME		(0x02u)		/* Time information valid */
#define GNSS_ALIVE_BACKUP_POS		(0x04u)		/* Backup position not older than x hours */

enum MODE
{
	MODE_SURFACE	= 0,
	MODE_DIVE 		= 1,
	MODE_CALIB 		= 2,
	MODE_SLEEP 		= 3,
	MODE_SHUTDOWN   = 4,
	MODE_ENDDIVE	= 5,
	MODE_BOOT		= 6,
	MODE_CHARGESTART = 7,
	MODE_TEST		= 8,
	MODE_POWERUP 	= 9,
};

enum ACCIDENT_BITS
{
	ACCIDENT_DECOSTOP 	= 0x01,
	ACCIDENT_CNS		= 0x02,
	ACCIDENT_CNSLVL2	= 0x02 + 0x04,
	ACCIDENT_SPARE2		= 0x08,
	ACCIDENT_SPARE3		= 0x10,
	ACCIDENT_SPARE4		= 0x20,
	ACCIDENT_SPARE5		= 0x40,
	ACCIDENT_SPARE6		= 0x80
};


typedef struct{
uint8_t button:1;
uint8_t date:1;
uint8_t time:1;
uint8_t clearDeco:1;
uint8_t compass:1;
uint8_t devicedata:1;
uint8_t batterygauge:1;
uint8_t accident:1;
} confirmbit8_t;

typedef struct{
uint8_t checkCompass:1;
uint8_t checkADC:1;
uint8_t reserve:5;
uint8_t extADC:1;
uint8_t compass:8;
} hw_Info_t;


#define CRBUTTON 			(0x01)
#define CRDATE 				(0x02)
#define CRTIME 				(0x04)
#define CRCLEARDECO			(0x08)
#define CRCOMPASS 			(0x10)
#define CRDEVICEDATA 		(0x20)
#define CRBATTERY			(0x40)
#define CRACCIDENT			(0x80)

typedef union{
confirmbit8_t ub;
uint8_t uw;
} confirmbit8_Type;

typedef struct
{
		uint8_t checkCode[4];

} 	SDataExchangeHeader;

typedef struct
{
		uint8_t checkCode[4];

} 	SDataExchangeFooter;

typedef struct
{
	SDataExchangeHeader header;
	SLifeData lifeData;
} SDataExchangeMasterToSlave;

typedef struct
{
	int32_t temperature;
	uint32_t status;
	uint64_t sensorId;
	int32_t intensity;
	int32_t ambient;
	int32_t pressure;
	int32_t humidity;
} SSensorDataDiveO2;

typedef struct
{
		//pressure
		float temperature;
		float pressure_mbar;
		float surface_mbar;
		float ascent_rate_meter_per_min;
		//toxic
		float otu;
		float cns;
		uint16_t desaturation_time_minutes;
		uint16_t no_fly_time_minutes;
		//tisssue
		float tissue_nitrogen_bar[16];
		float tissue_helium_bar[16];
		//maxcrushingpressure
		float  max_crushing_pressure_he[16];
		float  max_crushing_pressure_n2[16];
		float  adjusted_critical_radius_he[16];
		float  adjusted_critical_radius_n2[16];
		// Compass
		float compass_heading;
		float compass_roll;
		float compass_pitch;
		int16_t compass_DX_f;
		int16_t compass_DY_f;
		int16_t compass_DZ_f;
		//time
		uint16_t counterSecondsShallowDepth;
		uint32_t localtime_rtc_tr;
		uint32_t localtime_rtc_dr;
		uint32_t divetime_seconds;
		uint32_t surfacetime_seconds;
		uint32_t dive_time_seconds_without_surface_time;
		//battery /* take care of uint8_t count to be in multiplies of 4 */
		float battery_voltage;
		float battery_charge;
		//ambient light
		uint16_t ambient_light_level;
		uint16_t SPARE_ALIGN32;
		float	extADC_voltage[3];
		uint16_t CO2_ppm;
		uint16_t CO2_signalStrength;
		uint16_t externalInterface_CmdAnswer;
		uint8_t	alignmentdummy;
		uint8_t externalInterface_SensorID;						/* Used to identify how to read the sensor data array */
		uint8_t sensor_data[EXTIF_SENSOR_INFO_SIZE];			/* sensor specific data array. Content may vary from sensor type to sensor type */
		uint8_t sensor_map[EXT_INTERFACE_SENSOR_CNT];
		uint8_t SPARE_OldWireless[5]; 							/* 64 - 12 for extADC - 6 for CO2 - 34 for sensor (+dummmy) - sensor map*/
		// PIC data
		uint8_t button_setting[4]; /* see dependency to SlaveData->buttonPICdata */
		uint8_t SPARE1;
		//debug
		uint32_t pressure_uTick;
		uint32_t compass_uTick;
		SGnssInfo gnssInfo;

} 	SExchangeData;

typedef struct
{
		uint8_t VPMconservatism;
		SGas actualGas;

		int8_t offsetPressureSensor_mbar;
		int8_t offsetTemperatureSensor_centiDegree;

		uint16_t externalInterface_Cmd;

		uint8_t externalInterface_SensorMap[EXT_INTERFACE_SENSOR_CNT];

		float UNUSED1[16-1];//VPM_adjusted_critical_radius_he[16];
		float UNUSED2[16];//VPM_adjusted_critical_radius_n2[16];
		float UNUSED3[16];//VPM_adjusted_crushing_pressure_he[16];
		float UNUSED4[16];//VPM_adjusted_crushing_pressure_n2[16];
		float UNUSED5[16];//VPM_initial_allowable_gradient_he[16];
		float UNUSED6[16];//VPM_initial_allowable_gradient_n2[16];
		float UNUSED7[16];//VPM_max_actual_gradient[16];

		RTC_TimeTypeDef newTime;
		RTC_DateTypeDef newDate;

		float ambient_pressure_mbar_ceiling;
		float descend_rate_bar_per_minute;
		float ascend_rate_bar_per_minute;

		uint16_t timeoutDiveReachedZeroDepth;
		uint16_t divetimeToCreateLogbook;

		uint8_t buttonResponsiveness[4];

		SDevice DeviceData;

		float newBatteryGaugePercentageFloat;

} 	SReceiveData;


typedef struct
{
	SDataExchangeHeader header;

	uint8_t mode;
	uint8_t power_on_reset;
	uint8_t RTE_VERSION_high;
	uint8_t RTE_VERSION_low;

	uint8_t chargeStatus;
	uint8_t boolPICdata;
	confirmbit8_Type confirmRequest; // confirmbit8_Type
	uint8_t boolADCO2Data;

	uint8_t boolPressureData;
	uint8_t boolCompassData;
	uint8_t boolTisssueData;
	uint8_t boolCrushingData;

	uint8_t boolToxicData;
	uint8_t boolTimeData;
	uint8_t boolBatteryData;
	uint8_t boolAmbientLightData;

	uint8_t accidentFlags;
	uint8_t sensorErrors;
	uint8_t spare2;
	uint8_t spare3;

	SExchangeData data[2];
	SDataExchangeFooter footer;
	uint8_t CRC_feature_by_SPI[4];
} SDataExchangeSlaveToMaster;


typedef struct
{
	SDataExchangeHeader header;

	uint8_t mode;
	uint8_t power_on_reset;
	uint8_t RTE_VERSION_high;
	uint8_t RTE_VERSION_low;

	uint8_t chargeStatus;
	hw_Info_t hw_Info;
	uint8_t spare1;

	uint8_t boolDeviceData;
	uint8_t boolVpmRepetitiveDataValid;
	uint8_t bool3;
	uint8_t bool4;

	uint8_t spare1_1;
	uint8_t spare1_2;
	uint8_t spare1_3;
	uint8_t spare1_4;

	uint8_t spare2_1;
	uint8_t spare2_2;
	uint8_t spare2_3;
	uint8_t spare2_4;

	SDevice DeviceData[2];
	SVpmRepetitiveData VpmRepetitiveData;

	uint8_t arraySizeOfMinimumSExChangeDate[(2 * sizeof(SExchangeData)) - ((2 * sizeof(SDevice)) + sizeof(SVpmRepetitiveData))];
	SDataExchangeFooter footer;
	uint8_t CRC_feature_by_SPI[4];
} SDataExchangeSlaveToMasterDeviceData;


typedef struct
{
	SDataExchangeHeader header;

	uint8_t mode;
	uint8_t getDeviceDataNow;
	uint8_t diveModeInfo;
	uint8_t setEndDive;

	uint8_t bool4;
	uint8_t setButtonSensitivityNow;
	uint8_t setDateNow;
	uint8_t setTimeNow;

	uint8_t calibrateCompassNow;
	uint8_t clearDecoNow;
	uint8_t setBatteryGaugeNow;
	uint8_t bool9;

	uint8_t revisionHardware;
	uint8_t revisionCRCx0x7A;
	uint8_t spare1_3;
	uint8_t spare1_4;

	uint8_t setAccidentFlag;
	uint8_t spare2_1;
	uint8_t spare2_2;
	uint8_t spare2_3;

	SReceiveData data;
	uint8_t arraySizeOfMinimumSExChangeDate[(2 * sizeof(SExchangeData)) - sizeof(SReceiveData)];
	SDataExchangeFooter footer;
	uint8_t CRC_feature_by_SPI[4];
} SDataReceiveFromMaster;


/* Size of Transmission buffer */
#define EXCHANGE_BUFFERSIZE			(sizeof(SDataExchangeSlaveToMaster) - 2)
#define EXCHANGE_BUFFERSIZE2			(sizeof(SDataReceiveFromMaster) - 2)
// header: 	  	5
// mode+bool:   5
// data				552 ( 69 * float/4 * 2 )
// footer:		  4
// ______________
// SUM				566
// CRC_feature does not count into BUFFERSIZE!

//(COUNTOF(struct SDataExchangeSlaveToMaster) + 1)

/* Exported macro ------------------------------------------------------------*/
//#define COUNTOF(__BUFFER__)   (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

#endif /* DATA_EXCHANGE_H */

