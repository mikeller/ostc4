/**
  ******************************************************************************
  * @file    scheduler.c 
  * @author  heinrichs weikamp gmbh
  * @date    27-March-2014
  * @version V0.0.6
  * @since   18-June-2015
  * @brief   the main part except for base.c
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 
	
	
//#define DEBUGMODE

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "baseCPU2.h"
#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "scheduler.h"
#include "pressure.h"
#include "compass.h"
#include "batteryGasGauge.h"
#include "batteryCharger.h"
#include "spi.h"
#include "rtc.h"
#include "dma.h"
#include "adc.h"
#include "calc_crush.h"
#include "stm32f4xx_hal_rtc_ex.h"
#include "decom.h"
#include "tm_stm32f4_otp.h"
#include "externalInterface.h"
#include "uart.h"
#include "GNSS.h"
#include "uartProtocol_GNSS.h"
#include "math.h"
#include "configuration.h"

/* uncomment to enable restoting of last known date in case of a power loss (RTC looses timing data) */
/* #define RESTORE_LAST_KNOWN_DATE */

#define INVALID_PREASURE_VALUE 			(0.0f)
#define START_DIVE_MOUNTAIN_MODE_BAR	(0.88f)
#define START_DIVE_IMMEDIATLY_BAR		(1.16f)

/* Ascent rate calculation */
typedef enum
{
	ASCENT_NONE = 0,
	ASCENT_RISING,
	ASCENT_FALLING,
} AscentStates_t;

/* Private types -------------------------------------------------------------*/
const SGas Air = {79,0,0,0,0};

/* Exported variables --------------------------------------------------------*/
SGlobal global;
SDevice DeviceDataFlash;
uint8_t deviceDataFlashValid = 0;
uint8_t deviceDataSubSeconds = 0;

/* Private variables ---------------------------------------------------------*/
static uint16_t ManualExitDiveCounter = 0;   /* The computer will exit dive mode in shallow area immediately. Increase depth to restart dive while counter is active */

/* can be lost while in sleep */
uint8_t clearDecoNow = 0;
uint8_t setButtonsNow = 0;

/* has to be in SRAM2 */
uint8_t secondsCount = 0;

static uint8_t dospisync = SPI_SYNC_METHOD_NONE;

SScheduleCtrl Scheduler;
 
/* Private function prototypes -----------------------------------------------*/

_Bool vpm_crush2(void);
void scheduleUpdateDeviceData(void);
long get_nofly_time_minutes(void);
void copyActualGas(SGas gas);
void copyPressureData(void);
void copyCnsAndOtuData(void);
void copyTimeData(void);
void copyCompassData(void);
void copyCompassDataDuringCalibration(int16_t dx, int16_t dy, int16_t dz);
void copyAmbientLightData(void);
void copyTissueData(void);
void copyVpmCrushingData(void);
void copyDeviceData(void);
void copyPICdata(void);
void copyExtADCdata();
void copyExtCO2data();
void copyGNSSdata(void);
static void schedule_update_timer_helper(int8_t thisSeconds);
static void evaluateAscentSpeed(void);
uint32_t time_elapsed_ms(uint32_t ticksstart,uint32_t ticksnow);

void scheduleSetDate(SDeviceLine *line);

/* Exported functions --------------------------------------------------------*/

void initGlobals(void)
{
	bzero(&global, sizeof(SGlobal));
	
	global.dataSendToSlavePending = 0;
	global.dataSendToSlaveIsValid = 1;
	global.dataSendToSlaveIsNotValidCount = 0;

	global.mode = MODE_POWERUP;
	global.repetitive_dive = 0;
	global.conservatism = 0;
	global.whichGas = 0;
	global.aktualGas[0] = Air;
	global.lifeData.actualGas = global.aktualGas[0];

	const uint8_t button_standard_sensitivity = 51;		/* 51 equals a percentage of 85% which was the default value before */
	global.ButtonResponsiveness[0] = button_standard_sensitivity;
	global.ButtonResponsiveness[1] = button_standard_sensitivity;
	global.ButtonResponsiveness[2] = button_standard_sensitivity;
	global.ButtonResponsiveness[3] = button_standard_sensitivity;
	
	global.ButtonPICdata[0] = 0xFF;
	global.ButtonPICdata[1] = 0xFF;
	global.ButtonPICdata[2] = 0xFF;
	global.ButtonPICdata[3] = 0xFF;

	global.I2C_SystemStatus = HAL_ERROR; // 0x00 would be everything working
	
	global.lifeData.battery_voltage = BATTERY_DEFAULT_VOLTAGE;

	global.lifeData.pressure_ambient_bar = INVALID_PREASURE_VALUE;
	global.lifeData.pressure_surface_bar = INVALID_PREASURE_VALUE;
	decom_reset_with_1000mbar(&global.lifeData);
	
	global.demo_mode = 0;

	for(int i = 0; i < MAX_SENSORS; i++)
	{
		global.sensorError[i] = HAL_OK; // HAL_OK = 0;
	}

	global.dataSendToMaster.RTE_VERSION_high = firmwareVersionHigh();//RTE_VERSION_HIGH;;
	global.dataSendToMaster.RTE_VERSION_low = firmwareVersionLow();//RTE_VERSION_LOW;;
	global.dataSendToMaster.chargeStatus = CHARGER_off;
	
	global.dataSendToMaster.power_on_reset = 0;
	global.dataSendToMaster.header.checkCode[0] = 0xA1;
	global.dataSendToMaster.header.checkCode[1] = SPI_RX_STATE_OFFLINE;
	global.dataSendToMaster.header.checkCode[2] = 0xA3;
	global.dataSendToMaster.header.checkCode[3] = 0xA4;
	global.dataSendToMaster.footer.checkCode[3] = 0xE4;
	global.dataSendToMaster.footer.checkCode[2] = 0xE3;
	global.dataSendToMaster.footer.checkCode[1] = 0xE2;
	global.dataSendToMaster.footer.checkCode[0] = 0xE1;
	global.dataSendToMaster.sensorErrors = 0;

	global.sync_error_count = 0;
	global.check_sync_not_running = 0;

	global.deviceDataSendToMaster.RTE_VERSION_high = firmwareVersionHigh();//RTE_VERSION_HIGH;
	global.deviceDataSendToMaster.RTE_VERSION_low = firmwareVersionLow();//RTE_VERSION_LOW;
	global.deviceDataSendToMaster.chargeStatus = CHARGER_off;

	global.deviceDataSendToMaster.power_on_reset = 0;
	global.deviceDataSendToMaster.header.checkCode[0] = 0xDF;
	global.deviceDataSendToMaster.header.checkCode[1] = 0xDE;
	global.deviceDataSendToMaster.header.checkCode[2] = 0xDD;
	global.deviceDataSendToMaster.header.checkCode[3] = 0xDC;
	global.deviceDataSendToMaster.footer.checkCode[3] = 0xE4;
	global.deviceDataSendToMaster.footer.checkCode[2] = 0xE3;
	global.deviceDataSendToMaster.footer.checkCode[1] = 0xE2;
	global.deviceDataSendToMaster.footer.checkCode[0] = 0xE1;
	
	global.dataSendToSlave.getDeviceDataNow = 0;
	
	global.deviceData.batteryChargeCompleteCycles.value_int32 = 0;
	global.deviceData.batteryChargeCycles.value_int32 = 0;
	global.deviceData.depthMaximum.value_int32 = 0;
	global.deviceData.diveCycles.value_int32 = 0;
	global.deviceData.hoursOfOperation.value_int32 = 0;
	global.deviceData.temperatureMaximum.value_int32 = INT32_MIN;
	global.deviceData.temperatureMinimum.value_int32 = INT32_MAX;
	global.deviceData.voltageMinimum.value_int32 = INT32_MAX;

	Scheduler.communicationTimeout = SPI_COM_TIMEOUT_START;
	Scheduler_Request_sync_with_SPI(SPI_SYNC_METHOD_HARD);
}

void reinitGlobals(void)
{
	global.dataSendToSlavePending = 0;
	global.dataSendToSlaveIsValid = 0;
	global.dataSendToSlaveIsNotValidCount = 0;
	global.sync_error_count = 0;
	global.check_sync_not_running = 0;
	Scheduler.communicationTimeout = SPI_COM_TIMEOUT_START;
}

