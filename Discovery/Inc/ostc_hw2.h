///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/ostc_hw2.h
/// \brief
/// \author Heinrichs Weikamp
/// \date   2018
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
#ifndef OSTC_HW2_H
#define OSTC_HW2_H

#include "stm32f429xx.h"

/*
#define DISPLAY_BACKLIGHT_PIN                       GPIO_PIN_7
#define DISPLAY_BACKLIGHT_GPIO_PORT					GPIOC
#define DISPLAY_BACKLIGHT_GPIO_ENABLE()             __GPIOC_CLK_ENABLE()
*/
#define SMALL_BOARD_PCB9_AND_LATER
#define TESTPIN

#define DISPLAY_RESETB_PIN                          GPIO_PIN_13
#define DISPLAY_RESETB_GPIO_PORT                    GPIOC
#define DISPLAY_RESETB_GPIO_ENABLE()                __GPIOC_CLK_ENABLE()

#define DISPLAY_CSB_PIN								GPIO_PIN_8
#define DISPLAY_CSB_GPIO_PORT                       GPIOI
#define DISPLAY_CSB_GPIO_ENABLE()                   __GPIOI_CLK_ENABLE()

#define VSYNC_IRQ_PIN                               GPIO_PIN_3
#define VSYNC_IRQ_GPIO_PORT                         GPIOE
#define VSYNC_IRQ_GPIO_ENABLE()                     __GPIOE_CLK_ENABLE()
#define VSYNC_IRQ_EXTI_IRQn                         EXTI3_IRQn

#ifdef SMALL_BOARD_PCB9_AND_LATER
#define BUTTON_BACK_PIN								GPIO_PIN_0
#define BUTTON_BACK_EXTI_IRQn                       EXTI0_IRQn
#define BUTTON_BACK_GPIO_PORT                       GPIOB
#define BUTTON_BACK_GPIO_ENABLE()                   __GPIOB_CLK_ENABLE()

#define BUTTON_ENTER_PIN							GPIO_PIN_1
#define BUTTON_ENTER_EXTI_IRQn                      EXTI1_IRQn
#define BUTTON_ENTER_GPIO_PORT                      GPIOA
#define BUTTON_ENTER_GPIO_ENABLE()                  __GPIOA_CLK_ENABLE()

#define BUTTON_NEXT_PIN								GPIO_PIN_2
#define BUTTON_NEXT_EXTI_IRQn                       EXTI2_IRQn
#define BUTTON_NEXT_GPIO_PORT                       GPIOA
#define BUTTON_NEXT_GPIO_ENABLE()                   __GPIOA_CLK_ENABLE()
#else
#define BUTTON_BACK_PIN								GPIO_PIN_0
#define BUTTON_BACK_EXTI_IRQn                       EXTI0_IRQn
#define BUTTON_BACK_GPIO_PORT                       GPIOB
#define BUTTON_BACK_GPIO_ENABLE()                   __GPIOB_CLK_ENABLE()

#define BUTTON_ENTER_PIN							GPIO_PIN_4
#define BUTTON_ENTER_EXTI_IRQn                      EXTI4_IRQn
#define BUTTON_ENTER_GPIO_PORT                      GPIOH
#define BUTTON_ENTER_GPIO_ENABLE()                  __GPIOH_CLK_ENABLE()

#define BUTTON_NEXT_PIN								GPIO_PIN_2
#define BUTTON_NEXT_EXTI_IRQn                       EXTI2_IRQn
#define BUTTON_NEXT_GPIO_PORT                       GPIOA
#define BUTTON_NEXT_GPIO_ENABLE()                   __GPIOA_CLK_ENABLE()

#define BUTTON_CUSTOM_PIN							GPIO_PIN_1
#define BUTTON_CUSTOM_EXTI_IRQn                     EXTI1_IRQn
#define BUTTON_CUSTOM_GPIO_PORT                     GPIOA
#define BUTTON_CUSTOM_GPIO_ENABLE()                 __GPIOA_CLK_ENABLE()
#endif


#ifdef TESTPIN
#define TEST_PIN									GPIO_PIN_4
#define TEST_GPIO_PORT								GPIOH
#define TEST_GPIO_ENABLE()                          __GPIOH_CLK_ENABLE()
#endif


#define EXTFLASH_CSB_PIN							GPIO_PIN_6
#define EXTFLASH_CSB_GPIO_PORT                      GPIOF
#define EXTFLASH_CSB_GPIO_ENABLE()                  __GPIOF_CLK_ENABLE()

#define OSCILLOSCOPE_PIN							GPIO_PIN_3
#define OSCILLOSCOPE_GPIO_PORT                      GPIOA
#define OSCILLOSCOPE_GPIO_ENABLE()                  __GPIOA_CLK_ENABLE()

#define OSCILLOSCOPE2_PIN							GPIO_PIN_11
#define OSCILLOSCOPE2_GPIO_PORT                     GPIOB
#define OSCILLOSCOPE2_GPIO_ENABLE()                 __GPIOB_CLK_ENABLE()

/* Bluetooth */
#define BLE_NENABLE_PIN								GPIO_PIN_7
#define BLE_NENABLE_GPIO_PORT                       GPIOB
#define BLE_NENABLE_GPIO_ENABLE()                   __GPIOB_CLK_ENABLE()
/*
#define BLE_NENABLE_PIN								GPIO_PIN_11
#define BLE_NENABLE_GPIO_PORT                       GPIOD
#define BLE_NENABLE_GPIO_ENABLE()                   __GPIOD_CLK_ENABLE()
*/
#define USARTx                                      USART1
#define USARTx_CLK_ENABLE()                         __USART1_CLK_ENABLE();
#define USARTx_FORCE_RESET()                        __USART1_FORCE_RESET()
#define USARTx_RELEASE_RESET()                      __USART1_RELEASE_RESET()

