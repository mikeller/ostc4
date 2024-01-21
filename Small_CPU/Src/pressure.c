/**
  ******************************************************************************
  * @file    pressure.c 
  * @author  heinrichs weikamp gmbh
  * @date    2014
  * @version V0.0.2
  * @since   20-Oct-2016
  * @brief   
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
	V0.0.2		18-Oct-2016		pressure_calculation_AN520_004_mod_MS5803_30BA__09_2015
	
	@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 



/* surface time
 the last 30 minutes will be saved once per minute in a endless loop
 at the beginning of a dive the oldest value will be used
*/
#include "math.h"
#include "scheduler.h"
#include "pressure.h"
#include "i2c.h"
#include "rtc.h"

#define CMD_RESET 0x1E // ADC reset command
#define CMD_ADC_READ 0x00 // ADC read command
#define CMD_ADC_CONV 0x40 // ADC conversion command
#define CMD_ADC_D1 0x00 // ADC D1 conversion
#define CMD_ADC_D2 0x10 // ADC D2 conversion
#define CMD_ADC_256 0x00 // ADC OSR=256
#define CMD_ADC_512 0x02 // ADC OSR=512
#define CMD_ADC_1024 0x04 // ADC OSR=1024
#define CMD_ADC_2048 0x06 // ADC OSR=2056
#define CMD_ADC_4096 0x08 // ADC OSR=4096
#define CMD_PROM_RD 0xA0 // Prom read command

/* remove comment to use a predefined profile for pressure changes instead of real world data */
/* #define SIMULATE_PRESSURE */


#define PRESSURE_SURFACE_MAX_MBAR			(1060.0f)		/* It is unlikely that pressure at surface is greater than this value => clip to it */

#define PRESSURE_MINIMUM					(0.0f)
#define TEMPERATURE_MINIMUM					(-100.0f)

#define PRESSURE_SURFACE_QUE					(30u)			/* history buffer [minutes] for past pressure measurements */
#define PRESSURE_SURFACE_EVA_WINDOW				(15u)			/* Number of entries evaluated during instability test. Used to avoid detection while dive enters water */
#define PRESSURE_SURFACE_STABLE_LIMIT			(10u)			/* Define pressure as stable if delta (mBar) is below this value */
#define PRESSURE_SURFACE_DETECT_STABLE_CNT		(5u)			/* Event count to detect stable condition */
#define PRESSURE_SURFACE_UNSTABLE_LIMIT			(50u)			/* Define pressure as not stable if delta (mBar) is larger than this value */
#define PRESSURE_SURFACE_DETECT_UNSTABLE_CNT	(3u)			/* Event count to detect unstable condition */


static uint8_t PRESSURE_ADDRESS = DEVICE_PRESSURE_MS5803;					/* Default Address */

static uint16_t get_ci_by_coef_num(uint8_t coef_num);
//void pressure_calculation_new(void);
//void pressure_calculation_old(void);
static void pressure_calculation_AN520_004_mod_MS5803_30BA__09_2015(void);
//static uint8_t crc4(uint16_t n_prom[]);

static HAL_StatusTypeDef pressure_sensor_get_data(void);
static uint32_t get_adc(void);
uint8_t pressureSensorInitSuccess = 0;

static uint16_t C[8] = { 1 };
static uint32_t D1 = 1;
static uint32_t D2 = 1;
//static uint8_t n_crc;

/*
short C2plus10000 = -1;
short C3plus200 = -1;
short C4minus250 = -1;
short UT1 = -1;
short C6plus100 = -1;
*/
static float pressure_offset = 0.0;		/* Offset value which may be specified by the user via PC Software */
static float temperature_offset = 0.0;	/* Offset value which may be specified by the user via PC Software */

static float ambient_temperature = 0;
static float ambient_pressure_mbar = 1000.0;
static float surface_pressure_mbar = 1000.0;
static float surface_ring_mbar[PRESSURE_SURFACE_QUE] = { 0 };