void scheduleSpecial_Evaluate_DataSendToSlave(void)
{
	//TEMPORARY fix for compass calibration.
	//TODO: Fix I2C timeout for complete solving problem.
	if(global.mode==MODE_CALIB){
		return;
	}

	global.dataSendToSlavePending = 0;
	if(!global.dataSendToSlaveIsValid) return;
	
	global.dataSendToMaster.confirmRequest.uw = 0;
	
	if(TM_OTP_Read(0,0) == 0xFF)
	{
		if(global.dataSendToSlave.revisionHardware == (global.dataSendToSlave.revisionCRCx0x7A ^ 0x7A))
			TM_OTP_Write(0,0,global.dataSendToSlave.revisionHardware);
	}
	
	if(global.dataSendToSlave.setAccidentFlag)
	{
		global.dataSendToMaster.confirmRequest.ub.accident = 1;
		global.deviceData.diveAccident.value_int32 = global.dataSendToSlave.setAccidentFlag;
		scheduleSetDate(&global.deviceData.diveAccident);
		global.accidentFlag |= global.dataSendToSlave.setAccidentFlag;
		if(global.accidentFlag == ACCIDENT_CNS) // LVL1
			global.accidentRemainingSeconds = 2*60*60;
		else
			global.accidentRemainingSeconds = 24*60*60;
	}
	
	if(global.dataSendToSlave.setTimeNow)
	{
		global.dataSendToMaster.confirmRequest.ub.time = 1;
		RTC_SetTime(global.dataSendToSlave.data.newTime);
		schedule_update_timer_helper(0);
	}
	
	if(global.dataSendToSlave.setDateNow)
	{
		global.dataSendToMaster.confirmRequest.ub.date = 1;
		RTC_SetDate(global.dataSendToSlave.data.newDate);
		schedule_update_timer_helper(0);
	}		

	if(global.dataSendToSlave.calibrateCompassNow)
	{
		global.dataSendToMaster.confirmRequest.ub.compass = 1;
		global.mode = MODE_CALIB;
	}		

	if(global.dataSendToSlave.clearDecoNow)
	{
		global.dataSendToMaster.confirmRequest.ub.clearDeco = 1;
		clearDecoNow = 1;
	}		

	if(global.dataSendToSlave.setButtonSensitivityNow)
	{
		global.dataSendToMaster.confirmRequest.ub.button = 1;
		global.ButtonResponsiveness[0] = global.dataSendToSlave.data.buttonResponsiveness[0];
		global.ButtonResponsiveness[1] = global.dataSendToSlave.data.buttonResponsiveness[1];
		global.ButtonResponsiveness[2] = global.dataSendToSlave.data.buttonResponsiveness[2];
		global.ButtonResponsiveness[3] = global.dataSendToSlave.data.buttonResponsiveness[3];
		setButtonsNow = 1;
	}		

	if(global.dataSendToSlave.setBatteryGaugeNow)
	{
		if(global.mode!=MODE_CALIB){
		global.dataSendToMaster.confirmRequest.ub.batterygauge = 1;
		battery_gas_gauge_set(global.dataSendToSlave.data.newBatteryGaugePercentageFloat);
		}
	}		

	if(global.dataSendToSlave.setEndDive)
	{
		ManualExitDiveCounter = 30 * 60; /* This will cause the computer to leave dive mode if in shallow area and increase the depth to enter dive mode for the next 30 minutes */
	}

	if((global.mode == MODE_SURFACE) && (global.dataSendToSlave.mode == MODE_SHUTDOWN))
	{
		global.mode = MODE_SHUTDOWN;
	}

	if(global.mode == MODE_DIVE)
	{
		copyActualGas(global.dataSendToSlave.data.actualGas);
	}
	else
	{
		copyActualGas(Air);
		global.settings.divetimeToCreateLogbook = global.dataSendToSlave.data.divetimeToCreateLogbook;
		global.settings.timeoutDiveReachedZeroDepth = global.dataSendToSlave.data.timeoutDiveReachedZeroDepth;
	}
	
	/* for simulation / testing */
	global.ceiling_from_main_CPU_mbar	= global.dataSendToSlave.data.ambient_pressure_mbar_ceiling;
	
	/* Set pressure and temperature offsets */
	pressure_set_offset (global.dataSendToSlave.data.offsetPressureSensor_mbar, global.dataSendToSlave.data.offsetTemperatureSensor_centiDegree);


	/* for device data updates */
	deviceDataFlashValid = 0;
	memcpy(&DeviceDataFlash, &global.dataSendToSlave.data.DeviceData, sizeof(SDevice));
	deviceDataFlashValid = 1;


	/* handle external interface requests */

	if((global.dataSendToSlave.data.externalInterface_Cmd && EXT_INTERFACE_33V_ON) != externalInterface_isEnabledPower33())
	{
		externalInterface_SwitchPower33(global.dataSendToSlave.data.externalInterface_Cmd && EXT_INTERFACE_33V_ON);
	}

	if(((global.dataSendToSlave.data.externalInterface_Cmd & EXT_INTERFACE_ADC_ON) != 0) != externalInterface_isEnabledADC())
	{
		externalInterface_SwitchADC(1-externalInterface_isEnabledADC());
	}

	externalInface_SetSensorMap(global.dataSendToSlave.data.externalInterface_SensorMap);
	if(global.dataSendToSlave.data.externalInterface_Cmd & 0x00FF)	/* lowest nibble for commands */
	{
		externalInterface_ExecuteCmd(global.dataSendToSlave.data.externalInterface_Cmd);
	}


#if 0
	//TODO: Temporary placed here. Duration ~210 ms.
	if (global.I2C_SystemStatus != HAL_OK) {
		MX_I2C1_TestAndClear();
		MX_I2C1_Init();
//		init_pressure();
//		compass_init(0, 7);
//		accelerator_init();
	}
#endif /* already called once a second */
}


/**
  ******************************************************************************
* @brief   schedule_time_compare_helper. 
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    20-Oct-2016
  ******************************************************************************
  */

uint8_t RtcBugFixChsw(uint8_t inStupidTime)
{
		uint8_t multiplesOf16 = 0;
	
	multiplesOf16 = inStupidTime / 16;

	inStupidTime -= multiplesOf16 * 16;
	
	return (10 * multiplesOf16) + inStupidTime;
}

uint32_t schedule_time_compare_helper(RTC_TimeTypeDef timeNow, RTC_DateTypeDef dateNow, RTC_TimeTypeDef timeLast, RTC_DateTypeDef dateLast)
{
	uint32_t nowInSeconds;
	uint32_t lastInSeconds;
	uint32_t resultDiff;

	nowInSeconds  = (uint32_t)RtcBugFixChsw(timeNow.Hours) * 3600;
	nowInSeconds += (uint32_t)RtcBugFixChsw(timeNow.Minutes) * 60;
	nowInSeconds += (uint32_t)RtcBugFixChsw(timeNow.Seconds);

	lastInSeconds  = (uint32_t)RtcBugFixChsw(timeLast.Hours) * 3600;
	lastInSeconds += (uint32_t)RtcBugFixChsw(timeLast.Minutes) * 60;
	lastInSeconds += (uint32_t)RtcBugFixChsw(timeLast.Seconds);
	
	if(dateNow.Date != dateLast.Date)
	{
		resultDiff = 86400 + nowInSeconds - lastInSeconds;
	}
	else
	{
		resultDiff = nowInSeconds - lastInSeconds;
	}
	return resultDiff;
}



/**
  ******************************************************************************
* @brief   schedule_update_timer_helper. 
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    20-Oct-2016
	* @brief	use 0 for init
						use -1 for RTC controlled
						use >= 1 for manual control
  ******************************************************************************
  */
extern RTC_HandleTypeDef RTCHandle;

static void schedule_update_timer_helper(int8_t thisSeconds)
{
	static RTC_TimeTypeDef sTimeLast;
	static RTC_DateTypeDef sDateLast;
	RTC_TimeTypeDef sTimeNow;
	RTC_DateTypeDef sDateNow;
	uint32_t secondsPast;

	HAL_RTC_GetTime(&RTCHandle, &sTimeNow, RTC_FORMAT_BCD);
	HAL_RTC_GetDate(&RTCHandle, &sDateNow, RTC_FORMAT_BCD);

	if(thisSeconds != 0) // otherwise just store sTimeLast, sDateLast
	{
		if(thisSeconds > 0) // use this value instead, good for pre-loading sTimeLast and sDateLast
		{
			secondsPast	= thisSeconds;
		} else {
			// thisSeconds < 0 and not <= !
			secondsPast = schedule_time_compare_helper(sTimeNow, sDateNow, sTimeLast, sDateLast);
		}

		if(global.seconds_since_last_dive)
		{
			if(secondsPast >= 777900)
			{
				global.seconds_since_last_dive = 0;
			}
			else
			{
				uint32_t tempNewValue = ((uint32_t)global.seconds_since_last_dive) + secondsPast;
				if(tempNewValue > 777900) // a bit more than nine days [seconds]
					global.seconds_since_last_dive = 0;
				else
					global.seconds_since_last_dive = (long)tempNewValue;
			}
		}
	}
		
	sTimeLast = sTimeNow;
	sDateLast = sDateNow;
}

/**
  ******************************************************************************
* @brief   schedule_check_resync. 
  * @author  heinrichs weikamp gmbh
  * @version V0.0.2
  * @date    18-June-2015
  ******************************************************************************
  */

