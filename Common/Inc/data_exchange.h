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
/* 2nd nibble target sensor ID (if sensor command is active) */
/* 3 + 4th nibble control channel */
#define EXT_INTERFACE_33V_ON		(0x8000u)	/* Bit set to enable 3.3V power interface */
#define EXT_INTERFACE_ADC_ON		(0x4000u)	/* Bit set to enable ADC conversion */
#define EXT_INTERFACE_BUZZER_ON		(0x2000u)	/* Bit set to enable the buzzer */

/* Command subset */
#define EXT_INTERFACE_AUTODETECT 	(0x0001u)	/* Start auto detection of connected sensors	*/
#define EXT_INTERFACE_COPY_SENSORMAP (0x0002u)	/* Use the sensor map provided by master for internal operations */
#define EXT_INTERFACE_CO2_CALIB 	(0x0010u)	/* Request calibration of CO2Sensor */
#define EXT_INTERFACE_O2_INDICATE	(0x0020u)	/* Request LED to blink */
#define EXT_INTERFACE_HUD_UPDATE	(0x0030u)	/* Update status sequence */
#define EXT_INTERFACE_HUD_ABORT		(0x0031u)	/* Abort status sequence */

#define DATA_BUFFER_ADC				(0x01u)
#define DATA_BUFFER_CO2				(0x02u)

#define EXTIF_SENSOR_INFO_SIZE		(32u)		/* size of data array reserved for extended sensor data from external interface */

#define CO2_WARNING_LEVEL_PPM		(2000u)	    /* Early warning to indicate unexpected high co2 concentration (yellow) */
#define CO2_ALARM_LEVEL_PPM			(5000u)		/* starting by this level CO2 has a negative impact on health (long exposure) */

#define GNSS_ALIVE_STATE_ALIVE		(0x01u)		/* Communication to module active: a valid frame was received from the module within the last GNSS_ALIVE_TIMEOUT_MS (set by the RTE). Steady state, it does not toggle */
#define GNSS_ALIVE_STATE_TIME		(0x02u)		/* Time information valid */
#define GNSS_ALIVE_POS_VALID		(0x04u)		/* coord contains a valid position: fix of the receiver or last known position (not older than GNSS_POS_MAX_AGE_S), see accuracy for age and quality */
#define GNSS_ALIVE_FIX_OK			(0x08u)		/* gnssFixOK flag of the receiver: the fix is within the DOP and accuracy masks of the receiver. Only set while the module is alive */
#define GNSS_ALIVE_EPH_SHIFT		(4u)
#define GNSS_ALIVE_EPH_MASK			(0xF0u)		/* number (0..15) of GPS, Galileo, BeiDou and QZSS satellites with ephemeris available in the last UBX-NAV-SAT frame. Only valid while the module is alive. GLONASS and SBAS are not counted, see GNSS_ParseNavSatData */
#define GNSS_EPH_COUNT(pGnssInfo)	(((pGnssInfo)->alive & GNSS_ALIVE_EPH_MASK) >> GNSS_ALIVE_EPH_SHIFT)
#define GNSS_HAS_FIX(pGnssInfo)		(((pGnssInfo)->fixType >= 2) && (((pGnssInfo)->alive & GNSS_ALIVE_POS_VALID) != 0))	/* the receiver has a fix with a valid position at the moment (and not just a last known position) */

#define PRESSURE_BOTTLE_CNT			(0x2u)		/* Number of bottle information (max 255 bar) */

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
		uint8_t gnssDebug_A[2];									/* part of the GNSS debug record (was a 16 bit spare), see GNSS_DebugRecord_Put */
		float	extADC_voltage[3];
		uint16_t CO2_ppm;
		uint16_t CO2_signalStrength;
		uint16_t externalInterface_CmdAnswer;
		uint8_t	gnssDebug_B;											/* part of the GNSS debug record (was an alignment dummy) */
		uint8_t externalInterface_SensorID;						/* Used to identify how to read the sensor data array */
		uint8_t sensor_data[EXTIF_SENSOR_INFO_SIZE];			/* sensor specific data array. Content may vary from sensor type to sensor type */
		uint8_t sensor_map[EXT_INTERFACE_SENSOR_CNT];
		uint8_t pressure_bottle[PRESSURE_BOTTLE_CNT];
		uint8_t gnssDebug_C[3]; 								/* part of the GNSS debug record (was SPARE_OldWireless) */
		// PIC data
		uint8_t button_setting[4]; /* see dependency to SlaveData->buttonPICdata */
		uint8_t SPARE1;
		//debug
		uint32_t pressure_uTick;
		uint32_t compass_uTick;
		SGnssInfo gnssInfo;
		uint8_t tempstick[EXT_INTERFACE_TEMPSTICK_MAX];

} 	SExchangeData;

