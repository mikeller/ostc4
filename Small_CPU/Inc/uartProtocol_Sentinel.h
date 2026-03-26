/**
  ******************************************************************************
  * @file    uartProtocol_Sentinel.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    15-Jan-2024
  * @brief	 Interface functionality read data from Sentinel rebreather
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
#ifndef UART_PROTOCOL_SENTINEL_H
#define UART_PROTOCOL_SENTINEL_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "configuration.h"
#include "stm32f4xx_hal.h"

/* Bit flags for different sensor types provided by Sentinel or Red Head/Bare */

#define SENTINEL_O2			0x01
#define SENTINEL_CO2		0x02
#define SENTINEL_PRESSURE	0x04
#define SENTINEL_TEMPSTICK	0x08



#define	UART_SENTINEL_O2_P 	'T'				/* Primary O2 sensor */
#define	UART_SENTINEL_O2_S	'S'				/* Secondary O2 sensor */
#define	UART_SENTINEL_PRESSURE_O2 	'I'		/* O2 pressure */
#define	UART_SENTINEL_PRESSURE_D	'J'		/* Diluent pressure */
#define	UART_SENTINEL_TEMPSTICK		'Y'		/* Sector value of the tempstick */

 typedef enum
  {
	UART_SENTINEL_INIT = 0,		/* Default Status for every sensor type */
	UART_SENTINEL_IDLE,			/* sensor detected and no communication pending */
	UART_SENTINEL_ERROR,
	UART_SENTINEL_DONE,					/* Signal that data was received and next channel may be processed */
  	UART_SENTINEL_OPERATING = 20,		/* normal operation */
  } uartSentinelStatus_t;

  typedef enum
  {
  	SENTRX_Ready= 0,					/* Initial state */
 	SENTRX_DetectStart,					/* validate start byte */
 	SENTRX_SelectData,					/* Data contained in this frame */
  	SENTRX_Data0,						/* Process incoming data */
 	SENTRX_Data1,
 	SENTRX_Data2,
 	SENTRX_Data3,
 	SENTRX_Data4,
 	SENTRX_Data5,
 	SENTRX_Data6,
 	SENTRX_Data7,
 	SENTRX_Data8,
 	SENTRX_Data9,
 	SENTRX_Data10,
 	SENTRX_CheckSum,
 	SENTRX_DataComplete
  } receiveStateSentinel_t;


void uartSentinel_Control(void);
void uartSentinel_ProcessData(uint8_t data);
uint8_t uartSentinel_isSensorConnected();

#endif /* UART_PROTOCOL_SENTINEL_H */
