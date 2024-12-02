/**
  ******************************************************************************
	* @copyright heinrichs weikamp
  * @file   		logbook.c
  * @author 		heinrichs weikamp gmbh and heinrichs weikamp gmbh
  * @date   		22-Apr-2014
  * @version		V0.0.3
  * @since			03-Feb-2016
  * @brief			Everything about creating and evaluating the logbook
	*							without the flash part which is included in externLogbookFlash.c
	*							and the USB/Bluetooth part in tComm.c
	*							CHANGE V0.0.3 hw: ppO2 sensor values
	*							CHANGE V0.0.4 hw: fix event bytes according to hwos_interface.odt dated 160610 in bitbucket hwOS
	* @bug
	* @warning
  @verbatim
  ==============================================================================
              ##### Header #####
  ==============================================================================
  [..] SLogbookHeader
				The order has changed in comparsion to OSTC3 for perfect alignment
				with 16bit and 32bit. The header is 256kB as well.
				DO NOT rearrange anything but add new data to a second page
				beyond diveHeaderEnd. Use extraPagesWithData to indicate that there is
				data available that was not available in the OSTC3 256KB
				This data will be behind the diveHeaderEnd. DO NOT delete diveHeaderEnd.

	[..] SLogbookHeaderOSTC3
				is the format used by the OSTC3.
				logbook_getHeaderOSTC3() does the job using the global headers in logbook.c

	[..] SSmallHeader
				- is the format used by the OSTC3

	[..] Summary
				The header format is not perfect and might be optimized prior to
				releasing the diving computer. For now it is good to be compatible
				with PC software available for checking the content of the logbook


	@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "logbook.h"
//#include "test_vpm.h"
#include "externLogbookFlash.h"
#include "data_exchange.h"
#include "decom.h"
#include "tHome.h" // for  tHome_findNextStop()
#include "settings.h"
#include "configuration.h"
 
/* Private types -------------------------------------------------------------*/

#define LOGBOOK_VERSION (0x30)
#define LOGBOOK_VERSION_OSTC3 (0x24)

#define DEFAULT_SAMPLES	(100)	/* Number of sample data bytes in case of an broken header information */
#define DUMMY_SAMPLES	(1000)	/* Maximum number of samples profided by a dummy dive profile */

typedef struct /* don't forget to adjust void clear_divisor(void) */
{
	uint8_t temperature;
	uint8_t deco_ndl;
	uint8_t gradientFactor;
	uint8_t ppo2;
	uint8_t decoplan;
	uint8_t cns;
	uint8_t tank;
} SDivisor;

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static SLogbookHeader  gheader;
static SLogbookHeaderOSTC3	headerOSTC3;
static SLogbookHeaderOSTC3compact headerOSTC3compact;
static SSmallHeader smallHeader;
static SDivisor divisor;
static SDivisor divisorBackup;

static SSmallHeader smallDummyHeader;
static uint16_t	dummyWriteIdx;
static uint16_t	dummyReadIdx;
static uint8_t dummyMemoryBuffer[5000];


/* Private function prototypes -----------------------------------------------*/
static void clear_divisor(void);
static void logbook_SetAverageDepth(float average_depth_meter);
static void logbook_SetMinTemperature(float min_temperature_celsius);
static void logbook_SetMaxCNS(float max_cns_percentage);
static void logbook_SetCompartmentDesaturation(const SDiveState * pStateReal);
static void logbook_SetLastStop(float last_stop_depth_bar);
static void logbook_writedata(void * data, int length_byte);
static void logbook_UpdateHeader(const SDiveState * pStateReal);
static void logbook_createDummyProfile(SLogbookHeader* pHeader, uint16_t length, uint16_t* depth, int16_t* temperature, uint16_t* ppo2);

/* Exported functions --------------------------------------------------------*/

void logbook_EndDive(void)
{
	ext_flash_close_new_dive_log((uint8_t*) &gheader);
}


//  ===============================================================================
//	logbook_last_totalDiveCount
/// @brief	Fix setting issues
///	@date    04-April-2016
///
/// @return diveNumber (totalDiveCounter) of latest log entry, 0 if not a valid header
//  ===============================================================================
uint16_t logbook_lastDive_diveNumber(void)
{
	SLogbookHeader  tempLogbookHeader;
	if(logbook_getHeader(0, &tempLogbookHeader))
	{
		return tempLogbookHeader.diveNumber;
	}
	else
	{
		return 0;
	}
}


/**
  ******************************************************************************
  * @brief   logbook_getCurrentHeader. /
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
    * @return  SLogbookHeader*:
*/
SLogbookHeader* logbook_getCurrentHeader(void)
{
    return &gheader;
}

/**
  ******************************************************************************
  * @brief   logbook_getNumberOfHeaders. /
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    18-May-2016
  ******************************************************************************
	*
  * @return  uint8_t : number of valid headers (0xFAFA) found.
*/
uint8_t logbook_getNumberOfHeaders(void)
{
	return ext_flash_count_dive_headers();
}


/**
  ******************************************************************************
  * @brief   logbook_getHeader. /
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
    * @param  StepBackwards : 0 Last lokbook entry, 1 second to last entry, etc.
     * @param  SSLogbookHeader* pLogbookHeader:  Output found LogbookHeader
    * @return  uint8_t : 1 = success
*/
uint8_t logbook_getHeader(uint8_t StepBackwards,SLogbookHeader* pLogbookHeader)
{
    ext_flash_read_dive_header((uint8_t *)pLogbookHeader,  StepBackwards);
    if(pLogbookHeader->diveHeaderStart != 0xFAFA)
        return 0;
    else
        return 1;
}

/**
  ******************************************************************************
  * @brief   logbook_initNewdiveProfile. /
  *           creates header and smallHeader from diveState and global Settings
  *           and writes new lookboock entry on flash device
  *           diveState
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
    * @param  SDiveState* pInfo: Input
    * @param  SSettings* pSettings:  Input
*/

void logbook_initNewdiveProfile(const SDiveState* pInfo, SSettings* pSettings)
{
	RTC_DateTypeDef Sdate;
	RTC_TimeTypeDef Stime;

	for(int i = 0; i < sizeof(SLogbookHeader); i++)
	{
		((uint8_t*)(&gheader))[i] = 0;
	}
	gheader.diveHeaderStart = 0xFAFA;
	gheader.diveHeaderEnd = 0xFBFB;
	gheader.samplingRate = 2;
	if(pInfo->diveSettings.diveMode == DIVEMODE_OC)
  {
    for(int i = 0; i < 5; i++)
    {
      gheader.gasordil[i].oxygen_percentage = pSettings->gas[i+1].oxygen_percentage;
      gheader.gasordil[i].helium_percentage = pSettings->gas[i+1].helium_percentage;
      gheader.gasordil[i].note.uw = pSettings->gas[i+1].note.uw;
      gheader.gasordil[i].depth_meter = pSettings->gas[i+1].depth_meter;
    }
  }
  else
  {
    for(int i = 0; i < 5; i++)
    {
      gheader.gasordil[i].oxygen_percentage = pSettings->gas[i+6].oxygen_percentage;
      gheader.gasordil[i].helium_percentage = pSettings->gas[i+6].helium_percentage;
      gheader.gasordil[i].note.uw = pSettings->gas[i+6].note.uw;
      gheader.gasordil[i].depth_meter = pSettings->gas[i+6].depth_meter;
    }

    for(int i = 0; i < 5; i++)
    {
      gheader.setpoint[i].setpoint_cbar = pSettings->setpoint[i+1].setpoint_cbar;
      gheader.setpoint[i].depth_meter = pSettings->setpoint[i+1].depth_meter;
    }
  }
//	header.gasordil[pInfo->lifeData.actualGas.GasIdInSettings].depth_meter = 0;

	translateDate(pInfo->lifeData.dateBinaryFormat, &Sdate);
	translateTime(pInfo->lifeData.timeBinaryFormat, &Stime);
	gheader.dateYear = Sdate.Year;
	gheader.dateMonth = Sdate.Month;
	gheader.dateDay = Sdate.Date;
	gheader.timeHour = Stime.Hours;
	gheader.timeMinute = Stime.Minutes;
	gheader.cnsAtBeginning = (uint16_t)pInfo->lifeData.cns;
	gheader.surfacePressure_mbar = (uint16_t)(pInfo->lifeData.pressure_surface_bar * 1000);
	gheader.firmwareVersionHigh = firmwareVersion_16bit_high();
	gheader.firmwareVersionLow = firmwareVersion_16bit_low();
	gheader.logbookProfileVersion = LOGBOOK_VERSION;
	gheader.salinity = pSettings->salinity;
	gheader.diveNumber = pSettings->totalDiveCounter;
	gheader.personalDiveCount = pSettings->personalDiveCount;

	gheader.diveMode = pInfo->diveSettings.diveMode;
	gheader.CCRmode = pInfo->diveSettings.CCR_Mode;
	gheader.lastDecostop_m = pSettings->last_stop_depth_meter;

	if(pInfo->diveSettings.deco_type.ub.standard == GF_MODE)
	{
		gheader.decoModel = 1;
		gheader.gfLow_or_Vpm_conservatism = pInfo->diveSettings.gf_low;
		gheader.gfHigh = pInfo->diveSettings.gf_high;
	}
	else
	{
		gheader.decoModel = 2;
		gheader.gfLow_or_Vpm_conservatism = pInfo->diveSettings.vpm_conservatism;
		gheader.gfHigh = 0;
	}

	memcpy(gheader.n2Compartments, pInfo->lifeData.tissue_nitrogen_bar, 64);
	memcpy(gheader.heCompartments, pInfo->lifeData.tissue_helium_bar, 64);

	logbook_SetCompartmentDesaturation(pInfo);

	ext_flash_start_new_dive_log_and_set_actualPointerSample((uint8_t*)&gheader);

	smallHeader.profileLength[0] = 0xFF;
	smallHeader.profileLength[1] = 0xFF;
	smallHeader.profileLength[2] = 0xFF;
	smallHeader.samplingRate_seconds = 2;
	smallHeader.numDivisors = 7;

	smallHeader.tempType = 0;
	smallHeader.tempLength = 2;
	smallHeader.tempDivisor = 6;

	smallHeader.deco_ndlType = 1;
	smallHeader.deco_ndlLength = 2;
	smallHeader.deco_ndlDivisor = 6; //= 6;

	/* GF in % at actual position */
	smallHeader.gfType =  2;
	smallHeader.gfLength = 1;
	smallHeader.gfDivisor = 0; //12;

	/* 3 Sensors: 8bit ppO2 in 0.01bar, 16bit voltage in 0,1mV */
	smallHeader.ppo2Type = 3;
	smallHeader.ppo2Length = 9;
	smallHeader.ppo2Divisor = 2; //2

	/* last 15 stops in minutes (last, second_to_last, ... */
	/* last stop depth is defined in header */
	smallHeader.decoplanType = 4;
	smallHeader.decoplanLength = 15;
	smallHeader.decoplanDivisor = 12;//12;

	smallHeader.cnsType = 5;
	smallHeader.cnsLength = 2;
	smallHeader.cnsDivisor = 12;

	smallHeader.tankType = 6;
#ifdef ENABLE_BOTTLE_SENSOR
	smallHeader.tankLength = 2;
	smallHeader.tankDivisor = 30;	/* log tank data once a minute */
#else
	smallHeader.tankLength = 0;
	smallHeader.tankDivisor = 0;
#endif
	logbook_writedata((void *) &smallHeader,sizeof(smallHeader));

	clear_divisor();
}