void schedule_check_resync(void)
{
	/* counter is incremented in cyclic 100ms loop and reset to 0 if the transmission complete callback is called */
	if((global.check_sync_not_running >= Scheduler.communicationTimeout))
	{
//		global.dataSendToSlaveIsNotValidCount = 0;
		global.check_sync_not_running = 0;
		global.sync_error_count++;

		/* Try to start communication again. If exchange is stuck during execution for some reason the TX will be aborted by the
		 * function error handler
		 */
		HAL_SPI_TransmitReceive_DMA(&hspi1,(uint8_t*) &(global.dataSendToMaster),(uint8_t*) &(global.dataSendToSlave), EXCHANGE_BUFFERSIZE);
		Scheduler.communicationTimeout = SPI_COM_TIMEOUT_COMMON;	/* Reduce error detection time */
		Scheduler_Request_sync_with_SPI(SPI_SYNC_METHOD_HARD);
	}
}


/**
  ******************************************************************************
* @brief   scheduleDiveMode. /  Dive Mode: Main Loop
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
  */
void scheduleDiveMode(void)
{
	uint32_t ticksdiff = 0; 
	uint32_t lasttick = 0;
	uint8_t extAdcChannel = 0;
	uint8_t counterAscentRate = 0;
	global.dataSendToMaster.mode = MODE_DIVE;
	global.deviceDataSendToMaster.mode = MODE_DIVE;
	uint8_t counter_exit = 0;

	Scheduler.counterSPIdata100msec = 0;
	Scheduler.counterCompass100msec = 0;
	Scheduler.counterPressure100msec = 0;
	Scheduler.counterAmbientLight100msec = 0;
	Scheduler.tick_execute1second = SCHEDULER_TICK_EXE1SEC;
	
	global.deviceData.diveCycles.value_int32++;
	scheduleSetDate(&global.deviceData.diveCycles);
	global.lifeData.counterSecondsShallowDepth = 0;

	/* Get the last stable value in case of an unstable surface history condition */
	if(!is_surface_pressure_stable())
	{
		set_last_surface_pressure_stable();
	}
	global.lifeData.pressure_surface_bar = get_surface_mbar() / 1000.0f;
	ManualExitDiveCounter = 0;	/* reset early exit request */

	Scheduler.tickstart = HAL_GetTick();
	while(global.mode == MODE_DIVE)
	{
		lasttick = HAL_GetTick();
		ticksdiff = time_elapsed_ms(Scheduler.tickstart,lasttick);

		externalInterface_HandleUART();
		if(ticksdiff >= Scheduler.counterSPIdata100msec * 100 + 10)
		{
			if(SPI_Evaluate_RX_Data()!=0) /* did we receive something ? */
			{
				Scheduler.counterSPIdata100msec++;
			}
			schedule_check_resync();

			if(externalInterface_isEnabledADC())
			{
				extAdcChannel = externalInterface_ReadAndSwitch();
				if(extAdcChannel != EXTERNAL_ADC_NO_DATA)
				{
					externalInterface_CalculateADCValue(extAdcChannel);
				}
			}
			copyExtADCdata();
			copyExtCO2data();
		}

		//Evaluate pressure at 20 ms, 120 ms, 220 ms,....
		if(ticksdiff >= Scheduler.counterPressure100msec * 100 + 20)
		{
				global.check_sync_not_running++;
				pressure_update_alternating();
				scheduleUpdateDeviceData();
#ifdef DEMOMODE
				if(global.demo_mode)
				{
					int turbo_seconds = demo_modify_temperature_and_pressure(global.lifeData.dive_time_seconds, Scheduler.counterPressure100msec, global.ceiling_from_main_CPU_mbar);
					if(turbo_seconds)
					{
						global.lifeData.dive_time_seconds += turbo_seconds;
						decom_tissues_exposure((int)(turbo_seconds), &global.lifeData);
						copyTissueData();
					}
					if((global.lifeData.counterSecondsShallowDepth > 1) && (global.lifeData.counterSecondsShallowDepth < (global.settings.timeoutDiveReachedZeroDepth - 10)))
						global.lifeData.counterSecondsShallowDepth = (global.settings.timeoutDiveReachedZeroDepth - 10);
				}
#endif

				counterAscentRate++;
				if(counterAscentRate == 4)
				{
					global.lifeData.pressure_ambient_bar = get_pressure_mbar() / 1000.0f;
					evaluateAscentSpeed();
					counterAscentRate = 0;
				}
				copyPressureData();
				Scheduler.counterPressure100msec++;
		}
			//evaluate compass data at 50 ms, 150 ms, 250 ms,....
		if(ticksdiff >= Scheduler.counterCompass100msec * 100 + 50)
		{
			compass_read();
			acceleration_read();
			compass_calc();
			copyCompassData();
			Scheduler.counterCompass100msec++;
		}
		
		if(ticksdiff >= Scheduler.counterAmbientLight100msec * 100 + 70)
		{
			adc_ambient_light_sensor_get_data();
			copyAmbientLightData();
			Scheduler.counterAmbientLight100msec++;
		}

		//Evaluate tissues, toxic data, vpm, etc. once a second
		if(ticksdiff >= Scheduler.tick_execute1second)
		{
			Scheduler.tick_execute1second = 0xFFFFFFFF;	/* execute once only in the second cycle */
			if(global.dataSendToSlave.diveModeInfo != DIVEMODE_Apnea)
			{
				scheduleUpdateLifeData(0); // includes tissues
				global.lifeData.dive_time_seconds++; // there is dive_time_seconds_without_surface_time too
				global.lifeData.ppO2 = decom_calc_ppO2(global.lifeData.pressure_ambient_bar, &global.lifeData.actualGas);
				decom_oxygen_calculate_cns(&global.lifeData.cns,global.lifeData.ppO2);
				decom_oxygen_calculate_otu(&global.lifeData.otu,global.lifeData.ppO2);
				battery_gas_gauge_get_data();


				/** counter_exit allows safe exit via button for testing
					* and demo_mode is exited too if applicable.
					*/
				if(global.dataSendToMaster.mode == MODE_ENDDIVE)
				{
					counter_exit++;
					if(counter_exit >= 2)
					{
						global.mode = MODE_SURFACE;
						global.demo_mode = 0;
					}
				}

				if(is_ambient_pressure_close_to_surface(&global.lifeData))
				{

					global.lifeData.counterSecondsShallowDepth++;
					if((global.lifeData.counterSecondsShallowDepth >= global.settings.timeoutDiveReachedZeroDepth) || ((global.lifeData.dive_time_seconds < 60) && (global.demo_mode == 0))
							|| (ManualExitDiveCounter))
					{
						global.seconds_since_last_dive = 1; // start counter
						schedule_update_timer_helper(0); // zum starten :-)
						global.dataSendToMaster.mode = MODE_ENDDIVE;
						global.deviceDataSendToMaster.mode = MODE_ENDDIVE;
					}
				}
				else
				{
					global.lifeData.counterSecondsShallowDepth = 0;
					global.lifeData.dive_time_seconds_without_surface_time++;
				}
				vpm_crush2();
			}
			else // DIVEMODE_Apnea
			{
				global.lifeData.dive_time_seconds++;

				// exit dive mode
				if(global.dataSendToMaster.mode == MODE_ENDDIVE)
				{
					counter_exit++;
					if(counter_exit >= 2)
					{
						scheduleUpdateLifeData(-1); // 'restart' tissue calculations without calculating time during apnea mode
						global.lifeData.dive_time_seconds = 0; // use backup noflytime and desaturation time
						global.mode = MODE_SURFACE;
						global.demo_mode = 0;
					}
				}

				// surface break
				if(is_ambient_pressure_close_to_surface(&global.lifeData))
				{
					global.lifeData.ascent_rate_meter_per_min = 0;
					global.lifeData.counterSecondsShallowDepth++;
					if(global.lifeData.counterSecondsShallowDepth > 3) // time for main cpu to copy to apnea_last_dive_time_seconds
					{
						global.lifeData.dive_time_seconds = 0; // this apnea dive ends here
					}
					if((global.lifeData.counterSecondsShallowDepth >= global.settings.timeoutDiveReachedZeroDepth) || (ManualExitDiveCounter))
					{
						global.dataSendToMaster.mode = MODE_ENDDIVE;
						global.deviceDataSendToMaster.mode = MODE_ENDDIVE;
					}
				}
				else
				{
					global.lifeData.counterSecondsShallowDepth = 0;
					global.lifeData.dive_time_seconds_without_surface_time++; 
				}
			} // standard dive or DIVEMODE_Apnea
		
			copyVpmCrushingData();
			copyTimeData();
			copyCnsAndOtuData();
			copyBatteryData();

			// new hw 170523
			if(global.I2C_SystemStatus != HAL_OK)
			{
				MX_I2C1_TestAndClear();
				HAL_Delay(100);
				I2C_DeInit();
				HAL_Delay(100);
				MX_I2C1_Init();
				HAL_Delay(100);

				init_pressure();
			}
		}
		if(ticksdiff >= 1000)
		{
			/* reset counter */
			Scheduler.tickstart = HAL_GetTick();
			Scheduler.counterSPIdata100msec = 0;
			Scheduler.counterCompass100msec = 0;
			Scheduler.counterPressure100msec = 0;
			Scheduler.counterAmbientLight100msec = 0;
			Scheduler.tick_execute1second = SCHEDULER_TICK_EXE1SEC;
		}
	}
}