static uint8_t surface_pressure_writeIndex = 0;
static float surface_pressure_stable_value = 0;
static uint8_t surface_pressure_stable = 0;

static uint8_t secondCounterSurfaceRing = 0;
static uint8_t avgCount = 0;
static float runningAvg = 0;

float get_temperature(void)
{
	return ambient_temperature;
}

float get_pressure_mbar(void)
{
	return ambient_pressure_mbar;
}

float get_surface_mbar(void)
{
	return surface_pressure_mbar;
}


void init_surface_ring(uint8_t force)
{
	if((surface_ring_mbar[0] == 0) || (force))		/* only initialize once. Keep value in place in case of an i2c recovery */
	{
		secondCounterSurfaceRing = 0;				/* restart calculation */
		avgCount = 0;
		runningAvg = 0;

		for(int i=0; i<PRESSURE_SURFACE_QUE; i++)
			surface_ring_mbar[i] = ambient_pressure_mbar;
		surface_pressure_mbar = ambient_pressure_mbar;
		surface_pressure_writeIndex = 0;			/* index of the oldest value in the ring buffer */
	}
}

uint8_t is_surface_pressure_stable(void)
{
	return surface_pressure_stable;
}

float set_last_surface_pressure_stable(void)
{
	surface_pressure_mbar = surface_pressure_stable_value;
	return surface_pressure_stable_value;
}

/* iterate backward through the history memory and evaluate the changes pressure changes during the last 30 minutes */
void evaluate_surface_pressure()
{
	uint8_t index;
	float lastvalue;
	uint8_t stablecnt = 0;
	uint8_t unstablecnt = 0;
	uint8_t EvaluationWindow = PRESSURE_SURFACE_QUE - PRESSURE_SURFACE_EVA_WINDOW;	/* do not use the latest 15 values to avoid unstable condition due to something like fin handling */
	uint8_t EvaluatedValues = 0;

	lastvalue = surface_ring_mbar[surface_pressure_writeIndex];
	surface_pressure_stable_value = surface_ring_mbar[surface_pressure_writeIndex]; /* default: if no stable value is found return the oldest value */
	index = surface_pressure_writeIndex;
	surface_pressure_stable = 1;

	if(index == 0)
	{
		index = PRESSURE_SURFACE_QUE - 1;
	}
	else
	{
		index = index - 1;
	}
	do
	{
		if((EvaluatedValues < EvaluationWindow) &&
			(fabs(surface_pressure_stable_value - surface_ring_mbar[index]) > PRESSURE_SURFACE_UNSTABLE_LIMIT)) /* unusual change during last 30 minutes */
		{
			unstablecnt++;
			if(unstablecnt > PRESSURE_SURFACE_DETECT_UNSTABLE_CNT)
			{
				surface_pressure_stable = 0;
			}
		}
	/* search for a value which does not change for several iterations */
		if (fabs(lastvalue - surface_ring_mbar[index]) < PRESSURE_SURFACE_STABLE_LIMIT)
		{
			stablecnt++;
		}
		else
		{
			stablecnt = 0;
		}
		if ((stablecnt >= PRESSURE_SURFACE_DETECT_STABLE_CNT) && (surface_pressure_stable == 0)&&(surface_pressure_stable_value == surface_ring_mbar[surface_pressure_writeIndex])) /* pressure is unstable => search for new stable value */
		{
			surface_pressure_stable_value = surface_ring_mbar[index];
			unstablecnt = 0;
		}

		lastvalue = surface_ring_mbar[index];

		if(index == 0)
		{
			index = PRESSURE_SURFACE_QUE - 1;
		}
		else
		{
			index = index - 1;
		}
		EvaluatedValues++;
	} while (index != surface_pressure_writeIndex);
}
void update_surface_pressure(uint8_t call_rhythm_seconds)
{
	if(is_init_pressure_done())
	{
		runningAvg = (runningAvg * avgCount + ambient_pressure_mbar) / (avgCount +1);
		avgCount++;
		secondCounterSurfaceRing += call_rhythm_seconds;

		if(secondCounterSurfaceRing >= 60)
		{
			if(runningAvg < PRESSURE_SURFACE_MAX_MBAR)
			{
				surface_ring_mbar[surface_pressure_writeIndex] = runningAvg;
			}
			else
			{
				surface_ring_mbar[surface_pressure_writeIndex] =	PRESSURE_SURFACE_MAX_MBAR;
			}
			surface_pressure_writeIndex++; /* the write index is now pointing to the oldest value in the buffer which will be overwritten next time */

			if(surface_pressure_writeIndex == PRESSURE_SURFACE_QUE)
			{
				surface_pressure_writeIndex = 0;
			}

			surface_pressure_mbar = surface_ring_mbar[surface_pressure_writeIndex]; /* 30 minutes old measurement */

			secondCounterSurfaceRing = 0;
			avgCount = 1;	/* use the current value as starting point but restart the weight decrement of the measurements */
		}
		evaluate_surface_pressure();
	}
}

