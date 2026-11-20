/*
 * GNSS.c
 *
 *  Created on: 03.10.2020
 *      Author: SimpleMethod
 *
 *Copyright 2020 SimpleMethod
 *
 *Permission is hereby granted, free of charge, to any person obtaining a copy of
 *this software and associated documentation files (the "Software"), to deal in
 *the Software without restriction, including without limitation the rights to
 *use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *of the Software, and to permit persons to whom the Software is furnished to do
 *so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *THE SOFTWARE.
 ******************************************************************************
 */

#include <string.h>
#include "GNSS.h"
#include "data_exchange.h"
#include "rtc.h"

union u_Short uShort;
union i_Short iShort;
union u_Long uLong;
union i_Long iLong;

/*!
 * Structure initialization.
 * @param GNSS Pointer to main GNSS structure.
 * @param huart Pointer to uart handle.
 */
void GNSS_Init(GNSS_StateHandle *GNSS, UART_HandleTypeDef *huart) {
	GNSS->huart = huart;
	GNSS->year = 0;
	GNSS->month = 0;
	GNSS->day = 0;
	GNSS->hour = 0;
	GNSS->min = 0;
	GNSS->sec = 0;
	GNSS->fixType = 0;
	GNSS->lon = 0;
	GNSS->lat = 0;
	GNSS->height = 0;
	GNSS->hMSL = 0;
	GNSS->hAcc = 0;
	GNSS->vAcc = 0;
	GNSS->gSpeed = 0;
	GNSS->headMot = 0;
	GNSS->lastFrameValid = 0;		/* the tick of an earlier frame is meaningless after a restart or a sleep phase */
	GNSS->numEphSat = 0;
	GNSS->fixOk = 0;
	GNSS->numTracked = 0;
	GNSS->bestCno = 0;
	GNSS->psmState = 0;
	GNSS->pvtParsed = 0;
	GNSS->verifyBad = 0;
	GNSS->verifyResend = 0;
	GNSS->verifyTries = 0;
	GNSS->verifyGaveUp = 0;
	GNSS->psmZeroValid = 0;
}

/*!
 * Remember that the receiver answered. Used to derive the alive indication, see GNSS_ALIVE_TIMEOUT_MS.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_NoteFrameReceived(GNSS_StateHandle *GNSS) {
	GNSS->lastFrameTick = HAL_GetTick();
	GNSS->lastFrameValid = 1;
}

static uint8_t GNSS_Limit(uint8_t value, uint8_t limit) {
	return (value > limit) ? limit : value;
}

/*!
 * Fill one record of the GNSS debug record sequence, see data_exchange.h.
 * @param GNSS Pointer to main GNSS structure.
 * @param index Record index 0 .. GNSS_DEBUG_REC_CNT - 1, a higher index is treated like 0.
 * @param rec The record.
 */
void GNSS_GetDebugRecord(const GNSS_StateHandle *GNSS, uint8_t index, uint8_t rec[GNSS_DEBUG_REC_SIZE]) {
	memset(rec, 0, GNSS_DEBUG_REC_SIZE);
	if(index >= GNSS_DEBUG_REC_CNT)
	{
		index = 0;
	}
	rec[0] = index;
	switch(index)
	{
		case GNSS_DEBUG_REC_SUMMARY_A:
			rec[4] = (GNSS_Limit(GNSS->numTracked, 15) << 4);
			rec[5] = GNSS->bestCno;
			break;
		case GNSS_DEBUG_REC_SUMMARY_B:
			rec[3] = GNSS->pvtParsed ? GNSS->psmState : GNSS_PSM_UNKNOWN;
			break;
		default:
			break;
	}
	GNSS_DebugRecord_Seal(rec);
}

