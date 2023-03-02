/**
 ******************************************************************************
 * @copyright heinrichs weikamp
 * @file   		base.c including main()
 * @author 		heinrichs weikamp gmbh
 * @date    		15-Aug-2014
 * @version		V1.0.3
 * @since			21-Nov-2014
 * @brief			The beginning of it all. main() is part of this.
 *							 + Do the inits for hardware
 *							 + Do the inits for sub-systems like menu, dive screen etc.
 *							 + Start IRQs
 *							 + Start MainTasks not in IRQs
 * @bug
 * @warning
 @verbatim
 ==============================================================================
 ##### What about hardware without 8 MHz oscillator #####
 ==============================================================================
 [..] modify OTP Byte 1 at 0x1FFF7800 with ST-Link utility

 ==============================================================================
 ##### Where is the RTE Firmware version #####
 ==============================================================================
 [..] in baseCPU2.c <just here below :->

 ==============================================================================
 ##### What to do with the RTE Firmware version #####
 ==============================================================================
 [..] change the values RTErequiredHigh and RTErequiredLow in settings.c
 to start warning via the firmware if not updated

 ==============================================================================
 ##### What it does #####
 ==============================================================================
 [..] All realtime stuff and all what has to be done during sleep

 [..] RealTimeClock. The entire time and date handling (including divetime)
 [..] Hardware control for pressure sensor, compass, battery monitor
 [..] Calculations of tissue load, critical radius, otu, cns
 [..] Switching off the power of the main CPU after request from it.

 ==============================================================================
 ##### IRQs #####
 ==============================================================================
 [..] The IRQs are are only used for SystemTick and SPI TransferComplete after
 DMA data reception.

 [..] HAL_SPI_TxRxCpltCallback() restarts DMA and will call
 scheduleSpecial_Evaluate_DataSendToSlave() only if it is not blocked
 by I2C.
 If the evaluation is blocked it has to be tested and executed afterwards.
 I2C is executed _without_ the usage of interrupts.

 ==============================================================================
 ##### Main loop #####
 ==============================================================================
 [..] is a combination of the while loop below in main.c and code in scheduler.c
 It is similar to the DR5 code / logic - in contrast to the main CPU
 Switching the state is done via global.mode
 The loops in scheduler all run in the main execution thread without
 any job stacks (like it was in the DR5).

 ==============================================================================
 ##### Real Time Clock #####
 ==============================================================================
  The RTC is a separate part of hardware inside the CPU and is not affected
 by reset. Only power-on reset does change something.
 This is fine but the RTC is vital for the Sleep mode as Wakeuptimer.
 This is the only date/time system in the OSTC. The main CPU is passive.
 Data transfer is done with localtime_rtc_tr and localtime_rtc_dr
 in HAL_RTC format to the main CPU and as HAL_RTC structs the way back for
 setting the actual time and date.
 The RTC unit has 20 Byte of V_bat powered SRAM. It could be used
 for something useful in both CPUs.

 ==============================================================================
 ##### File system  #####
 ==============================================================================
 [..] some files are used for both CPUs, like decom.c/.h, data_central.h, ...


 ==============================================================================
 ##### Unique device ID register (96 bits)  #####
 ==============================================================================
 [..] some files are used for both CPUs, like decom.c/.h, data_central.h, ...


 ==============================================================================
 ##### I2C  #####
 ==============================================================================
 [..] used for pressure, compass, (accelerator) and battery gauge
 main cpu and pic (button) is spi


 ==============================================================================
 ##### Firmware Update Info #####
 ==============================================================================
 V0.85			160531	scheduleCheck_pressure_reached_dive_mode_level() changes
 160606	global.no_fly_time_minutes is at least 24h after the dive
 160613	ambient light fixed
 160720	compass calib to Flash  (8000 writes max. as erase has problems)
 160829	do not reset main CPU on power on!
 V0.91			161018	pressure_calculation_AN520_004_mod_MS5803_30BA__09_2015();
 V0.92+		161020	global.sensorError[MAX_SENSORS]
 fix missing init_pressure(); at powerUp of RTE
 added HAL_StatusTypeDef for many functions in pressure.c
 161024	no_fly_time_minutes Backup FIX
 seconds_since_last_dive now related to RTC clock
 161121	close to surface starts at 1 meter below last known surface pressure
 161121	in surface mode dive mode starts @1 mtr difference if surface 880 hPa instead of 700 hPa before
 V0.97+		170213	added global.dataSendToSlave.diveModeInfo for DIVEMODE_Apnea
 added global.dataSendToSlave.setEndDive
 DIVEMODE_Apnea special in scheduler.c (ticksdiff >= 1000) -> no tissue, cns, otu, no change in noFly Time etc.
 V0.99			170320	new HAL Driver Repository
 V1.01			170509	old HAL Driver Repository

 @endverbatim
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 heinrichs weikamp</center></h2>
 *
 ******************************************************************************
 */

