/**
  ******************************************************************************
  * @file    compass.c 
  * @author  heinrichs weikamp gmbh
  * @date    27-March-2014
  * @version V0.2.0
  * @since   21-April-2016
  * @brief   for Honeywell Compass and ST LSM303D
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
	V0.1.0		09-March-2016
	V0.2.0		21-April-2016	Orientation fixed for LSM303D,
													roll and pitch added to calibration output,
													orientation double checked with datasheets and layout
													as well as with value output during calibration
	V0.2.1		19-May-2016		New date rate config and full-scale selection
	
	@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 

#include <math.h>
#include <string.h>

#include "compass.h"
#include "compass_LSM303D.h"

#include "i2c.h"
#include "spi.h"
#include "scheduler.h"
#include "RTE_FlashAccess.h" // to store compass_calib_data

#include "stm32f4xx_hal.h"

extern uint32_t time_elapsed_ms(uint32_t ticksstart,uint32_t ticksnow);
extern SGlobal global;


/// crazy compass calibration stuff
typedef struct
{
unsigned short int compass_N;							
float Su, Sv, Sw;													
float Suu, Svv, Sww, Suv, Suw, Svw;				
float Suuu, Svvv, Swww;										
float Suuv, Suuw, Svvu, Svvw, Swwu, Swwv;	
} SCompassCalib; 


#define Q_PI    (18000)
#define Q_PIO2  (9000)



//////////////////////////////////////////////////////////////////////////////
// fifth order of polynomial approximation of atan(), giving 0.05 deg max error
//
#define K1  (5701)  // Needs K1/2**16
#define K2  (1645)  // Needs K2/2**48  WAS NEGATIV
#define K3  ( 446)  // Needs K3/2**80

const float PI = 3.14159265;	///< pi, used in compass_calc()

typedef short int Int16;
typedef signed char Int8;
typedef Int16 Angle;


/// The (filtered) components of the magnetometer sensor
int16_t compass_DX_f; ///< output from sensor
int16_t compass_DY_f; ///< output from sensor
int16_t compass_DZ_f; ///< output from sensor


/// Found soft-iron calibration values, deduced from already filtered values
int16_t compass_CX_f; ///< calibration value
int16_t compass_CY_f; ///< calibration value
int16_t compass_CZ_f; ///< calibration value


float compass_DX_min;
float compass_DX_max;

float compass_DY_min;
float compass_DY_max;

float compass_DZ_min;
float compass_DZ_max;



/// The (filtered) components of the accelerometer sensor
int16_t accel_DX_f; ///< output from sensor
int16_t accel_DY_f; ///< output from sensor
int16_t accel_DZ_f; ///< output from sensor


/// The compass result values
float compass_heading;	///< the final result calculated in compass_calc()
float compass_roll;			///< the final result calculated in compass_calc()
float compass_pitch;		///< the final result calculated in compass_calc()


uint8_t compass_gain; ///< 7 on start, can be reduced during calibration

uint8_t  hardwareCompass = compass_generation_undef;	///< either HMC5883L (=1) or LSM303D (=2) or LSM303AGR (=3) or not defined yet (=0)

/// LSM303D variables
uint8_t magDataBuffer[6];	///< here raw data from LSM303D is stored, can be local
uint8_t accDataBuffer[6];	///< here raw data from LSM303D is stored, can be local


//	struct accel_scale	_accel_scale;
unsigned		_accel_range_m_s2;
float			_accel_range_scale;
unsigned		_accel_samplerate;
unsigned		_accel_onchip_filter_bandwith;

//	struct mag_scale	_mag_scale;
unsigned		_mag_range_ga;
float			_mag_range_scale;
unsigned		_mag_samplerate;

// default scale factors
float _accel_scale_x_offset = 0.0f;
float _accel_scale_x_scale  = 1.0f;
float _accel_scale_y_offset = 0.0f;
float _accel_scale_y_scale  = 1.0f;
float _accel_scale_z_offset = 0.0f;
float _accel_scale_z_scale  = 1.0f;

float _mag_scale_x_offset = 0.0f;
float _mag_scale_x_scale = 1.0f;
float _mag_scale_y_offset = 0.0f;
float _mag_scale_y_scale = 1.0f;
float _mag_scale_z_offset = 0.0f;
float _mag_scale_z_scale = 1.0f;


/* External function prototypes ----------------------------------------------*/

extern void copyCompassDataDuringCalibration(int16_t dx, int16_t dy, int16_t dz);

/* Private function prototypes -----------------------------------------------*/

void compass_reset_calibration(SCompassCalib *g);
void compass_add_calibration(SCompassCalib *g);
void compass_solve_calibration(SCompassCalib *g);

void compass_init_HMC5883L(uint8_t fast, uint8_t gain);
void compass_sleep_HMC5883L(void);
void compass_read_HMC5883L(void);

void accelerator_init_MMA8452Q(void);
void accelerator_sleep_MMA8452Q(void);
void acceleration_read_MMA8452Q(void);

void compass_init_LSM303D(uint8_t fast, uint8_t gain);
void compass_sleep_LSM303D(void);
void compass_read_LSM303D(void);
void acceleration_read_LSM303D(void);

void compass_init_LSM303AGR(uint8_t fast, uint8_t gain);
void compass_sleep_LSM303AGR(void);
void compass_read_LSM303AGR(void);
void acceleration_read_LSM303AGR(void);

int LSM303D_accel_set_onchip_lowpass_filter_bandwidth(unsigned bandwidth);
int compass_calib_common(void);

void compass_calc_roll_pitch_only(void);

void rotate_mag_3f(float *x, float *y, float *z);
void rotate_accel_3f(float *x, float *y, float *z);


/* Exported functions --------------------------------------------------------*/


//  ===============================================================================
//	compass_init
/// @brief	This might be called several times with different gain values during calibration
/// 				On first call it figures out which hardware is integrated
///
/// @param 	gain: 7 is max gain, compass_calib() might reduce it
//  ===============================================================================

