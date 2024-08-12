///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/ostc.c
/// \brief  Hardware specific configuration
/// \author Heinrichs Weikamp gmbh
/// \date   05-Dec-2014
///
/// \details
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

/* Includes ------------------------------------------------------------------*/
#include "ostc.h"
#include "stm32f4xx_hal.h"

#ifndef BOOTLOADER_STANDALONE
#include "tCCR.h"
#endif

/* Exported variables --------------------------------------------------------*/
SPI_HandleTypeDef hspiDisplay;
SPI_HandleTypeDef cpu2DmaSpi;


UART_HandleTypeDef UartHandle;
#ifdef USART_PIEZO
UART_HandleTypeDef UartPiezoTxHandle;
#endif
UART_HandleTypeDef UartIR_HUD_Handle;

__IO ITStatus UartReady = RESET;
__IO ITStatus UartReadyHUD = RESET;

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private variables with external access via get_xxx() function -------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/** SPI init function
    * called from HAL
    */
void MX_SPI_Init(void)
{
    hspiDisplay.Instance = SPI5;
    hspiDisplay.Init.Mode = SPI_MODE_MASTER;
    hspiDisplay.Init.Direction = SPI_DIRECTION_2LINES;
    hspiDisplay.Init.DataSize = SPI_DATASIZE_8BIT;
    hspiDisplay.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspiDisplay.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspiDisplay.Init.NSS = SPI_NSS_SOFT;
    hspiDisplay.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;//SPI_BAUDRATEPRESCALER_4;//SPI_BAUDRATEPRESCALER_256;
    hspiDisplay.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspiDisplay.Init.TIMode = SPI_TIMODE_DISABLED;
    hspiDisplay.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    HAL_SPI_Init(&hspiDisplay);

    cpu2DmaSpi.Instance                 = SPI1;
    cpu2DmaSpi.Init.Mode                = SPI_MODE_MASTER;
    cpu2DmaSpi.Init.Direction           = SPI_DIRECTION_2LINES;
    cpu2DmaSpi.Init.DataSize            = SPI_DATASIZE_8BIT;
    cpu2DmaSpi.Init.CLKPolarity         = SPI_POLARITY_LOW;
    cpu2DmaSpi.Init.CLKPhase            = SPI_PHASE_1EDGE;
    cpu2DmaSpi.Init.NSS                 = SPI_NSS_SOFT;//SPI_NSS_HARD_OUTPUT;//SPI_NSS_SOFT;
    cpu2DmaSpi.Init.BaudRatePrescaler   = SPI_BAUDRATEPRESCALER_128; 
    cpu2DmaSpi.Init.FirstBit            = SPI_FIRSTBIT_MSB;
    cpu2DmaSpi.Init.TIMode              = SPI_TIMODE_DISABLED;
    cpu2DmaSpi.Init.CRCCalculation 		= SPI_CRCCALCULATION_DISABLED;
    cpu2DmaSpi.Init.CRCPolynomial 		= 7;

    HAL_SPI_Init(&cpu2DmaSpi);
}


void MX_GPIO_Backlight_max_static_only_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_BACKLIGHT_GPIO_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;//GPIO_PULLUP; /* should be normally high */
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

    GPIO_InitStruct.Pin = TIM_BACKLIGHT_PIN;
    HAL_GPIO_Init(TIM_BACKLIGHT_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(TIM_BACKLIGHT_GPIO_PORT,TIM_BACKLIGHT_PIN,GPIO_PIN_SET);
}


void MX_GPIO_One_Button_only_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    BUTTON_NEXT_GPIO_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;//GPIO_PULLUP; /* should be normally high */
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

    GPIO_InitStruct.Pin = BUTTON_NEXT_PIN;
    HAL_GPIO_Init(BUTTON_NEXT_GPIO_PORT, &GPIO_InitStruct);
}


