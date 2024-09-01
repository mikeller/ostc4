///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/ostc.h
/// \brief  Hardware specific configuration
/// \author heinrichs weikamp gmbh
/// \date   05-Dec-2014
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2014-2018 Heinrichs Weikamp gmbh
///
///     This program is free software: you can redistribute it and/or modify
///     it under the terms of the GNU General Public License as published by
///     the Free Software Foundation, either version 3 of the License, or
///     (at your option) any later version.
///
///     This program is distributed in the hope that it will be useful,
///     but WITHOUT ANY WARRANTY; without even the implied warranty of
///     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///     GNU General Public License for more details.
///
///     You should have received a copy of the GNU General Public License
///     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef OSTC_H
#define OSTC_H

//#define OSTC_ON_DISCOVERY_HARDWARE

/* Includes ------------------------------------------------------------------*/
//#include "stm32f4xx_hal_conf.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"

#ifdef OSTC_ON_DISCOVERY_HARDWARE
 #include "ostc_discovery.h"
#else
 #include "ostc_hw2.h"
// #include "ostc_hw1.h"
#endif

#define SDRAM_TIMEOUT                            ((uint32_t)0xFFFF)
#define SDRAM_MEMORY_WIDTH                       FMC_SDRAM_MEM_BUS_WIDTH_16
#define SDCLOCK_PERIOD                           FMC_SDRAM_CLOCK_PERIOD_3
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

#define DISPLAY_VERSION_LCD						(0u)
#define DISPLAY_VERSION_NEW						(1u)

/* Exported variables --------------------------------------------------------*/

extern SPI_HandleTypeDef hspiDisplay;
extern SPI_HandleTypeDef cpu2DmaSpi;

extern UART_HandleTypeDef UartHandle;
extern __IO ITStatus UartReady;


#ifdef USART_IR_HUD
extern UART_HandleTypeDef UartIR_HUD_Handle;
#endif

#ifdef USART_PIEZO
extern UART_HandleTypeDef UartPiezoTxHandle;
#endif

/* Exported functions --------------------------------------------------------*/

void MX_SPI_Init(void);
void MX_GPIO_Init(void);
void MX_UART_Init(void);
uint8_t MX_UART_ButtonAdjust(uint8_t *array);

void MX_SmallCPU_Reset_To_Boot(void);
void MX_SmallCPU_Reset_To_Standard(void);
void MX_SmallCPU_NO_Reset_Helper(void);

void MX_tell_reset_logik_alles_ok(void);

void MX_Bluetooth_PowerOn(void);
void MX_Bluetooth_PowerOff(void);

void MX_GPIO_Backlight_max_static_only_Init(void);

void MX_GPIO_One_Button_only_Init(void);
GPIO_PinState MX_GPIO_Read_The_One_Button(void);

void MX_TestPin_High(void);
void MX_TestPin_Low(void);

void SetDisplayVersion(uint8_t version);
uint8_t isNewDisplay(void);


#endif // OSTC_H
