/**
  ******************************************************************************
  * @file    data_exchange_main.c
  * @author  heinrichs weikamp gmbh
  * @date    13-Oct-2014
  * @version V0.0.3
  * @since   17-Feb-2016

	* @brief   Communication with the second CPU == RTE system
  *
  @verbatim
  ==============================================================================
                        ##### Version Changes #####
  ==============================================================================
	160217 V0.0.3 pStateUsed->decolistXXXXX.tickstamp = HAL_GetTick(); added
	150627 V0.0.2 
	
  ==============================================================================
                        ##### How to use #####
  ==============================================================================

  ==============================================================================
            ##### Button, Set Time, Clear Deco etc Request #####
  ==============================================================================
	was updated (151207) for buttons and clear deco at the moment only
	using requestNecessary and checking in DataEX_copy_to_LifeData()
	Hence if there is no confirm from the smallCPU on the data after the request
	the request will be send again.
	
	==============================================================================
                        ##### Device Data #####
  ==============================================================================
	
	main CPU always sends the device data info that it has at the moment

		on start it is INT32_MIN, INT32_MAX and 0 
		as initialized  in data_central.c variable declaration
	
	second small CPU gets request to send its device data
		
		on reception the data is merged with the data in externLogbookFlash,
		stored on the externLogbookFlash and from now on send to small CPU

  ==============================================================================
            ##### Magnet Reset #####
  ==============================================================================

	@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h> // for memcpy
#include <math.h>
#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "ostc.h"
#include "settings.h"
#include "data_central.h"
#include "data_exchange_main.h"
#include "base.h"
#include "decom.h"
#include "calc_crush.h" /* for vpm_init */
#include "simulation.h"
#include "tCCR.h"
#include "timer.h"
#include "buehlmann.h"
#include "externLogbookFlash.h"

/* #define TESTBENCH */

/* Exported variables --------------------------------------------------------*/
static uint8_t	wasPowerOn = 0;
static confirmbit8_Type requestNecessary = { .uw = 0 };
static uint8_t wasUpdateNotPowerOn = 0;

/* Private variables with external access ------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t	told_reset_logik_alles_ok = 0;
static hw_Info_t hw_Info;

static SDataReceiveFromMaster dataOut;
static SDataExchangeSlaveToMaster dataIn;

static uint8_t data_old__lost_connection_to_slave_counter_temp = 0;
static uint8_t data_old__lost_connection_to_slave_counter_retry = 0;
static uint32_t data_old__lost_connection_to_slave_counter_total = 0;

static uint8_t DeviceDataUpdated = 0;

static uint16_t externalInterface_Cmd = 0;

/* Private types -------------------------------------------------------------*/
#define UNKNOWN_TIME_HOURS		1
#define UNKNOWN_TIME_MINUTES	0
#define UNKNOWN_TIME_SECOND		0
#define UNKNOWN_DATE_DAY		1
#define UNKNOWN_DATE_MONTH		1
#define UNKNOWN_DATE_YEAR		16


/* Private function prototypes -----------------------------------------------*/
static uint8_t DataEX_check_header_and_footer_ok(void);
static uint8_t DataEX_check_header_and_footer_shifted(void);
static uint8_t DataEX_check_header_and_footer_devicedata(void);
static void DataEX_check_DeviceData(void);

/* Exported functions --------------------------------------------------------*/
uint8_t DataEX_was_power_on(void)
{
	return wasPowerOn;
}

static uint8_t count_DataEX_Error_Handler = 0;
static uint8_t last_error_DataEX_Error_Handler = 0;

static void DataEX_Error_Handler(uint8_t answer)
{
	count_DataEX_Error_Handler++;
	last_error_DataEX_Error_Handler = answer;

	/* A wrong footer indicates a communication interrupt. State machine is waiting for new data which is not received because no new transmission is triggered */
	/* ==> Abort data exchange to enable a new RX / TX cycle */
	if(answer == HAL_BUSY)
	{
		HAL_SPI_Abort_IT(&cpu2DmaSpi);
	}

  return;
}


uint32_t DataEX_lost_connection_count(void)
{
	return data_old__lost_connection_to_slave_counter_total; 
}


SDataReceiveFromMaster *dataOutGetPointer(void)
{
	return &dataOut;
}

void DataEX_init(void)
{
	SDiveState * pStateReal = stateRealGetPointerWrite();
	pStateReal->data_old__lost_connection_to_slave = 0; //initial value
	data_old__lost_connection_to_slave_counter_temp = 0;
	data_old__lost_connection_to_slave_counter_total = 0;
	DeviceDataUpdated = 0;

	memset((void *)&dataOut, 0, sizeof(SDataReceiveFromMaster));

	dataOut.header.checkCode[0] = 0xBB;
	dataOut.header.checkCode[1] = SPI_RX_STATE_OK;
	dataOut.header.checkCode[2] = SPI_RX_STATE_OK;
	dataOut.header.checkCode[3] = 0xBB;

	dataOut.footer.checkCode[0] = 0xF4;
	dataOut.footer.checkCode[1] = 0xF3;
	dataOut.footer.checkCode[2] = 0xF2;
	dataOut.footer.checkCode[3] = 0xF1;
}


static void DataEx_call_helper_requests(void)
{
	static uint8_t setDateWasSend = 0;
	static uint8_t setTimeWasSend = 0;
	static uint8_t calibrateCompassWasSend = 0;
	static uint8_t setButtonSensitivityWasSend = 0;
	static uint8_t clearDecoWasSend = 0;
	static uint8_t getDeviceDataWasSend = 0;	
	static uint8_t setAccidentFlagWasSend = 0;
	static uint8_t setEndDiveWasSend = 0;
	
	if(getDeviceDataWasSend)
	{
		dataOut.getDeviceDataNow = 0;
	}
	getDeviceDataWasSend = 0;
	if(dataOut.getDeviceDataNow)
	{
		getDeviceDataWasSend = 1;
		requestNecessary.ub.devicedata = 1;
	}
	
	if(setEndDiveWasSend)
	{
		dataOut.setEndDive = 0;
		//requestNecessary.ub.XXX = 1; not implemented and no space here
	}
	setEndDiveWasSend = 0;
	if(dataOut.setEndDive)
	{
		setEndDiveWasSend = 1;
	}
	
	if(setAccidentFlagWasSend)
	{
		dataOut.setAccidentFlag = 0;
	}
	setAccidentFlagWasSend = 0;
	if(dataOut.setAccidentFlag)
	{
		setAccidentFlagWasSend = 1;
		requestNecessary.ub.accident = 1;
	}

	if(setDateWasSend)
	{
		dataOut.setDateNow = 0;
	}
	setDateWasSend = 0;
	if(dataOut.setDateNow)
	{
		setDateWasSend = 1;
		requestNecessary.ub.date = 1;
	}

	if(setTimeWasSend)
	{
		dataOut.setTimeNow = 0;
	}
	setTimeWasSend = 0;
	if(dataOut.setTimeNow)
	{
		setTimeWasSend = 1;
		requestNecessary.ub.time = 1;
	}

	if(calibrateCompassWasSend)
	{
		dataOut.calibrateCompassNow = 0;
	}
	calibrateCompassWasSend = 0;
	if(dataOut.calibrateCompassNow)
	{
		calibrateCompassWasSend = 1;
	}

	if(clearDecoWasSend)
	{
		dataOut.clearDecoNow = 0;
		requestNecessary.ub.compass = 1;
	}
	if(dataOut.clearDecoNow)
	{
		clearDecoWasSend = 1;
		requestNecessary.ub.clearDeco = 1;
	}
	
	if(setButtonSensitivityWasSend)
	{
		dataOut.setButtonSensitivityNow = 0;
	}
	setButtonSensitivityWasSend = 0;
	if(dataOut.setButtonSensitivityNow)
	{
		setButtonSensitivityWasSend = 1;
		requestNecessary.ub.button = 1;
	}
}


