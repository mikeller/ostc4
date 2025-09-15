/**
  ******************************************************************************
	* @copyright	heinrichs weikamp gmbh
  * @file    		demo.c
  * @author  		heinrichs weikamp gmbh
	* @date    		26-Nov-2015
  * @version 		0.1
  * @since	 		26-Nov-2015
  * @brief   		f�r die Messe und die Vitrine
	* @bug
	* @warning
  @verbatim
  ==============================================================================
	

	==============================================================================
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

#include "demo.h"
#include "base.h" // for BUTTON_BACK, ...
#include "data_exchange_main.h" // for time_elapsed_ms()
#include "settings.h"
#include "ostc.h"
#include "tInfoLogger.h"
#include <string.h>

#ifndef DEMOMODE

/* Exported functions --------------------------------------------------------*/

uint8_t demoGetCommand(void) { return 0; }
uint8_t demoModeActive(void) { return 0; }
void demoSendCommand(uint8_t action) { return; }


#else

/* Private variables with external access ------------------------------------*/

uint8_t demoModeActiveStatus = 1;
uint32_t systickDemoMode;

uint16_t demoModeActual = 0;

#define BUTTON_BACK (BUTTON_BACK_PIN | 0x80)
#define BUTTON_NEXT (BUTTON_NEXT_PIN | 0x80)
#define BUTTON_ENTER (BUTTON_ENTER_PIN | 0x80)

/* Private typedef -----------------------------------------------------------*/

#define MAXCOMMANDS 200

#ifdef SHOWCCRMODE
const uint8_t commandlist[MAXCOMMANDS][3] = 
{
/* Button, deci-seconds to wait */

	// start for both
	{0, 40},
	// start OC, activate CC
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_ENTER, 0}, // Change to CC
	{BUTTON_ENTER, 0}, // Leave Edit
	{BUTTON_BACK, 0}, 	// Cursor
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_BACK, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	// start CC @ 20, set cursor to Sim  Menu
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_BACK, 50},
	{0, 0},
	// re-loop @ 30
	{0, 3},
	// Logbook
	{BUTTON_BACK, 	0},
	{BUTTON_ENTER, 20},
	{BUTTON_NEXT, 	20},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	0},
	// surface screen
	{0, 2},
	// menu
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_ENTER, 30},
	{BUTTON_ENTER, 0},
	// simulator
	{0, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	// the end, repeat from start
	{255, 0}
};
#else // OC Mode Demo
const uint8_t commandlist[MAXCOMMANDS][2] = 
{
/* Button, seconds to wait */

	// start for both
	{0, 50}, // wait IS IMPORTANT
	// start CC, activate OC
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_BACK, 0},	// Leave Cursor Mode
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0}, // OC
	{BUTTON_BACK, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	// start OC @ line 20
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0},
	{BUTTON_NEXT, 0}, // OC -> SIM
	{BUTTON_BACK, 0}, // Exit Menu
	{0, 0},
	{0, 0},
	{0, 0},
	// re-loop @ 30
	{0, 10},
	// Logbook
	{BUTTON_BACK, 	4},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	10},
	{BUTTON_ENTER, 20},
	{BUTTON_NEXT, 	20},
	{BUTTON_NEXT, 	20},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	5},
	{BUTTON_BACK, 	0}, // Exit Logbook Mode
	// surface screen
	{0, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},

	// menu
	// activate Configure Deco Gas
	{BUTTON_NEXT, 	0}, // Start Menu
	{BUTTON_NEXT, 	2}, // SIM -> OC
	{BUTTON_ENTER, 2},
	{BUTTON_NEXT, 	1},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	// Mix 30 -> 90
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_NEXT, 	0},
	{BUTTON_ENTER, 0},
	// Mix 90 -> 99
	{BUTTON_BACK, 	0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 5},
	// First -> Deco
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER, 2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER, 2},
	{BUTTON_ENTER, 10},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	30},
/*
	// OC -> SIM
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	1},
	{BUTTON_ENTER, 10},
*/
	// SYS2 Layout
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER,  2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER,  2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER,  2},
	{BUTTON_BACK, 	2},
	{BUTTON_BACK, 	10},
	// SYS2 -> SIM
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER, 10},
	// simulator
	{BUTTON_ENTER, 40}, // Start Simulator
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 90},
	
	{BUTTON_NEXT, 	5}, // Quit
	{BUTTON_NEXT, 	5},
	{BUTTON_ENTER,  5},
	{BUTTON_ENTER, 50},
	// surface screen
	{0, 10},
	// Logbook
	{BUTTON_BACK, 20},
	{BUTTON_ENTER, 20},
	{BUTTON_NEXT, 	20},
	{BUTTON_NEXT, 	20},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	5},
	{BUTTON_BACK, 	0}, // Exit Logbook Mode
	// surface screen
	{0, 10},
	// activate Configure Deco Gas, go to Gas 2
	{BUTTON_NEXT, 	0}, // Start Menu
	{BUTTON_NEXT, 	2}, // SIM -> OC
	{BUTTON_ENTER, 2},
	{BUTTON_NEXT, 	1},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	// Mix Decogas 99 -> EAN30 First
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	0},
	{BUTTON_BACK, 	0},
	{BUTTON_ENTER, 0},
	{BUTTON_NEXT, 	0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 0},
	{BUTTON_ENTER, 2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER, 5},
	{BUTTON_ENTER, 5},
	{BUTTON_ENTER, 10}, // Now EAN 30 First
	{BUTTON_BACK, 	0}, // Exit Edit Mode
	{BUTTON_BACK, 	30}, // Exit Cursor Mode
