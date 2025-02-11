///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/logbook.h
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

#ifndef LOGBOOK_H
#define LOGBOOK_H

#include "data_central.h"
#include "settings.h"

typedef struct
{
    uint8_t setpoint_cbar;
    uint8_t depth_meter;
} SSetpointLog;

typedef struct
{
    uint8_t oxygen_percentage;
    uint8_t helium_percentage;
    uint8_t depth_meter;
    gasbit8_Type note;
} SGasListLog;

//Logbook
typedef struct
{
    uint16_t diveHeaderStart;
    uint8_t  pBeginProfileData[3];
    uint8_t  pEndProfileData[3];
    uint8_t  profileLength[3];
    uint8_t  logbookProfileVersion;
    uint8_t  dateYear;
    uint8_t  dateMonth;
    uint8_t  dateDay;
    uint8_t  timeHour;
    uint8_t  timeMinute;
    uint8_t  extraPagesWithData; /* from here on: changes in order with respect to OSTC3 */
    uint16_t maxDepth;
    uint16_t diveTimeMinutes;
    uint8_t  diveTimeSeconds;
    uint8_t  samplingRate;
    int16_t  minTemp;
    uint16_t surfacePressure_mbar;
    uint16_t desaturationTime;
    SGasListLog gasordil[NUM_GASES];
    uint8_t  firmwareVersionLow;
    uint8_t  firmwareVersionHigh;
    uint16_t batteryVoltage;
    uint16_t cnsAtBeginning;
    uint8_t  gfAtBeginning;
    uint8_t  gfAtEnd;
    uint16_t personalDiveCount;
    SSetpointLog setpoint[NUM_GASES];
    uint16_t maxCNS;
    uint16_t averageDepth_mbar;
    uint16_t total_diveTime_seconds;
    uint8_t  salinity;
    uint8_t  gfLow_or_Vpm_conservatism;
    uint8_t  gfHigh;
    uint8_t  decoModel;
    float    n2Compartments[16];
    float    heCompartments[16];
    uint8_t  n2CompartDesatTime_min[16];
    uint8_t  heCompartDesatTime_min[16];
    uint16_t diveNumber;
    uint8_t  lastDecostop_m;
    uint8_t  CCRmode;
    uint8_t  diveMode;
    uint8_t  hwHudLastStatus; /* from here on identical to OSTC3 again */
    uint16_t hwHudBattery_mV;
    uint8_t batteryGaugeRegisters[5];	/* former batteryGaugeRegisters (6 Bytes) which were not used => use as reserve to keep memory layout */
    uint8_t batteryCharge;				/* first reuse byte */
    uint16_t diveHeaderEnd;
} SLogbookHeader;

//Logbook OSTC3
typedef struct
{
    uint8_t diveHeaderStart[2];
    uint8_t pBeginProfileData[3];
    uint8_t pEndProfileData[3];
    uint8_t logbookProfileVersion;
    uint8_t profileLength[3];
    uint8_t dateYear;
    uint8_t dateMonth;
    uint8_t dateDay;
    uint8_t timeHour;
    uint8_t timeMinute;
    uint8_t maxDepth[2];
    uint8_t diveTimeMinutes[2];
    uint8_t diveTimeSeconds;
    uint8_t minTemp[2];
    uint8_t surfacePressure_mbar[2];
    uint8_t desaturationTime[2];
    uint8_t gasordil[NUM_GASES*4];
    uint8_t firmwareVersionLow;
    uint8_t firmwareVersionHigh;
    uint8_t batteryVoltage[2];
    uint8_t samplingRate;
    uint8_t cnsAtBeginning[2];
    uint8_t gfAtBeginning;
    uint8_t gfAtEnd;
    uint8_t personalDiveCount[2];
    uint8_t batteryCharge;
    uint8_t setpoint[5*2];
    uint8_t salinity;
    uint8_t maxCNS[2];
    uint8_t averageDepth_mbar[2];
    uint8_t total_diveTime_seconds[2];
    uint8_t gfLow_or_Vpm_conservatism;
    uint8_t gfHigh;
    uint8_t decoModel;
    uint8_t diveNumber[2];
    uint8_t diveMode;
    uint8_t n2CompartDesatTime_min[16];
    uint8_t n2Compartments[16*4];
    uint8_t heCompartDesatTime_min[16];
    uint8_t heCompartments[16*4];
    uint8_t lastDecostop_m;
    uint8_t safetyDistance_10cm;
    uint8_t hwHudBattery_mV[2];
    uint8_t hwHudLastStatus;
    uint8_t batteryGaugeRegisters[6];
    uint8_t diveHeaderEnd[2];
} SLogbookHeaderOSTC3;