/**
  ******************************************************************************
  * @brief   clear_divisor /  clears divisor struct
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
*/
static void clear_divisor(void)
{
	divisor.cns = smallHeader.cnsDivisor - 1;
	divisor.decoplan = smallHeader.decoplanDivisor - 1;
	divisor.deco_ndl = smallHeader.deco_ndlDivisor - 1;
	divisor.gradientFactor = smallHeader.gfDivisor -1 ;
	divisor.ppo2 = smallHeader.ppo2Divisor - 1;
	divisor.tank = smallHeader.tankDivisor - 1;
	divisor.temperature = smallHeader.tempDivisor - 1;
}


/**
  ******************************************************************************
  * @brief   add16. /  adds 16 bit variable to 8 bit array
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
    * @param  uint8_t *pos: Output 8 bit array
    * @param  uint16_t var:  16 bit variable
*/
static void addU16(uint8_t *pos, uint16_t var)
{
       *((uint16_t*)pos) = var;
}

static void addS16(uint8_t *pos, int16_t var)
{
       *((int16_t*)pos) = var;
}

/**
  ******************************************************************************
  * @brief   logbook_writeSample. /  Writes one logbook sampl
  * @author  heinrichs weikamp
  * @date    22-April-2014
  * @version V0.0.2
  * @since   20-June-2016
	* @bug		 Deco/NDL Status fixed in V0.0.2


******************************************************************************
	*
  * @param  SDiveState state:
  */

void logbook_writeSample(const SDiveState *state)
{
    uint8_t sample[256];
  //  int position = 0;
    int length = 0;
//    _Bool bEvent = 0;
    uint8_t nextstopDepthMeter = 0;
		uint16_t nextstopLengthSeconds = 0;
		uint8_t	nextstopLengthMinutes = 0;
    bit8_Type eventByte1, eventByte2;
    bit8_Type profileByteFlag;
    int i = 0;
    for(i = 0; i <256 ;i++)
            sample[i] = 0;
    addU16(sample, (uint16_t)(state->lifeData.depth_meter * 100));
    length += 2;
    sample[2] = 0;
    length++;
    eventByte1.uw = 0;
    eventByte2.uw = 0;
    //uint16_t tmpU16 = 0;
	const SDecoinfo * pDecoinfo; // new hw 160620

    //BuildEevntyte 1
		// sub old 0-3 only one at a time
    if(state->events.manualMarker)
    {
        eventByte1.uw = 6;
    }
		else
    if(state->warnings.decoMissed)
    {
        eventByte1.uw = 2;
    }
		else
    if(state->warnings.ppO2Low)
    {
        eventByte1.uw = 4;
    }
		else
    if(state->warnings.ppO2High)
    {
        eventByte1.uw = 5;
    }
		else
    if(state->warnings.lowBattery)
    {
        eventByte1.uw = 7;
    }
		else
    if(state->warnings.slowWarning)
    {
        eventByte1.uw = 1;
    }
		// sub bit 4 to 7
    if(state->events.manualGasSet)
    {
        eventByte1.ub.bit4 = 1;
    }
    if(state->events.gasChange)
    {
        eventByte1.ub.bit5 = 1;
    }
    if(state->events.setpointChange)
    {
        eventByte1.ub.bit6 = 1;
    }
		// sub bit 7 + eventbyte2
    if(state->events.bailout)
    {
        eventByte1.ub.bit7 = 1;
        eventByte2.ub.bit0 = 1;
    }
    if (state->events.compassHeadingUpdate) {
        eventByte1.ub.bit7 = 1;
        eventByte2.ub.bit1 = 1;
    }
    //Add EventByte 1
    if(eventByte1.uw > 0)
    {
        sample[length] = eventByte1.uw;
        length++;
    }
    if(eventByte2.uw > 0)
    {
        sample[length] = eventByte2.uw;
        length++;
    }
    //Add EventInfos
    if(state->events.manualGasSet)
    {
        //manual gas in %O2 & %He
        sample[length] = state->events.info_manualGasSetO2;
        length += 1;
        sample[length] = state->events.info_manualGasSetHe;
        length += 1;
    }
    if(state->events.gasChange)
    {
        //Current gas (gasid)
        sample[length] = state->events.info_GasChange;
        length += 1;
    }
    if(state->events.setpointChange)
    {
        //New setpoint in cbar
        sample[length] = state->events.info_SetpointChange;
        length += 1;
    }
    if(state->events.bailout)
    {
      //bailout gas in % O2 & %He
        sample[length] = state->events.info_bailoutO2;
        length += 1;
        sample[length] = state->events.info_bailoutHe;
        length += 1;
    }
    if (state->events.compassHeadingUpdate) {
        // New heading and type of heading
        sample[length++] = state->events.info_compassHeadingUpdate & 0xFF;
        sample[length++] = (state->events.info_compassHeadingUpdate & 0xFF00) >> 8;
    }


    if(divisor.temperature == 0)
    {
			divisor.temperature = smallHeader.tempDivisor - 1;
			addS16(&sample[length], (int16_t)((state->lifeData.temperature_celsius * 10.0f) + 0.5f));
			length += 2;
    }
    else
    {
        divisor.temperature--;
    }
		
		
    if(smallHeader.deco_ndlDivisor > 0)
    {
      if(divisor.deco_ndl == 0)
      {
				divisor.deco_ndl  = smallHeader.deco_ndlDivisor - 1;
				
				if(stateUsed->diveSettings.deco_type.ub.standard == GF_MODE)
					pDecoinfo = &stateUsed->decolistBuehlmann;
				else if(stateUsed->diveSettings.deco_type.ub.standard == VPM_MODE)
					pDecoinfo = &stateUsed->decolistVPM;
				else // should not happen as only GF and VPM at the moment
				{
					sample[length] = 0;
					length += 1;
					sample[length] = 0;
					length += 1;
					pDecoinfo = &stateUsed->decolistBuehlmann;	/* use GF per default if something went wrong */
				}
					
				if(pDecoinfo->output_ndl_seconds > 0)
				{
					sample[length] = 0;
					length += 1;
					sample[length] = (uint8_t)(pDecoinfo->output_ndl_seconds / 60);

					// Limit stored sample within 0 to 240 mins (Since it's 8bit UINT only)
					if ((pDecoinfo->output_ndl_seconds / 60) > 240) sample[length] = 240;
					if ((pDecoinfo->output_ndl_seconds / 60) < 0) sample[length] = 0;

					length += 1;
				}
				else if(pDecoinfo->output_time_to_surface_seconds)
				{
					tHome_findNextStop(pDecoinfo->output_stop_length_seconds, &nextstopDepthMeter, &nextstopLengthSeconds);
					nextstopLengthMinutes = (nextstopLengthSeconds +59 ) / 60;
					
					sample[length] = nextstopDepthMeter;
					length += 1;
					sample[length] = nextstopLengthMinutes;
					length += 1;
				}
				else
				{
					sample[length] = 0;
					length += 1;
					sample[length] = 0;
					length += 1;
				}
      }
      else
      {
          divisor.deco_ndl --;
      }
    }

		
    if(smallHeader.ppo2Divisor)
    {
      if(divisor.ppo2 == 0)
      {
          divisor.ppo2 = smallHeader.ppo2Divisor - 1;

        for(int i = 0; i <3; i++)
        {
          sample[length] = (uint8_t)(state->lifeData.ppO2Sensor_bar[i] * 100.0f + 0.5f);
          length += 1;
          addU16(&sample[length], (uint16_t)(state->lifeData.sensorVoltage_mV[i] * 10.0f + 0.5f));
          length += 2;
        }
      }
      else
      {
          divisor.ppo2--;
      }
    }


    if(smallHeader.decoplanDivisor)
    {
      if(divisor.decoplan == 0)
      {
          divisor.decoplan  = smallHeader.decoplanDivisor - 1;
          if(state->diveSettings.deco_type.ub.standard == VPM_MODE)
          {
            for(int i = 0; i <15; i++)
            {
              sample[length] = state->decolistVPM.output_stop_length_seconds[i] / 60;
              length += 1;
            }
          }
          else if(state->diveSettings.deco_type.ub.standard == GF_MODE)
          {
            for(int i = 0; i <15; i++)
            {
              sample[length] = state->decolistBuehlmann.output_stop_length_seconds[i] / 60;
              length += 1;
            }
          }
          else
          {
             for(int i = 0; i <15; i++)
             {
                sample[length] = 0;
                length += 1;
             }
          }
         // add16(&sample[length], state.temperature);
          //length += 2;
      }
      else
      {
          divisor.decoplan --;
      }
    }
    if(divisor.cns == 0)
    {
        divisor.cns = smallHeader.cnsDivisor - 1;
        addU16(&sample[length], (uint16_t)state->lifeData.cns);
        length += 2;
    }
    else
    {
        divisor.cns--;
    }

#ifdef ENABLE_BOTTLE_SENSOR
    if(smallHeader.tankDivisor)
    {
      if(divisor.tank == 0)
      {
          divisor.tank = smallHeader.tankDivisor - 1;
  		  addS16(&sample[length], ((state->lifeData.bottle_bar[state->lifeData.actualGas.GasIdInSettings])));
  		  length += smallHeader.tankLength;
      }
      else
      {
          divisor.tank--;
      }
    }
#endif

    profileByteFlag.uw = length - 3;
    if(eventByte1.uw)
    {
        profileByteFlag.ub.bit7 = 1;
    }
    sample[2] = profileByteFlag.uw;
    logbook_writedata((void *) sample,length);

}

