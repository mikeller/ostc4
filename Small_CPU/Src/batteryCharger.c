/**
  ******************************************************************************
  * @file    batteryCharger.c 
  * @author  heinrichs weikamp gmbh
  * @date    09-Dec-2014
  * @version V0.0.1
  * @since   09-Dec-2014
  * @brief   LTC4054 Battery Charger
  *           
  @verbatim                 
  ============================================================================== 
                        ##### How to use #####
  ============================================================================== 

The bq5105x provides one status output, CHG. This output is an open-drain NMOS device that is rated to 20 V.
The open-drain FET connected to the CHG pin will be turned on whenever the output (BAT) of the charger is
enabled. As a note, the output of the charger supply will not be enabled if the VRECT-REG does not converge to the
no-load target voltage.

CHG F4 7 O Open-drain output � active when BAT is enabled. Float if not used.

@endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 
/* Includes ------------------------------------------------------------------*/
#include "batteryCharger.h"
#include "batteryGasGauge.h"
#include "stm32f4xx_hal.h"
#include "scheduler.h"


#define CHARGER_DEBOUNCE_SECONDS	(6u)		/* 6 seconds used to avoid problems with charger interrupts / disconnections */

static uint16_t battery_charger_counter = 0;
static chargerState_t batteryChargerState = Charger_NotConnected;

void set_charge_state(uint8_t newState)
{
	if(newState < Charger_END)
	{
		batteryChargerState = newState;
	}
}

uint8_t get_charge_state(void)
{
	return batteryChargerState;
}

void init_battery_charger_status(void)
{
	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return;
	#endif

  CHARGE_IN_GPIO_ENABLE();
  CHARGE_OUT_GPIO_ENABLE();
	
	ReInit_battery_charger_status_pins();
}

void ReInit_battery_charger_status_pins(void)
{
	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return;
	#endif

  GPIO_InitTypeDef   GPIO_InitStructure;

  GPIO_InitStructure.Pin = CHARGE_IN_PIN;
  GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(CHARGE_IN_GPIO_PORT, &GPIO_InitStructure); 

  GPIO_InitStructure.Pin = CHARGE_OUT_PIN;
  GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  HAL_GPIO_Init(CHARGE_OUT_GPIO_PORT, &GPIO_InitStructure); 
}


void DeInit_battery_charger_status_pins(void)
{
	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return;
	#endif
  GPIO_InitTypeDef   GPIO_InitStructure;


	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
  GPIO_InitStructure.Pull = GPIO_NOPULL;

  GPIO_InitStructure.Pin = CHARGE_IN_PIN;
  HAL_GPIO_Init(CHARGE_IN_GPIO_PORT, &GPIO_InitStructure); 

  GPIO_InitStructure.Pin = CHARGE_OUT_PIN;
  HAL_GPIO_Init(CHARGE_OUT_GPIO_PORT, &GPIO_InitStructure); 
}