/**
  ******************************************************************************
* @brief   scheduleSurfaceMode / surface mode: Main Loop
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
  */


//  ===============================================================================
//	scheduleTestMode
/// @brief	included for sealed hardware with permanent RTE update message
//  ===============================================================================
void scheduleTestMode(void)
{
	uint32_t ticksdiff = 0; 
	uint32_t lasttick = 0;
	Scheduler.tickstart = HAL_GetTick();

	Scheduler.counterPressure100msec = 0;
	
	float temperature_carousel = 0.0f;
	float temperature_changer = 0.1f;
	
	while(global.mode == MODE_TEST)
	{
		lasttick = HAL_GetTick();
		ticksdiff = time_elapsed_ms(Scheduler.tickstart,lasttick);

		//Evaluate received data at 10 ms, 110 ms, 210 ms,...
		if(ticksdiff >= Scheduler.counterSPIdata100msec * 100 + 10)
		{
			if(SPI_Evaluate_RX_Data()!=0) /* did we receive something ? */
			{
				Scheduler.counterSPIdata100msec++;
			}
			schedule_check_resync();
		}
		
		//Evaluate pressure at 20 ms, 120 ms, 220 ms,...
		if(ticksdiff >= Scheduler.counterPressure100msec * 100 + 20)
		{
				global.check_sync_not_running++;

				pressure_update_alternating();
				scheduleUpdateDeviceData();
				global.lifeData.ascent_rate_meter_per_min = 0;
				copyPressureData();

				if(temperature_carousel > 20.0f)
				{
					temperature_carousel = 20.0f;
					temperature_changer = -0.1f;
				}
				else
				if(temperature_carousel < 0)
				{
					temperature_carousel = 0;
					temperature_changer = +0.1f;
				}
					
				temperature_carousel += temperature_changer;
				
				uint8_t boolPressureData = !global.dataSendToMaster.boolPressureData;

				global.dataSendToMaster.data[boolPressureData].pressure_mbar = get_pressure_mbar();

				global.dataSendToMaster.data[boolPressureData].temperature = temperature_carousel;
				global.dataSendToMaster.data[boolPressureData].pressure_uTick = HAL_GetTick();
				global.dataSendToMaster.boolPressureData = boolPressureData;
				Scheduler.counterPressure100msec++;
		}
		
		if(ticksdiff >= 1000)
		{
			//Set back tick counter
			Scheduler.tickstart = HAL_GetTick();
			Scheduler.counterPressure100msec = 0;
			Scheduler.counterSPIdata100msec = 0;
		}
	};	
}



void scheduleSurfaceMode(void)
{
	uint32_t ticksdiff = 0; 
	uint32_t lasttick = 0;
	uint8_t extAdcChannel = 0;
	uint8_t batteryToggle = 0;		/* ADC is operating in automatic 2 second cycles => consider for battery charge function call */

	Scheduler.tickstart = HAL_GetTick();
	Scheduler.counterSPIdata100msec = 0;
	Scheduler.counterCompass100msec = 0;
	Scheduler.counterPressure100msec = 0;
	Scheduler.counterAmbientLight100msec = 0;
	Scheduler.tick_execute1second = SCHEDULER_TICK_EXE1SEC;

	global.dataSendToMaster.mode = MODE_SURFACE;
	global.deviceDataSendToMaster.mode = MODE_SURFACE;

	while(global.mode == MODE_SURFACE)
	{

		lasttick = HAL_GetTick();
		ticksdiff = time_elapsed_ms(Scheduler.tickstart,lasttick);

		if(setButtonsNow == 1)
		{
			if(scheduleSetButtonResponsiveness())
				setButtonsNow = 0;
		}

		externalInterface_HandleUART();
#ifdef ENABLE_GPIO_V2
		UART6_HandleUART();
#endif

		/* Evaluate received data at 10 ms, 110 ms, 210 ms,... duration ~<1ms */
		if(ticksdiff >= Scheduler.counterSPIdata100msec * 100 + 10)
		{
			if(SPI_Evaluate_RX_Data()!=0) /* did we receive something ? */
			{			
				Scheduler.counterSPIdata100msec++;
			}
			schedule_check_resync();
			if(externalInterface_isEnabledADC())
			{
				extAdcChannel = externalInterface_ReadAndSwitch();
				if(extAdcChannel != EXTERNAL_ADC_NO_DATA)
				{
					externalInterface_CalculateADCValue(extAdcChannel);

				}
			}
			copyExtADCdata();
			copyExtCO2data();
		}

		/* Evaluate pressure at 20 ms, 120 ms, 220 ms,... duration ~22ms] */
		if(ticksdiff >= Scheduler.counterPressure100msec * 100 + 20)
		{
				global.check_sync_not_running++;
				pressure_update_alternating();
				scheduleUpdateDeviceData();
				global.lifeData.ascent_rate_meter_per_min = 0;
				copyPressureData();
				Scheduler.counterPressure100msec++;
				
				if (!is_ambient_pressure_close_to_surface(&global.lifeData))
					global.mode = MODE_DIVE;
		}
		
		/* Evaluate compass data at 50 ms, 150 ms, 250 ms,... duration ~5ms */
		if(ticksdiff >= Scheduler.counterCompass100msec * 100 + 50)
		{
			compass_read();
			acceleration_read();
			compass_calc();
			copyCompassData();
			Scheduler.counterCompass100msec++;
		}

		/* evaluate compass data at 70 ms, 170 ms, 270 ms,... duration <1ms */
		if(ticksdiff >= Scheduler.counterAmbientLight100msec * 100 + 70)
		{
			adc_ambient_light_sensor_get_data();
			copyAmbientLightData();

#if defined ENABLE_GNSS_SUPPORT || defined ENABLE_GPIO_V2
			copyGNSSdata();
#endif
			Scheduler.counterAmbientLight100msec++;
		}



		/*  Evaluate tissues, toxic data, etc. once a second... duration ~1ms */
		if(ticksdiff >= Scheduler.tick_execute1second)
		{
			Scheduler.tick_execute1second = 0xFFFFFFFF;
			if(clearDecoNow)
			{
				decom_reset_with_1000mbar(&global.lifeData); ///< this should almost reset desaturation time
				// new 160215 hw
				global.repetitive_dive = 0;
				global.seconds_since_last_dive = 0; ///< this will reset OTU and CNS as well
				global.no_fly_time_minutes = 0;
				global.accidentFlag = 0;
				global.accidentRemainingSeconds = 0;
				vpm_init(&global.vpm, global.conservatism, global.repetitive_dive, global.seconds_since_last_dive);
				clearDecoNow = 0;
			}

			if(ManualExitDiveCounter)
			{
				ManualExitDiveCounter--;
			}

			if(global.seconds_since_last_dive)
			{
				schedule_update_timer_helper(-1);
			}

			if(global.accidentRemainingSeconds)
			{
				global.accidentRemainingSeconds--;
				if(!global.accidentRemainingSeconds)
					global.accidentFlag = 0;
			}
			global.dataSendToMaster.accidentFlags = global.accidentFlag;

			update_surface_pressure(1);
			scheduleUpdateLifeData(0);
			decom_oxygen_calculate_otu_degrade(&global.lifeData.otu, global.seconds_since_last_dive);
			decom_oxygen_calculate_cns_degrade(&global.lifeData.cns, global.seconds_since_last_dive);

			/* start desaturation calculation after first valid measurement has been done */
			if(global.lifeData.pressure_surface_bar != INVALID_PREASURE_VALUE)
			{
				global.lifeData.desaturation_time_minutes = decom_calc_desaturation_time(global.lifeData.tissue_nitrogen_bar,global.lifeData.tissue_helium_bar,global.lifeData.pressure_surface_bar);
			}
			else
			{
				global.lifeData.desaturation_time_minutes = 0;
			}

			if(!batteryToggle)
			{
				battery_gas_gauge_get_data();
				battery_charger_get_status_and_contral_battery_gas_gauge(2);
				batteryToggle = 1;
			}
			else
			{
				batteryToggle = 0;
			}

			copyCnsAndOtuData();
			copyTimeData();
			copyBatteryData();
			copyDeviceData();


/* check if I2C is not up an running and try to reactivate if necessary. Also do initialization if problem occured during startup */
			if(global.I2C_SystemStatus != HAL_OK)
			{
				MX_I2C1_TestAndClear();
				HAL_Delay(100);
				I2C_DeInit();
				HAL_Delay(100);
				MX_I2C1_Init();
				HAL_Delay(100);

				if(global.I2C_SystemStatus == HAL_OK)
				{
					init_pressure();
					if(is_init_pressure_done())		/* Init surface data with initial measurement */
					{
						init_surface_ring(0);
					}

					if(!battery_gas_gauge_CheckConfigOK())
					{
						 init_battery_gas_gauge();
					}
				}
			}
			externalInterface_AutodetectSensor();
		}

		if(ticksdiff >= 1000)
		{
			//Set back tick counter
			Scheduler.tickstart = HAL_GetTick();
			Scheduler.counterSPIdata100msec = 0;
			Scheduler.counterCompass100msec = 0;
			Scheduler.counterPressure100msec = 0;
			Scheduler.counterAmbientLight100msec = 0;
			Scheduler.tick_execute1second = SCHEDULER_TICK_EXE1SEC;
		}
	}
}