/**
  ******************************************************************************
  * @brief   readSample. /  Reads data of one logbook sample
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  int32_t* depth: output Value
  * @param  int16_t * gasid: output Value
  * @param  int32_t* temperature: output Value
  * @param  int32_t* sensor1, sensor2, sensor3: output Value
  * @param  int32_t* cns: output Value
  * @return bytes read / 0 = reading Error
  */
static uint16_t readSample(int32_t* depth, int16_t * gasid, int16_t* setpoint_cbar, int32_t* temperature, int32_t* sensor1, int32_t* sensor2,
						   int32_t* sensor3, int32_t* cns, SManualGas* manualGas, int16_t* bailout, int16_t* decostopDepth, uint16_t* tank, uint8_t* event)
{
	int length = 0;
	_Bool bEvent = 0;
	bit8_Type eventByte1, eventByte2;
	bit8_Type profileByteFlag;

	eventByte1.uw = 0;
	eventByte2.uw = 0;
	uint8_t tempU8 = 0;
	uint16_t temp = 0;
	uint16_t bytesRead = 0;

	if(gasid)
		*gasid = -1;
	if(temperature)
			*temperature = -1000;
	if(sensor1)
			*sensor1 = -1;
	if(sensor2)
			*sensor2 = -1;
	if(sensor3)
			*sensor3 = -1;
	if(cns)
			*cns = -1;
  if(setpoint_cbar)
    *setpoint_cbar = -1;
  if(bailout)
    *bailout = -1;
  if(tank)
	  *tank = 0;

  if(manualGas)
  {
    manualGas->percentageO2 =-1;
    manualGas->percentageHe =-1;
  }
  if(decostopDepth)
    *decostopDepth = -1;

	ext_flash_read_next_sample_part( (uint8_t*)&temp, 2);
	if(depth)
        *depth = (int32_t)temp;
	bytesRead += 2;

	ext_flash_read_next_sample_part( &profileByteFlag.uw, 1);
	bytesRead ++;

	bEvent = profileByteFlag.ub.bit7;
	profileByteFlag.ub.bit7 = 0;
	length = profileByteFlag.uw;

	if(bEvent)
	{
			ext_flash_read_next_sample_part( &eventByte1.uw, 1);
			bytesRead ++;

			length--;

			/* marker */
			if((eventByte1.ub.bit1 && eventByte1.ub.bit2 && !eventByte1.ub.bit0) && (event != NULL))		/* 3 lsb low bit means battery low */
			{
				*event = 1;
			}

			//second event byte
			if(eventByte1.ub.bit7)
			{
				ext_flash_read_next_sample_part( &eventByte2.uw, 1);
				bytesRead ++;
				length--;
			}
			else
			{
				eventByte2.uw = 0;
			}
		
			//manual Gas Set
			if( eventByte1.ub.bit4)
			{
          //Evaluate manual Gas
				ext_flash_read_next_sample_part( (uint8_t*)&tempU8, 1);
				bytesRead +=1;
				length -= 1;
				manualGas->percentageO2 = tempU8;
				ext_flash_read_next_sample_part( (uint8_t*)&tempU8, 1);
				bytesRead +=1;
				length -= 1;
				manualGas->percentageHe = tempU8;
				if(gasid) *gasid = 0;
			}
			//gas change
			if( eventByte1.ub.bit5)
			{
					ext_flash_read_next_sample_part( &tempU8, 1);
					bytesRead +=1;
					length -= 1;
					if(gasid)
							*gasid = (uint16_t)tempU8;
			}
			//SetpointChange
			if( eventByte1.ub.bit6)
			{
					ext_flash_read_next_sample_part( &tempU8, 1);
					*setpoint_cbar = tempU8;
					bytesRead +=1;
					length -= 1;
			}
			
			// second event Byte
			//bailout
			if(eventByte2.ub.bit0)
			{
				//evaluate bailout gas Gas
				 *bailout = 1;

				ext_flash_read_next_sample_part( (uint8_t*)&tempU8, 1);
				bytesRead +=1;
				length -= 1;
				manualGas->percentageO2 = tempU8;
				ext_flash_read_next_sample_part( (uint8_t*)&tempU8, 1);
				bytesRead +=1;
				length -= 1;
				manualGas->percentageHe = tempU8;

				if(gasid)
						*gasid = 0;
			}
	}

	if(divisor.temperature == 0)
	{
			divisor.temperature = smallHeader.tempDivisor - 1;
			ext_flash_read_next_sample_part( (uint8_t*)&temp, 2);
			bytesRead +=2;
			length -= 2;
			if(temperature)
			{
					*temperature = (int32_t)temp;
			}
	}
	else
	{
			divisor.temperature--;
	}

	if(smallHeader.deco_ndlDivisor > 0)
  {
    if(divisor.deco_ndl == 0)
    {
      divisor.deco_ndl = smallHeader.deco_ndlDivisor - 1;
      ext_flash_read_next_sample_part( &tempU8, 1);
			if(decostopDepth)
			{
				*decostopDepth = tempU8 * 100;
			}
      ext_flash_read_next_sample_part( &tempU8, 1);
      bytesRead += 2;
      length -= 2;
    }
    else
    {
        divisor.deco_ndl--;
    }
  }

	if(divisor.ppo2 == 0)
	{
      int32_t ppO2Tmp = 0;
			divisor.ppo2 = smallHeader.ppo2Divisor -1;
			for(int i = 0; i <3 ; i++)
      {
        ext_flash_read_next_sample_part( &tempU8, 1);
        ppO2Tmp += tempU8;
        bytesRead +=1;
        length -= 1;
        ext_flash_read_next_sample_part( (uint8_t*)&temp, 2);
        bytesRead +=2;
        length -= 2;
				if(sensor1 && (i==0))
					*sensor1 = (((int32_t)tempU8) * 0xFFFF) + temp;
				if(sensor2 && (i==1))
					*sensor2 = (((int32_t)tempU8) * 0xFFFF) + temp;
				if(sensor3 && (i==2))
					*sensor3 = (((int32_t)tempU8) * 0xFFFF) + temp;
      }
	}
	else
	{
			divisor.ppo2--;
	}

  if(smallHeader.decoplanDivisor > 0)
  {
    if(divisor.decoplan == 0)
    {
      divisor.decoplan = smallHeader.decoplanDivisor - 1;
      for(int i = 0; i <15; i++)
        ext_flash_read_next_sample_part( &tempU8, 1);
      bytesRead += 15;
      length -= 15;
    }
    else
    {
        divisor.decoplan--;
    }
  }

	
	
	if(divisor.cns == 0)
	{
			 divisor.cns = smallHeader.cnsDivisor - 1;

      ext_flash_read_next_sample_part( (uint8_t*)&temp, 2);
			bytesRead +=2;
			length -= 2;
			if(cns)
			{
					*cns = (int32_t)temp;
			}
	}
	else
	{
			divisor.cns--;
	}

	if(smallHeader.tankDivisor)
	{
		if(divisor.tank == 0)
		{
				divisor.tank = smallHeader.tankDivisor - 1;
				ext_flash_read_next_sample_part( (uint8_t*)&temp, 2);
				bytesRead +=2;
				length -= 2;
				if(tank)
				{
						*tank = (uint16_t)temp;
				}
		}
		else
		{
				divisor.tank--;
		}
	}

	if (length != 0)
			return 0;

	return bytesRead;
}
/**
  ******************************************************************************
  * @brief   logbook_readSampleData. /  Reads sample data of whole logbook entry
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  uint8_t StepBackwards: witch lookbook entry?
  * @param  uint16_t length : maxlength of output arrays
  * @param  int32_t* depth : output  array
  * @param  int16_t * gasid : output  array
  * @param  int32_t* temperature : output  array
  * @param  int32_t* ppo2 : output  array
  * @param  int32_t* cns : output  array
  * @return length of output
  */
