/**
  ******************************************************************************
  * @file    wireless.c 
  * @author  heinrichs weikamp gmbh
  * @date    02-July-2015
  * @version V0.0.2
  * @since   24-March-2016
  * @brief   Data transfer via magnetic field using Manchester code
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
	history:
	160324 V 0.0.2  f�r Bonex Sender (Only in old hardware. No longer supported)
	
	
PA2 triggers to falling edge
PA1 triggers to rising edge
see baseCPU2.c definitions of ports for details
time base is the systick of CMSIS with SysTick->VAL for sub ms info


start id is 4 ms low (fall to rise time)
old: followed by 2 ms high (but can continue with high on sending '0' next (Hi->Lo)
160324: followed by bit 0: high to low of byte 1

even index numbers are falling edge
odd index numbers are rising edge

first bit ('1' or '0' can be evaluated with triggerTimeDiffArray[2] (falling edge)
if it is much longer than 2ms than the first bit is '0'
if it is 2ms than it is '1'

// Manchester code: 1 = low to high; 0 = high to low;
bits are either
rising edge at the middle of the bit transfer -> 1
falling edge at the middle of the bit transfer -> 0

an array is used with even index for falling and odd for rising
the next time difference array entry is set to 0 to identify the loss of any edge
the time index is stored as microseconds
and generated by systick and SysTick->VAL (that is counting down from ->LOAD)

old: startTime is the first falling edge, hence the ver first falling edge of the start byte)
160324: startTime is the first rising edge

160324: total transfer time (excluding start with 8 ms) 10*8*2ms + 2ms (stop bit) = 162 ms

Definition: first bit is always '0'
old: to recognize second start bit easily!
old: hence after 2 ms there is a falling edge!
160324: after 1 ms is a falling edge of first bit with value 0

160324:
Measurement of send data: length 162 ms including stop bit
81 bit

@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "wireless.h"
#include "scheduler.h"

/* array starts with first falling edge
 * old: beginning of first start byte, 
 * old: data starts on 3
 * new: beginning of first bit that is always 0 -> high->low
 * new: data starts on 0
 */
 
#define ttdaSize (1200)

#define TIMEOUTSIZE (20000)

#define WIRELESS_OK 	(0)
#define WIRELESS_FAIL (1)

#define FALLING (0)
#define RISING 	(1)

//#define STEP_HALFBIT (1010)
#define STEP_HALFBIT (1000)
#define LIMIT_STARTBIT (5000)
#define MIN_TIME_STARTBIT (3800)
#define MAX_TIME_FIRST_STARTBIT (5200)
#define MIN_TIME_BOTH_STARTBITS (MIN_TIME_STARTBIT + MIN_TIME_STARTBIT)
#define MAX_TIME_BOTH_STARTBITS (MAX_TIME_FIRST_STARTBIT + 4200)
// includes necessary start bit (2000 us) and timing issues 
#define HELPER_MAX_DATA_LENGTH_TOLERANCE (10*STEP_HALFBIT)
#define HELPER_MAX_DATA_BYTES (10) ///< 160324: 10, before 8
#define MAX_DATA_BITS (HELPER_MAX_DATA_BYTES*8)
#define MAX_DATA_LENGTH ((STEP_HALFBIT*2*MAX_DATA_BITS) + HELPER_MAX_DATA_LENGTH_TOLERANCE)

#define MAX_DIFF_IN_MS ((MAX_DATA_LENGTH+999)/1000)

void wireless_reset(uint32_t timeMain, uint32_t timeSub);

/* triggerTimeDiffArray in us with max 65 ms */

int32_t triggerNewArray[ttdaSize][2];

int32_t triggerNewArrayDebugStartBits[20][2];

//uint16_t triggerTimeDiffArray[ttdaSize];
uint16_t ttdaPointer;
uint32_t startTimeMain;

uint32_t subDivisorX1000;
uint8_t	blockDataCapture = 0;
uint8_t	evaluateRequest = 0;							///<

uint8_t	found_first_4ms_high = 0;					///<
uint8_t	found_second_4ms_low = 0;					///<




