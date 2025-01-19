/**
  ******************************************************************************
  * @file    gpio.c
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    08-Dec-2024
  * @brief   Definitions for GPIO operations (GPIO_V2)
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2024 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/

#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "data_exchange.h"
#include "scheduler.h"

/* Exported variables --------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void GPIO_LEDs_VIBRATION_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	__GPIOA_CLK_ENABLE();
	GPIO_InitStructure.Pin = LED_CONTROL_PIN_RED;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init( GPIOA, &GPIO_InitStructure);
	HAL_GPIO_WritePin( GPIOA, LED_CONTROL_PIN_RED, GPIO_PIN_SET);

	GPIO_InitStructure.Pin = LED_CONTROL_PIN_GREEN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init( GPIOA, &GPIO_InitStructure);
	HAL_GPIO_WritePin( GPIOA, LED_CONTROL_PIN_GREEN, GPIO_PIN_SET);

	GPIO_InitStructure.Pin = VIBRATION_CONTROL_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLDOWN;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init( GPIOA, &GPIO_InitStructure);
	HAL_GPIO_WritePin( GPIOA, VIBRATION_CONTROL_PIN, GPIO_PIN_RESET);
}

void GPIO_GNSS_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	__GPIOB_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPS_POWER_CONTROL_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init( GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin( GPIOB, GPS_POWER_CONTROL_PIN, GPIO_PIN_SET);

	GPIO_InitStructure.Pin = GPS_BCKP_CONTROL_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLDOWN;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init( GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin( GPIOB, GPS_BCKP_CONTROL_PIN, GPIO_PIN_SET);
}

void GPIO_Power_MainCPU_Init(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	__GPIOC_CLK_ENABLE();
	GPIO_InitStructure.Pin = MAINCPU_CONTROL_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
	HAL_GPIO_Init( GPIOC, &GPIO_InitStructure);
	HAL_GPIO_WritePin( GPIOC, MAINCPU_CONTROL_PIN, GPIO_PIN_RESET);
}

#ifdef ENABLE_GPIO_V2
void GPIO_HandleBuzzer()
{
	static uint32_t buzzerOnTick = 0;
	static uint8_t buzzerWasOn = 0;

	if(((global.dataSendToSlave.data.externalInterface_Cmd & EXT_INTERFACE_BUZZER_ON) != 0))
	{
		if(!buzzerWasOn)
		{
			buzzerOnTick = HAL_GetTick();
			GPIO_VIBRATION_ON();
			/* GPIO_LED_RED_ON(); */

			if(time_elapsed_ms(buzzerOnTick,HAL_GetTick()) > EXT_INTERFACE_BUZZER_ON_TIME_MS)
			{
				GPIO_VIBRATION_OFF();
			/*	GPIO_LED_RED_OFF(); */
			}
		}
		buzzerWasOn = 1;
	}
	else
	{
		if(buzzerWasOn)
		{
			buzzerOnTick = 0;
			GPIO_VIBRATION_OFF();
			/* GPIO_LED_RED_OFF(); */
		}
		buzzerWasOn = 0;
	}
}
#endif
void GPIO_Power_MainCPU_ON(void) {
	HAL_GPIO_WritePin( GPIOC, MAINCPU_CONTROL_PIN, GPIO_PIN_RESET);
}

void GPIO_Power_MainCPU_OFF(void) {
	HAL_GPIO_WritePin( GPIOC, MAINCPU_CONTROL_PIN, GPIO_PIN_SET);
}

#ifdef ENABLE_GPIO_V2
void GPIO_LED_GREEN_ON(void) {
	HAL_GPIO_WritePin( GPIOA, LED_CONTROL_PIN_GREEN, GPIO_PIN_RESET);
}

void GPIO_LED_GREEN_OFF(void) {
	HAL_GPIO_WritePin( GPIOA, LED_CONTROL_PIN_GREEN, GPIO_PIN_SET);
}

void GPIO_LED_RED_ON(void) {
	HAL_GPIO_WritePin( GPIOA, LED_CONTROL_PIN_RED, GPIO_PIN_RESET);
}

void GPIO_LED_RED_OFF(void) {
	HAL_GPIO_WritePin( GPIOA, LED_CONTROL_PIN_RED, GPIO_PIN_SET);
}

void GPIO_VIBRATION_ON(void) {
	HAL_GPIO_WritePin( GPIOA, VIBRATION_CONTROL_PIN, GPIO_PIN_SET);
}

void GPIO_VIBRATION_OFF(void) {
	HAL_GPIO_WritePin( GPIOA, VIBRATION_CONTROL_PIN, GPIO_PIN_RESET);
}

void GPIO_GPS_ON(void) {
	HAL_GPIO_WritePin( GPIOB, GPS_POWER_CONTROL_PIN, GPIO_PIN_RESET);
}

void GPIO_GPS_OFF(void) {
	HAL_GPIO_WritePin( GPIOB, GPS_POWER_CONTROL_PIN, GPIO_PIN_SET);
}

void GPIO_GPS_BCKP_ON(void) {
	HAL_GPIO_WritePin( GPIOB, GPS_BCKP_CONTROL_PIN, GPIO_PIN_SET);
}

void GPIO_GPS_BCKP_OFF(void) {
	HAL_GPIO_WritePin( GPIOB, GPS_BCKP_CONTROL_PIN, GPIO_PIN_RESET);
}
#endif

/* Private functions ---------------------------------------------------------*/


/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