/* The size of the structure defines the size of the SPI frames in both directions, which the main CPU, the RTE and the bootloader have to agree on */
_Static_assert(sizeof(SExchangeData) == 576u, "Size of SExchangeData changed");

/* GNSS debug record.
 * The six unused bytes of SExchangeData (gnssDebug_A/B/C) carry one record of 6 bytes per exchange. The RTE sends the records in turn (about one per 100 ms, a
 * record is refreshed about once a second), the main CPU collects them in SGnssDebugInfo. This avoids a bigger SExchangeData and with it a different SPI frame.
 * Byte 0 is the record index (bits 0..3) and a check value (bits 4..7: XOR of the nibbles of the index and the payload) against records which were read while
 * the RTE was writing them. Bytes 1..5 are the payload:
 * record 0 (summary A): [1] seconds since the last valid frame (255 = none or 255 s and more), [2] pDOP in 0.1 (255 = unknown), [3] horizontal accuracy of the
 *                       current fix in m as in GNSS_EncodeHAcc (255 = none), [4] satellites with C/N0 > 0 (bits 4..7) and satellites used (bits 0..3), both
 *                       limited to 15, [5] best C/N0 in dBHz (0 = none)
 * record 1 (summary B): [1] time flags (bit 0 validDate, 1 validTime, 2 fullyResolved, 3 confirmedAvai, 4 confirmedDate, 5 confirmedTime, all of UBX-NAV-PVT),
 *                       [2] satellites used in the solution (numSV of UBX-NAV-PVT), [3] psmState of UBX-NAV-PVT (0 not active, 1 enabled, 2 acquisition, 3 tracking,
 *                       4 power optimized tracking, 5 inactive; GNSS_PSM_UNKNOWN until a UBX-NAV-PVT was parsed since the receiver was initialized or woken), [4] number of UART errors of the receiver
 *                       port and [5] number of UBX-ACK-NAK, both saturating at 255 and counted since the RTE started
 * records 2..9 (satellite 0..7, sorted by C/N0): [1] bits 0..2 gnssId, bit 3 svUsed, bit 4 ephAvail, bits 5..6 health (UBX-NAV-SAT), [2] svId (0 = no satellite),
 *                       [3] C/N0 in dBHz, [4] qualityInd, [5] elevation in degrees (255 = unknown)
 * record 10 (UBX-MON-HW A): [1..2] noisePerMS (little endian), [3..4] agcCnt (little endian, 0..8191), [5] aStatus (bits 0..3) and aPower (bits 4..7)
 * record 11 (UBX-MON-HW B): [1] flags (bit 0 rtcCalib, bit 1 safeBoot, bits 2..3 jammingState, bit 4 xtalAbsent), [2] jamInd (CW jamming indicator, offset 45),
 *                       [3] seconds since the last UBX-MON-HW (255 = none or 255 s and more), [4] payload length of the last UBX-MON-HW (saturating at 255, 0 = none),
 *                       [5] bit 0: the last UBX-MON-HW has the accepted length (GNSS_MONHW_LEN) and the values of record 10 and the other fields of this record are valid,
 *                       bit 1: the identification text is valid (UBX-MON-VER received), bits 4..7: number of UBX-MON-VER requests (limited to 15)
 * record 12 (identification text chunk): [1] bits 0..6 chunk number, bit 7 text valid (UBX-MON-VER received, otherwise the characters are not valid),
 *                       [2..5] four characters of the text at offset 4 * chunk number. The text has GNSS_IDENT_SIZE bytes of fixed fields (see GNSS_IDENT_*), each is
 *                       filled with its characters and 0 up to the end of the field.
 *                       With the debug view (HAVE_DEBUG_VIEW) GNSS_DEBUG_EXTRA_BLOCKS more items follow the text chunks in the same sequence: bits 0..6 of [1] are
 *                       GNSS_DEBUG_EXTRA_BASE + n (bit 7 is not used) and [2..5] are four bytes of the raw read back of UBX-CFG-ANT and UBX-CFG-ITFM:
 *                       n = 0: CFG-ANT flags (low, high byte) and pins (low, high byte); n = 1: CFG-ITFM config, little endian; n = 2: CFG-ITFM config2, little endian;
 *                       n = 3: state of the poll of CFG-ANT, of CFG-ITFM (bits 0..6, GNSS_CFGPOLL_*: 0 not polled, 1 polling, 2 answer received, 3 NAK, 4 no answer; bit 7
 *                       GNSS_CFGPOLL_HAVE: the values were read since the power up, they are kept while a new request is open), the number of requests in a row which
 *                       were not answered, for CFG-ANT and for CFG-ITFM;
 *                       UBX-MON-HW2 (read only poll of the ADC input): n = 4: ofsI (I1), magI (U1), ofsQ (I1), magQ (U1); n = 5: lowLevCfg, little endian; n = 6: postStatus,
 *                       little endian; n = 7: cfgSource, state of the poll (bits 0..6, GNSS_CFGPOLL_*, bit 7 GNSS_CFGPOLL_HAVE: a reply with the accepted length was received
 *                       since the power up and the values are those of it), number of requests in a row which were not answered, payload length of the last reply;
 *                       UBX-NAV-STATUS (read only poll): n = 8: msss (ms since the start or reset of the receiver), little endian; n = 9: ttff (ms), little endian;
 *                       n = 10: gpsFix, flags, fixStat, flags2; n = 11: state of the poll (as for n = 7), number of requests in a row which were not answered, number of
 *                       resets of the receiver detected since the power up (msss smaller than in the reply before), payload length of the last reply;
 *                       the counts of lost or disturbed polls and the UART errors: n = 12 .. 16 are four bytes each of a field of GNSS_DEBUG_DIAG_SIZE bytes: the 15 counts
 *                       pollLost[poll][cause] (poll 0 UBX-NAV-PVT, 1 UBX-NAV-SAT, 2 UBX-MON-HW, cause GNSS_CAUSE_*, index poll * 5 + cause) since the wake up, then the
 *                       5 UART error counts (GNSS_UARTERR_*: parity, framing, noise, overrun, DMA) since the RTE started, all saturating at 255;
 *                       UBX-MON-IO (read only poll, the values of one port of the receiver): n = 17: rxBytes; n = 18: txBytes; n = 19: parityErrs (U2), framingErrs (U2);
 *                       n = 20: overrunErrs (U2), breakCond (U2), all little endian; n = 21: state of the poll (as for n = 7), number of requests in a row which were
 *                       not answered, port number of the values, payload length of the last reply;
 *                       n = 22: the read back of UBX-CFG-GNSS (the mask is in record 13): state of the last request (bits 0..6, GNSS_CFGPOLL_*, bit 7 GNSS_CFGPOLL_HAVE:
 *                       the mask was read since the power up), number of requests since the power up, payload length of the last reply;
 *                       receive path of the receiver port on the RTE (counted since the RTE started, saturating): n = 23: rxDiscards (U2, unread data discarded because the DMA
 *                       caught up with the reader), rxStarts (U2, DMA chunks started); n = 24: rxMaxGapMs (U2, longest time between two calls of the reader), rxLateGaps (U1,
 *                       gaps longer than 100 ms), rxStartFails (U1, DMA chunks which could not be started); n = 25 and 26: the first four and the second four bytes of the last
 *                       gap between frames (bytes which are not the start of a frame, only gaps of 3 bytes and more); n = 27: length of that gap (U1, 0 none yet), flags (bit 0:
 *                       the first 8 bytes are printable characters or CR, LF, bit 1: the first byte is '$'), number of gaps of printable characters, number of other gaps;
 *                       UBX-CFG-PRT of the receiver port (read only poll with the port number as payload, port GNSS_CFGPRT_PORT of the RTE): n = 28: state of the poll (as for
 *                       n = 7), number of requests in a row which were not answered, portID of the reply, payload length of the last reply; n = 29: baudRate, little endian;
 *                       n = 30: inProtoMask (U2), outProtoMask (U2), little endian; n = 31: mode (X4), little endian
 * record 13 (power): [1] powerSetupValue read back from UBX-CFG-PMS (0 full power, 1 balanced, 2 interval, 3..5 aggressive, 0xFF no predefined setup), [2] mask of the
 *                       GNSS which are enabled in UBX-CFG-GNSS (bit = gnssId), [3] dynModel of UBX-CFG-NAV5, [4] seconds since the last UBX-NAV-PVT with psmState 0
 *                       (255 = none since the wake up or 255 s and more), [5] bit 0: [1] is valid, bit 1: [2] is valid, bit 2: [3] is valid, bit 3: the power up did not get
 *                       an acknowledge after the repetitions, bit 4: the check of the power save state gave up, bit 6: a UBX-NAV-PVT with psmState 0 was seen since the wake up
 * record 14 (polls A): [1] repetitions of the power up since the wake up, [2] full power commands sent by the check of the power save state, [3] UBX-NAV-PVT requests sent,
 *                       [4] answered, [5] UBX-NAV-SAT requests sent (the counts cover the most recent requests of the poll: both counts of a poll are halved when its sent
 *                       count reaches GNSS_POLL_WINDOW of the RTE, 16)
 * record 15 (polls B): [1] UBX-NAV-SAT requests answered, [2] UBX-MON-HW requests sent, [3] answered, [4] and [5] 0
 * The RTE sends a sequence of GNSS_DEBUG_SCHEDULE_LEN records: the records 0..11 and 13..15 once and GNSS_DEBUG_SCHEDULE_TEXT items of the text record (the next item
 * each time: the chunks of the text, with the debug view the extra items after them). At the rate of one record per 100 ms every record is refreshed every
 * GNSS_DEBUG_SCHEDULE_LEN / 10 s and all items of the text record are sent in (GNSS_DEBUG_TEXT_ITEMS / GNSS_DEBUG_SCHEDULE_TEXT) sequences. Without the debug view that is
 * 1.8 s and 22 / 3 * 1.8 s (13 s), with it 3.0 s and 3.6 sequences (11 s for all items, see GNSS_DEBUG_TEXT_ITEMS).
 * The check value includes a constant, so that a frame with all bytes 0 (a RTE which does not send records) is not accepted.
 * The field offsets are those of the u-blox 8 / u-blox M8 receiver description (UBX-13003221), the module of the OSTC5 is not identified until the text has been
 * received. UBX-MON-HW is accepted with the 60 byte payload of the u-blox 8 / u-blox M8 receiver description only (the same layout is documented in the ZED-F9H
 * interface description UBX-19030118 as deprecated message), other lengths are reported as length only. */
