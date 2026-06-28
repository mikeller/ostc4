/**
  ******************************************************************************
  * @file    uartProtocol_Co2.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    31-Jul-2023
  * @brief   Interface functionality to external, UART based CO2 sensors
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
#include <uartProtocol_Co2.h>
#include "uart.h"
#include "externalInterface.h"

#ifdef ENABLE_CO2_SUPPORT
static uint8_t CO2Connected = 0;						/* Binary indicator if a sensor is connected or not */
static receiveStateCO2_t rxState = CO2RX_Ready;



float LED_Level = 0.0;							/* Normalized LED value which may be used as indication for the health status of the sensor */
float LED_ZeroOffset = 0.0;
float pCO2 = 0.0;



void uartCo2_SendCmd(uint8_t CO2Cmd, uint8_t *cmdString, uint8_t *cmdLength)
{
	*cmdLength = 0;

	switch (CO2Cmd)
	{
		case CO2CMD_MODE_POLL:		*cmdLength = snprintf((char*)cmdString, 20, "K 2\r\n");
				break;
		case CO2CMD_MODE_STREAM:	*cmdLength = snprintf((char*)cmdString, 20, "K 1\r\n");
				break;
		case CO2CMD_CALIBRATE_H:	*cmdLength = snprintf((char*)cmdString, 20, "P 10 1\r\n");		/* set 400ppm as reference => 1 (256) and 144 */
				break;
		case CO2CMD_CALIBRATE_L:	*cmdLength = snprintf((char*)cmdString, 20, "P 11 144\r\n");
				break;
		case CO2CMD_CALIBRATE:		*cmdLength = snprintf((char*)cmdString, 20, "G\r\n");
				break;
		case CO2CMD_GETDATA:		*cmdLength = snprintf((char*)cmdString, 20, "Q\r\n");
				break;
		case CO2CMD_GETSCALE:		*cmdLength = snprintf((char*)cmdString, 20, ".\r\n");
				break;
		default: *cmdLength = 0;
			break;
	}
	if(cmdLength != 0)
	{
		UART_SendCmdString(cmdString);
	}
}


void uartCo2_Control(void)
{
	static uint8_t cmdString[20];
	static uint8_t cmdLength = 0;
	static uint8_t lastComState = UART_CO2_INIT;

	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartCO2Status_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	if(localComState == UART_CO2_ERROR)
	{
		localComState = lastComState;
	}

	switch(localComState)
	{
		case UART_CO2_INIT:		CO2Connected = 0;
								externalInterface_SetCO2Scale(0.0);
								UART_ReadData(SENSOR_CO2, 1);	/* flush buffer */
								UART_StartDMA_Receiption(&Uart1Ctrl);
								localComState = UART_CO2_SETUP;
								uartCo2_SendCmd(CO2CMD_GETSCALE, cmdString, &cmdLength);
			break;
		case UART_CO2_SETUP:	if(externalInterface_GetCO2Scale() == 0.0)
								{
									uartCo2_SendCmd(CO2CMD_GETSCALE, cmdString, &cmdLength);
								}
								else
								{
									uartCo2_SendCmd(CO2CMD_MODE_POLL, cmdString, &cmdLength);
									localComState = UART_CO2_MODE;
								}
			break;
		case UART_CO2_MODE:		uartCo2_SendCmd(CO2CMD_MODE_POLL, cmdString, &cmdLength);
			break;
		case UART_CO2_CALIBRATE_H:	uartCo2_SendCmd(CO2CMD_CALIBRATE_H, cmdString, &cmdLength);
			break;
		case UART_CO2_CALIBRATE_L:	uartCo2_SendCmd(CO2CMD_CALIBRATE_L, cmdString, &cmdLength);
			break;
		case UART_CO2_CALIBRATE:	uartCo2_SendCmd(CO2CMD_CALIBRATE, cmdString, &cmdLength);
									localComState = UART_CO2_IDLE;
			break;
		case UART_CO2_IDLE:		if(externalInterface_GetCO2Scale() == 0.0)	/* artifact from streaming mode => not needed for polling */
								{
									uartCo2_SendCmd(CO2CMD_GETSCALE, cmdString, &cmdLength);
									localComState = UART_CO2_SETUP;
								}
								else
								{
									uartCo2_SendCmd(CO2CMD_GETDATA, cmdString, &cmdLength);
									localComState = UART_CO2_OPERATING;
								}
			break;
		default:				if(cmdLength != 0)
								{
									UART_SendCmdString(cmdString);		/* resend last command */
									cmdLength = 0;
								}
			break;
	}
	lastComState = localComState;
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}