uint8_t DataEX_call(void)
{
	static uint32_t RTEOfflineCnt = 0;
	static uint8_t SusppressCom = 0;

	uint8_t SPI_DMA_answer = 0;

	if(SusppressCom)
	{
		SusppressCom--;
	}
	else
	{
		if(data_old__lost_connection_to_slave_counter_temp >= 2) /* error reaction is triggered whenever communication could not be reestablished within two cycles */
		{
			data_old__lost_connection_to_slave_counter_temp = 0;
			if(DataEX_check_header_and_footer_shifted())
			{
				if(RTEOfflineCnt > 1)		/* RTE restarted communication after a longer silent time => restart error handling to recover */
				{
					data_old__lost_connection_to_slave_counter_retry = 0;
					RTEOfflineCnt = 0;
				}

				/* We received shifted data. Step one. Reset DMA to see if the problem is located at main */
				if (data_old__lost_connection_to_slave_counter_retry == 0)
				{
					HAL_SPI_Abort_IT(&cpu2DmaSpi);
				}
				/* reset of own DMA does not work ==> request reset of slave dma by indicating shifted reception */
				if (data_old__lost_connection_to_slave_counter_retry == 1)
				{
					dataOut.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_SHIFTED;
				}

				/* stop communication with RTE to trigger RTE timeout reaction */
				if (data_old__lost_connection_to_slave_counter_retry == 2)
				{
					SusppressCom = 3;
				}

				data_old__lost_connection_to_slave_counter_retry++;
			}
			else
			{
				RTEOfflineCnt++;	/* based on footer status the RTE does not seem to provide data in time */
				dataOut.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_OFFLINE;
			}
		}
	#if USE_OLD_SYNC_METHOD
		/* one cycle with NotChipSelect true to clear slave spi buffer */
		else
		{
			HAL_GPIO_WritePin(SMALLCPU_CSB_GPIO_PORT,SMALLCPU_CSB_PIN,GPIO_PIN_RESET);
		}
	#endif

		DataEx_call_helper_requests();

	//HAL_GPIO_WritePin(OSCILLOSCOPE2_GPIO_PORT,OSCILLOSCOPE2_PIN,GPIO_PIN_RESET); /* only for testing with Oscilloscope */

		if(SusppressCom == 0)
		{
			HAL_GPIO_WritePin(SMALLCPU_CSB_GPIO_PORT,SMALLCPU_CSB_PIN,GPIO_PIN_RESET);

			SPI_DMA_answer = HAL_SPI_TransmitReceive_DMA(&cpu2DmaSpi, (uint8_t *)&dataOut, (uint8_t *)&dataIn, EXCHANGE_BUFFERSIZE);
			if(SPI_DMA_answer != HAL_OK)
			{
				DataEX_Error_Handler(SPI_DMA_answer);
			}
		}
	}
//	HAL_GPIO_WritePin(SMALLCPU_CSB_GPIO_PORT,SMALLCPU_CSB_PIN,GPIO_PIN_SET);
//HAL_Delay(3);
//HAL_GPIO_WritePin(OSCILLOSCOPE2_GPIO_PORT,OSCILLOSCOPE2_PIN,GPIO_PIN_SET); /* only for testing with Oscilloscope */

	return 1;
}


static uint32_t SPI_CALLBACKS;
uint32_t get_num_SPI_CALLBACKS(void){
	return SPI_CALLBACKS;
}

SDataExchangeSlaveToMaster* get_dataInPointer(void){
	return &dataIn;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == &cpu2DmaSpi)
	{
		HAL_GPIO_WritePin(SMALLCPU_CSB_GPIO_PORT,SMALLCPU_CSB_PIN,GPIO_PIN_SET);
		SPI_CALLBACKS+=1;
	}
}