void compass_init(uint8_t fast, uint8_t gain)
{

// don't call again with fast, gain in calib mode etc.
// if unknown
	if(hardwareCompass == COMPASS_NOT_RECOGNIZED)
	{
		return;
	}
	
	if(hardwareCompass == compass_generation_undef)					/* check if compass had been detected before */
	{
		tfull32 dataBlock[4];
		if(BFA_readLastDataBlock(dataBlock) == BFA_OK)
		{
			if(dataBlock[3].Word16.hi16 == BFA_calc_Block_Checksum(dataBlock))
			{
				compass_CX_f = dataBlock[0].Word16.low16;
				compass_CY_f = dataBlock[0].Word16.hi16;
				compass_CZ_f = dataBlock[1].Word16.low16;
				hardwareCompass = dataBlock[1].Word16.hi16;
				if(hardwareCompass >= compass_generation_future)		/* no generation stored (including COMPASS_NOT_RECOGNIZED) */
				{
					hardwareCompass = compass_generation_undef;
				}
			}
		}
	}

// old code but without else
	if(hardwareCompass == compass_generation_undef)
	{
		uint8_t data = WHO_AM_I;
		I2C_Master_Transmit( DEVICE_COMPASS_303D, &data, 1);
		I2C_Master_Receive(  DEVICE_COMPASS_303D, &data, 1);
		if(data == WHOIAM_VALUE_LSM303D)
			hardwareCompass = compass_generation2;			//LSM303D;
		data = WHO_AM_I;
		I2C_Master_Transmit( DEVICE_ACCELARATOR_303AGR, &data, 1);
		I2C_Master_Receive(  DEVICE_ACCELARATOR_303AGR, &data, 1);
		if(data == WHOIAM_VALUE_LSM303AGR)
			hardwareCompass = compass_generation3;			//LSM303AGR;
	}

/* No compass identified => Retry */
	if(hardwareCompass == compass_generation_undef)
	{
		I2C_DeInit();
		HAL_Delay(100);
		MX_I2C1_Init();
		HAL_Delay(100);
		uint8_t data = WHO_AM_I;
		I2C_Master_Transmit( DEVICE_COMPASS_303D, &data, 1);
		I2C_Master_Receive(  DEVICE_COMPASS_303D, &data, 1);
		if(data == WHOIAM_VALUE_LSM303D)
		{
			hardwareCompass = compass_generation2;			//LSM303D;
		}
		else
		{
			data = WHO_AM_I;
			I2C_Master_Transmit( DEVICE_ACCELARATOR_303AGR, &data, 1);
			I2C_Master_Receive(  DEVICE_ACCELARATOR_303AGR, &data, 1);
			if(data == WHOIAM_VALUE_LSM303AGR)
				hardwareCompass = compass_generation3;			//LSM303AGR;
		}
	}

/* Assume that a HMC5883L is equipped by default if detection still failed */
	if(hardwareCompass == compass_generation_undef)
		hardwareCompass = compass_generation1;				//HMC5883L;
	
	HAL_StatusTypeDef resultOfOperationHMC_MMA = HAL_TIMEOUT;

	// test if both chips of the two-chip solution (gen 1) are present
	if(hardwareCompass == compass_generation1)			// HMC5883L)
	{
		HAL_Delay(100);
		I2C_DeInit();
		HAL_Delay(100);
		MX_I2C1_Init();
		HAL_Delay(100);
		uint8_t data = 0x2A; // CTRL_REG1 of DEVICE_ACCELARATOR_MMA8452Q
		resultOfOperationHMC_MMA = I2C_Master_Transmit( DEVICE_ACCELARATOR_MMA8452Q, &data, 1);
		if(resultOfOperationHMC_MMA == HAL_OK)
		{
			hardwareCompass = compass_generation1;			//HMC5883L; // all fine, keep it
		}
		else
		{
			hardwareCompass = COMPASS_NOT_RECOGNIZED;
		}
	}
	
	if(hardwareCompass == compass_generation2)			//LSM303D)
		compass_init_LSM303D(fast, gain);
	if(hardwareCompass == compass_generation3)			//LSM303AGR)
		compass_init_LSM303AGR(fast, gain);
	if(hardwareCompass == compass_generation1)			//HMC5883L)
		compass_init_HMC5883L(fast, gain);
	
	if(global.deviceDataSendToMaster.hw_Info.compass == 0)
	{
		global.deviceDataSendToMaster.hw_Info.compass = hardwareCompass;
		global.deviceDataSendToMaster.hw_Info.checkCompass = 1;
	}
}


//  ===============================================================================
//	compass_calib
/// @brief with onchip_lowpass_filter configuration for accelerometer of LSM303D
//  ===============================================================================
int compass_calib(void)
{
	if(hardwareCompass == compass_generation2)			//LSM303D)
	{
		LSM303D_accel_set_onchip_lowpass_filter_bandwidth(773);
		int out = compass_calib_common();
		LSM303D_accel_set_onchip_lowpass_filter_bandwidth(LSM303D_ACCEL_DEFAULT_ONCHIP_FILTER_FREQ);
		return out;
	}
	else
	if(hardwareCompass == compass_generation1)			//HMC5883L)
	{
		return compass_calib_common();
	}
	else
	if(hardwareCompass == compass_generation3)			//LSM303AGR)
	{
		return compass_calib_common();
	}
	else
	{
		return 0; // standard answer of compass_calib_common();
	}


}


//  ===============================================================================
//	compass_sleep
/// @brief	low power mode
//  ===============================================================================
void compass_sleep(void)
{
	if(hardwareCompass == compass_generation2)			//LSM303D
		compass_sleep_LSM303D();
	if(hardwareCompass == compass_generation1)			//HMC5883L
		compass_sleep_HMC5883L();
	if(hardwareCompass == compass_generation3)			//LSM303AGR
		compass_sleep_LSM303AGR();
}


//  ===============================================================================
//	compass_read
/// @brief	reads magnetometer and accelerometer for LSM303D,
///					otherwise magnetometer only
//  ===============================================================================
void compass_read(void)
{
	if(hardwareCompass == compass_generation2)			//LSM303D)
		compass_read_LSM303D();
	if(hardwareCompass == compass_generation1)			//HMC5883L)
		compass_read_HMC5883L();
	if(hardwareCompass == compass_generation3)			//LSM303AGR)
		compass_read_LSM303AGR();

}


//  ===============================================================================
//	accelerator_init
/// @brief	empty for for LSM303D
//  ===============================================================================
void accelerator_init(void)
{
	if(hardwareCompass == compass_generation1)			//HMC5883L)
		accelerator_init_MMA8452Q();
}


//  ===============================================================================
//	accelerator_sleep
/// @brief	empty for for LSM303D
//  ===============================================================================
void accelerator_sleep(void)
{
	if(hardwareCompass == compass_generation1)			//HMC5883L)
		accelerator_sleep_MMA8452Q();
}


//  ===============================================================================
//	acceleration_read
/// @brief	empty for for LSM303D
//  ===============================================================================
void  acceleration_read(void)
{
	if(hardwareCompass == compass_generation2)			//LSM303D)
		acceleration_read_LSM303D();
	if(hardwareCompass == compass_generation1)			//HMC5883L)
		acceleration_read_MMA8452Q();
	if(hardwareCompass == compass_generation3)			//LSM303AGR)
		acceleration_read_LSM303AGR();
}


/* Private functions ---------------------------------------------------------*/

//  ===============================================================================
//	LSM303AGR_read_reg
//  ===============================================================================
uint8_t LSM303AGR_read_reg(uint8_t addr)
{
  uint8_t data;

	I2C_Master_Transmit( DEVICE_COMPASS_303AGR, &addr, 1);
	I2C_Master_Receive(  DEVICE_COMPASS_303AGR, &data, 1);
	return data;
}


