///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/cv_heartbeat.c
/// \brief  providing functionality to connect OSTC to a Polar HC10 heartbeat sensor
/// \author heinrichs weikamp gmbh
/// \date   03-July-2025
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2025 Heinrichs Weikamp gmbh
///
///     This program is free software: you can redistribute it and/or modify
///     it under the terms of the GNU General Public License as published by
///     the Free Software Foundation, either version 3 of the License, or
///     (at your option) any later version.
///
///     This program is distributed in the hope that it will be useful,
///     but WITHOUT ANY WARRANTY; without even the implied warranty of
///     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///     GNU General Public License for more details.
///
///     You should have received a copy of the GNU General Public License
///     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#include "cv_heartbeat.h"
#include "tMenuEdit.h"

#include "gfx_fonts.h"
#include "tHome.h"
#include "ostc.h"
#include "tComm.h"
#include "tInfoLogger.h"

static sensorHeartbeat_State_t heartbeatState = SENSOR_HB_OFFLINE;

static uint8_t OnAction_Heartbeat(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action);
static uint32_t startDetection_ms;

#define MAX_BT_DEVICE 10		/* max number of device which may be handled */
static btDdeviceData_t btDeviceList[MAX_BT_DEVICE];
static btDeviceService_t curDeviceService[10];
static uint8_t curServiceIndex = 0;
static uint8_t curBtIndex = 0;
static uint8_t connHandle = ' ';

static indicatior_t checkIndicators(uint8_t* pdata)
{
#if 0
	static uint8_t foundRSSI = 0;
	static uint8_t foundNAME = 0;
	static uint8_t foundMNF = 0;
	static uint8_t foundUUID = 0;
	static uint8_t foundOK = 0;
#endif
	indicatior_t ret = NO_INDICATOR;

	if(strcmp((char*)pdata,"+UBTD:") == 0)
	{
		ret = DEVICE_INDICATOR;
	}
	else if(strcmp((char*)pdata,"+UUBTACLC:") == 0)
	{
		ret = CONNECTION_INDICATOR;
	}
	else if(strcmp((char*)pdata,"+UBTGDP:") == 0)
	{
		ret = SERVICE_INDICATOR;
	}

	return ret;
}

static void handleOK()
{
	switch(heartbeatState)
	{
		case SENSOR_HB_ENABLE_BLE:	heartbeatState = SENSOR_HB_CHECK_CONFIG;
							break;
		case SENSOR_HB_CHECK_CONFIG: heartbeatState = SENSOR_HB_DISCOVER;
							break;
		case SENSOR_HB_RESTART: 	heartbeatState = SENSOR_HB_OFFLINE;
							break;
		case SENSOR_HB_DISCOVER: heartbeatState = SENSOR_HB_CONNECT;
							break;
		case SENSOR_HB_SERVICES: heartbeatState = SENSOR_HB_OFFLINE;
							break;
		default:
							break;
	}
}

static void handleERROR()
{

}