/*!
 * Parse data to unique chip ID standard.
 * Look at: 32.19.1.1 u-blox 8 Receiver description
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseUniqID(GNSS_StateHandle *GNSS) {
	for (int var = 0; var < 4; var++) {
		GNSS->uniqueID[var] = GNSS_Handle.uartWorkingBuffer[10 + var];
	}
}

/*!
 * Parse data to navigation position velocity time solution standard.
 * Look at: 32.17.15.1 u-blox 8 Receiver description.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParsePVTData(GNSS_StateHandle *GNSS) {

	static float searchCnt = 1.0;

	RTC_TimeTypeDef sTimeNow;

	uShort.bytes[0] = GNSS_Handle.uartWorkingBuffer[10];
	GNSS->yearBytes[0]=GNSS_Handle.uartWorkingBuffer[10];
	uShort.bytes[1] = GNSS_Handle.uartWorkingBuffer[11];
	GNSS->yearBytes[1]=GNSS_Handle.uartWorkingBuffer[11];
	GNSS->year = uShort.uShort;
	GNSS->month = GNSS_Handle.uartWorkingBuffer[12];
	GNSS->day = GNSS_Handle.uartWorkingBuffer[13];
	GNSS->hour = GNSS_Handle.uartWorkingBuffer[14];
	GNSS->min = GNSS_Handle.uartWorkingBuffer[15];
	GNSS->sec = GNSS_Handle.uartWorkingBuffer[16];
	GNSS->fixType = GNSS_Handle.uartWorkingBuffer[26];
	GNSS->fixOk = (GNSS_Handle.uartWorkingBuffer[27] & 0x01u) ? 1 : 0;	/* flags: gnssFixOK (bit 0) */
	GNSS->psmState = (GNSS_Handle.uartWorkingBuffer[27] >> 2) & 0x07u;	/* flags bits 2..4 */
	GNSS->pvtParsed = 1;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 30];
		GNSS->lonBytes[var]= GNSS_Handle.uartWorkingBuffer[var + 30];
	}
	GNSS->lon = iLong.iLong;
	GNSS->fLon=(float)iLong.iLong/10000000.0;
	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 34];
		GNSS->latBytes[var]=GNSS_Handle.uartWorkingBuffer[var + 34];
	}
	GNSS->lat = iLong.iLong;
	GNSS->fLat=(float)iLong.iLong/10000000.0;
	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 38];
	}
	GNSS->height = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 42];
		GNSS->hMSLBytes[var] = GNSS_Handle.uartWorkingBuffer[var + 42];
	}
	GNSS->hMSL = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 46];
	}
	GNSS->hAcc = uLong.uLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 50];
	}
	GNSS->vAcc = uLong.uLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 66];
		GNSS->gSpeedBytes[var] = GNSS_Handle.uartWorkingBuffer[var + 66];
	}
	GNSS->gSpeed = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 70];
	}
	GNSS->headMot = iLong.iLong * 1e-5; // todo I'm not sure this good options.

	if((GNSS->fLat == 0.0) && (GNSS->fLon == 0.0))
	{
		GNSS->fLat = searchCnt++;
	}

	if((GNSS_Handle.uartWorkingBuffer[17] & 0x03) == 0x03)	/* date/time valid */
	{
		GNSS->alive |= GNSS_ALIVE_STATE_TIME;
	}
	else
	{
		GNSS->alive &= ~GNSS_ALIVE_STATE_TIME;
	}

	/* The receiver has a fix with a position if the fix type is 2D, 3D or GNSS + dead reckoning (not "no fix", "dead reckoning only" or "time only")
	 * and it reports an accuracy estimate for it (the accuracy is not available if the message was too short). */
	if(((GNSS->fixType >= 2) && (GNSS->fixType <= 4)) && (GNSS->hAcc != 0) && (GNSS->hAcc != 0xFFFFFFFFu))
	{
		RTC_GetTime(&sTimeNow);
		GNSS->lastPosValid = 1;
		GNSS->last_fLat = GNSS->fLat;
		GNSS->last_fLon = GNSS->fLon;
		GNSS->last_hAcc = GNSS_EncodeHAcc(GNSS->hAcc);
		GNSS->last_numSatUsed = GNSS_EncodeNumSat(GNSS_Handle.uartWorkingBuffer[29]);	/* numSV: satellites used in the solution */
		GNSS->last_fixTime_s = ((uint32_t)sTimeNow.Hours * 3600u) + ((uint32_t)sTimeNow.Minutes * 60u) + sTimeNow.Seconds;
	}
}

/*!
 * Parse data to UTC time solution standard.
 * Look at: 32.17.30.1 u-blox 8 Receiver description.
 * @param GNSS Pointer to main GNSS structure.
 */
/* The broadcast ephemeris of GPS, Galileo, BeiDou and QZSS satellites is valid for hours (for GPS usually 4 hours, see the u-blox 8 / M8
 * receiver description, chapter "Forcing a Receiver Reset"). A GLONASS ephemeris only covers about 30 minutes (same document, chapter
 * "AssistNow Autonomous") and SBAS satellites have none, so they are not counted. gnssId values: see the appendix "Satellite Numbering" */
static uint8_t GNSS_hasLongLivedEphemeris(uint8_t gnssId) {
	return ((gnssId == 0) || (gnssId == 2) || (gnssId == 3) || (gnssId == 5));	/* GPS, Galileo, BeiDou, QZSS */
}