//#define DEBUG_PIN_ACTIVE
/* Includes ------------------------------------------------------------------*/

#include "baseCPU2.h"

// From Small_CPU/Inc:
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "rtc.h"
#include "adc.h"
#include "compass.h"
#include "pressure.h"
#include "batteryGasGauge.h"
#include "batteryCharger.h"
#include "scheduler.h"
#include "tm_stm32f4_otp.h"
#include "externalInterface.h"
#include "uart.h"

// From Common/Inc:
#include "calc_crush.h"
#include "decom.h"
#include "FirmwareData.h"

// From Common/Drivers/
#include "stm32f4xx_hal.h"
#include <stdio.h>

uint8_t coldstart __attribute__((section (".noinit")));

uint8_t hasExternalClock(void) {
	if ((TM_OTP_Read(0, 0) > 0) && (TM_OTP_Read(0, 0) < 0xFF))
		return 1;
	else
		return 0;
}

// SHALL LOAD AT 0x08000000 + 0x00005000 = 0x08005000.
// See CPU2-RTE.ld
const SFirmwareData cpu2_FirmwareData __attribute__(( section(".firmware_data") ))= {
		.versionFirst = 3,
		.versionSecond = 0,
		.versionThird = 0,
		.versionBeta = 0,

/* 4 bytes with trailing 0 */
		.signature = "mh",

		.release_year = 22,
		.release_month = 8,
		.release_day = 19,
		.release_sub = 0,

		/* max 48 with trailing 0 */
		//release_info ="12345678901234567890123456789012345678901"
		.release_info = "stable Nov'19",

		/* for safety reasons and coming functions */
		.magic[0] = FIRMWARE_MAGIC_FIRST, .magic[1] = FIRMWARE_MAGIC_SECOND,
		.magic[2] = FIRMWARE_MAGIC_CPU2_RTE, /* the magic byte for RTE */
		.magic[3] = FIRMWARE_MAGIC_END
};


uint8_t firmwareVersionHigh(void) {
	return cpu2_FirmwareData.versionFirst;
}

uint8_t firmwareVersionLow(void) {
	return cpu2_FirmwareData.versionSecond;
}

/** @addtogroup OSTC4
 * @{
 */

