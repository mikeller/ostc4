/**
  ******************************************************************************
  * @file    uartProtocol_GNSS.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    30-Sep-2024
  * @brief   Interface functionality operation of GNSS devices
  *
  @verbatim


  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2024 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

#include <string.h>
#include "scheduler.h"
#include <uartProtocol_GNSS.h>
#include "uart.h"
#include "GNSS.h"
#include "configuration.h"
#include "externalInterface.h"
#include "rtc.h"


#if defined ENABLE_GNSS_INTERN || defined ENABLE_GNSS_EXTERN

static uartGnssStatus_t gnssState = UART_GNSS_INIT;
static gnssRequest_s activeRequest = {0,0};

static receiveStateGnss_t rxState = GNSSRX_READY;
static uint8_t GnssConnected = 0;						/* Binary indicator if a sensor is connected or not */
static uint16_t writeIndex = 0;
static uint16_t dataToRead = 0;
static uint8_t ReqPowerDown = 0;
#ifdef GNSS_DEBUG_POLLS
static uint8_t rxHunting = 0;		/* bytes which are not the start of a frame were seen and counted since the last start of a frame */
#endif

void uartGnss_ReqPowerDown(uint8_t request)
{
	if(GnssConnected)
	{
		ReqPowerDown = request;
	}
}

uint8_t uartGnss_isPowerDownRequested()
{
	return ReqPowerDown;
}

uartGnssStatus_t uartGnss_GetState()
{
	return gnssState;
}
void uartGnss_SetState(uartGnssStatus_t newState)
{
	gnssState = newState;
}

/* Returns 1 if the command is being transmitted, 0 if it was not (nothing to send or the transmitter is still busy) */
uint8_t UART_Gnss_SendCmd(uint8_t GnssCmd)
{
	const uint8_t* pData;
	uint8_t txLength = 0;
	uint8_t sent = 0;
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	UBX_MGA_INI_TIME_UTC_t msg_DateTime;
#ifdef ENABLE_GNSS_FIXED_POSITION
	UBX_MGA_INI_POS_LLH_t msg_Position;
#endif

	switch (GnssCmd)
	{
		case GNSSCMD_LOADCONF_0:	pData = configUBX;
									txLength = sizeof(configUBX) / sizeof(uint8_t);
				break;
		case GNSSCMD_LOADCONF_1:	pData = setNMEA410;
									txLength = sizeof(setNMEA410) / sizeof(uint8_t);
				break;
		case GNSSCMD_LOADCONF_2:	pData = setGNSS;
									txLength = sizeof(setGNSS) / sizeof(uint8_t);
				break;
		case GNSSCMD_SETMOBILE:		pData = setPortableMode;
									txLength = sizeof(setPortableMode) / sizeof(uint8_t);
				break;
		case GNSSCMD_GET_PVT_DATA:	pData = getPVTData;
									txLength = sizeof(getPVTData) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_NAV_DATA:	pData = getNavigatorData;
									txLength = sizeof(getNavigatorData) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_NAVSAT_DATA: pData = getNavSat;
									  txLength = sizeof(getNavSat) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_MONHW_DATA: pData = getMonHw;
									  txLength = sizeof(getMonHw) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_CFGPMS_DATA: pData = getCfgPms;
									  txLength = sizeof(getCfgPms) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_CFGGNSS_DATA: pData = getCfgGnss;
									  txLength = sizeof(getCfgGnss) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_CFGNAV5_DATA: pData = getCfgNav5;
									  txLength = sizeof(getCfgNav5) / sizeof(uint8_t);
			break;
#ifdef GNSS_DEBUG_POLLS
		case GNSSCMD_GET_CFGANT_DATA: pData = getCfgAnt;
									  txLength = sizeof(getCfgAnt) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_CFGITFM_DATA: pData = getCfgItfm;
									  txLength = sizeof(getCfgItfm) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_MONHW2_DATA: pData = getMonHw2;
									  txLength = sizeof(getMonHw2) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_NAVSTATUS_DATA: pData = getNavStatus;
									  txLength = sizeof(getNavStatus) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_MONIO_DATA: pData = getMonIo;
									  txLength = sizeof(getMonIo) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_CFGPRT_DATA: pData = getCfgPrt;
									  txLength = sizeof(getCfgPrt) / sizeof(uint8_t);
			break;
#endif
		case GNSSCMD_GET_MONVER_DATA: pData = getMonVer;
									  txLength = sizeof(getMonVer) / sizeof(uint8_t);
			break;
		case GNSSCMD_MODE_PWS:		pData = setPowerLow;
		  	  	  	  	  	  	  	txLength = sizeof(setPowerLow) / sizeof(uint8_t);
		  	break;
		case GNSSCMD_MODE_NORMAL:	pData = setPowerNormal;
				  	  	  	  	   	txLength = sizeof(setPowerNormal) / sizeof(uint8_t);
				  	break;
		case GNSSCMD_SET_CONFIG:	pData = setConfig;
						  	  	  	txLength = sizeof(setConfig) / sizeof(uint8_t);
				     break;
		case GNSSCMD_SETDATETIME:
		  							RTC_GetDate(&date);
		  							RTC_GetTime(&time);

									memset(&msg_DateTime, 0, sizeof(UBX_MGA_INI_TIME_UTC_t));
									msg_DateTime.header1 = 0xB5;
									msg_DateTime.header2 = 0x62;
									msg_DateTime.msgClass = 0x13;
									msg_DateTime.msgID = 0x40;
									msg_DateTime.length = sizeof(UBX_MGA_INI_TIME_UTC_t) - 6;
									msg_DateTime.type = 0x10;		/* UBX-MGA-INI-TIME_UTC */
									msg_DateTime.version = 0x00;
									msg_DateTime.year = date.Year + 2000;
									msg_DateTime.month = date.Month;
									msg_DateTime.day = date.Date;
									if((time.Hours - 2) < 0)
									{
										msg_DateTime.hour = time.Hours + 24 - 2;
									}
									else
									{
										msg_DateTime.hour = time.Hours - 2;
									}
									msg_DateTime.minute = time.Minutes;
									msg_DateTime.second =  time.Seconds;
									msg_DateTime.accuracy_seconds = 60;
									msg_DateTime.accuracy_nano = 0x0;
									msg_DateTime.flags = 0x00;
									msg_DateTime.leapsec = 0x80;

									pData = (uint8_t*)&msg_DateTime;
								  	txLength = sizeof(msg_DateTime) / sizeof(uint8_t);
						     break;
#ifdef ENABLE_GNSS_FIXED_POSITION
			case GNSSCMD_SET_POSITION:  memset(&msg_Position, 0, sizeof(UBX_MGA_INI_POS_LLH_t));
										msg_Position.header1 = 0xB5;
										msg_Position.header2 = 0x62;
										msg_Position.class_id = 0x13;
										msg_Position.msg_id = 0x40;
										msg_Position.length = sizeof(UBX_MGA_INI_POS_LLH_t) - 6;
										msg_Position.type = 0x01;		/* UBX-MGA-INI-POS_LLH */
										msg_Position.version = 0x00;
										msg_Position.latitude = (int32_t) (47.77147 * 1e7);		/* 1e-7 deg */
										msg_Position.longitude = (int32_t) (8.98647 * 1e7);		/* 1e-7 deg */
										msg_Position.altitude = (int32_t) 40000;				/* cm */
										msg_Position.accuracy = 100000;							/* cm (stddev) => 1 km */
										pData = (uint8_t*)&msg_Position;
										txLength = sizeof(msg_Position) / sizeof(uint8_t);
							break;
#endif
		default:
			break;
	}
	if(txLength != 0)
	{
		activeRequest.class = pData[2];
		activeRequest.id = pData[3];
#ifdef GNSS_DEBUG_POLLS
		rxHunting = 0;
#endif
		sent = UART_SendCmdUbx(pData, txLength);
	}
	return sent;
}

