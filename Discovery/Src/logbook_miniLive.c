/**
  ******************************************************************************
	* @copyright heinrichs weikamp
  * @file   		logbook_miniLive.c
  * @author 		heinrichs weikamp gmbh
  * @date   		13-March-2015
  * @version		V0.0.1
  * @since			13-March-2015
  * @brief			little logbook for during the dive
	* @bug
	* @warning
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/


#include <string.h>
#include "logbook_miniLive.h"
#include "data_exchange.h"
#include "logbook.h"
#include "tHome.h"

 /*
  ******************************************************************************
  * @brief   t7_updateMiniLiveLogbook. /  Create depth samples for view during dive
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    13-March-2015
  ******************************************************************************
	*
  */
	
#define MLLsize (296)
static uint16_t MLLdataDepth[MLLsize];
static uint16_t MLLpointer  = 0;
static uint8_t MLLtickIntervallSeconds = 2;

/* Replay Block data storage */
#define DEPTH_DATA_LENGTH	(1800u)				/* Resolution: 1 hours dive, sampling every 2 seconds */
uint16_t ReplayDepthData[DEPTH_DATA_LENGTH];
uint8_t ReplayMarkerData[DEPTH_DATA_LENGTH];
uint16_t liveDepthData[DEPTH_DATA_LENGTH];
uint16_t liveDepthDataMod[DEPTH_DATA_LENGTH];	/* live data modified to fit to marker checks */
uint16_t liveDecoData[DEPTH_DATA_LENGTH];
uint16_t liveDecoDataMod[DEPTH_DATA_LENGTH];
static uint16_t liveDataIndex = 0;
static uint16_t liveDataIndexMod = 0;

static uint8_t	ReplayDataResolution = 2;		/* Time represented by one sample (second) */
static uint16_t ReplayDataLength = 0;			/* Number of data entries */
static uint16_t ReplayDataMaxDepth = 0;
static uint16_t ReplayDataMinutes = 0;
static uint16_t ReplayDataOffset = 0xFFFF;		/* Stepbackwards format used by log functions */
static uint16_t  ReplayMarkerIndex = 0;

uint16_t *getMiniLiveLogbookPointerToData(void)
{
	return MLLdataDepth;
}


uint16_t getMiniLiveLogbookActualDataLength(void)
{
	return MLLpointer;
}

uint16_t MiniLiveLogbook_getNextMarkerIndex(uint16_t curIndex)
{
	uint16_t index = 0;
	
	if((ReplayMarkerData[0] != 0xFF) && (curIndex < ReplayDataLength))
	{
		index = curIndex;
		do
		{
			index++;
			if (index == ReplayDataLength)
			{
				index = 0;
			}
			if(ReplayMarkerData[index] != 0)
			{
				break;
			}
		}while (index != curIndex);
	}
	return index;
}

static uint16_t workdata[DEPTH_DATA_LENGTH];
static void compressMarkerData(uint16_t* pSource, uint16_t* pTarget, float step, uint16_t startIndex, uint16_t stopIndex)
{
	uint16_t workIndex = startIndex;
	float nextStep = (float)workIndex;

	while (workIndex <= ReplayMarkerIndex)
	{
		workdata[workIndex] = *pSource++;
		nextStep += step;
		while(nextStep < workIndex + 1)
		{
			if(*pSource != 0xFFFF)					/* do not average "ignore" value */
			{
				if(workdata[workIndex] == 0xFFFF)	/* first value to be shown */
				{
					workdata[workIndex] = *pSource;
				}
				else
				{
					workdata[workIndex] += *pSource;
					workdata[workIndex] /= 2;
				}
			}
			pSource++;
			nextStep += step;
		}
		workIndex++;
	}
	memcpy(&pTarget[startIndex],&workdata[startIndex],(workIndex - startIndex -1) * 2);
	while(workIndex < DEPTH_DATA_LENGTH)
	{
		pTarget[workIndex] = 0xFFFF;
		workIndex++;
	}
}
static void stretchMarkerData(uint16_t* pSource, uint16_t* pTarget, float step, uint16_t startIndex, uint16_t stopIndex)
{
	uint16_t workIndex = startIndex;
	float nextStep = (float)workIndex;

	while (workIndex <= stopIndex)
	{
		nextStep += step;
		if(nextStep > stopIndex)
		{
			nextStep = stopIndex;
		}
		while(workIndex <= (uint16_t)nextStep)
		{
			workdata[workIndex++] = *pSource;
		}
		pSource++;
	}
	memcpy(&pTarget[startIndex],&workdata[startIndex],(workIndex - startIndex) * 2);
	while(workIndex < DEPTH_DATA_LENGTH)
	{
		pTarget[workIndex] = 0xFFFF;
		workIndex++;
	}
}

