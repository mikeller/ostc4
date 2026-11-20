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

/* Polls which are only needed by the debug pages of the main CPU (UBX-MON-HW, UBX-MON-VER, read back of the configuration) are sent only if the debug view is
 * built (HAVE_DEBUG_VIEW of configuration.h) */
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
#define GNSS_POLL_WINDOW			(16u)	/* see pollSent */
#define GNSS_POLL_PVT				(0u)
#define GNSS_POLL_SAT				(1u)
#define GNSS_POLL_MONHW				(2u)
/* Causes of a poll which is lost or disturbed, counted per poll (UBX-NAV-PVT, UBX-NAV-SAT, UBX-MON-HW, see pollLost): */
#define GNSS_CAUSE_TIMEOUT			(0u)		/* nothing recognisable was received before the end of the poll slot (no other cause was noted for the request) */
#define GNSS_CAUSE_CHECKSUM			(1u)		/* a frame of the expected message with a wrong checksum */
#define GNSS_CAUSE_MISMATCH			(2u)		/* another message than the expected one, a length which does not fit the buffer, or a reply with another length than the accepted one */
#define GNSS_CAUSE_RESYNC			(3u)		/* bytes which are not the start of a frame (once per gap), or a start byte which is not followed by the second one */
#define GNSS_CAUSE_NAK				(4u)		/* the receiver rejected the request (UBX-ACK-NAK) */
#define GNSS_CAUSE_CNT				(5u)
/* Types of UART errors of the receiver port (HAL_UART_ErrorCallback), see uartErrType: */
#define GNSS_UARTERR_PARITY			(0u)
#define GNSS_UARTERR_FRAMING		(1u)
#define GNSS_UARTERR_NOISE			(2u)
#define GNSS_UARTERR_OVERRUN		(3u)
#define GNSS_UARTERR_DMA			(4u)
#define GNSS_UARTERR_CNT			(5u)
#define GNSS_CFGREAD_PMS			(0x01u)
#define GNSS_CFGREAD_GNSS			(0x02u)
#define GNSS_CFGREAD_NAV5			(0x04u)
#define GNSS_CFGREAD_ANT			(0x08u)
#define GNSS_CFGREAD_ITFM			(0x10u)
#define GNSS_CFGREAD_HW2			(0x20u)
#define GNSS_CFGREAD_STATUS			(0x40u)
#define GNSS_CFGREAD_IO				(0x80u)
#define GNSS_CFGREAD_PRT			(0x100u)
#define GNSS_CFGREAD_ALL			(0x11Fu)	/* the read back after the power up (the periodic polls above 0x10 other than UBX-CFG-PRT have their own first time) */

/* Read only polls of UBX-CFG-ANT (antenna supervisor) and UBX-CFG-ITFM (jamming monitor): the state of each poll (GNSS_CFGPOLL_*, data_exchange.h), see cfgAntState / cfgItfmState. Besides the poll after the
 * power up they are repeated every GNSS_CFGEXTRA_PERIOD_MS (an answer which did not come is tried GNSS_CFGEXTRA_MAX_TRIES times, a rejection (UBX-ACK-NAK) is not repeated) */
#define GNSS_CFGEXTRA_PERIOD_MS		(30000u)
#define GNSS_CFGEXTRA_MAX_TRIES		(4u)

/* UBX-MON-HW2 (ADC input of the receiver) is polled every GNSS_HW2_PERIOD_MS, the first time GNSS_HW2_FIRST_MS after the power up. The reply is 36 bytes: 38 ms at 9600
 * baud, in a poll slot of one second */
#define GNSS_HW2_PERIOD_MS			(20000u)
#define GNSS_HW2_FIRST_MS			(6000u)

/* UBX-NAV-STATUS (time since the start or reset of the receiver, time to first fix, flags) is polled every GNSS_STATUS_PERIOD_MS, the first time GNSS_STATUS_FIRST_MS
 * after the power up. The reply is 24 bytes: 25 ms at 9600 baud. A reset of the receiver is detected if the time since the start (msss) is smaller than in the reply before */
#define GNSS_STATUS_PERIOD_MS		(20000u)
#define GNSS_STATUS_FIRST_MS		(8000u)

