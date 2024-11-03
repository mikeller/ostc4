/**
  ******************************************************************************
  * @file    uartProtocol_O2.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    16-Jun-2023
  * @brief   Interface functionality to external, UART based O2 sensors
  *
  @verbatim


  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2023 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

#include <string.h>
#include "uart.h"
#include "uartProtocol_O2.h"
#include "externalInterface.h"


const  uint8_t errorStr[] = "#ERRO";
static uint32_t lastReceiveTick = 0;
static uartO2RxState_t rxState = O2RX_IDLE;
static uint8_t digO2Connected = 0;						/* Binary indicator if a sensor is connected or not */
static SSensorDataDiveO2 tmpSensorDataDiveO2;			/* intermediate storage for additional sensor data */

static uint8_t activeSensor = 0;
static uint8_t respondErrorDetected = 0;

void uartO2_InitData()
{
	  digO2Connected = 0;
}

void uartO2_SetupCmd(uint8_t O2State, uint8_t *cmdString, uint8_t *cmdLength)
{
	switch (O2State)
	{
		case UART_O2_CHECK:		*cmdLength = snprintf((char*)cmdString, 10, "#LOGO");
			break;
		case UART_O2_REQ_INFO: 	*cmdLength = snprintf((char*)cmdString, 10, "#VERS");
					break;
		case UART_O2_REQ_ID: 	*cmdLength = snprintf((char*)cmdString, 10, "#IDNR");
			break;
		case UART_O2_REQ_O2: 	*cmdLength = snprintf((char*)cmdString, 10, "#DOXY");
			break;
		case UART_O2_REQ_RAW:	*cmdLength = snprintf((char*)cmdString, 10, "#DRAW");
			break;
		default: *cmdLength = 0;
			break;
	}
	if(*cmdLength != 0)
	{
		cmdString[*cmdLength] = 0x0D;
		*cmdLength = *cmdLength + 1;
		cmdString[*cmdLength] = 0x0A;
		*cmdLength = *cmdLength + 1;
		cmdString[*cmdLength] = 0;
		*cmdLength = *cmdLength + 1;
	}
}

static uint8_t cmdLength = 0;
static uint8_t cmdString[10];

void uartO2_Control(void)
{
	static uint8_t lastComState = 0;
	static uint8_t lastActiveSensor = 0xFF;

	uint8_t activeSensor = externalInterface_GetActiveUartSensor();

	uartO2Status_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);
	externalInterface_GetSensorData(activeSensor + EXT_INTERFACE_MUX_OFFSET, (uint8_t*)&tmpSensorDataDiveO2);


	if(lastActiveSensor != activeSensor)
	{
		lastActiveSensor = activeSensor;
		if(localComState != UART_O2_ERROR)
		{
			lastComState = localComState;
		}
		else
		{
			lastComState = UART_O2_IDLE;
		}
		if(localComState == UART_O2_CHECK)
		{
			localComState = UART_O2_IDLE;
		}
		UART_FlushRxBuffer();
	}

	if(localComState == UART_O2_INIT)
	{
		memset((char*) &tmpSensorDataDiveO2, 0, sizeof(tmpSensorDataDiveO2));
		externalInterface_SetSensorData(0xFF,(uint8_t*)&tmpSensorDataDiveO2);

		localComState = UART_O2_CHECK;
		lastComState = UART_O2_CHECK;
		UART_clearRxBuffer();
		uartO2_SetupCmd(localComState,cmdString,&cmdLength);
		UART_SendCmdString(cmdString);
		rxState = O2RX_CONFIRM;
		respondErrorDetected = 0;
		digO2Connected = 0;
	}
	else
	{
		if(localComState == UART_O2_ERROR)
		{
			localComState = lastComState;
		}
		lastComState = localComState;
		if(localComState == UART_O2_IDLE)							/* cyclic request of o2 value */
		{
			if((activeSensor != MAX_MUX_CHANNEL) && (tmpSensorDataDiveO2.sensorId == 0))
			{
				localComState = UART_O2_REQ_ID;
			}
			else
			{
				localComState = UART_O2_REQ_RAW;
			}
		}
		rxState = O2RX_CONFIRM;
		uartO2_SetupCmd(localComState,cmdString,&cmdLength);
		UART_SendCmdString(cmdString);
	}
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}