void uartCo2_ProcessData(uint8_t data)
{
	static uint8_t dataType = 0;
	static uint32_t dataValue[3];
	static uint8_t dataIndex = 0;
	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartCO2Status_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	uint8_t *pMap = externalInterface_GetSensorMapPointer(0);

	if(rxState == CO2RX_Ready)		/* identify data content */
	{
		switch(data)
		{
			case 'P':
			case 'G':
			case 'K':
			case 'l':
			case 'D':
			case 'Z':
			case '.':			dataType = data;
								rxState = CO2RX_Data0;
								dataValue[0] = 0;
								dataIndex = 0;
				break;
			case '?':			localComState = UART_CO2_ERROR;
								rxState = CO2RX_Ready;
				break;
			default:			/* unknown or corrupted => ignore */
					break;
		}
	}
	else if((data >= '0') && (data <= '9'))
	{
		if((rxState >= CO2RX_Data0) && (rxState <= CO2RX_Data9))
		{
			dataValue[dataIndex] = dataValue[dataIndex] * 10 + (data - '0');

			switch (dataType)
			{
				case 'G':
				case 'K':
				case 'l':
				case 'D':
				case 'Z':
				case '.':
				default:	if((rxState == CO2RX_Data4))
							{
								rxState = CO2RX_DataComplete;	/* just one value to be received */
							}
							else
							{
								rxState++;
							}
					break;
				case 'P':		if(rxState == CO2RX_Data9)		/* get second parameter */
								{
									rxState = CO2RX_DataComplete;
								}
								else
								{
									rxState++;
								}
						break;
			}
		}
		else	/* protocol error data has max 5 digits */
		{
			if(rxState != CO2RX_DataComplete)	/* commands will not answer with number values */
			{
				rxState = CO2RX_Ready;
			}
		}
	}
	else if((data == ' ') || (data == '\n'))	/* Abort data detection */
	{
		if(rxState == CO2RX_DataComplete)
		{
			CO2Connected = 1;
			switch(localComState)
			{
				case UART_CO2_SETUP:	if(dataType == '.')
										{
											localComState = UART_CO2_MODE;
										}
					break;
				case UART_CO2_MODE:		if((dataType == 'K') && (dataValue[dataIndex] == 2))
										{
											localComState = UART_CO2_IDLE;
										}
					break;
				case UART_CO2_CALIBRATE_H:	if((dataType == 'P') && (dataValue[0] == 10) && (dataValue[1] == 1))
											{
												localComState = UART_CO2_CALIBRATE_L;
											}
											else
											{
												localComState = UART_CO2_IDLE;
											}
					break;
				case UART_CO2_CALIBRATE_L:	if((dataType == 'P') && (dataValue[0] == 11) && (dataValue[1] == 144))
											{
												localComState = UART_CO2_CALIBRATE;
											}
											else
											{
												localComState = UART_CO2_IDLE;
											}
					break;
				default: 					localComState = UART_CO2_IDLE;
					break;
			}

			switch(dataType)
			{
				case 'D':			externalInterface_SetCO2SignalStrength(dataValue[dataIndex]);
					break;
				case 'l':			LED_ZeroOffset = dataValue[dataIndex];
					break;
				case 'Z':			externalInterface_SetCO2Value(dataValue[dataIndex]);
									if(pMap[activeSensor + EXT_INTERFACE_MUX_OFFSET] == SENSOR_SENTINEL_CO2)
									{
										localComState = UART_CO2_DONE;
									}
					break;
				case '.':			externalInterface_SetCO2Scale(dataValue[dataIndex]);
					break;
				default:			rxState = CO2RX_Ready;
					break;
			}
			rxState = CO2RX_Ready;
		}
		else	/* multi parameter */
		{
			if(rxState == CO2RX_Data5)
			{
				switch(dataType)
				{
					case 'P':	dataIndex++;
								dataValue[dataIndex] = 0;
						break;
					default:
						break;
				}
			}
		}
		if((rxState != CO2RX_Data0) && (rxState != CO2RX_Data5))	/* reset state machine because message in wrong format */
		{
			rxState = CO2RX_Ready;
		}
	}
	else
	{
		if((rxState >= CO2RX_Data0) && (rxState <= CO2RX_Data4))
		{
			rxState = CO2RX_Ready; /* numerical data expected => abort */
		}
	}
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}

uint8_t uartCo2_isSensorConnected()
{
	return CO2Connected;
}

#endif