#ifdef DEMOMODE
float demo_modify_temperature_helper(float bottom_mbar_diff_to_surface)
{
	const float temperature_surface = 31.0;
	const float temperature_bottom = 14.0;

	const float temperature_difference = temperature_bottom - temperature_surface;
	
	// range 0.0 - 1.0
	float position_now = (ambient_pressure_mbar - surface_pressure_mbar) / bottom_mbar_diff_to_surface; 

	if(position_now <= 0)
		return temperature_surface;
	
	if(position_now >= 1)
		return temperature_bottom;

	return temperature_surface + (temperature_difference * position_now);
}


uint32_t demo_modify_temperature_and_pressure(int32_t divetime_in_seconds, uint8_t subseconds, float ceiling_mbar)
{
	
	const float descent_rate = 4000/60;
	const float ascent_rate = 1000/60;
	const uint32_t seconds_descend = (1 * 60) + 30;
	const uint32_t turbo_seconds_at_bottom_start = (0 * 60) + 0;
	const uint32_t seconds_descend_and_bottomtime = seconds_descend + turbo_seconds_at_bottom_start + (2 * 60) + 0;
	uint32_t time_elapsed_in_seconds;
	static float ambient_pressure_mbar_memory = 0;
	static uint32_t time_last_call = 0;
	
	if(divetime_in_seconds <= seconds_descend)
	{
		ambient_pressure_mbar = (divetime_in_seconds * descent_rate) + ((float)(subseconds) * descent_rate) + surface_pressure_mbar;
		ambient_temperature = demo_modify_temperature_helper(descent_rate * seconds_descend);

		time_last_call = divetime_in_seconds;
		return 0;
	}
	else
	if(divetime_in_seconds <= seconds_descend + turbo_seconds_at_bottom_start)
	{
		ambient_pressure_mbar = (seconds_descend * descent_rate) + surface_pressure_mbar;
		ambient_temperature = demo_modify_temperature_helper(descent_rate * seconds_descend);
		ambient_pressure_mbar_memory = ambient_pressure_mbar;
		time_last_call = divetime_in_seconds;
		return turbo_seconds_at_bottom_start;
	}
	else
	if(divetime_in_seconds <= seconds_descend_and_bottomtime)
	{
		ambient_pressure_mbar = (seconds_descend * descent_rate) + surface_pressure_mbar;
		ambient_temperature = demo_modify_temperature_helper(descent_rate * seconds_descend);
		ambient_pressure_mbar_memory = ambient_pressure_mbar;
		time_last_call = divetime_in_seconds;
		return 0;
	}
	else
	{
		time_elapsed_in_seconds = divetime_in_seconds - time_last_call;
		ambient_pressure_mbar = ambient_pressure_mbar_memory - time_elapsed_in_seconds * ascent_rate;

		if(ambient_pressure_mbar < surface_pressure_mbar)
			ambient_pressure_mbar = surface_pressure_mbar;
		else if(ambient_pressure_mbar < ceiling_mbar)
			ambient_pressure_mbar = ceiling_mbar;
		
		ambient_temperature = demo_modify_temperature_helper(descent_rate * seconds_descend);
		ambient_pressure_mbar_memory = ambient_pressure_mbar;
		time_last_call = divetime_in_seconds;
		return 0;
	}
}
#endif