uint16_t logbook_readSampleData(uint8_t StepBackwards, uint16_t length,uint16_t* depth, uint8_t*  gasid, int16_t* temperature, uint16_t* ppo2,
							    uint16_t* setpoint, uint16_t* sensor1, uint16_t* sensor2, uint16_t* sensor3, uint16_t* cns, uint8_t* bailout,
								uint16_t* decostopDepth, uint16_t* tank, uint8_t* event)
{
     //Test read
    //SLogbookHeader header;

    //logbook_getHeader(&header);
    SLogbookHeader header;
    int16_t iNum;
    int16_t firstgasid = 0;
    uint16_t retVal = 0;
    int16_t compression = 0;
    int16_t i;
   // uint32_t diveTime_seconds;
    int32_t depthVal = 0;
    int16_t  gasidVal = 0;
    int16_t setPointVal = 0;
    int16_t bailoutVal = 0;
    int16_t bailoutLast = 0;
    uint16_t setPointLast = 0;
    int32_t temperatureVal = 0;
    int32_t sensor1Val = 0;
    int32_t sensor2Val = 0;
    int32_t sensor3Val = 0;
    int32_t sensor1Last = 0;
    int32_t sensor2Last = 0;
    int32_t sensor3Last = 0;
    int32_t cnsVal = 0;
    int32_t depthLast = 0;
    int16_t  gasidLast = 0;
    int32_t temperatureLast = 0;
    int32_t temperatureFirst = 0;
    int32_t cnsLast = 0;
	int16_t decostepDepthVal = 0;
	int16_t decostepDepthLast = 0;
	uint16_t tankVal = 0;
	uint32_t small_profileLength = 0;
	uint8_t eventdata;

     SManualGas manualGasVal;
     SManualGas manualGasLast;
     manualGasLast.percentageO2 = 0;
     manualGasLast.percentageHe = 0;
     uint16_t numSamples = 0;

     float ambiant_pressure_bar = 0;
     float ppO2 = 0;
    ext_flash_read_dive_header((uint8_t*)&header,  StepBackwards);
    for(i = 0;i< 5;i++)
    {
        if(header.gasordil[i].note.ub.first)
            break;
    }
    firstgasid = i + 1;
    if(isLoopMode(header.diveMode))
      setPointLast = header.setpoint[0].setpoint_cbar;
    else
      setPointLast = 0;
    //diveTime_seconds = header.diveTime_seconds ;
    for(compression = 1; compression < 100; compression ++)
    {
    	numSamples = (header.total_diveTime_seconds / header.samplingRate)/compression;
        if(numSamples <= length)
        {
        	break;
        }
    }


    for(i = 0;i< length;i++)
    {
        if(depth)
            depth[i] = 0;
        if(temperature)
            temperature[i] = 0;
        if(gasid)
            gasid[i] = 0;
        if(ppo2)
            ppo2[i] = 0;
        if(setpoint)
            setpoint[i] = 0;
        if(sensor1)
            sensor1[i] = 0;
        if(sensor2)
            sensor2[i] = 0;
        if(sensor3)
            sensor3[i] = 0;
        if(cns)
            cns[i] = 0;
        if(tank)
        	tank[i] = 0;
    }
    //We start with fist gasid
    gasidLast = firstgasid;


		//uint16_t* ppo2, uint16_t* cns#
    uint32_t totalNumberOfBytes = 0;
    uint32_t bytesRead = 0;
    ext_flash_open_read_sample( StepBackwards,&totalNumberOfBytes);
    ext_flash_read_next_sample_part((uint8_t*)&smallHeader,  sizeof(SSmallHeader));
    bytesRead += sizeof(SSmallHeader);

    clear_divisor();

    iNum = 0;
    int counter = 0;
		temperatureLast = -1000;

	small_profileLength = (smallHeader.profileLength[2] << 16) + (smallHeader.profileLength[1] << 8) + smallHeader.profileLength[0];

	if(totalNumberOfBytes == small_profileLength)	/* sizes provided by header and small header are the same => real data */
	{
		while ((bytesRead < totalNumberOfBytes) && (iNum < length))
		{
				ext_flash_set_entry_point();
				divisorBackup = divisor;
				retVal = readSample(&depthVal,&gasidVal, &setPointVal, &temperatureVal, &sensor1Val, &sensor2Val, &sensor3Val, &cnsVal, &manualGasVal,
									&bailoutVal, &decostepDepthVal, &tankVal, &eventdata);

				if(retVal == 0)
				{
						//Error try to read again!!!
						ext_flash_reopen_read_sample_at_entry_point();
						divisor = divisorBackup;
						retVal = readSample(&depthVal,&gasidVal,&setPointVal, &temperatureVal, &sensor1Val, &sensor2Val, &sensor3Val, &cnsVal,
											&manualGasVal, &bailoutVal, &decostepDepthVal, &tankVal, &eventdata);

						if(retVal == 0)
								break;
				}
				bytesRead +=retVal;

				//if for some variable no new value is in the sample for (z.B. gasidVal = -1), we take the last value
				if(depthVal == -1)
						depthVal = depthLast;
				else
						depthLast = depthVal;

				if(gasidVal == -1)
						gasidVal = gasidLast;
				else
						gasidLast = gasidVal;

				if(temperatureVal == -1000)
						temperatureVal = temperatureLast;
				else
				{
					if(temperatureLast == -1000)
						temperatureFirst = temperatureVal;
					temperatureLast = temperatureVal;
				}

				if(setPointVal == -1)
					setPointVal = setPointLast;
				else
					setPointLast = setPointVal;

				if(sensor1Val == -1)
						sensor1Val = sensor1Last;
				else
						sensor1Last = sensor1Val;

				if(sensor2Val == -1)
						sensor2Val = sensor2Last;
				else
						sensor2Last = sensor2Val;

				if(sensor3Val == -1)
						sensor3Val = sensor3Last;
				else
						sensor3Last = sensor3Val;

				if(cnsVal == -1)
						cnsVal = cnsLast;
				else
						cnsLast = cnsVal;

				if(manualGasVal.percentageO2 == -1)
					manualGasVal = manualGasLast;
				else
					manualGasLast = manualGasVal;

				if(bailoutVal == -1)
					bailoutVal = bailoutLast;
				else
					bailoutLast = bailoutVal;

				if(decostepDepthVal == -1)
						decostepDepthVal = decostepDepthLast;
				else
						decostepDepthLast = decostepDepthVal;

				counter++;
				// Heed compression
				// Write here to arrays
				if(counter == compression)
				{
					if(depth)
						depth[iNum] = depthVal;
					if(gasid)
						gasid[iNum] = gasidVal;
					if(temperature)
						temperature[iNum] = temperatureVal;
					if(cns)
						cns[iNum] = cnsVal;
					if(bailout)
						bailout[iNum] = bailoutVal;
					if(decostopDepth)
						decostopDepth[iNum] = decostepDepthVal;

					if(ppo2)
					{
						//Calc ppo2 - Values
						SGas gas;
						gas.setPoint_cbar = setPointVal;
						if(gasidVal > 0)
						{
							if((gasidVal >= NUM_GASES) && (header.diveMode == DIVEMODE_PSCR))		/* in case gas switches the absolute gas ID is used => map to the 0..NUM_GASES index used in header */
							{
								gasidVal -= NUM_GASES;
							}
							gas.helium_percentage = header.gasordil[gasidVal - 1].helium_percentage;
							gas.nitrogen_percentage = 100 -  gas.helium_percentage - header.gasordil[gasidVal - 1].oxygen_percentage;
						}
						else
						{
							gas.helium_percentage = manualGasVal.percentageHe;
							gas.nitrogen_percentage = 100 -  gas.helium_percentage - manualGasVal.percentageO2;
						}
						ambiant_pressure_bar =((float)(depthVal + header.surfacePressure_mbar))/1000;

						if(header.diveMode == DIVEMODE_PSCR)
						{
							ppO2 = decom_calc_SimppO2(ambiant_pressure_bar, &gas);
						}
						else	/* open circuit calculation */
						{
							ppO2 = decom_calc_ppO2(ambiant_pressure_bar, &gas);
						}
						ppo2[iNum] = (uint16_t) ( ppO2 * 100);
					}

					if(tank)
					{
						tank[iNum] = tankVal;
					}
					if(setpoint)
						setpoint[iNum] = setPointVal;

					if(sensor1)
						sensor1[iNum] = (sensor1Val / 0xFFFF) & 0xFF;
					if(sensor2)
						sensor2[iNum] = (sensor2Val / 0xFFFF) & 0xFF;
					if(sensor3)
						sensor3[iNum] = (sensor3Val / 0xFFFF) & 0xFF;
					iNum++;
					counter = 0;

					if(event)
					{
						event[iNum] = eventdata;
						eventdata = 0;
					}
				}
		}
	}
	else
	{
		logbook_createDummyProfile(&header, numSamples, depth, temperature, ppo2);
		iNum = numSamples;
	}
		
		// Fix first Temperature Entries 150930 hw
		if(temperature)
		{
			int i = 0;
			while((temperature[i] == -1000) && (i < iNum))
				temperature[i++] = temperatureFirst;
		}
	
    ext_flash_close_read_sample();
    return iNum;
}