/* Sends the command for full power (UBX-CFG-PMS). If the transmitter is busy nothing happens, the next call of the control repeats it. The acknowledge is
 * evaluated by uartGnss_ProcessData (gnssState PWRUP) */
static void uartGnss_SendFullPower(void)
{
	if(UART_Gnss_SendCmd(GNSSCMD_MODE_NORMAL))
	{
		GNSS_Handle.pwrupSent = 1;
		GNSS_Handle.pwrupNak = 0;
		GNSS_Handle.pwrupSendTick = HAL_GetTick();
		rxState = GNSSRX_DETECT_ACK_0;
	}
}

/* Called after every UBX-NAV-PVT: the receiver reports psmState 0 at full power. Counts the reports of a power save state in a row for uartGnss_VerifyPower; a
 * power save state is expected while the unit is going to sleep or to dive, so it is not counted then */
static void uartGnss_EvaluatePsm(void)
{
	if(GNSS_Handle.psmState == 0)
	{
		GNSS_Handle.psmZeroTick = HAL_GetTick();
		GNSS_Handle.psmZeroValid = 1;
		GNSS_Handle.verifyBad = 0;
		GNSS_Handle.verifyResend = 0;
		GNSS_Handle.verifyTries = 0;
		GNSS_Handle.verifyGaveUp = 0;
	}
	else if(!ReqPowerDown)
	{
		if(GNSS_Handle.verifyBad < 255)
		{
			GNSS_Handle.verifyBad++;
		}
		if(GNSS_Handle.verifyBad >= GNSS_VERIFY_PVT_COUNT)
		{
			GNSS_Handle.verifyResend = 1;
		}
	}
}

/* Check of the power save state, called from the state IDLE (not while a power down is requested, not during the initialization or a power up, which end in
 * IDLE). Returns 1 if it started the power up again. The number of commands is limited (GNSS_VERIFY_MAX_RETRIES until psmState 0 was seen again), they are at
 * least GNSS_VERIFY_PVT_COUNT UBX-NAV-PVT apart (the counter starts again) or GNSS_VERIFY_SPACING_MS if no frames arrive. The wake up bytes are only sent if no valid
 * frame arrived for GNSS_FRAMES_STOPPED_MS (the receiver may be inactive); a receiver which answers is only sent the command. */