/** @addtogroup CPU2
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define BUTTON_OSTC_GPIO_PIN          GPIO_PIN_0
#define BUTTON_OSTC_GPIO_PORT        	GPIOA
#define BUTTON_OSTC_HAL_RCC_GPIO_CLK_ENABLE()					__HAL_RCC_GPIOA_CLK_ENABLE()
#define BUTTON_OSTC_IRQn              EXTI0_IRQn

#define BUTTON_TEST_GPIO_PIN          GPIO_PIN_3
#define BUTTON_TEST_GPIO_PORT        	GPIOA
#define BUTTON_TEST_GPIO_CLK_ENABLE()	__GPIOA_CLK_ENABLE()
#define BUTTON_TEST_IRQn              EXTI3_IRQn

#define WIRELSS_RISING_GPIO_PIN       GPIO_PIN_1
#define WIRELSS_RISING_GPIO_PORT      GPIOA
#define WIRELSS_RISING_HAL_RCC_GPIO_CLK_ENABLE()		 __HAL_RCC_GPIOA_CLK_ENABLE()
#define WIRELSS_RISING_IRQn						EXTI1_IRQn

#define WIRELSS_FALLING_GPIO_PIN      GPIO_PIN_2
#define WIRELSS_FALLING_GPIO_PORT     GPIOA
#define WIRELSS_FALLING_HAL_RCC_GPIO_CLK_ENABLE()		 __HAL_RCC_GPIOA_CLK_ENABLE()
#define WIRELSS_FALLING_IRQn					EXTI2_IRQn

#define WIRELSS_POWER_GPIO_PIN      	GPIO_PIN_12
#define WIRELSS_POWER_GPIO_PORT     	GPIOB
#define WIRELSS_POWER_HAL_RCC_GPIO_CLK_ENABLE()		 __HAL_RCC_GPIOB_CLK_ENABLE()


#define LED_CONTROL_PIN          		GPIO_PIN_3		/* PortC */
#define MAINCPU_CONTROL_PIN				GPIO_PIN_0		/* PortC */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t global_test_time_counter = 0;
SBackup backup;

/* Private function prototypes -----------------------------------------------*/
static void EXTI_Wakeup_Button_Init(void);
static void EXTI_Wakeup_Button_DeInit(void);

static void EXTI_Test_Button_Init(void);
static void EXTI_Test_Button_DeInit(void);

static void GPIO_LED_Init(void);
static void GPIO_Power_MainCPU_Init(void);
static void GPIO_Power_MainCPU_ON(void);
static void GPIO_Power_MainCPU_OFF(void);

#ifdef DEBUG_I2C_LINES
void GPIO_test_I2C_lines(void);
#endif

void sleep_prepare(void);

void SystemClock_Config(void);
void SystemClock_Config_HSI(void);
void SystemClock_Config_HSE(void);
void SYSCLKConfig_STOP_HSI(void);
void SYSCLKConfig_STOP_HSE(void);

void GPIO_new_DEBUG_Init(void);
void GPIO_new_DEBUG_LOW(void);
void GPIO_new_DEBUG_HIGH(void);

#define REGULAR_RUN

