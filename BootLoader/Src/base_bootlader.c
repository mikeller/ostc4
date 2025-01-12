///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   BootLoader/Src/base_bootlader.c
/// \brief  he beginning of it all. main() is part of this.
/// \author heinrichs weikamp gmbh
/// \date   26-February-2014
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2018 Heinrichs Weikamp gmbh
///
///     This program is free software: you can redistribute it and/or modify
///     it under the terms of the GNU General Public License as published by
///     the Free Software Foundation, either version 3 of the License, or
///     (at your option) any later version.
///
///     This program is distributed in the hope that it will be useful,
///     but WITHOUT ANY WARRANTY; without even the implied warranty of
///     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///     GNU General Public License for more details.
///
///     You should have received a copy of the GNU General Public License
///     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////
/**
	* @detail	 The beginning of it all. main() is part of this.
	*							 + Do the inits for hardware
	*							 + check for button press or update process reset trigger
	*							 + Do the inits for sub-systems like menu, dive screen etc.
	*							 + Start IRQs
	*							 + Start MainTasks not in IRQs
	* @bug
	* @warning
	@verbatim

	==============================================================================
							##### bootloader specific #####
	==============================================================================

	151130	hw	sleep on button3
								(MX_tell_reset_logik_alles_ok() + DataEX_call() in endless loop)

	==============================================================================
							##### bootloader specific #####
	==============================================================================

	Bootloader info is set right here in this file.
	The location is 0x0800A000 instead of 0x08050000 (firmware)

	on system reset (Menu Start Bootloader in firmware) the update process
	is started automatically if no button is pressed

	if the right button is pressed the bootloader menu is started

	after update process (with update or empty) cleaning of EEPROM is started
	afterwards the watchdog reset starts without activating the update process

	bluetooth chip is started in tComm on start of the mini bootloader firmware

	SMALLCPU_CSB_PIN must be re-set to 0 to communicate with small CPU / CPU2 / RTE

	for RealTerm to send file / firmware, Delays has to be increased to 0

	RTE update / SPI1 with DMA gave IBUSERR, now it is working fine :-) 150828
	==============================================================================
	from standard firmware, parts might be invalid here:
	==============================================================================
							##### IRQs #####
	==============================================================================
	[..] The IRQs are very important and most functions should only run there.

				PreemptPriority are as follows
				(#) 2 (low)		sprintf _only_ here. Don't use in maintask or anywhere else.
											Called by Buttons und Timer3
											Timer3 is 1/10 second
				(#) 1 (mid)		anything that should work while in IRQ2 like HalDelay(), VSYNC
											and DMA2D Transfer Complete for housekeepingFrame();
				(#)	0 (high)	_very very short_ interrupts like The HAL hardware part for
											spi, uart, i2c.

				SubPriority within 	PreemptPriority give the order to execute.
				Introduced 30.Oct.14 as it used by several HAL examples.
				Three levelAmbients are available (2 low,1 mid,0 high)

				The STM32F4 has 4bits for IRQ levelAmbients, divided 2/2 in this code
				with the NVIC_PRIORITYGROUP_2 setting.

	==============================================================================
							##### MainTask #####
	==============================================================================
	[..] For everything slow without importance to be 'in time'.
			 Like VPM and Buehlmann.
			 No sprintf and probably no GFX_SetFramesTopBottom() stuff neither.
			 If sprintf is called while sprintf is executed it blows up everything.

	==============================================================================
							##### Frames / the external SDRAM #####
	==============================================================================
	[..] The SDRAM is handled by getFrame() and releaseFrame().
			 Each frame with 800*480*2 Bytes.
			 Be careful to release every frame
			 otherwise there will be a memory leakage over time.
			 housekeepingFrame() in the MainTask takes care of cleaning the frames.
			 All frames are filled with 0x00. This will be transparent with color of
			 CLUT_Font020 (is CLUT 0) if the alpha is set for a 16bit pair.
			 housekeepingFrame() delays the cleaning of frames still used as screen
			 buffer to prevent flickering.

	==============================================================================
							##### Display #####
	==============================================================================
	[..] There is a Top layer, Bottom layer and background color.
			 All are perfectly alpha-blended by hardware.

				(#) top layer			has 800x480 option function calls only
													as it is not used for cursors here
				(#)	bottom layer	has free size and start option to be used
													for cursors (or sprites in the future ;-)
				(#) background		only black in the moment.
													ToDo: Could be anything else for warnings etc.
													if needed

	[..] Frame updates, switching and cursors is done with

				(#) GFX_SetFramesTopBottom() and the subset
						GFX_SetFrameTop() + GFX_SetFrameBottom()
						Those do not change anything on the display but give commands to..
				(#) GFX_change_LTDC()	The only place that changes the pointer.
															This prevents erratic behavior if several changes
															are made within one refresh rate of the screen.
															Is called in IRQ by PD4 and HAL_GPIO_EXTI_IRQHandler
															from VSYNC signal.

	[..] Content

				(#) Colors	by LookupTable only. This could be modified by
										system settings in the future. (gfx_color.h/.c)

				(#) Text		by text_multilinguage.h/.c with one char
										necessary only starting from '\x80'
										with automatic language switch by
										selected_language in SSettings
										see openEdit_Language() in tMenuEditSystem.c
										Therefore there are different functions
										for example:
										write_label_fix() for single char multilanguage
										write_label_var() for strings that could include
										multilanguage as well
										see GFX_write_string() to get an overview of the controls
										as well as the command list in gfx_engine.h
										There is no clear before writing, text overlay is always on.
										Many options to have LargeFont.SmallFont for numbers etc.

	==============================================================================
							##### Update, DualBoot and build-in FLASH memory usage #####
	==============================================================================
	[..] Boot0 pin, Boot1/PB2 pin and BFB2 software bit control the behaviour.
				PB2 should be tied to GND.
				Boot0 == VDD -> bootloader on start, otherwise boot from Bank1 or Bank2
				depending on BFB2.
				Bank2 contains the Fonts and should contain a proper test code in future
				Bank1 is the main code (Bank1 is 1 MB too, usage as of Oct. 14 is 200 KB)
	[..] Bootloader should be either UART or USB (on FS pins _only_)
				USB HS to FS like on the Eval board does not work.
	[..] Bootloader for the smaller CPU2 is implemented via the SPI used for DMA copy.

	==============================================================================
							##### Connection to CPU2 (STM32F411 as of Oct.14 #####
	==============================================================================
	[..] Connected via SPI and DMA for every purpose.
				two entire arrays are transfered for data security reasons
				with respect to master (STM32F429) might interrupt internal
				data copy in CPU2 (like hi byte, low byte, etc.).
	[..] The entire life data is calculated in CPU2. Like tissues, CNS,...
				Therefore the main unit is _not_ necessarily a Real Time system.
				Simulation on the main unit can be executed without disrupting life data.
	[..] SPI is triggered and timed by calling DataEX_call() in data_exchange_main.c
				DataEX_copy_to_LifeData() does the transfer from buffer to variables used.

	==============================================================================
							##### Menu, MenuEdit, Info #####
	==============================================================================
	[..] tMenu.c, tMenuEdit.c and tInfo.c is the system used.
				logbook is part of Info not Menu.
				The Info Menu is accessed by button 'Back'
				The regular Menu is accessed by button 'Enter'
	[..] Menu content is kept in frame memory for fast access.
				There is no need to build pages if the 'Enter' button is pressed.
				This is in contrast to MenuEdit pages.
	[..] Button control for new pages (and pages in general) have to implemented
				in tMenu.c, tMenuEdit.c or tInfo.c

	[..] ToDo (Oct. 14) Timeout for menus via Timer3 / IRQ 2

	==============================================================================
							##### specials #####
	==============================================================================
	[..] There was code for vector graphics from great demos
				(peridiummmm and jupiter) that can be fitted again

	 @endverbatim
	******************************************************************************
	*/

