 /*
 * GNSS.h
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

#ifndef INC_GNSS_H_
#define INC_GNSS_H_

#include "stm32f4xx_hal.h"
#include "data_exchange.h"

union u_Short
{
	uint8_t bytes[2];
	unsigned short uShort;
};

union i_Short
{
	uint8_t bytes[2];
	signed short iShort;
};

union u_Long
{
	uint8_t bytes[4];
	unsigned long uLong;
};

union i_Long
{
	uint8_t bytes[4];
	signed long iLong;
};

/* Size of the largest UBX frame which is accepted: 6 byte header, UBX-NAV-SAT payload (8 byte header + 12 bytes per satellite), 2 byte checksum */
#define GNSS_MAX_NAVSAT_SV		(64u)
#define GNSS_UBX_FRAME_SIZE_MAX	(16u + 12u * GNSS_MAX_NAVSAT_SV)

/* Polls of the receiver: UBX-NAV-SAT and UBX-NAV-PVT alternately, every GNSS_MON_POLL_INTERVAL-th poll is a UBX-MON poll instead (HW, or VER as long as it was not
 * received) so that NAV-SAT and NAV-PVT are still polled in five of six slots. Polls are one per second (see UART6_HandleUART). */
#define GNSS_MON_POLL_INTERVAL		(6u)
#define GNSS_MONVER_MAX_REQUESTS	(10u)	/* UBX-MON-VER is requested at most this many times per initialization of the receiver */

/* Polls which are only needed by the debug pages of the main CPU (UBX-MON-HW, UBX-MON-VER) are sent only if the debug view is built (HAVE_DEBUG_VIEW of
 * configuration.h) */
#ifdef HAVE_DEBUG_VIEW
#define GNSS_DEBUG_POLLS
#endif

/* Power up of the receiver after it was in power save mode (see uartProtocol_GNSS.c): wake up bytes, time for the receiver to wake (the receiver description
 * recommends about half a second), full power command; without acknowledge or with a rejection the sequence is repeated */
#define GNSS_WAKE_WAIT_MS			(500u)
#define GNSS_PWRUP_ACK_TIMEOUT_MS	(1000u)
#define GNSS_WAKE_MAX_RETRIES		(3u)
#define GNSS_WAKE_TX_TIMEOUT_MS		(3000u)	/* the wake up bytes cannot be sent for this long because the transmitter is busy: give up */

/* Check of the power save state: the receiver is meant to run at full power while the unit is awake. If GNSS_VERIFY_PVT_COUNT UBX-NAV-PVT in a row (about 7 s)
 * report a power save state, or no valid frame arrived for GNSS_FRAMES_STOPPED_MS, the full power command is sent again (with the wake up bytes in the second
 * case), at most GNSS_VERIFY_MAX_RETRIES times until psmState 0 was seen; commands of the second kind are at least GNSS_VERIFY_SPACING_MS apart */
#define GNSS_VERIFY_PVT_COUNT		(3u)
#define GNSS_VERIFY_MAX_RETRIES		(3u)
#define GNSS_FRAMES_STOPPED_MS		(3000u)
#define GNSS_VERIFY_SPACING_MS		(5000u)

#define GNSS_POS_MAX_AGE_S		(2u * 3600u)	/* a position which was received earlier is not reported as valid */

/* The receiver is reported as alive (GNSS_ALIVE_STATE_ALIVE) as long as a valid frame was received not longer ago than this.
 * The RTE polls NAV-SAT and NAV-PVT alternately once per second (see UART6_HandleUART), so a reply is expected about every second
 * (NAV-PVT alone about every two seconds). The window tolerates a few lost polls without making the indication flicker. */
#define GNSS_ALIVE_TIMEOUT_MS	(5000u)