//  ===============================================================================
//	wireless_init
/// @brief has to be called once to initialize the variables	 
//  ===============================================================================
void wireless_init(void)
{
	subDivisorX1000 = SysTick->LOAD; 	///< never changes
	wireless_reset(0,0);
	ttdaPointer = 0; 									///< is set back again by timeout or on error
	blockDataCapture = 0;
	//triggerTimeDiffArray[0] = 0;
}


/**
  ******************************************************************************
	* @brief   RECORDING functions
  * @author  heinrichs weikamp gmbh
  * @date    03-July-2015
  * @version V0.0.1
  * @since   03-July-2015
  ******************************************************************************
  */


//  ===============================================================================
//	wireless_getTime
/// @brief RECORDING
///
/// @param 	
//  ===============================================================================
void wireless_getTime(uint32_t *timeMain, uint32_t *timeSub)
{
	uint32_t timeSub2, timeMain2;
	
	*timeSub = SysTick->VAL;
	*timeMain = HAL_GetTick();
	timeSub2 = SysTick->VAL;
	timeMain2 = HAL_GetTick();

/*
	static uint32_t difference = 0;
	if(ttdaPointer == 1)
		difference  = 7; //& breakpoint
*/
	
	if((timeSub2 > *timeSub) && (timeMain2 == *timeMain))
		*timeMain -= 1;
	
	*timeSub = subDivisorX1000 - *timeSub;
	*timeSub *= 1000;
	*timeSub /= subDivisorX1000;
}



/* stored value _could be_ in multiples of 2 us
 * to have 1000 for each byte
 * 0000 is beginning of start bit (always 1)
 * 1000 is beginning of first bit (if timing is perfect, otherwise less or more)
 * ...
 * XXXX000 is beginning of XXXX bit
 * 
 */

//  ===============================================================================
//	wireless_time0keeper
/// @brief RECORDING	
///
/// @param 
/// @return still in us
//  ===============================================================================
int32_t wireless_time0keeper(uint8_t store1_or_diffOut0, uint32_t timeMain, uint32_t timeSub)
{
	static uint32_t storeMain = 0, storeSub = 0;
	
	/* store time0 */
	if(store1_or_diffOut0)
	{
		storeMain = timeMain;
		storeSub = timeSub;
		return 0;
	}

	/* evaluate time difference */
	int32_t diff = 0;
	
	if((timeMain < storeMain) || ((timeMain - storeMain) > MAX_DIFF_IN_MS))
			return -1;

	diff = timeMain - storeMain;
	diff *= 1000;
	diff += timeSub;
	diff -= storeSub;
		
	return diff;
}


//  ===============================================================================
//	wireless_save_time0
/// @brief RECORDING	
///
/// @param 
//  ===============================================================================
void wireless_save_time0(uint32_t timeMain, uint32_t timeSub)
{
	wireless_time0keeper(1, timeMain, timeSub);
}


//  ===============================================================================
//	wireless_get_time_since_time0
/// @brief RECORDING	
///
/// @param 
/// @return
//  ===============================================================================
int32_t wireless_get_time_since_time0(uint32_t timeMain, uint32_t timeSub)
{
	return wireless_time0keeper(0, timeMain, timeSub);
}


//  ===============================================================================
//	wireless_get_time_since_previous
/// @brief RECORDING	
///
/// @param 
/// @return
//  ===============================================================================
int32_t wireless_get_time_since_previous(uint8_t fallingOrRising, uint32_t timeMain, uint32_t timeSub)
{
	int32_t absoluteTimeDiff = wireless_time0keeper(0, timeMain, timeSub);
	
	if(fallingOrRising == FALLING) // same array
	{
		return (absoluteTimeDiff - triggerNewArray[ttdaPointer][FALLING]);
	}
	else
	if(ttdaPointer > 0)
	{
		return (absoluteTimeDiff - triggerNewArray[ttdaPointer - 1][RISING]);
	}
	else // not possible 
		return -1;
}


