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
	GNSS->numUsed = 0;
	GNSS->bestCno = 0;
	memset(GNSS->debugSat, 0, sizeof(GNSS->debugSat));
	GNSS->pDop = 0xFFFFu;
	GNSS->timeFlags = 0;
	GNSS->numSvUsedPvt = 0;
	GNSS->psmState = 0;
	GNSS->pvtParsed = 0;
	/* the error counters are kept: they count since the RTE started */
	GNSS->monHwSeen = 0;
	GNSS->monHwValid = 0;
	GNSS->monHwLength = 0;
	GNSS->monVerRequests = 0;
	GNSS->verifyBad = 0;
	GNSS->verifyResend = 0;
	GNSS->verifyTries = 0;
	GNSS->verifyGaveUp = 0;
	GNSS->psmZeroValid = 0;
	GNSS->monStartup = 2;		/* the first two poll slots after the start are UBX-MON-VER and UBX-MON-HW, the receiver identification is kept: it does not change */
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
 * @param age_s Seconds since the last valid frame.
 * @param rec The record.
 */
void GNSS_GetDebugRecord(const GNSS_StateHandle *GNSS, uint8_t index, uint8_t age_s, uint8_t rec[GNSS_DEBUG_REC_SIZE]) {
	const SGnssDebugSat* pSat;
	uint32_t pDop_tenth;

	memset(rec, 0, GNSS_DEBUG_REC_SIZE);
	if(index >= GNSS_DEBUG_REC_CNT)
	{
		index = 0;
	}
	rec[0] = index;
	switch(index)
	{
		case GNSS_DEBUG_REC_SUMMARY_A:
			pDop_tenth = (GNSS->pDop == 0xFFFFu) ? 255u : (GNSS->pDop / 10u);
			rec[1] = age_s;
			rec[2] = (pDop_tenth > 255u) ? 255u : (uint8_t)pDop_tenth;
			rec[3] = GNSS_EncodeHAcc(GNSS->hAcc);
			rec[4] = (GNSS_Limit(GNSS->numTracked, 15) << 4) | GNSS_Limit(GNSS->numUsed, 15);
			rec[5] = GNSS->bestCno;
			break;
		case GNSS_DEBUG_REC_SUMMARY_B:
			rec[1] = GNSS->timeFlags;
			rec[2] = GNSS->numSvUsedPvt;
			rec[3] = GNSS->pvtParsed ? GNSS->psmState : GNSS_PSM_UNKNOWN;
			rec[4] = GNSS->uartErrors;
			rec[5] = GNSS->nakCount;
			break;
		default:
			pSat = &GNSS->debugSat[index - GNSS_DEBUG_REC_SAT0];
			rec[1] = pSat->flags;
			rec[2] = pSat->svId;
			rec[3] = pSat->cno;
			rec[4] = pSat->quality;
			rec[5] = pSat->elev;
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
	/* values for the debug page: payload offsets of UBX-NAV-PVT (92 bytes) + 6 bytes of the frame header: valid 11, flags 21, flags2 22, numSV 23, pDOP 76 */
	GNSS->timeFlags = (GNSS_Handle.uartWorkingBuffer[17] & 0x07u) | ((GNSS_Handle.uartWorkingBuffer[28] >> 2) & 0x38u);	/* valid: validDate, validTime, fullyResolved; flags2 bits 5..7: confirmedAvai, confirmedDate, confirmedTime */
	GNSS->numSvUsedPvt = GNSS_Handle.uartWorkingBuffer[29];
	GNSS->psmState = (GNSS_Handle.uartWorkingBuffer[27] >> 2) & 0x07u;	/* flags bits 2..4 */
	GNSS->pvtParsed = 1;
	if((GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8)) >= 78)	/* the frame contains the pDOP */
	{
		GNSS->pDop = GNSS_Handle.uartWorkingBuffer[82] | (GNSS_Handle.uartWorkingBuffer[83] << 8);
	}
	else
	{
		GNSS->pDop = 0xFFFFu;
	}

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
	 * flags bit 11 (bit 3 of the second flags byte) is ephAvail. The same loop collects the values of the debug page: C/N0 > 0, svUsed (bit 3), best C/N0
	 * and the satellites with the highest C/N0 (insertion into a short sorted list) */
	GNSS->numEphSat = 0;
	GNSS->numTracked = 0;
	GNSS->numUsed = 0;
	GNSS->bestCno = 0;
	memset(GNSS->debugSat, 0, sizeof(GNSS->debugSat));
	for(loop = 0; loop < scanSat; loop++)
	{
		const uint8_t* pSv = &GNSS_Handle.uartWorkingBuffer[14 + loop * 12];
		SGnssDebugSat entry;
		uint8_t pos;

		if(GNSS_hasLongLivedEphemeris(pSv[0]) && (pSv[9] & 0x08u))
		{
			GNSS->numEphSat++;
		}
		if(pSv[2] > 0)
		{
			GNSS->numTracked++;
		}
		if(pSv[8] & 0x08u)
		{
			GNSS->numUsed++;
		}
		if(pSv[2] > GNSS->bestCno)
		{
			GNSS->bestCno = pSv[2];
		}

		entry.flags = (pSv[0] & 0x07u) | ((pSv[8] & 0x08u) ? 0x08u : 0) | ((pSv[9] & 0x08u) ? 0x10u : 0) | (((pSv[8] >> 4) & 0x03u) << 5);
		entry.svId = pSv[1];
		entry.cno = pSv[2];
		entry.quality = pSv[8] & 0x07u;
		entry.elev = (((int8_t)pSv[3] >= 0) && ((int8_t)pSv[3] <= 90)) ? pSv[3] : 255u;
		pos = 0;
		while((pos < GNSS_DEBUG_SAT_CNT) && (GNSS->debugSat[pos].svId != 0) && (GNSS->debugSat[pos].cno >= entry.cno))
		{
			pos++;
		}
		if(pos < GNSS_DEBUG_SAT_CNT)
		{
			for(uint8_t k = GNSS_DEBUG_SAT_CNT - 1; k > pos; k--)
			{
				GNSS->debugSat[k] = GNSS->debugSat[k - 1];
			}
			GNSS->debugSat[pos] = entry;
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

/*!
 * Parse UBX-MON-HW (class 0x0A, id 0x09) for the debug page. Look at: 32.16.4 u-blox 8 Receiver description. Offsets in the frame: header 6 bytes + payload offset.
 * Only the 60 byte payload of the description is accepted. For another length (other generations of receivers use other layouts) only the length is
 * reported, no value is taken from the frame.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseMonHwData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);

	GNSS->monHwSeen = 1;
	GNSS->monHwTick = HAL_GetTick();
	GNSS->monHwLength = (length > 255u) ? 255u : (uint8_t)length;
	GNSS->monHwValid = 0;
	if(length == GNSS_MONHW_LEN)
	{
		GNSS->noisePerMs = GNSS_Handle.uartWorkingBuffer[6 + 16] | (GNSS_Handle.uartWorkingBuffer[6 + 17] << 8);
		GNSS->agcCnt = GNSS_Handle.uartWorkingBuffer[6 + 18] | (GNSS_Handle.uartWorkingBuffer[6 + 19] << 8);
		GNSS->aStatus = GNSS_Handle.uartWorkingBuffer[6 + 20];
		GNSS->aPower = GNSS_Handle.uartWorkingBuffer[6 + 21];
		GNSS->hwFlags = GNSS_Handle.uartWorkingBuffer[6 + 22] & 0x1Fu;		/* rtcCalib, safeBoot, jammingState, xtalAbsent */
		GNSS->jamInd = GNSS_Handle.uartWorkingBuffer[6 + 45];
		GNSS->monHwValid = 1;
	}
}

/* copies a string without prefix into a field of the identification text: at most size - 1 characters, the rest of the field is 0 */
static void GNSS_CopyIdentField(uint8_t* pField, uint8_t size, const uint8_t* pSrc, uint8_t maxSrc) {
	uint8_t pos = 0;

	memset(pField, 0, size);
	while((pos < (size - 1u)) && (pos < maxSrc) && (pSrc[pos] != 0))
	{
		pField[pos] = ((pSrc[pos] >= 0x20u) && (pSrc[pos] < 0x7Fu)) ? pSrc[pos] : '?';		/* only printable characters */
		pos++;
	}
}

/* returns 1 if the string starts with the prefix */
static uint8_t GNSS_HasPrefix(const uint8_t* pStr, const char* pPrefix) {
	while(*pPrefix != 0)
	{
		if(*pStr != (uint8_t)*pPrefix)
		{
			return 0;
		}
		pStr++;
		pPrefix++;
	}
	return 1;
}

/*!
 * Parse UBX-MON-VER (class 0x0A, id 0x04) for the debug page. Look at: 32.16.13 u-blox 8 Receiver description: payload swVersion[30], hwVersion[10], N times
 * extension[30], all 0 terminated. The text for the main CPU consists of fixed fields (see GNSS_IDENT_*), strings which are longer than their field are truncated
 * and only the extension strings which are shown are taken: FWVER=, PROTVER=, MOD= (prefix removed) and the first one with a ';' (list of the supported GNSS).
 * The length of the frame is limited by the receive buffer (see uartGnss_ProcessData), the number of extension strings by the length of the payload.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseMonVerData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);
	const uint8_t* pPayload = &GNSS_Handle.uartWorkingBuffer[6];
	uint8_t ext = 0;
	uint8_t extCount;
	uint8_t gnssFound = 0;
	uint8_t newIdent[GNSS_IDENT_SIZE];
	const uint8_t* pExt;

	if(length < 40u)		/* swVersion and hwVersion are the minimum */
	{
		return;
	}
	memset(newIdent, 0, sizeof(newIdent));
	GNSS_CopyIdentField(&newIdent[GNSS_IDENT_SW_OFFSET], GNSS_IDENT_SW_SIZE, &pPayload[0], 30);
	GNSS_CopyIdentField(&newIdent[GNSS_IDENT_HW_OFFSET], GNSS_IDENT_HW_SIZE, &pPayload[30], 10);

	extCount = (uint8_t)(((length - 40u) / 30u) > 255u ? 255u : ((length - 40u) / 30u));
	for(ext = 0; ext < extCount; ext++)
	{
		pExt = &pPayload[40 + ext * 30];
		if(GNSS_HasPrefix(pExt, "FWVER="))
		{
			GNSS_CopyIdentField(&newIdent[GNSS_IDENT_FW_OFFSET], GNSS_IDENT_FW_SIZE, pExt + 6, 24);
		}
		else if(GNSS_HasPrefix(pExt, "PROTVER="))
		{
			GNSS_CopyIdentField(&newIdent[GNSS_IDENT_PROTO_OFFSET], GNSS_IDENT_PROTO_SIZE, pExt + 8, 22);
		}
		else if(GNSS_HasPrefix(pExt, "MOD="))
		{
			GNSS_CopyIdentField(&newIdent[GNSS_IDENT_MOD_OFFSET], GNSS_IDENT_MOD_SIZE, pExt + 4, 26);
		}
		else if(!gnssFound)
		{
			for(uint8_t pos = 0; (pos < 30) && (pExt[pos] != 0); pos++)
			{
				if(pExt[pos] == ';')
				{
					GNSS_CopyIdentField(&newIdent[GNSS_IDENT_GNSS_OFFSET], GNSS_IDENT_GNSS_SIZE, pExt, 30);
					gnssFound = 1;
					break;
				}
			}
		}
	}
	memcpy(GNSS->ident, newIdent, sizeof(newIdent));
	GNSS->identValid = 1;
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
