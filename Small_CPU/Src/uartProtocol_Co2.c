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
		case CO2CMD_MODE_POLL:		*cmdLength = snprintf((char*)cmdString, 10, "K 2\r\n");
				break;
		case CO2CMD_MODE_STREAM:	*cmdLength = snprintf((char*)cmdString, 10, "K 1\r\n");
				break;
		case CO2CMD_CALIBRATE:		*cmdLength = snprintf((char*)cmdString, 10, "G\r\n");
				break;
		case CO2CMD_GETDATA:		*cmdLength = snprintf((char*)cmdString, 10, "Q\r\n");
				break;
		case CO2CMD_GETSCALE:		*cmdLength = snprintf((char*)cmdString, 10, ".\r\n");
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
	static uint8_t cmdString[10];
	static uint8_t cmdLength = 0;
	static uint8_t lastComState = 0;

	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartCO2Status_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	uint8_t *pmap = externalInterface_GetSensorMapPointer(0);


	if(localComState == UART_CO2_ERROR)
	{
		localComState = lastComState;
	}

	if(localComState == UART_CO2_INIT)
	{
		CO2Connected = 0;
		externalInterface_SetCO2Scale(0.0);
		UART_ReadData(SENSOR_CO2, 1);	/* flush buffer */
		UART_StartDMA_Receiption(&Uart1Ctrl);
		localComState = UART_CO2_SETUP;
	}
	if(localComState == UART_CO2_SETUP)
	{
		if(externalInterface_GetCO2Scale() == 0.0)
		{
			uartCo2_SendCmd(CO2CMD_GETSCALE, cmdString, &cmdLength);
		}
		else
		{
			localComState = UART_CO2_IDLE;
		}
	}
	else
	{
		if(localComState == UART_CO2_CALIBRATE)
		{
			uartCo2_SendCmd(CO2CMD_CALIBRATE, cmdString, &cmdLength);
			localComState = UART_CO2_IDLE;
		}
		else if(pmap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)		/* sensor is working in polling mode if mux is connected to avoid interference with other sensors */
		{
			//if(cmdLength == 0)							/* poll data */
			if(localComState == UART_CO2_IDLE)
			{
				if(externalInterface_GetCO2Scale() == 0.0)
				{
					uartCo2_SendCmd(CO2CMD_GETSCALE, cmdString, &cmdLength);
					localComState = UART_CO2_SETUP;
				}
				else
				{
					uartCo2_SendCmd(CO2CMD_GETDATA, cmdString, &cmdLength);
					localComState = UART_CO2_OPERATING;
				}
			}
			else											/* resend last command */
			{
				UART_SendCmdString(cmdString);
				cmdLength = 0;
			}
		}
		else
		{
			localComState = UART_CO2_OPERATING;					/* sensor in streaming mode if not connected to mux => operating */
			UART_StartDMA_Receiption(&Uart1Ctrl);
		}
	}
	lastComState = localComState;
	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}


void uartCo2_ProcessData(uint8_t data)
{
	static uint8_t dataType = 0;
	static uint32_t dataValue = 0;
	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartCO2Status_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	if(rxState == CO2RX_Ready)		/* identify data content */
	{
		switch(data)
		{
			case 'G':
			case 'l':
			case 'D':
			case 'Z':
			case '.':			dataType = data;
								rxState = CO2RX_Data0;
								dataValue = 0;
				break;
			case '?':			localComState = UART_CO2_ERROR;
				break;
			default:			/* unknown or corrupted => ignore */
					break;
		}
	}
	else if((data >= '0') && (data <= '9'))
	{
		if((rxState >= CO2RX_Data0) && (rxState <= CO2RX_Data4))
		{
			dataValue = dataValue * 10 + (data - '0');
			rxState++;
			if(rxState == CO2RX_Data5)
			{
				rxState = CO2RX_DataComplete;
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
			if(localComState == UART_CO2_SETUP)
			{
				if(dataType == '.')
				{
					localComState = UART_CO2_IDLE;
				}
			}
			else
			{
				localComState = UART_CO2_IDLE;
			}
			if(externalInterface_GetCO2State() == 0)
			{
				externalInterface_SetCO2State(EXT_INTERFACE_33V_ON);
			}
			switch(dataType)
			{
				case 'D':			externalInterface_SetCO2SignalStrength(dataValue);
					break;
				case 'l':			LED_ZeroOffset = dataValue;
					break;
				case 'Z':			externalInterface_SetCO2Value(dataValue);
					break;
				case '.':			externalInterface_SetCO2Scale(dataValue);
					break;
				default:			rxState = CO2RX_Ready;
					break;
			}
			rxState = CO2RX_Ready;
		}
		if(rxState != CO2RX_Data0)	/* reset state machine because message in wrong format */
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

