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

#ifdef ENABLE_GNSS

static uartGnssStatus_t gnssOpState = UART_GNSS_INIT;
static receiveStateGnss_t rxState = GNSSRX_READY;

void ConvertByteToHexString(uint8_t byte, char* str)
{
	uint8_t worker = 0;
	uint8_t digit = 0;
	uint8_t digitCnt = 1;

	worker = byte;
	while((worker!=0) && (digitCnt != 255))
	{
		digit = worker % 16;
		if( digit < 10)
		{
			digit += '0';
		}
		else
		{
			digit += 'A' - 10;
		}
		str[digitCnt--]= digit;
		worker = worker / 16;
	}
}

void uartGnss_Control(void)
{
	static uint32_t delayStartTick = 0;

	uint32_t tick = HAL_GetTick();

	switch (gnssOpState)
	{
		case UART_GNSS_INIT:	delayStartTick = tick;
								gnssOpState = UART_GNSS_LOAD;
				break;
		case UART_GNSS_LOAD:	if(time_elapsed_ms(delayStartTick,HAL_GetTick()) > 1000)
								{
									GNSS_LoadConfig(&GNSS_Handle);
									gnssOpState = UART_GNSS_GET_ID;
									delayStartTick = tick;
								}
				break;
		case UART_GNSS_GET_ID:	if(time_elapsed_ms(delayStartTick,HAL_GetTick()) > 250)
								{
									GNSS_GetUniqID(&GNSS_Handle);
									gnssOpState = UART_GNSS_IDLE;
									rxState = GNSSRX_RECEIVING;
									delayStartTick = tick;
								}
				break;
		case UART_GNSS_IDLE:	if(time_elapsed_ms(delayStartTick,HAL_GetTick()) > 1000)
								{
									GNSS_GetPVTData(&GNSS_Handle);
									gnssOpState = UART_GNSS_OPERATING;
									rxState = GNSSRX_RECEIVING;
									delayStartTick = tick;
								}
				break;
		case UART_GNSS_OPERATING: if(time_elapsed_ms(delayStartTick,HAL_GetTick()) > 1000)
								{
									gnssOpState = UART_GNSS_IDLE;	/* simple error handling => start next request */
									rxState = GNSSRX_READY;
								}
				break;
		default:
				break;
	}
}

void uartGnss_ProcessData(void)
{
	if(rxState == GNSSRX_RECEIVING)
	{
		if(GNSS_ParseBuffer(&GNSS_Handle))
		{
			gnssOpState = UART_GNSS_IDLE;
		}
	}
}

#endif

