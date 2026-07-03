/**
  ******************************************************************************
  * @file    adc.c 
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    11-Dec-2014
  * @brief   ADC for ambient light sensor
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

/* Includes ------------------------------------------------------------------*/

#include "stm32f4xx_hal.h"
#include "adc.h"
#include "device_time_hooks.h"   /* sim_skip_* runtime gates (Phase 0 overlay) */

/* Exported variables --------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
	ADC_HandleTypeDef    AdcHandle;
__IO uint16_t uhADCxConvertedValue = 0;

/* Private types -------------------------------------------------------------*/
#define ADCx                            ADC1
#define ADCx_CLK_ENABLE()               __ADC1_CLK_ENABLE();
#define ADCx_CHANNEL_GPIO_CLK_ENABLE()  __GPIOF_CLK_ENABLE()
     
#define ADCx_FORCE_RESET()              __ADC_FORCE_RESET()
#define ADCx_RELEASE_RESET()            __ADC_RELEASE_RESET()

/* Definition for ADCx Channel Pin */
#define ADCx_GPIO_PIN                		GPIO_PIN_0
#define ADCx_GPIO_PORT         					GPIOB 
#define ADCx_GPIO_CLK_ENABLE()          __GPIOB_CLK_ENABLE()
#define ADCx_GPIO_CLK_DISABLE()         __GPIOB_CLK_DISABLE()

/* Definition for ADCx's Channel */
#define ADCx_CHANNEL                    ADC_CHANNEL_8

/* Definition for ADCx's NVIC */
//#define ADCx_IRQn                      ADC_IRQn

/* Private function prototypes -----------------------------------------------*/
void ADC_Start_single_IT_Conversion(void);

/* Exported functions --------------------------------------------------------*/

uint16_t get_ambient_light_level(void)
{
	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return 800;
	#endif
	
	return uhADCxConvertedValue;
}


static void ADCx_MspInit(ADC_HandleTypeDef *hadc)
{
	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return;
	#endif
	
  GPIO_InitTypeDef  GPIO_InitStruct;

  ADCx_GPIO_CLK_ENABLE();
  
  GPIO_InitStruct.Pin = ADCx_GPIO_PIN ;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ADCx_GPIO_PORT, &GPIO_InitStruct);

  ADCx_CLK_ENABLE(); 

//  HAL_NVIC_SetPriority(ADCx_IRQn, 2, 0);
//  HAL_NVIC_EnableIRQ(ADCx_IRQn);
}


void ADCx_DeInit(void)
{
//	HAL_ADC_Stop_IT(&AdcHandle);
  HAL_ADC_DeInit(&AdcHandle);
}


void ADCx_Init(void)
{
	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return;
	#endif
	
	ADC_ChannelConfTypeDef sConfig;
	
  AdcHandle.Instance          = ADCx;
  
  AdcHandle.Init.ClockPrescaler				 = ADC_CLOCKPRESCALER_PCLK_DIV4;
	AdcHandle.Init.Resolution            = ADC_RESOLUTION12b;
	AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
	AdcHandle.Init.ContinuousConvMode    = DISABLE;
	AdcHandle.Init.DiscontinuousConvMode = DISABLE;
	AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
	AdcHandle.Init.EOCSelection          = EOC_SINGLE_CONV;
	AdcHandle.Init.NbrOfConversion       = 1;
	AdcHandle.Init.DMAContinuousRequests = DISABLE;    
      
	ADCx_MspInit(&AdcHandle);
	HAL_ADC_Init(&AdcHandle);

	sConfig.Channel = ADCx_CHANNEL;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfig.Rank = 1;
  sConfig.Offset = 0;

	HAL_ADC_ConfigChannel(&AdcHandle, &sConfig);
	
//	ADC_Start_single_IT_Conversion();
}


uint32_t adc_debug_status = 0;

void adc_ambient_light_sensor_get_data(void)
{
	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return;
	#endif

	if(sim_skip_blocking_polls)
	{
		/* Renode's STM32F4 ADC peripheral is sufficiently incomplete
		 * that HAL_ADC_PollForConversion blocks until its full timeout.
		 * Skip; ambient light comes from SRAM-poke under sim. */
		return;
	}

	HAL_ADC_Start(&AdcHandle);
  HAL_ADC_PollForConversion(&AdcHandle, 10);
	adc_debug_status = HAL_ADC_GetState(&AdcHandle);
  if(adc_debug_status == HAL_ADC_STATE_EOC_REG + HAL_ADC_STATE_READY) // new HAL_ADC_STATE_READY 160613
//  if(HAL_ADC_GetState(&AdcHandle) == HAL_ADC_STATE_EOC_REG)
  uhADCxConvertedValue = HAL_ADC_GetValue(&AdcHandle);
	HAL_ADC_Stop(&AdcHandle);
}


/* Private functions ---------------------------------------------------------*/
/*
void ADC_Start_single_IT_Conversion(void)
{
	HAL_ADC_Start_IT(&AdcHandle);
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
	extern void copyAmbientLightData(void);

  uhADCxConvertedValue = HAL_ADC_GetValue(AdcHandle);
	copyAmbientLightData();
	ADC_Start_single_IT_Conversion();
}
*/

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