static uint8_t uartGnss_VerifyPower(void)
{
	uint32_t tick = HAL_GetTick();
	uint8_t framesStopped = GNSS_Handle.lastFrameValid && (time_elapsed_ms(GNSS_Handle.lastFrameTick, tick) > GNSS_FRAMES_STOPPED_MS);

	if(!(GNSS_Handle.verifyResend || (framesStopped && (time_elapsed_ms(GNSS_Handle.verifyActionTick, tick) > GNSS_VERIFY_SPACING_MS))))
	{
		return 0;
	}
	GNSS_Handle.verifyResend = 0;
	if(GNSS_Handle.verifyTries >= GNSS_VERIFY_MAX_RETRIES)
	{
		GNSS_Handle.verifyGaveUp = 1;
		return 0;
	}
	GNSS_Handle.verifyTries++;
	GNSS_Handle.verifyBad = 0;
	GNSS_Handle.verifyActionTick = tick;
	GNSS_Handle.pwrupSent = 0;
	GNSS_Handle.pwrupNak = 0;
	GNSS_Handle.wakeTries = 0;
	GNSS_Handle.wakeFailed = 0;
	GNSS_Handle.pwrupWake = framesStopped;
	if(framesStopped)
	{
		GNSS_Handle.wakeSent = 0;
		gnssState = UART_GNSS_WAKE;
	}
	else
	{
		gnssState = UART_GNSS_PWRUP;
		uartGnss_SendFullPower();
	}
	return 1;
}

/* Counting of the lost or disturbed cyclic polls by cause for the debug page (GNSS_CAUSE_*): the poll of the current state, if it is one of the three cyclic ones */
#ifdef GNSS_DEBUG_POLLS
static uint8_t uartGnss_PollIndex(uartGnssStatus_t state)
{
	if(state == UART_GNSS_GET_PVT)
	{
		return GNSS_POLL_PVT;
	}
	if(state == UART_GNSS_GET_SAT)
	{
		return GNSS_POLL_SAT;
	}
	if(state == UART_GNSS_GET_MONHW)
	{
		return GNSS_POLL_MONHW;
	}
	return 0xFFu;
}

static void uartGnss_NoteCause(uint8_t cause)
{
	uint8_t poll = uartGnss_PollIndex(gnssState);

	if(poll != 0xFFu)
	{
		GNSS_NotePollCause(&GNSS_Handle, poll, cause);
	}
}

void uartGnss_NoteTimeout(uartGnssStatus_t state)
{
	uint8_t poll = uartGnss_PollIndex(state);

	if(poll != 0xFFu)
	{
		GNSS_NotePollTimeout(&GNSS_Handle, poll);
	}
}
#else
#define uartGnss_NoteCause(cause)	((void)0)
#endif

/* The periodic read only polls (debug view): the state of each poll (GNSS_CFGPOLL_*) and the number of requests in a row which were not answered are kept in
 * GNSS_Handle. A request which was not answered is tried again after its period, at most GNSS_CFGEXTRA_MAX_TRIES times in a row, a rejected request (UBX-ACK-NAK)
 * is not repeated. Every poll costs one poll slot (the cyclic polls are delayed by that), the reply of each is short. */
#ifdef GNSS_DEBUG_POLLS
typedef struct
{
	uint16_t pendingBit;			/* GNSS_CFGREAD_*: the poll is due */
	uint8_t cmd;					/* GNSSCMD_GET_*: the request */
	uartGnssStatus_t state;			/* gnssState while the reply is awaited */
	uint8_t* pPollState;			/* GNSS_CFGPOLL_* of the poll */
	uint8_t* pTries;				/* requests in a row which were not answered */
	uint32_t period_ms;				/* time between the requests */
	uint32_t first_ms;				/* the first request is this long after the power up (0: with the read back of the configuration, the next one a period later) */
} uartGnssExtraPoll_t;