void GNSS_ParseNavSatData(GNSS_StateHandle *GNSS) {

	uint8_t loop = 0;
	uint8_t searchIndex = 0;
	uint8_t statIndex = 0;	/* only 4 state information will be forwarded */
	uint8_t signalQuality = 0;
	uint16_t payloadLength = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);
	uint8_t scanSat;	/* number of satellite blocks which are really part of the message */

	GNSS->numSat = GNSS_Handle.uartWorkingBuffer[11];
	scanSat = GNSS->numSat;
	if(payloadLength < 8)
	{
		scanSat = 0;
	}
	else if(scanSat > (payloadLength - 8) / 12)
	{
		scanSat = (payloadLength - 8) / 12;
	}

	/* count the satellites with ephemeris. Block N starts at byte 14 + 12 * N of the frame: gnssId, svId, cno, elev, azim (2), prRes (2), flags (4).
	 * flags bit 11 (bit 3 of the second flags byte) is ephAvail. The same loop counts the satellites with C/N0 > 0 and finds the best C/N0 */
	GNSS->numEphSat = 0;
	GNSS->numTracked = 0;
	GNSS->bestCno = 0;
	for(loop = 0; loop < scanSat; loop++)
	{
		const uint8_t* pSv = &GNSS_Handle.uartWorkingBuffer[14 + loop * 12];

		if(GNSS_hasLongLivedEphemeris(pSv[0]) && (pSv[9] & 0x08u))
		{
			GNSS->numEphSat++;
		}
		if(pSv[2] > 0)
		{
			GNSS->numTracked++;
		}
		if(pSv[2] > GNSS->bestCno)
		{
			GNSS->bestCno = pSv[2];
		}
	}
	loop = 0;

	memset(GNSS->statSat, 0, sizeof(GNSS->statSat));

	if(GNSS->numSat > 0)
	{
		searchIndex = 0;
		while((searchIndex < scanSat) && (statIndex < 4))	/* get good signal quality */
		{
			signalQuality = (GNSS_Handle.uartWorkingBuffer[22 + searchIndex * 12] & 0x7);
			if(signalQuality > 4)
			{
				GNSS->statSat[statIndex++] = signalQuality;
			}
			if(statIndex == 4) break;
			searchIndex++;
		}
		searchIndex = 0;
		while((searchIndex < scanSat) && (statIndex < 4))	/* get medium signal quality */
		{
			signalQuality = (GNSS_Handle.uartWorkingBuffer[22 + searchIndex * 12] & 0x7);
			if((signalQuality > 2) && (signalQuality <= 4))
			{
				GNSS->statSat[statIndex++] = signalQuality;
			}
			if(statIndex == 4) break;
			searchIndex++;
		}
		searchIndex = 0;
		while((searchIndex < scanSat) && (statIndex < 4))	/* get poor signal quality */
		{
			signalQuality = (GNSS_Handle.uartWorkingBuffer[22 + searchIndex * 12] & 0x7);
			if(signalQuality <= 2)
			{
				GNSS->statSat[statIndex++] = signalQuality;
			}
			if(statIndex == 4) break;
			searchIndex++;
		}
		loop++;
	}
}

void GNSS_ParseNavigatorData(GNSS_StateHandle *GNSS) {
	uShort.bytes[0] = GNSS_Handle.uartWorkingBuffer[18];
	uShort.bytes[1] = GNSS_Handle.uartWorkingBuffer[19];
	GNSS->year = uShort.uShort;
	GNSS->month = GNSS_Handle.uartWorkingBuffer[20];
	GNSS->day = GNSS_Handle.uartWorkingBuffer[21];
	GNSS->hour = GNSS_Handle.uartWorkingBuffer[22];
	GNSS->min = GNSS_Handle.uartWorkingBuffer[23];
	GNSS->sec = GNSS_Handle.uartWorkingBuffer[24];
}


/*!
 * Parse data to geodetic position solution standard.
 * Look at: 32.17.14.1 u-blox 8 Receiver description.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParsePOSLLHData(GNSS_StateHandle *GNSS) {
	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 10];
	}
	GNSS->lon = iLong.iLong;
	GNSS->fLon=(float)iLong.iLong/10000000.0;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 14];
	}
	GNSS->lat = iLong.iLong;
	GNSS->fLat=(float)iLong.iLong/10000000.0;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 18];
	}
	GNSS->height = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		iLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 22];
	}
	GNSS->hMSL = iLong.iLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 26];
	}
	GNSS->hAcc = uLong.uLong;

	for (int var = 0; var < 4; ++var) {
		uLong.bytes[var] = GNSS_Handle.uartWorkingBuffer[var + 30];
	}
	GNSS->vAcc = uLong.uLong;
}