void battery_charger_get_status_and_contral_battery_gas_gauge(uint8_t cycleTimeBase)
{
	static uint8_t notifyChargeComplete = 0;

	#ifdef OSTC_ON_DISCOVERY_HARDWARE
		return;
	#endif
	
	if(batteryChargerState == Charger_ColdStart)	/* wait for the first valid voltage meassurement */
	{
		if(global.lifeData.battery_voltage != BATTERY_DEFAULT_VOLTAGE)	/* wait for first valid voltage value */
		{
			if((global.lifeData.battery_voltage < BATTERY_CHARGER_CONNECTED_VOLTAGE)
					&& (global.lifeData.battery_voltage > BATTERY_ENDOF_CHARGE_VOLTAGE)) 	/* Voltage close to full state => maybe new battery inserted 	*/
			{
				battery_gas_gauge_set_charge_full();
			}
			batteryChargerState = Charger_NotConnected;
		}
	}
	else
	{	/* on disconnection or while disconnected */
		if(HAL_GPIO_ReadPin(CHARGE_IN_GPIO_PORT,CHARGE_IN_PIN))
		{
			switch(batteryChargerState)
			{
				case Charger_WarmUp:
				case Charger_Active:				global.dataSendToMaster.chargeStatus = CHARGER_lostConnection;
													global.deviceDataSendToMaster.chargeStatus = CHARGER_lostConnection;
													batteryChargerState = Charger_LostConnection;
													if(cycleTimeBase > CHARGER_DEBOUNCE_SECONDS)	/* adapt connection lost detection to sleep mode */
													{
														battery_charger_counter = cycleTimeBase + 1;
													}
													else
													{
														battery_charger_counter = CHARGER_DEBOUNCE_SECONDS;
													}
											break;
				case Charger_Finished:				if((get_voltage() >= BATTERY_ENDOF_CHARGE_VOLTAGE) && (get_voltage() < BATTERY_CHARGER_CONNECTED_VOLTAGE)) /* stopping does not necessarily mean battery is full */
													{
														global.dataSendToMaster.chargeStatus = CHARGER_complete;
														global.deviceDataSendToMaster.chargeStatus = CHARGER_complete;
														notifyChargeComplete = 1;
													}
													battery_charger_counter = 10;
													batteryChargerState = Charger_LostConnection;
					/* no break */
				case Charger_LostConnection:		/* the charger stops charging when charge current is 1/10 	*/
													/* Basically it is OK to rate a charging as complete if a defined voltage is reached */
													if(((battery_gas_gauge_isChargeValueValid() == 0) || (global.lifeData.battery_charge < 90)) && (get_voltage() >= BATTERY_ENDOF_CHARGE_VOLTAGE) && (get_voltage() < BATTERY_CHARGER_CONNECTED_VOLTAGE))
													{
														notifyChargeComplete = 1;
													}
													if(battery_charger_counter >= cycleTimeBase)
													{
														battery_charger_counter -= cycleTimeBase;
													}
													else
													{
														battery_charger_counter = 0;

														global.dataSendToMaster.chargeStatus = CHARGER_off;
														global.deviceDataSendToMaster.chargeStatus = CHARGER_off;

														if(notifyChargeComplete)
														{
															battery_gas_gauge_set_charge_full();
															scheduleUpdateDeviceDataChargerFull();
														}
														notifyChargeComplete = 0;
														batteryChargerState = Charger_NotConnected;
													}
											break;
				default:				 			batteryChargerState = Charger_NotConnected; 	/* unexpected state => reinitialize state machine */
					break;
			}
		}
		else
		{
			/* connected */
			/* wait for disconnection to write and reset */
			switch(batteryChargerState)
			{
					case Charger_NotConnected:		battery_charger_counter = 0;
													batteryChargerState = Charger_WarmUp;
											break;
					case Charger_LostConnection:		batteryChargerState = Charger_Active;
											break;
					case Charger_WarmUp:			battery_charger_counter += cycleTimeBase;
													if(battery_charger_counter >= CHARGER_DEBOUNCE_SECONDS )
													{
														scheduleUpdateDeviceDataChargerCharging();
														batteryChargerState = Charger_Active;
													}
							/* no break */
					case Charger_Finished:
					case Charger_Active:			global.dataSendToMaster.chargeStatus = CHARGER_running;
													global.deviceDataSendToMaster.chargeStatus = CHARGER_running;

													/* drive the output pin high to determine the state of the charger */
													GPIO_InitTypeDef   GPIO_InitStructure;
													GPIO_InitStructure.Pin = CHARGE_OUT_PIN;
													GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
													GPIO_InitStructure.Pull = GPIO_NOPULL;
													GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
													HAL_GPIO_Init(CHARGE_OUT_GPIO_PORT, &GPIO_InitStructure);
													HAL_GPIO_WritePin(CHARGE_OUT_GPIO_PORT, CHARGE_OUT_PIN,GPIO_PIN_SET);
													HAL_Delay(1);

													if(HAL_GPIO_ReadPin(CHARGE_IN_GPIO_PORT,CHARGE_IN_PIN))		/* high => charger stopped charging */
													{
														battery_charger_counter = 30;
														batteryChargerState = Charger_Finished;
													}
													else
													{
														if(global.lifeData.battery_charge > 100.0)				/* still charging but indicator is set to full => decrease to 99% to keep count increasing */
														{
															battery_gas_gauge_set(99.0);
														}
														if(batteryChargerState == Charger_Finished)				/* voltage dropped below the hysteresis again => charging restarted */
														{
															batteryChargerState = Charger_Active;
															notifyChargeComplete = 0;
														}
													}

													/* restore high impedance to be able to detect disconnection */
													GPIO_InitStructure.Pin = CHARGE_OUT_PIN;
													GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
													GPIO_InitStructure.Pull = GPIO_NOPULL;
													GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
													HAL_GPIO_Init(CHARGE_OUT_GPIO_PORT, &GPIO_InitStructure);
													HAL_Delay(1);
											break;

					default:						batteryChargerState = Charger_NotConnected; 	/* unexpected state => reinitialize state machine */
						break;
			}
		}
	}
}

/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
