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

#if defined ENABLE_GNSS || defined ENABLE_GNSS_SUPPORT

static receiveStateGnss_t rxState = GNSSRX_READY;
static uint8_t GnssConnected = 0;						/* Binary indicator if a sensor is connected or not */

static uint8_t writeIndex = 0;

static uint8_t dataToRead = 0;

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


void uartGnss_SendCmd(uint8_t GnssCmd)
{
	uint8_t* pData;
	uint8_t txLength = 0;

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
		case GNSSCMD_GET_PVT_DATA:	pData = getPVTData;
									txLength = sizeof(getPVTData) / sizeof(uint8_t);
			break;
		case GNSSCMD_GET_NAV_DATA:	pData = getNavigatorData;
									txLength = sizeof(getNavigatorData) / sizeof(uint8_t);
			break;

		default:
			break;
	}
	if(txLength != 0)
	{
		UART_SendCmdUbx(pData, txLength);
	}
}

void uartGnss_Control(void)
{
	static uint32_t warmupTick = 0;

	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartGnssStatus_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	switch (localComState)
	{
		case UART_GNSS_INIT:  		localComState = UART_GNSS_WARMUP;
									warmupTick =  HAL_GetTick();
									UART_clearRxBuffer();
				break;
		case UART_GNSS_WARMUP:		if(time_elapsed_ms(warmupTick,HAL_GetTick()) > 1000)
									{
										localComState = UART_GNSS_LOADCONF_0;
									}
				break;
		case UART_GNSS_LOADCONF_0:	uartGnss_SendCmd(GNSSCMD_LOADCONF_0);
									localComState = UART_GNSS_LOADCONF_1;
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_LOADCONF_1:	uartGnss_SendCmd(GNSSCMD_LOADCONF_1);
									localComState = UART_GNSS_LOADCONF_2;
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_LOADCONF_2:	uartGnss_SendCmd(GNSSCMD_LOADCONF_2);
									localComState = UART_GNSS_IDLE;
									rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_IDLE:		uartGnss_SendCmd(GNSSCMD_GET_PVT_DATA);
									localComState = UART_GNSS_GET_PVT;
									rxState = GNSSRX_DETECT_HEADER_0;
				break;
		default:
				break;
	}

	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);

}

void uartGnss_ProcessData(uint8_t data)
{
	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	GNSS_Handle.uartWorkingBuffer[writeIndex++] = data;
	switch(rxState)
	{
		case GNSSRX_DETECT_ACK_0:
		case GNSSRX_DETECT_HEADER_0:	if(data == 0xB5)
										{
											writeIndex = 0;
											memset(GNSS_Handle.uartWorkingBuffer,0, sizeof(GNSS_Handle.uartWorkingBuffer));
											GNSS_Handle.uartWorkingBuffer[writeIndex++] = data;
											rxState++;
										}
			break;
		case GNSSRX_DETECT_ACK_1:
		case GNSSRX_DETECT_HEADER_1:	if(data == 0x62)
			 	 	 	 	 	 	 	{
											rxState++;
			 	 	 	 	 	 	 	}
										else
										{
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
		case GNSSRX_DETECT_ACK_3:		if((data == 0x01) || (data == 0x00))
										{
											GnssConnected = 1;
											rxState = GNSSRX_READY;
										}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_2:	if(data == 0x01)
			 	 	 	 	 	 	 	{
											rxState++;
			 	 	 	 	 	 	 	}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_3:
										switch(data)
										{
											case 0x21: rxState = GNSSRX_READ_NAV_DATA;
														dataToRead = 20;
												break;
											case 0x07:	rxState = GNSSRX_READ_PVT_DATA;
														dataToRead = 92;
												break;
											case 0x02:  rxState = GNSSRX_READ_POSLLH_DATA;
												break;
											default: 	rxState = GNSSRX_DETECT_HEADER_0;
												break;
										}
			break;
			case GNSSRX_READ_NAV_DATA:
			case GNSSRX_READ_PVT_DATA:
			case GNSSRX_READ_POSLLH_DATA:		if(dataToRead > 0)
												{
													dataToRead--;
												}
												else
												{
														switch(rxState)
														{
															case GNSSRX_READ_NAV_DATA: GNSS_ParseNavigatorData(&GNSS_Handle);
																break;
															case GNSSRX_READ_PVT_DATA:	GNSS_ParsePVTData(&GNSS_Handle);
																break;
															case GNSSRX_READ_POSLLH_DATA:  GNSS_ParsePOSLLHData(&GNSS_Handle);
																break;
															default: 	rxState = GNSSRX_DETECT_HEADER_0;
																break;
														}
														rxState = GNSSRX_DETECT_HEADER_0;
														externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,UART_GNSS_IDLE);
												}
				break;
		default:	rxState = GNSSRX_READY;
			break;
	}
}

uint8_t uartGnss_isSensorConnected()
{
	return GnssConnected;
}

#endif