/* Includes ------------------------------------------------------------------*/
#include "base_bootloader.h"

// From Bootloader/Inc:
#include "tInfoBootloader.h"

// ?
#include "externLogbookFlash.h"
#include "firmwareEraseProgram.h"
#include "firmwareJumpToApplication.h"

// From Common/Inc:
#include "FirmwareData.h"

// From Common/Drivers:
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_flash_ex.h"
#include "stm32f4xx_hal_wwdg.h"

#ifdef BOOTLOADER_STANDALONE
#include "Fonts/Font_T144_plus.h"
#include "Fonts/Font_T84.h"
#include "Fonts/Font_T105.h"
#include "Fonts/Font_T54.h"
#include "Fonts/Font_T48_plus.h"
#include "Fonts/Font_T24.h"
#include "Fonts/Font_T42.h"
#include "Fonts/image_battery.h"
#include "Fonts/image_heinrichs_weikamp.h"
#include "Fonts/image_ostc.h"
#endif

// From Discovery/Inc (shall be shared...)
#include "data_exchange_main.h"
#include "display.h"
#include "gfx_engine.h"
#include "ostc.h"
#include "tComm.h"
#include "tStructure.h"

// From AC6 support:
#include <stdio.h>
#include <string.h> // for memcopy

/* Private define ------------------------------------------------------------*/
#define BUFFER_SIZE         ((uint32_t)0x00177000)
#define WRITE_READ_ADDR     ((uint32_t)0x0000)
#define REFRESH_COUNT       ((uint32_t)0x0569)   /* SDRAM refresh counter (90Mhz SD clock) */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t returnFromCommCleanUpRequest = 0;

const SFirmwareData bootloader_FirmwareData __attribute__(( section(".bootloader_firmware_data") )) =
{
		.versionFirst   = 1,
		.versionSecond  = 0,
		.versionThird   = 1,
		.versionBeta    = 1,

	/* 4 bytes with trailing 0 */
	.signature = "mh",

	.release_year   = 25,
	.release_month  = 1,
	.release_day    = 11,
	.release_sub    = 0,

	/* max 48 with trailing 0 */
	.release_info ="tComm with all",

	/* for safety reasons and coming functions*/
	.magic[0] = FIRMWARE_MAGIC_FIRST,
	.magic[1] = FIRMWARE_MAGIC_SECOND,
	.magic[2] = FIRMWARE_MAGIC_FIRMWARE, /* the magic byte */
	.magic[3] = FIRMWARE_MAGIC_END
};

#if 1
const SHardwareData HardwareData  __attribute__ ((section (".bootloader_hardware_data"))) = {

	// first 52 bytes
	.primarySerial = 0xFFFF,
	.primaryLicence	= 0xFF,
	.revision8bit = 0xFF,
	.production_year = 0xFF,
	.production_month = 0xFF,
	.production_day = 0xFF,
	.production_bluetooth_name_set = 0xFF,

	.production_info = {
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},

	// other 12 bytes (64 in total)
	.secondarySerial = 0xFFFF,
	.secondaryLicence = 0xFF,
	.secondaryReason8bit = 0xFF,
	.secondary_year = 0xFF,
	.secondary_month = 0xFF,
	.secondary_day = 0xFF,
	.secondary_bluetooth_name_set = 0xFF,
	.secondary_info = {0xFF,0xFF,0xFF,0xFF}
};
#endif

RTC_HandleTypeDef	RtcHandle;
TIM_HandleTypeDef   TimHandle; /* used in stm32f4xx_it.c too */
TIM_HandleTypeDef   TimBacklightHandle; /* used in stm32f4xx_it.c too */

uint32_t time_before;
uint32_t time_between;
uint32_t time_after;

/* SDRAM handler declaration */
SDRAM_HandleTypeDef hsdram;
FMC_SDRAM_TimingTypeDef SDRAM_Timing;
FMC_SDRAM_CommandTypeDef command;

FLASH_OBProgramInitTypeDef    OBInit;
FLASH_AdvOBProgramInitTypeDef AdvOBInit;