/********************************************************************************
 * @brief   logbook_InitAndWrite. /  Controls writing of logbook
 *          Should be called ten times per second
 *          Automatically Initializes logbook at beginning of dive,
 *          write samples every 2 seconds
 *          and finishes logbook after end of dive
*********************************************************************************/

void logbook_InitAndWrite(const SDiveState *pStateReal)
{
	SSettings *pSettings = settingsGetPointer();
	static uint8_t bDiveMode = 0;
	static uint32_t tickstart = 0;
	uint32_t ticksdiff = 0;
	uint32_t lasttick = 0;
	static float min_temperature_float_celsius = 0;

	if(!bDiveMode)
	{
		if((pStateReal->mode == MODE_DIVE) && (pStateReal->diveSettings.diveMode != DIVEMODE_Apnea) && (pStateReal->lifeData.dive_time_seconds >= 5))
		{
			//InitdiveProfile
			pSettings->totalDiveCounter++;
			logbook_initNewdiveProfile(pStateReal,settingsGetPointer());
			min_temperature_float_celsius = pStateReal->lifeData.temperature_celsius;
			//Write logbook sample
			logbook_writeSample(pStateReal);
			resetEvents(pStateReal);
			tickstart = HAL_GetTick();
			bDiveMode = 1;
		}
	}
	else if((pStateReal->mode == MODE_DIVE) && (pStateReal->diveSettings.diveMode != DIVEMODE_Apnea))
	{
		lasttick = HAL_GetTick();
		ticksdiff = time_elapsed_ms(tickstart,lasttick);
		//
		if(ticksdiff >= 2000)
		{
			//Write logbook sample
			logbook_writeSample(pStateReal);
			resetEvents(pStateReal);
			if(min_temperature_float_celsius > pStateReal->lifeData.temperature_celsius)
				min_temperature_float_celsius = pStateReal->lifeData.temperature_celsius;
			tickstart = lasttick;
			if((bDiveMode == 1) && (pStateReal->lifeData.dive_time_seconds >= pSettings->divetimeToCreateLogbook))
			{
				ext_flash_create_new_dive_log((uint8_t*)&gheader);
				/** save settings
					* with new lastDiveLogId and time and day
					*/
				pSettings->personalDiveCount++;
				if(pSettings->logbookOffset)
				{
					pSettings->logbookOffset++;
				}
				ext_flash_write_settings(0);
				ext_flash_disable_protection_for_logbook();

				ext_flash_CloseSector();	/* this is just a repair function which invalidates a not used sector in case a log maintenance was called before dive */
				bDiveMode = 3;
			}
			if(bDiveMode == 3)
				logbook_UpdateHeader(pStateReal);
		}
	}
	else if(bDiveMode == 3)
	{
		//End of Dive
		logbook_SetAverageDepth(pStateReal->lifeData.average_depth_meter);
		logbook_SetMinTemperature(min_temperature_float_celsius);
		logbook_SetMaxCNS(pStateReal->lifeData.cns);
		logbook_SetCompartmentDesaturation(pStateReal);
		logbook_SetLastStop(pStateReal->diveSettings.last_stop_depth_bar);
		gheader.batteryVoltage = pStateReal->lifeData.battery_voltage * 1000;
		if(pStateReal->lifeData.battery_charge > 0.0)
		{
			gheader.batteryCharge = pStateReal->lifeData.battery_charge;
		}
		else
		{
			gheader.batteryCharge = 0.0;
		}
		logbook_EndDive();
		bDiveMode = 0;
	} else
	{
	  ext_flash_enable_protection();
	}
}


/* Private functions ---------------------------------------------------------*/

/********************************************************************************
	* @brief   logbook_UpdateHeader. /
	*					 set date, time, max depth. etc. pp.
	*					 the internal pointer to the end of profile and length will be set by
						 ext_flash_close_new_dive_log() in externLogbookFlash.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    27-Nov-2014
*********************************************************************************/
static void logbook_UpdateHeader(const SDiveState *pStateReal)
{
//	uint16_t secondsAtShallow = 0;
	RTC_DateTypeDef Sdate;
	RTC_TimeTypeDef Stime;
	uint32_t time1_u32, time2_u32;
	uint32_t divetimeHelper;

	/* time and day */
	/* don't update CHANGE 160224 hw, maybe save actual time and date at other place
	translateDate(pStateReal->lifeData.dateBinaryFormat, &Sdate);
	translateTime(pStateReal->lifeData.timeBinaryFormat, &Stime);

	header.dateYear = Sdate.Year;
	header.dateMonth = Sdate.Month;
	header.dateDay = Sdate.Date;
	header.timeHour = Stime.Hours;
	header.timeMinute = Stime.Minutes;
	*/
	/// 160315 Quick fix for empty date problem
	if((!(gheader.dateYear)) || (!(gheader.dateMonth)) || (!(gheader.dateDay)))
	{
		translateDate(pStateReal->lifeData.dateBinaryFormat, &Sdate);
		translateTime(pStateReal->lifeData.timeBinaryFormat, &Stime);

		gheader.dateYear = Sdate.Year;
		gheader.dateMonth = Sdate.Month;
		gheader.dateDay = Sdate.Date;
		
		time1_u32 = (uint32_t)gheader.timeMinute + (uint32_t)(gheader.timeHour * 60);
		time2_u32 = (uint32_t)Stime.Minutes + (uint32_t)(Stime.Hours * 60);
		if(time2_u32 < time1_u32)
		{
			if(gheader.dateDay > 1)
			{
				gheader.dateDay -= 1;
			}
			else
			{
				gheader.dateMonth --;
				if(!gheader.dateMonth)
				{
					gheader.dateYear--;
					gheader.dateMonth = 12;
					gheader.dateDay = 31;
				}
				else
				{
					if(gheader.dateMonth == 2)
						gheader.dateDay = 28;
					else
					if((gheader.dateMonth == 4) || (gheader.dateMonth == 6) || (gheader.dateMonth == 9) || (gheader.dateMonth == 11))
						gheader.dateDay = 30;
					else
						gheader.dateDay = 31;
				}
			}
		}
	}
	
	/* duration */
	gheader.total_diveTime_seconds = pStateReal->lifeData.dive_time_seconds;
	gheader.maxDepth = pStateReal->lifeData.max_depth_meter * 100;

	/* old:
	
	secondsAtShallow = pSettings->timeoutDiveReachedZeroDepth;
	if(pStateReal->lifeData.dive_time_seconds <= secondsAtShallow)
		secondsAtShallow = 0;
	header.diveTimeMinutes = (header.total_diveTime_seconds - secondsAtShallow )/ 60;
	header.diveTimeSeconds = header.total_diveTime_seconds - secondsAtShallow - (header.diveTimeMinutes * 60);
	*/
	divetimeHelper = pStateReal->lifeData.dive_time_seconds_without_surface_time;
	gheader.diveTimeMinutes = (uint16_t)(divetimeHelper/60);
	divetimeHelper -= 60 * (uint32_t)gheader.diveTimeMinutes;
	gheader.diveTimeSeconds = (uint16_t)divetimeHelper;
	
	/* deco algorithm (final) */
	if(pStateReal->diveSettings.deco_type.ub.standard == GF_MODE)
	{
		gheader.decoModel = 1;
		gheader.gfLow_or_Vpm_conservatism = pStateReal->diveSettings.gf_low;
		gheader.gfHigh = pStateReal->diveSettings.gf_high;
	}
	else
	{
		gheader.decoModel = 2;
		gheader.gfLow_or_Vpm_conservatism = pStateReal->diveSettings.vpm_conservatism;
		gheader.gfHigh = 0;
	}

	/* tissue load */
	memcpy(gheader.n2Compartments, pStateReal->lifeData.tissue_nitrogen_bar, 64);
	memcpy(gheader.heCompartments, pStateReal->lifeData.tissue_helium_bar, 64);

}


