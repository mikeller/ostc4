/**
  ******************************************************************************
  * @file    gpio.h
  * @author  heinrichs weikamp gmbh
  * @version V0.0.1
  * @date    08-Dec-2024
  * @brief   new GPIO definitions (GPIO_V2)
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2024 heinrichs weikamp</center></h2>
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "configuration.h"

#define VIBRATION_CONTROL_PIN			GPIO_PIN_3		/* PortA */
#define LED_CONTROL_PIN_RED        		GPIO_PIN_2		/* PortA */
#define LED_CONTROL_PIN_GREEN      		GPIO_PIN_1		/* PortA */
#define MAINCPU_CONTROL_PIN				GPIO_PIN_0		/* PortC */
#define	GPS_POWER_CONTROL_PIN			GPIO_PIN_15		/* PortB */
#define	GPS_BCKP_CONTROL_PIN			GPIO_PIN_14		/* PortB */

void GPIO_LEDs_VIBRATION_Init(void);
void GPIO_GNSS_Init();
void GPIO_Power_MainCPU_Init(void);
void GPIO_Power_MainCPU_ON(void);
void GPIO_Power_MainCPU_OFF(void);

#ifdef ENABLE_GPIO_V2
void GPIO_LED_RED_OFF(void);
void GPIO_LED_RED_ON(void);
void GPIO_LED_GREEN_OFF(void);
void GPIO_LED_GREEN_ON(void);
void GPIO_VIBRATION_OFF(void);
void GPIO_VIBRATION_ON(void);
void GPIO_GPS_OFF(void);
void GPIO_GPS_ON(void);
void GPIO_GPS_BCKP_OFF(void);
void GPIO_GPS_BCKP_ON(void);
#endif
#ifdef __cplusplus
}
#endif
#endif /* GPIO_H */



/************************ (C) COPYRIGHT heinrichs weikamp *****END OF FILE****/