//  ===============================================================================
//	LSM303AGR_write_reg
//  ===============================================================================
void LSM303AGR_write_reg(uint8_t addr, uint8_t value)
{
  uint8_t data[2];

	data[0] = addr;
	data[1] = value;
	I2C_Master_Transmit( DEVICE_COMPASS_303AGR, data, 2);
}

//  ===============================================================================
//	LSM303AGR_acc_write_reg
//  ===============================================================================
void LSM303AGR_acc_write_reg(uint8_t addr, uint8_t value)
{
  uint8_t data[2];

	data[0] = addr;
	data[1] = value;
	I2C_Master_Transmit( DEVICE_ACCELARATOR_303AGR, data, 2);
}


//  ===============================================================================
//	LSM303AGR_write_checked_reg
//  ===============================================================================
void LSM303AGR_write_checked_reg(uint8_t addr, uint8_t value)
{
	LSM303AGR_write_reg(addr, value);
}

//  ===============================================================================
//	LSM303AGR_acc_write_checked_reg
//  ===============================================================================
void LSM303AGR_acc_write_checked_reg(uint8_t addr, uint8_t value)
{
	LSM303AGR_acc_write_reg(addr, value);
}

//  ===============================================================================
//	LSM303D_read_reg
//  ===============================================================================
uint8_t LSM303D_read_reg(uint8_t addr)
{
  uint8_t data;

	I2C_Master_Transmit( DEVICE_COMPASS_303D, &addr, 1);
	I2C_Master_Receive(  DEVICE_COMPASS_303D, &data, 1);
	return data;
}


//  ===============================================================================
//	LSM303D_write_reg
//  ===============================================================================
void LSM303D_write_reg(uint8_t addr, uint8_t value)
{
  uint8_t data[2];
	
	/* enable accel*/
	data[0] = addr;
	data[1] = value;
	I2C_Master_Transmit( DEVICE_COMPASS_303D, data, 2);
}


//  ===============================================================================
//	LSM303D_write_checked_reg
//  ===============================================================================
void LSM303D_write_checked_reg(uint8_t addr, uint8_t value)
{
	LSM303D_write_reg(addr, value);
}


//  ===============================================================================
//	LSM303D_modify_reg
//  ===============================================================================
void LSM303D_modify_reg(unsigned reg, uint8_t clearbits, uint8_t setbits)
{
	uint8_t	val;

	val = LSM303D_read_reg(reg);
	val &= ~clearbits;
	val |= setbits;
	LSM303D_write_checked_reg(reg, val);
}

//  ===============================================================================
//	LSM303D_accel_set_onchip_lowpass_filter_bandwidth
//  ===============================================================================
int LSM303D_accel_set_onchip_lowpass_filter_bandwidth(unsigned bandwidth)
{
	uint8_t setbits = 0;
	uint8_t clearbits = REG2_ANTIALIAS_FILTER_BW_BITS_A;

	if (bandwidth == 0) {
		bandwidth = 773;
	}

	if (bandwidth <= 50) {
		setbits |= REG2_AA_FILTER_BW_50HZ_A;
		_accel_onchip_filter_bandwith = 50;

	} else if (bandwidth <= 194) {
		setbits |= REG2_AA_FILTER_BW_194HZ_A;
		_accel_onchip_filter_bandwith = 194;

	} else if (bandwidth <= 362) {
		setbits |= REG2_AA_FILTER_BW_362HZ_A;
		_accel_onchip_filter_bandwith = 362;

	} else if (bandwidth <= 773) {
		setbits |= REG2_AA_FILTER_BW_773HZ_A;
		_accel_onchip_filter_bandwith = 773;

	} else {
		return -1;
	}

	LSM303D_modify_reg(ADDR_CTRL_REG2, clearbits, setbits);

	return 0;
}


//  ===============================================================================
//	LSM303D_accel_set_driver_lowpass_filter
//  ===============================================================================
int LSM303D_accel_set_driver_lowpass_filter(float samplerate, float bandwidth)
{
/*
	_accel_filter_x_set_cutoff_frequency(samplerate, bandwidth);
	_accel_filter_y_set_cutoff_frequency(samplerate, bandwidth);
	_accel_filter_z_set_cutoff_frequency(samplerate, bandwidth);
*/	
	return 0;
}


// rotate_mag_3f: nicht genutzt aber praktisch; rotate_accel_3f wird benutzt
//  ===============================================================================
//	rotate_mag_3f
/// @brief	swap axis in convient way, by hw
/// @param 	*x raw input is set to *y input
/// @param 	*y raw input is set to -*x input
/// @param 	*z raw is not touched
//  ===============================================================================
void rotate_mag_3f(float *x, float *y, float *z)
{
	return;
/*	
	*x = *x;		// HMC: *x = -*y
	*y = *y;		// HMC: *y =  *x  // change 20.04.2016: zuvor *y = -*y
	*z = *z;		// HMC: *z =  *z  
*/	
}


//  ===============================================================================
//	rotate_accel_3f
/// @brief	swap axis in convient way, by hw, same as MMA8452Q
/// @param 	*x raw input, output is with sign change
/// @param 	*y raw input, output is with sign change
/// @param 	*z raw input, output is with sign change
//  ===============================================================================
void rotate_accel_3f(float *x, float *y, float *z)
{
	*x = -*x;
	*y = -*y;
	*z = -*z;
	/* tested:
	x = x, y =-y, z=-z: does not work with roll
	x = x, y =y, z=-z: does not work with pitch
	x = x, y =y, z=z: does not work at all
	*/
}


//  ===============================================================================
//	compass_init_LSM303D
///					This might be called several times with different gain values during calibration
///					but gain change is not supported at the moment.
///
/// @param 	gain: 7 is max gain and set with here, compass_calib() might reduce it
//  ===============================================================================

//uint8_t testCompassLS303D[11];

void compass_init_LSM303D(uint8_t fast, uint8_t gain)
{
	if(fast == 0)
	{
		LSM303D_write_checked_reg(ADDR_CTRL_REG0, 0x00);
		LSM303D_write_checked_reg(ADDR_CTRL_REG1, 0x3F); // mod 12,5 Hz 3 instead of 6,25 Hz 2 BDU and all axis
		LSM303D_write_checked_reg(ADDR_CTRL_REG2, 0xC0); // 50Hz anti alias filter
		LSM303D_write_checked_reg(ADDR_CTRL_REG3, 0x00); // no interrupts
		LSM303D_write_checked_reg(ADDR_CTRL_REG4, 0x00); // no interrupts
		LSM303D_write_checked_reg(ADDR_CTRL_REG5, 0x68); // mod 12,5 Hz 8 instead of 6,25 Hz 4 High resolution
	}
	else
	{
		LSM303D_write_checked_reg(ADDR_CTRL_REG0, 0x00);
		LSM303D_write_checked_reg(ADDR_CTRL_REG1, 0x6F); // 100 Hz
		LSM303D_write_checked_reg(ADDR_CTRL_REG2, 0xC0);
		LSM303D_write_checked_reg(ADDR_CTRL_REG3, 0x00);
		LSM303D_write_checked_reg(ADDR_CTRL_REG4, 0x00);
		LSM303D_write_checked_reg(ADDR_CTRL_REG5, 0x74); // 100 Hz
	}
	LSM303D_write_checked_reg(ADDR_CTRL_REG6, 0x00);
	LSM303D_write_checked_reg(ADDR_CTRL_REG7, 0x00);

	return;
}