int __io_putchar(int ch) {
	ITM_SendChar(ch);
	return ch;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
/* #define DEBUG_RUNTIME TRUE */
#ifdef DEBUG_RUNTIME
#define MEASURECNT 60	/* number of measuremets to be stored */
static uint32_t loopcnt[MEASURECNT];
extern RTC_HandleTypeDef RTCHandle;
#endif

int main(void) {

#ifdef DEBUG_RUNTIME
    RTC_TimeTypeDef Stime;
    uint8_t measurementindex = 0;
    uint8_t lastsecond = 0xFF;
#endif

	HAL_Init();
	SystemClock_Config();

	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
	HAL_SYSTICK_CLKSourceConfig( SYSTICK_CLKSOURCE_HCLK);
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

	MX_RTC_init();
	GPIO_LED_Init();
	GPIO_new_DEBUG_Init(); // added 170322 hw
	initGlobals();

/*	printf("CPU2-RTE running...\n"); */

	HAL_Delay(100);

	MX_I2C1_Init();
	if (global.I2C_SystemStatus != HAL_OK)
	{
		if (MX_I2C1_TestAndClear() == GPIO_PIN_RESET) {
			MX_I2C1_TestAndClear(); // do it a second time
		}
		HAL_Delay(100);
		I2C_DeInit();
		HAL_Delay(100);
		MX_I2C1_Init();
		HAL_Delay(100);
	}



	//dangerous:	TM_OTP_Write(0,0, 0x01);
#ifdef REGULAR_RUN
	global.sensorError[SENSOR_PRESSURE_ID] = init_pressure();
	global.I2C_SystemStatus = global.sensorError[SENSOR_PRESSURE_ID];
	if (global.I2C_SystemStatus != HAL_OK)
	{
		if (MX_I2C1_TestAndClear() == GPIO_PIN_RESET) {
			MX_I2C1_TestAndClear(); // do it a second time
		}
		HAL_Delay(100);
		I2C_DeInit();
		HAL_Delay(100);
		MX_I2C1_Init();
		HAL_Delay(100);
		global.sensorError[SENSOR_PRESSURE_ID] = init_pressure();
		global.I2C_SystemStatus = global.sensorError[SENSOR_PRESSURE_ID];
	}

	global.dataSendToMaster.sensorErrors =
			global.sensorError[SENSOR_PRESSURE_ID];

	if(is_init_pressure_done())
	{
		init_surface_ring(0);
	}
	init_battery_gas_gauge();
	HAL_Delay(10);
	battery_gas_gauge_get_data();

	global.lifeData.battery_voltage = get_voltage();
	global.lifeData.battery_charge = get_charge();
	copyBatteryData();

	MX_SPI3_Init();

	if(coldstart != 0xA5)	/* Not reading a 0xA5 means the memory cells has not been initialized before => cold start */
	{
		coldstart = 0xA5;

		set_charge_state(Charger_ColdStart);

		global.dataSendToMaster.power_on_reset = 1;
		global.deviceDataSendToMaster.power_on_reset = 1;

		if (!scheduleSetButtonResponsiveness())
		{
			HAL_Delay(1);
			if (!scheduleSetButtonResponsiveness()) // send again, if problem it's not my problem here.
			{
				HAL_Delay(1);
				scheduleSetButtonResponsiveness(); // init
				HAL_Delay(1);
			}
		}
	}
	else
	{
		set_charge_state(Charger_NotConnected);
	}

	ADCx_Init();
	GPIO_Power_MainCPU_Init();

	externalInterface_InitPower33();

	global.mode = MODE_POWERUP;
#else
	init_pressure();
	init_surface_ring(0);

	ADCx_Init();
	GPIO_Power_MainCPU_Init();
	global.mode = MODE_TEST;
#endif
	while (1) {
/*		printf("Global mode = %d\n", global.mode); */

		switch (global.mode) {
		case MODE_POWERUP:
		case MODE_BOOT:
			//				ReInit_battery_charger_status_pins();
			compass_init(0, 7);
			accelerator_init();
			externalInterface_Init();

			if (global.mode == MODE_BOOT) {
				GPIO_Power_MainCPU_OFF();
				HAL_Delay(100); // for GPIO_Power_MainCPU_ON();
				GPIO_Power_MainCPU_ON();
			}
			SPI_synchronize_with_Master();
			MX_DMA_Init();
			MX_SPI1_Init();
			externalInterface_SwitchUART(externalInterface_GetUARTProtocol());
			SPI_Start_single_TxRx_with_Master(); /* be prepared for the first data exchange */
			Scheduler_Request_sync_with_SPI(SPI_SYNC_METHOD_HARD);
			EXTI_Test_Button_Init();
			global.mode = MODE_SURFACE;
			break;

		case MODE_CALIB:
            scheduleCompassCalibrationMode();
			break;

		case MODE_SURFACE:
			scheduleSurfaceMode();
			break;

		case MODE_TEST:
			break;

		case MODE_DIVE:
            backup.no_fly_time_minutes = global.no_fly_time_minutes;
            backup.seconds_since_last_dive = global.seconds_since_last_dive;

            vpm_init( &global.vpm, global.conservatism, global.repetitive_dive,
                      global.seconds_since_last_dive );
            global.no_fly_time_minutes = 0;
            global.lifeData.dive_time_seconds = 0;
            global.lifeData.dive_time_seconds_without_surface_time = 0;
            scheduleDiveMode();
            // done now in scheduler prior to change mode: global.seconds_since_last_dive = 1;

            if( global.lifeData.dive_time_seconds > 60 )
            {
                //No Fly time 60% of desaturationtime after dive
                global.no_fly_time_minutes = decom_calc_desaturation_time(
                    global.lifeData.tissue_nitrogen_bar,
                    global.lifeData.tissue_helium_bar,
                    global.lifeData.pressure_surface_bar ) * 60 / 100;
                if( global.no_fly_time_minutes < (24 * 60) )
                    global.no_fly_time_minutes = 24 * 60;
            }
            else
            {
                global.no_fly_time_minutes = backup.no_fly_time_minutes;
                global.seconds_since_last_dive = backup.seconds_since_last_dive;
            }

            global.lifeData.dive_time_seconds = 0;
            global.lifeData.dive_time_seconds_without_surface_time = 0;
            global.lifeData.counterSecondsShallowDepth = 0;

            backup.no_fly_time_minutes = 0;
            backup.seconds_since_last_dive = 0;
			break;

		case MODE_SHUTDOWN:
			HAL_Delay(200);
			global.mode = MODE_SLEEP;
			MX_SPI3_Init();
			break;

		case MODE_SLEEP:
			/*
			 sleep_prepare();
			 scheduleSleepMode_test();
			 */
			/*
			 GPIO_Power_MainCPU_OFF();
			 EXTI_Test_Button_DeInit();
			 EXTI_Wakeup_Button_Init();
			 NOT_USED_AT_THE_MOMENT_scheduleSleepMode();
			 */

			EXTI_Test_Button_DeInit();
			externalInterface_SwitchPower33(false);
			if (hasExternalClock())
				SystemClock_Config_HSI();
			sleep_prepare();

			GPIO_LED_Init();

			scheduleSleepMode();
			if (hasExternalClock())
				SystemClock_Config_HSE();
			GPIO_LED_Init();
			EXTI_Wakeup_Button_DeInit();
			ADCx_Init();
			GPIO_Power_MainCPU_Init();
			GPIO_Power_MainCPU_ON();
			compass_init(0, 7);
			accelerator_init();
			SPI_synchronize_with_Master();
			MX_DMA_Init();
			MX_SPI1_Init();
			SPI_Start_single_TxRx_with_Master();

			if(externalInterface_isEnabledPower33())
			{
				externalInterface_SwitchPower33(true);
			}
			externalInterface_InitDatastruct();
			// EXTILine0_Button_DeInit(); not now, later after testing
			break;
		}

#ifdef DEBUG_RUNTIME
		HAL_RTC_GetTime(&RTCHandle, &Stime, RTC_FORMAT_BCD);

        if(lastsecond == 0xFF)
        {
        	measurementindex = 0;
        	loopcnt[measurementindex] = 0;
        	lastsecond = Stime.Seconds;
        }
        loopcnt[measurementindex]++;

        if(lastsecond != Stime.Seconds)
        {
        	measurementindex++;
        	if (measurementindex == MEASURECNT) measurementindex = 0;
        	loopcnt[measurementindex] = 0;
        	lastsecond = Stime.Seconds;
        	if(measurementindex +1 < MEASURECNT) loopcnt[measurementindex +1] = 0xffff;	/* helps to identify the latest value */
        }
#endif
	}
}

/** @brief Button feedback - EXTI line detection callbacks
 * @param GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

	if (GPIO_Pin == BUTTON_OSTC_GPIO_PIN) {
		if (global.mode == MODE_SLEEP) {
			global.mode = MODE_BOOT;
		}
	}
	else
	if (GPIO_Pin == BUTTON_TEST_GPIO_PIN) {
		if (!global.demo_mode && (global.mode == MODE_SURFACE)) {
			global.demo_mode = 1;
			global.mode = MODE_DIVE;
		} else if (global.demo_mode && (global.mode == MODE_DIVE)
				&& (global.lifeData.dive_time_seconds > 10)) {
			global.demo_mode = 0;
			global.dataSendToMaster.mode = MODE_ENDDIVE;
			global.deviceDataSendToMaster.mode = MODE_ENDDIVE;
		}
	}
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSI)
 *            SYSCLK(Hz)                     = 100 MHz
 *            HCLK(Hz)                       = 100 MHz
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 2
 *            APB2 Prescaler                 = 1
 *            HSI Frequency(Hz)              = 16 MHz
 *            PLL_M                          = 16
 *            PLL_N                          = 400
 *            PLL_P                          = 4
 *            PLL_Q                          = 7 // no USB
 *            VDD(V)                         = 3.3
 *            Main regulator output voltage  = Scale1 mode
 *            Flash Latency(WS)              = 3
 * @param  None
 * @retval None
 */

void SystemClock_Config(void) {
	if (hasExternalClock())
		SystemClock_Config_HSE();
	else
		SystemClock_Config_HSI();
}

void SYSCLKConfig_STOP(void) {
	SYSCLKConfig_STOP_HSI();
}

void SystemClock_Config_HSE(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
//  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	__PWR_CLK_ENABLE(); // is identical to __HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE; //|RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	//RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 320;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

//  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
//  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
//  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

//  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

//  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
//  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void SystemClock_Config_HSI(void) {
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* The voltage scaling allows optimizing the power consumption when the device is
	 clocked below the maximum system frequency, to update the voltage scaling value
	 regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/* Enable HSI Oscillator and activate PLL with HSI as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 0x10;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 320;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
/*	
 RCC_OscInitTypeDef RCC_OscInitStruct;
 RCC_ClkInitTypeDef RCC_ClkInitStruct;
 
 __HAL_RCC_PWR_CLK_ENABLE();

 //__PWR_CLK_ENABLE();

 __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

 RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
 RCC_OscInitStruct.LSEState = RCC_LSE_ON;
 RCC_OscInitStruct.HSIState = RCC_HSI_ON;
 RCC_OscInitStruct.HSICalibrationValue = 16;
 RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
 RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
 RCC_OscInitStruct.PLL.PLLM = 16;
 RCC_OscInitStruct.PLL.PLLN = 320;
 RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
 RCC_OscInitStruct.PLL.PLLQ = 4;
 HAL_RCC_OscConfig(&RCC_OscInitStruct);

 RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
 RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
 RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
 RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
 RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
 HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
 }

 static void RtcClock_Config(void)
 {
 RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
 PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
 PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
 HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
 }
 */

/**
 * @brief  Configures system clock after wake-up from STOP: enable HSI, PLL
 *         and select PLL as system clock source.
 * @param  None
 * @retval None
 */
void SYSCLKConfig_STOP_HSE(void) {
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	uint32_t pFLatency = 0;

	/* Get the Oscillators configuration according to the internal RCC registers */
	HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

	/* After wake-up from STOP reconfigure the system clock: Enable HSI and PLL */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSIState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	/* Get the Clocks configuration according to the internal RCC registers */
	HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &pFLatency);

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, pFLatency);
}