#define GNSS_DEBUG_REC_SIZE			(6u)
#define GNSS_DEBUG_SAT_CNT			(8u)
#define GNSS_DEBUG_REC_SUMMARY_A	(0u)
#define GNSS_DEBUG_REC_SUMMARY_B	(1u)
#define GNSS_DEBUG_REC_SAT0			(2u)
#define GNSS_DEBUG_REC_MONHW_A		(GNSS_DEBUG_REC_SAT0 + GNSS_DEBUG_SAT_CNT)
#define GNSS_DEBUG_REC_MONHW_B		(GNSS_DEBUG_REC_MONHW_A + 1u)
#define GNSS_DEBUG_REC_TEXT			(GNSS_DEBUG_REC_MONHW_B + 1u)
#define GNSS_DEBUG_REC_POWER		(GNSS_DEBUG_REC_TEXT + 1u)
#define GNSS_DEBUG_REC_POLL_A		(GNSS_DEBUG_REC_POWER + 1u)
#define GNSS_DEBUG_REC_POLL_B		(GNSS_DEBUG_REC_POLL_A + 1u)
#define GNSS_DEBUG_REC_CNT			(GNSS_DEBUG_REC_POLL_B + 1u)			/* number of record types: the index is a 4 bit value */
#ifdef HAVE_DEBUG_VIEW
#define GNSS_DEBUG_SCHEDULE_TEXT	(15u)								/* items of the text record (chunks of the text, extra items) in a sequence of records */
#else
#define GNSS_DEBUG_SCHEDULE_TEXT	(3u)
#endif
#define GNSS_DEBUG_SCHEDULE_LEN		(GNSS_DEBUG_REC_CNT - 1u + GNSS_DEBUG_SCHEDULE_TEXT)	/* all records but the text once and the text chunks */
#define GNSS_PSM_UNKNOWN			(0xFFu)								/* power save state (psmState) not known yet */
#define GNSS_CFGPOLL_NONE			(0u)								/* state of the read only poll of UBX-CFG-ANT / UBX-CFG-ITFM: not polled since the power up */
#define GNSS_CFGPOLL_POLLING		(1u)								/* request sent, no answer yet */
#define GNSS_CFGPOLL_OK				(2u)								/* answer received */
#define GNSS_CFGPOLL_NAK			(3u)								/* the receiver rejected the request (UBX-ACK-NAK) */
#define GNSS_CFGPOLL_NOANSWER		(4u)								/* no answer within the time out */
#define GNSS_CFGPOLL_HAVE			(0x80u)								/* flag in the transmitted state: a reply was received since the power up, the values are valid */
#define GNSS_DEBUG_EXTRA_BASE		(0x60u)								/* number of the first extra item in the text record */
#ifdef HAVE_DEBUG_VIEW
#define GNSS_DEBUG_EXTRA_BLOCKS		(32u)								/* extra items which are sent with the text: the read back of UBX-CFG-ANT, UBX-CFG-ITFM, UBX-MON-HW2, UBX-NAV-STATUS, the counts of lost polls, UBX-MON-IO, the receive path, UBX-CFG-PRT */
/* items of the extra items (bits of extraMask of SGnssDebugInfo) which belong together: */
#define GNSS_EXTRA_MASK_ANT			(0x000009uL)						/* UBX-CFG-ANT: items 0 and 3 */
#define GNSS_EXTRA_MASK_ITFM		(0x00000EuL)						/* UBX-CFG-ITFM: items 1, 2 and 3 */
#define GNSS_EXTRA_MASK_HW2			(0x0000F0uL)						/* UBX-MON-HW2: items 4 to 7 */
#define GNSS_EXTRA_MASK_STATUS		(0x000F00uL)						/* UBX-NAV-STATUS: items 8 to 11 */
#define GNSS_EXTRA_MASK_DIAG		(0x01F000uL)						/* counts of lost polls and UART errors: items 12 to 16 */
#define GNSS_EXTRA_MASK_IO			(0x3E0000uL)						/* UBX-MON-IO: items 17 to 21 */
#define GNSS_EXTRA_MASK_CFGGNSS		(0x400000uL)						/* read back of UBX-CFG-GNSS: item 22 */
#define GNSS_EXTRA_MASK_RX			(0x0F800000uL)						/* receive path of the receiver port: items 23 to 27 */
#define GNSS_EXTRA_MASK_PRT			(0xF0000000uL)						/* UBX-CFG-PRT: items 28 to 31 */
#define GNSS_DEBUG_DIAG_SIZE		(20u)								/* bytes of the counts of lost polls (3 * 5) and UART errors (5), sent in five items */
#else
#define GNSS_DEBUG_EXTRA_BLOCKS		(0u)
#endif

