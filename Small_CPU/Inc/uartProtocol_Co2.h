/**
  ******************************************************************************
  * @file    uartProtocol_Co2.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    31-Jul-2023
  * @brief	 Interface functionality to handle external, UART based CO2 sensors
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
#ifndef UART_PROTOCOL_CO2_H
#define UART_PROTOCOL_CO2_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "configuration.h"
#include "stm32f4xx_hal.h"

 typedef enum
  {
	UART_CO2_INIT = 0,		/* Default Status for every sensor type */
	UART_CO2_IDLE,			/* sensor detected and no communication pending */
	UART_CO2_ERROR,
 	UART_CO2_SETUP = 10,	/* collecting data needed to be read out of the sensor once at startup */
  	UART_CO2_OPERATING,		/* normal operation */
	UART_CO2_CALIBRATE		/* request calibration */
  } uartCO2Status_t;

  typedef enum
  {
  	CO2RX_Ready= 0,					/* Initial state */
 	CO2RX_DetectStart,					/* validate start byte */
 	CO2RX_SelectData,					/* Data contained in this frame */
  	CO2RX_Data0,						/* Process incoming data */
 	CO2RX_Data1,
 	CO2RX_Data2,
 	CO2RX_Data3,
 	CO2RX_Data4,
 	CO2RX_Data5,
 	CO2RX_Data6,
 	CO2RX_Data7,
 	CO2RX_Data8,
 	CO2RX_Data9,
 	CO2RX_Data10,
 	CO2RX_Data11,
 	CO2RX_Data12,
 	CO2RX_DataComplete
  } receiveStateCO2_t;


 typedef enum
 {
 	CO2CMD_MODE_POLL,		/* Set operation mode of sensor to polling => only send data if requested */
 	CO2CMD_MODE_STREAM,		/* Set operation mode of sensor to streaming => send data every two seconds */
 	CO2CMD_CALIBRATE,		/* Calibrate sensor */
 	CO2CMD_GETSCALE,		/* Get scaling factor */
 	CO2CMD_GETDATA			/* Read sensor data */
 } co2SensorCmd_t;


void uartCo2_Control(void);
void uartCo2_ProcessData(uint8_t data);
void uartCo2_SendCmd(uint8_t CO2Cmd, uint8_t *cmdString, uint8_t *cmdLength);
uint8_t uartCo2_isSensorConnected();

#endif /* UART_PROTOCOL_CO2_H */