void SYSCLKConfig_STOP_HSI(void) {
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	uint32_t pFLatency = 0;

	/* Get the Oscillators configuration according to the internal RCC registers */
	HAL_RCC_GetOscConfig(&RCC_OscInitStruct);

	/* After wake-up from STOP reconfigure the system clock: Enable HSI and PLL */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.HSICalibrationValue = 0x10;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	/* Get the Clocks configuration according to the internal RCC registers */
	HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &pFLatency);

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	 clocks dividers */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, pFLatency);
}

/**
 * @brief SYSTICK callback
 * @param None
 * @retval None
 */
void HAL_SYSTICK_Callback(void) {
	HAL_IncTick();
}

/**
 * @brief  Configures GPIO for LED
 *					Might move with STM32Cube usage
 * @param  None
 * @retval None
 */
/*
 void GPIO_test_I2C_lines(void)
 {
 GPIO_InitTypeDef   GPIO_InitStructure;
 __GPIOB_CLK_ENABLE();
 GPIO_InitStructure.Pin = GPIO_PIN_8;
 GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
 GPIO_InitStructure.Pull = GPIO_PULLUP;
 GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
 HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
 GPIO_InitStructure.Pin = GPIO_PIN_9;
 HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET);
 HAL_Delay(10);
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_SET);
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
 HAL_Delay(10);
 }
 */