static uint8_t getDeviceList()
{
	static uint8_t firstDevice = 1;
	static uint8_t curLine[MAX_CHAR_PER_LINE];			/* holds complete line and is used for logging */
	static uint8_t curLineIndex = 0;
	static uint8_t parameter[40];		/* content of the parameter in read state */
	static uint8_t writeIndex = 0;
	static uint8_t complete = 0;

	static readDataType_t readType = BT_READ_NOTHING;

	char text[40];
	uint8_t data = 0;
	data = UART_getChar();

	while((data != 0) && (complete == 0))
	{
		if(curLineIndex == MAX_CHAR_PER_LINE)		/* avoid overflow */
		{
			InfoLogger_writeLine(curLine,curLineIndex,0);
			memset(curLine,0,sizeof(curLine));
			curLineIndex = 0;
		}
		if((data == '\r') || (data == '\n'))
		{
			if(curLineIndex > 0)
			{
				InfoLogger_writeLine(curLine,curLineIndex,0);
				if(strcmp((char*)curLine,"OK") == 0)
				{
					handleOK();
				}
				else
				{
					if(strcmp((char*)curLine,"ERROR") == 0)
					{
						handleERROR();
					}
				}
			}
			switch(readType)
			{
				case BT_READ_DEVICE_NAME: 	if(writeIndex < BLUEMOD_NAME_SIZE)
											{
												memcpy (btDeviceList[curBtIndex].name, parameter, writeIndex);
											}
								break;
				case BT_READ_SERV_UUID: 	if(writeIndex < 50)
											{
												memcpy(curDeviceService[curServiceIndex].uuid, parameter, writeIndex);
											}
											curServiceIndex++;
								break;
				default:
					break;
			}
			curLineIndex = 0;
			writeIndex = 0;
			memset(curLine,0,sizeof(curLine));
			readType = BT_READ_NOTHING;
		}
		else
		{
			if(curLineIndex < MAX_CHAR_PER_LINE) curLine[curLineIndex++] = data;

			if(data == ':')
			{
				switch(checkIndicators(curLine))
				{
					case DEVICE_INDICATOR: 		readType = BT_READ_DEVICE_ADDR;
						break;
					case CONNECTION_INDICATOR:  readType = BT_READ_CON_DETAILS;
						break;
					case SERVICE_INDICATOR: 	readType = BT_READ_SERV_HANDLE;
						break;
					default:
						break;
				}
				writeIndex = 0;
				memset(parameter,0,sizeof(parameter));
			}
			else
			{
				if(data == ',')					/* parameter end */
				{
					switch(readType)
					{
						case BT_READ_DEVICE_ADDR:  if(writeIndex < BLUEMOD_ADDR_SIZE-1)
													{
														if(firstDevice)
														{
															firstDevice = 0;
														}
														else
														{
															curBtIndex++;
														}
														parameter[writeIndex-1] = 0;	/*remove 'p' from address */
														strcpy((char*)btDeviceList[curBtIndex].address, (char*)parameter);
													}
													readType = BT_READ_DEVICE_RSSI;
								break;
						case BT_READ_DEVICE_RSSI:	if(writeIndex < BLUEMOD_RSSI_SIZE-1)
													{
														strcpy((char*)btDeviceList[curBtIndex].rssi, (char*)parameter);
													}
													readType = BT_READ_DEVICE_NAME;
								break;
						case BT_READ_DEVICE_NAME: 	if(writeIndex < BLUEMOD_NAME_SIZE-1)
													{
														memcpy(btDeviceList[curBtIndex].name, parameter, writeIndex);
													}
													readType = BT_READ_NOTHING;
										break;
						case BT_READ_CON_DETAILS:	connHandle = parameter[0];
													heartbeatState = SENSOR_HB_SERVICES;
													readType = BT_READ_NOTHING;
										break;
						case BT_READ_SERV_HANDLE:	curDeviceService[curServiceIndex].handle = parameter[0];
													readType = BT_READ_SERV_START;
										break;
						case BT_READ_SERV_START: 	if(writeIndex < 6)
													{
														memcpy(curDeviceService[curServiceIndex].start, parameter, writeIndex);
													}
													readType = BT_READ_SERV_END;
										break;
						case BT_READ_SERV_END: 		if(writeIndex < 6)
													{
														memcpy(curDeviceService[curServiceIndex].end, parameter, writeIndex);
													}
													readType = BT_READ_SERV_UUID;
										break;

						default:					readType = BT_READ_NOTHING;
							break;
					}
					writeIndex = 0;
					memset(parameter,0 , sizeof(parameter));
				}
				else
				{
				//	if(readType != BT_READ_NOTHING)
					{
						parameter[writeIndex++] = data;
					}
				}
			}
		}
		data = UART_getChar();
	}
	return complete;
}

sensorHeartbeat_State_t cv_heartbeat_getState()
{
	return heartbeatState;
}

void openEdit_Heartbeat(void)
{
    SSettings *settings = settingsGetPointer();

    char text[32];
    snprintf(text, 32, "\001%c%c", TXT_2BYTE, TXT2BYTE_Pulse);
    write_topline(text);

    set_globalState(StMOption_Heartbeat);
    resetMenuEdit(CLUT_MenuPageCvOption);

    snprintf(text, 32, "%c%c", TXT_2BYTE, TXT2BYTE_SensorDetect);
    write_field_button(StMOption_Heartbeat, 30, 299, ME_Y_LINE1, &FontT48, text);

    write_buttonTextline(TXT2BYTE_ButtonMinus, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonPlus);

    setEvent(StMOption_Heartbeat, (uint32_t)OnAction_Heartbeat);
}

static uint8_t OnAction_Heartbeat(uint32_t editId, uint8_t blockNumber, uint8_t digitNumber, uint8_t digitContent, uint8_t action)
{
	switch(heartbeatState)
	{
		case SENSOR_HB_OFFLINE:
				HAL_UART_AbortReceive_IT(&UartHandle);
				MX_UART_BT_Init_DMA();
				UART_StartDMARx();
				heartbeatState = SENSOR_HB_ENABLE_BLE;
				startDetection_ms = HAL_GetTick();
				curBtIndex = 0;
				memset(btDeviceList, 0, sizeof(btDeviceList));
			break;

		default:
			break;
	}
    return UNSPECIFIC_RETURN;
}