/* UBX-MON-IO (bytes and errors seen by the ports of the receiver) is polled every GNSS_IO_PERIOD_MS, the first time GNSS_IO_FIRST_MS after the power up. The reply has 20 bytes
 * per port (6 ports: 128 bytes with the frame, 133 ms at 9600 baud, in a poll slot of one second). The port of the receiver which is connected to the RTE is not known from the
 * RTE: it is port 1 (UART1, table of the port numbers in the u-blox 8 / u-blox M8 receiver description) unless that port has not sent anything and another one has, then the
 * one which has sent most bytes */
#define GNSS_IO_PERIOD_MS			(30000u)
#define GNSS_IO_FIRST_MS			(10000u)
#define GNSS_MONIO_PORT_SIZE		(20u)		/* bytes per port in UBX-MON-IO */
#define GNSS_MONIO_MAX_PORTS		(8u)		/* longer replies are not used */
#define GNSS_MONIO_DEFAULT_PORT		(1u)

/* Receive path of the receiver port (debug view): the DMA chunks which are restarted and the unread data which is discarded when the write pointer catches up with the read
 * pointer (UART_StartDMA_Receiption, in the interrupt), the longest time between two calls of the reader (UART6_HandleUART) and the first bytes of the bytes between the frames
 * of the receiver (see GNSS_NoteJunkByte). The ring holds GNSS_RX_RING_SIZE bytes of three chunks: 160 bytes or more which have not been read when a chunk is complete are
 * discarded (167 ms of the full rate of 9600 baud) */
#define GNSS_RX_LATE_GAP_MS			(100u)		/* a gap between two calls of the reader longer than this is counted (96 bytes at 9600 baud) */
#define GNSS_JUNK_BYTES				(8u)		/* bytes of a gap between frames which are kept */
#define GNSS_JUNK_MIN_LEN			(3u)		/* shorter gaps are not classified */
#define GNSS_JUNK_ASCII				(0x01u)		/* junkFlags: all kept bytes are printable characters (or CR, LF) */
#define GNSS_JUNK_DOLLAR			(0x02u)		/* junkFlags: the first byte is '$' */

/* UBX-CFG-PRT (protocols, baud rate and mode of a port of the receiver) of the port GNSS_CFGPRT_PORT is read after the power up and then every GNSS_PRT_PERIOD_MS. The request has
 * 9 bytes with the port number and the checksum (9 ms at 9600 baud), the reply 28 bytes (29 ms). The OSTC5 initializes the receiver with UBX-CFG-PRT for port 1 (configUBX:
 * UART1, 9600 baud, UBX only in and out), that is the port which is read. It is not saved (the save mask of setConfig does not include the port configuration) */
#define GNSS_PRT_PERIOD_MS			(60000u)
#define GNSS_CFGPRT_PORT			(1u)

/* Number of the periodic read only polls (see the table in uartProtocol_GNSS.c): UBX-CFG-ANT, UBX-CFG-ITFM, UBX-MON-HW2, UBX-NAV-STATUS, UBX-MON-IO, UBX-CFG-PRT */
#define GNSS_EXTRA_POLL_CNT			(6u)

/* The read back of UBX-CFG-GNSS after the power up is tried again, GNSS_CFGGNSS_RETRY_MS after the last attempt and only after a UBX-NAV-PVT reported full power (psmState 0 is
 * seen), at most GNSS_CFGGNSS_MAX_RETRIES times, if it was not read (no answer, or a reply which cannot be used); a rejection (NAK) is not repeated */