typedef struct
{
	UART_HandleTypeDef *huart;

	uint8_t uniqueID[4];
	uint8_t uartWorkingBuffer[GNSS_UBX_FRAME_SIZE_MAX];

	unsigned short year;
	uint8_t yearBytes[2];
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t fixType;

	signed long lon;
	uint8_t lonBytes[4];
	signed long lat;
	uint8_t latBytes[4];
	float fLon;
	float fLat;

	signed long height;
	signed long hMSL;
	uint8_t hMSLBytes[4];
	unsigned long hAcc;
	unsigned long vAcc;

	signed long gSpeed;
	uint8_t gSpeedBytes[4];
	signed long headMot;

	uint8_t numSat;
	uint8_t statSat[4];
	uint8_t numEphSat;			/* satellites of the last UBX-NAV-SAT with long lived ephemeris (GPS, Galileo, BeiDou, QZSS) available */
	uint8_t fixOk;				/* gnssFixOK flag of the last UBX-NAV-PVT */

	/* values for the GNSS debug page, see the GNSS debug record in data_exchange.h */
	uint8_t numTracked;			/* satellites of the last UBX-NAV-SAT with C/N0 > 0 */
	uint8_t numUsed;			/* satellites of the last UBX-NAV-SAT which are used for navigation (svUsed) */
	uint8_t bestCno;			/* highest C/N0 (dBHz) of the last UBX-NAV-SAT */
	SGnssDebugSat debugSat[GNSS_DEBUG_SAT_CNT];	/* the satellites with the highest C/N0 of the last UBX-NAV-SAT */
	uint16_t pDop;				/* pDOP of the last UBX-NAV-PVT in 0.01, 0xFFFF if not available */
	uint8_t timeFlags;			/* valid flags and confirmed flags of the last UBX-NAV-PVT, see the debug record */
	uint8_t numSvUsedPvt;		/* numSV of the last UBX-NAV-PVT */
	uint8_t psmState;			/* psmState of the last UBX-NAV-PVT */
	uint8_t pvtParsed;			/* a UBX-NAV-PVT was parsed since the receiver was initialized or woken: psmState is valid */
	uint8_t uartErrors;			/* UART errors of the receiver port since the RTE started (saturating) */
	uint8_t nakCount;			/* UBX-ACK-NAK received since the RTE started (saturating) */

	/* read only polls of UBX-MON-HW and UBX-MON-VER for the debug page */
	uint8_t monHwSeen;			/* a UBX-MON-HW (of any length) was received since the receiver was (re)initialized */
	uint8_t monHwValid;			/* the last UBX-MON-HW has the accepted length GNSS_MONHW_LEN, the following values were taken from it */
	uint8_t monHwLength;		/* payload length of the last UBX-MON-HW (saturating at 255) */
	uint32_t monHwTick;			/* HAL tick of the last UBX-MON-HW */
	uint16_t noisePerMs;
	uint16_t agcCnt;
	uint8_t aStatus;
	uint8_t aPower;
	uint8_t hwFlags;
	uint8_t jamInd;
	uint8_t identValid;			/* UBX-MON-VER was received, ident contains its text */
	uint8_t monVerRequests;		/* number of UBX-MON-VER requests since the receiver was (re)initialized, limited to GNSS_MONVER_MAX_REQUESTS */
	uint8_t monStartup;			/* number of poll slots after the initialization which are used for UBX-MON-HW / UBX-MON-VER without waiting */
	uint8_t ident[GNSS_IDENT_SIZE];	/* identification text, see data_exchange.h */

	/* switching the receiver to full power (initialization, wake up after sleep or dive), see the comment of the state machine in uartProtocol_GNSS.c */
	uint8_t pwrupWake;			/* the power up follows a wake up (0xFF bytes) of a receiver in power save mode */
	uint8_t pwrupSent;			/* the full power command of the current attempt was transmitted */
	uint8_t pwrupNak;			/* the receiver rejected the command (UBX-ACK-NAK) */
	uint32_t pwrupSendTick;		/* HAL tick at which the full power command was transmitted */
	uint8_t wakeSent;			/* the wake up bytes of the current attempt were transmitted */
	uint8_t wakeTries;			/* repetitions of the power up sequence in the current power up (0 .. GNSS_WAKE_MAX_RETRIES) */
	uint8_t wakeFailed;			/* the power up got no acknowledge after GNSS_WAKE_MAX_RETRIES repetitions */

	/* check of the power save state (psmState of UBX-NAV-PVT) while the unit is awake, see uartGnss_VerifyPower */
	uint8_t verifyBad;			/* number of UBX-NAV-PVT in a row which report a power save state (not 0) */
	uint8_t verifyResend;		/* GNSS_VERIFY_PVT_COUNT in a row: send full power again */
	uint8_t verifyTries;		/* full power commands sent by the check since psmState was 0 the last time (0 .. GNSS_VERIFY_MAX_RETRIES) */
	uint8_t verifyGaveUp;		/* the check did not succeed after GNSS_VERIFY_MAX_RETRIES commands */
	uint32_t verifyActionTick;	/* HAL tick of the last command of the check */
	uint32_t psmZeroTick;		/* HAL tick of the last UBX-NAV-PVT with psmState 0 */
	uint8_t psmZeroValid;		/* psmZeroTick is valid */

	uint8_t alive;
	uint8_t lastFrameValid;		/* a valid frame was received since the receiver was (re)initialized */
	uint32_t lastFrameTick;		/* HAL tick of the last valid frame received from the receiver (see GNSS_NoteFrameReceived) */

	float last_fLon;	/* last known position storage and time stamp */
	float last_fLat;
	uint8_t lastPosValid;		/* last_* contains a position which was received from the receiver */
	uint8_t last_hAcc;			/* accuracy information of the last known position (see SGnssAccuracy) */
	uint8_t last_numSatUsed;
	uint32_t last_fixTime_s;	/* RTC time of day in s when the last known position was received */

}GNSS_StateHandle;

