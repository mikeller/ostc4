/**
  ******************************************************************************
  * @file    batteryCharger.h
  * @author  heinrichs weikamp gmbh
  * @date    09-Dec-2014
  * @version V0.0.1
  * @since   09-Dec-2014
  * @brief	 LTC4054 Standalone Linear Li-Ion Battery Charger 
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BATTERY_CHARGER_H
#define BATTERY_CHARGER_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#define CHARGE_IN_PIN				GPIO_PIN_2
#define CHARGE_IN_GPIO_PORT			GPIOC
#define CHARGE_IN_GPIO_ENABLE()		__GPIOC_CLK_ENABLE()

#define CHARGE_OUT_PIN				GPIO_PIN_1
#define CHARGE_OUT_GPIO_PORT		GPIOC
#define CHARGE_OUT_GPIO_ENABLE()	__GPIOC_CLK_ENABLE()

 typedef enum
 {
 	Charger_NotConnected = 0,		/* This is identified reading CHARGE_IN_PIN == HIGH */
 	Charger_WarmUp,					/* Charging started but counter did not yet reach a certain limit (used to debounce connect / disconnect events to avoid multiple increases of statistic charging cycle counter) */
 	Charger_Active,					/* Charging identified by  CHARGE_IN_PIN == LOW for a certain time */
 	Charger_Finished,
 	Charger_LostConnection,			/* Intermediate state to debounce disconnecting events (including charging error state like over temperature) */
 	Charger_ColdStart,				/* Cold start condition => check if an loaded battery has been inserted */
 	Charger_END
 } chargerState_t;


void init_battery_charger_status(void);
void set_charge_state(chargerState_t newState);
uint8_t get_charge_state(void);
void ReInit_battery_charger_status_pins(void);
void DeInit_battery_charger_status_pins(void);
void battery_charger_get_status_and_contral_battery_gas_gauge(uint8_t cycleTimeBase);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_CHARGER_H */

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