void MiniLiveLogbook_checkMarker(void)
{
	static uint16_t lastLifeIndex = 0;
	uint16_t* pDepthData;
	uint16_t* pDecoData;
	float step;
	uint16_t lastMarkerIndex = ReplayMarkerIndex;

	ReplayMarkerIndex = MiniLiveLogbook_getNextMarkerIndex(ReplayMarkerIndex);
	if(ReplayMarkerIndex <= lastMarkerIndex)		/* no other marker found or last marker checked => reset marker to 0 to deactivate check function */
	{
		ReplayMarkerIndex = 0;
		lastLifeIndex = 0;
		liveDataIndexMod = liveDataIndex;
	}
	else
	{
		if(lastMarkerIndex == 0)	/* use real live data */
		{
			pDepthData = &liveDepthData[0];
			pDecoData = &liveDecoData[0];
			lastLifeIndex = 0;
		}
		else
		{
			pDepthData = &liveDepthDataMod[lastMarkerIndex];  /* work with already modified data */
			pDecoData = &liveDecoDataMod[lastMarkerIndex];
		}
		if(lastLifeIndex == liveDataIndex)					/* repeated button press before new data was generated => draw straight line */
		{
			step = ReplayMarkerIndex-lastMarkerIndex;
		}
		else
		{
			step = (ReplayMarkerIndex-lastMarkerIndex) / (float)(liveDataIndex - lastLifeIndex); /* the live data shall be modified to match the history data */
		}

		lastLifeIndex = liveDataIndex;

		if(step < 1)		/* compression needed */
		{
			compressMarkerData(pDepthData, liveDepthDataMod, step, lastMarkerIndex, ReplayMarkerIndex);
			compressMarkerData(pDecoData, liveDecoDataMod, step, lastMarkerIndex, ReplayMarkerIndex);
		}
		else				/* stretch data */
		{
			stretchMarkerData(pDepthData, liveDepthDataMod, step, lastMarkerIndex, ReplayMarkerIndex);
			stretchMarkerData(pDecoData, liveDecoDataMod, step, lastMarkerIndex, ReplayMarkerIndex);
		}
		liveDataIndexMod = ReplayMarkerIndex;
	}
}


void compressBuffer_uint16(uint16_t* pdata, uint16_t size)
{
	uint16_t* pTarget = pdata;
	uint16_t* pSource = pdata;
	uint16_t  result = 0;
	uint16_t index = 0;
	
	for(index = 0; index < size/2; index++)
	{
		*pTarget = *pSource++;
		*pTarget += *pSource++;
		result = *pTarget /= 2;
		if((*pTarget != 0) && (result == 0))	/* avoid termination of information by round up to 1 */
		{
			*pTarget++ = 1;
		}
		else
		{
			*pTarget++ = result;
		}
	}
	memset(pTarget,0,size/2);
}

void updateMiniLiveLogbook( _Bool checkOncePerSecond)
{
	static uint8_t bDiveMode = 0;
	static uint32_t last_second = 0;
	static uint8_t secondsCount = 0;
	static uint8_t lifesecondsCount = 0;

	const SDecoinfo* pDecoinfo;
	uint8_t stopDepth = 0;
	uint16_t stopTime = 0;

	if(checkOncePerSecond)
	{
		uint32_t now =  current_second();
		if( last_second == now)
				return;
		last_second = now;
	}
	secondsCount++;
	lifesecondsCount++;
	
	if(!bDiveMode)
	{
		if((stateUsed->mode == MODE_DIVE) && (stateUsed->lifeData.dive_time_seconds >= 5))
		{
			secondsCount = 0;
			MLLtickIntervallSeconds = 2;
			bDiveMode = 1;
			MLLpointer = 1;
			for(int i=0;i<MLLsize;i++)
				MLLdataDepth[i] = 0;

			for(liveDataIndex = 0; liveDataIndex < DEPTH_DATA_LENGTH; liveDataIndex++)
			{
				liveDepthData[liveDataIndex] = 0xFFFF;
				liveDecoData[liveDataIndex] = 0xFFFF;
			}
			lifesecondsCount = 0;
			liveDataIndex = 0;
			liveDataIndexMod = 0;
			liveDepthData[liveDataIndex++] = 0;	/* start at 0 */
		}
	}
	else if(stateUsed->mode == MODE_DIVE)
	{
		bDiveMode = 3;
		//
		if(secondsCount >= MLLtickIntervallSeconds)
		{
			secondsCount = 0;
			/* in case of a buffer overrun the buffer is divided and the first half is filled with a compressed image of the complete buffer */
			if((MLLpointer >= MLLsize) && (MLLtickIntervallSeconds < 127))
			{
				MLLpointer = 0;
				MLLtickIntervallSeconds *= 2;

				compressBuffer_uint16(MLLdataDepth,MLLsize);
				MLLpointer = MLLsize/2;
			}
			if(MLLpointer < MLLsize)
				MLLdataDepth[MLLpointer++] = (int)(stateUsed->lifeData.depth_meter * 10);
		}
		if(lifesecondsCount >= ReplayDataResolution)
		{
			lifesecondsCount = 0;

			if(liveDataIndex >= DEPTH_DATA_LENGTH)		/* compress data */
			{
				ReplayDataResolution *= 2;
				compressBuffer_uint16(liveDepthData,DEPTH_DATA_LENGTH);
				compressBuffer_uint16(liveDepthDataMod, DEPTH_DATA_LENGTH);
				compressBuffer_uint16(ReplayDepthData,DEPTH_DATA_LENGTH);		/* also compress Replay data to simplify mapping between live and replay data */
				liveDataIndex = DEPTH_DATA_LENGTH / 2;
				liveDataIndexMod /= 2;
			}
			liveDepthData[liveDataIndex] = (int)(stateUsed->lifeData.depth_meter * 100);
			liveDepthDataMod[liveDataIndexMod] = liveDepthData[liveDataIndex];

			if(stateUsed->diveSettings.deco_type.ub.standard == VPM_MODE)
			{
				pDecoinfo = &stateUsed->decolistVPM;
			}
			else
			{
				pDecoinfo = &stateUsed->decolistBuehlmann;
			}
			tHome_findNextStop(pDecoinfo->output_stop_length_seconds, &stopDepth, &stopTime);
			if(stopDepth)
			{
				liveDecoData[liveDataIndex] = stopDepth * 100;
				liveDecoDataMod[liveDataIndexMod] = stopDepth * 100;
			}
			else
			{
				liveDecoData[liveDataIndex] = 0xFFFF;
				liveDecoDataMod[liveDataIndexMod] = 0xFFFF;
			}
			liveDataIndex++;
			liveDataIndexMod++;
		}
	}
	else if(bDiveMode == 3)
	{
		//End of Dive
		for(int i=0;i<MLLsize;i++)
			MLLdataDepth[i] = 0;
		bDiveMode = 0;
	}
}