static void GPIO_LED_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	__GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Pin = LED_CONTROL_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init( GPIOC, &GPIO_InitStructure);
}

void GPIO_new_DEBUG_Init(void) {
#ifdef DEBUG_PIN_ACTIVE
	GPIO_InitTypeDef GPIO_InitStructure;

	__GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Pin = LED_CONTROL_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif
}

void GPIO_new_DEBUG_LOW(void) {
#ifdef DEBUG_PIN_ACTIVE
	HAL_GPIO_WritePin(GPIOC,LED_CONTROL_PIN,GPIO_PIN_RESET);
#endif
}

void GPIO_new_DEBUG_HIGH(void) {
#ifdef DEBUG_PIN_ACTIVE
	HAL_GPIO_WritePin(GPIOC,LED_CONTROL_PIN,GPIO_PIN_SET);
#endif
}

static void GPIO_Power_MainCPU_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	__GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Pin = MAINCPU_CONTROL_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init( GPIOC, &GPIO_InitStructure);
	HAL_GPIO_WritePin( GPIOC, MAINCPU_CONTROL_PIN, GPIO_PIN_RESET);
}

static void GPIO_Power_MainCPU_ON(void) {
	HAL_GPIO_WritePin( GPIOC, MAINCPU_CONTROL_PIN, GPIO_PIN_RESET);
}