uint8_t is_init_pressure_done(void)
{
	return pressureSensorInitSuccess;
}

uint8_t init_pressure(void)
{
	uint8_t buffer[1];
	buffer[0] = 0x1E;			// Reset Command
	uint8_t retValue = 0xFF;
	
	pressureSensorInitSuccess = false;

/* Probe new sensor first */
	retValue = I2C_Master_Transmit(  DEVICE_PRESSURE_MS5837, buffer, 1);
	if(retValue != HAL_OK)
	{
		PRESSURE_ADDRESS = DEVICE_PRESSURE_MS5803;			// use old sensor
		HAL_Delay(100);
		I2C_DeInit();
		HAL_Delay(100);
		MX_I2C1_Init();
		HAL_Delay(100);
	}
	else
	{
		PRESSURE_ADDRESS = DEVICE_PRESSURE_MS5837;			// Success, use new sensor
	}
	HAL_Delay(3);		//2.8ms according to datasheet

	buffer[0] = 0x1E;			// Reset Command
	retValue = 0xFF;

/* Send reset request to pressure sensor */
	retValue = I2C_Master_Transmit(  PRESSURE_ADDRESS, buffer, 1);
	if(retValue != HAL_OK)
	{
		return (HAL_StatusTypeDef)retValue;
	}
	HAL_Delay(3);		//2.8ms according to datasheet
	
	for(uint8_t i=0;i<7;i++)
	{
		C[i] = get_ci_by_coef_num(i);
	}
	// n_crc = crc4(C); // no evaluation at the moment hw 151026

	if(global.I2C_SystemStatus == HAL_OK)
	{
		pressureSensorInitSuccess = 1;
		retValue = pressure_update();
	}
	return retValue;
}


static uint32_t get_adc(void)
{
	uint8_t buffer[1];
	uint8_t resivebuf[4];
	uint32_t answer = 0xFFFFFFFF;

	buffer[0] = 0x00; // Get ADC
	if(I2C_Master_Transmit( PRESSURE_ADDRESS, buffer, 1) == HAL_OK)
	{
		if(I2C_Master_Receive(  PRESSURE_ADDRESS, resivebuf, 4) == HAL_OK)
		{
			resivebuf[3] = 0;
			answer = 256*256 *(uint32_t)resivebuf[0]  + 256 * (uint32_t)resivebuf[1] + (uint32_t)resivebuf[2];
		}
	}
	return answer;
}


static uint16_t get_ci_by_coef_num(uint8_t coef_num)
{
	uint8_t resivebuf[2];

	uint8_t cmd = CMD_PROM_RD+coef_num*2; 
	I2C_Master_Transmit( PRESSURE_ADDRESS, &cmd, 1);
	I2C_Master_Receive(  PRESSURE_ADDRESS, resivebuf, 2);
	return (256*(uint16_t)resivebuf[0]) + (uint16_t)resivebuf[1];
}



uint8_t pressure_update(void)
{
	HAL_StatusTypeDef statusReturn = HAL_TIMEOUT;
	
	statusReturn = pressure_sensor_get_data();
	pressure_calculation();
	return (uint8_t)statusReturn;
}

/* Switch between pressure and temperature measurement with every successful read operation */
void pressure_update_alternating(void)
{
	static uint8_t getTemperature= 0;

	if(getTemperature)
	{
		if(pressure_sensor_get_temperature_raw() == HAL_OK)
		{
			getTemperature = 0;
		}
	}
	else
	{
		if(pressure_sensor_get_pressure_raw() == HAL_OK)
		{
			getTemperature = 1;
		}
	}
	pressure_calculation();
	return;
}