inline void Scheduler_Request_sync_with_SPI(uint8_t SyncMethod)
{
	if( SyncMethod < SPI_SYNC_METHOD_INVALID)
	{
		dospisync = SyncMethod;
	}
}

void Scheduler_SyncToSPI(uint8_t TXtick)
{
	uint32_t deltatick = 0;
	int8_t TXcompensation;

	switch(dospisync)
	{
		case SPI_SYNC_METHOD_HARD:
				//Set back tick counter
				Scheduler.tickstart = HAL_GetTick() - 4; /* consider 4ms offset for transfer */
				Scheduler.counterSPIdata100msec = 0;
				Scheduler.counterCompass100msec = 0;
				Scheduler.counterPressure100msec = 0;
				Scheduler.counterAmbientLight100msec = 0;
				dospisync = SPI_SYNC_METHOD_NONE;
			break;
		case SPI_SYNC_METHOD_SOFT:
				deltatick = time_elapsed_ms(Scheduler.tickstart,HAL_GetTick());
				deltatick %= 100;  						/* clip to 100ms window */
				if(Scheduler.tickstart - deltatick >= 0) /* adjust start time to the next 100ms window */
				{
					Scheduler.tickstart -= deltatick;
				}
				else
				{
					Scheduler.tickstart = 0xFFFFFFFF- (deltatick - Scheduler.tickstart);
				}
				dospisync = SPI_SYNC_METHOD_NONE;
			break;
		default:										/* continous sync activity */
			if(TXtick < 100)							/* do not handle unexpected jump length > 100ms */
			{
				TXtick += 4;	/* add 4ms TX time to offset of 100ms time stamp */
				deltatick = time_elapsed_ms(Scheduler.tickstart,HAL_GetTick());
				deltatick %= 100;
				if(deltatick > 50)
				{
					TXcompensation = deltatick - 100; /* neg drift */
				}
				else
				{
					TXcompensation = deltatick;		/* pos drift */
				}
				TXcompensation = TXtick - TXcompensation;
				Scheduler.tickstart -= TXcompensation;
			}
			else
			{
				Scheduler_Request_sync_with_SPI(SPI_SYNC_METHOD_SOFT); /* A large shift in 100ms cycle occured => clip to 100ms in next sync call */
			}
			break;
	}
}

/**
  ******************************************************************************
	* @brief   scheduleCompassCalibrationMode
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @since   31-March-2015
  * @date    31-March-2015
  ******************************************************************************
  */
void scheduleCompassCalibrationMode(void)
{
	compass_init(1,7); // fast mode, max gain
	compass_calib(); // duration : 1 minute!
	compass_init(0,7); // back to normal mode

	if(global.seconds_since_last_dive)
	{
				schedule_update_timer_helper(-1);
	}

	scheduleUpdateLifeData(0);
	global.mode = MODE_SURFACE;
}


/**
  ******************************************************************************
	* @brief   scheduleSleepMode / sleep mode: Main Loop
  * @author  heinrichs weikamp gmbh
  * @version V0.0.2
  * @since   31-March-2015
  * @date    22-April-2014
  ******************************************************************************
  */

void scheduleSleepMode(void)
{
	global.dataSendToMaster.mode = 0;
	global.deviceDataSendToMaster.mode = 0;
	secondsCount = 0;
	
	/* prevent button wake up problem while in sleep_prepare
	 * sleep prepare does I2C_DeInit()
	 */
	if(global.mode != MODE_SLEEP)
		MX_I2C1_Init();
	else
	do
	{
		I2C_DeInit();

#ifdef DEBUGMODE
		HAL_Delay(2000);
#else
		RTC_StopMode_2seconds();
#endif
		

		
		if(global.mode == MODE_SLEEP)
			secondsCount += 2;

		externalInterface_InitPower33();
		MX_I2C1_Init();
		pressure_sensor_get_pressure_raw();

/* check if I2C is not up and running and try to reactivate if necessary. Also do initialization if problem occurred during startup */
		if(global.I2C_SystemStatus != HAL_OK)
		{
			MX_I2C1_TestAndClear();
			HAL_Delay(100);
			I2C_DeInit();
			HAL_Delay(100);
			MX_I2C1_Init();
			HAL_Delay(100);

			if((global.I2C_SystemStatus == HAL_OK) && (!is_init_pressure_done()))
			{
				init_pressure();
			}
		}

		if((secondsCount >= 30) || (global.mode != MODE_SLEEP)) /* Service battery charge state in case sleep is left */
		{
			pressure_sensor_get_temperature_raw();
			battery_gas_gauge_get_data();
			ReInit_battery_charger_status_pins();
			battery_charger_get_status_and_contral_battery_gas_gauge(secondsCount);
//			DeInit_battery_charger_status_pins();
			secondsCount = 0;
		}
		
		pressure_calculation();

		scheduleUpdateDeviceData();
		update_surface_pressure(2);

		if(global.seconds_since_last_dive)
		{
			schedule_update_timer_helper(-1);
		}

		if(global.accidentRemainingSeconds)
		{
			if(global.accidentRemainingSeconds > 2)			
				global.accidentRemainingSeconds -= 2;
			else
			{
				global.accidentRemainingSeconds = 0;
				global.accidentFlag = 0;
			}
		}

		if (((!is_ambient_pressure_close_to_surface(&global.lifeData)) && (global.lifeData.pressure_surface_bar > START_DIVE_MOUNTAIN_MODE_BAR ))
				|| (global.lifeData.pressure_ambient_bar > START_DIVE_IMMEDIATLY_BAR))
		{
			global.mode = MODE_BOOT;
		}
		scheduleUpdateLifeData(2000);
	}
	while(global.mode == MODE_SLEEP);
	/* new section for system after Standby */
	scheduleUpdateLifeData(-1);
	clearDecoNow = 0;
	setButtonsNow = 0;
	reinitGlobals();
	ReInit_battery_charger_status_pins();
}



/* Private functions ---------------------------------------------------------*/

/**
  ******************************************************************************
	* @brief   scheduleUpdateLifeData / calculates tissues
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
  */
	
	
void scheduleUpdateLifeData(int32_t asynchron_milliseconds_since_last)
{
	static _Bool first = 1;
	static uint32_t tickstart = 0;
	static uint32_t ticksrest = 0;

	uint32_t ticksdiff = 0; 
	uint32_t ticksnow = 0;
	uint32_t time_seconds = 0;
	uint8_t whichGasTmp = 0;
	
	uint8_t updateTissueData = 0;


	if(global.lifeData.pressure_surface_bar == INVALID_PREASURE_VALUE)
	{
		updateTissueData = 1;
	}

	if(asynchron_milliseconds_since_last < 0)
	{
		first = 1;
		tickstart = 0;
		ticksrest = 0;
		return;
	}

	if(!asynchron_milliseconds_since_last && first)
	{
		tickstart = HAL_GetTick();
		first = 0;
		return;
	}

	whichGasTmp = global.whichGas;
	global.lifeData.actualGas = global.aktualGas[whichGasTmp];
	global.lifeData.pressure_ambient_bar = get_pressure_mbar() / 1000.0f;
	global.lifeData.pressure_surface_bar = get_surface_mbar() / 1000.0f;

	if(updateTissueData)
	{
		decom_reset_with_ambientmbar(global.lifeData.pressure_surface_bar,&global.lifeData);
	}

	if(!asynchron_milliseconds_since_last)
	{
		ticksnow = HAL_GetTick();
		ticksdiff = time_elapsed_ms(tickstart,ticksnow);
	}
	else
	{
		first = 1;
		ticksdiff = asynchron_milliseconds_since_last;
	}

	if(ticksrest > 1000) 	// whatever happens after standby with STM32L476
		ticksrest = 0;			// maybe move static to SRAM2 

	ticksdiff += ticksrest;
	time_seconds = ticksdiff/ 1000;
	ticksrest = ticksdiff - time_seconds * 1000; 
	tickstart = ticksnow;

	decom_tissues_exposure((int)time_seconds, &global.lifeData);
	if(global.demo_mode)
		decom_tissues_exposure((int)(3*time_seconds), &global.lifeData);
  copyTissueData();
}


/**
  ******************************************************************************
* @brief   scheduleUpdateDeviceData
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    16-March-2015
	*
	* two step process
	* first compare with data from main CPU == externalLogbookFlash
	* second update with new sensor data
  ******************************************************************************
  */
