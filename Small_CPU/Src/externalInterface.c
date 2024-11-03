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
#include "data_central.h"
#include "i2c.h"
#include "externalInterface.h"
#include "scheduler.h"
#include "uart.h"
#include "data_exchange.h"
#include "pressure.h"
#include "uartProtocol_O2.h"
#include "uartProtocol_Co2.h"
#include "uartProtocol_Sentinel.h"
#include "uartProtocol_GNSS.h"

extern SGlobal global;
extern UART_HandleTypeDef huart1;

#define ADC_ANSWER_LENGTH		(5u)		/* 3424 will provide addr + 4 data bytes */
#define ADC_TIMEOUT				(10u)		/* conversion stuck for unknown reason => restart */
#define ADC_REF_VOLTAGE_MV		(2048.0f)	/* reference voltage of MPC3424*/
#define ADC_CYCLE_INTERVAL_MS	(1000u)		/* start adc read out once per second*/

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

#define LOOKUP_CO2_CORR_TABLE_SCALE	(1000u)
#define LOOKUP_CO2_CORR_TABLE_MAX	(30000u)

#define REQUEST_INT_SENSOR_MS	(1500)		/* Minimum time interval for cyclic sensor data requests per sensor (UART mux) */
#define COMMAND_TX_DELAY		(30u)		/* The time the sensor needs to recover from a invalid command request */
#define TIMEOUT_SENSOR_ANSWER	(300)		/* Time till a request is repeated if no answer was received */

#define activeSensorId (activeUartChannel + EXT_INTERFACE_MUX_OFFSET)	/* Used if UART channels are applied to Sensor map */

static uint8_t activeChannel = 0;			/* channel which is in request */
static uint8_t recBuf[ADC_ANSWER_LENGTH];
static uint8_t timeoutCnt = 0;
static uint8_t externalInterfacePresent = 0;
static uint8_t delayAdcConversion = 0;
static uint32_t startTickADC = 0;

float externalChannel_mV[MAX_ADC_CHANNEL];
static uint8_t  externalV33_On = 0;
static uint8_t  externalADC_On = 0;
static uint8_t  externalUART_Protocol = 0;
static uint16_t externalCO2Value;
static uint16_t externalCO2SignalStrength;
static uint16_t externalCO2Status = 0;
static float 	externalCO2Scale = 0.0;

static uint8_t lastSensorDataId = 0;
static SSensorDataDiveO2 sensorDataDiveO2[EXT_INTERFACE_SENSOR_CNT];
static externalInterfaceAutoDetect_t externalAutoDetect = DETECTION_OFF;
static externalInterfaceSensorType SensorMap[EXT_INTERFACE_SENSOR_CNT] ={ SENSOR_OPTIC, SENSOR_OPTIC, SENSOR_OPTIC, SENSOR_NONE, SENSOR_NONE};
static externalInterfaceSensorType tmpSensorMap[EXT_INTERFACE_SENSOR_CNT];
static externalInterfaceSensorType MasterSensorMap[EXT_INTERFACE_SENSOR_CNT];
static externalInterfaceSensorType foundSensorMap[EXT_INTERFACE_SENSOR_CNT];
static uint8_t	Mux2ADCMap[MAX_ADC_CHANNEL];
static uint8_t externalInterface_SensorState[EXT_INTERFACE_SENSOR_CNT];

static float LookupCO2PressureCorrection[LOOKUP_CO2_CORR_TABLE_MAX / LOOKUP_CO2_CORR_TABLE_SCALE];		/* lookup table for pressure compensation values */

static uint16_t externalInterfaceMuxReqIntervall = 0xffff;		/* delay between switching from one MUX channel to the next */
static uint8_t activeUartChannel = 0xff;


static void externalInface_MapUartToLegacyADC(uint8_t* pMap);
static void externalInterface_CheckBaudrate(uint8_t sensorType);

void externalInterface_Init(void)
{
	uint16_t index;
	uint16_t coeff;
	activeChannel = 0;
	timeoutCnt = 0;
	delayAdcConversion = 0;
	if(externalInterface_StartConversion(activeChannel) == HAL_OK)
	{
		externalInterfacePresent = 1;
		global.deviceDataSendToMaster.hw_Info.extADC = 1;
	}
	global.deviceDataSendToMaster.hw_Info.checkADC = 1;



/* Create a lookup table based on GSS application note AN001: PRESSURE COMPENSATION OF A CO2 SENSOR */
/* The main purpose of the sensor in the dive application is to be a warning indicator */
/* => no exact values necessary => a lookup table with 1000ppm scaling should be sufficient */
	LookupCO2PressureCorrection [0] = -0.0014;
	for(index = 1; index < (LOOKUP_CO2_CORR_TABLE_MAX / LOOKUP_CO2_CORR_TABLE_SCALE); index++)
	{
		coeff = index * LOOKUP_CO2_CORR_TABLE_SCALE;
		LookupCO2PressureCorrection[index] = 2.811*pow(10,-38)*pow(coeff,6)- 9.817*pow(10,-32)*pow(coeff,5)+1.304*pow(10,-25)*pow(coeff,4)-8.216*pow(10,-20)*pow(coeff,3)+2.311*pow(10,-14)*pow(coeff,2) - 2.195*pow(10,-9)*coeff - 1.471*pow(10,-3);
	}
	externalInterface_InitDatastruct();
}

