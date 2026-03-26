/**
  ******************************************************************************
  * @file    uartProtocol_O2.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    18-Jun-2023
  * @brief	 Interface functionality to handle external, UART based O2 sensors
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
#ifndef UART_PROTOCOL_O2_H
#define UART_PROTOCOL_O2_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "configuration.h"
#include "stm32f4xx_hal.h"

 typedef enum
 {
 	UART_COMMON_INIT = 0,	/* Default Status for every sensor type */
	UART_COMMON_IDLE,		/* sensor detected and no communication pending */
	UART_COMMON_ERROR,		/* Error message received from sensor */
	UART_COMMON_DONE		/* Signal that data was received and next channel may be processed */
 } uartCommonStatus_t;

 typedef enum
 {
	UART_O2_INIT = UART_COMMON_INIT,		/* Default Status for every sensor type */
	UART_O2_IDLE = UART_COMMON_IDLE,		/* sensor detected and no communication pending */
	UART_O2_ERROR = UART_COMMON_ERROR,		/* Error message received from sensor */
 	UART_O2_CHECK = 50,						/* send blink command and check if sensor answers */
 	UART_O2_REQ_INFO,						/* request information about available internal sensors of sensor */
	UART_O2_REQ_ID,							/* request ID of sensor */
 	UART_O2_REQ_O2,							/* O2 value has been requested and is in receiption progress */
	UART_O2_REQ_RAW,						/* Request O2 and extended raw data */
 } uartO2Status_t;


 typedef enum
  {
	O2RX_IDLE = 0,			/* no reception pending */
	O2RX_CONFIRM,			/* check the command echo */
	O2RX_GETNR,				/* extract the sensor number */
	O2RX_GETO2,				/* extract the ppo2 */
	O2RX_GETTEMP,			/* extract the temperature */
	O2RX_GETSTATUS,			/* extract the sensor status */
	O2RX_GETTYPE,			/* extract the sensor type (should be 8) */
	O2RX_GETCHANNEL,		/* extract the number of sensor channels (should be 1) */
	O2RX_GETVERSION,		/* extract the sensor version */
	O2RX_GETSUBSENSORS,		/* extract the available measures (O2, temperature, humidity etc) */
	O2RX_GETDPHI,			/* extract phase shift */
	O2RX_INTENSITY,			/* extract intensity of signal */
	O2RX_AMBIENTLIGHT,		/* extract the intensity of the ambient light */
	O2RX_PRESSURE,			/* extract pressure within the sensor housing */
	O2RX_HUMIDITY			/* extract humidity within the sensor housing */
  } uartO2RxState_t;

void uartO2_Control(void);
void uartO2_ProcessData(uint8_t data);
void uartO2_SetChannel(uint8_t channel);
uint8_t uartO2_isSensorConnected();




#endif /* EXTERNAL_INTERFACE_H */
