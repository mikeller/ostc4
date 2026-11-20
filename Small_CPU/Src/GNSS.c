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
	memset(GNSS->pollSent, 0, sizeof(GNSS->pollSent));
	memset(GNSS->pollAnswered, 0, sizeof(GNSS->pollAnswered));
#ifdef GNSS_DEBUG_POLLS
	memset(GNSS->pollLost, 0, sizeof(GNSS->pollLost));
	GNSS->pollAnomaly = 0;
#endif
	GNSS->cfgReadPending = 0;
#ifdef GNSS_DEBUG_POLLS
	GNSS_ResetReadOnlyPolls(GNSS);
#endif
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

/* The sequence of the debug records, see data_exchange.h: GNSS_DEBUG_SCHEDULE_LEN slots, the text slot sends the next item of the text and the extra items. With the
 * debug view a text slot follows each of the other records (30 slots, 3 s), without it the text slots are at the end (18 slots) */
#define GNSS_TXT	GNSS_DEBUG_REC_TEXT
static const uint8_t debugSchedule[] =
{
#ifdef HAVE_DEBUG_VIEW
	0, GNSS_TXT, 1, GNSS_TXT, 2, GNSS_TXT, 3, GNSS_TXT, 4, GNSS_TXT, 5, GNSS_TXT, 6, GNSS_TXT, 7, GNSS_TXT, 8, GNSS_TXT, 9, GNSS_TXT, 10, GNSS_TXT, 11, GNSS_TXT,
	GNSS_DEBUG_REC_POWER, GNSS_TXT, GNSS_DEBUG_REC_POLL_A, GNSS_TXT, GNSS_DEBUG_REC_POLL_B, GNSS_TXT
#else
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	GNSS_TXT, GNSS_DEBUG_REC_POWER, GNSS_DEBUG_REC_POLL_A, GNSS_DEBUG_REC_POLL_B, GNSS_TXT, GNSS_TXT
#endif
};
_Static_assert(sizeof(debugSchedule) == GNSS_DEBUG_SCHEDULE_LEN, "The debug record schedule does not have GNSS_DEBUG_SCHEDULE_LEN slots");

uint8_t GNSS_DebugScheduleRecord(uint8_t slot) {
	return debugSchedule[(slot < GNSS_DEBUG_SCHEDULE_LEN) ? slot : 0];
}

#ifdef GNSS_DEBUG_POLLS
/*!
 * The values of the read only polls are not valid until they were read after a (re)initialization or a power up of the receiver.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ResetReadOnlyPolls(GNSS_StateHandle *GNSS) {
	GNSS->cfgPmsValid = 0;
	GNSS->cfgGnssValid = 0;
	GNSS->cfgGnssState = GNSS_CFGPOLL_NONE;
	GNSS->cfgGnssTries = 0;
	GNSS->cfgGnssLength = 0;
	GNSS->cfgNav5Valid = 0;
	GNSS->cfgAntState = GNSS_CFGPOLL_NONE;
	GNSS->cfgAntTries = 0;
	GNSS->cfgItfmState = GNSS_CFGPOLL_NONE;
	GNSS->cfgItfmTries = 0;
	GNSS->hw2State = GNSS_CFGPOLL_NONE;
	GNSS->hw2Tries = 0;
	GNSS->hw2Have = 0;
	GNSS->hw2Length = 0;
	GNSS->statusState = GNSS_CFGPOLL_NONE;
	GNSS->statusTries = 0;
	GNSS->statusHave = 0;
	GNSS->statusLength = 0;
	GNSS->msssPrevValid = 0;		/* the receiver was reset by the initialization or is started again after it was off */
	GNSS->resetCount = 0;
	GNSS->ioState = GNSS_CFGPOLL_NONE;
	GNSS->ioTries = 0;
	GNSS->ioHave = 0;
	GNSS->ioLength = 0;
}
#endif

/*!
 * Fill one record of the GNSS debug record sequence, see data_exchange.h.
 * @param GNSS Pointer to main GNSS structure.
 * @param index Record index 0 .. GNSS_DEBUG_REC_CNT - 1, a higher index is treated like 0.
 * @param age_s Seconds since the last valid frame.
 * @param textChunk Number of the chunk of the identification text which is sent by a text record (a higher number is treated like 0).
 * @param rec The record.
 */
