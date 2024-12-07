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
		UART_GNSS_INIT = 0,		/* Default Status for every sensor type */
		UART_GNSS_IDLE,			/* sensor detected and no communication pending */
		UART_GNSS_ERROR,		/* Error message received from sensor */
		UART_GNSS_WARMUP = 10,
		UART_GNSS_LOADCONF_0,
		UART_GNSS_LOADCONF_1,
		UART_GNSS_LOADCONF_2,
		UART_GNSS_GET_PVT,
		UART_GNSS_GET_SAT
  } uartGnssStatus_t;

  typedef enum
  {
  	GNSSRX_READY = 0,			/* Initial state */
	GNSSRX_DETECT_HEADER_0,
	GNSSRX_DETECT_HEADER_1,
	GNSSRX_DETECT_HEADER_2,
	GNSSRX_DETECT_HEADER_3,
	GNSSRX_DETECT_LENGTH_0,
	GNSSRX_DETECT_LENGTH_1,
	GNSSRX_DETECT_ACK_0,
	GNSSRX_DETECT_ACK_1,
	GNSSRX_DETECT_ACK_2,
	GNSSRX_DETECT_ACK_3,
	GNSSRX_READ_DATA,
	GNSSRX_READ_CK_A,
	GNSSRX_READ_CK_B,
  } receiveStateGnss_t;


  typedef enum
  {
  	GNSSCMD_LOADCONF_0 = 0,
	GNSSCMD_LOADCONF_1,
	GNSSCMD_LOADCONF_2,
	GNSSCMD_GET_NAV_DATA,
	GNSSCMD_GET_PVT_DATA,
	GNSSCMD_GET_POSLLH_DATA,
	GNSSCMD_GET_NAVSAT_DATA
  } gnssSensorCmd_t;

  typedef struct
  {
    uint8_t class;
    uint8_t id;
  } gnssRequest_s;

uartGnssStatus_t uartGnss_GetState(void);
void uartGnss_SetState(uartGnssStatus_t newState);
void uartGnss_Control(void);
void uartGnss_ProcessData(uint8_t data);
uint8_t uartGnss_isSensorConnected();
void uartGnss_SendCmd(uint8_t GnssCmd);

#endif /* UART_PROTOCOL_GNSS_H */