void externalInterface_InitDatastruct(void)
{
	uint8_t index = 0;
	/* init data values */
	externalUART_Protocol = 0;
	externalCO2Value = 0;
	externalCO2SignalStrength = 0;
	externalCO2Status = 0;
	externalCO2Scale = 0.0;
	externalAutoDetect = DETECTION_OFF;

	for(index = 0; index < MAX_ADC_CHANNEL; index++)
	{
		externalChannel_mV[index] = 0.0;
	}
	memset(externalInterface_SensorState,UART_COMMON_INIT,sizeof(externalInterface_SensorState));
	externalInface_MapUartToLegacyADC(SensorMap);
	activeUartChannel = 0xFF;
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
		if(time_elapsed_ms(startTickADC, HAL_GetTick()) >  ADC_CYCLE_INTERVAL_MS)
		{
			if(delayAdcConversion)
			{
				if(UART_isComActive(activeUartChannel) == 0)
				{
					externalInterface_StartConversion(activeChannel);
					delayAdcConversion = 0;
				}
			}
			else if(I2C_Master_Receive(DEVICE_EXTERNAL_ADC, recBuf, ADC_ANSWER_LENGTH) == HAL_OK)
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
							startTickADC = HAL_GetTick();
						}
						else
						{
							nextChannel++;
						}
					}

					activeChannel = nextChannel;
					if(activeChannel == 0)
					{
						delayAdcConversion = 1;		/* wait for next cycle interval */
					}
					else
					{
						if(UART_isComActive(activeUartChannel) == 0)
						{
							externalInterface_StartConversion(activeChannel);
						}
						else
						{
							delayAdcConversion = 1;
						}
					}
					timeoutCnt = 0;
				}
			}

		if(timeoutCnt++ >= ADC_TIMEOUT)
		{
			externalInterface_StartConversion(activeChannel);
			delayAdcConversion = 0;
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
	uint8_t localId = channel;
	uint8_t index = 0;

	if(localId >= MAX_ADC_CHANNEL)		/* at the moment sensor visualization is focused on the three ADC channels => map Mux sensors */
	{
		for(index = 0; index < MAX_ADC_CHANNEL; index++)
		{
			if(Mux2ADCMap[index] == localId)
			{
				localId = index;
				break;
			}
		}
	}

	if(localId < MAX_ADC_CHANNEL)
	{
		externalChannel_mV[localId] = value;
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
			startTickADC = HAL_GetTick();
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
		case EXT_INTERFACE_UART_OFF:
		case EXT_INTERFACE_UART_CO2:
		case EXT_INTERFACE_UART_O2:
		case EXT_INTERFACE_UART_SENTINEL:
		case EXT_INTERFACE_UART_GNSS:
				if((externalAutoDetect <= DETECTION_START)
					|| ((protocol == EXT_INTERFACE_UART_O2) && (externalAutoDetect >= DETECTION_UARTMUX) && (externalAutoDetect <= DETECTION_DIGO2_3))

#ifdef ENABLE_CO2_SUPPORT
					|| ((externalAutoDetect >= DETECTION_CO2_0) && (externalAutoDetect <= DETECTION_CO2_3))
#endif
#ifdef ENABLE_GNSS_SUPPORT
					|| ((externalAutoDetect >= DETECTION_GNSS_0) && (externalAutoDetect <= DETECTION_GNSS_3))
#endif

#ifdef ENABLE_SENTINEL_MODE
														   || ((protocol == EXT_INTERFACE_UART_SENTINEL) && (externalAutoDetect == DETECTION_SENTINEL))
#endif
					)
				{
					lastSensorDataId = 0;
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

uint8_t externalInterface_GetActiveUartSensor()
{
	return activeUartChannel;
}

void externalInterface_SetSensorState(uint8_t sensorIdx, uint8_t state)
{
	if(sensorIdx < EXT_INTERFACE_SENSOR_CNT)
	{
		externalInterface_SensorState[sensorIdx] = state;
	}
}

uint8_t externalInterface_GetSensorState(uint8_t sensorIdx)
{
	uint8_t ret = COMMON_SENSOR_STATE_INVALID;
	if(sensorIdx < EXT_INTERFACE_SENSOR_CNT)
	{
		ret = externalInterface_SensorState[sensorIdx];
	}
	return ret;
}

/* The supported sensors from GSS have different scaling factors depending on their accuracy. The factor may be read out of the sensor */
void externalInterface_SetCO2Scale(float CO2Scale)
{
	if((CO2Scale == 10) || (CO2Scale == 100))
	{
		externalCO2Scale = CO2Scale;
	}
}
float externalInterface_GetCO2Scale()
{
	return externalCO2Scale;
}

void externalInterface_SetCO2Value(uint16_t CO2_ppm)
{
	float local_ppm = CO2_ppm * externalCO2Scale;

#ifndef ENABLE_EXTERNAL_PRESSURE
	float local_corr = 0.0;

	if (local_ppm >= LOOKUP_CO2_CORR_TABLE_MAX)
	{
		local_corr = -0.0014;
	}
	else
	{
		local_corr = LookupCO2PressureCorrection[((uint16_t) (local_ppm / LOOKUP_CO2_CORR_TABLE_SCALE))];
	}
	local_ppm = local_ppm / (1.0 + (local_corr * (get_surface_mbar() - get_pressure_mbar())));
#else
/* The external pressure value is passed via ADC channel2 and calibration is done at firmware => just forward sensor data */
/* compensation is done at firmware side. This is for testing only. Take care the the same algorithm is taken as used for the lookup table */
#endif
	externalCO2Value = local_ppm / externalCO2Scale;
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


uint8_t externalInterface_GetSensorData(uint8_t sensorId, uint8_t* pDataStruct)
{
	uint8_t index = 0;
	uint8_t localId = sensorId;
	if(localId == 0xFF)
	{
		localId = lastSensorDataId;
	}

	if((pDataStruct != NULL) && (localId <= EXT_INTERFACE_SENSOR_CNT))
	{
		memcpy(pDataStruct, &sensorDataDiveO2[localId], sizeof(SSensorDataDiveO2));
	}
	else
	{
		localId = 0xFF;
	}
	if(localId > MAX_ADC_CHANNEL)		/* at the moment sensor visualization is focused on the three ADC channels => map Mux sensors */
	{
		for(index = 0; index < MAX_ADC_CHANNEL; index++)
		{
			if(Mux2ADCMap[index] == localId)
			{
				localId = index;
			}
		}
	}

	return localId;
}

void externalInterface_SetSensorData(uint8_t sensorId, uint8_t* pDataStruct)
{
	uint8_t index = 0;

	if(pDataStruct != NULL)
	{
		if((sensorId != 0xFF) && (sensorId < EXT_INTERFACE_SENSOR_CNT))
		{
			memcpy(&sensorDataDiveO2[sensorId], pDataStruct, sizeof(SSensorDataDiveO2));
			lastSensorDataId = sensorId;
			if(sensorId >= MAX_ADC_CHANNEL)
			{
				for(index = 0; index < MAX_ADC_CHANNEL; index++)
				{
					if(Mux2ADCMap[index] == sensorId)
					{
						memcpy(&sensorDataDiveO2[index], pDataStruct, sizeof(SSensorDataDiveO2));
						lastSensorDataId = index;
						break;
					}
				}
			}
		}
		else
		{
			memset(&sensorDataDiveO2,0,sizeof(sensorDataDiveO2));
			lastSensorDataId = 0xFF;
		}
	}
}

void externalInface_SetSensorMap(uint8_t* pMap)
{
	if(pMap != NULL)
	{
		memcpy(MasterSensorMap, pMap, EXT_INTERFACE_SENSOR_CNT);		/* the map is not directly copied. Copy is done via cmd request */
	}

}

void externalInface_MapUartToLegacyADC(uint8_t* pMap)
{
	uint8_t index, index2;

	memset(Mux2ADCMap,0xFF, sizeof(Mux2ADCMap));

	for(index2 = 0; index2 < MAX_ADC_CHANNEL; index2++)					/* Unmap old mirror instances */
	{
		if((pMap[index2] == SENSOR_DIGO2M) || (pMap[index2] == SENSOR_CO2M) || (pMap[index2] == SENSOR_GNSSM))
		{
			pMap[index2] = SENSOR_NONE;
		}
	}

	/* Map Mux O2 sensors to ADC Slot if ADC slot is not in use */
	for(index = EXT_INTERFACE_MUX_OFFSET; index < EXT_INTERFACE_SENSOR_CNT-1; index++)
	{
		if(pMap[index] == SENSOR_DIGO2)
		{
			for(index2 = 0; index2 < MAX_ADC_CHANNEL; index2++)
			{
				if(pMap[index2] == SENSOR_NONE)
				{
					pMap[index2] = SENSOR_DIGO2M;		/* store a mirror instance needed for visualization */
					Mux2ADCMap[index2] = index;
					break;
				}
			}
		}
	}
	for(index = EXT_INTERFACE_MUX_OFFSET; index < EXT_INTERFACE_SENSOR_CNT-1; index++)
	{
		if(pMap[index] == SENSOR_CO2)
		{
			for(index2 = 0; index2 < MAX_ADC_CHANNEL; index2++)
			{
				if(pMap[index2] == SENSOR_NONE)
				{
					pMap[index2] = SENSOR_CO2M;		/* store a mirror instance needed for visualization */
					Mux2ADCMap[index2] = index;
					break;
				}
			}
		}
	}
	for(index = EXT_INTERFACE_MUX_OFFSET; index < EXT_INTERFACE_SENSOR_CNT-1; index++)
	{
		if(pMap[index] == SENSOR_GNSS)
		{
			for(index2 = 0; index2 < MAX_ADC_CHANNEL; index2++)
			{
				if(pMap[index2] == SENSOR_NONE)
				{
					pMap[index2] = SENSOR_GNSSM;		/* store a mirror instance needed for visualization */
					Mux2ADCMap[index2] = index;
					break;
				}
			}
		}
	}
#ifdef ENABLE_SENTINEL_MODE
	if(pMap[EXT_INTERFACE_MUX_OFFSET] == SENSOR_SENTINEL)
	{
		for(index2 = 0; index2 < MAX_ADC_CHANNEL; index2++)
		{
				pMap[index2] = SENSOR_SENTINELM;		/* store a mirror instance needed for visualization */
				Mux2ADCMap[index2] = index2 + EXT_INTERFACE_MUX_OFFSET;
		}
	}
#endif
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
	static uint8_t sensorIndex = 0;
	static uint8_t uartMuxChannel = 0;
#ifdef ENABLE_GNSS_SUPPORT
static	uint8_t detectionDelayCnt = 0;
#endif
	uint8_t index = 0;

	uint8_t cntSensor = 0;
	uint8_t cntUARTSensor = 0;
#ifdef ENABLE_CO2_SUPPORT
	uint8_t cmdString[10];
	uint8_t cmdLength = 0;
#endif

	if(externalAutoDetect != DETECTION_OFF)
	{
		switch(externalAutoDetect)
		{
			case DETECTION_INIT:	externalInterfaceMuxReqIntervall = 0;
									sensorIndex = 0;
									uartMuxChannel = 0;
									tmpSensorMap[0] = SENSOR_OPTIC;
									tmpSensorMap[1] = SENSOR_OPTIC;
									tmpSensorMap[2] = SENSOR_OPTIC;
									tmpSensorMap[3] = SENSOR_NONE;
									tmpSensorMap[4] = SENSOR_NONE;
									tmpSensorMap[5] = SENSOR_NONE;
									tmpSensorMap[6] = SENSOR_NONE;
									tmpSensorMap[7] = SENSOR_NONE;

									memset(foundSensorMap, SENSOR_NONE, sizeof(foundSensorMap));
									memset(externalInterface_SensorState,UART_COMMON_INIT,sizeof(externalInterface_SensorState));
									memset(Mux2ADCMap,0, sizeof(Mux2ADCMap));

									if(externalInterfacePresent)
									{
										externalInterface_SwitchPower33(0);
										externalInterface_SwitchUART(EXT_INTERFACE_UART_OFF);
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
											foundSensorMap[index] = SENSOR_ANALOG;
										}
										else
										{
											tmpSensorMap[sensorIndex++] = SENSOR_NONE;
										}
									}
									externalInterfaceMuxReqIntervall = 1100;
									externalAutoDetect = DETECTION_UARTMUX;
									externalInterface_SwitchUART(EXT_INTERFACE_UART_O2);
									externalInterface_CheckBaudrate(SENSOR_DIGO2);
									UART_MUX_SelectAddress(MAX_MUX_CHANNEL);
									uartO2_SetChannel(MAX_MUX_CHANNEL);
									activeUartChannel = MAX_MUX_CHANNEL;
									tmpSensorMap[EXT_INTERFACE_SENSOR_CNT-1] = SENSOR_MUX;
				break;
			case DETECTION_UARTMUX: if(uartO2_isSensorConnected())
									{
										uartMuxChannel = 1;
										tmpSensorMap[EXT_INTERFACE_SENSOR_CNT-1] = SENSOR_MUX;
										foundSensorMap[EXT_INTERFACE_SENSOR_CNT-1] = SENSOR_MUX;
									}
									else
									{
										tmpSensorMap[EXT_INTERFACE_SENSOR_CNT-1] = SENSOR_NONE;
									}
									externalAutoDetect = DETECTION_DIGO2_0;
									uartO2_SetChannel(0);
									activeUartChannel = 0;
									tmpSensorMap[EXT_INTERFACE_MUX_OFFSET] = SENSOR_DIGO2;
									externalInterface_SensorState[EXT_INTERFACE_MUX_OFFSET] = UART_COMMON_INIT;
									externalInterface_SwitchUART(EXT_INTERFACE_UART_O2);
									if(foundSensorMap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)
									{
										UART_MUX_SelectAddress(0);
									}
				break;
			case DETECTION_DIGO2_0:
			case DETECTION_DIGO2_1: 
			case DETECTION_DIGO2_2:
			case DETECTION_DIGO2_3:
									if(uartO2_isSensorConnected())
									{
										foundSensorMap[externalAutoDetect - DETECTION_DIGO2_0 + EXT_INTERFACE_MUX_OFFSET] = SENSOR_DIGO2;
									}
									tmpSensorMap[EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
									if(uartMuxChannel)
									{
										externalInterface_SwitchUART(EXT_INTERFACE_UART_O2);
										UART_MUX_SelectAddress(uartMuxChannel);
										externalInterface_SensorState[uartMuxChannel + EXT_INTERFACE_MUX_OFFSET] = UART_COMMON_INIT;
										uartO2_SetChannel(uartMuxChannel);
										activeUartChannel = uartMuxChannel;
										tmpSensorMap[uartMuxChannel - 1 + EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
										tmpSensorMap[uartMuxChannel + EXT_INTERFACE_MUX_OFFSET] = SENSOR_DIGO2;

										if(uartMuxChannel < MAX_MUX_CHANNEL - 1)
										{
											uartMuxChannel++;
										}
									}
									else
									{
										externalAutoDetect = DETECTION_DIGO2_3; /* skip detection of other serial sensors */
									}
									externalAutoDetect++;
#ifdef ENABLE_CO2_SUPPORT
									if(externalAutoDetect == DETECTION_CO2_0)
									{
										tmpSensorMap[uartMuxChannel + EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
										if(foundSensorMap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)
										{
											UART_MUX_SelectAddress(0);
										}
										activeUartChannel = 0;
										tmpSensorMap[uartMuxChannel - 1 + EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
										uartMuxChannel = 1;
										tmpSensorMap[EXT_INTERFACE_MUX_OFFSET] = SENSOR_CO2;
										externalInterface_SensorState[EXT_INTERFACE_MUX_OFFSET] = UART_COMMON_INIT;
										externalInterface_CheckBaudrate(SENSOR_CO2);
										if(foundSensorMap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)		/* switch sensor operation mode depending on HW config */
										{
											uartCo2_SendCmd(CO2CMD_MODE_POLL, cmdString, &cmdLength);
										}
										else
										{
											uartCo2_SendCmd(CO2CMD_MODE_STREAM, cmdString, &cmdLength);
										}
									}
				break;
			case DETECTION_CO2_0:
			case DETECTION_CO2_1:
			case DETECTION_CO2_2:
			case DETECTION_CO2_3:	if(uartCo2_isSensorConnected())
									{
										foundSensorMap[EXT_INTERFACE_MUX_OFFSET + activeUartChannel] = SENSOR_CO2;
#ifdef ENABLE_GNSS_SUPPORT
										externalAutoDetect = DETECTION_GNSS_0;	/* only one CO2 sensor supported */
#else
										externalAutoDetect = DETECTION_DONE;	/* only one CO2 sensor supported */
#endif
									}
									else if(foundSensorMap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)
									{
										externalInterface_CheckBaudrate(SENSOR_DIGO2);
										UART_MUX_SelectAddress(uartMuxChannel);
										activeUartChannel = uartMuxChannel;
										tmpSensorMap[uartMuxChannel - 1 + EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
										tmpSensorMap[EXT_INTERFACE_MUX_OFFSET + uartMuxChannel] = SENSOR_CO2;
										externalInterface_SensorState[EXT_INTERFACE_MUX_OFFSET + uartMuxChannel] = UART_COMMON_INIT;
										externalInterface_CheckBaudrate(SENSOR_CO2);
										uartCo2_SendCmd(CO2CMD_MODE_POLL, cmdString, &cmdLength);
										externalAutoDetect++;
										uartMuxChannel++;
									}
									else
									{


#if defined ENABLE_SENTINEL_MODE || defined ENABLE_GNSS_SUPPORT
#ifdef ENABLE_GNSS_SUPPORT
										externalAutoDetect = DETECTION_GNSS_0;
										externalInterface_SwitchUART(EXT_INTERFACE_UART_GNSS);
#else
#ifdef ENABLE_SENTINEL_MODE
										externalAutoDetect = DETECTION_SENTINEL;
#endif
#endif
#else
										externalAutoDetect = DETECTION_DONE;
#endif
									}
#endif

#ifdef ENABLE_GNSS_SUPPORT
									if(externalAutoDetect == DETECTION_GNSS_0)
									{
										tmpSensorMap[uartMuxChannel + EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
										if(foundSensorMap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)
										{
											externalInterface_CheckBaudrate(SENSOR_DIGO2);
											UART_MUX_SelectAddress(0);
										}
										activeUartChannel = 0;
										tmpSensorMap[uartMuxChannel - 1 + EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
										uartMuxChannel = 1;
										tmpSensorMap[EXT_INTERFACE_MUX_OFFSET] = SENSOR_GNSS;
										externalInterface_SensorState[EXT_INTERFACE_MUX_OFFSET] = UART_COMMON_INIT;
										externalInterface_CheckBaudrate(SENSOR_GNSS);
										externalInterfaceMuxReqIntervall = 500;	/* iterations needed for module config */
										detectionDelayCnt = 6;
									}
							break;
			case DETECTION_GNSS_0:
			case DETECTION_GNSS_1:
			case DETECTION_GNSS_2:
			case DETECTION_GNSS_3:	if(detectionDelayCnt == 0)
									{
										if(uartGnss_isSensorConnected())
										{
											foundSensorMap[EXT_INTERFACE_MUX_OFFSET + activeUartChannel] = SENSOR_GNSS;
	#ifdef ENABLE_SENTINEL_MODE
											externalAutoDetect = DETECTION_SENTINEL;	/* only one GNSS sensor supported */
	#else
											externalAutoDetect = DETECTION_DONE;		/* only one GNSS sensor supported */
	#endif
										}
										else if(foundSensorMap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)
										{
											externalInterface_CheckBaudrate(SENSOR_DIGO2);
											UART_MUX_SelectAddress(uartMuxChannel);
											activeUartChannel = uartMuxChannel;
											tmpSensorMap[uartMuxChannel - 1 + EXT_INTERFACE_MUX_OFFSET] = SENSOR_NONE;
											tmpSensorMap[EXT_INTERFACE_MUX_OFFSET + uartMuxChannel] = SENSOR_CO2;
											externalInterface_SensorState[EXT_INTERFACE_MUX_OFFSET + uartMuxChannel] = UART_COMMON_INIT;
											externalInterface_CheckBaudrate(SENSOR_CO2);
								//			uartGnss_SendCmd(GNSSCMD_MODE_POLL, cmdString, &cmdLength);
											externalAutoDetect++;
											detectionDelayCnt = 3;
											uartMuxChannel++;
										}
										else
										{
	#ifdef ENABLE_SENTINEL_MODE
											externalAutoDetect = DETECTION_SENTINEL;
	#else
											externalAutoDetect = DETECTION_DONE;
	#endif
										}
									}
									else
									{
										detectionDelayCnt--;
									}
	#endif
#ifdef ENABLE_SENTINEL_MODE
									if(externalAutoDetect == DETECTION_SENTINEL)
									{
										externalInterface_SensorState[EXT_INTERFACE_MUX_OFFSET] = UART_COMMON_INIT;
										uartO2_SetChannel(0);
										activeUartChannel = 0;
										tmpSensorMap[EXT_INTERFACE_MUX_OFFSET] = SENSOR_SENTINEL;
										externalInterface_SwitchUART(EXT_INTERFACE_UART_SENTINEL);
										externalInterface_CheckBaudrate(SENSOR_SENTINEL);
										UART_StartDMA_Receiption();
									}
				break;

			case DETECTION_SENTINEL:
			case DETECTION_SENTINEL2:
									if(uartSentinel_isSensorConnected())
									{
										for(index = EXT_INTERFACE_MUX_OFFSET; index < EXT_INTERFACE_MUX_OFFSET+3; index++)
										{
											foundSensorMap[index] = SENSOR_SENTINEL;
										}
									}
									externalAutoDetect++;
#endif
				break;
			case DETECTION_DONE:	externalAutoDetect = DETECTION_OFF;
									externalInterface_SwitchUART(EXT_INTERFACE_UART_OFF);
									activeUartChannel = 0xFF;
									cntSensor = 0;
									cntUARTSensor = 0;
									for(index = 0; index < EXT_INTERFACE_SENSOR_CNT-1; index++)
									{
										if((foundSensorMap[index] >= SENSOR_ANALOG) && (foundSensorMap[index] < SENSOR_MUX))
										{
											cntSensor++;
										}

										if((foundSensorMap[index] == SENSOR_DIGO2) || (foundSensorMap[index] == SENSOR_CO2) || (foundSensorMap[index] == SENSOR_GNSS))
										{
											cntUARTSensor++;
										}
#ifdef ENABLE_SENTINEL_MODE
										if(foundSensorMap[index] == SENSOR_SENTINEL)		/* The Sentinel has a fixed setup */
										{
											cntSensor = 3;
											cntUARTSensor = 1;
											break;
										}
#endif
									}
									externalInface_MapUartToLegacyADC(foundSensorMap);
									externalInterfaceMuxReqIntervall = 0xFFFF;
									if(cntSensor == 0)		/* return default sensor map if no sensor at all has been detected */
									{
										foundSensorMap[0] = SENSOR_OPTIC;
										foundSensorMap[1] = SENSOR_OPTIC;
										foundSensorMap[2] = SENSOR_OPTIC;
									}
									else
									{
										if(cntUARTSensor != 0)
										{
											externalInterfaceMuxReqIntervall = REQUEST_INT_SENSOR_MS / cntUARTSensor;
										}
									}
									memcpy(SensorMap, foundSensorMap, sizeof(foundSensorMap));
									memset(externalInterface_SensorState, UART_COMMON_INIT, sizeof(externalInterface_SensorState));
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
	uint8_t cntUARTSensor = 0;

	switch(Cmd & 0x00FF)		/* lower byte is reserved for commands */
	{
		case EXT_INTERFACE_AUTODETECT:	externalAutoDetect = DETECTION_INIT;
										for(index = 0; index < 3; index++)
										{
											SensorMap[index] = SENSOR_SEARCH;
										}
			break;
		case EXT_INTERFACE_CO2_CALIB:	for(index = 0; index < EXT_INTERFACE_SENSOR_CNT; index++)
										{
											if(SensorMap[index] == SENSOR_CO2)
											{
												externalInterface_SensorState[index] = UART_CO2_CALIBRATE;
												break;
											}
										}
			break;
		case EXT_INTERFACE_COPY_SENSORMAP:	if(externalAutoDetect == DETECTION_OFF)
											{
												memcpy(SensorMap, MasterSensorMap, sizeof(MasterSensorMap));
												for(index = 0; index < EXT_INTERFACE_SENSOR_CNT; index++)
												{
													if((SensorMap[index] == SENSOR_DIGO2) || (SensorMap[index] == SENSOR_CO2) || (SensorMap[index] == SENSOR_GNSS))
													{
														cntUARTSensor++;
													}
												}
												externalInface_MapUartToLegacyADC(SensorMap);
												if(cntUARTSensor > 0)
												{
													externalInterfaceMuxReqIntervall = REQUEST_INT_SENSOR_MS / cntUARTSensor;
													activeUartChannel = 0xFF;
												}
												else
												{
													externalInterfaceMuxReqIntervall = 0xFFFF;
												}
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

uint8_t ExternalInterface_SelectUsedMuxChannel(uint8_t currentChannel)
{
	uint8_t index = currentChannel;
	uint8_t newChannel = index;
	uint8_t *pmap = externalInterface_GetSensorMapPointer(0);

	do
	{
		index++;
		if(index == MAX_MUX_CHANNEL)
		{
			index = 0;
		}
		if(((pmap[index + EXT_INTERFACE_MUX_OFFSET] == SENSOR_DIGO2)
				|| (pmap[index + EXT_INTERFACE_MUX_OFFSET] == SENSOR_CO2)
				|| (pmap[index + EXT_INTERFACE_MUX_OFFSET] == SENSOR_GNSS))
				&& (index != activeUartChannel))
		{
			newChannel = index;
			break;
		}
	} while(index != currentChannel);

	return newChannel;
}

void externalInterface_CheckBaudrate(uint8_t sensorType)
{
	uint32_t newBaudrate = 0;

	switch(sensorType)
	{
			case SENSOR_GNSS:
			case SENSOR_SENTINEL:
			case SENSOR_CO2:		newBaudrate = 9600;
				break;
			case SENSOR_DIGO2:
		default:	newBaudrate = 19200;
			break;
	}
	if(huart1.Init.BaudRate != newBaudrate)
	{
		UART_ChangeBaudrate(newBaudrate);
	}
}

void externalInterface_HandleUART()
{
	static uint8_t retryRequest = 0;
	static uint32_t lastRequestTick = 0;
	static uint32_t TriggerTick = 0;
	uint8_t index = 0;
	static uint8_t timeToTrigger = 0;
	uint32_t tick =  HAL_GetTick();
	uint8_t *pmap = externalInterface_GetSensorMapPointer(0);
	uint8_t forceMuxChannel = 0;


	if(externalInterfaceMuxReqIntervall != 0xFFFF)
	{
		if(activeUartChannel == 0xFF)
		{
			MX_USART1_UART_Init();
			activeUartChannel = ExternalInterface_SelectUsedMuxChannel(0);
			uartO2_SetChannel(activeUartChannel);

			switch(pmap[activeUartChannel + EXT_INTERFACE_MUX_OFFSET])
			{
				case SENSOR_DIGO2:
				case SENSOR_GNSS:
				case SENSOR_CO2:
				case SENSOR_SENTINEL: externalInterface_CheckBaudrate(pmap[activeUartChannel + EXT_INTERFACE_MUX_OFFSET]);
					break;
				default: 			externalInterface_CheckBaudrate(SENSOR_DIGO2);
					break;
			}
			if(pmap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX)
			{
				HAL_Delay(10);	/* make sure MUX is available for reception after wakeup */
				UART_MUX_SelectAddress(activeUartChannel);
			}
		}

		if(externalInterface_SensorState[activeSensorId] != UART_COMMON_INIT)
		{
			UART_ReadData(pmap[activeSensorId]);
			UART_WriteData();
		}
		if(externalInterface_SensorState[activeSensorId] == UART_COMMON_INIT)
		{
			lastRequestTick = tick;
			TriggerTick = tick - 10;	/* just to make sure control is triggered */
			timeToTrigger = 1;
			retryRequest = 0;
		}
		else if(((retryRequest == 0)		/* timeout or error */
				&& (((time_elapsed_ms(lastRequestTick,tick) > (TIMEOUT_SENSOR_ANSWER)) && (externalInterface_SensorState[activeSensorId] != UART_O2_IDLE))	/* retry if no answer after half request interval */
					|| (externalInterface_SensorState[activeSensorId] == UART_O2_ERROR))))
		{
			/* The channel switch will cause the sensor to respond with an error message. */
			/* The sensor needs ~30ms to recover before he is ready to receive the next command => transmission delay needed */

			TriggerTick = tick;
			timeToTrigger = COMMAND_TX_DELAY;
			retryRequest = 1;
		}

		else if(time_elapsed_ms(lastRequestTick,tick) > externalInterfaceMuxReqIntervall)	/* switch sensor and / or trigger next request */
		{
			lastRequestTick = tick;
			TriggerTick = tick;
			retryRequest = 0;
			timeToTrigger = 1;

			if((externalInterface_SensorState[activeSensorId] == UART_O2_REQ_O2)		/* timeout */
					|| (externalInterface_SensorState[activeSensorId] == UART_O2_REQ_RAW)
					|| (externalInterface_SensorState[activeSensorId] == UART_CO2_OPERATING)
					|| (externalInterface_SensorState[activeSensorId] == UART_GNSS_GET_PVT))
			{
				forceMuxChannel = 1;
				externalInterface_SensorState[activeSensorId] = UART_O2_IDLE;
				switch(pmap[activeSensorId])
				{
					case SENSOR_DIGO2: setExternalInterfaceChannel(activeSensorId,0.0);
						break;
					case SENSOR_CO2: externalInterface_SetCO2Value(0.0);
									 externalInterface_SetCO2State(0);
						break;
					default:
						break;
				}
			}

			if(pmap[EXT_INTERFACE_SENSOR_CNT-1] == SENSOR_MUX) /* select next sensor if mux is connected */
			{
				if(activeUartChannel < MAX_MUX_CHANNEL)
				{
					index = ExternalInterface_SelectUsedMuxChannel(activeUartChannel);
					if((index != activeUartChannel) || (forceMuxChannel))
					{
						forceMuxChannel = 0;
						timeToTrigger = 100;
						activeUartChannel = index;
						if((pmap[index + EXT_INTERFACE_MUX_OFFSET] == SENSOR_DIGO2)
								|| (pmap[index + EXT_INTERFACE_MUX_OFFSET] == SENSOR_CO2)
								|| (pmap[index + EXT_INTERFACE_MUX_OFFSET] == SENSOR_GNSS))
						{
							uartO2_SetChannel(activeUartChannel);
							externalInterface_CheckBaudrate(SENSOR_MUX);
							UART_MUX_SelectAddress(activeUartChannel);
							externalInterface_CheckBaudrate(pmap[activeUartChannel + EXT_INTERFACE_MUX_OFFSET]);
						}
					}
				}
			}
			else
			{
				timeToTrigger = 1;
			}
		}
		if((timeToTrigger != 0) && (time_elapsed_ms(TriggerTick,tick) > timeToTrigger))
		{
			timeToTrigger = 0;
			switch (pmap[activeSensorId])
			{
				case SENSOR_MUX:
				case SENSOR_DIGO2:	uartO2_Control();
					break;
#ifdef ENABLE_CO2_SUPPORT
				case SENSOR_CO2:	uartCo2_Control();
					break;
#endif
#ifdef ENABLE_GNSS_SUPPORT
				case SENSOR_GNSS:	uartGnss_Control();
						break;
#endif
#ifdef ENABLE_SENTINEL_MODE
				case SENSOR_SENTINEL: uartSentinel_Control();
				break;
#endif
				default:
					break;
			}
		}
#if 0
		else
		{
			if(((time_elapsed_ms(lastRequestTick,tick) > (externalInterfaceMuxReqIntervall - 100))
				 && externalInterface_SensorState[activeSensorId] = UART_COMMON_IDLE))
			{
				externalInterface_ReadAndSwitch();
			}
		}
#endif
	}

#if 0
#ifdef ENABLE_SENTINEL_MODE
		if(externalInterface_GetUARTProtocol() & (EXT_INTERFACE_UART_SENTINEL))
		{
			UART_HandleSentinelData();
		}
#endif
#endif
}