//  ===============================================================================
//	compass_sleep_LSM303D
// 	@brief	Gen 2 chip
//  ===============================================================================
void compass_sleep_LSM303D(void)
{
	LSM303D_write_checked_reg(ADDR_CTRL_REG1, 0x00); // CNTRL1: acceleration sensor Power-down mode
	LSM303D_write_checked_reg(ADDR_CTRL_REG7, 0x02); // CNTRL7: magnetic sensor Power-down mode
}


//  ===============================================================================
//	acceleration_read_LSM303D
// 	output is accel_DX_f, accel_DY_f, accel_DZ_f
//  ===============================================================================
void acceleration_read_LSM303D(void)
{
  uint8_t data;
	float xraw_f, yraw_f, zraw_f;
	float accel_report_x, accel_report_y, accel_report_z;
	
  memset(accDataBuffer,0,6);

	accel_DX_f = 0;
	accel_DY_f = 0;
	accel_DZ_f = 0;
	
	for(int i=0;i<6;i++)
	{
		data = ADDR_OUT_X_L_A + i;
		I2C_Master_Transmit( DEVICE_COMPASS_303D, &data, 1);
		I2C_Master_Receive(  DEVICE_COMPASS_303D, &accDataBuffer[i], 1);
	}
	
	xraw_f = ((float)( (int16_t)((accDataBuffer[1] << 8) | (accDataBuffer[0]))));
	yraw_f = ((float)( (int16_t)((accDataBuffer[3] << 8) | (accDataBuffer[2]))));
	zraw_f = ((float)( (int16_t)((accDataBuffer[5] << 8) | (accDataBuffer[4]))));	

	rotate_accel_3f(&xraw_f, &yraw_f, &zraw_f); 
	
	// mh
	accel_report_x = xraw_f;
	accel_report_y = yraw_f;
	accel_report_z = zraw_f;

	accel_DX_f = ((int16_t)(accel_report_x));
	accel_DY_f = ((int16_t)(accel_report_y));
	accel_DZ_f = ((int16_t)(accel_report_z));
}


//  ===============================================================================
//	compass_read_LSM303D
///
/// output is compass_DX_f, compass_DY_f, compass_DZ_f
//  ===============================================================================
void compass_read_LSM303D(void)
{
  uint8_t data;
//	float xraw_f, yraw_f, zraw_f;
//	float mag_report_x, mag_report_y, mag_report_z;
	
  memset(magDataBuffer,0,6);

	compass_DX_f = 0;
	compass_DY_f = 0;
	compass_DZ_f = 0;
	
	for(int i=0;i<6;i++)
	{
		data = ADDR_OUT_X_L_M + i;
		I2C_Master_Transmit( DEVICE_COMPASS_303D, &data, 1);
		I2C_Master_Receive(  DEVICE_COMPASS_303D, &magDataBuffer[i], 1);
	}

	// mh 160620 flip x and y if flip display
	compass_DX_f = (((int16_t)((magDataBuffer[1] << 8) | (magDataBuffer[0]))));
	compass_DY_f = (((int16_t)((magDataBuffer[3] << 8) | (magDataBuffer[2]))));
	compass_DZ_f = (((int16_t)((magDataBuffer[5] << 8) | (magDataBuffer[4]))));
	// no rotation
	return;
}


//  ===============================================================================
//	compass_init_LSM303AGR
///					This might be called several times with different gain values during calibration
///					but gain change is not supported at the moment.
///
/// @param 	gain: 7 is max gain and set with here, compass_calib() might reduce it
//  ===============================================================================

void compass_init_LSM303AGR(uint8_t fast, uint8_t gain)
{
	if(fast == 0)
	{
		// init compass
		LSM303AGR_write_checked_reg(0x60, 0x80); // CFG_REG_A_M     10Hz continuous measurement
		LSM303AGR_write_checked_reg(0x61, 0x03); // CFG_REG_B_M		Enable offset cancellation and low pass filter
		LSM303AGR_write_checked_reg(0x62, 0x10); // CFG_REG_C_M		Avoid incoherence (BDU)
		LSM303AGR_write_checked_reg(0x63, 0x00); // INT_CTRL_REG_M	No interrupts

		// init accel (Same chip, but different address...)
		LSM303AGR_acc_write_checked_reg(0x1F, 0x00); // TEMP_CFG_REG_A (Temp sensor off)
		LSM303AGR_acc_write_checked_reg(0x20, 0x27); // CTRL_REG1_A (10Hz, x,y,z = ON, low power mode)
		LSM303AGR_acc_write_checked_reg(0x21, 0x00); // CTRL_REG2_A	(High pass filter normal mode)
		LSM303AGR_acc_write_checked_reg(0x22, 0x00); // CTRL_REG3_A	(no interrupts)
		LSM303AGR_acc_write_checked_reg(0x23, 0x88); // CTRL_REG4_A, High Resolution Mode enabled, enable BDU
	}
	else
	{
		// init compass
		LSM303AGR_write_checked_reg(0x60, 0x84); // 20Hz
		LSM303AGR_write_checked_reg(0x61, 0x03); // CFG_REG_B_M
		LSM303AGR_write_checked_reg(0x62, 0x10); // CFG_REG_C_M
		LSM303AGR_write_checked_reg(0x63, 0x00); // INT_CTRL_REG_M

		// init accel (Same chip, but different address...)
		LSM303AGR_acc_write_checked_reg(0x1F, 0x00); // TEMP_CFG_REG_A (Temp sensor off)
		LSM303AGR_acc_write_checked_reg(0x20, 0x47); // CTRL_REG1_A (50Hz, x,y,z = ON)
		LSM303AGR_acc_write_checked_reg(0x21, 0x00); // CTRL_REG2_A
		LSM303AGR_acc_write_checked_reg(0x22, 0x00); // CTRL_REG3_A
		LSM303AGR_acc_write_checked_reg(0x23, 0x88); // CTRL_REG4_A, High Resolution Mode enabled
	}

	return;
}


//  ===============================================================================
//	compass_sleep_LSM303D
// 	@brief	Gen 2 chip
//  ===============================================================================
void compass_sleep_LSM303AGR(void)
{
	LSM303AGR_write_checked_reg(0x60, 0x13); // low power and idle mode
	LSM303AGR_write_checked_reg(0x61, 0x04); // pulse only at power on
	LSM303AGR_write_checked_reg(0x62, 0x51); // int mag pin used (?), BDU and DRDY is output
	LSM303AGR_write_checked_reg(0x63, 0x00); // no interrupts


	LSM303AGR_acc_write_checked_reg(0x1F, 0x00); // temperature off
	LSM303AGR_acc_write_checked_reg(0x20, 0x00); //	power down
}


