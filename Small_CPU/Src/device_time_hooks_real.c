/**
 * Production no-op + real-hardware impls of the CPU2 sim hooks.
 *
 * Linked into production firmware builds (CPU2 RTE). The sim impl
 * (with the Renode workarounds) lives in Small_CPU/Src/device_time_hooks.c.
 * CMake selects which one at link time based on OSTC4_BUILD_RENODE.
 *
 * Mirrors the foundation-B Discovery/Src/device_time_hooks_real.c CPU1 pattern.
 * Each flag here is a `volatile const` so the compiler treats it as
 * read-only at all call sites; sim impl puts the same symbol with the
 * sim-mode value (1) to override behaviour at the production read sites.
 */
#include <stdint.h>
#include "device_time_hooks.h"

volatile const uint8_t sim_skip_rtc_stop_mode  = 0;
volatile const uint8_t sim_skip_init_delays    = 0;
volatile const uint8_t sim_skip_blocking_polls = 0;