static void GPIO_Power_MainCPU_OFF(void) {
	HAL_GPIO_WritePin( GPIOC, MAINCPU_CONTROL_PIN, GPIO_PIN_SET);
}

/**
 * @brief  Configures EXTI Line0 (connected to PA0 + PA1 pin) in interrupt mode
 * @param  None
 * @retval None
 */

static void EXTI_Wakeup_Button_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	BUTTON_OSTC_HAL_RCC_GPIO_CLK_ENABLE();
	GPIO_InitStructure.Pin = BUTTON_OSTC_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init( BUTTON_OSTC_GPIO_PORT, &GPIO_InitStructure);

	HAL_NVIC_SetPriority( BUTTON_OSTC_IRQn, 0x0F, 0);
	HAL_NVIC_EnableIRQ( BUTTON_OSTC_IRQn);
}

static void EXTI_Wakeup_Button_DeInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	GPIO_InitStructure.Pull = GPIO_NOPULL;

	GPIO_InitStructure.Pin = BUTTON_OSTC_GPIO_PIN;
	HAL_GPIO_Init( BUTTON_OSTC_GPIO_PORT, &GPIO_InitStructure);
	HAL_NVIC_DisableIRQ( BUTTON_OSTC_IRQn);
}

static void EXTI_Test_Button_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	BUTTON_TEST_GPIO_CLK_ENABLE();
	GPIO_InitStructure.Pin = BUTTON_TEST_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init( BUTTON_TEST_GPIO_PORT, &GPIO_InitStructure);
	HAL_NVIC_SetPriority( BUTTON_TEST_IRQn, 0x0F, 0);
	HAL_NVIC_EnableIRQ( BUTTON_TEST_IRQn);
}

static void EXTI_Test_Button_DeInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	GPIO_InitStructure.Pull = GPIO_NOPULL;

	GPIO_InitStructure.Pin = BUTTON_TEST_GPIO_PIN;
	HAL_GPIO_Init( BUTTON_TEST_GPIO_PORT, &GPIO_InitStructure);
	HAL_NVIC_DisableIRQ( BUTTON_TEST_IRQn);
}

/* NUCLEO C 13
 KEY_BUTTON_GPIO_CLK_ENABLE();
 GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
 GPIO_InitStructure.Pull = GPIO_NOPULL;
 GPIO_InitStructure.Pin = KEY_BUTTON_PIN;
 HAL_GPIO_Init(KEY_BUTTON_GPIO_PORT, &GPIO_InitStructure);
 HAL_NVIC_SetPriority(KEY_BUTTON_EXTI_IRQn, 2, 0);
 HAL_NVIC_EnableIRQ(KEY_BUTTON_EXTI_IRQn);
 */

