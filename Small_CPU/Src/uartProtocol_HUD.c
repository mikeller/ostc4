/**
  ******************************************************************************
  * @file    uartProtocol_HUD.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    24-Feb-2026
  * @brief   Interface functionality to external, UART based HUD
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
#include <uartProtocol_HUD.h>
#include "uart.h"
#include "externalInterface.h"

#ifdef ENABLE_HUD_SUPPORT
static uint8_t HUDConnected = 0;						/* Binary indicator if a sensor is connected or not */
static receiveStateHUD_t rxState = HUDRX_Ready;



void uartHUD_SendCmd(uint8_t HUDCmd)
{
	uint8_t cmdLength = 0;
	uint8_t cmdBuf[HUD_MAX_CMD_LENGTH];
	uint8_t index = 0;
	uint16_t checkSum = 0;

	cmdBuf[0] = HUD_CMD_BYTE_START;
	switch (HUDCmd)
	{
		case HUDCMD_GETINFO:	cmdBuf[1] = HUD_CMD_BYTE_INFO;
								cmdLength = 1;
				break;
		case HUDCMD_UPDATE:		cmdBuf[1] = HUD_CMD_BYTE_UPDATE;
								externalInterface_GetHUDSequence(&cmdBuf[3],&cmdBuf[2]);
								cmdLength = 19;
				break;
		case HUDCMD_ABORTSEQ:	cmdBuf[1] = HUD_CMD_BYTE_STOP;
								cmdLength = 1;
				break;
		default: cmdLength = 0;
			break;
	}
	if(cmdLength != 0)
	{
		cmdLength++; 	/* add Startbyte */
		for(index = 0; index < cmdLength; index++)
		{
			if(index > 2)		/* hard coded number of pulses = 2 */
			{
				cmdBuf[index] |= 0x10;
			}
			checkSum += cmdBuf[index];
		}
		cmdBuf[cmdLength++] = (checkSum & 0x00FF);	/* low byte */
		cmdBuf[cmdLength++] = (checkSum >> 8);		/* high byte */
		UART_SendCmdRaw(cmdBuf,cmdLength);
	}
}


void uartHUD_Control(void)
{
	static uint8_t cmdString[20];
	static uint8_t cmdLength = 0;
	static uint8_t lastComState = UART_HUD_INIT;

	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartHUDStatus_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	if(localComState == UART_HUD_ERROR)
	{
		localComState = lastComState;
	}

	switch(localComState)
	{
		case UART_HUD_INIT:		HUDConnected = 0;
								UART_ReadData(SENSOR_HUD, 1);	/* flush buffer */
								UART_StartDMA_Receiption(&Uart1Ctrl);
								localComState = UART_HUD_SETUP;
								uartHUD_SendCmd(HUDCMD_GETINFO);
			break;
		case UART_HUD_SETUP:	uartHUD_SendCmd(HUDCMD_GETINFO);
								rxState = HUDRX_DetectStart;
			break;
		case UART_HUD_UPDATE:	uartHUD_SendCmd(HUDCMD_UPDATE);
								rxState = HUDRX_Ready;
			break;
		case UART_HUD_ABORT:	uartHUD_SendCmd(HUDCMD_ABORTSEQ);
								rxState = HUDRX_Ready;
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

void uartHUD_ProcessData(uint8_t data)
{
	static uint8_t dataValue[HUD_INFO_DATA_LENGTH];
	static uint8_t dataIndex = 0;
	static uint16_t checkSum = 0;
	static uint16_t rxCheckSum = 0;
	uint8_t activeSensor = externalInterface_GetActiveUartSensor();
	uartHUDStatus_t localComState = externalInterface_GetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET);

	if((localComState == UART_HUD_SETUP) && (rxState == HUDRX_Ready))
	{
		rxState = HUDRX_DetectStart;
	}
	switch(rxState)
	{
		case HUDRX_DetectStart:	if(data == 0xAA)
								{
									dataIndex = 0;
									memset(dataValue,0,HUD_INFO_DATA_LENGTH);
									dataValue[dataIndex++] = data;
									checkSum = data;
									rxState = HUDRX_RXData;
								}
			break;
		case HUDRX_RXData:		dataValue[dataIndex++] = data;
								checkSum += data;
								if(dataIndex == HUD_INFO_DATA_LENGTH)
								{
									rxState = HUDRX_CheckSum_L;
								}
			break;
		case HUDRX_CheckSum_L:	rxCheckSum = data;
								rxState++;
			break;
		case HUDRX_CheckSum_H:	rxCheckSum |= (data << 8);
								if(checkSum == rxCheckSum)
								{
									HUDConnected = 1;
									if(localComState == UART_HUD_SETUP)
									{
										externalInterface_SetSensorData(activeSensor + EXT_INTERFACE_MUX_OFFSET, &dataValue[1]);
										localComState = UART_HUD_ABORT;		/* reset default sequence */
									}
								}
								rxState = HUDRX_DetectStart;
			break;
		default:				if(data == 'K')		/* OK respond from HUD */
								{
									localComState = UART_HUD_IDLE;
								}
								if(data == 'N')		/* NOK respond from HUD */
								{
									localComState = UART_HUD_ERROR;
								}
								if(data == 0xff)
								{
									localComState = UART_HUD_IDLE;
								}
			break;
	}

	externalInterface_SetSensorState(activeSensor + EXT_INTERFACE_MUX_OFFSET,localComState);
}

uint8_t uartHUD_isSensorConnected()
{
	return HUDConnected;
}

#endif

