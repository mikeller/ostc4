///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/base.c
/// \brief  main(): init hardware, IRQs and start sub-systems
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
@verbatim
==============================================================================
                        ##### Firmware Info #####
==============================================================================
[..] In settings.c including text and magic stuff
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
[..] For everthing slow without importance to be 'in time'.
         Like VPM and Buehlmann.
         No sprintf and probably no GFX_SetFramesTopBottom() stuff neither.
         If sprintf is called while sprintf is executed it blows up everything.

==============================================================================
                        ##### Frames / the external SDRAM #####
==============================================================================
[..] The SDRAM is handled by getFrame() and releaseFrame().
         Each frame with 800*480*2 Bytes.
         Be carefull to release every frame
         otherwise there will be a memory leakage over time.
         housekeepingFrame() in the MainTask takes care of cleaning the frames.
         All frames are filled with 0x00. This will be transparent with color of
         CLUT_Font020 (is CLUT 0) if the alpha is set for a 16bit pair.
         housekeepingFrame() delays the cleaning of frames still used as screen
         buffer to prevent flickering.

[..] use global variable frameCounter[] in gfxengine.c to control memory
         all but the last three are identical to caller_id
         for example 0x05 are the menu frames
         the last but one is a sum for higher numbers (shouldn't be any)
         the last but one are those in status RELEASED
         the last are those CLEAR (as of 151202 down to 4 in logbook mode)

[..] 4 pages are used for two double memories for screenshots (since Nov. 15)

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
                                                        This prevents erratic behaviour if several changes
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
                                    Therefore there are differnent functions
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
                        ##### settings #####
==============================================================================
[..] files: settings.c, settings.h
            1. adjust struct SSettings in settings.h
            2. adjust const SSettings SettingsStandard in settings.c
            3. adjust set_new_settings_missing_in_ext_flash()
            4. adjust check_and_correct_settings()  IMPORTANT as it changes values!

==============================================================================
                        ##### specials #####
==============================================================================
[..] There was code for vector graphics from great demos
            (peridiummmm and jupiter) that can be fitted again

==============================================================================
                        ##### ppO2 sensors #####
==============================================================================
[..] in tCCR.c is function get_ppO2SensorWeightedResult_cbar();

 @endverbatim
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include <string.h> // for memcopy
#include <math.h>
#include "configuration.h"

#include "stm32f4xx_hal.h"
#include "ostc.h"
#include "base.h"
#include "display.h"
#include "gfx_engine.h"
#include "show_logbook.h"
#include "text_multilanguage.h"
#include "tHome.h"
#include "tInfo.h"
#include "tInfoLog.h"
#include "tInfoSensor.h"
#include "tMenu.h"
#include "tMenuEdit.h"
#include "tMenuEditGasOC.h"
#include "tStructure.h"
#include "externLogbookFlash.h"
#include "tComm.h"
#include "tCCR.h"
#include "data_exchange.h"
#include "data_exchange_main.h"
#include "vpm.h"
#include "buehlmann.h"
#include "logbook.h"
#include "check_warning.h"
#include "simulation.h"
#include "decom.h"
#include "timer.h"
#include "logbook_miniLive.h"
#include "test_vpm.h"
#include "tDebug.h"
#include "motion.h"
#include "t7.h"
#include "t3.h"
#include "tMenuEditSetpoint.h"

#ifdef DEMOMODE
#include "demo.h"
static void TIM_DEMO_init(void);
#endif


//#include "lodepng.h"
//#include <stdlib.h> // for malloc and free

/** @addtogroup OSTC 4
    * @{
    */

/* Private typedef -----------------------------------------------------------*/

//#define NO_TIMEOUT
//#define	QUICK_SLEEP

/* Private define ------------------------------------------------------------*/
#define REFRESH_COUNT       ((uint32_t)0x0569)   /**< for SDRAM refresh counter (90Mhz SD clock) */
#define INVALID_BUTTON ((uint8_t) 0xFF)
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static RTC_HandleTypeDef		RtcHandle; /* used to change time and date, no RTC is running on this MCU */
TIM_HandleTypeDef   TimHandle; /* used in stm32f4xx_it.c too */
static TIM_HandleTypeDef   TimBacklightHandle;
#ifdef DEMOMODE
TIM_HandleTypeDef   TimDemoHandle; /* used in stm32f4xx_it.c too */
#endif


static uint8_t blBoost = 0;
static uint8_t RequestModeChange = 0;

static uint8_t LastButtonPressed = INVALID_BUTTON;
static uint32_t LastButtonPressedTick = 0;
static uint32_t BaseTick100ms;			/* Tick at last 100ms cycle */

/* SDRAM handler declaration */
static SDRAM_HandleTypeDef hsdram;
static FMC_SDRAM_TimingTypeDef SDRAM_Timing;
static FMC_SDRAM_CommandTypeDef command;

/* This was used for Dual Boot */
//FLASH_OBProgramInitTypeDef    OBInit;
//FLASH_AdvOBProgramInitTypeDef AdvOBInit;

/* Private variables with external access ------------------------------------*/

static uint32_t globalStateID = 0;
static uint32_t time_without_button_pressed_deciseconds = 0; /**< langbeschreibung (eigenes Feld) warum diese variable verwendet wird um den sleepmode zu aktivieren */
uint8_t bootToBootloader = 0;	///< set  in tComm.c to install firmware updates, calls resetToFirmwareUpdate()
static uint8_t returnFromCommCleanUpRequest = 0; ///< use this to exit bluetooth mode and call tComm_exit()
uint32_t base_tempLightLevel = 0;
static uint8_t	wasFirmwareUpdateCheckBattery = 0;
static uint8_t DoDisplayRefresh = 0;			/* trigger to refresh display data */
static uint8_t DoHousekeeping = 0;				/* trigger to cleanup the frame buffers */
static SButtonLock ButtonLockState = LOCK_OFF;  /* Used for button unlock sequence */


/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command);
static void SDRAM_Config(void);
static void EXTILine_Buttons_Config(void);
static void TIM_init(void);
static void TIM_BACKLIGHT_init(void);
static uint32_t TIM_BACKLIGHT_adjust(void);
static void gotoSleep(void);
static void deco_loop(void);
static void resetToFirmwareUpdate(void);
static void TriggerButtonAction(void);
static void TriggerButtonUnlock(void);
static void EvaluateButton(void);
static void RefreshDisplay(void);
static void TimeoutControlRequestModechange(void);
static void TimeoutControl(void);

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
static uint8_t ButtonAction = ACTION_END;