GNSS_StateHandle GNSS_Handle;

typedef struct {
	uint8_t header1;
	uint8_t header2;
    uint8_t msgClass;
    uint8_t msgID;
    uint16_t length;
    uint8_t type;
    uint8_t version;
    uint8_t ref;
    uint8_t leapsec;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t flags;
    uint32_t nanoseconds;
    uint16_t accuracy_seconds;
    uint16_t reserved;
    uint32_t accuracy_nano;
}  __attribute__((packed)) UBX_MGA_INI_TIME_UTC_t;

typedef struct {
    uint8_t header1;
    uint8_t header2;
    uint8_t class_id;
    uint8_t msg_id;
    uint16_t length;
    uint8_t type;
    uint8_t version;
    uint16_t reserved;
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    uint32_t accuracy;
} __attribute__((packed)) UBX_MGA_INI_POS_LLH_t;


enum GNSSMode{Portable=0, Stationary=1, Pedestrian=2, Automotiv=3, Airbone1G=5, Airbone2G=6,Airbone4G=7,Wirst=8,Bike=9};

static const uint8_t configUBX[]={0xB5,0x62,0x06,0x00,0x14,0x00,0x01,0x00,0x00,0x00,0xD0,0x08,0x00,0x00,0x80,0x25,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00};

static const uint8_t setNMEA410[]={0xB5,0x62,0x06,0x17,0x14,0x00,0x00,0x41,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

//Activation of navigation system: Galileo, Glonass, GPS, SBAS, IMES
static const uint8_t setGNSS[]={0xB5,0x62,0x06,0x3E,0x24,0x00,0x00,0x00,0x20,0x04,0x00,0x08,0x10,0x00,0x01,0x00,0x01,0x01,0x01,0x01,0x03,0x00,0x01,0x00,0x01,0x01,0x02,0x04,0x08,0x00,0x01,0x00,0x01,0x01,0x06,0x08,0x0E,0x00,0x01,0x00,0x01,0x01};

static const uint8_t getDeviceID[]={0xB5,0x62,0x27,0x03,0x00,0x00};

static const uint8_t getNavigatorData[]={0xB5,0x62,0x01,0x21,0x00,0x00};

static const uint8_t getPOSLLHData[]={0xB5,0x62,0x01,0x02,0x00,0x00};

static const uint8_t getPVTData[]={0xB5,0x62,0x01,0x07,0x00,0x00};

static const uint8_t getNavSat[]={0xB5,0x62,0x01,0x35,0x00,0x00};

/* read only polls (UBX-MON-HW, UBX-MON-VER) */
static const uint8_t getMonHw[]={0xB5,0x62,0x0A,0x09,0x00,0x00};

static const uint8_t getMonVer[]={0xB5,0x62,0x0A,0x04,0x00,0x00};

static const uint8_t setPowerLow[]={0xB5,0x62,0x06,0x86,0x08,0x00,0x00,0x02,0x10,0x0E,0x14,0x00,0x00,0x00};

static const uint8_t setPowerNormal[]={0xB5,0x62,0x06,0x86,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/* UBX-CFG-NAV5 */
static const uint8_t setPortableMode[]={0xB5,0x62,0x06,0x24,0x24,0x00,0xFF,0xFF,0x00,0x03,0x00,0x00,0x00,0x00,0x10,0x27,0x00,0x00,0x05,0x00,0xFA,0x00,0xFA,0x00,0x64,0x00,0x5E,0x01,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const uint8_t setPedestrianMode[]={0xB5,0x62,0x06,0x24,0x24,0x00,0xFF,0xFF,0x03,0x03,0x00,0x00,0x00,0x00,0x10,0x27,0x00,0x00,0x05,0x00,0xFA,0x00,0xFA,0x00,0x64,0x00,0x5E,0x01,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

static const uint8_t setConfig[]={0xB5,0x62,0x06,0x09,0x0D,0x00, 0x00,0x00,0x00,0x00, 0x18,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x01};
/* UBX-MGA-INI-TIME_UTC */
static const uint8_t setDateTime[] = {0xB5,0x62,0x13,0x40,0x18,0x00,0x10,0x00,0x00,0x80,0x07,0xE9,0x01,0x02,0x03,0x04,0x05,0x00,0x01,0x02,0x03,0x04,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00};

static const uint8_t setPortableType[]={};
void GNSS_Init(GNSS_StateHandle *GNSS, UART_HandleTypeDef *huart);
void GNSS_NoteFrameReceived(GNSS_StateHandle *GNSS);
void GNSS_GetDebugRecord(const GNSS_StateHandle *GNSS, uint8_t index, uint8_t age_s, uint8_t rec[GNSS_DEBUG_REC_SIZE]);
void GNSS_ParseMonHwData(GNSS_StateHandle *GNSS);
void GNSS_ParseMonVerData(GNSS_StateHandle *GNSS);
void GNSS_LoadConfig(GNSS_StateHandle *GNSS);
uint8_t GNSS_ParseBuffer(GNSS_StateHandle *GNSS);

void GNSS_GetUniqID(GNSS_StateHandle *GNSS);
void GNSS_ParseUniqID(GNSS_StateHandle *GNSS);

void GNSS_GetNavigatorData(GNSS_StateHandle *GNSS);
void GNSS_ParseNavigatorData(GNSS_StateHandle *GNSS);
void GNSS_ParseNavSatData(GNSS_StateHandle *GNSS);

void GNSS_GetPOSLLHData(GNSS_StateHandle *GNSS);
void GNSS_ParsePOSLLHData(GNSS_StateHandle *GNSS);

void GNSS_GetPVTData(GNSS_StateHandle *GNSS);
void GNSS_ParsePVTData(GNSS_StateHandle *GNSS);

void GNSS_SetMode(GNSS_StateHandle *GNSS, short gnssMode);
#endif /* INC_GNSS_H_ */