//  ===============================================================================
//	wireless_reset
/// @brief RECORDING	
///
/// @param 
//  ===============================================================================
void wireless_reset(uint32_t timeMain, uint32_t timeSub)
{
	found_first_4ms_high = 0;
	found_second_4ms_low = 0;
	ttdaPointer = 0;
	startTimeMain = 0;
	evaluateRequest = 0;
	wireless_save_time0(timeMain, timeSub);
	triggerNewArray[ttdaPointer][FALLING] = 0;
	triggerNewArray[ttdaPointer][RISING] = 0;
}




//  ===============================================================================
//	wireless_trigger_FallingEdgeSignalHigh
/// @brief RECORDING	
///
//  ===============================================================================
void wireless_trigger_FallingEdgeSignalHigh(void)
{
	uint32_t timeSub, timeMain;
	int32_t timeDifference;
	
	wireless_getTime(&timeMain, &timeSub);

	if((blockDataCapture) || (evaluateRequest == 1))
		return;

	if(evaluateRequest == 2)
		wireless_reset(0, 0);
	
	if(!found_first_4ms_high)	// trying to get first start 4ms 
	{
		if(ttdaPointer == 0)
		{
			wireless_reset(timeMain, timeSub);
		}
		else
		{
			timeDifference = wireless_get_time_since_time0(timeMain, timeSub);
			if((timeDifference > 0) && (timeDifference <= LIMIT_STARTBIT))
			{
				triggerNewArray[ttdaPointer][0] = timeDifference;
			}
			else
			{
				wireless_reset(timeMain, timeSub);
			}
		}
	}
	else // here comes the very important time0 for all following data
		if(!found_second_4ms_low) ///< the beginning of the very first is recorded here :-)
	{
		timeDifference = wireless_get_time_since_time0(timeMain, timeSub);
		if((timeDifference > 0) && (timeDifference >= MIN_TIME_BOTH_STARTBITS) && (timeDifference <= MAX_TIME_BOTH_STARTBITS))
		{
			// now we are ready for data
			for(int i=0;i<20;i++)
			{
				if(i <= ttdaPointer)
				{
					triggerNewArrayDebugStartBits[i][FALLING] = triggerNewArray[i][FALLING];
					triggerNewArrayDebugStartBits[i][RISING] = triggerNewArray[i][FALLING];
				}
				else
				{
					triggerNewArrayDebugStartBits[i][FALLING] = 0;
					triggerNewArrayDebugStartBits[i][RISING] = 0;
				}
			}
			wireless_reset(timeMain, timeSub);	///< reset all including ttdaPointer and more
			startTimeMain = timeMain;						///< set again
			found_first_4ms_high = 1;						///< set again
			found_second_4ms_low = 1; 					///< set now: ready for recording
		}
	}
	else
	{
		timeDifference = wireless_get_time_since_time0(timeMain, timeSub);

		if((timeDifference > MAX_DATA_LENGTH) || (ttdaPointer > (ttdaSize - 2)))
		{
			evaluateRequest = 1;
		}
		else
		{
			triggerNewArray[ttdaPointer][FALLING] = timeDifference;
			triggerNewArray[ttdaPointer][RISING] = 0;
		}
	}
}


//  ===============================================================================
//	wireless_trigger_RisingEdgeSilence
/// @brief RECORDING	
///
//  ===============================================================================
void wireless_trigger_RisingEdgeSilence(void)
{
	uint32_t timeSub, timeMain;
	int32_t timeDifference;
	
	wireless_getTime(&timeMain, &timeSub);

	if((blockDataCapture) || (evaluateRequest == 1))
		return;

	if(evaluateRequest == 2)
	{
		wireless_reset(0, 0);
		return; // Falling Edge first
	}
	
	timeDifference = wireless_get_time_since_time0(timeMain, timeSub);
	
	if(!found_first_4ms_high)
	{
		if((timeDifference > 0) && (timeDifference <= MAX_TIME_FIRST_STARTBIT))
		{
			triggerNewArray[ttdaPointer++][RISING] = timeDifference;
			triggerNewArray[ttdaPointer][FALLING] = 0;

			if(timeDifference >= MIN_TIME_STARTBIT)	///< start bit is the 4 ms 
			{
				found_first_4ms_high = 1;
				found_second_4ms_low = 0;
			}
		}
	}
	else
	{
		if((timeDifference > MAX_DATA_LENGTH) || (ttdaPointer > (ttdaSize - 2)))
		{
			evaluateRequest = 1;
		}
		else
		{
			triggerNewArray[ttdaPointer++][RISING] = timeDifference;
			triggerNewArray[ttdaPointer][FALLING] = 0;
		}
	}
}