//  ===============================================================================
//	acceleration_read_LSM303AGR
// 	output is accel_DX_f, accel_DY_f, accel_DZ_f
//  ===============================================================================
void acceleration_read_LSM303AGR(void)
{
  uint8_t data;
	float xraw_f, yraw_f, zraw_f;
	float accel_report_x, accel_report_y, accel_report_z;

  memset(accDataBuffer,0,6);

	accel_DX_f = 0;
	accel_DY_f = 0;
	accel_DZ_f = 0;

	for(int i=0;i<6;i++)
	{
		data = 0x28 + i;			// OUT_X_L_A
		I2C_Master_Transmit( DEVICE_ACCELARATOR_303AGR, &data, 1);
		I2C_Master_Receive(  DEVICE_ACCELARATOR_303AGR, &accDataBuffer[i], 1);
	}

	xraw_f = ((float)( (int16_t)((accDataBuffer[1] << 8) | (accDataBuffer[0]))));
	yraw_f = ((float)( (int16_t)((accDataBuffer[3] << 8) | (accDataBuffer[2]))));
	zraw_f = ((float)( (int16_t)((accDataBuffer[5] << 8) | (accDataBuffer[4]))));

	rotate_accel_3f(&xraw_f, &yraw_f, &zraw_f);

	// mh
	accel_report_x = xraw_f;
	accel_report_y = yraw_f;
	accel_report_z = -zraw_f;		// flip Z in gen 2 hardware

	accel_DX_f = ((int16_t)(accel_report_x));
	accel_DY_f = ((int16_t)(accel_report_y));
	accel_DZ_f = ((int16_t)(accel_report_z));
}


//  ===============================================================================
//	compass_read_LSM303AGR
///
/// output is compass_DX_f, compass_DY_f, compass_DZ_f
//  ===============================================================================
void compass_read_LSM303AGR(void)
{
  uint8_t data;
//	float xraw_f, yraw_f, zraw_f;
//	float mag_report_x, mag_report_y, mag_report_z;

  memset(magDataBuffer,0,6);

	compass_DX_f = 0;
	compass_DY_f = 0;
	compass_DZ_f = 0;

	for(int i=0;i<6;i++)
	{
		data = 0x68 + i;	// OUTX_L_REG_M
		I2C_Master_Transmit( DEVICE_COMPASS_303AGR, &data, 1);
		I2C_Master_Receive(  DEVICE_COMPASS_303AGR, &magDataBuffer[i], 1);
	}

	// mh 160620 flip x and y if flip display
	compass_DX_f = (((int16_t)((magDataBuffer[1] << 8) | (magDataBuffer[0]))));
	compass_DY_f = (((int16_t)((magDataBuffer[3] << 8) | (magDataBuffer[2]))));
	compass_DZ_f = (((int16_t)((magDataBuffer[5] << 8) | (magDataBuffer[4]))));

	// align axis in gen 2 hardware
	compass_DZ_f *= -1;

	return;
}


// --------------------------------------------------------------------------------
// ----------EARLIER COMPONENTS ---------------------------------------------------
// --------------------------------------------------------------------------------

//  ===============================================================================
//	compass_init_HMC5883L
/// @brief	The horrible Honeywell compass chip
///					This might be called several times during calibration
///
/// @param 	fast: 1 is fast mode, 0 is normal mode
/// @param 	gain: 7 is max gain and set with here, compass_calib() might reduce it
//  ===============================================================================
void compass_init_HMC5883L(uint8_t fast, uint8_t gain)
{
	uint8_t write_buffer[4];

	compass_gain = gain;
	uint16_t length = 0;
	write_buffer[0] = 0x00; // 00 = config Register A
	
	if( fast )
		write_buffer[1] = 0x38;	// 0b 0011 1000; // ConfigA: 75Hz, 2 Samples averaged
	else
		write_buffer[1] = 0x68; // 0b 0110 1000; // ConfigA:  3Hz, 8 Samples averaged

	switch(gain)
	{
			case 7:
				write_buffer[2] = 0xE0;  //0b 1110 0000; // ConfigB: gain
				break;
			case 6:
				write_buffer[2] = 0xC0;  //0b 1100 0000; // ConfigB: gain
				break;
			case 5:
				write_buffer[2] = 0xA0;  //0b 1010 0000; // ConfigB: gain
				break;
			case 4:
				write_buffer[2] = 0x80;  //0b 1000 0000; // ConfigB: gain
				break;
			case 3:
				write_buffer[2] = 0x60;  //0b 0110 0000; // ConfigB: gain
				break;
			case 2:
				write_buffer[2] = 0x40;  //0b 01000 0000; // ConfigB: gain
				break;
			case 1:
				write_buffer[2] = 0x20;  //0b 00100 0000; // ConfigB: gain
				break;
			case 0:
				write_buffer[2] = 0x00;  //0b 00000 0000; // ConfigB: gain
				break;
	}
	write_buffer[3] = 0x00; // Mode: continuous mode
	length = 4;
	//hmc_twi_write(0);
	I2C_Master_Transmit(  DEVICE_COMPASS_HMC5883L, write_buffer, length);
}



//  ===============================================================================
//	compass_sleep_HMC5883L
/// @brief	Power-down mode for Honeywell compass chip
//  ===============================================================================
void compass_sleep_HMC5883L(void)
{
	uint8_t write_buffer[4];
	uint16_t length = 0;
	
	write_buffer[0] = 0x00; // 00 = config Register A
	write_buffer[1] = 0x68; // 0b 0110 1000; // ConfigA
	write_buffer[2] = 0x20; // 0b 0010 0000; // ConfigB
	write_buffer[3] = 0x02; // 0b 0000 0010; // Idle Mode
	length = 4;
	I2C_Master_Transmit(  DEVICE_COMPASS_HMC5883L, write_buffer, length);
}