void DateEx_copy_to_dataOut(void)
{
	const SDiveState * pStateReal = stateRealGetPointer();
	SSettings *settings = settingsGetPointer();

	uint8_t SensorActive[SENSOR_END];
	uint8_t index = 0;


	if(get_globalState() == StStop)
		dataOut.mode = MODE_SHUTDOWN;
	else
		dataOut.mode = 0;

	dataOut.diveModeInfo = pStateReal->diveSettings.diveMode; // hw 170215
	
	memcpy(&dataOut.data.DeviceData, stateDeviceGetPointer(), sizeof(SDevice));

	dataOut.data.VPMconservatism = pStateReal->diveSettings.vpm_conservatism;
	dataOut.data.actualGas = pStateReal->lifeData.actualGas;
	dataOut.data.ambient_pressure_mbar_ceiling = (pStateReal->decolistBuehlmann.output_ceiling_meter * 100) + (pStateReal->lifeData.pressure_surface_bar * 1000);
	dataOut.data.divetimeToCreateLogbook = settings->divetimeToCreateLogbook;
	dataOut.data.timeoutDiveReachedZeroDepth = settings->timeoutDiveReachedZeroDepth;
	
	dataOut.data.offsetPressureSensor_mbar = settings->offsetPressure_mbar;
	dataOut.data.offsetTemperatureSensor_centiDegree = settings->offsetTemperature_centigrad;


	memcpy(dataOut.data.externalInterface_SensorMap, settings->ext_sensor_map, EXT_INTERFACE_SENSOR_CNT);

	memset(SensorActive, 0, sizeof(SensorActive));
	for (index = 0; index < EXT_INTERFACE_SENSOR_CNT; index++)
	{
		switch(settings->ext_sensor_map[index])
		{
			case SENSOR_ANALOG:	SensorActive[SENSOR_ANALOG] = 1;
				break;
			case SENSOR_DIGO2M:	SensorActive[SENSOR_DIGO2] = 1;
				break;
			case SENSOR_CO2:	SensorActive[SENSOR_CO2] = 1;
				break;
#ifdef ENABLE_SENTINEL_MODE
			case SENSOR_SENTINEL:	SensorActive[SENSOR_SENTINEL] = 1;
				break;
#endif
			default:
				break;
		}
	}

	if(SensorActive[SENSOR_ANALOG])
	{
		externalInterface_Cmd |= EXT_INTERFACE_ADC_ON | EXT_INTERFACE_33V_ON;
	}
	if((SensorActive[SENSOR_DIGO2]) || (SensorActive[SENSOR_CO2]))
	{
		externalInterface_Cmd |= EXT_INTERFACE_33V_ON;
	}

#ifdef ENABLE_SENTINEL_MODE
	if(SensorActive[SENSOR_SENTINEL])
	{
			externalInterface_Cmd |= EXT_INTERFACE_33V_ON;
			externalInterface_Cmd &= (~EXT_INTERFACE_ADC_ON);
	}
#endif

	dataOut.data.externalInterface_Cmd = externalInterface_Cmd;
	externalInterface_Cmd = 0;

	if((hardwareDataGetPointer()->primarySerial <= 32) || (((hardwareDataGetPointer()->primarySerial == 72) && (hardwareDataGetPointer()->secondarySerial == 15))))
	{
		dataOut.revisionHardware = 0x00;
		dataOut.revisionCRCx0x7A = 0x7A;
	}
	else
	if(hardwareDataGetPointer()->primarySerial < 0xFFFF)
	{
		dataOut.revisionHardware = hardwareDataGetPointer()->revision8bit;
		dataOut.revisionCRCx0x7A = hardwareDataGetPointer()->revision8bit ^ 0x7A;
	}
	else
	{
		dataOut.revisionHardware = 0xFF;
		dataOut.revisionCRCx0x7A = 0xFF;
	}
	
	if(DataEX_check_header_and_footer_ok() && !told_reset_logik_alles_ok)
	{
		MX_tell_reset_logik_alles_ok();
		told_reset_logik_alles_ok = 1;
	}

	if(DataEX_check_header_and_footer_ok() && (dataIn.power_on_reset == 1))
	{
		if(!wasUpdateNotPowerOn)
			wasPowerOn = 1;
		
		settingsHelperButtonSens_keepPercentageValues(settingsGetPointerStandard()->ButtonResponsiveness[3], settings->ButtonResponsiveness);
		setButtonResponsiveness(settings->ButtonResponsiveness);
		DataEX_setExtInterface_Cmd(EXT_INTERFACE_COPY_SENSORMAP);
	}
}


void DataEX_copy_to_deco(void)
{
	if(decoLock == DECO_CALC_running)
        return;

	if(decoLock == DECO_CALC_init_as_is_start_of_dive)
	{
	    vpm_init(&stateUsedWrite->vpm,  stateUsedWrite->diveSettings.vpm_conservatism, 0, 0);
	    buehlmann_init();
	    timer_init();
	    resetEvents(stateUsedWrite);
	    stateUsedWrite->diveSettings.internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero = 0;
	}

	if(decoLock == DECO_CALC_FINSHED_Buehlmann)
    {

    }
    switch(decoLock)
    {

    //Deco_calculation finished
    case DECO_CALC_FINSHED_vpm:
        memcpy(&stateUsedWrite->decolistVPM,&stateDeco.decolistVPM,sizeof(SDecoinfo));
        stateUsedWrite->decolistVPM.tickstamp = HAL_GetTick();
        stateUsedWrite->vpm.deco_zone_reached = stateDeco.vpm.deco_zone_reached;
        for(int i = 0; i< 16; i++)
        {
        	stateUsedWrite->vpm.adjusted_critical_radius_he[i] = stateDeco.vpm.adjusted_critical_radius_he[i];
        	stateUsedWrite->vpm.adjusted_critical_radius_n2[i] = stateDeco.vpm.adjusted_critical_radius_n2[i];
        	stateUsedWrite->vpm.adjusted_crushing_pressure_he[i] = stateDeco.vpm.adjusted_crushing_pressure_he[i];
        	stateUsedWrite->vpm.adjusted_crushing_pressure_n2[i] = stateDeco.vpm.adjusted_crushing_pressure_n2[i];
        	stateUsedWrite->vpm.initial_allowable_gradient_he[i] = stateDeco.vpm.initial_allowable_gradient_he[i];
        	stateUsedWrite->vpm.initial_allowable_gradient_n2[i] = stateDeco.vpm.initial_allowable_gradient_n2[i];
        	stateUsedWrite->vpm.max_actual_gradient[i] = stateDeco.vpm.max_actual_gradient[i];
        }
        break;
    case DECO_CALC_FINSHED_Buehlmann:
        memcpy(&stateUsedWrite->decolistBuehlmann,&stateDeco.decolistBuehlmann,sizeof(SDecoinfo));
        stateUsedWrite->decolistBuehlmann.tickstamp = HAL_GetTick();
        //Copy Data to be stored if regular Buehlmann, not FutureBuehlmann
        stateUsedWrite->diveSettings.internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero = stateDeco.diveSettings.internal__pressure_first_stop_ambient_bar_as_upper_limit_for_gf_low_otherwise_zero;
        break;
    case DECO_CALC_FINSHED_FutureBuehlmann:
        memcpy(&stateUsedWrite->decolistFutureBuehlmann,&stateDeco.decolistFutureBuehlmann,sizeof(SDecoinfo));
        stateUsedWrite->decolistFutureBuehlmann.tickstamp = HAL_GetTick();
        break;
    case DECO_CALC_FINSHED_Futurevpm:
        memcpy(&stateUsedWrite->decolistFutureVPM,&stateDeco.decolistFutureVPM,sizeof(SDecoinfo));
        stateUsedWrite->decolistFutureVPM.tickstamp = HAL_GetTick();
        break;
    }

    //Copy Inputdata from stateReal to stateDeco
    memcpy(&stateDeco.lifeData,&stateUsedWrite->lifeData,sizeof(SLifeData));
    memcpy(&stateDeco.diveSettings,&stateUsedWrite->diveSettings,sizeof(SDiveSettings));
    memcpy(&stateDeco.decolistVPM,&stateUsedWrite->decolistVPM,sizeof(SDecoinfo));

    stateDeco.vpm.deco_zone_reached = stateUsedWrite->vpm.deco_zone_reached;
    // memcpy(&stateDeco.vpm,&pStateUsed->vpm,sizeof(SVpm));
    for(int i = 0; i< 16; i++)
    {
        stateDeco.vpm.max_crushing_pressure_he[i] = stateUsedWrite->vpm.max_crushing_pressure_he[i];
        stateDeco.vpm.max_crushing_pressure_n2[i] = stateUsedWrite->vpm.max_crushing_pressure_n2[i];
        stateDeco.vpm.adjusted_critical_radius_he[i] = stateUsedWrite->vpm.adjusted_critical_radius_he[i];
        stateDeco.vpm.adjusted_critical_radius_n2[i] = stateUsedWrite->vpm.adjusted_critical_radius_n2[i];
    }
	decoLock = DECO_CALC_ready;
}