static const uartGnssExtraPoll_t extraPoll[GNSS_EXTRA_POLL_CNT] =
{
	{ GNSS_CFGREAD_ANT, GNSSCMD_GET_CFGANT_DATA, UART_GNSS_GET_CFGANT, &GNSS_Handle.cfgAntState, &GNSS_Handle.cfgAntTries, GNSS_CFGEXTRA_PERIOD_MS, 0 },
	{ GNSS_CFGREAD_ITFM, GNSSCMD_GET_CFGITFM_DATA, UART_GNSS_GET_CFGITFM, &GNSS_Handle.cfgItfmState, &GNSS_Handle.cfgItfmTries, GNSS_CFGEXTRA_PERIOD_MS, 0 },
	{ GNSS_CFGREAD_HW2, GNSSCMD_GET_MONHW2_DATA, UART_GNSS_GET_MONHW2, &GNSS_Handle.hw2State, &GNSS_Handle.hw2Tries, GNSS_HW2_PERIOD_MS, GNSS_HW2_FIRST_MS },
	{ GNSS_CFGREAD_STATUS, GNSSCMD_GET_NAVSTATUS_DATA, UART_GNSS_GET_NAVSTATUS, &GNSS_Handle.statusState, &GNSS_Handle.statusTries, GNSS_STATUS_PERIOD_MS, GNSS_STATUS_FIRST_MS },
	{ GNSS_CFGREAD_IO, GNSSCMD_GET_MONIO_DATA, UART_GNSS_GET_MONIO, &GNSS_Handle.ioState, &GNSS_Handle.ioTries, GNSS_IO_PERIOD_MS, GNSS_IO_FIRST_MS },
	{ GNSS_CFGREAD_PRT, GNSSCMD_GET_CFGPRT_DATA, UART_GNSS_GET_CFGPRT, &GNSS_Handle.prtState, &GNSS_Handle.prtTries, GNSS_PRT_PERIOD_MS, 0 },
};

static const uartGnssExtraPoll_t* uartGnss_FindExtraPoll(uartGnssStatus_t state)
{
	for(uint8_t i = 0; i < GNSS_EXTRA_POLL_CNT; i++)
	{
		if(extraPoll[i].state == state)
		{
			return &extraPoll[i];
		}
	}
	return 0;
}

/* The state is one in which the reply of a read only poll is awaited */
static uint8_t uartGnss_IsPollState(uartGnssStatus_t state)
{
	return (state == UART_GNSS_GET_PVT) || (state == UART_GNSS_GET_SAT) || (state == UART_GNSS_GET_MONHW) || (state == UART_GNSS_GET_MONVER)
			|| (state == UART_GNSS_GET_CFGPMS) || (state == UART_GNSS_GET_CFGGNSS) || (state == UART_GNSS_GET_CFGNAV5) || (uartGnss_FindExtraPoll(state) != 0);
}

/* The periodic polls start again with the power up: the first request of each is first_ms later */
static void uartGnss_RestartExtraPolls(void)
{
	uint32_t tick = HAL_GetTick();

	for(uint8_t i = 0; i < GNSS_EXTRA_POLL_CNT; i++)
	{
		GNSS_Handle.extraTick[i] = (extraPoll[i].first_ms != 0) ? (tick - (extraPoll[i].period_ms - extraPoll[i].first_ms)) : tick;
	}
}

/* Called from the state IDLE (a poll is over then): a request which did not get an answer is marked, and the requests which are due are queued */
static void uartGnss_ExtraPollHousekeeping(void)
{
	uint32_t tick = HAL_GetTick();

	if(GNSS_Handle.cfgGnssState == GNSS_CFGPOLL_POLLING)
	{
		GNSS_Handle.cfgGnssState = GNSS_CFGPOLL_NOANSWER;
	}
	if((!GNSS_Handle.cfgGnssValid) && (GNSS_Handle.cfgGnssState != GNSS_CFGPOLL_NONE) && (GNSS_Handle.cfgGnssState != GNSS_CFGPOLL_NAK) && GNSS_Handle.psmZeroValid
			&& (GNSS_Handle.cfgGnssTries <= GNSS_CFGGNSS_MAX_RETRIES) && (time_elapsed_ms(GNSS_Handle.cfgGnssTick, tick) >= GNSS_CFGGNSS_RETRY_MS))
	{
		GNSS_Handle.cfgReadPending |= GNSS_CFGREAD_GNSS;
	}
	for(uint8_t i = 0; i < GNSS_EXTRA_POLL_CNT; i++)
	{
		if(*extraPoll[i].pPollState == GNSS_CFGPOLL_POLLING)
		{
			*extraPoll[i].pPollState = GNSS_CFGPOLL_NOANSWER;
			(*extraPoll[i].pTries)++;
		}
		if(time_elapsed_ms(GNSS_Handle.extraTick[i], tick) >= extraPoll[i].period_ms)
		{
			GNSS_Handle.extraTick[i] = tick;
			if((*extraPoll[i].pPollState != GNSS_CFGPOLL_NAK) && (*extraPoll[i].pTries < GNSS_CFGEXTRA_MAX_TRIES))
			{
				GNSS_Handle.cfgReadPending |= extraPoll[i].pendingBit;
			}
		}
	}
}

/* Polls the next message of GNSS_Handle.cfgReadPending (read only, one per call, first the read back of the configuration in the order PMS, GNSS, NAV5, then the
 * periodic polls). Returns 1 if a poll was started. A request which cannot be sent or is not answered is not repeated at once: the value stays invalid */