#define GNSS_MONHW_LEN				(60u)								/* accepted payload length of UBX-MON-HW */
#define GNSS_MONHW2_LEN				(28u)								/* accepted payload length of UBX-MON-HW2 */
#define GNSS_NAVSTATUS_LEN			(16u)								/* accepted payload length of UBX-NAV-STATUS */
#define GNSS_CFGPRT_LEN				(20u)								/* accepted payload length of UBX-CFG-PRT of a UART port */

/* Identification text of the receiver, built from UBX-MON-VER: swVersion, hwVersion and the extension strings FWVER=, PROTVER=, MOD= (prefix removed) and the
 * first extension string with a ';' (the list of supported GNSS, e.g. GPS;GLO;GAL;BDS). Fixed fields: */
#define GNSS_IDENT_SW_OFFSET		(0u)
#define GNSS_IDENT_SW_SIZE			(24u)		/* longer software versions are truncated */
#define GNSS_IDENT_HW_OFFSET		(GNSS_IDENT_SW_OFFSET + GNSS_IDENT_SW_SIZE)
#define GNSS_IDENT_HW_SIZE			(10u)
#define GNSS_IDENT_FW_OFFSET		(GNSS_IDENT_HW_OFFSET + GNSS_IDENT_HW_SIZE)
#define GNSS_IDENT_FW_SIZE			(12u)
#define GNSS_IDENT_PROTO_OFFSET		(GNSS_IDENT_FW_OFFSET + GNSS_IDENT_FW_SIZE)
#define GNSS_IDENT_PROTO_SIZE		(8u)
#define GNSS_IDENT_MOD_OFFSET		(GNSS_IDENT_PROTO_OFFSET + GNSS_IDENT_PROTO_SIZE)
#define GNSS_IDENT_MOD_SIZE			(14u)
#define GNSS_IDENT_GNSS_OFFSET		(GNSS_IDENT_MOD_OFFSET + GNSS_IDENT_MOD_SIZE)
#define GNSS_IDENT_GNSS_SIZE		(20u)
#define GNSS_IDENT_SIZE				(GNSS_IDENT_GNSS_OFFSET + GNSS_IDENT_GNSS_SIZE)
#define GNSS_IDENT_CHUNK_SIZE		(4u)
#define GNSS_IDENT_CHUNKS			(GNSS_IDENT_SIZE / GNSS_IDENT_CHUNK_SIZE)
_Static_assert((GNSS_IDENT_SIZE % GNSS_IDENT_CHUNK_SIZE) == 0u, "The identification text has to consist of complete chunks");
_Static_assert(GNSS_IDENT_CHUNKS < 128u, "The chunk number is a 7 bit value");
#define GNSS_DEBUG_TEXT_ITEMS		(GNSS_IDENT_CHUNKS + GNSS_DEBUG_EXTRA_BLOCKS)		/* items which the text slots of the sequence send in turn */
_Static_assert(GNSS_IDENT_CHUNKS < GNSS_DEBUG_EXTRA_BASE, "The extra items must not collide with the text chunks");
_Static_assert((GNSS_DEBUG_EXTRA_BASE + GNSS_DEBUG_EXTRA_BLOCKS) <= 0x80u, "The item number is a 7 bit value");
_Static_assert(GNSS_DEBUG_EXTRA_BLOCKS <= 32u, "extraMask has 32 bits");