static uint32_t pressure_sensor_get_one_value(uint8_t cmd, HAL_StatusTypeDef *statusReturn)
{
	uint8_t command = CMD_ADC_CONV + cmd;
	uint32_t adcValue = 0;
	HAL_StatusTypeDef statusReturnTemp = HAL_TIMEOUT;
	
	statusReturnTemp = I2C_Master_Transmit( PRESSURE_ADDRESS, &command, 1);

	if(statusReturn)
	{
		*statusReturn = statusReturnTemp;
	}

	switch (cmd & 0x0f) // wait necessary conversion time
	{
		case CMD_ADC_256 : HAL_Delay(2); break;
		case CMD_ADC_512 : HAL_Delay(4); break;
		case CMD_ADC_1024: HAL_Delay(5); break;
		case CMD_ADC_2048: HAL_Delay(7); break;
		case CMD_ADC_4096: HAL_Delay(11); break;
		default:
			break;
	}
	adcValue = get_adc();
/*	if(adcValue == 0xFFFFFFFF)
	{
		if(statusReturn)
		{
			*statusReturn = HAL_ERROR;
		}
	}*/

	return adcValue;
}


static HAL_StatusTypeDef pressure_sensor_get_data(void)
{
	uint32_t requestedValue = 0;
	HAL_StatusTypeDef statusReturn1 = HAL_TIMEOUT;
	HAL_StatusTypeDef statusReturn2 = HAL_TIMEOUT;
	


	requestedValue = pressure_sensor_get_one_value(CMD_ADC_D2 + CMD_ADC_4096, &statusReturn2);
	if (statusReturn2 == HAL_OK)
	{
		D2 = requestedValue;
	}

	requestedValue = pressure_sensor_get_one_value(CMD_ADC_D1 + CMD_ADC_4096, &statusReturn1);
	if (statusReturn1 == HAL_OK)
	{
		D1 = requestedValue;
	}
	if(statusReturn2 > statusReturn1) // if anything is not HAL_OK (0x00) or worse
		return statusReturn2;
	else
		return statusReturn1;
}


HAL_StatusTypeDef  pressure_sensor_get_pressure_raw(void)
{
	uint32_t requestedValue = 0;
	HAL_StatusTypeDef statusReturn = HAL_TIMEOUT;

	requestedValue = pressure_sensor_get_one_value(CMD_ADC_D1 + CMD_ADC_4096, &statusReturn);
	if (statusReturn == HAL_OK)
	{
		D1 = requestedValue;
	}

	return statusReturn;
}


HAL_StatusTypeDef  pressure_sensor_get_temperature_raw(void)
{
	uint32_t requestedValue = 0;
	HAL_StatusTypeDef statusReturn = HAL_TIMEOUT;

	requestedValue = pressure_sensor_get_one_value(CMD_ADC_D2 + CMD_ADC_4096, &statusReturn);
	if (statusReturn == HAL_OK)
	{
		D2 = requestedValue;
	}
	return statusReturn;
}


