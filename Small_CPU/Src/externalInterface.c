/**
  ******************************************************************************
  * @file    externalInterface.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    07-Nov-2020
  * @brief   Interface functionality to proceed external analog signal via i2c connection
  *
  @verbatim
  ==============================================================================
                ##### stm32f4xx_hal_i2c.c modification #####
  ==============================================================================
	The LTC2942 requires an repeated start condition without stop condition
	for data reception.

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

#include <math.h>
#include <string.h>
#include "i2c.h"
#include "externalInterface.h"
#include "scheduler.h"
#include "uart.h"
#include "data_exchange.h"

extern SGlobal global;
extern UART_HandleTypeDef huart1;

#define ADC_ANSWER_LENGTH	(5u)		/* 3424 will provide addr + 4 data bytes */
#define ADC_TIMEOUT			(10u)		/* conversion stuck for unknown reason => restart */
#define ADC_REF_VOLTAGE_MV	(2048.0f)	/* reference voltage of MPC3424*/

#define ADC_START_CONVERSION		(0x80)
#define ADC_GAIN_4					(0x02)
#define ADC_GAIN_4_VALUE			(4.0f)
#define ADC_GAIN_8					(0x03)
#define ADC_GAIN_8_VALUE			(8.0f)
#define ADC_RESOLUTION_16BIT		(0x08)
#define ADC_RESOLUTION_16BIT_VALUE	(16u)
#define ADC_RESOLUTION_18BIT		(0x0C)
#define ADC_RESOLUTION_18BIT_VALUE	(18u)

#define ANSWER_CONFBYTE_INDEX		(4u)

static uint8_t activeChannel = 0;			/* channel which is in request */
static uint8_t recBuf[ADC_ANSWER_LENGTH];
static uint8_t timeoutCnt = 0;
static uint8_t externalInterfacePresent = 0;

float externalChannel_mV[MAX_ADC_CHANNEL];
static uint8_t  externalV33_On = 0;
static uint8_t  externalADC_On = 0;
static uint8_t  externalUART_Protocol = 0;
static uint16_t externalCO2Value;
static uint16_t externalCO2SignalStrength;
static uint16_t  externalCO2Status = 0;

static uint8_t sensorDataId = 0;
static SSensorDataDiveO2 sensorDataDiveO2;
static externalInterfaceAutoDetect_t externalAutoDetect = DETECTION_OFF;
static externalInterfaceSensorType SensorMap[EXT_INTERFACE_SENSOR_CNT] ={ SENSOR_ANALOG, SENSOR_ANALOG, SENSOR_ANALOG, SENSOR_NONE, SENSOR_NONE};
static externalInterfaceSensorType tmpSensorMap[EXT_INTERFACE_SENSOR_CNT];
static externalInterfaceSensorType MasterSensorMap[EXT_INTERFACE_SENSOR_CNT];


void externalInterface_Init(void)
{
	activeChannel = 0;
	timeoutCnt = 0;
	externalInterfacePresent = 0;
	if(externalInterface_StartConversion(activeChannel) == HAL_OK)
	{
		externalInterfacePresent = 1;
		global.deviceDataSendToMaster.hw_Info.extADC = 1;
	}
	global.deviceDataSendToMaster.hw_Info.checkADC = 1;

	externalInterface_InitDatastruct();
}

void externalInterface_InitDatastruct(void)
{
	uint8_t index = 0;
	/* init data values */
	externalV33_On = 0;
	externalCO2Value = 0;
	externalCO2SignalStrength = 0;
	externalCO2Status = 0;
	externalAutoDetect = DETECTION_OFF;

	for(index = 0; index < MAX_ADC_CHANNEL; index++)
	{
		externalChannel_mV[index] = 0.0;
	}
}


uint8_t externalInterface_StartConversion(uint8_t channel)
{
	uint8_t retval = 0;
	uint8_t confByte = 0;

	if(channel < MAX_ADC_CHANNEL)
	{
		confByte = ADC_START_CONVERSION | ADC_RESOLUTION_16BIT | ADC_GAIN_8;
		confByte |= channel << 5;
		retval = I2C_Master_Transmit(DEVICE_EXTERNAL_ADC, &confByte, 1);
	}
	return retval;
}