/* Private variables with external access ------------------------------------*/

uint32_t globalStateID = 0;
uint8_t globalModeID = SURFMODE;
uint32_t time_without_button_pressed_deciseconds = 0;
uint8_t bootToBootloader = 0;

/* Private function prototypes -----------------------------------------------*/

//static void LCD_ToggleFramebuffer(GFX_DrawCfgTypeDef *hconfig);
//static void LCD_Config(GFX_DrawCfgTypeDef *hconfig);
static void SystemClock_Config(void);
static void Error_Handler(void);

static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command);
static void SDRAM_Config(void);
//static void DualBoot(void);
static void EXTILine_Buttons_Config(void);
//static void RTC_init(void);
static void TIM_init(void);
static void TIM_BACKLIGHT_init(void);
//static void TIM_BACKLIGHT_adjust(void);
static void gotoSleep(void);
uint8_t checkResetForFirmwareUpdate(void);
void DeleteResetToFirmwareUpdateRegister(void);
void reset_to_firmware_using_Watchdog(void);
void reset_to_update_using_system_reset(void);

//static void DualBootToBootloader(void);

/* ITM Trace-------- ---------------------------------------------------------*/
/*
#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000

struct __FILE { int handle; };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f) {
	if (DEMCR & TRCENA) {
		while (ITM_Port32(0) == 0);
		ITM_Port8(0) = ch;
	}
	return(ch);
}
*/

/* Private functions ---------------------------------------------------------*/

/**
	* @brief   Main program
	* @param  None
	* @retval None
	*/

void GPIO_test_I2C_lines(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;
	__GPIOA_CLK_ENABLE();
	__GPIOG_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPIO_PIN_7;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);
	GPIO_InitStructure.Pin = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	while(1)
	{
	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_7,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(GPIOG,GPIO_PIN_7,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_3,GPIO_PIN_SET);
	HAL_Delay(10);
	}
}


int main(void)
{

/*
	HAL_Init();
	SystemClock_Config();
GPIO_test_I2C_lines();
*/
	uint32_t pLayerInvisible;
	uint32_t firmware_load_result;
	uint8_t magicbyte = 0;
	uint8_t callForUpdate;
	uint8_t status = 0;
	char textVersion[32];
	uint8_t ptr;
	uint32_t pOffset;

	const SHardwareData* HardwareData = hardwareDataGetPointer();

	set_globalState(StBoot0);

	HAL_Init();
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2);
	SystemClock_Config();

	MX_GPIO_Init();

	/* button press is only 40 to 50 us low */
	MX_GPIO_One_Button_only_Init();

	uint32_t i = 500000;

	callForUpdate = __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST);

	if(callForUpdate)
	{
		i = 0;
	}
	else
	if(			(firmware_MainCodeIsProgammed() == 0)
			||	(HardwareData->primarySerial == 0xFFFF)
			||	(HardwareData->production_bluetooth_name_set == 0xFF))
	{
		i = 1;
	}
	else
	{
		while(MX_GPIO_Read_The_One_Button() && i)
		{
			i--;
			__NOP();
		}
		if(i)
		{
			i = 200000;
			while(!MX_GPIO_Read_The_One_Button() && i)
			{
				i--;
				__NOP();
			}
			if(i)
			{
				i = 200000;
				while(MX_GPIO_Read_The_One_Button() && i)
				{
					i--;
					__NOP();
				}
				if(i)
				{
					i = 200000;
					while(!MX_GPIO_Read_The_One_Button() && i)
					{
						i--;
						__NOP();
					}
					if(i)
					{
						i = 200000;
						while(MX_GPIO_Read_The_One_Button() && i)
						{
							i--;
							__NOP();
						}
					}
				}
			}
		}
	}

	if((i == 0) && (callForUpdate == 0))
		firmware_JumpTo_Application();

	MX_SPI_Init();
	SDRAM_Config();
	HAL_Delay(100);

	GFX_init1_no_DMA(&pLayerInvisible, 2);

	TIM_BACKLIGHT_init();

// -----------------------------

	display_power_on__1_of_2__pre_RGB();
	GFX_LTDC_Init();
	GFX_LTDC_LayerDefaultInit(TOP_LAYER, pLayerInvisible);
	GFX_LTDC_LayerDefaultInit(BACKGRD_LAYER, pLayerInvisible);
	GFX_SetFramesTopBottom(pLayerInvisible,pLayerInvisible,480);
	HAL_Delay(20);
	display_power_on__2_of_2__post_RGB();

// -----------------------------
	GFX_change_LTDC();
	GFX_hwBackgroundOn();
	GFX_change_LTDC();
// -----------------------------
	tInfoBootloader_init();
