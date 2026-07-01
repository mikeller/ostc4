///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/t7.h
/// \brief  Header file of Divemode with 7 windows plus plugin
/// \author heinrichs weikamp gmbh
/// \date   23-April-2014
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
#ifndef T7CV_H
#define T7CV_H

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "gfx_engine.h"
#include "configuration.h"

/* Exported functions --------------------------------------------------------*/

void t7_cv_hello(void);
void t7_cv_debug(void);
void t7_cv_gasList(void);
void t7_cv_EADTime(void);
void t7_cv_Decolist(void);
void t7_cv_sensors(void);
void t7_cv_sensors_mV(void);
void t7_cv_Cave(void);

#endif /* T7CV_H */