GPIO_PinState MX_GPIO_Read_The_One_Button(void)
{
    return HAL_GPIO_ReadPin(BUTTON_NEXT_GPIO_PORT, BUTTON_NEXT_PIN);
}

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    DISPLAY_CSB_GPIO_ENABLE();
    DISPLAY_RESETB_GPIO_ENABLE();
    EXTFLASH_CSB_GPIO_ENABLE();
    SMALLCPU_CSB_GPIO_ENABLE();
    OSCILLOSCOPE_GPIO_ENABLE();
    OSCILLOSCOPE2_GPIO_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

    GPIO_InitStruct.Pin = DISPLAY_CSB_PIN;
    HAL_GPIO_Init(DISPLAY_CSB_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DISPLAY_RESETB_PIN;
    HAL_GPIO_Init(DISPLAY_RESETB_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = EXTFLASH_CSB_PIN;
    HAL_GPIO_Init(EXTFLASH_CSB_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OSCILLOSCOPE_PIN;
    HAL_GPIO_Init(OSCILLOSCOPE_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OSCILLOSCOPE2_PIN;
    HAL_GPIO_Init(OSCILLOSCOPE2_GPIO_PORT, &GPIO_InitStruct);

#ifdef DISPLAY_BACKLIGHT_PIN
    DISPLAY_BACKLIGHT_GPIO_ENABLE();
    GPIO_InitStruct.Pin = DISPLAY_BACKLIGHT_PIN;
    HAL_GPIO_Init(DISPLAY_BACKLIGHT_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DISPLAY_BACKLIGHT_GPIO_PORT,DISPLAY_BACKLIGHT_PIN,GPIO_PIN_SET);
#endif

#ifdef SMALLCPU_CSB_PIN
    SMALLCPU_CSB_GPIO_ENABLE();
    GPIO_InitStruct.Pin = SMALLCPU_CSB_PIN;
    HAL_GPIO_Init(SMALLCPU_CSB_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SMALLCPU_CSB_GPIO_PORT,SMALLCPU_CSB_PIN,GPIO_PIN_SET);
#endif

#ifdef SMALLCPU_BOOT0_PIN
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    SMALLCPU_BOOT0_GPIO_ENABLE();
    GPIO_InitStruct.Pin = SMALLCPU_BOOT0_PIN;
    HAL_GPIO_Init(SMALLCPU_BOOT0_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SMALLCPU_BOOT0_GPIO_PORT,SMALLCPU_BOOT0_PIN,GPIO_PIN_RESET);
    GPIO_InitStruct.Pull = GPIO_PULLUP;
#endif

#ifdef IR_HUD_ENABLE_PIN
    IR_HUD_ENABLE_GPIO_ENABLE();
    GPIO_InitStruct.Pin = IR_HUD_ENABLE_PIN;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(IR_HUD_ENABLE_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(IR_HUD_ENABLE_GPIO_PORT,IR_HUD_ENABLE_PIN,GPIO_PIN_SET);
    GPIO_InitStruct.Pull = GPIO_PULLUP;
#endif

#ifdef BLE_NENABLE_PIN
    BLE_NENABLE_GPIO_ENABLE();
    MX_Bluetooth_PowerOff();
#endif

#ifdef TESTPIN
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    TEST_GPIO_ENABLE();
    GPIO_InitStruct.Pin = TEST_PIN;
    HAL_GPIO_Init(TEST_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(TEST_GPIO_PORT,TEST_PIN,GPIO_PIN_SET);
    GPIO_InitStruct.Pull = GPIO_PULLUP;
#endif
}


void MX_TestPin_High(void)
{
#ifdef TESTPIN
    HAL_GPIO_WritePin(TEST_GPIO_PORT,TEST_PIN,GPIO_PIN_SET);
#endif
}


void MX_TestPin_Low(void)
{
#ifdef TESTPIN
    HAL_GPIO_WritePin(TEST_GPIO_PORT,TEST_PIN,GPIO_PIN_RESET);
#endif
}

void MX_Bluetooth_PowerOn(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Pin = BLE_NENABLE_PIN;
    HAL_GPIO_Init(BLE_NENABLE_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BLE_NENABLE_GPIO_PORT,BLE_NENABLE_PIN,GPIO_PIN_RESET);
}


void MX_Bluetooth_PowerOff(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pin = BLE_NENABLE_PIN;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BLE_NENABLE_GPIO_PORT, &GPIO_InitStruct);
}


void MX_SmallCPU_Reset_To_Boot(void)
{
#ifdef SMALLCPU_NRESET_PIN
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

    SMALLCPU_NRESET_GPIO_ENABLE();
    GPIO_InitStruct.Pin = SMALLCPU_NRESET_PIN;
    HAL_GPIO_Init(SMALLCPU_NRESET_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SMALLCPU_NRESET_GPIO_PORT,SMALLCPU_NRESET_PIN,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SMALLCPU_BOOT0_GPIO_PORT,SMALLCPU_BOOT0_PIN,GPIO_PIN_SET);
    HAL_Delay(2);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(SMALLCPU_NRESET_GPIO_PORT, &GPIO_InitStruct);
    HAL_Delay(100);
    HAL_GPIO_WritePin(SMALLCPU_BOOT0_GPIO_PORT,SMALLCPU_BOOT0_PIN,GPIO_PIN_RESET);
#endif
}


void MX_SmallCPU_NO_Reset_Helper(void)
{
#ifdef SMALLCPU_NRESET_PIN
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

    SMALLCPU_NRESET_GPIO_ENABLE();
    HAL_GPIO_Init(SMALLCPU_NRESET_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SMALLCPU_NRESET_GPIO_PORT,SMALLCPU_NRESET_PIN,GPIO_PIN_SET);
//	HAL_Delay(100);
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  HAL_GPIO_Init(SMALLCPU_NRESET_GPIO_PORT, &GPIO_InitStruct);
#endif
}


void MX_SmallCPU_Reset_To_Standard(void)
{
#ifdef SMALLCPU_NRESET_PIN
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;

    SMALLCPU_NRESET_GPIO_ENABLE();
    GPIO_InitStruct.Pin = SMALLCPU_NRESET_PIN;
    HAL_GPIO_Init(SMALLCPU_NRESET_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SMALLCPU_NRESET_GPIO_PORT,SMALLCPU_NRESET_PIN,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SMALLCPU_BOOT0_GPIO_PORT,SMALLCPU_BOOT0_PIN,GPIO_PIN_RESET);
    HAL_Delay(2);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(SMALLCPU_NRESET_GPIO_PORT, &GPIO_InitStruct);
#endif
}


uint8_t MX_UART_ButtonAdjust(uint8_t *array)
{
#ifdef USART_PIEZO
    uint8_t answer[4];
    HAL_UART_Transmit(&UartPiezoTxHandle,array,4,1000);
    HAL_UART_Receive(&UartPiezoTxHandle,answer,4,2000);
    if(	(answer[0] == array[0])
        &&(answer[1] == array[1])
        &&(answer[2] == array[2])
        &&(answer[3] == array[3]))
    return 1;
#endif
    return 0;
}


void MX_UART_Init(void)
{
  /*##-1- Configure the UART peripheral ######################################*/
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  /* UART1 configured as follow:
      - Word Length = 8 Bits
      - Stop Bit = One Stop bit
      - Parity = None
      - BaudRate = 9600 baud
      - Hardware flow control disabled (RTS and CTS signals) */

#ifdef USARTx_CTS_PIN
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_RTS_CTS;
#else
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
#endif
    UartHandle.Instance        = USARTx;
    UartHandle.Init.BaudRate   = 115200;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;
    HAL_UART_Init(&UartHandle);

#ifdef USART_PIEZO
    UartPiezoTxHandle.Instance        = USART_PIEZO;
    UartPiezoTxHandle.Init.BaudRate   = 1200;
    UartPiezoTxHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartPiezoTxHandle.Init.StopBits   = UART_STOPBITS_1;
    UartPiezoTxHandle.Init.Parity     = UART_PARITY_NONE;
    UartPiezoTxHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartPiezoTxHandle.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&UartPiezoTxHandle);
#endif

#ifdef USART_IR_HUD
    UartIR_HUD_Handle.Instance        = USART_IR_HUD;
    UartIR_HUD_Handle.Init.BaudRate   = 2400;
    UartIR_HUD_Handle.Init.WordLength = UART_WORDLENGTH_8B;
    UartIR_HUD_Handle.Init.StopBits   = UART_STOPBITS_1;
    UartIR_HUD_Handle.Init.Parity     = UART_PARITY_NONE;
    UartIR_HUD_Handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartIR_HUD_Handle.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&UartIR_HUD_Handle);
#endif
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &UartHandle)
        UartReady = SET;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &UartHandle)
        UartReady = SET;
    else
    if(huart == &UartIR_HUD_Handle)
    {
#ifndef BOOTLOADER_STANDALONE
    	tCCR_SetRXIndication();
#endif
		UartReadyHUD = SET;
    }
}

void MX_tell_reset_logik_alles_ok(void)
{
#ifdef RESET_LOGIC_ALLES_OK_PIN
    GPIO_InitTypeDef GPIO_InitStruct;

    RESET_LOGIC_ALLES_OK_GPIO_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Pin = RESET_LOGIC_ALLES_OK_PIN;
    HAL_GPIO_Init(RESET_LOGIC_ALLES_OK_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(RESET_LOGIC_ALLES_OK_GPIO_PORT,RESET_LOGIC_ALLES_OK_PIN,GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(RESET_LOGIC_ALLES_OK_GPIO_PORT,RESET_LOGIC_ALLES_OK_PIN,GPIO_PIN_SET);

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(RESET_LOGIC_ALLES_OK_GPIO_PORT, &GPIO_InitStruct);
#endif
}


#ifndef BOOTLOADER_STANDALONE
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart == &UartIR_HUD_Handle)
        tCCR_restart();
}
#endif