void StoreButtonAction(uint8_t action)
{
	ButtonAction = action;
}

//  ===============================================================================
//	main
/// @brief	This function makes initializations and has the nonIRQ endless loop
///					for bluetooth and deco calculations
///
//  ===============================================================================
int main(void)
{
    uint32_t pLayerInvisible;
    uint16_t totalDiveCounterFound;

	SStateList status;

    set_globalState( StBoot0 );
    LastButtonPressed = 0;

    HAL_Init();
    HAL_NVIC_SetPriorityGrouping( NVIC_PRIORITYGROUP_2 );

    SystemClock_Config();

    MX_GPIO_Init();
    //  MX_SmallCPU_NO_Reset_Helper();	 //161116 hw
    MX_Bluetooth_PowerOff();	/* disable module, needed in case of e.g. a reset event to make sure module is configured from scratch */
    MX_SPI_Init();
    MX_UART_Init();
    SDRAM_Config();
    HAL_Delay( 100 );

    stateRealGetPointerWrite()->lastKnownBatteryPercentage = 0; // damit das nicht in settings kopiert wird.
    set_settings_to_Standard();
    mod_settings_for_first_start_with_empty_ext_flash();
    ext_flash_read_settings();
    if( newFirmwareVersionCheckViaSettings() ) // test for old firmware version in loaded settings
    {
        wasFirmwareUpdateCheckBattery = 1;
        set_settings_button_to_factory_with_individual_buttonBalance(); // will adapt individual values
    }
    //settingsGetPointer()->bluetoothActive = 0; 	/* MX_Bluetooth_PowerOff();  unnecessary as part of MX_GPIO_Init() */
    //settingsGetPointer()->compassBearing = 0;
    set_new_settings_missing_in_ext_flash(); // includes update of firmware version  161121

    GFX_init( &pLayerInvisible );
    TIM_BACKLIGHT_init();

    // new 170508: bluetooth on at start
    settingsGetPointer()->bluetoothActive = 1;
    MX_Bluetooth_PowerOn();
    tComm_StartBlueModConfig();

    /*
    if( (hardwareDataGetPointer()->primarySerial == 20+18)
     || (hardwareDataGetPointer()->primarySerial == 20+25)
     || (hardwareDataGetPointer()->primarySerial == 20+27))
    {
        MX_Bluetooth_PowerOn();
        tComm_Set_Bluetooth_Name(1);
    }
    */
    errorsInSettings = check_and_correct_settings();
    createDiveSettings();

#ifdef QUICK_SLEEP
    settingsGetPointer()->timeoutSurfacemode = 20;
#else
    settingsGetPointer()->timeoutSurfacemode = 120;
#endif

#ifdef DEMOMODE
    demoConfigureSettings();
    TIM_DEMO_init();
#endif

// -----------------------------

    display_power_on__1_of_2__pre_RGB();
    GFX_LTDC_Init();
    GFX_LTDC_LayerDefaultInit( TOP_LAYER, pLayerInvisible );
    GFX_LTDC_LayerDefaultInit( BACKGRD_LAYER, pLayerInvisible );
    GFX_SetFramesTopBottom( pLayerInvisible, pLayerInvisible, 480 );
    HAL_Delay( 20 );
    display_power_on__2_of_2__post_RGB();
    GFX_use_colorscheme( settingsGetPointer()->tX_colorscheme );

    tHome_init();
    tI_init();
    tM_init();
    tMenuEdit_init();
    tInfoLog_init();
    tComm_init();
    DataEX_init();
    settingsHelperButtonSens_keepPercentageValues(settingsGetPointer()->ButtonResponsiveness[3], settingsGetPointer()->ButtonResponsiveness);
    setButtonResponsiveness( settingsGetPointer()->ButtonResponsiveness );
    set_globalState_tHome();

    GFX_start_VSYNC_IRQ();
    tCCR_init();

    GFX_logoAutoOff();
    EXTILine_Buttons_Config();

#ifdef TRUST_LOG_CONSISTENCY
    if(!ext_dive_log_consistent())	/* only repair log if an invalid entry was detected */
    {
    	ext_flash_repair_dive_log();
    }

#else	/* always check and repair log */
    ext_flash_repair_dive_log();
    //ext_flash_repair_SPECIAL_dive_numbers_starting_count_with(1);
#endif
    totalDiveCounterFound = logbook_lastDive_diveNumber();
    if( settingsGetPointer()->totalDiveCounter < totalDiveCounterFound )
            settingsGetPointer()->totalDiveCounter = totalDiveCounterFound;

    if( settingsGetPointer()->debugModeOnStart )
    {
        settingsGetPointer()->debugModeOnStart = 0;
        ext_flash_write_settings(0);
        setDebugMode();
        openInfo( StIDEBUG );
    }
    InitMotionDetection();

    TIM_init();		/* start cylic 100ms task */

    if( settingsGetPointer()->buttonLockActive )
    {
    	ButtonLockState = LOCK_FIRST_PRESS;
    }

    /* @brief main LOOP
     *
     * this is executed while no IRQ interrupts it
     * - deco calculation
     * - bluetooth
     * and resetToFirmwareUpdate()
     * because tComm_control() does not exit before disconnection
     */
    while( 1 )
    {
        if( bootToBootloader )
            resetToFirmwareUpdate();

        tCCR_control();
        if( tComm_control() )// will stop while loop if tComm Mode started until exit from UART
        {
            createDiveSettings();
            updateMenu();
            ext_flash_write_settings(0);
        }

        /* check if tasks depending on global state are pending */
        get_globalStateList(&status);
        if(status.base == BaseHome)
        {
            tMenuEdit_writeSettingsToFlash();
        }

        DataEX_merge_devicedata(); 	/* data is exchanged at startup and every 10 minutes => check if something changed */

        deco_loop();
        if((ButtonLockState) && (stateUsed->mode == MODE_SURFACE))
        {
        	TriggerButtonUnlock();
        }
        else
        {
        	TriggerButtonAction();
        }
        if(DoHousekeeping)
        {
           	DoHousekeeping = housekeepingFrame();
        }
        if(DoDisplayRefresh)							/* set every 100ms by timer interrupt */
        {
	        DoDisplayRefresh = 0;

	        updateSetpointStateUsed();
            if(stateUsed == stateSimGetPointer())
            {
                simulation_UpdateLifeData(1);
            }
            check_warning();
            updateMiniLiveLogbook(1);

        	RefreshDisplay();
        	TimeoutControl();							/* exit menus if needed */

#ifdef ENABLE_MOTION_CONTROL
        	if((stateUsed->mode == MODE_DIVE) && (settingsGetPointer()->MotionDetection != MOTION_DETECT_OFF))		/* handle motion events in divemode only */
        	{
        		if(get_globalState() != StD)
        		{
        			suspendMotionDetection(10);		/* do not change custom views while divers is operating the computer */
        		}
        		HandleMotionDetection();
        	}
#endif

#ifdef SIM_WRITES_LOGBOOK
        if(stateUsed == stateSimGetPointer())
            logbook_InitAndWrite(stateUsed);
#endif
        	if(stateUsed == stateRealGetPointer())	/* Handle log entries while in dive mode*/
                logbook_InitAndWrite(stateUsed);
        }
    }
}




