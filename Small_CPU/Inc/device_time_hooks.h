/**
 * Sim infrastructure header for the Renode harness.
 *
 * This file declares the host-controlled symbols that the OSTC4
 * simulator pokes from the host side via the Renode XML-RPC monitor:
 * the continuous sim-time scaling triple (speed/base/offset), and a
 * layout table that exposes SGlobal struct offsets to the host
 * without forcing it to parse DWARF.
 *
 * These symbols and the override of HAL_GetTick() live in
 * device_time_hooks.c, which is compiled ONLY when OSTC4_BUILD_RENODE=ON.
 * Production firmware never sees this header or the symbols.
 *
 * Long-term goal: every sim control surface lives behind this header.
 * Production source files (scheduler.c, base.c, ...) should not need
 * a single #ifdef RENODE_BUILD for sim infrastructure - only for
 * residual firmware-fitting workarounds that pending Renode patches
 * will retire.
 *
 * See:
 *   - docs/superpowers/specs/2026-05-06-renode-flash-emulation-design.md §3
 *     (zero-RENODE_BUILD-gates trajectory)
 *   - docs/sim/architecture.md (sim infrastructure overview)
 */
#ifndef SIM_HOOKS_H
#define SIM_HOOKS_H

#include <stdint.h>

/* The flags below are read from production source files (rtc.c, etc.).
 * They are defined in device_time_hooks_real.c (production, all 0) or device_time_hooks.c
 * (RENODE_BUILD, value depends on flag). Production source compiles
 * bit-identically with or without RENODE_BUILD. */

/* When non-zero, RTC_StopMode_2seconds() / RTC_Stop_11ms() skip the
 * real HAL_PWR_EnterSTOPMode sequence and substitute a short busy-wait.
 * Reason: HAL_PWR_EnterSTOPMode stalls under emulation (no RTC wakeup
 * IRQ from Renode's STM32_RTC model). Production = 0 (real STOP entry).
 * Sim = 1 (busy-wait substitute). */
extern volatile const uint8_t sim_skip_rtc_stop_mode;

/* When non-zero, peripheral-init HAL_Delay calls in baseCPU2.c, pressure.c
 * etc. are skipped. Reason: real hardware needs settling time after I2C
 * resets, ADC sample requests, button-init retries; under emulation those
 * delays simply stretch the test runtime without any benefit. Production
 * = 0 (delays preserved). Sim = 1. */
extern volatile const uint8_t sim_skip_init_delays;

/* When non-zero, blocking peripheral-poll waits (ADC conversion poll,
 * MS5xxx pressure-sensor conversion delay) are skipped. Reason: real
 * hardware needs the poll to wait for conversion completion; under
 * emulation the peripherals don't actually convert, so polling waits
 * for nothing. Production = 0 (real polls). Sim = 1 (skip). */
extern volatile const uint8_t sim_skip_blocking_polls;

#endif /* SIM_HOOKS_H */
