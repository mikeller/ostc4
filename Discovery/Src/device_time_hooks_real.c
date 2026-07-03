///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Src/device_time_hooks_real.c
/// \brief  Production no-op impls of the CPU1 sim-infrastructure hooks.
/// \author heinrichs weikamp gmbh
///
/// Linked into production firmware builds. The sim impl (with the
/// Renode workarounds) lives in Discovery/Src/device_time_hooks.c. CMake
/// selects which one at link time based on OSTC4_BUILD_RENODE.
///
/// $Id$
///////////////////////////////////////////////////////////////////////////////
/// \par Copyright (c) 2026 Heinrichs Weikamp gmbh
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

#include <stdint.h>
#include "device_time_hooks.h"

volatile const uint8_t sim_disable_timeouts = 0;
volatile const uint8_t sim_pin_update_necessary_off = 0;

void sim_main_loop_tick_hook(void)
{
	/* No-op in production: the real hardware TIM4 fires NVIC IRQ 30
	 * normally, so HAL_TIM_PeriodElapsedCallback gets called via the
	 * IRQ path without main-loop intervention. */
}