/*
	// OC -> SIM
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	1},
	{BUTTON_ENTER, 10},
*/	
	// SYS2 Layout
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER,  2},
	{BUTTON_NEXT, 	2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER,  2},
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER,  2},
	{BUTTON_BACK, 	2},
	{BUTTON_BACK, 	10},
	// SYS -> SIM
	{BUTTON_NEXT, 	2},
	{BUTTON_ENTER, 10},
	// simulator
	{BUTTON_ENTER, 40}, // Start Simulator
	{BUTTON_ENTER, 5},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 90},
	{BUTTON_NEXT, 	5}, // Menu?
	{BUTTON_NEXT, 	5}, // Quit?
	{BUTTON_ENTER, 10},
	{BUTTON_ENTER, 50},
	// the end, repeat from start
	{255, 0}
};
#endif


/* Exported functions --------------------------------------------------------*/

uint8_t demoModeActive(void)
{
	return demoModeActiveStatus;
}

uint8_t demoGetCommand(void)
{
	if(!demoModeActiveStatus)
		return 0;
	
	uint32_t milliSecondsSinceLast;
	uint32_t systick_now;


	systick_now = HAL_GetTick();
	milliSecondsSinceLast = time_elapsed_ms(systickDemoMode, systick_now);

	#ifdef SHOWCCRMODE
	if((demoModeActual == 1) && (settingsGetPointer()->dive_mode == DIVEMODE_CCR))
		demoModeActual = 20;
	#else
	if((demoModeActual == 1) && (settingsGetPointer()->dive_mode == DIVEMODE_OC))
		demoModeActual = 20;
	#endif
	
	if(demoModeActual > MAXCOMMANDS - 2)
		demoModeActual = 30;
	

	uint32_t actualDelay = ((commandlist[demoModeActual][1]) * 100) + 15;
	
	if(milliSecondsSinceLast < actualDelay)
		return 0;
	
	systickDemoMode = systick_now;
	
	if(commandlist[demoModeActual][0] == 255)
		demoModeActual = 30;
	else
		demoModeActual++;
	
	return commandlist[demoModeActual][0];
}


void demoSendCommand(uint8_t action)
{
	demoModeActiveStatus = 0;
	
}

void demoConfigureSettings(void)
{
	settingsGetPointer()->gas[1].oxygen_percentage = 21;
	settingsGetPointer()->gas[1].helium_percentage = 0;
	settingsGetPointer()->gas[1].depth_meter = 0;
	settingsGetPointer()->gas[1].note.ub.first = 0;
	settingsGetPointer()->gas[1].note.ub.active = 0;
	settingsGetPointer()->gas[1].note.ub.deco = 0;
	settingsGetPointer()->gas[1].note.ub.travel = 0;

	settingsGetPointer()->gas[2].oxygen_percentage = 30;
	settingsGetPointer()->gas[2].helium_percentage = 0;
	settingsGetPointer()->gas[2].depth_meter = 0;
	settingsGetPointer()->gas[2].note.ub.first = 1;
	settingsGetPointer()->gas[2].note.ub.active = 1;
	settingsGetPointer()->gas[2].note.ub.deco = 0;
	settingsGetPointer()->gas[2].note.ub.travel = 0;
	
	settingsGetPointer()->ButtonResponsiveness[0] = 90;
	settingsGetPointer()->ButtonResponsiveness[1] = 90;
	settingsGetPointer()->ButtonResponsiveness[2] = 90;
	settingsGetPointer()->ButtonResponsiveness[3] = 90;
}

/* Includes ------------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private variables with external access ------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/


#endif // DEMO

#ifdef ENABLE_USART_RADIO				/* debug function to check receiption of radio data */
void demo_HandleData(void)
{
	static uint8_t comStarted = 0;
	static uint8_t text[50];
	static uint8_t index = 0;
	static uint32_t startTick = 0;
	static uint8_t firstData = 1;
	uint8_t data = 0;

	switch(comStarted)
	{
		case 0: 	startTick = HAL_GetTick();
					comStarted++;
				break;
		case 1:		if(time_elapsed_ms(startTick, HAL_GetTick()) > 5000)
					{
						MX_UART_RADIO_Init_DMA();
						UART_StartDMARxRadio();
						comStarted++;
						sprintf((char*)text,"RadioStarted");
						InfoLogger_writeLine(text,strlen((char*)text),1);
					}
			break;
		case 2:				data = UART_getChar();
							if(data != 0)
							{
								if(firstData)
								{
									firstData = 0;
									sprintf((char*)text,"FirstData");
									InfoLogger_writeLine(text,strlen((char*)text),1);
								}
								if((index == 50) || (data =='r')  || (data =='n'))
								{
									if(index > 0)
									{
										InfoLogger_writeLine(text,index,0);
										index = 0;
									}
								}
								else
								{
									text[index++] = data;
								}
							}
			break;
		default:
			break;
	}


	{
	}
}
#endif


/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
