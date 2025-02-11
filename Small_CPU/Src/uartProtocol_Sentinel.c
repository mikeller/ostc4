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
static uint8_t SentinelConnected = 0;						/* Binary indicator if a sensor is connected or not */
static receiveStateSentinel_t rxState = SENTRX_Ready;

extern sUartComCtrl Uart1Ctrl;

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

void uartSentinel_Control(void)
{
	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartSentinelStatus_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	if(localComState == UART_SENTINEL_INIT)
	{
		SentinelConnected = 0;
		UART_StartDMA_Receiption(&Uart1Ctrl);
		localComState = UART_SENTINEL_IDLE;
	}
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}

void uartSentinel_ProcessData(uint8_t data)
{
	static uint8_t dataType = 0;
	static uint32_t dataValue[3];
	static uint8_t dataValueIdx = 0;

	static uint8_t checksum = 0;
	static char checksum_str[]="00";

	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartSentinelStatus_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	switch(rxState)
	{
			case SENTRX_Ready:	if((data >= 'a') && (data <= 'z'))
							{
								rxState = SENTRX_DetectStart;
								checksum = 0;
							}
					break;

			case SENTRX_DetectStart: 	checksum += data;
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

			case SENTRX_SelectData:		checksum += data;
									switch(data)
									{
										case 'T':	dataType = data;
											break;
										case '0': 	if(dataType != 0xff)
													{
														rxState = SENTRX_Data0;
														dataValueIdx = 0;
														dataValue[0] = 0;

													}
													else
													{
														rxState = SENTRX_Ready;
													}
											break;
										default:	rxState = SENTRX_Ready;
									}
					break;

			case SENTRX_Data0:
			case SENTRX_Data1:
			case SENTRX_Data2:
			case SENTRX_Data4:
			case SENTRX_Data5:
			case SENTRX_Data6:
			case SENTRX_Data8:
			case SENTRX_Data9:
			case SENTRX_Data10: checksum += data;
							if((data >= '0') && (data <= '9'))
							{
								dataValue[dataValueIdx] = dataValue[dataValueIdx] * 10 + (data - '0');
								rxState++;
							}
							else
							{
								rxState = SENTRX_Ready;
							}
					break;

			case SENTRX_Data3:
			case SENTRX_Data7:	checksum += data;
							if(data == '0')
							{
								rxState++;
								dataValueIdx++;
								dataValue[dataValueIdx] = 0;
							}
							else
							{
								rxState = SENTRX_Ready;
							}
					break;
			case SENTRX_Data11: rxState = SENTRX_DataComplete;
							ConvertByteToHexString(checksum,checksum_str);
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
										setExternalInterfaceChannel(0,(float)(dataValue[0] / 10.0));
										setExternalInterfaceChannel(1,(float)(dataValue[1] / 10.0));
										setExternalInterfaceChannel(2,(float)(dataValue[2] / 10.0));
										SentinelConnected = 1;
										localComState = UART_SENTINEL_OPERATING;
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