//  ===============================================================================
//	wireless_position_next
/// @brief RECORDING
///
/// @param 
/// @return
//  ===============================================================================
uint8_t wireless_position_next(uint16_t *ttdaPointerNow, uint8_t *typeNow)
{
	if(*typeNow == FALLING)
		*typeNow = RISING;
	else if(*ttdaPointerNow < (ttdaSize - 1))
	{
		*ttdaPointerNow += 1;
		*typeNow = FALLING;
	}
	else
	{
		return WIRELESS_FAIL;
	}
	return WIRELESS_OK;

}


//  ===============================================================================
//	wireless_position_previous
/// @brief RECORDING	
///
/// @param 
/// @return
//  ===============================================================================
uint8_t wireless_position_previous(uint16_t *ttdaPointerNow, uint8_t *typeNow)
{
	if(*typeNow == RISING)
		*typeNow = FALLING;
	else if(*ttdaPointerNow > 0)
	{
		*ttdaPointerNow -= 1;
		*typeNow = RISING;
	}
	else
	{
		return WIRELESS_FAIL;
	}
	return WIRELESS_OK;
}


//  ===============================================================================
//	wireless_position_compare
/// @brief RECORDING	
///
/// @param 
/// @return
//  ===============================================================================
int8_t wireless_position_compare(uint16_t ttdaPointerLeft, uint8_t typeLeft, uint16_t ttdaPointerRight, uint8_t typeRight)
{
	if(ttdaPointerLeft < ttdaPointerRight)
		return -1;
	else
	if(ttdaPointerLeft > ttdaPointerRight)
		return 1;
	else
	if(typeLeft < typeRight)
		return -1;
	else
	if(typeLeft > typeRight)
		return 1;
	else
		return 0;
}


//  ===============================================================================
//	wireless_debug
/// @brief	
///
/// @param 
/// @return
//  ===============================================================================
/* outlined because of errors while compiling
void wireless_debug(int8_t	*adcData, uint16_t max_size_adc)
{
	// debug 
	uint32_t dataVisual[201];
	uint8_t dataVisualValue[201];
	int8_t dataVisualResult[201];
	
	dataVisualValue[0] = 1;

	for(int i=0;i<201;i++)
		dataVisualResult[i] = -1;
	
	for(int i=0;i<201-4;i +=4)
	{
		dataVisualValue[i] = 1;
		dataVisualValue[i+1] = 1;
		dataVisualValue[i+2] = 0;
		dataVisualValue[i+3] = 0;
	}

	dataVisual[0] = triggerNewArray[0][FALLING];

	int j = 1;
	uint32_t valueStore = 0;
	for(int i=0;i<50;i++)
	{
		valueStore = triggerNewArray[i][FALLING];
		dataVisual[j++] = valueStore;
		dataVisual[j++] = valueStore + 1;
		valueStore = triggerNewArray[i][RISING];
		dataVisual[j++] = valueStore;
		dataVisual[j++] = valueStore + 1;
	}


	if(max_size_adc > 0)
	{
		int jStep = 0;
		int jData = 0;
		for(int i=0;i<201;i++)
		{
			if(dataVisual[i] >= jStep)
			{
				if(adcData[jData] > 0)
					dataVisualResult[i] = 1;
				else
					dataVisualResult[i] = 0;
				jStep += STEP_HALFBIT + STEP_HALFBIT;
				jData++;
				if(jData >= max_size_adc)
					break;
			}
		}
	}	
}
*/