#define GNSS_CFGGNSS_RETRY_MS		(20000u)
#define GNSS_CFGGNSS_MAX_RETRIES	(5u)

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

	/* polls of the cyclic requests since the receiver was initialized or woken, index GNSS_POLL_PVT, GNSS_POLL_SAT, GNSS_POLL_MONHW. They count the most recent requests:
	 * both counts of a poll are halved when its sent counter reaches GNSS_POLL_WINDOW, so they cover the last GNSS_POLL_WINDOW / 2 to GNSS_POLL_WINDOW requests (older ones
	 * count half, a quarter, ...) and show a loss which stopped as recovered after a few polls. A request which is on its way is counted as sent, so one is missing in
	 * the answered count most of the time. Only the ratio is meaningful, not the number. */
	uint8_t pollSent[3];		/* requests which were transmitted */
	uint8_t pollAnswered[3];	/* requests which were answered with a valid frame */
	uint8_t pollLost[3][GNSS_CAUSE_CNT];	/* per poll and cause (GNSS_CAUSE_*) the lost or disturbed polls since the wake up (saturating at 255, not halved) */
	uint8_t pollAnomaly;		/* a cause other than the time out was noted for the current request */
	uint8_t uartErrType[GNSS_UARTERR_CNT];	/* UART errors of the receiver port by type (GNSS_UARTERR_*) since the RTE started (saturating at 255), uartErrors counts the callbacks */

	/* read back of the configuration after the power up (debug page, read only polls), see GNSS_ParseCfg... */
	uint16_t cfgReadPending;	/* bit mask GNSS_CFGREAD_*: the configuration messages which are polled next, one per poll slot */
	uint8_t cfgPmsValid;
	uint8_t cfgPmsSetup;		/* powerSetupValue of UBX-CFG-PMS: 0 full power, 1 balanced, 2 interval, 3 to 5 aggressive, 0xFF no predefined setup */
	uint8_t cfgGnssValid;
	uint8_t cfgGnssMask;		/* bit n: the GNSS with gnssId n is enabled in UBX-CFG-GNSS */
	uint8_t cfgGnssState;		/* GNSS_CFGPOLL_* of the last request of UBX-CFG-GNSS (NOANSWER: not answered before the end of the poll slot, OK with cfgGnssValid 0: reply which cannot be used) */
	uint8_t cfgGnssTries;		/* requests since the power up (the first one and the retries) */
	uint8_t cfgGnssLength;		/* payload length of the last reply (saturating at 255) */
	uint32_t cfgGnssTick;		/* HAL tick of the last request */
	uint8_t cfgNav5Valid;
	uint8_t cfgDynModel;		/* dynModel of UBX-CFG-NAV5 */
	uint8_t cfgAntState;		/* GNSS_CFGPOLL_* of UBX-CFG-ANT */
	uint8_t cfgAntTries;		/* requests in a row which were not answered */
	uint8_t cfgAntHave;			/* cfgAntFlags and cfgAntPins are those of a reply since the power up (they are kept while a new request is open) */
	uint16_t cfgAntFlags;		/* raw flags of UBX-CFG-ANT (bit 0 svcs, 1 scd, 2 ocd, 3 pdwnOnSCD, 4 recovery) */
	uint16_t cfgAntPins;		/* raw pins of UBX-CFG-ANT (bits 0..4 pinSwitch, 5..9 pinSCD, 10..14 pinOCD, bit 15 reconfig) */
	uint8_t cfgItfmState;		/* GNSS_CFGPOLL_* of UBX-CFG-ITFM */
	uint8_t cfgItfmTries;
	uint8_t cfgItfmHave;		/* the same for cfgItfmConfig and cfgItfmConfig2 */
	uint32_t cfgItfmConfig;		/* raw config of UBX-CFG-ITFM (bits 0..3 bbThreshold, 4..8 cwThreshold, 9..30 algorithmBits, bit 31 enable) */
	uint32_t cfgItfmConfig2;	/* raw config2 of UBX-CFG-ITFM (bits 0..11 generalBits, 12..13 antSetting, bit 14 enable2) */
	uint32_t cfgExtraTick;		/* HAL tick of the last periodic request of UBX-CFG-ANT / UBX-CFG-ITFM */
	uint32_t extraTick[GNSS_EXTRA_POLL_CNT];	/* HAL tick of the last periodic request of each of the periodic read only polls */

	/* UBX-MON-HW2 (read only poll): the ADC input of the receiver. The values are those of the last reply with the accepted length, hw2Have: there was one since the power up */
	uint8_t hw2State;			/* GNSS_CFGPOLL_* */
	uint8_t hw2Tries;
	uint8_t hw2Have;
	uint8_t hw2Length;			/* payload length of the last reply (saturating at 255) */
	int8_t ofsI;
	uint8_t magI;
	int8_t ofsQ;
	uint8_t magQ;
	uint8_t cfgSource;
	uint32_t lowLevCfg;
	uint32_t postStatus;

	/* UBX-NAV-STATUS (read only poll), the values are those of the last reply with the accepted length (statusHave: there was one since the power up) */
	uint8_t statusState;		/* GNSS_CFGPOLL_* */
	uint8_t statusTries;
	uint8_t statusHave;
	uint8_t statusLength;		/* payload length of the last reply (saturating at 255) */
	uint8_t statusFix;			/* gpsFix */
	uint8_t statusFlags;		/* flags: bit 0 gpsFixOk, 1 diffSoln, 2 wknSet, 3 towSet */
	uint8_t statusFixStat;
	uint8_t statusFlags2;
	uint32_t ttff;				/* ms */
	uint32_t msss;				/* ms since the start or reset of the receiver */
	uint8_t msssPrevValid;		/* msss of an earlier reply since the power up is known: a smaller msss is a reset */
	uint8_t resetCount;			/* resets of the receiver detected since the power up (saturating) */

	/* UBX-MON-IO (read only poll): the values of port ioPort of the last reply with a length of a whole number of ports (ioHave: there was one since the power up) */
	uint8_t ioState;			/* GNSS_CFGPOLL_* */
	uint8_t ioTries;
	uint8_t ioHave;
	uint8_t ioLength;			/* payload length of the last reply (saturating at 255) */
	uint8_t ioPort;				/* port number of the values */
	uint32_t ioRxBytes;			/* bytes received by the receiver on the port */
	uint32_t ioTxBytes;			/* bytes sent by the receiver on the port */
	uint16_t ioParityErrs;		/* 100 ms timeslots with parity errors seen by the receiver */
	uint16_t ioFramingErrs;
	uint16_t ioOverrunErrs;
	uint16_t ioBreakCond;
	/* receive path of the receiver port (debug view), since the RTE started, all saturating. rxDiscards and rxStarts are counted in the interrupt of the DMA (a count can be
	 * lost if the reader is interrupted while it counts) */
	volatile uint16_t rxDiscards;	/* chunks which were started after the unread data of the chunk was discarded (the read pointer was set to the write pointer) */
	volatile uint16_t rxStarts;		/* DMA chunks which were started */
	volatile uint8_t rxStartFails;	/* DMA chunks which could not be started (the reception is stopped until the next command) */
	uint16_t rxMaxGapMs;			/* longest time between two calls of the reader (HAL tick: it stops in STOP mode, which is only used while the unit sleeps) */
	uint8_t rxLateGaps;				/* gaps longer than GNSS_RX_LATE_GAP_MS */
	uint8_t rxReadTickValid;
	uint32_t rxReadTick;			/* HAL tick of the last call of the reader */
	uint8_t junkOpen;				/* a gap between frames is open (bytes which are not the start of a frame were received after the last frame) */
	uint8_t junkCounted;			/* the open gap was classified */
	uint8_t junkWorkLen;			/* bytes of the open gap (saturating at 255) */
	uint8_t junkWork[GNSS_JUNK_BYTES];
	uint8_t junkLen;				/* the last classified gap: length (saturating at 255), 0: none yet */
	uint8_t junkFlags;				/* GNSS_JUNK_* */
	uint8_t junkBytes[GNSS_JUNK_BYTES];	/* its first bytes */
	uint8_t junkAscii;				/* classified gaps of printable characters (text, e.g. NMEA sentences), since the RTE started, saturating */
	uint8_t junkBinary;				/* classified gaps with other bytes */
	/* UBX-CFG-PRT of port GNSS_CFGPRT_PORT (read only poll), the values are those of the last reply with the accepted length (prtHave: there was one since the power up) */
	uint8_t prtState;			/* GNSS_CFGPOLL_* */
	uint8_t prtTries;
	uint8_t prtHave;
	uint8_t prtLength;			/* payload length of the last reply (saturating at 255) */
	uint8_t prtPortId;			/* portID of the reply */
	uint32_t prtMode;			/* mode (UART: bits 6..7 character length, 9..11 parity, 12..13 stop bits) */
	uint32_t prtBaud;			/* baudRate */
	uint16_t prtInProto;		/* inProtoMask (bit 0 UBX, 1 NMEA, 2 RTCM, 5 RTCM3) */
	uint16_t prtOutProto;		/* outProtoMask (bit 0 UBX, 1 NMEA, 5 RTCM3) */
	uint8_t ident[GNSS_IDENT_SIZE];	/* identification text, see data_exchange.h */

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

