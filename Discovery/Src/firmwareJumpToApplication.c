/**
  ******************************************************************************
  * @file    firmwareJumpToApplication.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    05-May-2015
  * @version V0.0.1
  * @since   05-May-2015
  * @brief   jump to application in higher flash region
  *
  @verbatim
  ==============================================================================
                        ##### How to use #####
  ==============================================================================

  ==============================================================================
                        ##### From  AN2557 #####
							STM32F10xxx In-Application programming CD00161640.pdf   2010
  ==============================================================================
User program conditions
The user application to be loaded into the Flash memory using IAP should be built with
these configuration settings:
1. Set the program load address at 0x08003000, using your toolchain linker file
2. Relocate the vector table at address 0x08003000, using the
"NVIC_SetVectorTable"function or the VECT_TAB_OFFSET definition inside the
"system_stm32f10x.c"

can be found here system_stm32f4xx.c


	@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "firmwareJumpToApplication.h"

/* Exported variables --------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/
typedef  void (*pFunction)(void);
#define ApplicationAddress    0x08040000

/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t JumpAddress;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
uint8_t firmware_MainCodeIsProgammed(void)
{
	uint32_t content_start;
	content_start = *(__IO uint32_t*)ApplicationAddress;
	
	if ((content_start & 0x2FFE0000 ) == 0x20000000)
		return 1;
	else
		return 0;
}

void firmware_JumpTo_Application(void)
{
	/* Test if user code is programmed starting from address "ApplicationAddress" */
	/* The code checks if the address assigned for the estack is in a valid range. this check does not consider that a 192k STM32 version is in use */
	/* => workaround in linker file to store the estack in a valid range. TODO: change implementation for next generation bootloader */
	if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
	{ 
		/* Jump to user application */
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		Jump_To_Application = (pFunction) JumpAddress;
		/* Initialize user application's Stack Pointer */
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		Jump_To_Application();
	}
  while (1)
  {}
}

/* Private functions ---------------------------------------------------------*/
