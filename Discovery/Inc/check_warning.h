///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/check_warning.h
/// \brief  header file for check and set events for warnings
/// \author heinrichs weikamp gmbh
/// \date   17-Nov-2014
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

#ifndef CHECK_WARNING_H
#define CHECK_WARNING_H

#include <stdint.h>
#include "data_central.h"

/* Exported function prototypes ----------------------------------------------*/
void check_warning(void);
void check_warning2(SDiveState *pDiveState);
uint8_t actualBetterGasId(void);
uint8_t actualBetterBailoutGasId(void);
uint8_t actualBetterSetpointId(void);
uint8_t actualLeftMaxDepth(const SDiveState * pDiveState);
void set_warning_fallback(void);
void clear_warning_fallback(void);
uint8_t debounce_warning_fallback(uint16_t debounceStepms);
void reset_debounce_warning_fallback();

uint8_t getSetpointHighId(void);
uint8_t getSetpointLowId(void);
uint8_t getSetpointDecoId(void);
void requestBuzzerActivation(uint8_t active);
uint8_t getBuzzerActivationState();
#endif // CHECK_WARNING_H