void scheduleSetDate(SDeviceLine *line)
{
	extern RTC_HandleTypeDef RTCHandle;

	line->date_rtc_dr = (uint32_t)(RTCHandle.Instance->DR & RTC_DR_RESERVED_MASK); 
	line->time_rtc_tr = (uint32_t)(RTCHandle.Instance->TR & RTC_TR_RESERVED_MASK); 
}


void scheduleCopyDeviceData(SDeviceLine *lineWrite, const SDeviceLine *lineRead)
{	
	lineWrite->date_rtc_dr = lineRead->date_rtc_dr; 
	lineWrite->time_rtc_tr = lineRead->time_rtc_tr;
	lineWrite->value_int32 = lineRead->value_int32;
}


void scheduletranslateDate(uint32_t datetmpreg, RTC_DateTypeDef *sDate)
{
  datetmpreg = (uint32_t)(datetmpreg & RTC_DR_RESERVED_MASK);

  /* Fill the structure fields with the read parameters */
  sDate->Year = (uint8_t)((datetmpreg & (RTC_DR_YT | RTC_DR_YU)) >> 16);
  sDate->Month = (uint8_t)((datetmpreg & (RTC_DR_MT | RTC_DR_MU)) >> 8);
  sDate->Date = (uint8_t)(datetmpreg & (RTC_DR_DT | RTC_DR_DU));
  sDate->WeekDay = (uint8_t)((datetmpreg & (RTC_DR_WDU)) >> 13);

	/* Convert the date structure parameters to Binary format */
	sDate->Year = (uint8_t)RTC_Bcd2ToByte(sDate->Year);
	sDate->Month = (uint8_t)RTC_Bcd2ToByte(sDate->Month);
	sDate->Date = (uint8_t)RTC_Bcd2ToByte(sDate->Date);
}

void scheduleCheckDate(void)
{
	uint32_t localdate;
	RTC_DateTypeDef sDate;
	localdate =	(uint32_t)(RTCHandle.Instance->DR & RTC_DR_RESERVED_MASK);
	scheduletranslateDate(localdate, &sDate);

	/* RTC start in year 2000 in case of a power loss. Use the operation counter time stamp to bring at last date to a more realistic value */
	if(sDate.Year < 15)
	{
		scheduletranslateDate(DeviceDataFlash.hoursOfOperation.date_rtc_dr, &sDate);
		if(sDate.Year > 16)
		{
			RTC_SetDate(sDate);
		}
	}

}

void scheduleUpdateDeviceData(void)
{
	/* first step, main CPU */

	if(deviceDataFlashValid)
	{
		/* max values */
		if(global.deviceData.hoursOfOperation.value_int32 < DeviceDataFlash.hoursOfOperation.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.hoursOfOperation, &DeviceDataFlash.hoursOfOperation);
#ifdef RESTORE_LAST_KNOWN_DATE
			scheduleCheckDate();
#endif
		}
		if(global.deviceData.batteryChargeCompleteCycles.value_int32 < DeviceDataFlash.batteryChargeCompleteCycles.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.batteryChargeCompleteCycles, &DeviceDataFlash.batteryChargeCompleteCycles);
		}
		if(global.deviceData.batteryChargeCycles.value_int32 < DeviceDataFlash.batteryChargeCycles.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.batteryChargeCycles, &DeviceDataFlash.batteryChargeCycles);
		}
		if(global.deviceData.temperatureMaximum.value_int32 < DeviceDataFlash.temperatureMaximum.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.temperatureMaximum, &DeviceDataFlash.temperatureMaximum);
		}
		if(global.deviceData.depthMaximum.value_int32 < DeviceDataFlash.depthMaximum.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.depthMaximum, &DeviceDataFlash.depthMaximum);
		}
		if(global.deviceData.diveCycles.value_int32 < DeviceDataFlash.diveCycles.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.diveCycles, &DeviceDataFlash.diveCycles);
		}
		
		/* min values */
		if(global.deviceData.temperatureMinimum.value_int32 > DeviceDataFlash.temperatureMinimum.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.temperatureMinimum, &DeviceDataFlash.temperatureMinimum);
		}
		if(global.deviceData.voltageMinimum.value_int32 > DeviceDataFlash.voltageMinimum.value_int32)
		{
			scheduleCopyDeviceData(&global.deviceData.voltageMinimum, &DeviceDataFlash.voltageMinimum);
		}
	}		
		
	/* second step, sensor data */
	int32_t temperature_centigrad_int32;
	int32_t pressure_mbar_int32;
	int32_t voltage_mvolt_int32;
	
	temperature_centigrad_int32 = (int32_t)(get_temperature() * 100);
	if(temperature_centigrad_int32 < global.deviceData.temperatureMinimum.value_int32)
	{
		global.deviceData.temperatureMinimum.value_int32 = temperature_centigrad_int32;
		scheduleSetDate(&global.deviceData.temperatureMinimum);
	}

	if(temperature_centigrad_int32 > global.deviceData.temperatureMaximum.value_int32)
	{
		global.deviceData.temperatureMaximum.value_int32 = temperature_centigrad_int32;
		scheduleSetDate(&global.deviceData.temperatureMaximum);
	}

	pressure_mbar_int32 = (int32_t)get_pressure_mbar();
	if(pressure_mbar_int32 > global.deviceData.depthMaximum.value_int32)
	{
		global.deviceData.depthMaximum.value_int32 = pressure_mbar_int32;
		scheduleSetDate(&global.deviceData.depthMaximum);
	}
	
	voltage_mvolt_int32 = (int32_t)(get_voltage() * 1000);
	if(voltage_mvolt_int32 < global.deviceData.voltageMinimum.value_int32)
	{
		global.deviceData.voltageMinimum.value_int32 = voltage_mvolt_int32;
		scheduleSetDate(&global.deviceData.voltageMinimum);
	}

	/* third step, counter */
	switch (global.mode)
	{
		case MODE_SURFACE:
		case MODE_DIVE:
		default:
			deviceDataSubSeconds++;
			if(deviceDataSubSeconds > 10)
			{
				deviceDataSubSeconds = 0;
				global.deviceData.hoursOfOperation.value_int32++;
				scheduleSetDate(&global.deviceData.hoursOfOperation);
			}
			break;

		case MODE_SLEEP:
		case MODE_SHUTDOWN:
			break;
	}
}


void scheduleUpdateDeviceDataChargerFull(void)
{
	global.deviceData.batteryChargeCompleteCycles.value_int32++;
	scheduleSetDate(&global.deviceData.batteryChargeCompleteCycles);
}


void scheduleUpdateDeviceDataChargerCharging(void)
{
	global.deviceData.batteryChargeCycles.value_int32++;
	scheduleSetDate(&global.deviceData.batteryChargeCycles);
}


/**
  ******************************************************************************
* @brief   vpm_crush / calls vpm calc_crushing_pressure every four seconds during descend
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
  */
_Bool vpm_crush2(void)
{
    int i = 0;
    static float starting_ambient_pressure = 0;
    static float ending_ambient_pressure = 0;
    static float time_calc_begin = -1;
		static float initial_helium_pressure[16];
		static float initial_nitrogen_pressure[16];
		ending_ambient_pressure = global.lifeData.pressure_ambient_bar * 10;

	if((global.lifeData.dive_time_seconds <= 4) || (starting_ambient_pressure >= ending_ambient_pressure))
	{
		time_calc_begin = global.lifeData.dive_time_seconds;
		starting_ambient_pressure = global.lifeData.pressure_ambient_bar * 10;
		for( i = 0; i < 16; i++)
		{
			initial_helium_pressure[i] = global.lifeData.tissue_helium_bar[i] * 10;
			initial_nitrogen_pressure[i] = global.lifeData.tissue_nitrogen_bar[i] * 10;
		}
		return 0;
	}
	if(global.lifeData.dive_time_seconds - time_calc_begin >= 4)
	{
		if(ending_ambient_pressure > starting_ambient_pressure + 0.5f)
		{
			float rate = (ending_ambient_pressure - starting_ambient_pressure) * 60 / 4;
			calc_crushing_pressure(&global.lifeData, &global.vpm, initial_helium_pressure, initial_nitrogen_pressure, starting_ambient_pressure, rate);

			time_calc_begin = global.lifeData.dive_time_seconds;
			starting_ambient_pressure = global.lifeData.pressure_ambient_bar * 10;
			for( i = 0; i < 16; i++)
			{
				initial_helium_pressure[i] = global.lifeData.tissue_helium_bar[i] * 10;
				initial_nitrogen_pressure[i] =  global.lifeData.tissue_nitrogen_bar[i] * 10;
			}

			return 1;
		}

	}
	return 0;
}


long get_nofly_time_minutes(void)
{
	
	if(global.no_fly_time_minutes <= 0)
		return 0;
	
	long minutes_since_last_dive = global.seconds_since_last_dive/60;

	if((global.seconds_since_last_dive > 0) && (global.no_fly_time_minutes > minutes_since_last_dive))
	{
			return (global.no_fly_time_minutes - minutes_since_last_dive);
	}
	else
	{
		global.no_fly_time_minutes = 0;
		return 0;
	}
}