/* read back of the configuration (polls of UBX-CFG-PMS, UBX-CFG-GNSS, UBX-CFG-NAV5) */
static const uint8_t getCfgPms[]={0xB5,0x62,0x06,0x86,0x00,0x00};

static const uint8_t getCfgGnss[]={0xB5,0x62,0x06,0x3E,0x00,0x00};

static const uint8_t getCfgNav5[]={0xB5,0x62,0x06,0x24,0x00,0x00};

#ifdef GNSS_DEBUG_POLLS
static const uint8_t getCfgAnt[]={0xB5,0x62,0x06,0x13,0x00,0x00};

static const uint8_t getCfgItfm[]={0xB5,0x62,0x06,0x39,0x00,0x00};

static const uint8_t getMonHw2[]={0xB5,0x62,0x0A,0x0B,0x00,0x00};

static const uint8_t getNavStatus[]={0xB5,0x62,0x01,0x03,0x00,0x00};

static const uint8_t getMonIo[]={0xB5,0x62,0x0A,0x02,0x00,0x00};

/* UBX-CFG-PRT poll with the port number as payload (GNSS_CFGPRT_PORT) */
static const uint8_t getCfgPrt[]={0xB5,0x62,0x06,0x00,0x01,0x00,GNSS_CFGPRT_PORT};
#endif

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
void GNSS_GetDebugRecord(const GNSS_StateHandle *GNSS, uint8_t index, uint8_t age_s, uint8_t textChunk, uint8_t rec[GNSS_DEBUG_REC_SIZE]);
void GNSS_ParseMonHwData(GNSS_StateHandle *GNSS);
void GNSS_ParseMonVerData(GNSS_StateHandle *GNSS);
void GNSS_ParseCfgPmsData(GNSS_StateHandle *GNSS);
void GNSS_ParseCfgGnssData(GNSS_StateHandle *GNSS);
void GNSS_ParseCfgNav5Data(GNSS_StateHandle *GNSS);
void GNSS_ParseCfgAntData(GNSS_StateHandle *GNSS);
void GNSS_ParseCfgItfmData(GNSS_StateHandle *GNSS);
void GNSS_ParseMonHw2Data(GNSS_StateHandle *GNSS);
void GNSS_ParseNavStatusData(GNSS_StateHandle *GNSS);
void GNSS_ParseMonIoData(GNSS_StateHandle *GNSS);
void GNSS_ParseCfgPrtData(GNSS_StateHandle *GNSS);
void GNSS_ResetReadOnlyPolls(GNSS_StateHandle *GNSS);
uint8_t GNSS_DebugScheduleRecord(uint8_t slot);
void GNSS_NotePollSent(GNSS_StateHandle *GNSS, uint8_t poll);
void GNSS_NotePollAnswered(GNSS_StateHandle *GNSS, uint8_t poll);
void GNSS_NotePollCause(GNSS_StateHandle *GNSS, uint8_t poll, uint8_t cause);
void GNSS_NotePollTimeout(GNSS_StateHandle *GNSS, uint8_t poll);
void GNSS_NoteUartError(GNSS_StateHandle *GNSS, uint32_t errorCode);
#ifdef GNSS_DEBUG_POLLS
void GNSS_NoteRxDiscard(GNSS_StateHandle *GNSS);
void GNSS_NoteRxStart(GNSS_StateHandle *GNSS, uint8_t started);
void GNSS_NoteReadCall(GNSS_StateHandle *GNSS, uint32_t tick);
void GNSS_NoteJunkByte(GNSS_StateHandle *GNSS, uint8_t data);
void GNSS_NoteJunkEnd(GNSS_StateHandle *GNSS);
#endif
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



