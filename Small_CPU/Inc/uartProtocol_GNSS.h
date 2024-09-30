/**
  ******************************************************************************
  * @file    uartProtocol_GNSS.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    30-Sep-2024
  * @brief	 Interface functionality for operation of gnss devices
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
#ifndef UART_PROTOCOL_GNSS_H
#define UART_PROTOCOL_GNSS_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "configuration.h"
#include "stm32f4xx_hal.h"

 typedef enum
  {
	UART_GNSS_INIT = 0,			/* Default Status for every sensor type */
	UART_GNSS_LOAD,				/* Load modul configuration */
	UART_GNSS_GET_ID,			/* get unique module ID */
	UART_GNSS_IDLE,				/* sensor detected and no communication pending */
	UART_GNSS_ERROR,
  	UART_GNSS_OPERATING,		/* normal operation => cyclic request of PVT data */
  } uartGnssStatus_t;

  typedef enum
  {
  	GNSSRX_READY = 0,			/* Initial state */
	GNSSRX_RECEIVING,			/* Pending data receiption */
  } receiveStateGnss_t;


void uartGnss_Control(void);
void uartGnss_ProcessData(void);
uint8_t uartSentinel_isSensorConnected();

#endif /* UART_PROTOCOL_GNSS_H */