_Static_assert(GNSS_DEBUG_REC_CNT <= 16u, "The record index is a 4 bit value");
_Static_assert(GNSS_IDENT_CHUNKS <= 32u, "The chunks received are collected in a 32 bit mask");

/* sets the check value of a record: the index is already set in the low bits of byte 0 */
static inline void GNSS_DebugRecord_Seal(uint8_t rec[GNSS_DEBUG_REC_SIZE]) {
	uint8_t chk = (rec[0] & 0x0Fu) ^ 0x05u;		/* all bytes 0 is not a valid record */
	for(uint8_t i = 1; i < GNSS_DEBUG_REC_SIZE; i++) {
		chk ^= (rec[i] & 0x0Fu) ^ (rec[i] >> 4);
	}
	rec[0] = (rec[0] & 0x0Fu) | (uint8_t)(chk << 4);
}

/* returns 1 if the check value of the record is correct */
static inline uint8_t GNSS_DebugRecord_IsValid(const uint8_t rec[GNSS_DEBUG_REC_SIZE]) {
	uint8_t copy[GNSS_DEBUG_REC_SIZE];
	for(uint8_t i = 0; i < GNSS_DEBUG_REC_SIZE; i++) {
		copy[i] = rec[i];
	}
	GNSS_DebugRecord_Seal(copy);
	return (copy[0] == rec[0]) && ((rec[0] & 0x0Fu) < GNSS_DEBUG_REC_CNT);
}