//Supports threadsave copying!!!
void copyActualGas(SGas gas)
{
	uint8_t whichGas = !global.whichGas;
	global.aktualGas[whichGas] = gas;
	global.whichGas = whichGas;
}


//Supports threadsave copying!!!
void copyPressureData(void)
{
	global.dataSendToMaster.sensorErrors = global.I2C_SystemStatus;
	uint8_t boolPressureData = !global.dataSendToMaster.boolPressureData;
	global.dataSendToMaster.data[boolPressureData].temperature = get_temperature();
	global.dataSendToMaster.data[boolPressureData].pressure_mbar = get_pressure_mbar();
	global.dataSendToMaster.data[boolPressureData].surface_mbar = get_surface_mbar();
	global.dataSendToMaster.data[boolPressureData].ascent_rate_meter_per_min = global.lifeData.ascent_rate_meter_per_min;
	global.dataSendToMaster.data[boolPressureData].pressure_uTick = HAL_GetTick();
	global.dataSendToMaster.boolPressureData = boolPressureData;
	global.dataSendToMaster.data[boolPressureData].SPARE1 = is_surface_pressure_stable();
}


//Supports threadsave copying!!!
void copyCnsAndOtuData(void)
{
	//uint8_t dataSendToMaster.
	uint8_t boolToxicData = !global.dataSendToMaster.boolToxicData;
	global.dataSendToMaster.data[boolToxicData].cns = global.lifeData.cns;
	global.dataSendToMaster.data[boolToxicData].otu = global.lifeData.otu;
	global.dataSendToMaster.data[boolToxicData].desaturation_time_minutes = global.lifeData.desaturation_time_minutes;
	global.dataSendToMaster.data[boolToxicData].no_fly_time_minutes = get_nofly_time_minutes();
	global.dataSendToMaster.boolToxicData = boolToxicData;
}


//Supports threadsave copying!!!
void copyTimeData(void)
{
	extern RTC_HandleTypeDef RTCHandle;
	
	uint8_t boolTimeData = !global.dataSendToMaster.boolTimeData;
	global.dataSendToMaster.data[boolTimeData].localtime_rtc_tr =  (uint32_t)(RTCHandle.Instance->TR & RTC_TR_RESERVED_MASK); 
	global.dataSendToMaster.data[boolTimeData].localtime_rtc_dr =  (uint32_t)(RTCHandle.Instance->DR & RTC_DR_RESERVED_MASK); 
	global.dataSendToMaster.data[boolTimeData].divetime_seconds = (uint32_t)global.lifeData.dive_time_seconds;
	global.dataSendToMaster.data[boolTimeData].dive_time_seconds_without_surface_time = (uint32_t)global.lifeData.dive_time_seconds_without_surface_time;
	global.dataSendToMaster.data[boolTimeData].surfacetime_seconds = (uint32_t)global.seconds_since_last_dive;
	global.dataSendToMaster.data[boolTimeData].counterSecondsShallowDepth = (uint32_t)global.lifeData.counterSecondsShallowDepth;
	global.dataSendToMaster.boolTimeData = boolTimeData;
}


//Supports threadsave copying!!!
void copyCompassData(void)
{
	extern float compass_heading;
	extern float compass_roll;
	extern float compass_pitch;
	//uint8_t dataSendToMaster.
	uint8_t boolCompassData = !global.dataSendToMaster.boolCompassData;
	global.dataSendToMaster.data[boolCompassData].compass_heading = compass_heading;
	global.dataSendToMaster.data[boolCompassData].compass_roll = compass_roll;
	global.dataSendToMaster.data[boolCompassData].compass_pitch = compass_pitch;
	global.dataSendToMaster.data[boolCompassData].compass_DX_f = 0;
	global.dataSendToMaster.data[boolCompassData].compass_DY_f = 0;
	global.dataSendToMaster.data[boolCompassData].compass_DZ_f = 0;
	global.dataSendToMaster.data[boolCompassData].compass_uTick = HAL_GetTick();
	global.dataSendToMaster.boolCompassData = boolCompassData;
}


void copyCompassDataDuringCalibration(int16_t dx, int16_t dy, int16_t dz)
{
	extern float compass_heading;
	extern float compass_roll;
	extern float compass_pitch;
	//uint8_t dataSendToMaster.
	uint8_t boolCompassData = !global.dataSendToMaster.boolCompassData;
	global.dataSendToMaster.data[boolCompassData].compass_heading = compass_heading;
	global.dataSendToMaster.data[boolCompassData].compass_roll = compass_roll;
	global.dataSendToMaster.data[boolCompassData].compass_pitch = compass_pitch;
	global.dataSendToMaster.data[boolCompassData].compass_DX_f = dx;
	global.dataSendToMaster.data[boolCompassData].compass_DY_f = dy;
	global.dataSendToMaster.data[boolCompassData].compass_DZ_f = dz;
	global.dataSendToMaster.boolCompassData = boolCompassData;
}


//Supports threadsave copying!!!
void copyBatteryData(void)
{
	uint8_t boolBatteryData = !global.dataSendToMaster.boolBatteryData;
	global.lifeData.battery_charge = get_charge();
	global.dataSendToMaster.data[boolBatteryData].battery_voltage = get_voltage();

	if(battery_gas_gauge_isChargeValueValid())
	{
		global.dataSendToMaster.data[boolBatteryData].battery_charge= global.lifeData.battery_charge;
	}
	else
	{
		global.dataSendToMaster.data[boolBatteryData].battery_charge = global.lifeData.battery_charge * -1.0;	/* negate value to show that this is just an assumption */
	}
	global.dataSendToMaster.boolBatteryData = boolBatteryData;
}


//Supports threadsave copying!!!
void copyAmbientLightData(void)
{
	uint8_t boolAmbientLightData = !global.dataSendToMaster.boolAmbientLightData;
	global.dataSendToMaster.data[boolAmbientLightData].ambient_light_level = get_ambient_light_level();
	global.dataSendToMaster.boolAmbientLightData = boolAmbientLightData;
}


//Supports threadsave copying!!!
void copyTissueData(void)
{
	//uint8_t dataSendToMaster.
	uint8_t boolTisssueData = !global.dataSendToMaster.boolTisssueData;
	for(int i = 0; i < 16; i++)
	{
		global.dataSendToMaster.data[boolTisssueData].tissue_nitrogen_bar[i] = global.lifeData.tissue_nitrogen_bar[i];
		global.dataSendToMaster.data[boolTisssueData].tissue_helium_bar[i] = global.lifeData.tissue_helium_bar[i];
	}
	global.dataSendToMaster.boolTisssueData = boolTisssueData;
}


//Supports threadsave copying!!!
void copyVpmCrushingData(void)
{
	//uint8_t dataSendToMaster.
	uint8_t boolCrushingData = !global.dataSendToMaster.boolCrushingData;
	for(int i = 0; i < 16; i++)
	{
		global.dataSendToMaster.data[boolCrushingData].max_crushing_pressure_n2[i] = global.vpm.max_crushing_pressure_n2[i];
		global.dataSendToMaster.data[boolCrushingData].max_crushing_pressure_he[i] = global.vpm.max_crushing_pressure_he[i];
		global.dataSendToMaster.data[boolCrushingData].adjusted_critical_radius_he[i] = global.vpm.adjusted_critical_radius_he[i];
		global.dataSendToMaster.data[boolCrushingData].adjusted_critical_radius_n2[i] = global.vpm.adjusted_critical_radius_n2[i];
	}
	global.dataSendToMaster.boolCrushingData = boolCrushingData;
}


void copyDeviceData(void)
{
	uint8_t boolDeviceData = !global.deviceDataSendToMaster.boolDeviceData;
	memcpy(&global.deviceDataSendToMaster.DeviceData[boolDeviceData], &global.deviceData,sizeof(SDevice));
	global.deviceDataSendToMaster.boolDeviceData = boolDeviceData; 

	global.deviceDataSendToMaster.boolVpmRepetitiveDataValid = 0;
	memcpy(&global.deviceDataSendToMaster.VpmRepetitiveData.adjusted_critical_radius_he, 		&global.vpm.adjusted_critical_radius_he,		sizeof(16*4));
	memcpy(&global.deviceDataSendToMaster.VpmRepetitiveData.adjusted_critical_radius_n2, 		&global.vpm.adjusted_critical_radius_n2,		sizeof(16*4));
	memcpy(&global.deviceDataSendToMaster.VpmRepetitiveData.adjusted_crushing_pressure_he,	&global.vpm.adjusted_crushing_pressure_he,	sizeof(16*4));
	memcpy(&global.deviceDataSendToMaster.VpmRepetitiveData.adjusted_crushing_pressure_n2,	&global.vpm.adjusted_crushing_pressure_n2,	sizeof(16*4));
	memcpy(&global.deviceDataSendToMaster.VpmRepetitiveData.initial_allowable_gradient_he,	&global.vpm.initial_allowable_gradient_he,	sizeof(16*4));
	memcpy(&global.deviceDataSendToMaster.VpmRepetitiveData.initial_allowable_gradient_n2,	&global.vpm.initial_allowable_gradient_n2,	sizeof(16*4));
	memcpy(&global.deviceDataSendToMaster.VpmRepetitiveData.max_actual_gradient, 					 	&global.vpm.max_actual_gradient,						sizeof(16*4));
	global.deviceDataSendToMaster.VpmRepetitiveData.repetitive_variables_not_valid = global.vpm.repetitive_variables_not_valid;
	global.deviceDataSendToMaster.boolVpmRepetitiveDataValid = 1;
}