//  ===============================================================================
//	timer IRQ
/// @brief	this is called periodically
///
/// - refresh screen (the actual change is done in the VSYNC IRQ)
/// - start data transfer with RTE / small CPU (DateEX....)
/// - update logbook
/// - timeouts
/// ....
///
/// all this in three steps / switch() routines in a given order
/// as the previous switch() might influence the next functions
/// to be called
///
//  ===============================================================================

//#define NO_TIMEOUT
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
#ifdef DEMOMODE
    if(htim->Instance==TIM7)
    {
        HAL_GPIO_EXTI_Callback(demoGetCommand());
        return;
    }
#endif
    SStateList status;
    _Bool modeChange = 0;

    BaseTick100ms = HAL_GetTick();	/* store start of 100ms cycle */

    if(returnFromCommCleanUpRequest)
    {
        tComm_exit();
        returnFromCommCleanUpRequest = 0;
    }

    get_globalStateList(&status);

    switch(status.base)
    {
    case BaseHome:
    case BaseMenu:
    case BaseInfo:
        DateEx_copy_to_dataOut();
        DataEX_copy_to_LifeData(&modeChange);
//foto session :-)  stateRealGetPointerWrite()->lifeData.battery_charge = 99;
//foto session :-)  stateSimGetPointerWrite()->lifeData.battery_charge = 99;
        DataEX_copy_to_deco();
        DataEX_call();

        timer_UpdateSecond(1);
        base_tempLightLevel = TIM_BACKLIGHT_adjust();
        tCCR_tick();
        tHome_tick();
        break;
    case BaseStop:
        DateEx_copy_to_dataOut();
        DataEX_call();
        DataEX_control_connection_while_asking_for_sleep();
        break;
    default:
    case BaseComm:
        if(get_globalState() == StUART_RTECONNECT)
        {
            DateEx_copy_to_dataOut();
            DataEX_call();
            DataEX_copy_to_LifeData(0);
        }
        break;
    }

    get_globalStateList(&status);
    if(modeChange)
    {
    	TimeoutControlRequestModechange();
    }
    if(status.base == BaseComm) /* main loop not serviced in com mode */
    {
    	tComm_refresh();
    }
    else
    {
    	DoDisplayRefresh = 1;
    }
}


/* button and VSYNC IRQ
 *
 * VSYNC will switch foreground and background picture
 * if demanded. see GFX_change_LTDC()
 *
 */
//  ===============================================================================
//	HAL_GPIO_EXTI_Callback
/// @brief	button and VSYNC IRQ
///
/// VSYNC will switch foreground and background picture if demanded -
/// see GFX_change_LTDC()
///
//  ===============================================================================
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (!GPIO_Pin)
		return;

	if (GPIO_Pin == VSYNC_IRQ_PIN) // rechts, unten
	{
		GFX_change_LTDC();
		DoHousekeeping = 1;
		/*
		 #ifdef DEMOMODE
		 static uint8_t countCall = 0;
		 if(countCall++ < 10)
		 return;
		 countCall = 0;
		 uint8_t buttonAction = demoGetCommand();
		 if(buttonAction)
		 GPIO_Pin = buttonAction;
		 else
		 #endif
		 */

	}
	if((GPIO_Pin == BUTTON_ENTER_PIN) || (GPIO_Pin == BUTTON_NEXT_PIN) || (GPIO_Pin == BUTTON_BACK_PIN))
	{
		LastButtonPressed = GPIO_Pin;
		LastButtonPressedTick = HAL_GetTick();
	}

	EvaluateButton();