static void DataEX_helper_copy_deviceData(SDeviceLine *lineWrite, const SDeviceLine *lineRead)
{	
	lineWrite->date_rtc_dr = lineRead->date_rtc_dr; 
	lineWrite->time_rtc_tr = lineRead->time_rtc_tr;
	lineWrite->value_int32 = lineRead->value_int32;
}

static void DataEX_helper_SetTime(RTC_TimeTypeDef inStimestructure, uint32_t *outTimetmpreg)
{
  inStimestructure.TimeFormat = RTC_HOURFORMAT_24;

	*outTimetmpreg = 		(uint32_t)(((uint32_t)RTC_ByteToBcd2(inStimestructure.Hours) << 16U) | \
											((uint32_t)RTC_ByteToBcd2(inStimestructure.Minutes) << 8U) | \
											((uint32_t)RTC_ByteToBcd2(inStimestructure.Seconds)) | \
											(((uint32_t)inStimestructure.TimeFormat) << 16U));  
}


static void DataEX_helper_SetDate(RTC_DateTypeDef inSdatestructure, uint32_t *outDatetmpreg)
{
   *outDatetmpreg = (((uint32_t)RTC_ByteToBcd2(inSdatestructure.Year) << 16U) | \
										((uint32_t)RTC_ByteToBcd2(inSdatestructure.Month) << 8U) | \
										((uint32_t)RTC_ByteToBcd2(inSdatestructure.Date)) | \
										((uint32_t)inSdatestructure.WeekDay << 13U));   
}


static void DataEX_helper_set_Unknown_Date_deviceData(SDeviceLine *lineWrite)
{	
	RTC_DateTypeDef sdatestructure;
	RTC_TimeTypeDef stimestructure;

	stimestructure.Hours = UNKNOWN_TIME_HOURS;
	stimestructure.Minutes = UNKNOWN_TIME_MINUTES;
	stimestructure.Seconds = UNKNOWN_TIME_SECOND;

	sdatestructure.Date = UNKNOWN_DATE_DAY;
	sdatestructure.Month = UNKNOWN_DATE_MONTH;
	sdatestructure.Year = UNKNOWN_DATE_YEAR;
	setWeekday(&sdatestructure);

	DataEX_helper_SetTime(stimestructure, &lineWrite->time_rtc_tr);
	DataEX_helper_SetDate(sdatestructure, &lineWrite->date_rtc_dr);
}


static uint8_t DataEX_helper_Check_And_Correct_Date_deviceData(SDeviceLine *lineWrite)
{
	uint8_t retval = 0;
	RTC_DateTypeDef sdatestructure;
	RTC_TimeTypeDef stimestructure;

	// from lineWrite to structure
	translateDate(lineWrite->date_rtc_dr, &sdatestructure);
	translateTime(lineWrite->time_rtc_tr, &stimestructure);

	/* Check if date is out of range */
	if(!(	(sdatestructure.Year >= 15)
			&& (sdatestructure.Year <= 30)
			&& (sdatestructure.Month <= 12)))
	{
		DataEX_helper_set_Unknown_Date_deviceData(lineWrite);
		retval = 1;
	}
	return retval;
}


static uint8_t DataEX_helper_Check_And_Correct_Value_deviceData(SDeviceLine *lineWrite, int32_t from, int32_t to, uint8_t defaulttofrom)
{
	uint8_t retval = 0;
	RTC_DateTypeDef sdatestructure;

	/* Is value out of valid range? */
	if(!(lineWrite->value_int32 >= from && lineWrite->value_int32 <= to))
	{
		if(defaulttofrom)
		{
			lineWrite->value_int32 = from;
		}
		else
		{
			lineWrite->value_int32 = to;
		}
		DataEX_helper_set_Unknown_Date_deviceData(lineWrite);
	}

	/* This is just a repair function to restore metric if a corruption occurred in an older fw version */
	if(((lineWrite->value_int32 == to) && defaulttofrom )
		|| ((lineWrite->value_int32 == from) && !defaulttofrom ))
	{
		translateDate(lineWrite->date_rtc_dr, &sdatestructure);
		if(sdatestructure.Year == UNKNOWN_DATE_YEAR)
		{
			if(defaulttofrom)
			{
				lineWrite->value_int32 = from;
			}
			else
			{
				lineWrite->value_int32 = to;
			}
		}
	}
	return retval;
}


static void DataEX_check_DeviceData(void)
{
	SDevice *DeviceData = stateDeviceGetPointerWrite();

	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->batteryChargeCompleteCycles);
	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->batteryChargeCycles);
	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->depthMaximum);
	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->diveCycles);
	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->hoursOfOperation);
	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->temperatureMaximum);
	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->temperatureMinimum);
	DataEX_helper_Check_And_Correct_Date_deviceData(&DeviceData->voltageMinimum);

	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->batteryChargeCompleteCycles, 0, 10000,1);
	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->batteryChargeCycles, 0, 20000,1);
	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->depthMaximum, 0, (500*100)+1000,1);
	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->diveCycles, 0, 20000,1);
	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->hoursOfOperation, 0, 1000000,1);
	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->temperatureMaximum, -30*100, 150*100,1);
	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->temperatureMinimum, -30*100, 150*100,0);
	DataEX_helper_Check_And_Correct_Value_deviceData(&DeviceData->voltageMinimum, 2*1000, 6*1000,0);
}


static void DataEX_merge_DeviceData_and_store(void)
{
	uint16_t dataLengthRead;
	SDevice DeviceDataFlash;
	SDevice *DeviceData = stateDeviceGetPointerWrite();

	dataLengthRead = ext_flash_read_devicedata((uint8_t *)&DeviceDataFlash,sizeof(SDevice));

	if(dataLengthRead == 0)
	{
		ext_flash_write_devicedata(false);
		return;
	}

	/* max values */
	if(DeviceData->batteryChargeCompleteCycles.value_int32 < DeviceDataFlash.batteryChargeCompleteCycles.value_int32)
	{
		DataEX_helper_copy_deviceData(&DeviceData->batteryChargeCompleteCycles, &DeviceDataFlash.batteryChargeCompleteCycles);
	}
	if(DeviceData->batteryChargeCycles.value_int32 < DeviceDataFlash.batteryChargeCycles.value_int32)
	{
		DataEX_helper_copy_deviceData(&DeviceData->batteryChargeCycles, &DeviceDataFlash.batteryChargeCycles);
	}
	if(DeviceData->temperatureMaximum.value_int32 < DeviceDataFlash.temperatureMaximum.value_int32)
	{
		DataEX_helper_copy_deviceData(&DeviceData->temperatureMaximum, &DeviceDataFlash.temperatureMaximum);
	}
	if(DeviceData->depthMaximum.value_int32 < DeviceDataFlash.depthMaximum.value_int32)
	{
		DataEX_helper_copy_deviceData(&DeviceData->depthMaximum, &DeviceDataFlash.depthMaximum);
	}
	if(DeviceData->diveCycles.value_int32 < DeviceDataFlash.diveCycles.value_int32)
	{
		DataEX_helper_copy_deviceData(&DeviceData->diveCycles, &DeviceDataFlash.diveCycles);
	}
	if(DeviceData->hoursOfOperation.value_int32 < DeviceDataFlash.hoursOfOperation.value_int32)
	{
		DataEX_helper_copy_deviceData(&DeviceData->hoursOfOperation, &DeviceDataFlash.hoursOfOperation);
	}
	

	/* min values */
	if(DeviceData->temperatureMinimum.value_int32 > DeviceDataFlash.temperatureMinimum.value_int32)
	{
		DataEX_helper_copy_deviceData(&DeviceData->temperatureMinimum, &DeviceDataFlash.temperatureMinimum);
	}
	if(DeviceData->voltageMinimum.value_int32 > DeviceDataFlash.voltageMinimum.value_int32)
	{
			DataEX_helper_copy_deviceData(&DeviceData->voltageMinimum, &DeviceDataFlash.voltageMinimum);
	}
	
	DataEX_check_DeviceData	();
	ext_flash_write_devicedata(false);
}