#ifdef SIMULATE_PRESSURE
void pressure_simulation()
{
	static uint32_t tickstart = 0;
	static float pressure_sim_mbar = 0;
	static uint32_t passedSecond = 0;
	static uint32_t secondtick = 0;

	uint32_t lasttick = 0;



	if( tickstart == 0)
	{
		tickstart = HAL_GetTick(); /* init time stamp */
		secondtick = tickstart;
		pressure_sim_mbar = 1000;
	}

	lasttick = HAL_GetTick();
	if(time_elapsed_ms(secondtick,lasttick) > 1000) /* one second passed since last tick */
	{
		secondtick = lasttick;
		passedSecond++;

#ifdef DIVE_AFTER_LANDING
		if(passedSecond < 10) pressure_sim_mbar = 1000.0;	 /* stay stable for 10 seconds */
		else if(passedSecond < 300) pressure_sim_mbar -= 1.0; /* decrease pressure in 5 minutes target 770mbar => delta 330 */
		else if(passedSecond < 900) pressure_sim_mbar += 0.0;	/*stay stable 10 minutes*/
		else if(passedSecond < 1500) pressure_sim_mbar += 0.5;	/* return to 1 bar in 10 Minutes*/
		else if(passedSecond < 1800) pressure_sim_mbar += 0.0;	/* 5 minutes break */
		else if(passedSecond < 2000) pressure_sim_mbar += 10.0; /* start dive */
		else if(passedSecond < 2300) pressure_sim_mbar += 0.0;  /* stay on depth */
		else if(passedSecond < 2500) pressure_sim_mbar -= 10.0; /* return to surface */
		else pressure_sim_mbar = 1000.0;					/* final state */
#else	/* short dive */
		if(passedSecond < 10) pressure_sim_mbar = 1000.0;	   /* stay stable for 10 seconds */
		else if(passedSecond < 180) pressure_sim_mbar += 10.0; /* Start dive */
		else if(passedSecond < 300) pressure_sim_mbar += 0.0;	/*stay on depth*/
		else if(passedSecond < 460) pressure_sim_mbar -= 10.0;	/* return to surface */
		else if(passedSecond < 600) pressure_sim_mbar += 0.0;   /* stay */
		else if(passedSecond < 610) pressure_sim_mbar = 1000.0; /* get ready for second dive */
		else if(passedSecond < 780) pressure_sim_mbar += 10.0; /* Start dive */
		else if(passedSecond < 900) pressure_sim_mbar += 0.0;	/*stay on depth*/
		else if(passedSecond < 1060) pressure_sim_mbar -= 10.0;	/* return to surface */
		else if(passedSecond < 1200) pressure_sim_mbar += 0.0;   /* stay */
		else pressure_sim_mbar = 1000.0;					/* final state */
#endif
	}


	ambient_pressure_mbar = pressure_sim_mbar;
	ambient_temperature = 25.0;
	return;
}

#endif

void pressure_calculation(void)
{
	if(global.I2C_SystemStatus != HAL_OK)
		return;

#ifdef SIMULATE_PRESSURE
	pressure_simulation();
#else
	pressure_calculation_AN520_004_mod_MS5803_30BA__09_2015();
#endif
}