void uartO2_ProcessData(uint8_t data)
{
	static uint8_t cmdReadIndex = 0;
	static uint8_t errorReadIndex = 0;
	static char tmpRxBuf[30];
	static uint8_t tmpRxIdx = 0;

	uint32_t tmpO2 = 0;
	uint32_t tmpData = 0;

	uint32_t tick =  HAL_GetTick();

	uartO2Status_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	lastReceiveTick = tick;
	switch(rxState)
	{
		case O2RX_CONFIRM:	if(data == '#')
							{
								cmdReadIndex = 0;
								errorReadIndex = 0;
							}
							if(errorReadIndex < sizeof(errorStr)-1)
							{
								if(data == errorStr[errorReadIndex])
								{
									errorReadIndex++;
								}
								else
								{
									errorReadIndex = 0;
								}
							}
							else
							{
								respondErrorDetected = 1;
								errorReadIndex = 0;
								if(localComState != UART_O2_IDLE)
								{
									localComState = UART_O2_ERROR;
								}
							}
							if(data == cmdString[cmdReadIndex])
							{
								cmdReadIndex++;
								if(cmdReadIndex == cmdLength - 3)
								{
									errorReadIndex = 0;
									if((activeSensor == MAX_MUX_CHANNEL))
									{
										if(respondErrorDetected)
										{
											digO2Connected = 0;		/* the multiplexer mirrors the incoming message and does not generate an error information => no mux connected */
										}
										else
										{
											digO2Connected = 1;
										}
									}
									else							/* handle sensors which should respond with an error message after channel switch */
									{
										digO2Connected = 1;
									}
									tmpRxIdx = 0;
									memset((char*) tmpRxBuf, 0, sizeof(tmpRxBuf));
									cmdReadIndex = 0;
									switch (localComState)
									{
											case UART_O2_CHECK:	localComState = UART_O2_IDLE;
																rxState = O2RX_IDLE;
												break;
											case UART_O2_REQ_ID: rxState = O2RX_GETNR;
												break;
											case UART_O2_REQ_INFO: rxState = O2RX_GETTYPE;
												break;
											case UART_O2_REQ_RAW:
											case UART_O2_REQ_O2:	rxState = O2RX_GETO2;
												break;
											default:	localComState = UART_O2_IDLE;
														rxState = O2RX_IDLE;
													break;
									}
								}
							}
							else
							{
								cmdReadIndex = 0;
							}
				break;

			case O2RX_GETSTATUS:
			case O2RX_GETTEMP:
			case O2RX_GETTYPE:
			case O2RX_GETVERSION:
			case O2RX_GETCHANNEL:
			case O2RX_GETSUBSENSORS:
			case O2RX_GETO2:
			case O2RX_GETNR:
			case O2RX_GETDPHI:
			case O2RX_INTENSITY:
			case O2RX_AMBIENTLIGHT:
			case O2RX_PRESSURE:
			case O2RX_HUMIDITY:
								if(data != 0x0D)
								{
									if(data != ' ')		/* the following data entities are placed within the data stream => no need to store data at the end */
									{
										tmpRxBuf[tmpRxIdx++] = data;
									}
									else
									{
										if(tmpRxIdx != 0)
										{
											switch(rxState)
											{
												case O2RX_GETCHANNEL:	StringToInt(tmpRxBuf,&tmpData);
																		rxState = O2RX_GETVERSION;
														break;
												case O2RX_GETVERSION:	StringToInt(tmpRxBuf,&tmpData);
																		rxState = O2RX_GETSUBSENSORS;
														break;
												case O2RX_GETTYPE: 		StringToInt(tmpRxBuf,&tmpData);
																		rxState = O2RX_GETCHANNEL;
														break;

												case O2RX_GETO2: 		StringToInt(tmpRxBuf,&tmpO2);

																		setExternalInterfaceChannel(activeSensor + EXT_INTERFACE_MUX_OFFSET,(float)(tmpO2 / 10000.0));
																		rxState = O2RX_GETTEMP;
													break;
												case O2RX_GETTEMP:		StringToInt(tmpRxBuf,(uint32_t*)&tmpSensorDataDiveO2.temperature);
																		rxState = O2RX_GETSTATUS;
													break;
												case O2RX_GETSTATUS:	StringToInt(tmpRxBuf,&tmpSensorDataDiveO2.status);				/* raw data cycle */
																		rxState = O2RX_GETDPHI;
													break;
												case O2RX_GETDPHI:		/* ignored to save memory and most likly irrelevant for diver */
																		rxState = O2RX_INTENSITY;
																									break;
												case O2RX_INTENSITY:	StringToInt(tmpRxBuf,(uint32_t*)&tmpSensorDataDiveO2.intensity);				/* raw data cycle */
																		rxState = O2RX_AMBIENTLIGHT;
																									break;
												case O2RX_AMBIENTLIGHT:	StringToInt(tmpRxBuf,(uint32_t*)&tmpSensorDataDiveO2.ambient);				/* raw data cycle */
																		rxState = O2RX_PRESSURE;
																									break;
												case O2RX_PRESSURE:	StringToInt(tmpRxBuf,(uint32_t*)&tmpSensorDataDiveO2.pressure);					/* raw data cycle */
																		rxState = O2RX_HUMIDITY;
																									break;
												default:
													break;
											}
											memset((char*) tmpRxBuf, 0, tmpRxIdx);
											tmpRxIdx = 0;
										}
									}
								}
								else
								{							/* the following data items are the last of a sensor respond => store temporal data */
									switch (rxState)
									{
										case O2RX_GETSTATUS:		StringToInt(tmpRxBuf,&tmpSensorDataDiveO2.status);
																	externalInterface_SetSensorData(activeSensor + EXT_INTERFACE_MUX_OFFSET,(uint8_t*)&tmpSensorDataDiveO2);
																	localComState = UART_O2_IDLE;
																	rxState = O2RX_IDLE;
												break;
										case O2RX_GETSUBSENSORS:	StringToInt(tmpRxBuf,&tmpData);
										localComState = UART_O2_IDLE;
																	rxState = O2RX_IDLE;
												break;
										case O2RX_HUMIDITY:			StringToInt(tmpRxBuf,(uint32_t*)&tmpSensorDataDiveO2.humidity);				/* raw data cycle */
																	externalInterface_SetSensorData(activeSensor + EXT_INTERFACE_MUX_OFFSET,(uint8_t*)&tmpSensorDataDiveO2);
																	localComState = UART_O2_IDLE;
																	rxState = O2RX_IDLE;
												break;
										case  O2RX_GETNR: 			StringToUInt64((char*)tmpRxBuf,&tmpSensorDataDiveO2.sensorId);
																	externalInterface_SetSensorData(activeSensor + EXT_INTERFACE_MUX_OFFSET,(uint8_t*)&tmpSensorDataDiveO2);
																	localComState = UART_O2_IDLE;
																	rxState = O2RX_IDLE;
											break;
										default:		localComState = UART_O2_IDLE;
														rxState = O2RX_IDLE;
											break;
									}
								}
					break;
			default:				rxState = O2RX_IDLE;
				break;

	}
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}


uint8_t uartO2_isSensorConnected()
{
	return digO2Connected;
}

void uartO2_SetChannel(uint8_t channel)
{
	if(channel <= MAX_MUX_CHANNEL)
	{
		activeSensor = channel;
	}
}