/**
 * @brief  Wake Up Timer callback
 * @param  hrtc: RTC handle
 * @retval None
 */

/*
 void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
 {
 static uint8_t uwCounter = 0;
 uwCounter = 1;
 }
 */

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {

}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {

}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle) {

}

void sleep_prepare(void) {
	EXTI_Wakeup_Button_Init();

	compass_sleep();
	HAL_Delay(100);
	accelerator_sleep();
	HAL_Delay(100);

	I2C_DeInit();
	MX_SPI_DeInit();
	MX_SPI3_DeInit();
	ADCx_DeInit();

	GPIO_InitTypeDef GPIO_InitStruct;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init( GPIOH, &GPIO_InitStruct);
#ifdef DEBUGMODE
	GPIO_InitStruct.Pin = GPIO_PIN_All ^ ( GPIO_PIN_3 | GPIO_PIN_8 | GPIO_PIN_9); /* debug */
#endif
	HAL_GPIO_Init( GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin =
			GPIO_PIN_All
					^ ( MAINCPU_CONTROL_PIN | CHARGE_OUT_PIN | CHARGE_IN_PIN | EXT33V_CONTROL_PIN | LED_CONTROL_PIN); /* power off & charger in & charge out & OSC32 & ext33Volt */

	HAL_GPIO_Init( GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_All ^ ( GPIO_PIN_0);
#ifdef DEBUGMODE
	GPIO_InitStruct.Pin = GPIO_PIN_All ^ ( GPIO_PIN_0 | GPIO_PIN_13 | GPIO_PIN_14); /* wake up button & debug */
#endif
	HAL_GPIO_Init( GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_All;
	HAL_GPIO_Init( GPIOH, &GPIO_InitStruct);

	GPIO_Power_MainCPU_OFF();

#ifndef DEBUGMODE
	__HAL_RCC_GPIOB_CLK_DISABLE();
#endif
	__HAL_RCC_GPIOH_CLK_DISABLE();

	HAL_Delay(1000);
}

/*
 void sleep_test(void)
 {
 GPIO_InitTypeDef GPIO_InitStruct;

 __HAL_RCC_GPIOA_CLK_ENABLE();
 __HAL_RCC_GPIOB_CLK_ENABLE();
 __HAL_RCC_GPIOC_CLK_ENABLE();
 __HAL_RCC_GPIOH_CLK_ENABLE();

 GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
 GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
 GPIO_InitStruct.Pull = GPIO_NOPULL;
 GPIO_InitStruct.Pin = GPIO_PIN_All;
 HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
 HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

 GPIO_InitStruct.Pin =  GPIO_PIN_All ^ ( GPIO_PIN_0 | GPIO_PIN_15 | GPIO_PIN_14);
 HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

 GPIO_InitStruct.Pin =  GPIO_PIN_All ^ ( GPIO_PIN_0);
 HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

 GPIO_Power_MainCPU_OFF();

 GPIO_InitStruct.Pull = GPIO_PULLUP;
 GPIO_InitStruct.Pin =  GPIO_PIN_0;
 HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

 //  __HAL_RCC_GPIOA_CLK_DISABLE();
 __HAL_RCC_GPIOB_CLK_DISABLE();
 //		__HAL_RCC_GPIOC_CLK_DISABLE();
 __HAL_RCC_GPIOH_CLK_DISABLE();


 HAL_Delay(5000);
 while(1)
 {
 RTC_StopMode_2seconds();
 HAL_Delay(200);
 }
 }
 */

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

/**
 * @}
 */
/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */

/*TxRx only here. Every 100 ms.*/
uint8_t ticks100ms=0;
void SysTick_Handler(void)
{
  HAL_IncTick();
  if(ticks100ms<100){
	  ticks100ms++;
  }else
  {
	  ticks100ms=0;
  }
}
/**
 * @}
 */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