//  ===============================================================================
//	accelerator_init_MMA8452Q
/// @brief	Power-down mode for acceleration chip used in combination with Honeywell compass
//  ===============================================================================
void accelerator_init_MMA8452Q(void)
{
	uint8_t write_buffer[4];
	uint16_t length = 0;
	//HAL_Delay(1);
	//return;
	write_buffer[0] = 0x0E; // XYZ_DATA_CFG
	write_buffer[1] = 0x00;//0b00000000; // High pass Filter=0 , +/- 2g range
	length = 2;
	I2C_Master_Transmit(  DEVICE_ACCELARATOR_MMA8452Q, write_buffer, length);
	//HAL_Delay(1);
	write_buffer[0] = 0x2A; // CTRL_REG1
	write_buffer[1] = 0x34; //0b00110100; // CTRL_REG1: 160ms data rate, St.By Mode, reduced noise mode
	write_buffer[2] = 0x02; //0b00000010; // CTRL_REG2: High Res in Active mode
	length = 3;
	I2C_Master_Transmit(  DEVICE_ACCELARATOR_MMA8452Q, write_buffer, length);
	
	//HAL_Delay(1);
	//hw_delay_us(100);
	write_buffer[0] = 0x2A; // CTRL_REG1
	write_buffer[1] = 0x35; //0b00110101; // CTRL_REG1: ... Active Mode
	length = 2;
	I2C_Master_Transmit(  DEVICE_ACCELARATOR_MMA8452Q, write_buffer, length);
/*	
	HAL_Delay(6);
	compass_read();
	HAL_Delay(1);
	acceleration_read();
	
	compass_calc();
*/	
}


//  ===============================================================================
//	accelerator_sleep_MMA8452Q
/// @brief	compass_sleep_HMC5883L
//  ===============================================================================
void accelerator_sleep_MMA8452Q(void)
{
	uint16_t length = 0;
	uint8_t write_buffer[4];
	
	write_buffer[0] = 0x2A; // CTRL_REG1
	write_buffer[1] = 0x00; //0b00000000; // CTRL_REG1: Standby Mode
	length = 2;
	I2C_Master_Transmit(  DEVICE_ACCELARATOR_MMA8452Q, write_buffer, length);
}


//  ===============================================================================
//	compass_read_HMC5883L
/// @brief	The new ST 303D - get ALL data and store in static variables
///
/// output is compass_DX_f, compass_DY_f, compass_DZ_f
//  ===============================================================================
void compass_read_HMC5883L(void)
{
	uint8_t buffer[20];
	compass_DX_f = 0;
	compass_DY_f = 0;
	compass_DZ_f = 0;
	uint8_t length = 0;
	uint8_t read_buffer[6];
	signed_tword 	data;
	for(int i = 0; i<6;i++)
		read_buffer[i] = 0;
	buffer[0] = 0x03; // 03 = Data Output X MSB Register
	length = 1;
	I2C_Master_Transmit(  DEVICE_COMPASS_HMC5883L, buffer, length);
	length = 6;
	I2C_Master_Receive(  DEVICE_COMPASS_HMC5883L, read_buffer, length);
	

	data.Byte.hi = read_buffer[0];
	data.Byte.low = read_buffer[1];
	//Y = Z
	compass_DY_f	= - data.Word;

	data.Byte.hi = read_buffer[2];
	data.Byte.low = read_buffer[3];
	compass_DZ_f = data.Word;

	data.Byte.hi = read_buffer[4];
	data.Byte.low = read_buffer[5]; 
	//X = -Y
	 compass_DX_f 	= data.Word;
}


//  ===============================================================================
//	acceleration_read_MMA8452Q
/// @brief	The old MMA8452Q used with the Honeywell compass
///					get the acceleration data and store in static variables
///
/// output is accel_DX_f, accel_DY_f, accel_DZ_f
//  ===============================================================================
void  acceleration_read_MMA8452Q(void)
{
	uint8_t buffer[20];
	accel_DX_f = 0;
	accel_DY_f = 0;
	accel_DZ_f = 0;
	uint8_t length = 0;
//	bit8_Type status ;
	uint8_t read_buffer[7];
	signed_tword 	data;
	for(int i = 0; i<6;i++)
		read_buffer[i] = 0;
	buffer[0] = 0x00; // 03 = Data Output X MSB Register
	length = 1;
	I2C_Master_Transmit(  DEVICE_ACCELARATOR_MMA8452Q, buffer, length);
	length = 7;
	I2C_Master_Receive(  DEVICE_ACCELARATOR_MMA8452Q, read_buffer, length);
	
//	status.uw = read_buffer[0];
	data.Byte.hi = read_buffer[1];
	data.Byte.low =  read_buffer[2];
	accel_DX_f =data.Word/16;
	data.Byte.hi = read_buffer[3];
	data.Byte.low =  read_buffer[4];
	accel_DY_f =data.Word/16;
	data.Byte.hi = read_buffer[5];
	data.Byte.low =  read_buffer[6];
	accel_DZ_f =data.Word/16;

	accel_DX_f *= -1;
	accel_DY_f *= -1;
	accel_DZ_f *= -1;
}


//  ===============================================================================
//	compass_calc_roll_pitch_only
/// @brief	only the roll and pitch parts of compass_calc()
///
/// input is accel_DX_f, accel_DY_f, accel_DZ_f
/// output is compass_pitch and compass_roll
//  ===============================================================================
void compass_calc_roll_pitch_only(void)
{
	 float sinPhi, cosPhi, sinTeta, cosTeta;
	 float Phi, Teta;
	 float mx_corr, my_corr;

	 float offsetX, offsetY, offsetZ;
	 float scaleX, scaleY,scaleZ;

	 float local_DX = compass_DX_f;
	 float local_DY = compass_DY_f;
	 float local_DZ = compass_DZ_f;


	 if(local_DX < compass_DX_min) compass_DX_min = local_DX;
	 if(local_DY < compass_DY_min) compass_DY_min = local_DY;
	 if(local_DZ < compass_DZ_min) compass_DZ_min = local_DZ;

	 if(local_DX > compass_DX_max) compass_DX_max = local_DX;
	 if(local_DY > compass_DY_max) compass_DY_max = local_DY;
	 if(local_DZ > compass_DZ_max) compass_DZ_max = local_DZ;

	//---- Calculate sine and cosine of roll angle Phi -----------------------
	Phi= atan2f(accel_DY_f, accel_DZ_f) ;
	compass_roll = Phi * 180.0f /PI;
	sinPhi = sinf(Phi);
	cosPhi = cosf(Phi);

	Teta = atanf(-(float)accel_DX_f/(accel_DY_f * sinPhi + accel_DZ_f * cosPhi));
	compass_pitch = Teta * (180.0 / PI);

	sinTeta = sinf(Teta);
	cosTeta = cosf(Teta);

	offsetX = (compass_DX_max + compass_DX_min) / 2.0;
	offsetY = (compass_DY_max + compass_DY_min) / 2.0;
	offsetZ = (compass_DZ_max + compass_DZ_min) / 2.0;

	scaleX = 2.0 / (compass_DX_max - compass_DX_min);
	scaleY = 2.0 / (compass_DY_max - compass_DY_min);
	scaleZ = 2.0 / (compass_DZ_max - compass_DZ_min);

	local_DX -= offsetX;
	local_DX *= scaleX;

	local_DY -= offsetY;
	local_DY *= scaleY;

	local_DZ -= offsetZ;
	local_DZ *= scaleZ;

	mx_corr = local_DX * cosTeta + local_DZ * sinTeta;
	my_corr = local_DX * sinPhi * sinTeta + local_DY * cosPhi - local_DZ * sinPhi * cosTeta;

	compass_heading = atan2(my_corr, mx_corr) * (180.0 / PI);


    if (compass_heading < 0) {
    	compass_heading += 360.0;
    }
}


