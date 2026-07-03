///////////////////////////////////////////////////////////////////////////////
/// -*- coding: UTF-8 -*-
///
/// \file   Discovery/Inc/device_time_hooks.h
/// \brief  CPU1 sim-infrastructure hooks (foundation-B/C followup).
/// \author heinrichs weikamp gmbh
///
/// Mirrors the foundation-A Small_CPU/Inc/device_time_hooks.h pattern. base.c
/// (and any other production source) calls these hooks unconditionally;
/// link-time selection picks the production no-op impl
/// (device_time_hooks_real.c) or the Renode workaround impl (device_time_hooks.c).
///
/// Result: base.c contains zero #ifdef RENODE_BUILD gates while
/// preserving the sim workarounds it used to embed inline.
///
/// Design: see
/// docs/superpowers/specs/2026-05-06-mock-mode-decision-design.md §5.1
/// for the link-time-dispatch pattern that motivates this file.
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

#ifndef SIM_HOOKS_H
#define SIM_HOOKS_H

#include <stdint.h>

/* Called once per main-loop iteration. Production impl is a no-op.
 * Sim impl drives HAL_TIM_PeriodElapsedCallback from the main loop body
 * because Renode's STM32_Timer model accepts the HAL register writes
 * for TIM4 but never fires NVIC IRQ 30, leaving DoDisplayRefresh stuck
 * at 0 otherwise. Internal divider keeps the rate ~10 Hz. */
void sim_main_loop_tick_hook(void);

/* When non-zero, TimeoutControl() returns immediately so surface
 * auto-sleep doesn't fire under fast-time emulation. Production impl
 * defines this as 0; sim impl defines it as 1. The variable is read
 * once per TimeoutControl call so production cost is one load + one
 * branch (negligible at the ~10 Hz call rate). */
extern volatile const uint8_t sim_disable_timeouts;

/* When non-zero, t7.c pins `updateNecessary = 0` regardless of the
 * RTE-version + font-update checks. Required under emulation because
 * the CPU2 mock's lost_connection counters cycle faster than the
 * firmware polls them; the resulting "Please update RTE" blink
 * desyncs from frame capture and yields flaky goldens.
 *
 * Production = 0 (real RTE check applies). Sim = 1.
 *
 * Single-byte cost in production firmware; read once per t7 surface
 * frame paint (~10 Hz) - negligible. */
extern volatile const uint8_t sim_pin_update_necessary_off;

#endif /* SIM_HOOKS_H */