static inline void GNSS_DebugRecord_Put(SExchangeData* d, const uint8_t rec[GNSS_DEBUG_REC_SIZE]) {
	d->gnssDebug_A[0] = rec[0];
	d->gnssDebug_A[1] = rec[1];
	d->gnssDebug_B = rec[2];
	d->gnssDebug_C[0] = rec[3];
	d->gnssDebug_C[1] = rec[4];
	d->gnssDebug_C[2] = rec[5];
}

static inline void GNSS_DebugRecord_Get(const SExchangeData* d, uint8_t rec[GNSS_DEBUG_REC_SIZE]) {
	rec[0] = d->gnssDebug_A[0];
	rec[1] = d->gnssDebug_A[1];
	rec[2] = d->gnssDebug_B;
	rec[3] = d->gnssDebug_C[0];
	rec[4] = d->gnssDebug_C[1];
	rec[5] = d->gnssDebug_C[2];
}

/* GNSS debug values collected by the main CPU from the records */
typedef struct {
	uint8_t flags;			/* bits 0..2 gnssId, bit 3 svUsed, bit 4 ephAvail, bits 5..6 health */
	uint8_t svId;			/* 0: no satellite */
	uint8_t cno;			/* dBHz */
	uint8_t quality;		/* qualityInd 0..7 */
	uint8_t elev;			/* degrees, 255 unknown */
} SGnssDebugSat;