//  ===============================================================================
//	compass_calc
/// @brief	all the fancy stuff first implemented in OSTC3
///
/// input is compass_DX_f, compass_DY_f, compass_DZ_f, accel_DX_f, accel_DY_f, accel_DZ_f
/// and compass_CX_f, compass_CY_f, compass_CZ_f
/// output is compass_heading, compass_pitch and compass_roll
//  ===============================================================================
void compass_calc(void)
{
     float	sinPhi, cosPhi, sinTeta, cosTeta;
		 float Phi, Teta, Psi;
     int16_t iBfx, iBfy;
     int16_t iBpx, iBpy, iBpz;
   
    //---- Make hard iron correction -----------------------------------------
    // Measured magnetometer orientation, measured ok.
    // From matthias drawing: (X,Y,Z) --> (X,Y,Z) : no rotation.
    iBpx = compass_DX_f - compass_CX_f; // X
    iBpy = compass_DY_f - compass_CY_f; // Y
    iBpz = compass_DZ_f - compass_CZ_f; // Z

    //---- Calculate sine and cosine of roll angle Phi -----------------------
    //sincos(accel_DZ_f, accel_DY_f, &sin, &cos);
    Phi= atan2f(accel_DY_f, accel_DZ_f) ;
		compass_roll = Phi * 180.0f /PI;
		sinPhi = sinf(Phi);
		cosPhi = cosf(Phi);

		//---- rotate by roll angle (-Phi) ---------------------------------------
    iBfy = iBpy * cosPhi - iBpz * sinPhi;
    iBpz = iBpy * sinPhi + iBpz * cosPhi;
    //Gz = imul(accel_DY_f, sin) + imul(accel_DZ_f, cos);
		
    //---- calculate sin and cosine of pitch angle Theta ---------------------
    //sincos(Gz, -accel_DX_f, &sin, &cos);     // NOTE: changed sin sign.
		// Teta takes into account roll of computer and sends combination of Y and Z :-) understand now hw 160421
		Teta = atanf(-(float)accel_DX_f/(accel_DY_f * sinPhi + accel_DZ_f * cosPhi));
		compass_pitch = Teta * 180.0f /PI;
		sinTeta = sinf(Teta);
		cosTeta = cosf(Teta);
    /* correct cosine if pitch not in range -90 to 90 degrees */
    if( cosTeta < 0 ) cosTeta = -cosTeta;

    ///---- de-rotate by pitch angle Theta -----------------------------------
    iBfx = iBpx *  cosTeta + iBpz * sinTeta;

    //---- Detect uncalibrated compass ---------------------------------------
    if( !compass_CX_f && !compass_CY_f && !compass_CZ_f )
    {
        compass_heading = -1;
        return;
    }

    //---- calculate current yaw = e-compass angle Psi -----------------------
    // Result in degree (no need of 0.01 deg precision...
			Psi = atan2f(-iBfy,iBfx);
		 compass_heading = Psi * 180.0f /PI;
    // Result in 0..360 range:
    if( compass_heading < 0 )
        compass_heading += 360;
}


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // - Calibration - ///////////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* can be lost during sleep as those are reset with compass_reset_calibration() */

//  ===============================================================================
//	compass_reset_calibration
/// @brief	all the fancy stuff first implemented in OSTC3
///
/// output is struct g and compass_CX_f, compass_CY_f, compass_CZ_f
///
/// @param 	g: is a struct with crazy stuff like Suuu, Svvv, Svvu, etc.
///					all is set to zero here
//  ===============================================================================
void compass_reset_calibration(SCompassCalib *g)
{
    g->compass_N = 0;
    g->Su = g->Sv = g->Sw = 0.0;
    g->Suu = g->Svv = g->Sww = g->Suv = g->Suw = g->Svw = 0.0;
    g->Suuu = g->Svvv = g->Swww = 0.0;
    g->Suuv = g->Suuw = g->Svvu = g->Svvw = g->Swwu = g->Swwv = 0.0;
    compass_CX_f = compass_CY_f = compass_CZ_f = 0.0;

    compass_DX_min = 10000;
    compass_DY_min = 10000;
    compass_DZ_min = 10000;

    compass_DX_max = -10000;
    compass_DY_max = -10000;
    compass_DZ_max = -10000;
}


//  ===============================================================================
//	compass_add_calibration
/// @brief	all the fancy stuff first implemented in OSTC3
///
/// input is compass_DX_f, compass_DY_f, compass_DZ_f
/// and compass_CX_f, compass_CY_f, compass_CZ_f
/// output is struct g
///
/// @param 	g: is a struct with crazy stuff like Suuu, Svvv, Svvu, etc.
//  ===============================================================================
void compass_add_calibration(SCompassCalib *g)
{
    float u, v, w;
   
    u = (compass_DX_f - compass_CX_f) / 32768.0f;
    v = (compass_DY_f - compass_CY_f) / 32768.0f;
    w = (compass_DZ_f - compass_CZ_f) / 32768.0f;

    g->compass_N++;
    g->Su += u;
    g->Sv += v;
    g->Sw += w;
    g->Suv += u*v;
    g->Suw += u*w;
    g->Svw += v*w;
    g->Suu  += u*u;
    g->Suuu += u*u*u;
    g->Suuv += v*u*u;
    g->Suuw += w*u*u;
    g->Svv  += v*v;
    g->Svvv += v*v*v;
    g->Svvu += u*v*v;
    g->Svvw += w*v*v;
    g->Sww  += w*w;
    g->Swww += w*w*w;
    g->Swwu += u*w*w;
    g->Swwv += v*w*w;
}

//////////////////////////////////////////////////////////////////////////////