static void DataEX_copy_to_DeviceData(void)
{
	SDataExchangeSlaveToMasterDeviceData * dataInDevice = (SDataExchangeSlaveToMasterDeviceData *)&dataIn;
	SDevice * pDeviceState = stateDeviceGetPointerWrite();

	memcpy(pDeviceState, &dataInDevice->DeviceData[dataInDevice->boolDeviceData], sizeof(SDevice));
	memcpy(&hw_Info, &dataInDevice->hw_Info, sizeof(dataInDevice->hw_Info));

	DeviceDataUpdated = 1;	/* indicate new data to be written to flash by background task (at last op hour count will be updated) */
}


static void DataEX_copy_to_VpmRepetitiveData(void)
{
	SDataExchangeSlaveToMasterDeviceData * dataInDevice = (SDataExchangeSlaveToMasterDeviceData *)&dataIn;
	SVpmRepetitiveData * pVpmState = stateVpmRepetitiveDataGetPointerWrite();

	if(dataInDevice->boolVpmRepetitiveDataValid)
	{
		memcpy(pVpmState, &dataInDevice->VpmRepetitiveData, sizeof(SVpmRepetitiveData));
		pVpmState->is_data_from_RTE_CPU = 1;
	}
}


void DataEX_control_connection_while_asking_for_sleep(void)
{
 	if(!DataEX_check_header_and_footer_ok())
	{
		if(DataEX_check_header_and_footer_devicedata())
		{
			data_old__lost_connection_to_slave_counter_retry = 0;
			data_old__lost_connection_to_slave_counter_temp = 0;
			stateRealGetPointerWrite()->data_old__lost_connection_to_slave = 0;
			dataOut.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_OK;
		}
		else
		{
			stateRealGetPointerWrite()->data_old__lost_connection_to_slave = 1;
			data_old__lost_connection_to_slave_counter_temp += 1;
			data_old__lost_connection_to_slave_counter_total += 1;
		}
	}
}

static float getSampleDepth(SDataExchangeSlaveToMaster *d, SDiveState *ds)
{
	float ambient = 0;
	float surface = 0;
	float depth = 0;
	float density = 0;

	ambient = d->data[d->boolPressureData].pressure_mbar / 1000.0f;
	surface = d->data[d->boolPressureData].surface_mbar / 1000.0f;
	density = ((float)( 100 + settingsGetPointer()->salinity)) / 100.0f;

#ifdef TESTBENCH
	/* do plausibility check (typically only needed at debug hardware) */
	if(ambient < 0)
	{
		ambient = 1.0;
	}
	if(surface < 0)
	{
		surface = 1.0;
	}
#endif
	ds->lifeData.pressure_ambient_bar = ambient;
	ds->lifeData.pressure_surface_bar = surface;
	depth = (ambient - surface) / (0.09807f * density);
	ds->lifeData.bool_temp1 = d->data[d->boolPressureData].SPARE1;

	return depth;
}

static float getTemperature(SDataExchangeSlaveToMaster *d)
{
	float temp = 0;
	temp = d->data[d->boolPressureData].temperature;

#ifdef TESTBENCH
	/* do plausibility check (typically only needed at debug hardware */
	if(temp < -40.0)
	{
		temp = 20.0;
	}
#endif
	return temp;
}

