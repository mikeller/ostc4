/**
  ******************************************************************************
  * @file    tempstick.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    27-Apr-2026
  * @brief   Support function for tempstick sensor
  *
  @verbatim
  ==============================================================================
                        ##### How to use #####
  ==============================================================================
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * COPYRIGHT(c) 2026 heinrichs weikamp
  *
  ******************************************************************************
  */

#include "tempstick.h"
#include "data_central.h"
#include "data_exchange.h"
#include "data_exchange_main.h"


#ifdef ENABLE_TEMPSTICK_SUPPORT
static uint16_t tempstickLog_Index = 0;
static uint16_t tempstickLog[EXT_INTERFACE_TEMPSTICK_MAX][TEMPSTICK_LOG_SIZE];		/* store max 255 minutes of tempstick data */
static uint32_t temstickLog_Tick = 0;

static uint8_t minValue[EXT_INTERFACE_TEMPSTICK_MAX];
static uint8_t maxValue[EXT_INTERFACE_TEMPSTICK_MAX];
static uint8_t activecnt[EXT_INTERFACE_TEMPSTICK_MAX];			/* is increased every time the new value is bigger then the last */

#define MAX_ACTIVE_CNT	(10u)

void tempstick_Init()
{
	memset(tempstickLog, TEMPSTICK_MAX_VALUE, EXT_INTERFACE_TEMPSTICK_MAX * TEMPSTICK_LOG_SIZE);
	memset(activecnt,0,EXT_INTERFACE_TEMPSTICK_MAX);
	memset(minValue,0xFF,sizeof(minValue));
	memset(maxValue,0,sizeof(maxValue));
}


void tempstick_EvaluateData()
{
	uint8_t index = 0;

	if(tempstickLog_Index >= 2)
	{
		for(index = 0; index < EXT_INTERFACE_TEMPSTICK_MAX; index++)
		{
			if(tempstickLog[index][tempstickLog_Index - 1] > tempstickLog[index][tempstickLog_Index - 2])	/* increasing */
			{
				if(activecnt[index] < MAX_ACTIVE_CNT)
				{
					activecnt[index]++;
				}
				else if(tempstickLog[index][tempstickLog_Index - 1] < tempstickLog[index][tempstickLog_Index - 2]) /* decreasing */
				{
					if(tempstickLog[index][tempstickLog_Index - 1] > minValue[index])
					{
						/* calculate activity based on the temperature reange which was established during heating */
						activecnt[index] = (float)(tempstickLog[index][tempstickLog_Index - 1] - minValue[index]) / (float)(maxValue[index] - minValue[index]) * 10.0;
					}
					else
					{
						activecnt[index] = 0;
					}
				}
			}
		}
	}
}
void tempstick_LogData(uint8_t simulated)
{
	uint8_t index = 0;
	if((time_elapsed_ms(temstickLog_Tick, HAL_GetTick()) > 30000 ) || (simulated))		/* log data every 30 seconds */
	{
		for(index = 0; index < EXT_INTERFACE_TEMPSTICK_MAX; index++)
		{
			if(stateUsed->lifeData.tempstick[index] < minValue[index])
			{
				minValue[index] = stateUsed->lifeData.tempstick[index];
			}
			if(stateUsed->lifeData.tempstick[index] > maxValue[index])
			{
				maxValue[index] = stateUsed->lifeData.tempstick[index];
			}
			tempstickLog[index][tempstickLog_Index] = TEMPSTICK_MAX_VALUE - stateUsed->lifeData.tempstick[index]; /* invert data as preparation for draw */
		}
		tempstickLog_Index++;
		temstickLog_Tick = HAL_GetTick();
		tempstick_EvaluateData();
	}
}

uint16_t tempstick_GetDataLength()
{
	return tempstickLog_Index;
}

uint8_t* tempstick_GetActiveCntPointer()
{
	return activecnt;
}

uint16_t* tempstickLog_GetDataPointer(uint8_t sensorId)
{
	uint16_t* pRet = NULL;
	if(sensorId < EXT_INTERFACE_TEMPSTICK_MAX)
	{
		pRet = tempstickLog[sensorId];
	}
	return pRet;
}
#endif /*ENABLE_SENTINEL_MODE */