#ifdef DEMOMODE
	uint8_t demoMachineCall = 0;
	if(GPIO_Pin & 0x80)
	{
		demoMachineCall = 1;
		GPIO_Pin &= 0x7F;
	}
#endif
}

static void RefreshDisplay()
{
	SStateList status;
	get_globalStateList(&status);
	switch(status.base)
	{
	case BaseHome:
		tHome_refresh();
		tM_check_content();
		break;
	case BaseMenu:
		tM_refresh_live_content();
		tMenuEdit_refresh_live_content();
		break;
	case BaseInfo:
		tInfo_refresh();
		break;
	case BaseComm: 		/* refresh already done in time callback */
		break;
	default:
		if(get_globalState() == StStop)
			tHome_sleepmode_fun();
		break;
	}
}

static void TriggerButtonUnlock()
{
	static uint32_t lastInput = 0;
	uint8_t action = ButtonAction;

	if(action != ACTION_END)
	{
		if(((time_elapsed_ms(lastInput, HAL_GetTick()) < 2000)) || (ButtonLockState == LOCK_FIRST_PRESS))
		{
			switch(ButtonLockState)
			{
				case LOCK_FIRST_PRESS:
				case LOCK_1:	if(action == ACTION_BUTTON_ENTER)
								{
									ButtonLockState = LOCK_2;
								}
								else
								{
									ButtonLockState = LOCK_1;
								}
					break;
				case LOCK_2:	if(action == ACTION_BUTTON_NEXT)
								{
									ButtonLockState = LOCK_3;
								}
								else
								{
									ButtonLockState = LOCK_1;
								}
					break;
				case LOCK_3:	if(action == ACTION_BUTTON_BACK)
								{
									ButtonLockState = LOCK_OFF;
								}
								else
								{
									ButtonLockState = LOCK_1;
								}
					break;

				default:	ButtonLockState = LOCK_OFF;
					break;
			}
		}
		else
		{
			ButtonLockState = LOCK_1;
		}
		lastInput = LastButtonPressedTick;
		ButtonAction = ACTION_END;
	}
}

static void TriggerButtonAction()
{
	uint8_t action = ButtonAction;
	SStateList status;
	SSettings* pSettings;
	pSettings = settingsGetPointer();


	if(action != ACTION_END)
	{
		get_globalStateList(&status);

		if (action == ACTION_BUTTON_CUSTOM) {
			GFX_screenshot();
		}

		switch (status.base) {
		case BaseStop:
			if (action == ACTION_BUTTON_BACK)
				resetToFirmwareUpdate();
			break;
		case BaseComm:		/* already handled in tim callback */
			break;
		case BaseHome:
			if (action == ACTION_BUTTON_NEXT) {
				if (status.page == PageSurface)
					openMenu(1);
				else
					tHomeDiveMenuControl(action);
			} else if (action == ACTION_BUTTON_BACK) {
				if (get_globalState() == StS)
				{
					openInfo(StILOGLIST);
				}
				else if ((status.page == PageDive) && (pSettings->design < 7))
				{
					if(pSettings->design == 3)
					{
						if(get_globalState() != StD)		/* located in submenu? => return */
						{
							set_globalState(StD);
						}
						else								/* return to t7 view */
						{
							pSettings->design = 7;
							if(pSettings->MotionDetection == MOTION_DETECT_SECTOR)
							{
								DefineSectorCount(CUSTOMER_DEFINED_VIEWS);
								MapCVToSector();
							}
						}
					}
					else
					{
						pSettings->design = 7; // auto switch to 9 if necessary
					}
				} else if ((status.page == PageDive) && (status.line != 0))
				{
					if(get_globalState() == StDMENU)
					{
						if ((pSettings->extraDisplay == EXTRADISPLAY_BIGFONT) || (pSettings->extraDisplay == EXTRADISPLAY_BFACTIVE))
						{
							pSettings->design = 3;
							if(pSettings->MotionDetection == MOTION_DETECT_SECTOR)
							{
								DefineSectorCount(CUSTOMER_DEFINED_VIEWS);
								MapCVToSector();
							}
						}
#ifdef HAVEDECOGAME
						else if (pSettings->extraDisplay	== EXTRADISPLAY_DECOGAME)
							pSettings->design = 4;
#endif
					}
					set_globalState(StD);
				}
				else
				{
					tHome_change_field_button_pressed();
				}
			} else if ((action == ACTION_BUTTON_ENTER) || (action == ACTION_PITCH_NEG) || (action == ACTION_PITCH_POS))
					{

						if ((status.page == PageDive) && (status.line == 0))
						{
							if(action == ACTION_BUTTON_ENTER)
							{
								suspendMotionDetection(10);
							}
							tHome_change_customview_button_pressed(action);
						}
						else if (status.page == PageSurface)
							tHome_change_customview_button_pressed(action);
						else
						{
							tHomeDiveMenuControl(action);
						}
					}
			break;

		case BaseMenu:
			if (status.line == 0)
				sendActionToMenu(action);
			else
				sendActionToMenuEdit(action);
			break;

		case BaseInfo:	switch(status.page)
						{
							case InfoPageLogList: 	sendActionToInfoLogList(action);
								break;
							case InfoPageLogShow: 	sendActionToInfoLogShow(action);
								break;
							case InfoPageSensor: 	sendActionToInfoSensor(action);
								break;
							default:				sendActionToInfo(action);
								break;
						}
			break;

		default:
			break;
		}
		ButtonAction = ACTION_END;
	}
}