typedef struct {
	uint8_t frameAge_s;
	uint8_t pDop_tenth;
	uint8_t hAcc_m;
	uint8_t numTracked;
	uint8_t numUsed;
	uint8_t bestCno;
	uint8_t timeFlags;
	uint8_t numSvUsedPvt;
	uint8_t psmState;
	uint8_t uartErrors;
	uint8_t nakCount;
	uint8_t recordsSeen;	/* bit 0: summary A received, bit 1: summary B, bit 2: at least one satellite record, bit 3: UBX-MON-HW A, bit 4: UBX-MON-HW B, bit 5: power, bit 6: polls A, bit 7: polls B */
	SGnssDebugSat sat[GNSS_DEBUG_SAT_CNT];

	/* UBX-MON-HW, valid if monHwValid */
	uint8_t monHwValid;		/* a UBX-MON-HW with the accepted length was received (otherwise only monHwLength and monHwAge_s are available) */
	uint8_t monHwLength;	/* payload length of the last UBX-MON-HW (255 or more is reported as 255), 0 if none was received */
	uint8_t monHwAge_s;		/* seconds since the last UBX-MON-HW, 255 = never or 255 s and more */
	uint16_t noisePerMs;
	uint16_t agcCnt;		/* 0..8191 */
	uint8_t aStatus;		/* 0 INIT, 1 DONTKNOW, 2 OK, 3 SHORT, 4 OPEN */
	uint8_t aPower;			/* 0 OFF, 1 ON, 2 DONTKNOW */
	uint8_t hwFlags;		/* bit 0 rtcCalib, bit 1 safeBoot, bits 2..3 jammingState, bit 4 xtalAbsent */
	uint8_t jamInd;			/* CW jamming indicator 0..255 */

	/* UBX-MON-VER */
	uint8_t identValid;		/* the text is complete (all chunks received while the RTE reported it as valid) */
	uint8_t monVerRequests;	/* number of UBX-MON-VER requests of the RTE (limited to 15) */
	uint32_t identChunkMask;	/* chunks received */
	char ident[GNSS_IDENT_SIZE + 1];	/* identification text, the last byte is always 0 */

	/* power state of the receiver, see the records 13 to 15 */
	uint8_t cfgPmsSetup;		/* powerSetupValue of UBX-CFG-PMS */
	uint8_t cfgGnssMask;		/* enabled GNSS (bit = gnssId) */
	uint8_t cfgDynModel;
	uint8_t cfgValid;			/* bit 0: cfgPmsSetup, bit 1: cfgGnssMask, bit 2: cfgDynModel are valid */
	uint8_t psmZeroAge_s;		/* seconds since the last UBX-NAV-PVT with psmState 0, 255 none */
	uint8_t powerFlags;			/* bit 3: power up failed, bit 4: the check of the power save state gave up, bit 6: psmState 0 seen since the wake up */
	uint8_t wakeTries;
	uint8_t verifyTries;
	uint8_t pollSent[3];		/* UBX-NAV-PVT, UBX-NAV-SAT, UBX-MON-HW */
	uint8_t pollAnswered[3];

	/* read back of UBX-CFG-ANT and UBX-CFG-ITFM (raw values), see the text record */
	uint8_t cfgAntState;		/* GNSS_CFGPOLL_*: 0 not polled, 1 polling, 2 answer received, 3 NAK, 4 no answer */
	uint8_t cfgAntHave;			/* the values are those of a reply since the power up */
	uint8_t cfgItfmHave;
	uint8_t cfgItfmState;
	uint8_t cfgAntTries;
	uint8_t cfgItfmTries;
	uint16_t cfgAntFlags;
	uint16_t cfgAntPins;
	uint32_t cfgItfmConfig;
	uint32_t cfgItfmConfig2;
	uint32_t extraMask;			/* bit n: extra item n was received */

	/* UBX-MON-HW2 (read only poll), see the text record: valid if hw2Have, hw2Length 28 is the accepted length */
	uint8_t hw2State;			/* GNSS_CFGPOLL_*: 0 not polled, 1 polling, 2 answer received, 3 NAK, 4 no answer */
	uint8_t hw2Have;			/* the values are those of a reply since the power up */
	uint8_t hw2Tries;
	uint8_t hw2Length;
	int8_t ofsI;
	uint8_t magI;
	int8_t ofsQ;
	uint8_t magQ;
	uint8_t cfgSource;			/* 114 ROM, 111 OTP, 112 config pins, 102 flash image */
	uint32_t lowLevCfg;
	uint32_t postStatus;

	/* UBX-NAV-STATUS (read only poll), see the text record: valid if statusHave, statusLength 16 is the accepted length */
	uint8_t statusState;		/* GNSS_CFGPOLL_* */
	uint8_t statusHave;
	uint8_t statusTries;
	uint8_t statusLength;
	uint8_t statusFix;			/* gpsFix */
	uint8_t statusFlags;		/* bit 0 gpsFixOk, 1 diffSoln, 2 wknSet, 3 towSet */
	uint8_t statusFixStat;
	uint8_t statusFlags2;
	uint32_t ttff;				/* ms */
	uint32_t msss;				/* ms since the start or reset of the receiver */
	uint8_t resetCount;			/* resets of the receiver detected since the power up */

	/* lost or disturbed polls since the wake up by poll (UBX-NAV-PVT, UBX-NAV-SAT, UBX-MON-HW) and cause (timeout, checksum, mismatch, resync, NAK), UART errors by type
	 * (parity, framing, noise, overrun, DMA) since the RTE started, see the text record */
	uint8_t pollLost[3][5];
	uint8_t uartErrType[5];

	/* UBX-MON-IO (read only poll) for port ioPort, see the text record: valid if ioHave, a length which is a multiple of 20 is accepted */
	uint8_t ioState;			/* GNSS_CFGPOLL_* */
	uint8_t ioHave;
	uint8_t ioTries;
	uint8_t ioLength;
	uint8_t ioPort;
	uint32_t ioRxBytes;
	uint32_t ioTxBytes;
	uint16_t ioParityErrs;
	uint16_t ioFramingErrs;
	uint16_t ioOverrunErrs;
	uint16_t ioBreakCond;

	/* last request of the read back of UBX-CFG-GNSS (the mask is cfgGnssMask, valid with bit 1 of cfgValid), see the text record */
	uint8_t cfgGnssState;		/* GNSS_CFGPOLL_* */
	uint8_t cfgGnssTries;
	uint8_t cfgGnssLength;

	/* receive path of the receiver port on the RTE, see the text record (valid with GNSS_EXTRA_MASK_RX in extraMask) */
	uint16_t rxDiscards;
	uint16_t rxStarts;
	uint16_t rxMaxGapMs;
	uint8_t rxLateGaps;
	uint8_t rxStartFails;
	uint8_t junkBytes[8];		/* first bytes of the last gap between frames */
	uint8_t junkLen;			/* its length, 0: none yet */
	uint8_t junkFlags;			/* bit 0: printable characters (or CR, LF) only, bit 1: the first byte is '$' */
	uint8_t junkAscii;			/* gaps of printable characters */
	uint8_t junkBinary;			/* other gaps */

	/* UBX-CFG-PRT of the receiver port (read only poll), see the text record: valid if prtHave, prtLength 20 is the accepted length */
	uint8_t prtState;			/* GNSS_CFGPOLL_* */
	uint8_t prtHave;
	uint8_t prtTries;
	uint8_t prtLength;
	uint8_t prtPortId;
	uint32_t prtBaud;
	uint16_t prtInProto;		/* inProtoMask: bit 0 UBX, 1 NMEA, 2 RTCM, 5 RTCM3 */
	uint16_t prtOutProto;		/* outProtoMask: bit 0 UBX, 1 NMEA, 5 RTCM3 */
	uint32_t prtMode;			/* UART mode: bits 6..7 character length, 9..11 parity, 12..13 stop bits */
} SGnssDebugInfo;

typedef struct
{
		uint8_t VPMconservatism;
		SGas actualGas;

		int8_t offsetPressureSensor_mbar;
		int8_t offsetTemperatureSensor_centiDegree;

		uint16_t externalInterface_Cmd;

		uint8_t externalInterface_SensorMap[EXT_INTERFACE_SENSOR_CNT];

		uint8_t externalInterface_HUD_Update[EXT_INTERFACE_HUD_LED_MAX];
		uint8_t externalInterface_HUD_Brightness;
		uint8_t UNUSED0[3];
		float UNUSED1[16-6];//VPM_adjusted_critical_radius_he[16]; => Reuse HUD_Update
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
	uint8_t displayVersion;
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