// -----------------------------
	if(i == 0)
	{
		tInfo_newpage("load firmware data");
		uint8_t* pBuffer = (uint8_t*)((uint32_t)0xD0000000); /* blocked via  GFX_init1_no_DMA */
		firmware_load_result = ext_flash_read_firmware(pBuffer,768000, &magicbyte);

		if((firmware_load_result > 0) && (firmware_load_result < 768000) && (magicbyte == 0xEE))
		{
			ptr = ext_flash_read_firmware_version(textVersion);
			textVersion[ptr++] = 'f';
			textVersion[ptr++] = 'o';
			textVersion[ptr++] = 'u';
			textVersion[ptr++] = 'n';
			textVersion[ptr++] = 'd';
			textVersion[ptr] = 0;

			tInfo_newpage(textVersion);
			tInfo_write("erase flash");
			status = firmware_eraseFlashMemory();
			if(status != HAL_OK)
			{
				tInfo_newpage("error. try again.");
				status = firmware_eraseFlashMemory();
				if(status != HAL_OK)
				{
					tInfo_newpage("error. skip update.");
					HAL_Delay(1000);
				}
			}
			if(status == HAL_OK)
			{
				tInfo_write("program flash");
				status = firmware_programFlashMemory(pBuffer,firmware_load_result);
				if(status != HAL_OK)
				{
					tInfo_newpage("error. try again.");
					status = firmware_programFlashMemory(pBuffer,firmware_load_result);
					if(status != HAL_OK)
					{
						tInfo_newpage("error. skip update.");
						HAL_Delay(1000);
					}
				}
			}
		}
	}

	/* here comes the variable upper firmware loader */
	if((i == 0) && (status == HAL_OK))
	{
		tInfo_newpage("load fontpack data");
		uint8_t* pBuffer = (uint8_t*)((uint32_t)0xD0000000); /* blocked via  GFX_init1_no_DMA */
		firmware_load_result = ext_flash_read_firmware2(&pOffset, pBuffer,768000*2,0,0);

		if((firmware_load_result > 0) && (firmware_load_result + pOffset <= 1024000))
		{
			ptr = 0;
			ptr += gfx_number_to_string(7,0,&textVersion[ptr],firmware_load_result);
			textVersion[ptr++] = ' ';
			textVersion[ptr++] = 'b';
			textVersion[ptr++] = 'y';
			textVersion[ptr++] = 't';
			textVersion[ptr++] = 'e';
			textVersion[ptr++] = 's';
			textVersion[ptr++] = ' ';
			textVersion[ptr++] = 'w';
			textVersion[ptr++] = 'i';
			textVersion[ptr++] = 't';
			textVersion[ptr++] = 'h';
			textVersion[ptr++] = ' ';
			ptr += gfx_number_to_string(7,0,&textVersion[ptr],pOffset);
			textVersion[ptr++] = ' ';
			textVersion[ptr++] = 'o';
			textVersion[ptr++] = 'f';
			textVersion[ptr++] = 'f';
			textVersion[ptr++] = 's';
			textVersion[ptr++] = 'e';
			textVersion[ptr++] = 't';
			textVersion[ptr] = 0;
			tInfo_newpage(textVersion);

			ptr = 0;
			textVersion[ptr++] = 'f';
			textVersion[ptr++] = 'o';
			textVersion[ptr++] = 'u';
			textVersion[ptr++] = 'n';
			textVersion[ptr++] = 'd';
			textVersion[ptr] = 0;

			tInfo_write(textVersion);
			tInfo_write("erase flash");
			status = firmware2_variable_upperpart_eraseFlashMemory(firmware_load_result,pOffset);
			if(status != HAL_OK)
			{
				tInfo_newpage("error. try again.");
				status = firmware2_variable_upperpart_eraseFlashMemory(firmware_load_result,pOffset);
				if(status != HAL_OK)
				{
					tInfo_newpage("error. skip update.");
					HAL_Delay(1000);
				}
			}
			if(status == HAL_OK)
			{
				tInfo_write("program flash");
				status = firmware2_variable_upperpart_programFlashMemory(firmware_load_result,pOffset,pBuffer,firmware_load_result,0);
				if(status != HAL_OK)
				{
					tInfo_newpage("error. try again.");
					status = firmware2_variable_upperpart_programFlashMemory(firmware_load_result,pOffset,pBuffer,firmware_load_result,0);
					if(status != HAL_OK)
					{
						tInfo_newpage("error. skip update.");
						HAL_Delay(1000);
					}
				}
			}
		}
	}

	if((i == 0) && (status == HAL_OK))
	{
		tInfo_newpage("done.");
		tInfo_write("cleaning.");
		ext_flash_erase_firmware_if_not_empty();
		ext_flash_erase_firmware2_if_not_empty();
		tInfo_write("reset device.");
		reset_to_firmware_using_Watchdog();
	}

	ptr = 0;
	textVersion[ptr++] = '\020';
	textVersion[ptr++] = 's';
	textVersion[ptr++] = 'e';
	textVersion[ptr++] = 'r';
	textVersion[ptr++] = 'i';
	textVersion[ptr++] = 'a';
	textVersion[ptr++] = 'l';
	textVersion[ptr++] = ' ';
	if(HardwareData->primarySerial == 0xFFFF)
	{
		textVersion[ptr++] = 'n';
		textVersion[ptr++] = 'o';
		textVersion[ptr++] = 't';
		textVersion[ptr++] = ' ';
		textVersion[ptr++] = 's';
		textVersion[ptr++] = 'e';
		textVersion[ptr++] = 't';
	}
	else if(HardwareData->secondarySerial == 0xFFFF)
	{
		textVersion[ptr++] = '#';
		ptr += gfx_number_to_string(5,1,&textVersion[ptr],HardwareData->primarySerial);
	}
	else
	{
		textVersion[ptr++] = '#';
		ptr += gfx_number_to_string(5,1,&textVersion[ptr],HardwareData->secondarySerial);
		textVersion[ptr++] = ' ';
		textVersion[ptr++] = '(';
		ptr += gfx_number_to_string(5,1,&textVersion[ptr],HardwareData->primarySerial);
		textVersion[ptr++] = ')';
	}
	textVersion[ptr++] = '\020';
	textVersion[ptr] = 0;

	TIM_init();
	MX_UART_Init();
	MX_Bluetooth_PowerOn();
	tComm_init();

	tInfo_button_text("exit","","sleep");
	tInfo_newpage("bootloader 250111");
	tInfo_write("start bluetooth");
	tInfo_write("");
	tInfo_write(textVersion);
	if(tComm_Set_Bluetooth_Name(0) == 0xFF)
	{
		tInfo_write("init bluetooth");
		tComm_StartBlueModBaseInit();
	}
	else
	{
		tInfo_write("bluetooth set");
		tComm_StartBlueModConfig();
	}

	set_globalState_Base();

	GFX_start_VSYNC_IRQ();

	EXTILine_Buttons_Config();