uint8_t prepareReplayLog(uint8_t StepBackwards)
{
	uint8_t retVal = 0;
	uint16_t index = 0;
	uint16_t dataLength = 0;
	uint8_t markerDetected = 0;

    SLogbookHeader logbookHeader;

    if(ReplayDataOffset == StepBackwards)				/* Entry already selected => reset selection */
    {
    	ReplayDataOffset = 0xFFFF;
    	ReplayDataResolution = 2;
		ReplayDataLength = 0;
		ReplayDataMaxDepth = 0;
		ReplayDataMinutes =  0;

    	retVal = 1;
    }
    else
    {
    	ReplayDataOffset = StepBackwards;
		logbook_getHeader(StepBackwards ,&logbookHeader);

		dataLength = logbook_readSampleData(StepBackwards, DEPTH_DATA_LENGTH, ReplayDepthData,NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, ReplayMarkerData);

	/* check if a marker is provided. If not disable marker functionality for the replay block */
		for(index = 0; index < dataLength; index++)
		{
			if(ReplayMarkerData[index] != 0)
			{
				markerDetected = 1;
				break;
			}
		}
		if(markerDetected == 0)
		{
			ReplayMarkerData[0] = 0xFF;
		}

		ReplayDataResolution = logbookHeader.total_diveTime_seconds / dataLength;
		ReplayDataLength = dataLength;
		ReplayDataMaxDepth = logbookHeader.maxDepth;
		ReplayDataMinutes =  logbookHeader.diveTimeMinutes;
		if(dataLength != 0)
		{
			retVal = 1;
		}
    }
	return retVal;
}

uint8_t getReplayInfo(uint16_t** pReplayData, uint8_t** pReplayMarker, uint16_t* DataLength, uint16_t* MaxDepth, uint16_t* diveMinutes)
{
	uint8_t retVal = 0;

	if((ReplayDataOffset != 0xFFFF) && (pReplayData != NULL) && (DataLength != NULL) && (MaxDepth != NULL) && (pReplayMarker != 0))
	{
		*pReplayData = ReplayDepthData;
		*pReplayMarker = ReplayMarkerData;
		*DataLength = ReplayDataLength;
		*MaxDepth = ReplayDataMaxDepth;
		*diveMinutes = ReplayDataMinutes;
		retVal = 1;
	}

	return retVal;
}

uint16_t *getMiniLiveReplayPointerToData(void)
{
	if(ReplayMarkerIndex == 0)
	{
		return liveDepthData;
	}
	else
	{
		return liveDepthDataMod;
	}
}
uint16_t *getMiniLiveDecoPointerToData(void)
{
	if(ReplayMarkerIndex == 0)
	{
		return liveDecoData;
	}
	else
	{
		return liveDecoDataMod;
	}
}
uint16_t getMiniLiveReplayLength(void)
{
	return liveDataIndex;
}

uint16_t getReplayOffset(void)
{
	return ReplayDataOffset;
}

uint16_t getReplayDataResolution(void)
{
	return ReplayDataResolution;
}

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