void DataEX_copy_to_LifeData(_Bool *modeChangeFlag)
{
	static uint16_t getDeviceDataAfterStartOfMainCPU = 20;

	SDiveState *pStateReal = stateRealGetPointerWrite();
	uint8_t idx;
	float meter = 0;
	SSettings *pSettings;
	
#ifdef ENABLE_EXTERNAL_PRESSURE
    float CO2Corr = 0.0;
#endif

	// wireless - �ltere daten aufr�umen
#if 0
	for(int i=0;i<(2*NUM_GASES+1);i++)
	{
		if(pStateReal->lifeData.bottle_bar[i])
		{
			if((pStateReal->lifeData.bottle_bar_age_MilliSeconds[i] == 0) || (pStateReal->lifeData.bottle_bar_age_MilliSeconds[i] > 60000))
			{
				pStateReal->lifeData.bottle_bar_age_MilliSeconds[i] =  0;
				pStateReal->lifeData.bottle_bar[i] = 0;
			}
			else
				pStateReal->lifeData.bottle_bar_age_MilliSeconds[i] +=  100;
		}
	}
#else
   	if(stateRealGetPointer()->lifeData.bottle_bar_age_MilliSeconds[stateRealGetPointer()->lifeData.actualGas.GasIdInSettings] < 6000)  /* max age after ten minutes */
   	{
   		stateRealGetPointerWrite()->lifeData.bottle_bar_age_MilliSeconds[stateRealGetPointer()->lifeData.actualGas.GasIdInSettings]++;
   	}
#endif
	if(!DataEX_check_header_and_footer_ok())
	{
		if(DataEX_check_header_and_footer_devicedata())
		{
			DataEX_copy_to_DeviceData();
			DataEX_copy_to_VpmRepetitiveData();
			data_old__lost_connection_to_slave_counter_temp = 0;
			data_old__lost_connection_to_slave_counter_retry = 0;
			/* Do not yet reset state. Wait till common data has been received in next cycle. Otherwise invalid data may be forwarded for processing */
			/* pStateReal->data_old__lost_connection_to_slave = 0; */
			dataOut.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_OK;
		}
		else
		{
			pStateReal->data_old__lost_connection_to_slave = 1;
			data_old__lost_connection_to_slave_counter_temp += 1;
			data_old__lost_connection_to_slave_counter_total += 1;
		}
		return;
	}
	else /* RX data OK */
	{
		data_old__lost_connection_to_slave_counter_temp = 0;
		data_old__lost_connection_to_slave_counter_retry = 0;
		pStateReal->data_old__lost_connection_to_slave = 0;
		dataOut.header.checkCode[SPI_HEADER_INDEX_RX_STATE] = SPI_RX_STATE_OK;
	}
	
	if(getDeviceDataAfterStartOfMainCPU)
	{
		getDeviceDataAfterStartOfMainCPU--;
		if(getDeviceDataAfterStartOfMainCPU == 0)
		{
			dataOut.getDeviceDataNow = 1;
			getDeviceDataAfterStartOfMainCPU = 10*60*10; /* * 100ms = 60 second => update device data every 10 minutes */
		}
	}

	if((requestNecessary.uw != 0) && (dataIn.confirmRequest.uw != 0))
	{
		if(((dataIn.confirmRequest.uw) & CRBUTTON) != 0)
		{
			requestNecessary.ub.button = 0;
		}
		if(((dataIn.confirmRequest.uw) & CRCLEARDECO) != 0)
		{
			requestNecessary.ub.clearDeco = 0;
		}
		if(((dataIn.confirmRequest.uw) & CRDATE) != 0)
		{
			requestNecessary.ub.date = 0;
		}
		if(((dataIn.confirmRequest.uw) & CRTIME) != 0)
		{
			requestNecessary.ub.time = 0;
		}
		if(((dataIn.confirmRequest.uw) & CRCOMPASS) != 0)
		{
			requestNecessary.ub.compass = 0;
		}
		if(((dataIn.confirmRequest.uw) & CRDEVICEDATA) != 0)
		{
			requestNecessary.ub.devicedata = 0;
		}
		if(((dataIn.confirmRequest.uw) & CRBATTERY) != 0)
		{
			requestNecessary.ub.batterygauge = 0;
		}
		if(((dataIn.confirmRequest.uw) & CRACCIDENT) != 0)
		{
			requestNecessary.ub.accident = 0;
		}

		if(requestNecessary.ub.button == 1)	/* send button values to RTE */
		{
			setButtonResponsiveness(settingsGetPointer()->ButtonResponsiveness);
		}
	}

	/*	uint8_t IAmStolenPleaseKillMe;
	 */
	pSettings = settingsGetPointer();

	if(pSettings->IAmStolenPleaseKillMe > 3)
	{
		pSettings->salinity = 0;
		dataIn.data[dataIn.boolPressureData].surface_mbar = 999;
		dataIn.data[dataIn.boolPressureData].pressure_mbar = 98971;
		dataIn.mode = MODE_DIVE;
	}


	/* internal sensor: HUD data	 */
	if(pSettings->ppo2sensors_source == O2_SENSOR_SOURCE_OPTIC)
	{
		pStateReal->lifeData.extIf_sensor_Id = 0;
		for(int i=0;i<3;i++)
		{
			pStateReal->lifeData.ppO2Sensor_bar[i] = get_ppO2Sensor_bar(i);
			pStateReal->lifeData.sensorVoltage_mV[i] = get_sensorVoltage_mV(i);
		}
		pStateReal->lifeData.HUD_battery_voltage_V = get_HUD_battery_voltage_V();
	}
	else /* use data from external ADC */
	{
		if(pStateReal->data_old__lost_connection_to_slave == 0)
		{
			for(idx = 0; idx < 3; idx++)
			{
				pStateReal->lifeData.sensorVoltage_mV[idx] = dataIn.data[(dataIn.boolADCO2Data && DATA_BUFFER_ADC)].extADC_voltage[idx];
				if(pStateReal->lifeData.sensorVoltage_mV[idx] < IGNORE_O2_VOLTAGE_LEVEL_MV)
				{
					pStateReal->lifeData.sensorVoltage_mV[idx] = 0.0;
					pStateReal->lifeData.ppO2Sensor_bar[idx] = 0;
				}
				else
				{
					if(dataIn.data[(dataIn.boolADCO2Data && DATA_BUFFER_ADC)].sensor_map[idx] == SENSOR_DIGO2M)
					{
						pStateReal->lifeData.ppO2Sensor_bar[idx] = pStateReal->lifeData.sensorVoltage_mV[idx] / 100.0;
					}
					else
					{
						pStateReal->lifeData.ppO2Sensor_bar[idx] = pStateReal->lifeData.sensorVoltage_mV[idx] * pSettings->ppo2sensors_calibCoeff[idx];
					}
				}
			}
		}
	}

	if(pStateReal->data_old__lost_connection_to_slave == 0)
	{

		pStateReal->lifeData.extIf_sensor_Id = dataIn.data[(dataIn.boolADCO2Data && DATA_BUFFER_ADC)].externalInterface_SensorID;
		if(pStateReal->lifeData.extIf_sensor_Id < 3)
		{

			memcpy(pStateReal->lifeData.extIf_sensor_data[pStateReal->lifeData.extIf_sensor_Id], dataIn.data[(dataIn.boolADCO2Data && DATA_BUFFER_ADC)].sensor_data, 32);
		}
		memcpy(pStateReal->lifeData.extIf_sensor_map, dataIn.data[(dataIn.boolADCO2Data && DATA_BUFFER_ADC)].sensor_map, EXT_INTERFACE_SENSOR_CNT);


		meter = getSampleDepth(&dataIn, pStateReal);

		pStateReal->pressure_uTick_old = pStateReal->pressure_uTick_new;
		pStateReal->pressure_uTick_new = dataIn.data[dataIn.boolPressureData].pressure_uTick;
		pStateReal->pressure_uTick_local_new = HAL_GetTick();
	
		pStateReal->lifeData.dateBinaryFormat = dataIn.data[dataIn.boolTimeData].localtime_rtc_dr;
		pStateReal->lifeData.timeBinaryFormat = dataIn.data[dataIn.boolTimeData].localtime_rtc_tr;

		pStateReal->lifeData.gnssPosition.Latitude = dataIn.data[0].fLat;
		pStateReal->lifeData.gnssPosition.Longitude = dataIn.data[0].fLon;
	}

	if(pStateReal->data_old__lost_connection_to_slave == 0)
	{
		//Start of diveMode?
		if(pStateReal->mode != MODE_DIVE && dataIn.mode == MODE_DIVE)
		{
			if(modeChangeFlag)
			{
				*modeChangeFlag = 1;
			}
			if(stateUsed == stateSimGetPointer())
			{
				simulation_exit();
			}
            disableTimer();

				// new 170508
			settingsGetPointer()->bluetoothActive = 0;
			MX_Bluetooth_PowerOff();
			//Init dive Mode
			decoLock = DECO_CALC_init_as_is_start_of_dive;
			pStateReal->lifeData.boolResetAverageDepth = 1;
		}

		pStateReal->lifeData.cns = dataIn.data[dataIn.boolToxicData].cns;
		pStateReal->lifeData.otu = dataIn.data[dataIn.boolToxicData].otu;
		pStateReal->lifeData.no_fly_time_minutes = dataIn.data[dataIn.boolToxicData].no_fly_time_minutes;
		pStateReal->lifeData.desaturation_time_minutes = dataIn.data[dataIn.boolToxicData].desaturation_time_minutes;

		//End of diveMode?
		if(pStateReal->mode == MODE_DIVE && dataIn.mode != MODE_DIVE)
		{
			if(modeChangeFlag)
			{
				*modeChangeFlag = 1;
			}
			createDiveSettings();

            disableTimer();

			if(pStateReal->warnings.cnsHigh)
			{
				if(pStateReal->lifeData.cns >= 130)
					dataOut.setAccidentFlag += ACCIDENT_CNSLVL2;
				else if(pStateReal->lifeData.cns >= 100)
					dataOut.setAccidentFlag += ACCIDENT_CNS;
			}
			if(pStateReal->warnings.decoMissed)
				dataOut.setAccidentFlag += ACCIDENT_DECOSTOP;
		}
		pStateReal->mode = dataIn.mode;
		pStateReal->chargeStatus = dataIn.chargeStatus;
	
		pStateReal->lifeData.depth_meter = meter;

		pStateReal->lifeData.temperature_celsius = getTemperature(&dataIn);
		pStateReal->lifeData.ascent_rate_meter_per_min = dataIn.data[dataIn.boolPressureData].ascent_rate_meter_per_min;
		if(pStateReal->mode != MODE_DIVE)
			pStateReal->lifeData.max_depth_meter = 0;
		else
		{
			if(meter > pStateReal->lifeData.max_depth_meter)
			  pStateReal->lifeData.max_depth_meter = meter;
		}

		if(requestNecessary.ub.clearDeco == 0)		/* No "reset deco" is send to RTE ? */
		{
			if(dataIn.accidentFlags & ACCIDENT_DECOSTOP)
			{
				pStateReal->decoMissed_at_the_end_of_dive = 1;
			}
			else
			{
				pStateReal->decoMissed_at_the_end_of_dive = 0;
			}
			if(dataIn.accidentFlags & ACCIDENT_CNS)
			{
				pStateReal->cnsHigh_at_the_end_of_dive = 1;
			}
			else
			{
				pStateReal->cnsHigh_at_the_end_of_dive = 0;
			}
		}
		pStateReal->lifeData.dive_time_seconds = (int32_t)dataIn.data[dataIn.boolTimeData].divetime_seconds;
		pStateReal->lifeData.dive_time_seconds_without_surface_time = (int32_t)dataIn.data[dataIn.boolTimeData].dive_time_seconds_without_surface_time;
		pStateReal->lifeData.counterSecondsShallowDepth = dataIn.data[dataIn.boolTimeData].counterSecondsShallowDepth;
		pStateReal->lifeData.surface_time_seconds = (int32_t)dataIn.data[dataIn.boolTimeData].surfacetime_seconds;

		pStateReal->lifeData.compass_heading = dataIn.data[dataIn.boolCompassData].compass_heading;
        if (pStateReal->lifeData.compass_heading != -1) {
            if (pSettings->FlipDisplay) { /* consider that diver is targeting into the opposite direction */
                pStateReal->lifeData.compass_heading -= 180.0;
            }

            if (pSettings->compassDeclinationDeg != 0) {
                pStateReal->lifeData.compass_heading = pStateReal->lifeData.compass_heading + pSettings->compassDeclinationDeg - 360.0;
            }

            while (pStateReal->lifeData.compass_heading < 0) {
                pStateReal->lifeData.compass_heading += 360.0;
            }
        }

		pStateReal->lifeData.compass_roll = dataIn.data[dataIn.boolCompassData].compass_roll;
		pStateReal->lifeData.compass_pitch = dataIn.data[dataIn.boolCompassData].compass_pitch;

		pStateReal->lifeData.compass_DX_f = dataIn.data[dataIn.boolCompassData].compass_DX_f;
		pStateReal->lifeData.compass_DY_f = dataIn.data[dataIn.boolCompassData].compass_DY_f;
		pStateReal->lifeData.compass_DZ_f = dataIn.data[dataIn.boolCompassData].compass_DZ_f;

		pStateReal->compass_uTick_old = pStateReal->compass_uTick_new;
		pStateReal->compass_uTick_new = dataIn.data[dataIn.boolCompassData].compass_uTick;
		pStateReal->compass_uTick_local_new = HAL_GetTick();
		compass_Inertia(pStateReal->lifeData.compass_heading);

		memcpy(pStateReal->lifeData.tissue_nitrogen_bar, dataIn.data[dataIn.boolTisssueData].tissue_nitrogen_bar,sizeof(pStateReal->lifeData.tissue_nitrogen_bar));
		memcpy(pStateReal->lifeData.tissue_helium_bar, dataIn.data[dataIn.boolTisssueData].tissue_helium_bar,sizeof(pStateReal->lifeData.tissue_helium_bar));
	
		if(pStateReal->mode == MODE_DIVE)
		{
			for(int i= 0; i <16; i++)
			{
				pStateReal->vpm.max_crushing_pressure_he[i] =  dataIn.data[dataIn.boolCrushingData].max_crushing_pressure_he[i];
				pStateReal->vpm.max_crushing_pressure_n2[i] = dataIn.data[dataIn.boolCrushingData].max_crushing_pressure_n2[i];
				pStateReal->vpm.adjusted_critical_radius_he[i] =  dataIn.data[dataIn.boolCrushingData].adjusted_critical_radius_he[i];
				pStateReal->vpm.adjusted_critical_radius_n2[i] = dataIn.data[dataIn.boolCrushingData].adjusted_critical_radius_n2[i];
			}
		}

		/* battery and ambient light sensors
		 */
		pStateReal->lifeData.ambient_light_level = dataIn.data[dataIn.boolAmbientLightData].ambient_light_level;
		pStateReal->lifeData.battery_charge = dataIn.data[dataIn.boolBatteryData].battery_charge;
		pStateReal->lifeData.battery_voltage = dataIn.data[dataIn.boolBatteryData].battery_voltage;

		/* PIC data
	 	 */
		for(int i=0;i<4;i++)
		{
			pStateReal->lifeData.buttonPICdata[i] = dataIn.data[dataIn.boolPICdata].button_setting[i];
		}

		/* sensorErrors
		 */
		pStateReal->sensorErrorsRTE = dataIn.sensorErrors;

		/* data from CO2 sensor */
		pStateReal->lifeData.CO2_data.CO2_ppm = dataIn.data[(dataIn.boolADCO2Data && DATA_BUFFER_CO2)].CO2_ppm;
		pStateReal->lifeData.CO2_data.signalStrength = dataIn.data[(dataIn.boolADCO2Data && DATA_BUFFER_CO2)].CO2_signalStrength;

#ifdef ENABLE_EXTERNAL_PRESSURE
		CO2Corr = 2.811*pow(10,-38)*pow(pStateReal->lifeData.CO2_data.CO2_ppm,6)- 9.817*pow(10,-32)*pow(pStateReal->lifeData.CO2_data.CO2_ppm,5)+1.304*pow(10,-25)*pow(pStateReal->lifeData.CO2_data.CO2_ppm,4)
				 -8.216*pow(10,-20)*pow(pStateReal->lifeData.CO2_data.CO2_ppm,3)+2.311*pow(10,-14)*pow(pStateReal->lifeData.CO2_data.CO2_ppm,2) - 2.195*pow(10,-9)*pStateReal->lifeData.CO2_data.CO2_ppm - 1.471*pow(10,-3);
		pStateReal->lifeData.CO2_data.CO2_ppm = pStateReal->lifeData.CO2_data.CO2_ppm / (1.0 + (CO2Corr * ((stateRealGetPointer()->lifeData.pressure_surface_bar * 1000) - ((stateRealGetPointer()->lifeData.ppO2Sensor_bar[2] *1000)))));
#endif
	}

	/* apnea specials
	 */
	if(pStateReal->diveSettings.diveMode == DIVEMODE_Apnea)
	{
		if(pStateReal->mode != MODE_DIVE)
		{
			pStateReal->lifeData.apnea_total_max_depth_meter = 0;
			pStateReal->lifeData.apnea_last_dive_time_seconds = 0;
			pStateReal->lifeData.apnea_last_max_depth_meter = 0;
		}
		else
		{
			if(pStateReal->lifeData.max_depth_meter > pStateReal->lifeData.apnea_total_max_depth_meter)
				pStateReal->lifeData.apnea_total_max_depth_meter = pStateReal->lifeData.max_depth_meter;
		}

		if(pStateReal->lifeData.dive_time_seconds > 15)
		{
			pStateReal->lifeData.apnea_last_dive_time_seconds = pStateReal->lifeData.dive_time_seconds;
		}

		if(pStateReal->lifeData.counterSecondsShallowDepth)
		{
			if(pStateReal->lifeData.max_depth_meter > 1.5f)
			{
				pStateReal->lifeData.apnea_last_max_depth_meter = pStateReal->lifeData.max_depth_meter;
			}
		// reset max_depth_meter, average_depth_meter and internal values
			pStateReal->lifeData.max_depth_meter = 0;
			pStateReal->lifeData.boolResetAverageDepth = 1;
		}
	}

	setAvgDepth(pStateReal);
}