static void EvaluateButton()
{
	uint8_t action = 0;
	SStateList status;
	SSettings* pSettings;
	pSettings = settingsGetPointer();

	if (GFX_logoStatus() != 0)
		return;

	if ((LastButtonPressed != INVALID_BUTTON) && (time_elapsed_ms(LastButtonPressedTick, HAL_GetTick()) > 40))
	{
		if (LastButtonPressed == BUTTON_BACK_PIN) { // links
			if (HAL_GPIO_ReadPin(BUTTON_BACK_GPIO_PORT, BUTTON_BACK_PIN) == 1) {
				action = ACTION_BUTTON_BACK;
			}
		}

		else if (LastButtonPressed == BUTTON_ENTER_PIN) { // mitte
			if (HAL_GPIO_ReadPin(BUTTON_ENTER_GPIO_PORT, BUTTON_ENTER_PIN) == 1) {
				action = ACTION_BUTTON_ENTER;
			}
		}

		else if (LastButtonPressed == BUTTON_NEXT_PIN) { // rechts
			if (HAL_GPIO_ReadPin(BUTTON_NEXT_GPIO_PORT, BUTTON_NEXT_PIN) == 1) {
				action = ACTION_BUTTON_NEXT;
			}
		}

		if(action != 0)
		{
			time_without_button_pressed_deciseconds = 0;
			if(pSettings->FlipDisplay) /* switch action resulting from pressed button */
			{
				if (action == ACTION_BUTTON_BACK)
				{
					action = ACTION_BUTTON_NEXT;
				}
				else
				{
					if (action == ACTION_BUTTON_NEXT)
					{
						action = ACTION_BUTTON_BACK;
					}
				}
			}
		}

#ifdef BUTTON_CUSTOM_PIN
		else
		if(LastButtonPressed == BUTTON_CUSTOM_PIN) // extra
				action = ACTION_BUTTON_CUSTOM;
#endif

	#ifdef DEMOMODE // user pressed button ?
		if((!demoMachineCall) && demoModeActive())
		{
			demoSendCommand(action);
			return;
		}
	#endif

		get_globalStateList(&status);
		if(status.base == BaseComm) /* main loop is not serviced in comm mode => react immediately */
		{
			switch(action)
			{
				case ACTION_BUTTON_BACK: tComm_exit();
					break;
				case ACTION_BUTTON_NEXT: tComm_RequestBluetoothStrength();
					break;
				default:
					break;
			}
		}
		else
		{
			StoreButtonAction(action);	/* Handle action in main loop */
		}
		LastButtonPressed = INVALID_BUTTON;
	}
}

static void gotoSleep(void)
{
    /* not at the moment of testing */
//	ext_flash_erase_firmware_if_not_empty();
    GFX_logoAutoOff();
    ext_flash_write_devicedata(true);	/* write data at default position */
    ext_flash_write_settings(true);		/* write data at default position */
    set_globalState(StStop);
}


// -----------------------------

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

