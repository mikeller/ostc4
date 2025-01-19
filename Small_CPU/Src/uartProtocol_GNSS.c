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


#if defined ENABLE_GNSS || defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2

static uartGnssStatus_t gnssState = UART_GNSS_INIT;
static gnssRequest_s activeRequest = {0,0};

static receiveStateGnss_t rxState = GNSSRX_READY;
static uint8_t GnssConnected = 0;						/* Binary indicator if a sensor is connected or not */
static uint8_t writeIndex = 0;
static uint8_t dataToRead = 0;
static uint8_t ReqPowerDown = 0;

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

void UART_Gnss_SendCmd(uint8_t GnssCmd)
{
	const uint8_t* pData;
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
		case GNSSCMD_MODE_PWS:		pData = setPowerLow;
		  	  	  	  	  	  	  	txLength = sizeof(setPowerLow) / sizeof(uint8_t);
		  	break;
		case GNSSCMD_MODE_NORMAL:	pData = setPowerNormal;
				  	  	  	  	   	txLength = sizeof(setPowerNormal) / sizeof(uint8_t);
				  	break;
		case GNSSCMD_SET_CONFIG:	pData = setConfig;
						  	  	  	txLength = sizeof(setConfig) / sizeof(uint8_t);
				     break;
		default:
			break;
	}
	if(txLength != 0)
	{
		activeRequest.class = pData[2];
		activeRequest.id = pData[3];
		UART_SendCmdUbx(pData, txLength);
	}
}

void uartGnss_Control(void)
{
	static uint32_t warmupTick = 0;
	static uint8_t dataToggle = 0;
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
		case UART_GNSS_SETMODE_MOBILE: UART_Gnss_SendCmd(GNSSCMD_LOADCONF_2);
									   rxState = GNSSRX_DETECT_ACK_0;
				break;
		case UART_GNSS_PWRDOWN:		UART_Gnss_SendCmd(GNSSCMD_MODE_PWS);
									rxState = GNSSRX_DETECT_ACK_0;
				break;

		case UART_GNSS_PWRUP:		UART_Gnss_SendCmd(GNSSCMD_MODE_NORMAL);
									rxState = GNSSRX_DETECT_ACK_0;
									gnssState = UART_GNSS_PWRUP;
				break;
		case UART_GNSS_SETCONF:		UART_Gnss_SendCmd(GNSSCMD_SET_CONFIG);
									rxState = GNSSRX_DETECT_ACK_0;
				break;

		case UART_GNSS_IDLE:		if(ReqPowerDown)
									{
										UART_Gnss_SendCmd(GNSSCMD_MODE_PWS);
										gnssState = UART_GNSS_PWRDOWN;
										rxState = GNSSRX_DETECT_ACK_0;
									}
									else
									{
										if(dataToggle)
										{
											UART_Gnss_SendCmd(GNSSCMD_GET_PVT_DATA);
											gnssState = UART_GNSS_GET_PVT;
											rxState = GNSSRX_DETECT_HEADER_0;
											dataToggle = 0;
										}
										else
										{
											UART_Gnss_SendCmd(GNSSCMD_GET_NAVSAT_DATA);
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

	GNSS_Handle.uartWorkingBuffer[writeIndex++] = data;
	if((rxState >= GNSSRX_DETECT_HEADER_2) && (rxState < GNSSRX_READ_CK_A))
	{
		ck_A += data;
		ck_B += ck_A;
	}

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
		case GNSSRX_DETECT_ACK_3:		if((data == 0x01))
										{
											rxState = GNSSRX_READY;
											switch(gnssState)
											{
												case UART_GNSS_PWRUP: gnssState = UART_GNSS_IDLE;
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
												case UART_GNSS_LOADCONF_2:	gnssState = UART_GNSS_SETMODE_MOBILE;
													break;
												case UART_GNSS_SETMODE_MOBILE:	rxState = GNSSRX_DETECT_ACK_0;
																				UART_Gnss_SendCmd(GNSSCMD_MODE_NORMAL);
																				gnssState = UART_GNSS_PWRUP;
													break;
												default:
													break;
											}
											GnssConnected = 1;
										}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_2:	if(data == activeRequest.class)
			 	 	 	 	 	 	 	{
											rxState++;
			 	 	 	 	 	 	 	}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
			break;
		case GNSSRX_DETECT_HEADER_3:	if(data == activeRequest.id)
 	 	 								{
											rxState = GNSSRX_DETECT_LENGTH_0;
 	 	 								}
										else
										{
											rxState = GNSSRX_DETECT_HEADER_0;
										}
				break;
			case GNSSRX_DETECT_LENGTH_0:	rxLength = GNSS_Handle.uartWorkingBuffer[4];
											rxState = GNSSRX_DETECT_LENGTH_1;
				break;
			case GNSSRX_DETECT_LENGTH_1:    rxLength += (GNSS_Handle.uartWorkingBuffer[5] << 8);
											rxState = GNSSRX_READ_DATA;
											dataToRead = rxLength;
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
													switch(gnssState)
													{
														case UART_GNSS_GET_PVT:GNSS_ParsePVTData(&GNSS_Handle);
															break;
														case UART_GNSS_GET_SAT: GNSS_ParseNavSatData(&GNSS_Handle);
															break;
														default:
															break;
													}
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