static void pressure_calculation_AN520_004_mod_MS5803_30BA__09_2015(void)
{
	static float runningAvg = 0;
	static uint8_t avgCnt = 0;

	uint32_t local_D1; // ADC value of the pressure conversion
	uint32_t local_D2; // ADC value of the temperature conversion
	int32_t local_Px10; // compensated pressure value
	int32_t local_Tx100; // compensated temperature value
	int64_t local_dT; // int32_t, difference between actual and measured temperature
	int64_t local_OFF; // offset at actual temperature
	int64_t local_SENS; // sensitivity at actual temperature

	float calc_pressure;

	int64_t T2;
	int64_t OFF2;
	int64_t SENS2;

	local_D1 = D1;
	local_D2 = D2;

	local_dT 		= ((int64_t)local_D2) - ((int64_t)C[5]) * 256; //pow(2,8);
	local_OFF 	= ((int64_t)C[2]) * 65536 + local_dT * ((int64_t)C[4]) / 128; // pow(2,16), pow(2,7)
	local_SENS 	= ((int64_t)C[1]) * 32768 + local_dT * ((int64_t)C[3]) / 256; // pow(2,15), pow(2,8)

	local_Tx100 = (int32_t)(2000 + (local_dT * ((int64_t)C[6])) / 8388608);// pow(2,23)


	if(local_Tx100 < 2000) // low temperature
	{
		T2  = 3 * local_dT;
		T2 *= local_dT;
		T2 /= 8589934592;

		OFF2 = ((int64_t)local_Tx100) - 2000;
		OFF2 *= OFF2;
		OFF2 *= 3;
		OFF2 /= 2;
		
		SENS2 = ((int64_t)local_Tx100) - 2000;
		SENS2 *= SENS2;
		SENS2 *= 5;
		SENS2 /= 8;

		local_Tx100 -= (int32_t)T2;
		local_OFF 	-= OFF2;
		local_SENS 	-= SENS2;
	}
	else
	{
		T2  = 7 * local_dT;
		T2 *= local_dT;
		T2 /= 137438953472;

		OFF2 = ((int64_t)local_Tx100) - 2000;
		OFF2 *= OFF2;
		OFF2 /= 16;
		
		local_Tx100 -= (int32_t)T2;
		local_OFF 	-= OFF2;
	}
	
	local_Px10 = (int32_t)(
							(((int64_t)((local_D1 * local_SENS) / 2097152)) - local_OFF)
								/  8192 );//     )) / 10; // pow(2,21), pow(2,13)

	ambient_temperature = ((float)local_Tx100) / 100;
	ambient_temperature	+= temperature_offset;

	if(ambient_temperature < TEMPERATURE_MINIMUM)
	{
		ambient_temperature = 20.0;
	}

	calc_pressure = ((float)local_Px10) / 10;
	calc_pressure += pressure_offset;

	runningAvg = (avgCnt * runningAvg + calc_pressure) / (avgCnt + 1);
	if (avgCnt < 10)	/* build an average considering the last measurements to have a weight "1 of 10" */
	{					/* Main reason for this is the jitter of up to +-10 HPa in surface mode which is caused */
		avgCnt++;		/* by the measurement range of the sensor which is focused on under water pressure measurement */
	}
	ambient_pressure_mbar = runningAvg;

	if(ambient_pressure_mbar < PRESSURE_MINIMUM)
	{
		ambient_pressure_mbar = 1000.0 + pressure_offset;
	}
}


/* taken from AN520 by meas-spec.com dated 9. Aug. 2011
 * short and int are both 16bit according to AVR/GCC google results
 */
/*static uint8_t crc4(uint16_t n_prom[])
{
uint16_t cnt; // simple counter
uint16_t n_rem; // crc reminder
uint16_t crc_read; // original value of the crc
uint8_t n_bit;
n_rem = 0x00;
crc_read=n_prom[7]; //save read CRC
n_prom[7]=(0xFF00 & (n_prom[7])); //CRC byte is replaced by 0
for (cnt = 0; cnt < 16; cnt++) // operation is performed on bytes
{ // choose LSB or MSB
if (cnt%2==1) n_rem ^= (uint16_t) ((n_prom[cnt>>1]) & 0x00FF);
else n_rem ^= (uint16_t) (n_prom[cnt>>1]>>8);
for (n_bit = 8; n_bit > 0; n_bit--)
{
if (n_rem & (0x8000))
{
n_rem = (n_rem << 1) ^ 0x3000;
}
else
{
n_rem = (n_rem << 1);
}
}
}
n_rem= (0x000F & (n_rem >> 12)); // // final 4-bit reminder is CRC code
n_prom[7]=crc_read; // restore the crc_read to its original place
return (n_rem ^ 0x00);
}

void test_calculation(void)
{
	C1 = 29112;
	C2 = 26814;
	C3 = 19125;
	C4 = 17865;
	C5 = 32057;
	C6 = 31305;
	
	C2_x_2p16 = C2 * 65536;
	C1_x_2p15 = C1 * 32768;
	
	D1 = 4944364;
	D2 = 8198974;	
		pressure_calculation() ;
};
*/
void pressure_set_offset (float pressureOffset, float temperatureOffset)
{
	if(pressure_offset != pressureOffset)				/* we received a new value => reinit surface que */
	{
		ambient_pressure_mbar -= pressure_offset;		/* revert old value */
		ambient_pressure_mbar += pressureOffset;		/* apply new offset */
		init_surface_ring(1);
	}

	pressure_offset = pressureOffset;
	temperature_offset = temperatureOffset;
}