//Logbook OSTC3
typedef struct
{
    uint8_t profileLength[3];
    uint8_t dateYear;
    uint8_t dateMonth;
    uint8_t dateDay;
    uint8_t timeHour;
    uint8_t timeMinute;
    uint8_t maxDepth[2];
    uint8_t diveTimeMinutes[2];
    uint8_t diveTimeSeconds;
    uint8_t totalDiveNumberLow;
    uint8_t totalDiveNumberHigh;
    uint8_t profileVersion;
} SLogbookHeaderOSTC3compact;


typedef struct
{
    uint8_t profileLength[3];
    uint8_t samplingRate_seconds;
    uint8_t numDivisors;
    uint8_t tempType;
    uint8_t tempLength;
    uint8_t tempDivisor;
    uint8_t deco_ndlType;
    uint8_t deco_ndlLength;
    uint8_t deco_ndlDivisor;
    uint8_t gfType;
    uint8_t gfLength;
    uint8_t gfDivisor;
    uint8_t ppo2Type;
    uint8_t ppo2Length;
    uint8_t ppo2Divisor;
    uint8_t decoplanType;
    uint8_t decoplanLength;
    uint8_t decoplanDivisor;
    uint8_t cnsType;
    uint8_t cnsLength;
    uint8_t cnsDivisor;
    uint8_t tankType;
    uint8_t tankLength;
    uint8_t tankDivisor;
} SSmallHeader;

typedef struct
{
    int8_t percentageO2;
    int8_t percentageHe;
} SManualGas;

void logbook_writeSample(const SDiveState *state);
void logbook_initNewdiveProfile(const SDiveState* pInfo, SSettings* pSettings);
void logbook_EndDive(void);

SLogbookHeader* logbook_getCurrentHeader(void);
SLogbookHeaderOSTC3 * logbook_build_ostc3header(SLogbookHeader* pLogbookHeader);
SLogbookHeaderOSTC3compact * logbook_build_ostc3header_compact(SLogbookHeader* pHead);

uint8_t logbook_getNumberOfHeaders(void);
uint8_t logbook_getHeader(uint8_t StepBackwards,SLogbookHeader* pLogbookHeader);
uint16_t logbook_readSampleData(uint8_t StepBackwards, uint16_t length,uint16_t* depth, uint8_t*  gasid, int16_t* temperature, uint16_t* ppo2,
							    uint16_t* setpoint, uint16_t* sensor1, uint16_t* sensor2, uint16_t* sensor3, uint16_t* cns, uint8_t* bailout,
								uint16_t* decostopDepth, uint16_t* tank, SGnssCoord* pPosition, uint8_t* event);
void logbook_test(void);
void logbook_InitAndWrite(SDiveState* pStateReal);
void logbook_recover_brokenlog(uint8_t headerId);

uint16_t logbook_lastDive_diveNumber(void);
uint16_t logbook_fillDummySampleBuffer(SLogbookHeader* pHeader);
void logbook_readDummySamples(uint8_t* pTarget, uint16_t length);

#endif /* LOGBOOK_H */