void setAvgDepth(SDiveState *pStateReal) {

	float *AvgDepthValue = &pStateReal->lifeData.average_depth_meter;
	float	DepthNow = pStateReal->lifeData.depth_meter; 
	static uint32_t AvgDepthCount = 0;
	static uint32_t AvgDepthTimer = 0;
	uint32_t AvgSecondsSinceLast;
	uint32_t DiveTime = pStateReal->lifeData.dive_time_seconds_without_surface_time;

	if(pStateReal->lifeData.boolResetAverageDepth)
	{
		*AvgDepthValue = DepthNow;
		AvgDepthCount = 0;
		AvgDepthTimer = DiveTime;
		pStateReal->lifeData.boolResetAverageDepth = 0;
	}
	else if (DiveTime > AvgDepthTimer)
	{
		AvgSecondsSinceLast = DiveTime - AvgDepthTimer;
		for(int i=0;i<AvgSecondsSinceLast;i++)
		{
			*AvgDepthValue = (*AvgDepthValue * AvgDepthCount + DepthNow) / (AvgDepthCount + 1);
			AvgDepthCount += 1;
		}
		AvgDepthTimer = DiveTime;
	}
	if(AvgDepthCount == 0)
		*AvgDepthValue = DepthNow;
}


uint8_t DataEX_check_RTE_version__needs_update(void)
{
	if(data_old__lost_connection_to_slave_counter_retry > 10)
		return 1;
	else
	{
		if(stateRealGetPointer()->data_old__lost_connection_to_slave == 0)
		{
			setActualRTEversion(dataIn.RTE_VERSION_high, dataIn.RTE_VERSION_low);
			
			if(RTEminimum_required_high() < dataIn.RTE_VERSION_high)
				return 0;
			else
			if((RTEminimum_required_high() == dataIn.RTE_VERSION_high) && (RTEminimum_required_low() <= dataIn.RTE_VERSION_low))
				return 0;
			else
				return 1;
		}
		else
			return 0;
	}
}


	/* Private functions ---------------------------------------------------------*/