static void logbook_SetAverageDepth(float average_depth_meter)
{
		gheader.averageDepth_mbar = (uint16_t)(average_depth_meter * 100);
}


static void logbook_SetMinTemperature(float min_temperature_celsius)
{
		gheader.minTemp = (int16_t)((min_temperature_celsius * 10.0f) + 0.5f);
}


static void logbook_SetMaxCNS(float max_cns_percentage)
{
	if(max_cns_percentage < 9999)
		gheader.maxCNS = (uint16_t)(max_cns_percentage);
	else
		gheader.maxCNS = 9999;
}


static void logbook_SetCompartmentDesaturation(const SDiveState * pStateReal)
{
	SLifeData2 secondaryInformation  = { 0 };

	decom_tissues_desaturation_time(&pStateReal->lifeData, &secondaryInformation);
	for(int i=0;i<16;i++)
	{
		if(secondaryInformation.tissue_nitrogen_desaturation_time_minutes[i] <= (15 * 255))
			gheader.n2CompartDesatTime_min[i] = (uint8_t)((secondaryInformation.tissue_nitrogen_desaturation_time_minutes[i] + 14) / 15);
		else
			gheader.n2CompartDesatTime_min[i] = 255;
		if(secondaryInformation.tissue_helium_desaturation_time_minutes[i] <= (15 * 255))
			gheader.heCompartDesatTime_min[i] = (uint8_t)((secondaryInformation.tissue_helium_desaturation_time_minutes[i] + 14 )/ 15);
		else
			gheader.heCompartDesatTime_min[i] = 255;
	}
}

static void logbook_SetLastStop(float last_stop_depth_bar)
{
	gheader.lastDecostop_m = (uint8_t)(last_stop_depth_bar / 10.0f);
}

static void logbook_writedata(void * data, int length_byte)
{
    ext_flash_write_sample(data, length_byte);
}

/********************************************************************************
	* @brief   logbook_build_ostc3header. /
  * @author  heinrichs weikamp gmbh
  * @version V0.0.2
  * @date    27-Nov-2014
*********************************************************************************/
SLogbookHeaderOSTC3 * logbook_build_ostc3header(SLogbookHeader* pHead)
{
	convert_Type data,data2;
	uint16_t dummyLength = 0;
	uint8_t returnEmptyHeader = 0;

	uint32_t headerProfileLength, sampleProfileLength, sampleProfileStart;


	if(pHead->diveHeaderStart != 0xFAFA)
	{
		returnEmptyHeader = 1;
	}
	else
	{
		memcpy(headerOSTC3.diveHeaderStart,			&pHead->diveHeaderStart,					2);
		memcpy(headerOSTC3.pBeginProfileData,		&pHead->pBeginProfileData,				3);
		memcpy(headerOSTC3.pEndProfileData,			&pHead->pEndProfileData,					3);

		data.u8bit.byteHigh = 0;
		data.u8bit.byteLow 		= pHead->pBeginProfileData[0];
		data.u8bit.byteMidLow 	= pHead->pBeginProfileData[1];
		data.u8bit.byteMidHigh 	= pHead->pBeginProfileData[2];

		sampleProfileStart = data.u32bit;

		data2.u8bit.byteHigh = 0;
		data2.u8bit.byteLow 			= pHead->pEndProfileData[0];
		data2.u8bit.byteMidLow 	= pHead->pEndProfileData[1];
		data2.u8bit.byteMidHigh 	= pHead->pEndProfileData[2];

		data.u8bit.byteHigh = 0;
		data.u8bit.byteLow 			= pHead->profileLength[0];
		data.u8bit.byteMidLow 	= pHead->profileLength[1];
		data.u8bit.byteMidHigh 	= pHead->profileLength[2];

		if(data.u32bit != 0xFFFFFF)		/* if the profile in use ? */
		{
			if(data2.u32bit < sampleProfileStart)		/* Wrap around of sample ring detected */
			{
				if(ext_flash_SampleOverrunValid() == 0)	/* Wrap around does not seem to be valid => fallback */
				{
					sampleProfileStart = 0;
				}
			}
			if( sampleProfileStart == 0)				/* should never happen unless OSTC with older debug version is in use (or invalid overrun) */
			{
				sampleProfileLength = 1;
				headerProfileLength = 2;
			}
			else
			{
				headerProfileLength = (pHead->profileLength[2] << 16) + (pHead->profileLength[1] << 8) + pHead->profileLength[0];
				sampleProfileLength = ext_flash_read_profilelength_small_header(sampleProfileStart);
			}

			if(sampleProfileLength != headerProfileLength)
			{
				dummyLength = logbook_fillDummySampleBuffer(pHead);

				data2.u32bit = sampleProfileStart + dummyLength;	/* calc new end address (which is equal to dummyLength) */
				data.u32bit = dummyLength;				    /* data is used below to represent the length */
			}

			data.u32bit += 3;
			headerOSTC3.profileLength[0] = data.u8bit.byteLow;
			headerOSTC3.profileLength[1] = data.u8bit.byteMidLow;
			headerOSTC3.profileLength[2] = data.u8bit.byteMidHigh;

			memcpy(headerOSTC3.gasordil, pHead->gasordil, 20);

			if(pHead->logbookProfileVersion == LOGBOOK_VERSION)
			{
				headerOSTC3.logbookProfileVersion = LOGBOOK_VERSION_OSTC3;
				memcpy(headerOSTC3.personalDiveCount, &pHead->personalDiveCount, 2);
				headerOSTC3.safetyDistance_10cm = 0;

				for(int i=0;i<5;i++)
				{
					if(!pHead->gasordil[i].note.ub.active)
						headerOSTC3.gasordil[3 + (i*4)] = 0;
					else if(pHead->gasordil[i].note.ub.first)
					{
				/* depth = 0, note = 1 */
						headerOSTC3.gasordil[2 + (i*4)] = 0;
				headerOSTC3.gasordil[3 + (i*4)] = 1;
					}
					else if( pHead->gasordil[i].depth_meter)
					{
				/* note = 3 */
						headerOSTC3.gasordil[3 + (i*4)] = 3;
					}
				}
			}
			else
			{
				headerOSTC3.logbookProfileVersion = 0xFF;
				headerOSTC3.personalDiveCount[0] = 0xFF;
				headerOSTC3.personalDiveCount[1] = 0xFF;
				headerOSTC3.safetyDistance_10cm = 0xFF;
			}

			headerOSTC3.dateYear = pHead->dateYear;
			headerOSTC3.dateMonth = pHead->dateMonth;
			headerOSTC3.dateDay = pHead->dateDay;
			headerOSTC3.timeHour = pHead->timeHour;
			headerOSTC3.timeMinute = pHead->timeMinute;

			memcpy(headerOSTC3.maxDepth, &pHead->maxDepth, 2);
			memcpy(headerOSTC3.diveTimeMinutes,	&pHead->diveTimeMinutes, 2);

			headerOSTC3.diveTimeSeconds = pHead->diveTimeSeconds;

			memcpy(headerOSTC3.minTemp,	&pHead->minTemp, 2);
			memcpy(headerOSTC3.surfacePressure_mbar,&pHead->surfacePressure_mbar, 2);
			memcpy(headerOSTC3.desaturationTime, &pHead->desaturationTime, 2);

			headerOSTC3.firmwareVersionHigh = pHead->firmwareVersionHigh;
			headerOSTC3.firmwareVersionLow =	pHead->firmwareVersionLow;

			memcpy(headerOSTC3.batteryVoltage, &pHead->batteryVoltage, 2);

			headerOSTC3.samplingRate = pHead->samplingRate;

			memcpy(headerOSTC3.cnsAtBeginning, &pHead->cnsAtBeginning, 2);

			headerOSTC3.gfAtBeginning = pHead->gfAtBeginning;
			headerOSTC3.gfAtEnd = pHead->gfAtEnd;

			memcpy(headerOSTC3.setpoint, pHead->setpoint, 10);

			headerOSTC3.salinity = pHead->salinity;

			memcpy(headerOSTC3.maxCNS, &pHead->maxCNS, 2);
			memcpy(headerOSTC3.averageDepth_mbar, &pHead->averageDepth_mbar, 2);
			memcpy(headerOSTC3.total_diveTime_seconds, &pHead->total_diveTime_seconds, 2);

			headerOSTC3.gfLow_or_Vpm_conservatism = pHead->gfLow_or_Vpm_conservatism;
			headerOSTC3.gfHigh = pHead->gfHigh;
			headerOSTC3.decoModel = pHead->decoModel;

			memcpy(headerOSTC3.diveNumber, &pHead->diveNumber, 2);

			headerOSTC3.diveMode = pHead->diveMode;
			headerOSTC3.batteryCharge = pHead->batteryCharge;

			memcpy(headerOSTC3.n2CompartDesatTime_min,pHead->n2CompartDesatTime_min, 16);
			memcpy(headerOSTC3.n2Compartments, pHead->n2Compartments, 64);
			memcpy(headerOSTC3.heCompartDesatTime_min,pHead->heCompartDesatTime_min, 16);
			memcpy(headerOSTC3.heCompartments, pHead->heCompartments, 64);

			headerOSTC3.lastDecostop_m = pHead->lastDecostop_m;

			memcpy(headerOSTC3.hwHudBattery_mV,	&pHead->hwHudBattery_mV, 2);

			headerOSTC3.hwHudLastStatus = pHead->hwHudLastStatus;

			memset(headerOSTC3.batteryGaugeRegisters, 0x00, 6);  /* The battery registers are not evaluated => Set to zero */

			memcpy(headerOSTC3.diveHeaderEnd, &pHead->diveHeaderEnd, 2);
		}
		else
		{
			returnEmptyHeader = 1;
		}
	}
	if(returnEmptyHeader)			/* profile not in use => return array full of 0xFF */
	{
		memset(&headerOSTC3, 0xFF, sizeof(headerOSTC3));
	}

	return &headerOSTC3;
}