/* 			copyPICdata(); is used in spi.c */
void copyPICdata(void)
{
	uint8_t boolPICdata = !global.dataSendToMaster.boolPICdata;
	for(int i = 0; i < 3; i++)
	{
		global.dataSendToMaster.data[boolPICdata].button_setting[i] = global.ButtonPICdata[i];
	}
	global.dataSendToMaster.boolPICdata = boolPICdata;
}

void copyExtADCdata()
{
	float value;

	uint8_t channel = 0;

	uint8_t boolADCBuffer =  ~(global.dataSendToMaster.boolADCO2Data & DATA_BUFFER_ADC);

	boolADCBuffer &= DATA_BUFFER_ADC;
	global.dataSendToMaster.boolADCO2Data &= ~DATA_BUFFER_ADC;

	for(channel = 0; channel < MAX_ADC_CHANNEL; channel++)
	{
		value = getExternalInterfaceChannel(channel);
		global.dataSendToMaster.data[boolADCBuffer && DATA_BUFFER_ADC].extADC_voltage[channel] = value;
	}
	global.dataSendToMaster.data[boolADCBuffer && DATA_BUFFER_ADC].externalInterface_SensorID = externalInterface_GetSensorData(0xFF, (uint8_t*)&global.dataSendToMaster.data[boolADCBuffer && DATA_BUFFER_ADC].sensor_data);
	memcpy(global.dataSendToMaster.data[boolADCBuffer && DATA_BUFFER_ADC].sensor_map,externalInterface_GetSensorMapPointer(1),EXT_INTERFACE_SENSOR_CNT);
	global.dataSendToMaster.boolADCO2Data |= boolADCBuffer;
}

void copyExtCO2data()
{
	uint16_t value;
	uint8_t boolCO2Buffer =  ~(global.dataSendToMaster.boolADCO2Data & DATA_BUFFER_CO2);

	global.dataSendToMaster.boolADCO2Data &= ~DATA_BUFFER_CO2;
	boolCO2Buffer &= DATA_BUFFER_CO2;

	if(externalInterface_GetCO2State())
	{
		value = externalInterface_GetCO2Value();
		global.dataSendToMaster.data[(boolCO2Buffer && DATA_BUFFER_CO2)].CO2_ppm = value;
		value = externalInterface_GetCO2SignalStrength();
		global.dataSendToMaster.data[(boolCO2Buffer && DATA_BUFFER_CO2)].CO2_signalStrength = value;
		global.dataSendToMaster.data[(boolCO2Buffer && DATA_BUFFER_CO2)].externalInterface_CmdAnswer = externalInterface_GetCO2State();
		externalInterface_SetCO2State(EXT_INTERFACE_33V_ON); 	/* clear command responses */
	}
	else
	{
		global.dataSendToMaster.data[(boolCO2Buffer && DATA_BUFFER_CO2)].CO2_ppm = 0;
		global.dataSendToMaster.data[(boolCO2Buffer && DATA_BUFFER_CO2)].CO2_signalStrength = 0;
		global.dataSendToMaster.data[(boolCO2Buffer && DATA_BUFFER_CO2)].externalInterface_CmdAnswer = 0;
	}
	global.dataSendToMaster.boolADCO2Data |= boolCO2Buffer;
}

void copyGNSSdata(void)
{
	global.dataSendToMaster.data[0].gnssInfo.fLat = GNSS_Handle.fLat;
	global.dataSendToMaster.data[0].gnssInfo.fLon = GNSS_Handle.fLon;
	global.dataSendToMaster.data[0].gnssInfo.fixType = GNSS_Handle.fixType;
	global.dataSendToMaster.data[0].gnssInfo.numSat = GNSS_Handle.numSat;
	memcpy(&global.dataSendToMaster.data[0].gnssInfo.signalQual,&GNSS_Handle.statSat, sizeof(GNSS_Handle.statSat));
}


typedef enum 
{
  SPI3_OK      = 0x00,
  SPI3_DEINIT  = 0x01,
} SPI3_StatusTypeDef;
/* if spi3 is running and the SPI3_ButtonAdjust call returns OK, all is fine
	 if the SPI3_ButtonAdjust call returns error, the spi3 is DeInit
	 and will be init the next call of scheduleSetButtonResponsiveness()
	 and data will be send again on the third call
	 therefore on return 0 of scheduleSetButtonResponsiveness() the caller flag should kept active
*/
uint8_t scheduleSetButtonResponsiveness(void)
{
	static uint8_t SPI3status = SPI3_OK;
	
	if((SPI3status == SPI3_OK) && (SPI3_ButtonAdjust(global.ButtonResponsiveness, global.ButtonPICdata)))
	{
		copyPICdata();
		return 1;
	}
	else
	{
		for(int i=0;i<3;i++)
		{
			global.ButtonPICdata[i] = 0xFF;
		}
		copyPICdata();
		
		if(SPI3status == SPI3_OK)
		{
			MX_SPI3_DeInit();
			SPI3status = SPI3_DEINIT;
		}
		else
		{
			MX_SPI3_Init();
			SPI3status = SPI3_OK;
		}
		return 0;
	}
}


//save time difference
uint32_t time_elapsed_ms(uint32_t ticksstart,uint32_t ticksnow)
{
	if(ticksstart <= ticksnow)
	{
			return ticksnow - ticksstart;
	}
	else
	{
		return 0xFFFFFFFF - ticksstart + ticksnow;
	}
}

/* same as in data_central.c */
_Bool is_ambient_pressure_close_to_surface(SLifeData *lifeData)
{
	_Bool retval = true;

	if(lifeData->pressure_ambient_bar != INVALID_PREASURE_VALUE)	/* as long as no valid data is available expect we are close to surface */
	{
		/* this will e.g. apply in case of a significant pressure change during last 30 minutes => use increased offset for surface detection */
		if (lifeData->pressure_ambient_bar > START_DIVE_IMMEDIATLY_BAR)
		{
			retval = false;
		}
		else if(is_surface_pressure_stable())		/* this is the expected start condition */
		{
			if((lifeData->pressure_ambient_bar >= (lifeData->pressure_surface_bar + 0.1f))
					&& (ManualExitDiveCounter == 0))		/* only if diver did not request to exit dive mode */
			{
				retval = false;
			}
		}
	}
	return retval;
}

void evaluateAscentSpeed()
{
	static uint32_t lastPressureTick = 0;
	static float lastPressure_bar = 0.0f;
	static AscentStates_t ascentState = ASCENT_NONE;
	static uint8_t ascentStableCnt = 0;
	uint32_t tickPressureDiff = 0;
	uint32_t lasttick = HAL_GetTick();
	float localAscentRate = 0.0;

	tickPressureDiff = time_elapsed_ms(lastPressureTick,lasttick); /* Calculate ascent rate every 400ms use timer to take care for small time shifts */
	if(tickPressureDiff != 0)
	{
		if(lastPressure_bar >= 0)
		{
			localAscentRate = (lastPressure_bar - global.lifeData.pressure_ambient_bar)  * (60000.0 / tickPressureDiff) * 10; /* bar * 10 = meter */
			if((fabs(localAscentRate) < 1.0) || (global.lifeData.pressure_ambient_bar < START_DIVE_IMMEDIATLY_BAR))
			{
				ascentState = ASCENT_NONE;
				ascentStableCnt = 0;
			}
			else if(localAscentRate > 0.0)
			{
				if(ascentState != ASCENT_FALLING)
				{
					if(ascentStableCnt < 5)
					{
						ascentStableCnt++;
					}
					else
					{
						ascentState = ASCENT_RISING;
					}
				}
				else
				{
					ascentState = ASCENT_NONE;
					ascentStableCnt = 0;
				}
			}
			else	/* must be falling */
			{
				if(ascentState != ASCENT_RISING)
				{
					if(ascentStableCnt < 5)
					{
						ascentStableCnt++;
					}
					else
					{
							ascentState = ASCENT_FALLING;
					}
				}
				else
				{
					ascentState = ASCENT_NONE;
					ascentStableCnt = 0;
				}
			}
			if(ascentState != ASCENT_NONE)
			{
				global.lifeData.ascent_rate_meter_per_min = localAscentRate;
			}
			else
			{
				global.lifeData.ascent_rate_meter_per_min = 0;
			}
		}
	}
	lastPressure_bar = global.lifeData.pressure_ambient_bar;
	lastPressureTick = lasttick;
}

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/