static uint8_t uartGnss_StartConfigRead(void)
{
	static const uint8_t order[3] = { GNSS_CFGREAD_PMS, GNSS_CFGREAD_GNSS, GNSS_CFGREAD_NAV5 };
	static const uint8_t cmd[3] = { GNSSCMD_GET_CFGPMS_DATA, GNSSCMD_GET_CFGGNSS_DATA, GNSSCMD_GET_CFGNAV5_DATA };
	static const uartGnssStatus_t state[3] = { UART_GNSS_GET_CFGPMS, UART_GNSS_GET_CFGGNSS, UART_GNSS_GET_CFGNAV5 };

	for(uint8_t i = 0; i < 3; i++)
	{
		if(GNSS_Handle.cfgReadPending & order[i])
		{
			GNSS_Handle.cfgReadPending &= (uint16_t)~order[i];
			if(order[i] == GNSS_CFGREAD_GNSS)
			{
				GNSS_Handle.cfgGnssState = GNSS_CFGPOLL_POLLING;
				GNSS_Handle.cfgGnssTick = HAL_GetTick();
				if(GNSS_Handle.cfgGnssTries < 255)
				{
					GNSS_Handle.cfgGnssTries++;
				}
			}
			UART_Gnss_SendCmd(cmd[i]);
			gnssState = state[i];
			rxState = GNSSRX_DETECT_HEADER_0;
			return 1;
		}
	}
	for(uint8_t i = 0; i < GNSS_EXTRA_POLL_CNT; i++)
	{
		if(GNSS_Handle.cfgReadPending & extraPoll[i].pendingBit)
		{
			GNSS_Handle.cfgReadPending &= (uint16_t)~extraPoll[i].pendingBit;
			*extraPoll[i].pPollState = GNSS_CFGPOLL_POLLING;
			UART_Gnss_SendCmd(extraPoll[i].cmd);
			gnssState = extraPoll[i].state;
			rxState = GNSSRX_DETECT_HEADER_0;
			return 1;
		}
	}
	GNSS_Handle.cfgReadPending = 0;
	return 0;
}
#endif

/* The initialization continues with the full power command which is sent at once; the sequence of a wake up is started in UART6_HandleUART */
static void uartGnss_StartPowerUpAfterInit(void)
{
	GNSS_Handle.pwrupWake = 0;
	GNSS_Handle.pwrupSent = 0;
	GNSS_Handle.wakeTries = 0;
	GNSS_Handle.wakeFailed = 0;
	gnssState = UART_GNSS_PWRUP;
	uartGnss_SendFullPower();
}