SButtonLock get_ButtonLock(void)
{
	return ButtonLockState;
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

void set_returnFromComm(void)
{
    returnFromCommCleanUpRequest = 1;
}

uint8_t font_update_required(void)
{
    uint8_t *fontVersionHigh;
    uint8_t *fontVersionLow;

    fontVersionHigh = (uint8_t *)0x08132000;
    fontVersionLow = (uint8_t *)0x08132001;

    if(FONTminimum_required_high() < *fontVersionHigh)
        return 0;

    if((FONTminimum_required_high() == *fontVersionHigh) && (FONTminimum_required_low() <= *fontVersionLow))
        return 0;

    return 1;
}


__attribute__((optimize("O0"))) void delayMicros(uint32_t micros)
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

#ifdef DEMOMODE
static void TIM_DEMO_init(void)
{
    uint16_t uwPrescalerValue = 0;

    uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 10000) - 1;

    /* Set TIMx instance */
    TimDemoHandle.Instance = TIM7;

    /* Initialize TIM3 peripheral as follows:
             + Period = 10000 - 1
             + Prescaler = ((SystemCoreClock/2)/10000) - 1
             + ClockDivision = 0
             + Counter direction = Up
    */
    TimDemoHandle.Init.Period = 1000 - 1;
    TimDemoHandle.Init.Prescaler = uwPrescalerValue;
    TimDemoHandle.Init.ClockDivision = 0;
    TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    if(HAL_TIM_Base_Init(&TimDemoHandle) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    /*##-2- Start the TIM Base generation in interrupt mode ####################*/
    /* Start Channel1 */
    if(HAL_TIM_Base_Start_IT(&TimDemoHandle) != HAL_OK)
    {
        /* Starting Error */
        Error_Handler();
    }
}
#endif



#ifndef TIM_BACKLIGHT

static uint32_t TIM_BACKLIGHT_adjust(void)
{
    return 0;
}

static void TIM_BACKLIGHT_init(void)
{
}
#else
static uint32_t TIM_BACKLIGHT_adjust(void)
{
    static uint32_t levelActual = 12000;
    static uint8_t brightnessModeLast = 0;
//	static _Bool wasLostConnection = 0;

    uint32_t levelAmbient;
    uint32_t levelMax;
    uint32_t levelMin;
    uint32_t levelUpStep_100ms	= 200;
    uint32_t levelDnStep_100ms	= 20;

    TIM_OC_InitTypeDef sConfig;
    sConfig.OCMode     = TIM_OCMODE_PWM1;
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;

    const SDiveState * pStateReal = stateRealGetPointer();


//	if(pStateReal->data_old__lost_connection_to_slave)
//	{
// changed 160613 from 6000 to 12000
// removed hw 161209
//		levelAmbient = 12000;
//		levelActual = 12000;
//		wasLostConnection = 1;
//	}
//	else
//	{
        SSettings *pSettings = settingsGetPointer();
        /* 300 - 4000 */
        /* important levelAmbient 300 - 1200 */
        levelAmbient = 10 * pStateReal->lifeData.ambient_light_level;

        if((pStateReal->chargeStatus == CHARGER_running) || (pStateReal->chargeStatus == CHARGER_lostConnection))
        {
        	levelMax = 1000;
        	levelMin = 500;
        }
        else
        {
        switch(	pSettings->brightness + blBoost)
        {
        case 0: /* Cave */
            levelMax = 3000;/* max 25 % (x2) */
            levelMin = 1500;
            break;
        case 1: /* Eco */
            levelMax = 6000;/* max 50 % (x2) */
            levelMin = 3000;
            break;
        case 2: /* Std */
            levelAmbient += 1000;
            levelMax = 9000;
            levelMin = 4500;
            levelUpStep_100ms += levelUpStep_100ms/2; // 4500 instead of 3000
            levelDnStep_100ms += levelDnStep_100ms/2;
            break;
        case 3: /* High */
        default:
            levelAmbient += 3000;
            levelMax = 12000; /* max 100% (x2) */
            levelMin = 6000;
            levelUpStep_100ms += levelUpStep_100ms; // 6000 instead of 3000
            levelDnStep_100ms += levelDnStep_100ms;
            break;
        case 4: /* New Max */
            levelAmbient = 12000;
            levelMax = 12000; /* max 100% (x2) */
            levelMin = 12000;
            levelUpStep_100ms += 12000;
            levelDnStep_100ms += 0;
            break;
        }

        if((pSettings->brightness != brightnessModeLast))// || wasLostConnection)
        {
            levelActual = levelAmbient;
            brightnessModeLast = pSettings->brightness;
//			wasLostConnection = 0;
        }
//	}
        }

    if(levelAmbient > levelActual)
        levelActual += levelUpStep_100ms;
    else
    if((levelAmbient < levelActual) && (levelActual > levelMin) && (levelActual > levelDnStep_100ms))
        levelActual -= levelDnStep_100ms;

    if(levelActual > levelMax)
        levelActual = levelMax;
    else
    if(levelActual < levelMin)
    {
        levelActual = levelMin;
    }
//	sConfig.Pulse = levelActual / 20;
    sConfig.Pulse = (levelMin + ((levelMax - levelMin)/2)) / 20; // added 170306

    /* xx - 600 */
    if(sConfig.Pulse > 600)
        sConfig.Pulse = 600;
    else
    if(sConfig.Pulse < 25)
        sConfig.Pulse = 25;

    HAL_TIM_PWM_ConfigChannel(&TimBacklightHandle, &sConfig, TIM_BACKLIGHT_CHANNEL);
    HAL_TIM_PWM_Start(&TimBacklightHandle, TIM_BACKLIGHT_CHANNEL);

    return levelActual;
}

static void TIM_BACKLIGHT_init(void)
{
    uint32_t uwPrescalerValue = 0;
    TIM_OC_InitTypeDef sConfig;

    uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 18000000) - 1;

    TimBacklightHandle.Instance = TIM_BACKLIGHT;

    /* Initialize TIM3 peripheral as follows:
        30 kHz
    */
    TimBacklightHandle.Init.Period = 600 - 1;
    TimBacklightHandle.Init.Prescaler = uwPrescalerValue;
    TimBacklightHandle.Init.ClockDivision = 0;
    TimBacklightHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&TimBacklightHandle);

    sConfig.OCMode     = TIM_OCMODE_PWM1;
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;
    sConfig.Pulse = 100; /* Initial brigthness of display */

    HAL_TIM_PWM_ConfigChannel(&TimBacklightHandle, &sConfig, TIM_BACKLIGHT_CHANNEL);
    HAL_TIM_PWM_Start(&TimBacklightHandle, TIM_BACKLIGHT_CHANNEL);
}
#endif


void set_Backlight_Boost(uint8_t level)
{
	if(level < 3)
	{
		blBoost = level;
	}
}

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
    * @brief  Perform the SDRAM exernal memory inialization sequence
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


/**
******************************************************************************
******************************************************************************
******************************************************************************
*/