/********************************************************************************
	* @brief   logbook_build_ostc3header_compact. /
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    31-Juli-2015
*********************************************************************************/
SLogbookHeaderOSTC3compact * logbook_build_ostc3header_compact(SLogbookHeader* pHead)
{
	uint8_t returnEmptyHeader = 0;
	convert_Type data, data2;
	uint32_t dummyLength = 0;
	uint32_t headerProfileLength, sampleProfileLength, sampleProfileStart;

	if(pHead->diveHeaderStart != 0xFAFA)
	{
		returnEmptyHeader = 1;
	}
	else
	{
		data.u8bit.byteHigh = 0;
		data.u8bit.byteLow 		= pHead->pBeginProfileData[0];
		data.u8bit.byteMidLow 	= pHead->pBeginProfileData[1];
		data.u8bit.byteMidHigh 	= pHead->pBeginProfileData[2];

		sampleProfileStart = data.u32bit;

		data2.u8bit.byteHigh = 0;
		data2.u8bit.byteLow 	= pHead->pEndProfileData[0];
		data2.u8bit.byteMidLow 	= pHead->pEndProfileData[1];
		data2.u8bit.byteMidHigh = pHead->pEndProfileData[2];

		data.u8bit.byteHigh = 0;
		data.u8bit.byteLow 		= pHead->profileLength[0];
		data.u8bit.byteMidLow 	= pHead->profileLength[1];
		data.u8bit.byteMidHigh 	= pHead->profileLength[2];

		if(data.u32bit != 0xFFFFFF)
		{
			if(data2.u32bit < sampleProfileStart)		/* Wrap around of sample ring detected */
			{
				if(ext_flash_SampleOverrunValid() == 0)	/* Wrap around does not seem to be valid => fallback */
				{
					sampleProfileStart = 0;
				}
			}

			if( sampleProfileStart == 0)			/* no sample data available => use dummy */
			{
				sampleProfileLength = 1;
				headerProfileLength = 2;
			}
			else
			{
				headerProfileLength = (pHead->profileLength[2] << 16) + (pHead->profileLength[1] << 8) + pHead->profileLength[0];
				sampleProfileLength = ext_flash_read_profilelength_small_header(sampleProfileStart);
			}
			if(sampleProfileLength != headerProfileLength)
			{
				dummyLength = logbook_fillDummySampleBuffer(pHead);

				data2.u32bit = sampleProfileStart + dummyLength;	/* calc new end address (which is equal to dummyLength) */
				data.u32bit = dummyLength;				   			/* data is used below to represent the length */
			}
			data.u32bit += 3;
			headerOSTC3compact.profileLength[0] = data.u8bit.byteLow;
			headerOSTC3compact.profileLength[1] = data.u8bit.byteMidLow;
			headerOSTC3compact.profileLength[2] = data.u8bit.byteMidHigh;

			headerOSTC3compact.dateYear = pHead->dateYear;
			headerOSTC3compact.dateMonth = pHead->dateMonth;
			headerOSTC3compact.dateDay = pHead->dateDay;
			headerOSTC3compact.timeHour = pHead->timeHour;
			headerOSTC3compact.timeMinute = pHead->timeMinute;

			memcpy(headerOSTC3compact.maxDepth, &pHead->maxDepth, 2);
			memcpy(headerOSTC3compact.diveTimeMinutes,	&pHead->diveTimeMinutes, 2);

			headerOSTC3compact.diveTimeSeconds = pHead->diveTimeSeconds;
			headerOSTC3compact.totalDiveNumberLow = pHead->diveNumber & 0xFF;
			headerOSTC3compact.totalDiveNumberHigh = (uint8_t)(pHead->diveNumber/256);
			headerOSTC3compact.profileVersion = 0x24; // Logbook-Profile version, 0x24 = date and time is start not end
		}
		else
		{
			returnEmptyHeader = 1;
		}
	}
	if(returnEmptyHeader)
	{
		memset(&headerOSTC3compact, 0xFF, sizeof(SLogbookHeaderOSTC3compact));
	}
	return &headerOSTC3compact;
}


/**
  ******************************************************************************
  * @brief   logbook_readSampleData. /  Reads sample data of whole logbook entry
  * @author  heinrichs weikamp
  * @version V0.0.1
  * @date    22-April-2014
  ******************************************************************************
	*
  * @param  uint8_t StepBackwards: witch lookbook entry?
  * @param  uint16_t length : maxlength of output arrays
  * @param  int32_t* depth : output  array
  * @param  int16_t * gasid : output  array
  * @param  int32_t* temperature : output  array
  * @param  int32_t* ppo2 : output  array
  * @param  int32_t* cns : output  array
  * @return length of output
  */
void logbook_recover_brokenlog(uint8_t headerId)
{
    int16_t retVal;
    int32_t depthVal = 0;
    int16_t  gasidVal = 0;
    int16_t setPointVal = 0;
    int16_t bailoutVal = 0;
    int32_t temperatureVal = 0;
    int32_t sensor1Val = 0;
    int32_t sensor2Val = 0;
    int32_t sensor3Val = 0;
    int32_t cnsVal = 0;
     SManualGas manualGasVal;

		//uint16_t* ppo2, uint16_t* cns#
     uint32_t bytesRead = 0;

    ext_flash_read_block_start();
    ext_flash_read_next_sample_part((uint8_t*)&smallHeader,  sizeof(SSmallHeader));
    bytesRead += sizeof(SSmallHeader);

    clear_divisor();


    int sampleCounter = 0;
    int maxdepth = 0;
    uint32_t avrdepth = 0;
    while (true)
    {

        ext_flash_set_entry_point();
        divisorBackup = divisor;
				retVal = readSample(&depthVal,&gasidVal, &setPointVal, &temperatureVal, &sensor1Val, &sensor2Val, &sensor3Val, &cnsVal, &manualGasVal, &bailoutVal, NULL, NULL, NULL);
        if(retVal == 0)
        {
          //Error try to read again!!!
          ext_flash_reopen_read_sample_at_entry_point();
          divisor = divisorBackup;
					retVal = readSample(&depthVal,&gasidVal, &setPointVal, &temperatureVal, &sensor1Val, &sensor2Val, &sensor3Val, &cnsVal, &manualGasVal, &bailoutVal, NULL, NULL, NULL);

          if(retVal == 0)
          {
              //Error try to read again!!!
              ext_flash_reopen_read_sample_at_entry_point();
              divisor = divisorBackup;
							retVal = readSample(&depthVal,&gasidVal, &setPointVal, &temperatureVal, &sensor1Val, &sensor2Val, &sensor3Val, &cnsVal, &manualGasVal, &bailoutVal, NULL, NULL, NULL);

              if(retVal == 0)
              {
                ext_flash_reopen_read_sample_at_entry_point();
                break;
              }

          }
        }
        if(depthVal > maxdepth)
          maxdepth = depthVal;
        avrdepth += depthVal;
        sampleCounter++;
        bytesRead +=retVal;
    }
    avrdepth/= sampleCounter;
    ext_flash_close_read_sample();
    SLogbookHeader header;

    ext_flash_read_dive_header2((uint8_t*) &header, headerId, false);
    header.total_diveTime_seconds = sampleCounter * header.samplingRate;
    header.diveTimeMinutes = header.total_diveTime_seconds /60;
    header.diveTimeSeconds = header.total_diveTime_seconds - header.diveTimeMinutes * 60;
    header.maxDepth = maxdepth;
    header.averageDepth_mbar = avrdepth;
    SSettings *	settings = settingsGetPointer();
    settings->lastDiveLogId = headerId;
    ext_flash_close_new_dive_log((uint8_t *)&header);
}