void uartGnss_Control(void)
{
	static uint32_t warmupTick = 0;
	static uint8_t dataToggle = 0;
#ifdef GNSS_DEBUG_POLLS
	static uint8_t monPollCount = 0;
	static uint8_t monToggle = 0;
#endif
	uint8_t activeSensor = 0;
	sUartComCtrl* pUartCtrl = UART_GetGnssCtrl();

	if(pUartCtrl == &Uart1Ctrl)
	{
		activeSensor = externalInterface_GetActiveUartSensor();
		gnssState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);
	}

	switch (gnssState)
	{
		case UART_GNSS_INIT:  		gnssState = UART_GNSS_WARMUP;
									warmupTick =  HAL_GetTick();
									UART_clearRxBuffer(pUartCtrl);
				break;
		case UART_GNSS_WARMUP:		if(time_elapsed_ms(warmupTick,HAL_GetTick()) > 1000)
									{
										gnssState = UART_GNSS_LOADCONF_0;
									}
				break;
		case UART_GNSS_LOADCONF_0:	UART_Gnss_SendCmd(GNSSCMD_LOADCONF_0);
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_LOADCONF_1:	UART_Gnss_SendCmd(GNSSCMD_LOADCONF_1);
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_LOADCONF_2:	UART_Gnss_SendCmd(GNSSCMD_LOADCONF_2);
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_SETMODE_MOBILE: UART_Gnss_SendCmd(GNSSCMD_SETMOBILE);
									   rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_SETDATE_TIME: UART_Gnss_SendCmd(GNSSCMD_SETDATETIME);
		   	   	   	   	   	   	   	// rxState = GNSSRX_DETECT_ACK_0;	/* aiding function will not acknoledge receiption (until config to do so) */
#ifdef ENABLE_GNSS_FIXED_POSITION
									gnssState = UART_GNSS_SET_POSITION;
#else
									gnssState = UART_GNSS_PWRUP;
#endif
				break;
#ifdef ENABLE_GNSS_FIXED_POSITION
		case UART_GNSS_SET_POSITION:  UART_Gnss_SendCmd(GNSSCMD_SET_POSITION);
	   	   	   	   					 //  rxState = GNSSRX_DETECT_ACK_0;	/* aiding function will not acknoledge receiption (until config to do so) */
										gnssState = UART_GNSS_PWRUP;
				break;
#endif
		case UART_GNSS_PWRDOWN:		UART_Gnss_SendCmd(GNSSCMD_MODE_PWS);
									rxState = GNSSRX_DETECT_ACK_0;
				break;

		case UART_GNSS_PWRUP:		if(pUartCtrl == &Uart1Ctrl)		/* external receiver: sent with every call as before */
									{
										UART_Gnss_SendCmd(GNSSCMD_MODE_NORMAL);
										rxState = GNSSRX_DETECT_ACK_0;
										gnssState = UART_GNSS_PWRUP;
									}
									else if(!GNSS_Handle.pwrupSent)
									{
										uartGnss_SendFullPower();
									}
				break;
		case UART_GNSS_SETCONF:		UART_Gnss_SendCmd(GNSSCMD_SET_CONFIG);
									rxState = GNSSRX_DETECT_ACK_0;
				break;

		case UART_GNSS_IDLE:
#ifdef GNSS_DEBUG_POLLS
									uartGnss_ExtraPollHousekeeping();
#endif
									if(ReqPowerDown)
									{
										GNSS_Handle.verifyResend = 0;
										UART_Gnss_SendCmd(GNSSCMD_MODE_PWS);
										gnssState = UART_GNSS_PWRDOWN;
										rxState = GNSSRX_DETECT_ACK_0;
									}
									else if((pUartCtrl != &Uart1Ctrl) && uartGnss_VerifyPower())
									{
										/* the full power command is sent again, the polls continue after the acknowledge */
									}
#ifdef GNSS_DEBUG_POLLS
									else if(GNSS_Handle.cfgReadPending && uartGnss_StartConfigRead())
									{
										/* a configuration message is polled instead of a cyclic request (read only) */
									}
#endif
									else
									{
										/* Every GNSS_MON_POLL_INTERVAL-th poll (and the first ones after the initialization) is a read only poll of UBX-MON-VER (until it was
										 * received, at most GNSS_MONVER_MAX_REQUESTS times) or UBX-MON-HW (alternating as long as the version is requested) instead of NAV-SAT / NAV-PVT.
										 * The answers are about 70 bytes (HW) and about 250 bytes (VER), the same as a NAV-SAT answer with 20 satellites, so a slot is not longer than before. */
#ifdef GNSS_DEBUG_POLLS
										monPollCount++;
										if((GNSS_Handle.monStartup != 0) || (monPollCount >= GNSS_MON_POLL_INTERVAL))
										{
											monPollCount = 0;
											if(GNSS_Handle.monStartup != 0)
											{
												GNSS_Handle.monStartup--;
											}
											if((!GNSS_Handle.identValid) && (GNSS_Handle.monVerRequests < GNSS_MONVER_MAX_REQUESTS) && (monToggle == 0))
											{
												UART_Gnss_SendCmd(GNSSCMD_GET_MONVER_DATA);
												gnssState = UART_GNSS_GET_MONVER;
												GNSS_Handle.monVerRequests++;
											}
											else
											{
												if(UART_Gnss_SendCmd(GNSSCMD_GET_MONHW_DATA))
												{
													GNSS_NotePollSent(&GNSS_Handle, GNSS_POLL_MONHW);
												}
												gnssState = UART_GNSS_GET_MONHW;
											}
											monToggle ^= 1u;
											rxState = GNSSRX_DETECT_HEADER_0;
										}
										else
#endif
										if(dataToggle)
										{
											if(UART_Gnss_SendCmd(GNSSCMD_GET_PVT_DATA))
											{
												GNSS_NotePollSent(&GNSS_Handle, GNSS_POLL_PVT);
											}
											gnssState = UART_GNSS_GET_PVT;
											rxState = GNSSRX_DETECT_HEADER_0;
											dataToggle = 0;
										}
										else
										{
											if(UART_Gnss_SendCmd(GNSSCMD_GET_NAVSAT_DATA))
											{
												GNSS_NotePollSent(&GNSS_Handle, GNSS_POLL_SAT);
											}
											gnssState = UART_GNSS_GET_SAT;
											rxState = GNSSRX_DETECT_HEADER_0;
											dataToggle = 1;
										}
									}
				break;
		default:
				break;
	}
	if(pUartCtrl == &Uart1Ctrl)
	{
		externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,gnssState);
	}

}


void uartGnss_ProcessData(uint8_t data)
{
	static uint16_t rxLength = 0;
	static uint8_t ck_A = 0;
	static uint8_t ck_B = 0;
	static uint8_t ck_A_Ref = 0;
	static uint8_t ck_B_Ref = 0;
	uint8_t activeSensor = 0;

	sUartComCtrl* pUartCtrl = UART_GetGnssCtrl();

	if(pUartCtrl == &Uart1Ctrl)
	{
		activeSensor = externalInterface_GetActiveUartSensor();
		gnssState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);
	}

	if(writeIndex < sizeof(GNSS_Handle.uartWorkingBuffer))		/* data which does not fit is skipped. The start of a frame resets the index */
	{
		GNSS_Handle.uartWorkingBuffer[writeIndex++] = data;
	}
	if((rxState >= GNSSRX_DETECT_HEADER_2) && (rxState < GNSSRX_READ_CK_A))
	{
		ck_A += data;
		ck_B += ck_A;
	}

