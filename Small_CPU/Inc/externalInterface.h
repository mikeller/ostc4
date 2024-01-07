/**
  ******************************************************************************
  * @file    externalInterface.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    07-Nov-2020
  * @brief	 Interface functionality to proceed external analog signal via i2c connection
  *
  @verbatim
  ==============================================================================
                        ##### How to use #####
  ==============================================================================
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef EXTERNAL_INTERFACE_H
#define EXTERNAL_INTERFACE_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "configuration.h"
#include "data_central.h"
#include "data_exchange.h"

#define MAX_ADC_CHANNEL		(3u)		/* number of channels to be read */
#define MAX_MUX_CHANNEL		(4u)		/* number of channels provided by the UART multiplexer */
#define EXTERNAL_ADC_NO_DATA	0xFF

#define EXT33V_CONTROL_PIN				GPIO_PIN_7	/* PortC */

#define MIN_ADC_VOLTAGE_MV	(5.0f)		/* miminal voltage to rate an ADC channel as active */

#define COMMON_SENSOR_STATE_INIT	(0x0u)	/* All individual state definitions shall start with a INIT state = 0 */
#define COMMON_SENSOR_STATE_INVALID (0xFFu) /* All individual state definitions shall not use 0xFF for operation control */


enum									/* Definitions for supported UART protocols */
{
	EXT_INTERFACE_UART_OFF	= 0,
	EXT_INTERFACE_UART_CO2,
	EXT_INTERFACE_UART_SENTINEL,
	EXT_INTERFACE_UART_O2
};


 typedef enum
 {
    DETECTION_OFF = 0,		/* no detection requested */
	DETECTION_INIT,			/* prepare external interface for operation if not already activated */
	DETECTION_START,
	DETECTION_ANALOG1,		/* check ADC channels for connected sensors */
	DETECTION_ANALOG2,
	DETECTION_UARTMUX,		/* check if a uart multiplexer is present */
	DETECTION_DIGO2_0,		/* check UART channel for connected DigO2 sensor */
	DETECTION_DIGO2_1,
	DETECTION_DIGO2_2,
	DETECTION_DIGO2_3,
#ifdef ENABLE_CO2_SUPPORT
	DETECTION_CO2_0,			/* check UART channel for connected CO2 sensor */
	DETECTION_CO2_1,
	DETECTION_CO2_2,
	DETECTION_CO2_3,
#endif
#ifdef ENABLE_SENTINEL_MODE
	DETECTION_SENTINEL,		/* check UART channel for connected Sentinel */
	DETECTION_SENTINEL2,
#endif
	DETECTION_DONE
 } externalInterfaceAutoDetect_t;




void externalInterface_Init(void);
void externalInterface_InitPower33(void);
void externalInterface_InitDatastruct(void);
uint8_t externalInterface_StartConversion(uint8_t channel);
uint8_t externalInterface_ReadAndSwitch();
float externalInterface_CalculateADCValue(uint8_t channel);
float getExternalInterfaceChannel(uint8_t channel);
uint8_t setExternalInterfaceChannel(uint8_t channel, float value);
void externalInterface_SwitchPower33(uint8_t state);
void externalInterface_SwitchADC(uint8_t state);
void externalInterface_SwitchUART(uint8_t protocol);
uint8_t externalInterface_isEnabledPower33(void);
uint8_t externalInterface_isEnabledADC(void);
uint8_t externalInterface_GetUARTProtocol(void);
void externalInterface_HandleUART(void);
void externalInterface_SetCO2Scale(float CO2Scale);
float externalInterface_GetCO2Scale(void);
void externalInterface_SetCO2Value(uint16_t CO2_ppm);
void externalInterface_SetCO2SignalStrength(uint16_t LED_qa);
uint16_t externalInterface_GetCO2Value(void);
uint16_t externalInterface_GetCO2SignalStrength(void);
void externalInterface_SetCO2State(uint16_t state);
uint16_t externalInterface_GetCO2State(void);
uint8_t externalInterface_GetSensorData(uint8_t sensorId, uint8_t* pDataStruct);
void externalInterface_SetSensorData(uint8_t sensorId, uint8_t* pDataStruct);
void externalInface_SetSensorMap(uint8_t* pMap);
uint8_t* externalInterface_GetSensorMapPointer(uint8_t finalMap);
void externalInterface_AutodetectSensor(void);
void externalInterface_ExecuteCmd(uint16_t Cmd);

uint8_t externalInterface_GetActiveUartSensor(void);
void externalInterface_SetSensorState(uint8_t sensorIdx, uint8_t state);
uint8_t externalInterface_GetSensorState(uint8_t sensorIdx);



#endif /* EXTERNAL_INTERFACE_H */