/* Check if conversion is done and trigger measurement of next channel */
uint8_t externalInterface_ReadAndSwitch()
{
	uint8_t retval = EXTERNAL_ADC_NO_DATA;
	uint8_t nextChannel;
	uint8_t* psensorMap = externalInterface_GetSensorMapPointer(0);

	if(externalADC_On)
	{
		if(I2C_Master_Receive(DEVICE_EXTERNAL_ADC, recBuf, ADC_ANSWER_LENGTH) == HAL_OK)
		{
			if((recBuf[ANSWER_CONFBYTE_INDEX] & ADC_START_CONVERSION) == 0)		/* !ready set => received data contains new value */
			{
				retval = activeChannel;										/* return channel number providing new data */
				nextChannel = activeChannel + 1;
				if(nextChannel == MAX_ADC_CHANNEL)
				{
					nextChannel = 0;
				}

				while((psensorMap[nextChannel] != SENSOR_ANALOG) && (nextChannel != activeChannel))
				{
					if(nextChannel == MAX_ADC_CHANNEL)
					{
						nextChannel = 0;
					}
					else
					{
						nextChannel++;
					}
				}

				activeChannel = nextChannel;
				externalInterface_StartConversion(activeChannel);
				timeoutCnt = 0;
			}
			else
			{
				if(timeoutCnt++ >= ADC_TIMEOUT)
				{
					externalInterface_StartConversion(activeChannel);
					timeoutCnt = 0;
				}
			}
		}
		else		/* take also i2c bus disturb into account */
		{
			if(timeoutCnt++ >= ADC_TIMEOUT)
			{
				externalInterface_StartConversion(activeChannel);
				timeoutCnt = 0;
			}
		}
	}
	return retval;
}
float externalInterface_CalculateADCValue(uint8_t channel)
{
	int32_t rawvalue = 0;
	float retValue = 0.0;
	if(channel < MAX_ADC_CHANNEL)
	{

		rawvalue = ((recBuf[0] << 16) | (recBuf[1] << 8) | (recBuf[2]));

		switch(recBuf[3] & 0x0C)			/* confbyte => Resolution bits*/
		{
			case ADC_RESOLUTION_16BIT:		rawvalue = rawvalue >> 8;										/* only 2 databytes received shift out confbyte*/
											if(rawvalue & (0x1 << (ADC_RESOLUTION_16BIT_VALUE-1)))			/* MSB set => negative number */
											{
												rawvalue |= 0xFFFF0000; 	/* set MSB for int32 */
											}
											else
											{
												rawvalue &= 0x0000FFFF;
											}
											externalChannel_mV[channel] = ADC_REF_VOLTAGE_MV * 2.0 / (float) pow(2,ADC_RESOLUTION_16BIT_VALUE);	/* calculate bit resolution */
				break;
			case ADC_RESOLUTION_18BIT:		if(rawvalue & (0x1 << (ADC_RESOLUTION_18BIT_VALUE-1)))			/* MSB set => negative number */
											{
												rawvalue |= 0xFFFE0000; 	/* set MSB for int32 */
											}
											externalChannel_mV[channel] = ADC_REF_VOLTAGE_MV * 2.0 / (float) pow(2,ADC_RESOLUTION_18BIT_VALUE);	/* calculate bit resolution */
							break;
			default: rawvalue = 0;
				break;
		}
		externalChannel_mV[channel] = externalChannel_mV[channel] * rawvalue / ADC_GAIN_8_VALUE;
		retValue = externalChannel_mV[channel];
	}
	return retValue;
}
float getExternalInterfaceChannel(uint8_t channel)
{
	float retval = 0;

	if(channel < MAX_ADC_CHANNEL)
	{
		retval = externalChannel_mV[channel];
	}
	return retval;
}

uint8_t setExternalInterfaceChannel(uint8_t channel, float value)
{
	uint8_t retval = 0;

	if(channel < MAX_ADC_CHANNEL)
	{
		externalChannel_mV[channel] = value;
		retval = 1;
	}
	return retval;
}

void externalInterface_InitPower33(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	GPIO_InitStructure.Pin = GPIO_PIN_7;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_SET);
}


uint8_t externalInterface_isEnabledPower33()
{
	return externalV33_On;
}

uint8_t externalInterface_isEnabledADC()
{
	return externalADC_On;
}

uint8_t externalInterface_GetUARTProtocol()
{
	return externalUART_Protocol;
}

void externalInterface_SwitchPower33(uint8_t state)
{
	if(state != externalV33_On)
	{
		if(state)
		{
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_RESET);
			externalV33_On = 1;
		}
		else
		{
			if(externalAutoDetect == DETECTION_OFF)
			{
				HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_SET);
				externalV33_On = 0;
				externalInterface_SetCO2Value(0);
				externalInterface_SetCO2SignalStrength(0);
			}
		}
	}
}
void externalInterface_SwitchADC(uint8_t state)
{
	uint8_t loop = 0;
	if((state) && (externalInterfacePresent))
	{
		if(externalADC_On == 0)
		{
			activeChannel = 0;
			externalInterface_StartConversion(activeChannel);
			externalADC_On = 1;
		}
	}
	else
	{
		if(externalAutoDetect == DETECTION_OFF)			/* block deactivation requests if auto detection is active */
		{
			externalADC_On = 0;
			for(loop = 0; loop < MAX_ADC_CHANNEL; loop++)
			{
				externalChannel_mV[loop] = 0;
			}
		}
	}
}