/*
	uint8_t* pBuffer1 = (uint8_t*)getFrame(20);
	firmware_load_result = ext_flash_read_firmware(pBuffer1,768000);

	if((firmware_load_result > 0) && (firmware_load_result < 768000))
	{
		firmware_eraseFlashMemory();
		firmware_programFlashMemory(pBuffer1,firmware_load_result);
		// not for testing
		//ext_flash_erase_firmware_if_not_empty();
		reset_to_firmware_using_Watchdog();
	}
*/
	while(1)
	{
//		if(bootToBootloader)
//			DualBootToBootloader();

		if(bootToBootloader)
			reset_to_update_using_system_reset();

		tComm_control(); // will stop while loop if tComm Mode started until exit from UART
	};
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	SStateList status;

	get_globalStateList(&status);

	switch(status.base)
	{
		default:
//			TIM_BACKLIGHT_adjust();
			break;
	}

	if(returnFromCommCleanUpRequest)
	{
		tComm_exit();
		returnFromCommCleanUpRequest = 0;
		GFX_hwBackgroundOn();
		tInfo_button_text("exit","","sleep");
		tInfo_newpage("bluetooth disonnected");
		tInfo_write("");
		tInfo_write("");
		tInfo_write("");
		tInfo_write("");
	}

	get_globalStateList(&status);

	switch(status.base)
	{
		case BaseComm:
			if(get_globalState() == StUART_STANDARD)
			 tComm_refresh();
			break;
		default:
			break;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	uint8_t action;
	SStateList status;
	static uint8_t counterToPreventSleep = 0;
	if(GPIO_Pin == VSYNC_IRQ_PIN) // rechts, unten
	{
		GFX_change_LTDC();
		housekeepingFrame();
		if(counterToPreventSleep < 250)
			counterToPreventSleep++;
		else
		if(counterToPreventSleep != 255)
		{
			counterToPreventSleep = 255;
		}

		return;
	}

	time_without_button_pressed_deciseconds	= 0;

	if(GFX_logoStatus() != 0)
		return;

	if(GPIO_Pin == BUTTON_BACK_PIN) // left
		action = ACTION_BUTTON_BACK;
	else
	if(GPIO_Pin == BUTTON_ENTER_PIN) // center
		action = ACTION_BUTTON_ENTER;
	else
	if(GPIO_Pin == BUTTON_NEXT_PIN) // right
		action = ACTION_BUTTON_NEXT;
#ifdef BUTTON_CUSTOM_PIN
	else
	if(GPIO_Pin == BUTTON_CUSTOM_PIN) // extra
		action = ACTION_BUTTON_CUSTOM;
#endif
	else
		action = 0;
	get_globalStateList(&status);

	switch(status.base)
	{
		case BaseComm:
			if(action == ACTION_BUTTON_BACK)
			{
				reset_to_firmware_using_Watchdog();
			}
		 break;

		default:
			if((action == ACTION_BUTTON_NEXT) && (counterToPreventSleep == 255) && (get_globalState() == StS))
			{
				while(1)
				{
					MX_tell_reset_logik_alles_ok();
					DataEX_call();
					HAL_Delay(100);
				}
			}
			else
			if(action == ACTION_BUTTON_BACK)
			{
				reset_to_firmware_using_Watchdog();
			}
			else
			if(action == ACTION_BUTTON_CUSTOM)
			{
				if(get_globalState() == StS)
					gotoSleep();
			}
			else
			if(action == ACTION_BUTTON_ENTER)
			{
				reset_to_update_using_system_reset();
			}
			break;
	}
}


void gotoSleep(void)
{
	ext_flash_erase_firmware_if_not_empty();
	set_globalState(StStop);
}

// -----------------------------


void MainBootLoaderInit(void)
{
	void (*SysMemBootJump)(void);
	SysMemBootJump=(void (*)(void)) (*((uint32_t *) 0x1fff0004));

	// DMA, SPI, UART, TIM, ExtIRQ, graphics DMA, LTDC

	HAL_RCC_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	__set_PRIMASK(1);

	__set_MSP(0x20002318);
	SysMemBootJump();
}

uint32_t get_globalState(void)
{
	return globalStateID;
}

void get_globalStateList(SStateList *output)
{
	output->base  = (uint8_t)((globalStateID >> 28) & 0x0F);
	output->page  = (uint8_t)((globalStateID >> 24) & 0x0F);
	output->line  = (uint8_t)((globalStateID >> 16) & 0xFF);
	output->field = (uint8_t)((globalStateID >> 8) & 0xFF);
	output->mode  = (uint8_t)((globalStateID     ) & 0xFF);
}

void get_idSpecificStateList(uint32_t id, SStateList *output)
{
	output->base  = (uint8_t)((id >> 28) & 0x0F);
	output->page  = (uint8_t)((id >> 24) & 0x0F);
	output->line  = (uint8_t)((id >> 16) & 0xFF);
	output->field = (uint8_t)((id >> 8) & 0xFF);
	output->mode  = (uint8_t)((id     ) & 0xFF);
}

void	set_globalState_Base(void)
{
	set_globalState(StS);
}

void set_globalState_Menu_Page(uint8_t page)
{
	globalStateID = ((BaseMenu << 28) + (page << 24));
}

void set_globalState_Log_Page(uint8_t pageIsLine)
{
	globalStateID = StILOGLIST + (pageIsLine << 16);
}


void set_globalState_Menu_Line(uint8_t line)
{
	globalStateID = ((globalStateID & MaskLineFieldDigit) + (line << 16));
}


void set_globalState(uint32_t newID)
{
	globalStateID = newID;
}



void delayMicros(uint32_t micros)
{
	micros = micros * (168/4) - 10;
	while(micros--);
}


void get_RTC_DateTime(RTC_DateTypeDef * sdatestructureget, RTC_TimeTypeDef * stimestructureget)
{
	/* Get the RTC current Time */
	if(sdatestructureget)
		HAL_RTC_GetTime(&RtcHandle, stimestructureget, FORMAT_BIN);
	/* Get the RTC current Date */
	if(stimestructureget)
		HAL_RTC_GetDate(&RtcHandle, sdatestructureget, FORMAT_BIN);
}


