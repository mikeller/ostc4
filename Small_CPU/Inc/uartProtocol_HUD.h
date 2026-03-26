/**
  ******************************************************************************
  * @file    uartProtocol_HUD.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    24-Feb-2026
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
#ifndef UART_PROTOCOL_HUD_H
#define UART_PROTOCOL_HUD_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "configuration.h"
#include "stm32f4xx_hal.h"


#define HUD_INFO_DATA_LENGTH		(24u)	/* expected number of received info data */
#define HUD_MAX_CMD_LENGTH			(32u) 	/* max length for a command sequence */

#define HUD_CMD_BYTE_START			(0xAA)	/* Start byte */
#define HUD_CMD_BYTE_INFO			(0x10)	/* Request HUD info */
#define HUD_CMD_BYTE_UPDATE			(0x20)	/* Update LED sequence command */
#define HUD_CMD_BYTE_STOP			(0x30)	/* Stip LED sequence execution */

 typedef enum
  {
	UART_HUD_INIT = 0,		/* Default Status for every sensor type */
	UART_HUD_IDLE,			/* sensor detected and no communication pending */
	UART_HUD_ERROR,
 	UART_HUD_SETUP = 30,	/* collecting data */
  	UART_HUD_UPDATE,		/* update the HUD status LEDs */
	UART_HUD_ABORT,			/* abort status sequence */
  } uartHUDStatus_t;

  typedef enum
  {
  	HUDRX_Ready= 0,					/* Initial state */
 	HUDRX_DetectStart,				/* validate start byte */
  	HUDRX_RXData,
	HUDRX_CheckSum_L,
	HUDRX_CheckSum_H,
 	HUDRX_DataComplete
  } receiveStateHUD_t;


 typedef enum
 {
	HUDCMD_GETINFO = 0,	/* Get HUD info */
 	HUDCMD_UPDATE,		/* Update LED sequence */
 	HUDCMD_ABORTSEQ		/* Abort LED sequence */
 } hudSensorCmd_t;


void uartHUD_Control(void);
void uartHUD_ProcessData(uint8_t data);
void uartHUD_SendCmd(uint8_t HUDCmd);
uint8_t uartHUD_isSensorConnected();

#endif /* UART_PROTOCOL_HUD_H */
