#ifndef TCOMM_CONFIG_WIRE_H
#define TCOMM_CONFIG_WIRE_H

#include <stdint.h>
#include <string.h>

typedef int (*configWireTransmit)(void *, const uint8_t *, uint16_t);

static inline void config_wire_put32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value >> 24);
    dst[1] = (uint8_t)(value >> 16);
    dst[2] = (uint8_t)(value >> 8);
    dst[3] = (uint8_t)value;
}

static inline uint8_t config_wire_discovery(uint8_t *dst, uint8_t model,
                                             const uint8_t firmware[4])
{
    dst[0] = 0x90;
    memcpy(dst + 1, "OCFG", 4);
    dst[5] = 1; dst[6] = 1; dst[7] = model;
    memcpy(dst + 8, firmware, 4);
    dst[12] = 0x4c;
    return 13;
}

static inline uint8_t config_wire_snapshot(uint8_t *dst, uint8_t model,
                                            const uint8_t firmware[4], const uint8_t abi[16])
{
    dst[0] = 0x91; dst[1] = 0;
    memcpy(dst + 2, "OSNP", 4);
    dst[6] = 1; dst[7] = model;
    memcpy(dst + 8, firmware, 4);
    memcpy(dst + 12, abi, 16);
    dst[28] = 0; /* count, updated for every entry */
    return 29;
}

static inline uint8_t config_wire_entry(uint8_t *dst, uint8_t type,
                                         uint32_t address, uint16_t size,
                                         uint32_t schema, const uint8_t abi[16])
{
    dst[0] = type; dst[1] = 1;
    config_wire_put32(dst + 2, address);
    dst[6] = (uint8_t)(size >> 8); dst[7] = (uint8_t)size;
    config_wire_put32(dst + 8, schema);
    memcpy(dst + 12, abi, 16);
    return 28;
}

static inline int config_wire_send(void *ctx, configWireTransmit transmit,
                                    const uint8_t *data, uint16_t length)
{
    return transmit(ctx, data, length) != 0;
}

#endif