void set_RTC_DateTime(RTC_DateTypeDef * sdatestructure, RTC_TimeTypeDef * stimestructure)
{
	if(sdatestructure)
		if(HAL_RTC_SetDate(&RtcHandle,sdatestructure,FORMAT_BCD) != HAL_OK)
		{
			/* Initialization Error */
			Error_Handler();
		}

	if(stimestructure)
		if(HAL_RTC_SetTime(&RtcHandle,stimestructure,FORMAT_BCD) != HAL_OK)
		{
			/* Initialization Error */
			Error_Handler();
		}
}

static void TIM_init(void)
{
	uint16_t uwPrescalerValue = 0;

	uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 10000) - 1;

	/* Set TIMx instance */
	TimHandle.Instance = TIMx;

	/* Initialize TIM3 peripheral as follows:
			 + Period = 10000 - 1
			 + Prescaler = ((SystemCoreClock/2)/10000) - 1
			 + ClockDivision = 0
			 + Counter direction = Up
	*/
	TimHandle.Init.Period = 1000 - 1;
	TimHandle.Init.Prescaler = uwPrescalerValue;
	TimHandle.Init.ClockDivision = 0;
	TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
	if(HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/*##-2- Start the TIM Base generation in interrupt mode ####################*/
	/* Start Channel1 */
	if(HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
	{
		/* Starting Error */
		Error_Handler();
	}
}

#ifndef TIM_BACKLIGHT
/*
static void TIM_BACKLIGHT_adjust(void)
{
}
*/
static void TIM_BACKLIGHT_init(void)
{
}
#else
/*
static void TIM_BACKLIGHT_adjust(void)
{

	TIM_OC_InitTypeDef sConfig;
	sConfig.OCMode     = TIM_OCMODE_PWM1;
	sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfig.OCFastMode = TIM_OCFAST_DISABLE;
	sConfig.Pulse = 600;

	HAL_TIM_PWM_ConfigChannel(&TimBacklightHandle, &sConfig, TIM_BACKLIGHT_CHANNEL);
	HAL_TIM_PWM_Start(&TimBacklightHandle, TIM_BACKLIGHT_CHANNEL);
}
*/
static void TIM_BACKLIGHT_init(void)
{
	uint32_t uwPrescalerValue = 0;
	TIM_OC_InitTypeDef sConfig;

	uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 18000000) - 1;

	TimBacklightHandle.Instance = TIM_BACKLIGHT;

	// Initialize TIM3 peripheral as follows:	30 kHz

	TimBacklightHandle.Init.Period = 600 - 1;
	TimBacklightHandle.Init.Prescaler = uwPrescalerValue;
	TimBacklightHandle.Init.ClockDivision = 0;
	TimBacklightHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
	HAL_TIM_PWM_Init(&TimBacklightHandle);

	sConfig.OCMode     = TIM_OCMODE_PWM1;
	sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfig.OCFastMode = TIM_OCFAST_DISABLE;
	sConfig.Pulse = 50 * 6;

	HAL_TIM_PWM_ConfigChannel(&TimBacklightHandle, &sConfig, TIM_BACKLIGHT_CHANNEL);
	HAL_TIM_PWM_Start(&TimBacklightHandle, TIM_BACKLIGHT_CHANNEL);
}
#endif

	/* Configure RTC prescaler and RTC data registers */
	/* RTC configured as follow:
			- Hour Format    = Format 24
			- Asynch Prediv  = Value according to source clock
			- Synch Prediv   = Value according to source clock
			- OutPut         = Output Disable
			- OutPutPolarity = High Polarity
			- OutPutType     = Open Drain */
	/*#define RTC_ASYNCH_PREDIV  0x7F    LSE as RTC clock */
	/*LSE: #define RTC_SYNCH_PREDIV   0x00FF  LSE as RTC clock */
	/*LSI: #define RTC_SYNCH_PREDIV   0x0130  LSI as RTC clock */
/*
static void RTC_init(void)
{
	RtcHandle.Instance = RTC;


	RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
	RtcHandle.Init.AsynchPrediv = 0x7F;
	RtcHandle.Init.SynchPrediv = 0x0130;
	RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
	RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	if(HAL_RTC_Init(&RtcHandle) != HAL_OK)
	{
		Error_Handler();
	}
}
*/