/**
    * @brief DMA2D configuration.
    * @note  This function Configure tha DMA2D peripheral :
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


static void deco_loop(void)
{
    typedef enum
    {
        CALC_VPM,
        CALC_VPM_FUTURE,
        CALC_BUEHLMANN,
        CALC_BUEHLMANN_FUTURE,
		CALC_INVALID
    } CALC_WHAT;

    static CALC_WHAT what = CALC_INVALID;
    static int counter = 0;
    if((stateUsed->mode != MODE_DIVE) || (stateUsed->diveSettings.diveMode == DIVEMODE_Apnea) || (stateUsed->diveSettings.diveMode == DIVEMODE_Gauge) || (decoLock != DECO_CALC_ready ))
        return;

    if((stateUsed->warnings.betterSetpoint) && (settingsGetPointer()->autoSetpoint) && (settingsGetPointer()->CCR_Mode == CCRMODE_FixedSetpoint))
   	{
        openEdit_DiveSelectBetterSetpoint(false);
   	}

    decoLock = DECO_CALC_running;

    if(stateDeco.diveSettings.deco_type.ub.standard == GF_MODE)
    {
// hw 151110 mh wants future TTS even in deco zone     if((what == CALC_BUEHLMANN) && (stateDeco.lifeData.pressure_ambient_bar > stateDeco.diveSettings.internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero))
        if(what == CALC_BUEHLMANN)
        {
            //Calc future
            what = CALC_BUEHLMANN_FUTURE;
        }
        else
            what = CALC_BUEHLMANN;

    }
    else
    {
// hw 151110 mh wants future TTS even in deco zone           if((what == CALC_VPM) && (!stateDeco.vpm.deco_zone_reached))
        if(what == CALC_VPM)
        {
            //Calc future
            what = CALC_VPM_FUTURE;
        }
        else
            what = CALC_VPM;
    }

    //In one of ten calc the other option
    if(counter == 10)
    {
        if(what == CALC_VPM)
             what = CALC_BUEHLMANN;
        if(what == CALC_BUEHLMANN)
                what = CALC_VPM;
        counter = 0;
    }

    decom_CreateGasChangeList(&stateDeco.diveSettings, &stateDeco.lifeData);

    switch(what)
    {
		case CALC_VPM:
				vpm_calc(&stateDeco.lifeData,&stateDeco.diveSettings,&stateDeco.vpm,&stateDeco.decolistVPM, DECOSTOPS);
				decoLock = DECO_CALC_FINSHED_vpm;
				return;
		 case CALC_VPM_FUTURE:
				decom_tissues_exposure(stateDeco.diveSettings.future_TTS_minutes * 60,&stateDeco.lifeData);
				vpm_calc(&stateDeco.lifeData,&stateDeco.diveSettings,&stateDeco.vpm,&stateDeco.decolistFutureVPM, FUTURESTOPS);
				decoLock = DECO_CALC_FINSHED_Futurevpm;
				return;
		case CALC_BUEHLMANN:
				buehlmann_calc_deco(&stateDeco.lifeData,&stateDeco.diveSettings,&stateDeco.decolistBuehlmann);
				buehlmann_ceiling_calculator(&stateDeco.lifeData, &stateDeco.decolistBuehlmann);
				buehlmann_super_saturation_calculator(&stateDeco.lifeData,&stateDeco.decolistBuehlmann);
				decoLock = DECO_CALC_FINSHED_Buehlmann;
				return;
		 case CALC_BUEHLMANN_FUTURE:
				decom_tissues_exposure(stateDeco.diveSettings.future_TTS_minutes * 60,&stateDeco.lifeData);
				buehlmann_calc_deco(&stateDeco.lifeData,&stateDeco.diveSettings,&stateDeco.decolistFutureBuehlmann);
				decoLock = DECO_CALC_FINSHED_FutureBuehlmann;
				return;
		 default: break;
    }
    counter++;
}

static void resetToFirmwareUpdate(void)
{
    __HAL_RCC_CLEAR_RESET_FLAGS();
    HAL_NVIC_SystemReset();
}

static void TimeoutControlRequestModechange(void)
{
	RequestModeChange = 1;
}

static void TimeoutControl(void)
{
    static uint8_t last_base;

    SStateList status;
    uint32_t timeout_in_seconds;
    uint32_t timeout_limit_Surface_in_seconds;
    _Bool InDiveMode = 0;

    get_globalStateList(&status);

    if(stateUsed->mode == MODE_DIVE)
    {
        InDiveMode = 1;
    }
    else
    {
        InDiveMode = 0;
    }
	/* timeout control */
	if(RequestModeChange) ///< from RTE, set in data_exchange_main.c
		time_without_button_pressed_deciseconds = (settingsGetPointer()->timeoutSurfacemode / 4) * 3;
	if(status.base != last_base)
		time_without_button_pressed_deciseconds = 0;
	last_base = status.base;
	timeout_in_seconds = time_without_button_pressed_deciseconds / 10;
	time_without_button_pressed_deciseconds += 1;
	if(RequestModeChange || (timeout_in_seconds != time_without_button_pressed_deciseconds / 10))
	{
	#ifdef NO_TIMEOUT
		timeout_in_seconds = 0;
	#else
		timeout_in_seconds += 1;
	#endif

		if(InDiveMode)
		{
			switch(status.base)
			{
			case BaseHome:
				if((status.line != 0) && (timeout_in_seconds  >= settingsGetPointer()->timeoutEnterButtonSelectDive))
				{
					set_globalState(StD);
					timeout_in_seconds = 0;
				}
			break;

			case BaseMenu:
				if((status.line == 0) && ((timeout_in_seconds  >= settingsGetPointer()->timeoutMenuDive) || RequestModeChange))
				{
					exitMenu();
					timeout_in_seconds = 0;
				}
				if((status.line != 0) && ((timeout_in_seconds  >= settingsGetPointer()->timeoutMenuEdit) || RequestModeChange))
				{
					exitMenuEdit_to_Home();
					timeout_in_seconds = 0;
				}
			break;
			default:
				break;
			}
		}
		else /* surface mode */
		{
			switch(status.base)
			{
			case BaseHome:
				/* The RTE will mark a charge value as suspect after startup. Main know the update condition and may confirm that the value is most likely valid */
	//			if(!(stateRealGetPointer()->warnings.lowBattery) && ((stateRealGetPointer()->lifeData.battery_charge > 9) || (wasFirmwareUpdateCheckBattery)))
				{
					if(stateRealGetPointer()->lifeData.battery_charge < 0.0)
					{
						if(fabs(stateRealGetPointerWrite()->lastKnownBatteryPercentage - fabs(stateRealGetPointer()->lifeData.battery_charge)) < 1.0)
						{
							setBatteryPercentage(settingsGetPointer()->lastKnownBatteryPercentage);	/* confirm that value provided by RTE is valid (maybe reset happened) */
						}
					}
					else
					{
						if(!(stateRealGetPointer()->warnings.lowBattery) && (stateRealGetPointer()->lifeData.battery_charge > 9))
						{
							stateRealGetPointerWrite()->lastKnownBatteryPercentage = stateRealGetPointer()->lifeData.battery_charge;
						}
					}
				}
				if((wasFirmwareUpdateCheckBattery) && (timeout_in_seconds > 3))
				{
					wasFirmwareUpdateCheckBattery = 0;
					setButtonResponsiveness(settingsGetPointer()->ButtonResponsiveness); // added 170306
					if(	(settingsGetPointer()->lastKnownBatteryPercentage > 0)
					&& 	(settingsGetPointer()->lastKnownBatteryPercentage <= 101.0)
					&& 	(stateRealGetPointer()->warnings.lowBattery))
					{
						setBatteryPercentage(settingsGetPointer()->lastKnownBatteryPercentage);
					}
				}
				// stuff before and new @161121 CCR-sensor limit 10 minutes
				if(isLoopMode(settingsGetPointer()->dive_mode) && (settingsGetPointer()->CCR_Mode == CCRMODE_Sensors))
				{
					timeout_limit_Surface_in_seconds = settingsGetPointer()->timeoutSurfacemodeWithSensors;
				}
				else
				{
					timeout_limit_Surface_in_seconds = settingsGetPointer()->timeoutSurfacemode;
				}
				if(timeout_in_seconds  >= timeout_limit_Surface_in_seconds)
				{
					gotoSleep();
				}
				break;
			case BaseMenu:
				if((status.line == 0) && ((timeout_in_seconds  >= settingsGetPointer()->timeoutMenuSurface) || RequestModeChange))
				{
					exitMenu();
					timeout_in_seconds = 0;
				}
				if((status.line != 0) && ((timeout_in_seconds  >= settingsGetPointer()->timeoutMenuEdit) || RequestModeChange))
				{
					if((status.page != (uint8_t)((StMPLAN >> 24) & 0x0F)) || (timeout_in_seconds  >= 10*(settingsGetPointer()->timeoutMenuEdit)))
					{
						exitMenuEdit_to_Home();
						timeout_in_seconds = 0;
					}
				}
				break;

			case BaseInfo:
				if((timeout_in_seconds  >= settingsGetPointer()->timeoutInfo) || RequestModeChange)
				{
					timeout_in_seconds = 0;

					switch(status.page)
					{
						case InfoPageLogList:	exitLog();
							break;
						case InfoPageLogShow:	show_logbook_exit();
												exitLog();
							break;
						case InfoPageCompass:		/* compass has individual timeout */
							break;
						case InfoPageSensor:	exitInfoToBack();
							break;
						default:				exitInfo();
							break;
					}
				}
				break;
			default:
				break;
			}
		}
	}
	RequestModeChange = 0;
}
// debugging by https://blog.feabhas.com/2013/02/developing-a-generic-hard-fault-handler-for-arm-cortex-m3cortex-m4/

