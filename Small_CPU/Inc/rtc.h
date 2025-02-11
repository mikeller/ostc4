/**
  ******************************************************************************
  * @file    rtc.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    10-Oct-2014
  * @brief   header file for rtc control
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RTC_H
#define RTC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */

#include "stm32f4xx_hal.h"
	 
void MX_RTC_init(void);
void RTC_StopMode_2seconds(void);
void RTC_Stop_11ms(void);
void RTC_SetTime(RTC_TimeTypeDef stimestructure);
void RTC_SetDate(RTC_DateTypeDef sdatestructure);

void RTC_GetTime(RTC_TimeTypeDef* pstimestructure);

#ifdef __cplusplus
}
#endif

#endif /* RTC_H */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