void externalInterface_SwitchUART(uint8_t protocol)
{
	switch(protocol)
	{
		case 0:
		case (EXT_INTERFACE_UART_CO2 >> 8):
		case (EXT_INTERFACE_UART_O2 >> 8):
		case (EXT_INTERFACE_UART_SENTINEL >> 8):
				if((externalAutoDetect <= DETECTION_START) || ((protocol == EXT_INTERFACE_UART_O2 >> 8) && (externalAutoDetect == DETECTION_DIGO2))
#ifdef ENABLE_CO2_SUPPORT
															|| ((protocol == EXT_INTERFACE_UART_CO2 >> 8) && (externalAutoDetect == DETECTION_CO2))
#endif
#ifdef ENABLE_SENTINEL_MODE
														   || ((protocol == EXT_INTERFACE_UART_SENTINEL >> 8) && (externalAutoDetect == DETECTION_SENTINEL))
#endif
					)
				{
					sensorDataId = 0;
					externalUART_Protocol = protocol;
					MX_USART1_UART_DeInit();
					if( protocol != 0)
					{
						MX_USART1_UART_Init();
					}
				}
			break;
		default:
			break;
	}
}

void externalInterface_SetCO2Value(uint16_t CO2_ppm)
{
	externalCO2Value = CO2_ppm;
}

void externalInterface_SetCO2SignalStrength(uint16_t LED_qa)
{
	externalCO2SignalStrength = LED_qa;
}

uint16_t externalInterface_GetCO2Value(void)
{
	return externalCO2Value;
}

uint16_t externalInterface_GetCO2SignalStrength(void)
{
	return externalCO2SignalStrength;
}


void externalInterface_SetCO2State(uint16_t state)
{
	externalCO2Status = state;
}

uint16_t externalInterface_GetCO2State(void)
{
	return externalCO2Status;
}


uint8_t externalInterface_GetSensorData(uint8_t* pDataStruct)
{

	if((pDataStruct != NULL) && sensorDataId != 0)
	{
		memcpy(pDataStruct, &sensorDataDiveO2, sizeof(sensorDataDiveO2));
	}
	return sensorDataId;
}

void externalInterface_SetSensorData(uint8_t dataId, uint8_t* pDataStruct)
{
	if(pDataStruct != NULL)
	{
		if(dataId != 0)
		{
			memcpy(&sensorDataDiveO2, pDataStruct, sizeof(sensorDataDiveO2));
		}
		else
		{
			memset(&sensorDataDiveO2,0,sizeof(sensorDataDiveO2));
		}
		sensorDataId = dataId;
	}
}

void externalInface_SetSensorMap(uint8_t* pMap)
{
	if(pMap != NULL)
	{
		memcpy(MasterSensorMap, pMap, 5);		/* the map is not directly copied. Copy is done via cmd request */
	}

}
uint8_t* externalInterface_GetSensorMapPointer(uint8_t finalMap)
{
	uint8_t* pret;

	if((externalAutoDetect != DETECTION_OFF) && (!finalMap))
	{
		pret = tmpSensorMap;
	}
	else
	{
		pret = SensorMap;
	}
	return pret;
}