/* Check if there is an empty frame provided by RTE (all 0) or even no data provided by RTE (all 0xFF)
 * If that is not the case the DMA is somehow not in sync
 */
static uint8_t DataEX_check_header_and_footer_shifted()
{
	uint8_t ret = 1;
	if((dataIn.footer.checkCode[0] == 0x00)
	&& (dataIn.footer.checkCode[1] == 0x00)
	&& (dataIn.footer.checkCode[2] == 0x00)
	&& (dataIn.footer.checkCode[3] == 0x00)) { ret = 0; }

	if((dataIn.footer.checkCode[0] == 0xff)
	&& (dataIn.footer.checkCode[1] == 0xff)
	&& (dataIn.footer.checkCode[2] == 0xff)
	&& (dataIn.footer.checkCode[3] == 0xff)) { ret = 0; }

	return ret;
}

static uint8_t DataEX_check_header_and_footer_ok(void)
{
	if(dataIn.header.checkCode[0] != 0xA1)
		return 0;
#if USE_OLD_HEADER_FORMAT
	if(dataIn.header.checkCode[1] != 0xA2)
		return 0;
	if(dataIn.header.checkCode[2] != 0xA3)
		return 0;
#endif
	if(dataIn.header.checkCode[3] != 0xA4)
		return 0;
	if(dataIn.footer.checkCode[0] != 0xE1)
		return 0;
	if(dataIn.footer.checkCode[1] != 0xE2)
		return 0;
	if(dataIn.footer.checkCode[2] != 0xE3)
		return 0;
	if(dataIn.footer.checkCode[3] != 0xE4)
		return 0;

	return 1;
}

static uint8_t DataEX_check_header_and_footer_devicedata(void)
{
	if(dataIn.header.checkCode[0] != 0xDF)
		return 0;
	if(dataIn.header.checkCode[1] != 0xDE)
		return 0;
	if(dataIn.header.checkCode[2] != 0xDD)
		return 0;
	if(dataIn.header.checkCode[3] != 0xDC)
		return 0;
	if(dataIn.footer.checkCode[0] != 0xE1)
		return 0;
	if(dataIn.footer.checkCode[1] != 0xE2)
		return 0;
	if(dataIn.footer.checkCode[2] != 0xE3)
		return 0;
	if(dataIn.footer.checkCode[3] != 0xE4)
		return 0;

	return 1;
}

void DataEX_merge_devicedata(void)
{
	if(DeviceDataUpdated)
	{
		DeviceDataUpdated = 0;
		DataEX_merge_DeviceData_and_store();
	}
}
uint8_t DataEX_external_ADC_Present(void)
{
	uint8_t retval;
	retval = hw_Info.extADC;

	return retval;
}

void DataEX_setExtInterface_Cmd(uint16_t Cmd)
{
	externalInterface_Cmd = Cmd;
	return;
}