#ifdef GNSS_DEBUG_POLLS
	if((rxState == GNSSRX_DETECT_HEADER_0) || (rxState == GNSSRX_DETECT_ACK_0))		/* between two frames: bytes which are not the start of a frame are kept for the debug page */
	{
		if(data == 0xB5)
		{
			GNSS_NoteJunkEnd(&GNSS_Handle);
		}
		else
		{
			GNSS_NoteJunkByte(&GNSS_Handle, data);
		}
	}
#endif
	switch(rxState)
	{
		case GNSSRX_DETECT_ACK_0:
		case GNSSRX_DETECT_HEADER_0:	if(data == 0xB5)
										{
											writeIndex = 0;
											memset(GNSS_Handle.uartWorkingBuffer,0xff, sizeof(GNSS_Handle.uartWorkingBuffer));
											GNSS_Handle.uartWorkingBuffer[writeIndex++] = data;
											rxState++;
											ck_A = 0;
											ck_B = 0;
#ifdef GNSS_DEBUG_POLLS
											rxHunting = 0;
#endif
										}
#ifdef GNSS_DEBUG_POLLS
										else if((rxState == GNSSRX_DETECT_HEADER_0) && (!rxHunting))
										{
											rxHunting = 1;		/* once per gap of bytes which are not the start of a frame */
											uartGnss_NoteCause(GNSS_CAUSE_RESYNC);
										}
#endif
			break;
		case GNSSRX_DETECT_ACK_1:
		case GNSSRX_DETECT_HEADER_1:	if(data == 0x62)
			 	 	 	 	 	 	 	{
											rxState++;
			 	 	 	 	 	 	 	}
										else
										{
											if(rxState == GNSSRX_DETECT_HEADER_1)
											{
												uartGnss_NoteCause(GNSS_CAUSE_RESYNC);		/* the start byte was not followed by the second one */
											}
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_ACK_2:		if(data == 0x05)
										{
											rxState++;
										}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_ACK_3:		if((data == 0x01))
										{
											rxState = GNSSRX_READY;
											switch(gnssState)
											{
												case UART_GNSS_PWRUP: gnssState = UART_GNSS_IDLE;		/* the receiver accepted the full power command */
																	  GNSS_Handle.pwrupSent = 0;
																	  GNSS_Handle.wakeFailed = 0;
																	  GNSS_Handle.verifyBad = 0;		/* reports from before the command do not count */
#ifdef GNSS_DEBUG_POLLS
																	  GNSS_Handle.cfgReadPending = GNSS_CFGREAD_ALL;		/* read back what the receiver has now */
																	  GNSS_ResetReadOnlyPolls(&GNSS_Handle);
																	  uartGnss_RestartExtraPolls();
#endif
													break;
												case UART_GNSS_PWRDOWN:	rxState = GNSSRX_DETECT_ACK_0;
																		UART_Gnss_SendCmd(GNSSCMD_SET_CONFIG);
																		gnssState = UART_GNSS_SETCONF;
													break;
												case UART_GNSS_SETCONF:	gnssState = UART_GNSS_INACTIVE;
													break;
												case UART_GNSS_LOADCONF_0:
												case UART_GNSS_LOADCONF_1:	gnssState++;
													break;
												case UART_GNSS_LOADCONF_2:
													gnssState = UART_GNSS_SETMODE_MOBILE;
													break;
												case UART_GNSS_SETMODE_MOBILE:
#ifdef ENABLE_GNSS_TIME_INIT
																				gnssState = UART_GNSS_SETDATE_TIME;
#elif defined ENABLE_GNSS_FIXED_POSITION
																				gnssState = UART_GNSS_SET_POSITION;
#else
																				uartGnss_StartPowerUpAfterInit();
#endif
													break;
												case UART_GNSS_SET_POSITION:	uartGnss_StartPowerUpAfterInit();
													break;
												case UART_GNSS_SETDATE_TIME: uartGnss_StartPowerUpAfterInit();
												default:
													break;
											}
#ifdef GNSS_DEBUG_POLLS
											if(uartGnss_IsPollState(gnssState))
											{
												rxState = GNSSRX_DETECT_HEADER_0;		/* the acknowledge of a poll: the reply is still to come (or was received) */
											}
#endif
											GnssConnected = 1;
											GNSS_NoteFrameReceived(&GNSS_Handle);
										}
										else
										{
											if(data == 0x00)		/* UBX-ACK-NAK: the receiver rejected a command, counted */
											{
												if(GNSS_Handle.nakCount < 255)
												{
													GNSS_Handle.nakCount++;
												}
												if(gnssState == UART_GNSS_PWRUP)
												{
													GNSS_Handle.pwrupNak = 1;		/* the power up sequence is repeated */
												}
												uartGnss_NoteCause(GNSS_CAUSE_NAK);
#ifdef GNSS_DEBUG_POLLS
												if(uartGnss_IsPollState(gnssState))		/* the receiver does not support the message: the poll is over */
												{
													const uartGnssExtraPoll_t* pExtra = uartGnss_FindExtraPoll(gnssState);

													if(pExtra != 0)
													{
														*pExtra->pPollState = GNSS_CFGPOLL_NAK;
													}
													else if(gnssState == UART_GNSS_GET_CFGGNSS)
													{
														GNSS_Handle.cfgGnssState = GNSS_CFGPOLL_NAK;
													}
													gnssState = UART_GNSS_IDLE;
												}
#endif
											}
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_2:	if(data == activeRequest.class)
			 	 	 	 	 	 	 	{
											rxState++;
			 	 	 	 	 	 	 	}
#ifdef GNSS_DEBUG_POLLS
										else if((data == 0x05) && uartGnss_IsPollState(gnssState))
										{
											rxState = GNSSRX_DETECT_ACK_3;		/* UBX-ACK-NAK instead of the reply: the receiver does not support the message */
										}
#endif
										else
										{
											uartGnss_NoteCause(GNSS_CAUSE_MISMATCH);		/* another class than the one of the request */
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_3:	if(data == activeRequest.id)
 	 	 								{
											rxState = GNSSRX_DETECT_LENGTH_0;
 	 	 								}
										else
										{
											uartGnss_NoteCause(GNSS_CAUSE_MISMATCH);		/* another message of the class of the request */
											rxState = GNSSRX_DETECT_HEADER_0;
										}
				break;
			case GNSSRX_DETECT_LENGTH_0:	rxLength = GNSS_Handle.uartWorkingBuffer[4];
											rxState = GNSSRX_DETECT_LENGTH_1;
				break;
			case GNSSRX_DETECT_LENGTH_1:    rxLength += (GNSS_Handle.uartWorkingBuffer[5] << 8);
											if((6u + (uint32_t)rxLength + 2u) > sizeof(GNSS_Handle.uartWorkingBuffer))	/* frame (header, payload, checksum) does not fit into the buffer => reject it */
											{
												uartGnss_NoteCause(GNSS_CAUSE_MISMATCH);
												rxState = GNSSRX_DETECT_HEADER_0;
												gnssState = UART_GNSS_IDLE;
											}
											else
											{
												rxState = GNSSRX_READ_DATA;
												dataToRead = rxLength;
											}
				break;
			case GNSSRX_READ_DATA:				if(dataToRead > 0)
												{
													dataToRead--;
												}
												if(dataToRead == 0)
												{
													rxState = GNSSRX_READ_CK_A;
												}
				break;
			case GNSSRX_READ_CK_A:				ck_A_Ref = data;
												rxState++;
				break;
			case GNSSRX_READ_CK_B:				ck_B_Ref = data;
												if((ck_A_Ref == ck_A) && (ck_B_Ref == ck_B))
												{
													GNSS_NoteFrameReceived(&GNSS_Handle);
													switch(gnssState)
													{
														case UART_GNSS_GET_PVT:GNSS_ParsePVTData(&GNSS_Handle);
																			   uartGnss_EvaluatePsm();
																			   GNSS_NotePollAnswered(&GNSS_Handle, GNSS_POLL_PVT);
															break;
														case UART_GNSS_GET_SAT: GNSS_ParseNavSatData(&GNSS_Handle);
																			   GNSS_NotePollAnswered(&GNSS_Handle, GNSS_POLL_SAT);
															break;
#ifdef GNSS_DEBUG_POLLS
														case UART_GNSS_GET_MONHW: GNSS_ParseMonHwData(&GNSS_Handle);
																				  GNSS_NotePollAnswered(&GNSS_Handle, GNSS_POLL_MONHW);
															break;
#ifdef GNSS_DEBUG_POLLS
														case UART_GNSS_GET_CFGPMS: GNSS_ParseCfgPmsData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_CFGGNSS: GNSS_ParseCfgGnssData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_CFGNAV5: GNSS_ParseCfgNav5Data(&GNSS_Handle);
															break;
#endif
														case UART_GNSS_GET_CFGANT: GNSS_ParseCfgAntData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_CFGITFM: GNSS_ParseCfgItfmData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_MONHW2: GNSS_ParseMonHw2Data(&GNSS_Handle);
															break;
														case UART_GNSS_GET_NAVSTATUS: GNSS_ParseNavStatusData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_MONIO: GNSS_ParseMonIoData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_CFGPRT: GNSS_ParseCfgPrtData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_MONVER: GNSS_ParseMonVerData(&GNSS_Handle);
															break;
#endif
														default:
															break;
													}
												}
												else
												{
													uartGnss_NoteCause(GNSS_CAUSE_CHECKSUM);
												}
												rxState = GNSSRX_DETECT_HEADER_0;
												gnssState = UART_GNSS_IDLE;
				break;

		default:	rxState = GNSSRX_READY;
			break;
	}
	if(pUartCtrl == &Uart1Ctrl)
	{
		externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,gnssState);
	}
}

uint8_t uartGnss_isSensorConnected()
{
	return GnssConnected;
}

#endif