//  ===============================================================================
//	wireless_debug_test_failed_AACCF1010203
/// @brief	
///
/// @param 
/// @return
//  ===============================================================================
uint8_t wireless_debug_test_failed_AACCF1010203(uint8_t *data)
{
	if(data[0] != 0xAA)
		return 1;
	if(data[1] != 0xCC)
		return 1;
	if(data[2] != 0xF1)
		return 1;
	if(data[3] != 0x01)
		return 1;
	if(data[4] != 0x02)
		return 0;
	if(data[5] != 0x03)
		return 1;
	
	return 0;
}


//  ===============================================================================
//	wireless_check_crc_failed
/// @brief	
///
/// @param 
/// @return
//  ===============================================================================
uint8_t wireless_check_crc_failed(uint8_t *dataOut, uint8_t maxData)
{
	return (wireless_debug_test_failed_AACCF1010203(dataOut));
}


/**
  ******************************************************************************
	* @brief   EVALUATION functions
  * @author  heinrichs weikamp gmbh
  * @date    03-July-2015
  * @version V0.0.2
  * @since   14-July-2015
  ******************************************************************************
  */

//  ===============================================================================
//	wireless_time0keeper
/// @brief EVALUATION
///
/// @param 
/// @return
//  ===============================================================================
uint8_t wireless_evaluate_internal_loop(uint8_t *dataOut, uint8_t maxData, int32_t shift, uint8_t *confidence)
{
	// variables
	int				iOut = 0;
	int				jAdc = 0;
	
	int8_t		adcData[MAX_DATA_BITS];
	uint16_t	adcPointer = 0;

	int8_t 		bitLeft = 0;
	int8_t		bitRight = 0;

	uint16_t	ttdaPointerStart = 0;
	uint16_t	ttdaPointerEnd = 0;
	uint8_t		typeStart = RISING;
	uint8_t		typeEnd = RISING;
	int32_t 	startTimeHalfBit = 0;
	int32_t		endTimeHalfBit = 0;
	int32_t		startOfThisPeak = 0;
	int32_t		endOfThisPeak = 0;
	uint8_t		wirelessErrorStatus = 0;
	uint8_t		timeToStop = 0;
	int32_t		valueSingle = 0;
	int32_t		halfbitTemp = 0;
	int			  confidenceTemp = 0;

	// safety end for all loops coming
	triggerNewArray[ttdaPointer][RISING] = INT32_MAX;

	ttdaPointerStart = 0;
	ttdaPointerEnd = 0;
	typeStart = RISING;
	typeEnd = RISING;
	startTimeHalfBit = 0;
	endTimeHalfBit = shift;
	adcPointer = 0;
	
	while(!timeToStop)
	{
		// start is latest start
		ttdaPointerEnd = ttdaPointerStart;
		typeEnd = typeStart;
		for(int doItTwice=0;doItTwice<2;doItTwice++)
		{
			startTimeHalfBit = endTimeHalfBit;
			endTimeHalfBit += STEP_HALFBIT;
			// find the end for this half bit; this will include values that continue to the next halfbit and negative values
			while(triggerNewArray[ttdaPointerEnd][typeEnd] < endTimeHalfBit)
				wireless_position_next(&ttdaPointerEnd,&typeEnd);

			if(triggerNewArray[ttdaPointerEnd][typeEnd] == INT32_MAX)
			{
				timeToStop = 1;
				break;
			}
			startOfThisPeak = startTimeHalfBit;
			wirelessErrorStatus = 0;
			halfbitTemp = 0;
			while(!wirelessErrorStatus && (wireless_position_compare(ttdaPointerStart,typeStart, ttdaPointerEnd,typeEnd) <= 0))
			{
				endOfThisPeak = triggerNewArray[ttdaPointerStart][typeStart];
				if(endOfThisPeak <= startOfThisPeak)
				{
					wireless_position_next(&ttdaPointerStart,&typeStart);
				}
				else
				{
					// TODO: what about time difference errors?
					if(endOfThisPeak >= endTimeHalfBit)
						valueSingle = endTimeHalfBit - startOfThisPeak;
					else
						valueSingle = endOfThisPeak - startOfThisPeak;
						
					if(typeStart == RISING)
					{
						halfbitTemp += valueSingle;
					}
				// next, also valid for every next halfbit
					if(endOfThisPeak <= endTimeHalfBit)
					{
						startOfThisPeak = endOfThisPeak;
						wireless_position_next(&ttdaPointerStart,&typeStart);
					}
					else
					{
						startOfThisPeak = endTimeHalfBit;
					}
					// should not be necessary, anyway
					if(startOfThisPeak == endTimeHalfBit)
						break;
				}
			}
			// store
			halfbitTemp *= 100;
			halfbitTemp /= STEP_HALFBIT;
			if(halfbitTemp > 100) halfbitTemp = 100;
			if(doItTwice == 0)
			{
				bitLeft = halfbitTemp;
			}
			else
			{
				bitRight = halfbitTemp;
			}
		}
		// Gewichtung und Bit Generierung
		adcData[adcPointer++] = (int8_t)((bitLeft - bitRight)); // possitive value
		if(adcPointer >= MAX_DATA_BITS)
			timeToStop = 1;
	}
	
	// Auswertung
	jAdc = 0;
	iOut = 0;
	
	for(int i=0;i<maxData;i++)
	{
		dataOut[i] = 0;
		for(int j=0;j<8;j++)
		{
			dataOut[i] *= 2;
			if((adcData[jAdc++] > 0))
				dataOut[i] |= 1;
			if(jAdc >= adcPointer)
			{
				j++;
				while(j<8)
				{
					jAdc = adcPointer + 1; // end signal
					dataOut[i] *= 2;
					j++;
				}
				break;
			}
		}
		if(jAdc > adcPointer)
			break;
		iOut++;
	}

	confidenceTemp = 0;
	for(int i=0;i<adcPointer;i++)
	{
		if(adcData[i] < 0)
			confidenceTemp -= adcData[i];
		else
			confidenceTemp += adcData[i];
	}
	// confidence in adcData is 0 to 127 only
	confidenceTemp *= 2;
	*confidence = (uint8_t)(confidenceTemp/adcPointer);
	
/*	
if(	(iOut>= 5) && wireless_debug_test_failed_AACCF1010203(dataOut))
	wireless_debug(adcData,MAX_DATA_BITS);
*/
	return iOut;

}