void externalInterface_AutodetectSensor()
{
	static uint8_t	sensorIndex = 0;
	uint8_t index = 0;

	if(externalAutoDetect != DETECTION_OFF)
	{
		switch(externalAutoDetect)
		{
			case DETECTION_INIT:	sensorIndex = 0;
									tmpSensorMap[0] = SENSOR_OPTIC;
									tmpSensorMap[1] = SENSOR_OPTIC;
									tmpSensorMap[2] = SENSOR_OPTIC;
									tmpSensorMap[3] = SENSOR_NONE;
									tmpSensorMap[4] = SENSOR_NONE;

									if(externalInterfacePresent)
									{
										externalInterface_SwitchPower33(0);
										externalInterface_SwitchUART(0);
										for(index = 0; index < MAX_ADC_CHANNEL; index++)
										{
											externalChannel_mV[index] = 0;
										}
										externalAutoDetect = DETECTION_START;
									}
									else
									{
										externalAutoDetect = DETECTION_DONE;	/* without external interface O2 values may only be received via optical port => return default sensor map */
									}
				break;
			case DETECTION_START:		tmpSensorMap[0] = SENSOR_ANALOG;
										tmpSensorMap[1] = SENSOR_ANALOG;
										tmpSensorMap[2] = SENSOR_ANALOG;
										externalInterface_SwitchPower33(1);
										externalInterface_SwitchADC(1);
										externalAutoDetect = DETECTION_ANALOG1;
				break;
			case DETECTION_ANALOG1:	externalAutoDetect = DETECTION_ANALOG2;		/* do a second loop to make sure all adc channels could be processed */
				break;
			case DETECTION_ANALOG2:	for(index = 0; index < MAX_ADC_CHANNEL; index++)
									{
										if(externalChannel_mV[index] > MIN_ADC_VOLTAGE_MV)
										{
											tmpSensorMap[sensorIndex++] = SENSOR_ANALOG;
										}
										else
										{
											tmpSensorMap[sensorIndex++] = SENSOR_NONE;
										}
									}
									externalAutoDetect = DETECTION_DIGO2;
									externalInterface_SwitchUART(EXT_INTERFACE_UART_O2 >> 8);
				break;
			case DETECTION_DIGO2:	if(UART_isDigO2Connected())
									{
										for(index = 0; index < 3; index++)	/* lookup a channel which may be used by digO2 */
										{
											if(tmpSensorMap[index] == SENSOR_NONE)
											{
												break;
											}
										}
										if(index == 3)
										{
											tmpSensorMap[2] = SENSOR_DIGO2;  /* digital sensor overwrites ADC */
										}
										else
										{
											tmpSensorMap[index] = SENSOR_DIGO2;
										}

										UART_setTargetChannel(index);
										/* tmpSensorMap[sensorIndex++] = SENSOR_DIGO2; */
									}
									externalAutoDetect++;
#ifdef ENABLE_CO2_SUPPORT
									externalInterface_SwitchUART(EXT_INTERFACE_UART_CO2 >> 8);
				break;
			case DETECTION_CO2:		if(UART_isCO2Connected())
									{
										for(index = 0; index < 3; index++)	/* lookup a channel which may be used by CO2*/
										{
											if(tmpSensorMap[index] == SENSOR_NONE)
											{
												break;
											}
										}
										if(index == 3)
										{
											tmpSensorMap[sensorIndex++] = SENSOR_CO2;  /* place Co2 sensor behind O2 sensors (not visible) */
										}
										else
										{
											tmpSensorMap[index] = SENSOR_CO2;
										}

									}
									externalAutoDetect++;
#endif
#ifdef ENABLE_SENTINEL_MODE
									externalInterface_SwitchUART(EXT_INTERFACE_UART_SENTINEL >> 8);
									UART_StartDMA_Receiption();
				break;

			case DETECTION_SENTINEL:
			case DETECTION_SENTINEL2:
									if(UART_isSentinelConnected())
									{
										for(index = 0; index < 3; index++)	/* Sentinel is occupiing all sensor slots */
										{
											tmpSensorMap[index] = SENSOR_SENTINEL;
										}
										sensorIndex = 3;
									}
									externalAutoDetect++;
#endif
				break;
			case DETECTION_DONE:	for(index = 0; index < EXT_INTERFACE_SENSOR_CNT; index++)
									{
										if(tmpSensorMap[index] != SENSOR_NONE)
										{
											break;
										}
									}

									if(index != EXT_INTERFACE_SENSOR_CNT)		/* return default sensor map if no sensor at all has been detected */
									{
										while(sensorIndex < EXT_INTERFACE_SENSOR_CNT)
										{
											tmpSensorMap[sensorIndex++] = SENSOR_NONE;
										}
									}
									else
									{
										tmpSensorMap[0] = SENSOR_OPTIC;
										tmpSensorMap[1] = SENSOR_OPTIC;
										tmpSensorMap[2] = SENSOR_OPTIC;
									}
									memcpy(SensorMap, tmpSensorMap, sizeof(tmpSensorMap));
									externalAutoDetect = DETECTION_OFF;
				break;
			default:
				break;
		}
	}
}


void externalInterface_ExecuteCmd(uint16_t Cmd)
{
	char cmdString[10];
	uint8_t cmdLength = 0;
	uint8_t index;

	switch(Cmd & 0x00FF)		/* lower byte is reserved for commands */
	{
		case EXT_INTERFACE_AUTODETECT:	externalAutoDetect = DETECTION_INIT;
										for(index = 0; index < 3; index++)
										{
											SensorMap[index] = SENSOR_SEARCH;
										}
			break;
		case EXT_INTERFACE_CO2_CALIB:	cmdLength = snprintf(cmdString, 10, "G\r\n");
			break;
		case EXT_INTERFACE_COPY_SENSORMAP:	if(externalAutoDetect == DETECTION_OFF)
											{
												memcpy(SensorMap, MasterSensorMap, 5);
												for(index = 0; index < 3; index++)
												{
													if(SensorMap[index] == SENSOR_DIGO2)
													{
														break;
													}
												}
												UART_setTargetChannel(index); /* if no slot for digO2 is found then the function will be called with an invalid parameter causing the overwrite function to fail */
											}
			break;
		default:
			break;
	}
	if(cmdLength != 0)
	{
		HAL_UART_Transmit(&huart1,(uint8_t*)cmdString,cmdLength,10);
	}
	return;
}

