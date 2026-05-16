/**
  ******************************************************************************
  * @file    uartProtocol_Co2.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    15-Jan-2024
  * @brief   Interface functionality to read data from Sentinel rebreather
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
#include <uartProtocol_Sentinel.h>
#include "uart.h"
#include "externalInterface.h"


#ifdef ENABLE_SENTINEL_MODE
static uint8_t SentinelConnected = 0;						/* Binary indicator if a sensor (and what type of subsensor) is connected or not */
static receiveStateSentinel_t rxState = SENTRX_Ready;

extern sUartComCtrl Uart1Ctrl;


const char hex[] = "0123456789ABCDEF";
void ConvertByteToHexString(uint8_t byte, char* str)
{
    str[0] = hex[(byte >> 4) & 0x0F];
    str[1] = hex[byte & 0x0F];
    str[2] = '\0';
}

void uartSentinel_Control(void)
{
	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartSentinelStatus_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	if(localComState == UART_SENTINEL_INIT)
	{
		SentinelConnected = 0;
		UART_StartDMA_Receiption(&Uart1Ctrl);
		localComState = UART_SENTINEL_IDLE;
		externalInterface_SetCO2Scale(10.0);
	}

	if((localComState == UART_SENTINEL_IDLE) || (localComState == UART_SENTINEL_DONE))
	{
		if(Uart1Ctrl.dmaRxActive == 0)
		{
			UART_StartDMA_Receiption(&Uart1Ctrl);
		}
		localComState = UART_SENTINEL_OPERATING;		/* state is only used for timeout detection */
	}
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}

void uartSentinel_ProcessData(uint8_t data)
{
	static uint8_t dataType = 0;
	static uint16_t dataValue[10];
	static uint8_t dataValueIdx = 0;
	static uint8_t rxByteCnt = 0;

	static uint8_t dataSetReceived = 0x0;

	static uint8_t checksum = 0;
	static char checksum_str[3];

	static uint32_t lastSlowDataTick = 0;

	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartSentinelStatus_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	switch(rxState)
	{
			case SENTRX_Ready:	if((data >= 'a') && (data <= 'z'))			/* Alive byte */
							{
								rxState = SENTRX_DetectStart;
								checksum = 0;
							}
					break;

			case SENTRX_DetectStart: 	checksum += data;					/* data available */
									if(data == '1')
								 	{
								 		rxState = SENTRX_SelectData;
								 		dataType = 0xFF;
								 	}
									else
									{
										rxState = SENTRX_Ready;
									}
					break;

			case SENTRX_SelectData:		checksum += data;					/* data type */
									switch(data)
									{
										case UART_SENTINEL_O2_P:
										case UART_SENTINEL_O2_S:
										case UART_SENTINEL_TEMPSTICK:
										case UART_SENTINEL_PRESSURE_O2:
										case UART_SENTINEL_PRESSURE_D:	dataType = data;
																		rxState = SENTRX_Data;
																		dataValueIdx = 0;
																		dataValue[0] = 0;
																		rxByteCnt = 0;
											break;
										default:	rxState = SENTRX_Ready;
											break;
									}
					break;

			case SENTRX_Data:  checksum += data;
									switch(dataType)		/* next data value or end? */
									{
										case UART_SENTINEL_O2_P:
										case UART_SENTINEL_O2_S:
																 if((rxByteCnt == 4 ) || (rxByteCnt == 8 ))
																 {
																	if (data == '0')
																	{
																		dataValueIdx++;
																		dataValue[dataValueIdx] = 0;
																	}
																	else
																	{
																		rxState = SENTRX_Ready;
																	}
																 }
																 else
																 {
																	 if(rxByteCnt == 11)
																	 {
																		 rxState = SENTRX_CheckSum;
																	 }
																 }

											break;
										case UART_SENTINEL_PRESSURE_O2:
										case UART_SENTINEL_PRESSURE_D:	if(rxByteCnt == 2)
																		{
																			rxState = SENTRX_CheckSum;
																		}
											break;
										case UART_SENTINEL_TEMPSTICK:	if(rxByteCnt == 23)
																		{
																			rxState = SENTRX_CheckSum;
																		}
																		else if((rxByteCnt == 3 ) || (rxByteCnt == 6) || (rxByteCnt == 9) || (rxByteCnt == 12 ) || (rxByteCnt == 15) || (rxByteCnt == 18) || (rxByteCnt == 21))
																		{
																			dataValueIdx++;
																			dataValue[dataValueIdx] = 0;
																		}
											break;
										default:
											break;
									}

									if((data >= '0') && (data <= '9'))
									{
											dataValue[dataValueIdx] = dataValue[dataValueIdx] * 10 + (data - '0');
											rxByteCnt++;
									}
									else
									{
										rxState = SENTRX_Ready;
									}

								break;

			case SENTRX_CheckSum: ConvertByteToHexString(checksum,checksum_str);
								if(data == checksum_str[0])
								{
									rxState = SENTRX_DataComplete;
								}
								else
								{
									rxState = SENTRX_Ready;
								}

				break;

			case SENTRX_DataComplete:	if(data == checksum_str[1])
									{
										switch(dataType)
										{
											case UART_SENTINEL_O2_P: 	setExternalInterfaceChannel(0,(float)(dataValue[0] / 10.0));
																		setExternalInterfaceChannel(1,(float)(dataValue[1] / 10.0));
																		setExternalInterfaceChannel(2,(float)(dataValue[2] / 10.0));
																		SentinelConnected |= SENTINEL_O2;
																		dataSetReceived |= SENTINEL_O2;
												break;
											case UART_SENTINEL_PRESSURE_O2:	externalInterface_SetBottlePressure(0,dataValue[0]);
																			SentinelConnected |= SENTINEL_PRESSURE;
																			dataSetReceived |= SENTINEL_PRESSURE;
												break;
											case UART_SENTINEL_PRESSURE_D: externalInterface_SetBottlePressure(1,dataValue[0]);
																			SentinelConnected |= SENTINEL_PRESSURE;
																			dataSetReceived |= SENTINEL_PRESSURE;
												break;
											case UART_SENTINEL_TEMPSTICK: 	SentinelConnected |= SENTINEL_TEMPSTICK;
																			dataSetReceived |= SENTINEL_TEMPSTICK;
																			externalInterface_SetTempstickValue(8,dataValue);
																			lastSlowDataTick = HAL_GetTick();

										}

										if((time_elapsed_ms(lastSlowDataTick, HAL_GetTick()) < 20000 ))
										{
											if(dataSetReceived & SENTINEL_O2 )
											{
												localComState = UART_SENTINEL_DONE;
												dataSetReceived = 0;
											}
										}
										else
										{
											if(dataSetReceived == (SENTINEL_O2 | SENTINEL_PRESSURE | SENTINEL_TEMPSTICK))
											{
												localComState = UART_SENTINEL_DONE;
												dataSetReceived = 0;
											}
										}
									}
									rxState = SENTRX_Ready;
				break;


			default:				rxState = SENTRX_Ready;
				break;

	}
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}

uint8_t uartSentinel_isSensorConnected()
{
	return SentinelConnected;
}

#endif