void logbook_createDummyProfile(SLogbookHeader* pHeader, uint16_t length, uint16_t* depth, int16_t* temperature, uint16_t* ppo2)
{
	uint8_t	 drawDeco = 1;
	uint16_t index = 0;
	uint16_t indexDescenStop = 0;
	uint16_t indexAscendStart = 0;
	uint16_t simDecentDepth = 0;
	uint16_t simDecentStep = 0;
	uint16_t simAcentDepth = 0;
	uint16_t simAcentStep = 0;
	float ambiant_pressure_bar = 0;

	simDecentStep = pHeader->maxDepth / (length / 6);							/* first 1/6 for descend */
	simAcentStep = pHeader->maxDepth / (length / 3);							/* first 1/3 for ascend */

	SGas gas;


	if(ppo2)
	{
		/* find first gas ID */
		for(index = 0; index < NUM_GASES; index++)
		{
			if(pHeader->gasordil[index].note.ub.first)
				break;
		}
		if(index != NUM_GASES)
		{
			gas.helium_percentage = pHeader->gasordil[index].helium_percentage;
			gas.nitrogen_percentage = 100 -  gas.helium_percentage - pHeader->gasordil[index].oxygen_percentage;
		}
	}

	while((index < length) && (simDecentDepth < pHeader->maxDepth))			/* draw decent */
	{
		depth[index] = simDecentDepth;
		temperature[index] = pHeader->minTemp;
		if(ppo2)
		{
			ambiant_pressure_bar =((float)(depth[index] + pHeader->surfacePressure_mbar))/1000;
			ppo2[index] = (uint16_t) ((decom_calc_ppO2(ambiant_pressure_bar, &gas )) * 100);
		}
		index++;
		simDecentDepth += simDecentStep;
	}
	indexDescenStop = index;
	index = length -1;
	while((index > indexDescenStop) && (simAcentDepth < pHeader->maxDepth))				/* draw ascend including max deco stop */
	{
		depth[index] = simAcentDepth;
		temperature[index] = pHeader->minTemp;
		if(ppo2)
		{
			ambiant_pressure_bar =((float)(depth[index] + pHeader->surfacePressure_mbar))/1000;
			ppo2[index] = (uint16_t) ((decom_calc_ppO2(ambiant_pressure_bar, &gas )) * 100);
		}
		if((drawDeco) && (simAcentDepth < pHeader->lastDecostop_m))						/* draw deco step */
		{
			drawDeco = length / 10;
			while(drawDeco)
			{
				index--;
				depth[index] = simAcentDepth;
				temperature[index] = pHeader->minTemp;
				if(ppo2)
				{
					ambiant_pressure_bar =((float)(depth[index] + pHeader->surfacePressure_mbar))/1000;
					ppo2[index] = (uint16_t) ((decom_calc_ppO2(ambiant_pressure_bar, &gas )) * 100);
				}
				drawDeco--;
			}
		}
		index--;
		simAcentDepth += simAcentStep;
	}
	indexAscendStart = index;
	index = indexDescenStop;
	while(index <= indexAscendStart)												/* draw isobar dive phase */
	{
		depth[index] = pHeader->maxDepth;
		temperature[index] = pHeader->minTemp;
		if(ppo2)
		{
			ambiant_pressure_bar =((float)(depth[index] + pHeader->surfacePressure_mbar))/1000;
			ppo2[index] = (uint16_t) ((decom_calc_ppO2(ambiant_pressure_bar, &gas )) * 100);
		}
		index++;
	}
}

void logbook_resetDummy()
{
	dummyWriteIdx = 0;
	dummyReadIdx = 0;
}

void logbook_writeDummy(void* data, uint16_t length)
{
	memcpy(&dummyMemoryBuffer[dummyWriteIdx],(uint8_t *)data, length);
	dummyWriteIdx += length;
}
void logbook_writeDummySample(uint16_t depth, int16_t temperature)
{
    uint8_t sample[10];
    int length = 0;

    int i = 0;
    for(i = 0; i <10 ;i++)  sample[i] = 0;
    addU16(sample, depth);
    length += 2;
    sample[2] = 0;
    length++;

    if(divisor.temperature == 0)
    {
			divisor.temperature = smallHeader.tempDivisor - 1;
			addS16(&sample[length], temperature);
			length += 2;
    }
    else
    {
        divisor.temperature--;
    }

    logbook_writeDummy((void *) sample,length);
}


uint16_t logbook_fillDummySampleBuffer(SLogbookHeader* pHeader)
{
	uint16_t depthArray[DUMMY_SAMPLES];
	int16_t temperatureArray[DUMMY_SAMPLES];
	uint16_t ppo2Array[DUMMY_SAMPLES];

	uint16_t index = 0;
	uint16_t dummyBufferSize = 0;
	uint16_t dummyProfileLength = 0;
	uint32_t overallSecond = pHeader->diveTimeMinutes * 60 + pHeader->diveTimeSeconds;

	logbook_resetDummy();
	clear_divisor();

	smallDummyHeader.profileLength[0] = 0xFF;
	smallDummyHeader.profileLength[1] = 0xFF;
	smallDummyHeader.profileLength[2] = 0xFF;
	smallDummyHeader.samplingRate_seconds = 2;
	smallDummyHeader.numDivisors = 7;

	smallDummyHeader.tempType = 0;
	smallDummyHeader.tempLength = 2;
	smallDummyHeader.tempDivisor = 6;

	smallDummyHeader.deco_ndlType = 1;
	smallDummyHeader.deco_ndlLength = 2;
	smallDummyHeader.deco_ndlDivisor = 0;

	/* GF in % at actual position */
	smallDummyHeader.gfType =  2;
	smallDummyHeader.gfLength = 1;
	smallDummyHeader.gfDivisor = 0;

	/* 3 Sensors: 8bit ppO2 in 0.01bar, 16bit voltage in 0,1mV */
	smallDummyHeader.ppo2Type = 3;
	smallDummyHeader.ppo2Length = 9;
	smallDummyHeader.ppo2Divisor = 0;

	/* last 15 stops in minutes (last, second_to_last, ... */
	/* last stop depth is defined in header */
	smallDummyHeader.decoplanType = 4;
	smallDummyHeader.decoplanLength = 15;
	smallDummyHeader.decoplanDivisor = 0;

	smallDummyHeader.cnsType = 5;
	smallDummyHeader.cnsLength = 2;
	smallDummyHeader.cnsDivisor = 0;

	smallDummyHeader.tankType = 6;
	smallDummyHeader.tankLength = 2;
	smallDummyHeader.tankDivisor = 0;

	if((overallSecond / smallDummyHeader.samplingRate_seconds) > DUMMY_SAMPLES)		/* reduce sample interval to keep buffer size */
	{
		smallDummyHeader.samplingRate_seconds = overallSecond / DUMMY_SAMPLES;
		dummyProfileLength = DUMMY_SAMPLES;
	}
	else
	{
		dummyProfileLength = overallSecond / smallDummyHeader.samplingRate_seconds;
	}
	logbook_writeDummy((void *) &smallDummyHeader,sizeof(smallDummyHeader));
	logbook_createDummyProfile(pHeader,dummyProfileLength, depthArray, temperatureArray, ppo2Array );

	for (index = 0; index < dummyProfileLength; index++)
	{
		logbook_writeDummySample(depthArray[index], temperatureArray[index]);
	}

	dummyBufferSize = dummyWriteIdx;

	return dummyBufferSize;		/* return size of dummy buffer */
}

void logbook_readDummySamples(uint8_t* pTarget, uint16_t length)
{
	memcpy(pTarget,&dummyMemoryBuffer[dummyReadIdx],length);
	dummyReadIdx += length;
}


/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
