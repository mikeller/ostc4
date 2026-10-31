#ifndef TCOMM_CONFIG_SNAPSHOT_H
#define TCOMM_CONFIG_SNAPSHOT_H

#include <stdint.h>
#include <string.h>

#define CONFIG_SLOT_SIZE 1024u
#define CONFIG_REGION_SIZE 512u
#define CONFIG_REGION_COUNT 5u

/* Return the final complete native record in a profile slot. A malformed
 * record invalidates the slot; older, well-framed layouts are not exported. */
static inline int config_profile_record(const uint8_t *slot, uint16_t native_size,
                                        uint32_t native_header, uint8_t *output)
{
    uint16_t offset = 0;
    int found = 0;

    if (native_size < 4 || native_size > CONFIG_REGION_SIZE)
        return 0;
    while (offset < CONFIG_SLOT_SIZE) {
        uint16_t length;
        uint32_t header;
        if (offset > CONFIG_SLOT_SIZE - 2u)
            return 0;
        length = (uint16_t)slot[offset] | ((uint16_t)slot[offset + 1] << 8);
        if (length == 0xffffu)
            return found;
        if (length < 4 || length > native_size || length > CONFIG_SLOT_SIZE - offset - 2u)
            return 0;
        memcpy(&header, slot + offset + 2u, sizeof(header));
        if ((header & 0xffff0000u) != 0xffff0000u)
            return 0;
        if (length == native_size && header == native_header) {
            memcpy(output, slot + offset + 2u, length);
            found = 1;
        } else {
            found = 0; /* most recent record has an unsupported layout */
        }
        offset += length + 2u;
    }
    return found;
}

/* Fixture-side wire descriptor guard. Not a production host parser. */
static inline int config_descriptor_valid(uint8_t type, uint8_t domain,
                                          uint32_t address, uint16_t length,
                                          uint16_t native_size, uint32_t schema,
                                          uint32_t known_schema)
{
    return type < CONFIG_REGION_COUNT && domain == 1 &&
           (address & 255u) == 0 && address >= 0x20000000u &&
           length == native_size && length <= CONFIG_REGION_SIZE &&
           address <= 0x20030000u - length && schema != 0 &&
           schema == known_schema;
}

static inline int config_ranges_overlap(uint32_t a, uint16_t a_size,
                                        uint32_t b, uint16_t b_size)
{
    return a < b + b_size && b < a + a_size;
}

#endif
