///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   BootLoader/Inc/base_bootloader.h
/// \brief  Header for main.c module
/// \author MCD Application Team
/// \date   26-February-2014
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
/**
	******************************************************************************
	* @attention
	*
	* <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
	*
	* Redistribution and use in source and binary forms, with or without modification,
	* are permitted provided that the following conditions are met:
	*   1. Redistributions of source code must retain the above copyright notice,
	*      this list of conditions and the following disclaimer.
	*   2. Redistributions in binary form must reproduce the above copyright notice,
	*      this list of conditions and the following disclaimer in the documentation
	*      and/or other materials provided with the distribution.
	*   3. Neither the name of STMicroelectronics nor the names of its contributors
	*      may be used to endorse or promote products derived from this software
	*      without specific prior written permission.
	*
	* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	*
	******************************************************************************
	*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BASE_BOOTLOADER_H
#define BASE_BOOTLOADER_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

#include "tStructure.h"
#include "settings.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

#define TOP_LAYER 1
#define BACKGRD_LAYER 0

#define SURFMODE 1
#define DIVEMODE 2

typedef enum
{
	ST_Boot = 0,
	ST_Surface,
	ST_Dive,
	ST_Menu,
	ST_END
} SState;

typedef enum
{
	ACTION_IDLE_TICK = 0,
	ACTION_IDLE_SECOND,
	ACTION_MODE_CHANGE,
	ACTION_TIMEOUT,
	ACTION_BUTTON_CUSTOM,
	ACTION_BUTTON_BACK,
	ACTION_BUTTON_NEXT,
	ACTION_BUTTON_ENTER,
	ACTION_BUTTON_ENTER_FINAL,
	ACTION_END
} SAction;

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

uint32_t get_globalState(void);
void set_globalState(uint32_t newID);
void get_globalStateList(SStateList *output);
void set_globalState_Menu_Page(uint8_t page);
void set_globalState_Menu_Line(uint8_t line);
void get_idSpecificStateList(uint32_t id, SStateList *output);
void delayMicros(uint32_t micros);
void get_RTC_DateTime(RTC_DateTypeDef * sdatestructureget, RTC_TimeTypeDef * stimestructureget);
void set_RTC_DateTime(RTC_DateTypeDef * sdatestructure, RTC_TimeTypeDef * stimestructure);
uint8_t get_globalMode(void);
void set_globalMode(uint8_t newMode);
void set_globalState_Log_Page(uint8_t pageIsLine);
void	set_globalState_Base(void);

void set_returnFromComm(void);

#endif /* BASE_BOOTLOADER_H */