#define USARTx_TX_AF                                GPIO_AF7_USART1
#define USARTx_TX_PIN                               GPIO_PIN_9
#define USARTx_TX_GPIO_PORT                         GPIOA
#define USARTx_TX_GPIO_CLK_ENABLE()                 __GPIOA_CLK_ENABLE()

#define USARTx_RX_AF                                GPIO_AF7_USART1
#define USARTx_RX_PIN                               GPIO_PIN_10
#define USARTx_RX_GPIO_PORT                         GPIOA
#define USARTx_RX_GPIO_CLK_ENABLE()                 __GPIOA_CLK_ENABLE()

#define USARTx_CTS_AF                               GPIO_AF7_USART1
#define USARTx_CTS_PIN                              GPIO_PIN_11
#define USARTx_CTS_GPIO_PORT                        GPIOA
#define USARTx_CTS_GPIO_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

#define USARTx_RTS_AF                               GPIO_AF7_USART1
#define USARTx_RTS_PIN                              GPIO_PIN_12
#define USARTx_RTS_GPIO_PORT                        GPIOA
#define USARTx_RTS_GPIO_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

#define USARTx_IRQn                                 USART1_IRQn
// to it directly#define USARTx_IRQHandler          USART1_IRQHandler

/*
#define IR_HUD_ENABLE_PIN                           GPIO_PIN_7
#define IR_HUD_ENABLE_GPIO_PORT                     GPIOD
#define IR_HUD_ENABLE_GPIO_ENABLE()                 __GPIOD_CLK_ENABLE()
*/

#define USART_IR_HUD                                USART2
#define USART_IR_HUD_CLK_ENABLE()                   __USART2_CLK_ENABLE();
#define USART_IR_HUD_FORCE_RESET()                  __USART2_FORCE_RESET()
#define USART_IR_HUD_RELEASE_RESET()                __USART2_RELEASE_RESET()
#define USART_IR_HUD_TX_AF                          GPIO_AF7_USART2
#define USART_IR_HUD_TX_PIN                         GPIO_PIN_5
#define USART_IR_HUD_TX_GPIO_PORT                   GPIOD
#define USART_IR_HUD_TX_GPIO_CLK_ENABLE()           __GPIOD_CLK_ENABLE()

#define USART_IR_HUD_RX_AF                          GPIO_AF7_USART2
#define USART_IR_HUD_RX_PIN                         GPIO_PIN_6
#define USART_IR_HUD_RX_GPIO_PORT                   GPIOD
#define USART_IR_HUD_RX_GPIO_CLK_ENABLE()           __GPIOD_CLK_ENABLE()
#define USART_IR_HUD_IRQn                           USART2_IRQn
// to it directly#define USART_IR_HUD_IRQHandler    USART2_IRQHandler

#define TIMx                                        TIM4
#define TIMx_CLK_ENABLE                             __TIM4_CLK_ENABLE
#define TIMx_IRQn                                   TIM4_IRQn
// to it directly #define TIMx_IRQHandler           TIM4_IRQHandler

#define TIM_BACKLIGHT                               TIM3
#define TIM_BACKLIGHT_CLK_ENABLE                    __TIM3_CLK_ENABLE
#define TIM_BACKLIGHT_IRQn                          TIM3_IRQn
#define TIM_BACKLIGHT_IRQHandler                    TIM3_IRQHandler
#define TIM_BACKLIGHT_CHANNEL                       TIM_CHANNEL_2
#define TIM_BACKLIGHT_PIN                           GPIO_PIN_7
#define TIM_BACKLIGHT_GPIO_PORT                     GPIOC
#define TIM_BACKLIGHT_GPIO_ENABLE()                 __GPIOC_CLK_ENABLE()

#define SMALLCPU_CSB_PIN							GPIO_PIN_5
#define SMALLCPU_CSB_GPIO_PORT                      GPIOC
#define SMALLCPU_CSB_GPIO_ENABLE()                  __GPIOC_CLK_ENABLE()

#define SMALLCPU_BOOT0_PIN                          GPIO_PIN_9
#define SMALLCPU_BOOT0_GPIO_PORT                    GPIOC
#define SMALLCPU_BOOT0_GPIO_ENABLE()                __GPIOC_CLK_ENABLE()

#define SMALLCPU_NRESET_PIN                         GPIO_PIN_8
#define SMALLCPU_NRESET_GPIO_PORT                   GPIOC
#define SMALLCPU_NRESET_GPIO_ENABLE()               __GPIOC_CLK_ENABLE()

#define RESET_LOGIC_ALLES_OK_PIN					GPIO_PIN_15
#define RESET_LOGIC_ALLES_OK_GPIO_PORT				GPIOB
#define RESET_LOGIC_ALLES_OK_GPIO_ENABLE()          __GPIOB_CLK_ENABLE()

/* new OSTC5 */
#define BLE_UBLOX_DSR_PIN                             GPIO_PIN_12
#define BLE_UBLOX_DSR_GPIO_PORT                       GPIOC
#define BLE_UBLOX_DSR_GPIO_ENABLE()                   __GPIOC_CLK_ENABLE()



#endif // OSTC_HW2_H