void cv_heartbeat_Control(void)
{
	static uint8_t action = 0;
	static uint8_t retry = 0;
	static uint8_t lastState = 0;
	static uint8_t devicesIndex = 0;


	char cmd[50];

	cmd[0] = 0;

	getDeviceList();

	if(action == 3)
	{
		action = 0;
		switch(heartbeatState)
		{
			case SENSOR_HB_ENABLE_BLE:	HAL_Delay(1000);
										snprintf(cmd, sizeof(cmd), "+++" ); //"AT+UBTD=2,1,5000\r\n"
										InfoLogger_writeLine((uint8_t*)cmd,3,1);
										HAL_UART_Transmit(&UartHandle, (uint8_t*)cmd, 3, 5000);
										HAL_Delay(1000);
										cmd[0] = 0;
				break;
			case SENSOR_HB_CHECK_CONFIG:  snprintf(cmd, sizeof(cmd), "AT+UBTCFG=2\r\n" );	// AT+UBTLE?

#if 0
				if(settingsGetPointer()->dive_mode == DIVEMODE_OC)
				{
					snprintf(cmd, sizeof(cmd), "AT+UBTLE=2\r\n" ); //+UBTLE=1 //"AT+UBTD=2,1,5000\r\n"
				}
				else
				{
					snprintf(cmd, sizeof(cmd), "AT+UBTLE=3\r\n" ); //+UBTLE=1 //"AT+UBTD=2,1,5000\r\n"
				}
#endif
				break;
			case SENSOR_HB_DISCOVER:	if(lastState != SENSOR_HB_DISCOVER)
										{
											snprintf(cmd, sizeof(cmd), "AT+UBTD=2,1\r\n" ); //+UBTLE=1 //"AT+UBTD=2,1,5000\r\n"
											devicesIndex = 0;
										}

										//snprintf(cmd, sizeof(cmd), "AT&W\r\n" ); //  AT+UBTD=2,1\r\n "AT+UBTD=2,1,5000\r\n"
				break;
#if 0
			case SENSOR_HB_RESTART:	snprintf(cmd, sizeof(cmd), "AT+UBTD=2,1\r\n" ); //+UBTLE=1 //"AT+UBTD=2,1,5000\r\n"

									//	snprintf(cmd, sizeof(cmd), "AT+CPWROFF\r\n" ); //  AT+UBTD=2,1\r\n "AT+UBTD=2,1,5000\r\n"
				break;
#endif
			case SENSOR_HB_CONNECT:		if(curBtIndex != devicesIndex)
										{
											snprintf(cmd, sizeof(cmd), "AT+UBTACLC=%s\r\n",btDeviceList[devicesIndex].address);
											devicesIndex++;
										}
				break;
			case SENSOR_HB_SERVICES:	if((connHandle >= '0') && (connHandle <= '9') && (lastState != SENSOR_HB_SERVICES))
										{
											snprintf(cmd, sizeof(cmd), "AT+UBTGDP=0%c\r\n",connHandle);
										}
										else
										{
											heartbeatState = SENSOR_HB_OFFLINE;
										}
				break;

			default:
				break;
		}
		if(cmd[0] != 0)
		{
			{
				InfoLogger_writeLine((uint8_t*)cmd,strlen(cmd),1);
				HAL_UART_Transmit(&UartHandle, (uint8_t*)cmd, strlen(cmd), 5000);
				retry++;
				if(retry == 3)
				{
					heartbeatState = SENSOR_HB_OFFLINE;
				}
			}
		}
		if(lastState != heartbeatState)
		{
			lastState = heartbeatState;
			retry = 0;
		}
	}
	else
	{
		action++;
	}
}
void refresh_Heartbeat(void)
{
	char text[32];
	uint8_t index = 0;

	snprintf(text, 32, "\001%c%c", TXT_2BYTE, TXT2BYTE_Pulse);
	write_topline(text);

	switch(heartbeatState)
	{
		case SENSOR_HB_OFFLINE:
		default:							snprintf(text, 32, "%c%c", TXT_2BYTE, TXT2BYTE_SensorDetect);
											write_label_var(30, 299, ME_Y_LINE1, &FontT48, text);

	    									if(curBtIndex > 0)
	    									{
	    										while((index < curBtIndex) && (index < 3))
	    										{
													snprintf(text, 40, "%s", btDeviceList[index].address);
													write_label_var(  30, 300,  ME_Y_LINE3 + (index * ME_Y_LINE_STEP), &FontT48, text);
													index++;
	    										}
	    									}
			break;
		case SENSOR_HB_ENABLE_BLE:			snprintf(text, 32, "Activate BLE");
											write_label_var(  30, 300, ME_Y_LINE1, &FontT48, text);
			break;
		case SENSOR_HB_DISCOVER:
	    									snprintf(text, 32, "Searching");
	    									write_label_var(  30, 300, ME_Y_LINE1, &FontT48, text);

	    									if(curBtIndex > 0)
	    									{
	    										while((index < curBtIndex) && (index < 4))
	    										{
													snprintf(text, 40, "%s", btDeviceList[index].address);
													write_label_var(  30, 300,  ME_Y_LINE2 + (index * ME_Y_LINE_STEP), &FontT48, text);
													index++;
	    										}
	    									}
			break;
	}

    write_buttonTextline(TXT2BYTE_ButtonMinus, TXT2BYTE_ButtonEnter, TXT2BYTE_ButtonPlus);
}

