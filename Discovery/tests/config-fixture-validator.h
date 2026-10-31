#ifndef CONFIG_FIXTURE_VALIDATOR_H
#define CONFIG_FIXTURE_VALIDATOR_H

#include <stdint.h>

/* Fixture-side guard; production host validation is not implemented here. */
static inline int config_descriptor_valid(uint8_t type, uint8_t domain,
                                          uint32_t address, uint16_t length,
                                          uint16_t native_size, uint32_t schema,
                                          uint32_t known_schema)
{
    return type < 5 && domain == 1 &&
           (address & 255u) == 0 && address >= 0x20000000u &&
           native_size != 0 && length != 0 &&
           length == native_size && length <= 512u &&
           address <= 0x20030000u - length && schema != 0 &&
           schema == known_schema;
}

static inline int config_ranges_overlap(uint32_t a, uint16_t a_size,
                                        uint32_t b, uint16_t b_size)
{
    return a < b + b_size && b < a + a_size;
}

#endif