//  ===============================================================================
//	compass_solve_calibration
/// @brief	all the fancy stuff first implemented in OSTC3
///
/// input is compass_CX_f, compass_CY_f, compass_CZ_f and g
/// output is struct g
///
/// @param 	g: is a struct with crazy stuff like Suuu, Svvv, Svvu, etc.
//  ===============================================================================
void compass_solve_calibration(SCompassCalib *g)
{
    float yu, yv, yw;
    float delta;
    float uc, vc, wc;
    

    //---- Normalize partial sums --------------------------------------------
    //
    // u, v, w should be centered on the mean value um, vm, wm:
    // x = u + um, with um = Sx/N
    //
    // So:
    // (u + um)**2 = u**2 + 2u*um + um**2
    // Su = 0, um = Sx/N
    // Sxx = Suu + 2 um Su + N*(Sx/N)**2 = Suu + Sx**2/N
    // Suu = Sxx - Sx**2/N
    yu = g->Su/g->compass_N;
    yv = g->Sv/g->compass_N;
    yw = g->Sw/g->compass_N;

    g->Suu -= g->Su*yu;
    g->Svv -= g->Sv*yv;
    g->Sww -= g->Sw*yw;

    // (u + um)(v + vm) = uv + u vm + v um + um vm
    // Sxy = Suv + N * um vm
    // Suv = Sxy - N * (Sx/N)(Sy/N);
    g->Suv -= g->Su*yv;
    g->Suw -= g->Su*yw;
    g->Svw -= g->Sv*yw;

    // (u + um)**3 = u**3 + 3 u**2 um + 3 u um**2 + um**3
    // Sxxx = Suuu + 3 um Suu + 3 um**2 Su + N.um**3
    // Su = 0, um = Sx/N:
    // Suuu = Sxxx - 3 Sx*Suu/N - N.(Sx/N)**3
    //      = Sxxx - 3 Sx*Suu/N - Sx**3/N**2

    // (u + um)**2 (v + vm) = (u**2 + 2 u um + um**2)(v + vm)
    // Sxxy = Suuv + vm Suu + 2 um (Suv + vm Su) + um**2 (Sv + N.vm)
    //
    // Su = 0, Sv = 0, vm = Sy/N:
    // Sxxy = Suuv + vm Suu + 2 um Suv + N um**2 vm
    //
    // Suuv = Sxxy - (Sy/N) Suu - 2 (Sx/N) Suv - (Sx/N)**2 Sy
    //      = Sxxy - Suu*Sy/N - 2 Suv*Sx/N - Sx*Sx*Sy/N/N
    //      = Sxxy - (Suu + Sx*Sx/N)*Sy/N - 2 Suv*Sx/N
    g->Suuu -= (3*g->Suu + g->Su*yu)*yu;
    g->Suuv -= (g->Suu + g->Su*yu)*yv + 2*g->Suv*yu;
    g->Suuw -= (g->Suu + g->Su*yu)*yw + 2*g->Suw*yu;

    g->Svvu -= (g->Svv + g->Sv*yv)*yu + 2*g->Suv*yv;
    g->Svvv -= (3*g->Svv + g->Sv*yv)*yv;
    g->Svvw -= (g->Svv + g->Sv*yv)*yw + 2*g->Svw*yv;

    g->Swwu -= (g->Sww + g->Sw*yw)*yu + 2*g->Suw*yw;
    g->Swwv -= (g->Sww + g->Sw*yw)*yv + 2*g->Svw*yw;
    g->Swww -= (3*g->Sww + g->Sw*yw)*yw;

    //---- Solve the system --------------------------------------------------
    // uc Suu + vc Suv + wc Suw = (Suuu + Svvu + Swwu) / 2
    // uc Suv + vc Svv + wc Svw = (Suuv + Svvv + Swwv) / 2
    // uc Suw + vc Svw + wc Sww = (Suuw + Svvw + Swww) / 2
    // Note this is symetric, with a positiv diagonal, hence
    // it always have a uniq solution.
    yu = 0.5f * (g->Suuu + g->Svvu + g->Swwu);
    yv = 0.5f * (g->Suuv + g->Svvv + g->Swwv);
    yw = 0.5f * (g->Suuw + g->Svvw + g->Swww);
    delta = g->Suu * (g->Svv * g->Sww - g->Svw * g->Svw)
          - g->Suv * (g->Suv * g->Sww - g->Svw * g->Suw)
          + g->Suw * (g->Suv * g->Svw - g->Svv * g->Suw);

    uc = (yu  * (g->Svv * g->Sww - g->Svw * g->Svw)
       -  yv  * (g->Suv * g->Sww - g->Svw * g->Suw)
       +  yw  * (g->Suv * g->Svw - g->Svv * g->Suw) )/delta;
    vc = (g->Suu * ( yv * g->Sww -  yw * g->Svw)
       -  g->Suv * ( yu * g->Sww -  yw * g->Suw)
       +  g->Suw * ( yu * g->Svw -  yv * g->Suw) )/delta;
    wc = (g->Suu * (g->Svv * yw  - g->Svw * yv )
       -  g->Suv * (g->Suv * yw  - g->Svw * yu )
       +  g->Suw * (g->Suv * yv  - g->Svv * yu ) )/delta;

    // Back to uncentered coordinates:
    // xc = um + uc
    uc = g->Su/g->compass_N + compass_CX_f/32768.0f + uc;
    vc = g->Sv/g->compass_N + compass_CY_f/32768.0f + vc;
    wc = g->Sw/g->compass_N + compass_CZ_f/32768.0f + wc;

    // Then save the new calibrated center:
    compass_CX_f = (short)(32768 * uc);
    compass_CY_f = (short)(32768 * vc);
    compass_CZ_f = (short)(32768 * wc);
}


//  ===============================================================================
//	compass_calib
/// @brief	the main loop for calibration
/// 				output is compass_CX_f, compass_CY_f, compass_CZ_f and g
///					160704 removed -4096 limit for LSM303D
///
/// @return always 0
//  ===============================================================================
int compass_calib_common(void)
{
	SCompassCalib g;

    // Starts with no calibration at all:
	compass_reset_calibration(&g);
	uint32_t tickstart = 0;
	tickstart = HAL_GetTick();
    /* run calibration for one minute */
 	while(time_elapsed_ms(tickstart,HAL_GetTick()) < 60000)
    {
		while((SPI_Evaluate_RX_Data() == 0) && (time_elapsed_ms(tickstart,HAL_GetTick()) < 60000))
		{
			HAL_Delay(1);
		}
		compass_read();
		acceleration_read();
		compass_calc_roll_pitch_only();
			
		if((hardwareCompass == compass_generation1 )		//HMC5883L)
				&&((compass_DX_f == -4096) ||
				 (compass_DY_f == -4096) ||
				 (compass_DZ_f == -4096) ))
		{
			if(compass_gain == 0)
				return -1;
			compass_gain--;
			compass_init(1, compass_gain);
			compass_reset_calibration(&g);
			continue;
		}

		copyCompassDataDuringCalibration(compass_DX_f,compass_DY_f,compass_DZ_f);
		compass_add_calibration(&g);
    }
        
    compass_solve_calibration(&g);
		
    if((hardwareCompass != compass_generation_undef)		/* if compass is not know at this point in time storing data makes no sense */
    	&& (hardwareCompass != COMPASS_NOT_RECOGNIZED))
    {
		tfull32 dataBlock[4];
		dataBlock[0].Word16.low16 = compass_CX_f;
		dataBlock[0].Word16.hi16 = compass_CY_f;
		dataBlock[1].Word16.low16 = compass_CZ_f;
		dataBlock[1].Word16.hi16 = hardwareCompass;
		dataBlock[2].Full32 = 0x7FFFFFFF;
		dataBlock[3].Word16.low16 = 0xFFFF;
		dataBlock[3].Word16.hi16 = BFA_calc_Block_Checksum(dataBlock);
		BFA_writeDataBlock(dataBlock);
    }
	return 0;
}