void GNSS_GetDebugRecord(const GNSS_StateHandle *GNSS, uint8_t index, uint8_t age_s, uint8_t textChunk, uint8_t rec[GNSS_DEBUG_REC_SIZE]) {
	const SGnssDebugSat* pSat;
	uint32_t pDop_tenth;
	uint32_t monHwAge_s;
	uint32_t psmZeroAge_s;

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
		case GNSS_DEBUG_REC_MONHW_A:
			if(GNSS->monHwValid)
			{
				rec[1] = GNSS->noisePerMs & 0xFFu;
				rec[2] = GNSS->noisePerMs >> 8;
				rec[3] = GNSS->agcCnt & 0xFFu;
				rec[4] = GNSS->agcCnt >> 8;
				rec[5] = (GNSS->aStatus & 0x0Fu) | (uint8_t)((GNSS->aPower & 0x0Fu) << 4);
			}
			break;
		case GNSS_DEBUG_REC_MONHW_B:
			monHwAge_s = GNSS->monHwSeen ? (time_elapsed_ms(GNSS->monHwTick, HAL_GetTick()) / 1000u) : 255u;
			rec[1] = GNSS->monHwValid ? GNSS->hwFlags : 0;
			rec[2] = GNSS->monHwValid ? GNSS->jamInd : 0;
			rec[3] = (monHwAge_s > 254u) ? 255u : (uint8_t)monHwAge_s;
			rec[4] = GNSS->monHwLength;
			rec[5] = (GNSS->monHwValid ? 0x01u : 0) | (GNSS->identValid ? 0x02u : 0) | (uint8_t)(GNSS_Limit(GNSS->monVerRequests, 15) << 4);
			break;
		case GNSS_DEBUG_REC_POWER:
			rec[1] = GNSS->cfgPmsSetup;
			rec[2] = GNSS->cfgGnssMask;
			rec[3] = GNSS->cfgDynModel;
			psmZeroAge_s = GNSS->psmZeroValid ? (time_elapsed_ms(GNSS->psmZeroTick, HAL_GetTick()) / 1000u) : 255u;
			rec[4] = (psmZeroAge_s > 254u) ? 255u : (uint8_t)psmZeroAge_s;
			rec[5] = (GNSS->cfgPmsValid ? 0x01u : 0) | (GNSS->cfgGnssValid ? 0x02u : 0) | (GNSS->cfgNav5Valid ? 0x04u : 0) | (GNSS->wakeFailed ? 0x08u : 0)
					| (GNSS->verifyGaveUp ? 0x10u : 0) | (GNSS->psmZeroValid ? 0x40u : 0);
			break;
		case GNSS_DEBUG_REC_POLL_A:
			rec[1] = GNSS->wakeTries;
			rec[2] = GNSS->verifyTries;
			rec[3] = GNSS->pollSent[GNSS_POLL_PVT];
			rec[4] = GNSS->pollAnswered[GNSS_POLL_PVT];
			rec[5] = GNSS->pollSent[GNSS_POLL_SAT];
			break;
		case GNSS_DEBUG_REC_POLL_B:
			rec[1] = GNSS->pollAnswered[GNSS_POLL_SAT];
			rec[2] = GNSS->pollSent[GNSS_POLL_MONHW];
			rec[3] = GNSS->pollAnswered[GNSS_POLL_MONHW];
			break;
		case GNSS_DEBUG_REC_TEXT:
			if(textChunk >= GNSS_DEBUG_TEXT_ITEMS)
			{
				textChunk = 0;
			}
			if(textChunk < GNSS_IDENT_CHUNKS)
			{
				rec[1] = textChunk | (GNSS->identValid ? 0x80u : 0);
				if(GNSS->identValid)
				{
					memcpy(&rec[2], &GNSS->ident[textChunk * GNSS_IDENT_CHUNK_SIZE], GNSS_IDENT_CHUNK_SIZE);
				}
			}
#ifdef GNSS_DEBUG_POLLS
			else		/* read back of UBX-CFG-ANT and UBX-CFG-ITFM */
			{
				rec[1] = (uint8_t)(GNSS_DEBUG_EXTRA_BASE + (textChunk - GNSS_IDENT_CHUNKS));
				switch(textChunk - GNSS_IDENT_CHUNKS)
				{
					case 0:
						rec[2] = GNSS->cfgAntFlags & 0xFFu;
						rec[3] = GNSS->cfgAntFlags >> 8;
						rec[4] = GNSS->cfgAntPins & 0xFFu;
						rec[5] = GNSS->cfgAntPins >> 8;
						break;
					case 1:
						memcpy(&rec[2], &GNSS->cfgItfmConfig, 4);		/* little endian like the receiver (the RTE is little endian) */
						break;
					case 2:
						memcpy(&rec[2], &GNSS->cfgItfmConfig2, 4);
						break;
					case 3:
						rec[2] = GNSS->cfgAntState;
						rec[3] = GNSS->cfgItfmState;
						rec[4] = GNSS->cfgAntTries;
						rec[5] = GNSS->cfgItfmTries;
						break;
					case 4:		/* UBX-MON-HW2 */
						rec[2] = (uint8_t)GNSS->ofsI;
						rec[3] = GNSS->magI;
						rec[4] = (uint8_t)GNSS->ofsQ;
						rec[5] = GNSS->magQ;
						break;
					case 5:
						memcpy(&rec[2], &GNSS->lowLevCfg, 4);
						break;
					case 6:
						memcpy(&rec[2], &GNSS->postStatus, 4);
						break;
					case 7:
						rec[2] = GNSS->cfgSource;
						rec[3] = GNSS->hw2State | (GNSS->hw2Have ? GNSS_CFGPOLL_HAVE : 0);
						rec[4] = GNSS->hw2Tries;
						rec[5] = GNSS->hw2Length;
						break;
					case 8:		/* UBX-NAV-STATUS */
						memcpy(&rec[2], &GNSS->msss, 4);
						break;
					case 9:
						memcpy(&rec[2], &GNSS->ttff, 4);
						break;
					case 10:
						rec[2] = GNSS->statusFix;
						rec[3] = GNSS->statusFlags;
						rec[4] = GNSS->statusFixStat;
						rec[5] = GNSS->statusFlags2;
						break;
					case 11:
						rec[2] = GNSS->statusState | (GNSS->statusHave ? GNSS_CFGPOLL_HAVE : 0);
						rec[3] = GNSS->statusTries;
						rec[4] = GNSS->resetCount;
						rec[5] = GNSS->statusLength;
						break;
					case 17:		/* UBX-MON-IO */
						memcpy(&rec[2], &GNSS->ioRxBytes, 4);
						break;
					case 18:
						memcpy(&rec[2], &GNSS->ioTxBytes, 4);
						break;
					case 19:
						memcpy(&rec[2], &GNSS->ioParityErrs, 2);
						memcpy(&rec[4], &GNSS->ioFramingErrs, 2);
						break;
					case 20:
						memcpy(&rec[2], &GNSS->ioOverrunErrs, 2);
						memcpy(&rec[4], &GNSS->ioBreakCond, 2);
						break;
					case 21:
						rec[2] = GNSS->ioState | (GNSS->ioHave ? GNSS_CFGPOLL_HAVE : 0);
						rec[3] = GNSS->ioTries;
						rec[4] = GNSS->ioPort;
						rec[5] = GNSS->ioLength;
						break;
					case 22:		/* the read back of UBX-CFG-GNSS */
						rec[2] = GNSS->cfgGnssState | (GNSS->cfgGnssValid ? GNSS_CFGPOLL_HAVE : 0);
						rec[3] = GNSS->cfgGnssTries;
						rec[4] = GNSS->cfgGnssLength;
						break;
					default:	/* 12..16: the counts of the lost polls by cause (15 bytes), then the UART error types (5 bytes), four bytes per item */
						{
							uint8_t diag[GNSS_DEBUG_DIAG_SIZE];

							memcpy(diag, GNSS->pollLost, 3u * GNSS_CAUSE_CNT);
							memcpy(&diag[3u * GNSS_CAUSE_CNT], GNSS->uartErrType, GNSS_UARTERR_CNT);
							memcpy(&rec[2], &diag[4u * (uint8_t)(textChunk - GNSS_IDENT_CHUNKS - 12u)], 4);
						}
						break;
				}
			}
#endif
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
#ifdef GNSS_DEBUG_POLLS
	if(length != GNSS_MONHW_LEN)
	{
		GNSS_NotePollCause(GNSS, GNSS_POLL_MONHW, GNSS_CAUSE_MISMATCH);
	}
#endif
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

/*!
 * Read back of the configuration (read only polls), see the u-blox 8 / u-blox M8 Receiver description: UBX-CFG-PMS (32.10.24, 8 bytes, powerSetupValue at
 * offset 1), UBX-CFG-GNSS (32.10.8: 4 byte header and 8 bytes per GNSS block with gnssId at offset 0 and the enable bit as bit 0 of the flags at offset 4),
 * UBX-CFG-NAV5 (36 bytes, dynModel at offset 2). A frame with another length is not used.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseCfgPmsData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);

	if(length == 8)
	{
		GNSS->cfgPmsSetup = GNSS_Handle.uartWorkingBuffer[6 + 1];
		GNSS->cfgPmsValid = 1;
	}
}

void GNSS_ParseCfgGnssData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);
	uint8_t blocks = GNSS_Handle.uartWorkingBuffer[6 + 3];
	uint8_t mask = 0;

	GNSS->cfgGnssLength = (length > 255u) ? 255u : (uint8_t)length;
	GNSS->cfgGnssState = GNSS_CFGPOLL_OK;
	if((length >= 4u) && (length == (4u + 8u * blocks)))
	{
		for(uint8_t i = 0; i < blocks; i++)
		{
			const uint8_t* pBlock = &GNSS_Handle.uartWorkingBuffer[6 + 4 + 8 * i];
			if((pBlock[0] < 8u) && (pBlock[4] & 0x01u))
			{
				mask |= (uint8_t)(1u << pBlock[0]);
			}
		}
		GNSS->cfgGnssMask = mask;
		GNSS->cfgGnssValid = 1;
	}
}

void GNSS_ParseCfgNav5Data(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);

	if(length == 36)
	{
		GNSS->cfgDynModel = GNSS_Handle.uartWorkingBuffer[6 + 2];
		GNSS->cfgNav5Valid = 1;
	}
}

/*!
 * UBX-CFG-ANT (32.10.1, 4 bytes) and UBX-CFG-ITFM (32.10.16, 8 bytes), raw values little endian; a reply of another length is not used. The meaning of the bits is
 * decoded by the main CPU (data_exchange.h).
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseCfgAntData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);

	if(length == 4)
	{
		GNSS->cfgAntFlags = GNSS_Handle.uartWorkingBuffer[6] | (GNSS_Handle.uartWorkingBuffer[7] << 8);
		GNSS->cfgAntPins = GNSS_Handle.uartWorkingBuffer[8] | (GNSS_Handle.uartWorkingBuffer[9] << 8);
		GNSS->cfgAntState = GNSS_CFGPOLL_OK;
		GNSS->cfgAntTries = 0;
	}
}

void GNSS_ParseCfgItfmData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);

	if(length == 8)
	{
		GNSS->cfgItfmConfig = (uint32_t)GNSS_Handle.uartWorkingBuffer[6] | ((uint32_t)GNSS_Handle.uartWorkingBuffer[7] << 8) | ((uint32_t)GNSS_Handle.uartWorkingBuffer[8] << 16)
				| ((uint32_t)GNSS_Handle.uartWorkingBuffer[9] << 24);
		GNSS->cfgItfmConfig2 = (uint32_t)GNSS_Handle.uartWorkingBuffer[10] | ((uint32_t)GNSS_Handle.uartWorkingBuffer[11] << 8) | ((uint32_t)GNSS_Handle.uartWorkingBuffer[12] << 16)
				| ((uint32_t)GNSS_Handle.uartWorkingBuffer[13] << 24);
		GNSS->cfgItfmState = GNSS_CFGPOLL_OK;
		GNSS->cfgItfmTries = 0;
	}
}

/*!
 * UBX-MON-HW2 (32.16.3, 28 bytes): ofsI at 0, magI 1, ofsQ 2, magQ 3, cfgSource 4, lowLevCfg 8 (U4), postStatus 20 (U4) of the payload (the same layout in pyubx2).
 * The values are only taken from a reply with the accepted length, for another length only the length is kept.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseMonHw2Data(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);
	const uint8_t* p = &GNSS_Handle.uartWorkingBuffer[6];

	GNSS->hw2Length = (length > 255u) ? 255u : (uint8_t)length;
	GNSS->hw2State = GNSS_CFGPOLL_OK;
	GNSS->hw2Tries = 0;
	if(length == GNSS_MONHW2_LEN)
	{
		GNSS->ofsI = (int8_t)p[0];
		GNSS->magI = p[1];
		GNSS->ofsQ = (int8_t)p[2];
		GNSS->magQ = p[3];
		GNSS->cfgSource = p[4];
		GNSS->lowLevCfg = (uint32_t)p[8] | ((uint32_t)p[9] << 8) | ((uint32_t)p[10] << 16) | ((uint32_t)p[11] << 24);
		GNSS->postStatus = (uint32_t)p[20] | ((uint32_t)p[21] << 8) | ((uint32_t)p[22] << 16) | ((uint32_t)p[23] << 24);
		GNSS->hw2Have = 1;
	}
}

/*!
 * UBX-NAV-STATUS (32.17.24, 16 bytes): iTOW at 0, gpsFix 4, flags 5, fixStat 6, flags2 7, ttff 8 (U4, ms), msss 12 (U4, ms since startup or reset); the same layout in
 * pyubx2 and the ublox_msgs package. A reply with another length is not used. A reset of the receiver is detected if msss is smaller than in the reply before.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseNavStatusData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);
	const uint8_t* p = &GNSS_Handle.uartWorkingBuffer[6];
	uint32_t msss;

	GNSS->statusLength = (length > 255u) ? 255u : (uint8_t)length;
	GNSS->statusState = GNSS_CFGPOLL_OK;
	GNSS->statusTries = 0;
	if(length == GNSS_NAVSTATUS_LEN)
	{
		msss = (uint32_t)p[12] | ((uint32_t)p[13] << 8) | ((uint32_t)p[14] << 16) | ((uint32_t)p[15] << 24);
		if(GNSS->msssPrevValid && (msss < GNSS->msss) && (GNSS->resetCount < 255))
		{
			GNSS->resetCount++;
		}
		GNSS->msssPrevValid = 1;
		GNSS->msss = msss;
		GNSS->statusFix = p[4];
		GNSS->statusFlags = p[5];
		GNSS->statusFixStat = p[6];
		GNSS->statusFlags2 = p[7];
		GNSS->ttff = (uint32_t)p[8] | ((uint32_t)p[9] << 8) | ((uint32_t)p[10] << 16) | ((uint32_t)p[11] << 24);
		GNSS->statusHave = 1;
	}
}

/* little endian values of the payload */
static uint32_t GNSS_Le32(const uint8_t* p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t GNSS_Le16(const uint8_t* p) {
	return (uint16_t)(p[0] | (p[1] << 8));
}

/*!
 * UBX-MON-IO (32.16.5): 20 bytes per port: rxBytes (U4) at 0, txBytes (U4) 4, parityErrs (U2) 8, framingErrs (U2) 10, overrunErrs (U2) 12, breakCond (U2) 14, the last four bytes
 * are reserved in the receiver description (pyubx2 has two busy flags there, they are not used). The first sixteen bytes agree in pyubx2. The reply is only used if its
 * length is a whole number of ports (at most GNSS_MONIO_MAX_PORTS). The values of GNSS_MONIO_DEFAULT_PORT are taken, or those of the port which sent most bytes if that
 * port has not sent anything.
 * @param GNSS Pointer to main GNSS structure.
 */
void GNSS_ParseMonIoData(GNSS_StateHandle *GNSS) {
	uint16_t length = GNSS_Handle.uartWorkingBuffer[4] + (GNSS_Handle.uartWorkingBuffer[5] << 8);
	const uint8_t* p = &GNSS_Handle.uartWorkingBuffer[6];
	uint8_t ports = (uint8_t)(length / GNSS_MONIO_PORT_SIZE);
	uint8_t port = GNSS_MONIO_DEFAULT_PORT;

	GNSS->ioLength = (length > 255u) ? 255u : (uint8_t)length;
	GNSS->ioState = GNSS_CFGPOLL_OK;
	GNSS->ioTries = 0;
	if((length != 0u) && ((length % GNSS_MONIO_PORT_SIZE) == 0u) && (ports <= GNSS_MONIO_MAX_PORTS))
	{
		if((port >= ports) || (GNSS_Le32(&p[port * GNSS_MONIO_PORT_SIZE + 4u]) == 0u))
		{
			uint32_t most = 0;

			for(uint8_t i = 0; i < ports; i++)
			{
				uint32_t tx = GNSS_Le32(&p[i * GNSS_MONIO_PORT_SIZE + 4u]);

				if(tx > most)
				{
					most = tx;
					port = i;
				}
			}
			if(most == 0u)
			{
				port = (GNSS_MONIO_DEFAULT_PORT < ports) ? GNSS_MONIO_DEFAULT_PORT : 0u;
			}
		}
		p = &p[port * GNSS_MONIO_PORT_SIZE];
		GNSS->ioPort = port;
		GNSS->ioRxBytes = GNSS_Le32(&p[0]);
		GNSS->ioTxBytes = GNSS_Le32(&p[4]);
		GNSS->ioParityErrs = GNSS_Le16(&p[8]);
		GNSS->ioFramingErrs = GNSS_Le16(&p[10]);
		GNSS->ioOverrunErrs = GNSS_Le16(&p[12]);
		GNSS->ioBreakCond = GNSS_Le16(&p[14]);
		GNSS->ioHave = 1;
	}
}

/*!
 * Counts the cyclic requests which were transmitted and answered.
 * @param GNSS Pointer to main GNSS structure.
 * @param poll GNSS_POLL_PVT, GNSS_POLL_SAT or GNSS_POLL_MONHW.
 */
void GNSS_NotePollSent(GNSS_StateHandle *GNSS, uint8_t poll) {
	if(GNSS->pollSent[poll] == 255)		/* keep the ratio */
	{
		for(uint8_t i = 0; i < 3; i++)
		{
			GNSS->pollSent[i] >>= 1;
			GNSS->pollAnswered[i] >>= 1;
		}
	}
	GNSS->pollSent[poll]++;
#ifdef GNSS_DEBUG_POLLS
	GNSS->pollAnomaly = 0;
#endif
}

/*!
 * Counts a lost or disturbed poll by its cause (GNSS_CAUSE_*), for the debug page. Not for the time out: see GNSS_NotePollTimeout.
 * @param GNSS Pointer to main GNSS structure.
 * @param poll GNSS_POLL_PVT, GNSS_POLL_SAT or GNSS_POLL_MONHW.
 * @param cause GNSS_CAUSE_*.
 */
void GNSS_NotePollCause(GNSS_StateHandle *GNSS, uint8_t poll, uint8_t cause) {
	if((poll < 3u) && (cause < GNSS_CAUSE_CNT))
	{
		if(GNSS->pollLost[poll][cause] < 255u)
		{
			GNSS->pollLost[poll][cause]++;
		}
		GNSS->pollAnomaly = 1;
	}
}

/*!
 * The poll slot ended without a reply: counted as a time out if nothing else was noted for the request (a checksum error or another message is the cause then).
 * @param GNSS Pointer to main GNSS structure.
 * @param poll GNSS_POLL_PVT, GNSS_POLL_SAT or GNSS_POLL_MONHW.
 */
void GNSS_NotePollTimeout(GNSS_StateHandle *GNSS, uint8_t poll) {
	if((poll < 3u) && (!GNSS->pollAnomaly) && (GNSS->pollLost[poll][GNSS_CAUSE_TIMEOUT] < 255u))
	{
		GNSS->pollLost[poll][GNSS_CAUSE_TIMEOUT]++;
	}
}

/*!
 * Counts the types of a UART error: the flags of the error code of the HAL (HAL_UART_ERROR_PE 0x01, NE 0x02, FE 0x04, ORE 0x08, DMA 0x10).
 * @param GNSS Pointer to main GNSS structure.
 * @param errorCode Error code of the UART handle.
 */
void GNSS_NoteUartError(GNSS_StateHandle *GNSS, uint32_t errorCode) {
	static const uint8_t flag[GNSS_UARTERR_CNT] = { 0x01u, 0x04u, 0x02u, 0x08u, 0x10u };		/* in the order of GNSS_UARTERR_* */

	for(uint8_t i = 0; i < GNSS_UARTERR_CNT; i++)
	{
		if((errorCode & flag[i]) && (GNSS->uartErrType[i] < 255u))
		{
			GNSS->uartErrType[i]++;
		}
	}
}

void GNSS_NotePollAnswered(GNSS_StateHandle *GNSS, uint8_t poll) {
	if(GNSS->pollAnswered[poll] < GNSS->pollSent[poll])		/* an answer to a request which was counted */
	{
		GNSS->pollAnswered[poll]++;
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