static void EXTILine_Buttons_Config(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;

	BUTTON_ENTER_GPIO_ENABLE();
	BUTTON_NEXT_GPIO_ENABLE();
	BUTTON_BACK_GPIO_ENABLE();

	/* Configure pin as weak PULLUP input  */
	/* buttons */
	GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;

	GPIO_InitStructure.Pin = BUTTON_ENTER_PIN;
	HAL_GPIO_Init(BUTTON_ENTER_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = BUTTON_NEXT_PIN;
	HAL_GPIO_Init(BUTTON_NEXT_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = BUTTON_BACK_PIN;
	HAL_GPIO_Init(BUTTON_BACK_GPIO_PORT, &GPIO_InitStructure);

	/* Enable and set EXTI Line0 Interrupt to the lowest priority */
	HAL_NVIC_SetPriority(BUTTON_ENTER_EXTI_IRQn, 2, 0);
	HAL_NVIC_SetPriority(BUTTON_NEXT_EXTI_IRQn,  2, 0);
	HAL_NVIC_SetPriority(BUTTON_BACK_EXTI_IRQn,  2, 0);
	HAL_NVIC_EnableIRQ(BUTTON_ENTER_EXTI_IRQn);
	HAL_NVIC_EnableIRQ(BUTTON_NEXT_EXTI_IRQn);
	HAL_NVIC_EnableIRQ(BUTTON_BACK_EXTI_IRQn);

#ifdef BUTTON_CUSTOM_PIN
	BUTTON_CUSTOM_GPIO_ENABLE();
	GPIO_InitStructure.Pin = BUTTON_CUSTOM_PIN;
	HAL_GPIO_Init(BUTTON_CUSTOM_GPIO_PORT, &GPIO_InitStructure);
	HAL_NVIC_SetPriority(BUTTON_CUSTOM_EXTI_IRQn,  2, 0);
	HAL_NVIC_EnableIRQ(BUTTON_CUSTOM_EXTI_IRQn);
#endif
}


/**
	* @brief  System Clock Configuration
	*         The system Clock is configured as follow :
	*            System Clock source            = PLL (HSE)
	*            SYSCLK(Hz)                     = 180000000
	*            HCLK(Hz)                       = 180000000
	*            AHB Prescaler                  = 1
	*            APB1 Prescaler                 = 4
	*            APB2 Prescaler                 = 2
	*            HSE Frequency(Hz)              = 8000000
	*            PLL_M                          = 8
	*            PLL_N                          = 360
	*            PLL_P                          = 2
	*            PLL_Q                          = 7
	*            VDD(V)                         = 3.3
	*            Main regulator output voltage  = Scale1 mode
	*            Flash Latency(WS)              = 5
	*         The LTDC Clock is configured as follow :
	*            PLLSAIN                        = 192
	*            PLLSAIR                        = 4
	*            PLLSAIDivR                     = 8
	* @param  None
	* @retval None
	*/
static void SystemClock_Config(void)
{
    /* Enable Power Control clock */
    __PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG( PWR_REGULATOR_VOLTAGE_SCALE1 );

    /*##-1- System Clock Configuration #########################################*/
    /* Enable HighSpeed Oscillator and activate PLL with HSE/HSI as source */
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
#ifdef DISC1_BOARD
    // Use High Speed Internal (HSI) oscillator, running at 16MHz.
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = 0x10;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM       = 16;				// HSI/16 is 1Mhz.
#else
    // Use High Speed External oscillator, running at 8MHz
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 8;               // HSE/8 is 1Mhz.
#endif
    // System clock = PLL (1MHz) * N/p = 180 MHz.
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    HAL_RCC_OscConfig( &RCC_OscInitStruct );

//  HAL_PWREx_ActivateOverDrive();
    HAL_PWREx_DeactivateOverDrive();

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                                                            | RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig( &RCC_ClkInitStruct, FLASH_LATENCY_8 );	//FLASH_LATENCY_5);

    /*##-2- LTDC Clock Configuration ###########################################*/
    /* LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/4 = 48 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / RCC_PLLSAIDIVR_8 = 48/8 = 6 Mhz */

    /* neu: 8MHz/8*300/5/8 = 7,5 MHz = 19,5 Hz bei 800 x 480 */
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 300;				//192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;				//4;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;//RCC_PLLSAIDIVR_4;// RCC_PLLSAIDIVR_2; // RCC_PLLSAIDIVR_8
    HAL_RCCEx_PeriphCLKConfig( &PeriphClkInitStruct );
}


/**
	* @brief  This function is executed in case of error occurrence.
	* @param  None
	* @retval None
	*/
static void Error_Handler(void)
{
		/* Turn LED3 on */
//    BSP_LED_On(LED3);
		while(1)
		{
		}
}

/**
	* @brief  Perform the SDRAM external memory initialization sequence
	* @param  hsdram: SDRAM handle
	* @param  Command: Pointer to SDRAM command structure
	* @retval None
	*/
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
	__IO uint32_t tmpmrd =0;
	/* Step 3:  Configure a clock configuration enable command */
	Command->CommandMode 			 = FMC_SDRAM_CMD_CLK_ENABLE;
	Command->CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 1;
	Command->ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 4: Insert 100 ms delay */
	HAL_Delay(100);

	/* Step 5: Configure a PALL (precharge all) command */
	Command->CommandMode 			 = FMC_SDRAM_CMD_PALL;
	Command->CommandTarget 	     = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 1;
	Command->ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 6 : Configure a Auto-Refresh command */
	Command->CommandMode 			 = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	Command->CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 4;
	Command->ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 7: Program the external memory mode register */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2          |
										 SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
										 SDRAM_MODEREG_CAS_LATENCY_3           |
										 SDRAM_MODEREG_OPERATING_MODE_STANDARD |
										 SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	Command->CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	Command->CommandTarget 		 = FMC_SDRAM_CMD_TARGET_BANK2;
	Command->AutoRefreshNumber 	 = 1;
	Command->ModeRegisterDefinition = tmpmrd;

	/* Send the command */
	HAL_SDRAM_SendCommand(hsdram, Command, 0x1000);

	/* Step 8: Set the refresh rate counter */
	/* (15.62 us x Freq) - 20 */
	/* neu: (8 us x Freq) - 20 */
	/* Set the device refresh counter */
	HAL_SDRAM_ProgramRefreshRate(hsdram, REFRESH_COUNT);
}

/*
static void DualBoot(void)
{
		// Set BFB2 bit to enable boot from Flash Bank2
		// Allow Access to Flash control registers and user Flash
		HAL_FLASH_Unlock();

		// Allow Access to option bytes sector
		HAL_FLASH_OB_Unlock();

		// Get the Dual boot configuration status
		AdvOBInit.OptionType = OBEX_BOOTCONFIG;
		HAL_FLASHEx_AdvOBGetConfig(&AdvOBInit);

		// Enable/Disable dual boot feature
		if (((AdvOBInit.BootConfig) & (FLASH_OPTCR_BFB2)) == FLASH_OPTCR_BFB2)
		{
			AdvOBInit.BootConfig = OB_DUAL_BOOT_DISABLE;
			HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
		}
		else
		{
			AdvOBInit.BootConfig = OB_DUAL_BOOT_ENABLE;
			HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
		}

		// Start the Option Bytes programming process
		if (HAL_FLASH_OB_Launch() != HAL_OK)
		{
			// User can add here some code to deal with this error
			while (1)
			{
			}
		}
		// Prevent Access to option bytes sector
		HAL_FLASH_OB_Lock();

		// Disable the Flash option control register access (recommended to protect
		// the option Bytes against possible unwanted operations)
		HAL_FLASH_Lock();

		// Initiates a system reset request to reset the MCU
		reset_to_firmware_using_Watchdog();
}
*/
/**
	******************************************************************************
	******************************************************************************
	******************************************************************************
	*/


/**
	* @brief DMA2D configuration.
	* @note  This function Configure the DMA2D peripheral :
	*        1) Configure the transfer mode : memory to memory W/ pixel format conversion
	*        2) Configure the output color mode as ARGB4444
	*        3) Configure the output memory address at SRAM memory
	*        4) Configure the data size : 320x120 (pixels)
	*        5) Configure the input color mode as ARGB8888
	*        6) Configure the input memory address at FLASH memory
	* @retval
	*  None
	*/

static void SDRAM_Config(void)
{
	/*##-1- Configure the SDRAM device #########################################*/
	/* SDRAM device configuration */
	hsdram.Instance = FMC_SDRAM_DEVICE;

	/* Timing configuration for 90 Mhz of SD clock frequency (180Mhz/2) */
	/* TMRD: 2 Clock cycles */
	SDRAM_Timing.LoadToActiveDelay    = 2;
	/* TXSR: min=70ns (6x11.90ns) */
	SDRAM_Timing.ExitSelfRefreshDelay = 7;
	/* TRAS: min=42ns (4x11.90ns) max=120k (ns) */
	SDRAM_Timing.SelfRefreshTime      = 4;
	/* TRC:  min=63 (6x11.90ns) */
	SDRAM_Timing.RowCycleDelay        = 7;
	/* TWR:  2 Clock cycles */
	SDRAM_Timing.WriteRecoveryTime    = 2;
	/* TRP:  15ns => 2x11.90ns */
	SDRAM_Timing.RPDelay              = 2;
	/* TRCD: 15ns => 2x11.90ns */
	SDRAM_Timing.RCDDelay             = 2;

	hsdram.Init.SDBank             = FMC_SDRAM_BANK2;
	hsdram.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_9;
	hsdram.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_13;
	hsdram.Init.MemoryDataWidth    = SDRAM_MEMORY_WIDTH;
	hsdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
	hsdram.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
	hsdram.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	hsdram.Init.SDClockPeriod      = SDCLOCK_PERIOD;
	hsdram.Init.ReadBurst          = FMC_SDRAM_RBURST_DISABLE;
	hsdram.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_1;

	/* Initialize the SDRAM controller */
	if(HAL_SDRAM_Init(&hsdram, &SDRAM_Timing) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/* Program the SDRAM external device */
	SDRAM_Initialization_Sequence(&hsdram, &command);
}


uint8_t checkResetForFirmwareUpdate(void)
{
	uint32_t backupRegisterContent;

	RTC_HandleTypeDef RtcHandle;
	RtcHandle.Instance = RTC;
	backupRegisterContent = HAL_RTCEx_BKUPRead(&RtcHandle,RTC_BKP_DR0);

	if(backupRegisterContent == 0x12345678)
		return 1;
	else
		return 0;
}

void DeleteResetToFirmwareUpdateRegister(void)
{
	RTC_HandleTypeDef RtcHandle;
	RtcHandle.Instance = RTC;
	__HAL_RTC_WRITEPROTECTION_DISABLE(&RtcHandle);
	HAL_RTCEx_BKUPWrite(&RtcHandle,RTC_BKP_DR0,0x00);
	__HAL_RTC_WRITEPROTECTION_ENABLE(&RtcHandle);
}

#ifdef  USE_FULL_ASSERT

/**
	* @brief  Reports the name of the source file and the source line number
	*         where the assert_param error has occurred.
	* @param  file: pointer to the source file name
	* @param  line: assert_param error line source number
	* @retval None
	*/
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
		 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif

/*
static void DualBootToBootloader(void)
{
		// Set BFB2 bit to enable boot from Flash Bank2
		// Allow Access to Flash control registers and user Falsh
		HAL_FLASH_Unlock();

		// Allow Access to option bytes sector
		HAL_FLASH_OB_Unlock();

		// Get the Dual boot configuration status
		AdvOBInit.OptionType = OPTIONBYTE_BOOTCONFIG;
		HAL_FLASHEx_AdvOBGetConfig(&AdvOBInit);

		// Enable/Disable dual boot feature
		if (((AdvOBInit.BootConfig) & (FLASH_OPTCR_BFB2)) == FLASH_OPTCR_BFB2)
		{
			AdvOBInit.BootConfig = OB_DUAL_BOOT_DISABLE;
			HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
			if (HAL_FLASH_OB_Launch() != HAL_OK)
			{
				while (1)
				{
				}
			}
		}
		else
		{

			AdvOBInit.BootConfig = OB_DUAL_BOOT_ENABLE;
			HAL_FLASHEx_AdvOBProgram (&AdvOBInit);
			if (HAL_FLASH_OB_Launch() != HAL_OK)
			{
				while (1)
				{
				}
			}
		}

		// Prevent Access to option bytes sector
		HAL_FLASH_OB_Lock();

		/ Disable the Flash option control register access (recommended to protect
		// the option Bytes against possible unwanted operations)
		HAL_FLASH_Lock();

		// Initiates a system reset request to reset the MCU
		reset_to_firmware_using_Watchdog();
}
*/

void reset_to_update_using_system_reset(void)
{
	__HAL_RCC_CLEAR_RESET_FLAGS();
	HAL_NVIC_SystemReset();
}

void reset_to_firmware_using_Watchdog(void)
{
	__HAL_RCC_CLEAR_RESET_FLAGS();
	__HAL_RCC_WWDG_CLK_ENABLE();

	WWDG_HandleTypeDef   WwdgHandle;
	WwdgHandle.Instance = WWDG;

	WwdgHandle.Init.Prescaler = WWDG_PRESCALER_8;
	WwdgHandle.Init.Window    = 80;
	WwdgHandle.Init.Counter   = 127;

	HAL_WWDG_Init(&WwdgHandle);
/*	HAL_WWDG_Start(&WwdgHandle);	has been removed from HAL library starting_V120 */
	while(1);
}


void set_returnFromComm(void)
{
	returnFromCommCleanUpRequest = 1;
}

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