//  ===============================================================================
//	wireless_evaluate_crc_error
/// @brief EVALUATION
///
/// @param 
/// @return
//  ===============================================================================
uint8_t wireless_evaluate_crc_error(uint8_t *dataIn, uint8_t maxData)
{
	uint8_t crcTest = 0;
	for(int i=0; i< maxData; i++)
		crcTest ^= dataIn[i];

	return crcTest;
}


//  ===============================================================================
//	wireless_evaluate
/// @brief EVALUATION
///
/// @param 
/// @return
//  ===============================================================================
uint8_t wireless_evaluate(uint8_t *dataOut, uint8_t maxData, uint8_t *confidence)
{
	uint32_t timeTick;
	uint32_t timeElapsed;
	
	timeTick = HAL_GetTick();
	timeElapsed = time_elapsed_ms(startTimeMain,timeTick);

	uint8_t start = 0;
	int	iOut = 0;

	// check condition for start 
	if(evaluateRequest == 2)
		return 0;

	if(evaluateRequest == 1)
		start = 1;

	if((ttdaPointer > 10) && (timeElapsed > (MAX_DATA_LENGTH/1000)))
		start = 1;

	if(!start)
		return 0;
	
	// start 
	blockDataCapture = 1;

	// evaluate
	iOut = wireless_evaluate_internal_loop(dataOut, maxData, 0, confidence);


	/*
	for(int i=0; i>=-500; i -= 100)
	{
		iOut = wireless_evaluate_internal_loop(dataOut, maxData, i, confidence);
		if(iOut < 5)
			break;
		if(wireless_check_crc_failed(dataOut,iOut) == 0)
			break;
	}
*/	
	// end
	evaluateRequest = 2;
	blockDataCapture = 0;
	
	return iOut;
}

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/