/*
void printErrorMsg(const char * errMsg)
{

//	printf(errMsg);
//	return;

     while(*errMsg != 0){
            ITM_SendChar(*errMsg);
            ++errMsg;
     }
}

enum { r0, r1, r2, r3, r12, lr, pc, psr};

void stackDump(uint32_t stack[])
{
     static char msg[80];
     sprintf(msg, "r0  = 0x%08x\n", stack[r0]);  printErrorMsg(msg);
     sprintf(msg, "r1  = 0x%08x\n", stack[r1]);  printErrorMsg(msg);
     sprintf(msg, "r2  = 0x%08x\n", stack[r2]);  printErrorMsg(msg);
     sprintf(msg, "r3  = 0x%08x\n", stack[r3]);  printErrorMsg(msg);
     sprintf(msg, "r12 = 0x%08x\n", stack[r12]); printErrorMsg(msg);
     sprintf(msg, "lr  = 0x%08x\n", stack[lr]);  printErrorMsg(msg);
     sprintf(msg, "pc  = 0x%08x\n", stack[pc]);  printErrorMsg(msg);
     sprintf(msg, "psr = 0x%08x\n", stack[psr]); printErrorMsg(msg);
}

void printUsageErrorMsg(uint32_t CFSRValue)
{
     printErrorMsg("Usage fault: ");
     CFSRValue >>= 16;                  // right shift to lsb
     if((CFSRValue & (1 << 9)) != 0) {
            printErrorMsg("Divide by zero\n");
     }
}

void Hard_Fault_Handler()//uint32_t stack[])
    {
     static char msg[80];
     printErrorMsg("In Hard Fault Handler\n");
     sprintf(msg, "SCB->HFSR = 0x%08x\n", SCB->HFSR);
     printErrorMsg(msg);
     if ((SCB->HFSR & (1 << 30)) != 0) {
             printErrorMsg("Forced Hard Fault\n");
             sprintf(msg, "SCB->CFSR = 0x%08x\n", SCB->CFSR );
             printErrorMsg(msg);
             if((SCB->CFSR & 0xFFFF0000) != 0) {
                 printUsageErrorMsg(SCB->CFSR);
            }
     }
     __ASM volatile("BKPT #01");
     while(1);
}

int my_store_of_MSP;

void HardFault_Handler(void)
{
    __asm ("MRS my_store_of_MSP, MSP");
    Hard_Fault_Handler();
}
*/

/*
__asm void HardFault_Handler(void)
{
    TST lr, #4     // Test for MSP or PSP
    ITE EQ
    MRSEQ r0, MSP
    MRSNE r0, PSP
    B __cpp(Hard_Fault_Handler)
}
*/
/*
HardFault_Handler\
    PROC
    EXPORT  HardFault_Handler
    B       .
    ENDP
*/

/*
__asm int f(int i)
{
        ADD i, i, #1 // error
}

EXPORT HardFault_Handler
HardFault_Handler FUNCTION
    MRS r0, MSP
    B __cpp(Hard_Fault_Handler)
ENDFUNC
*/
