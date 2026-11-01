#ifndef TCOMM_CONFIG_SNAPSHOT_H
#define TCOMM_CONFIG_SNAPSHOT_H

#include <stdint.h>
#include <string.h>

#define CONFIG_SLOT_SIZE 1024u
#define CONFIG_REGION_SIZE 512u
#define CONFIG_REGION_COUNT 5u

typedef int (*configLiveRead)(void *, uint8_t *, uint16_t);
typedef int (*configSlotRead)(void *, uint8_t, uint8_t *, uint16_t);

typedef struct {
    uint8_t regions[CONFIG_REGION_COUNT][CONFIG_REGION_SIZE]
        __attribute__((aligned(256)));
    uint8_t present[CONFIG_REGION_COUNT];
    uint8_t captured;
} configSnapshot;

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

static inline void config_snapshot_reset(configSnapshot *state)
{
    memset(state, 0, sizeof(*state));
}

/* One capture per session. live() must take a coherent short copy; slot()
 * reads one complete authoritative slot and must not change live settings. */
static inline int config_snapshot_capture(configSnapshot *state, uint16_t native_size,
                                           uint32_t schema, int profiles_enabled,
                                           configLiveRead live, configSlotRead slot,
                                           void *context)
{
    uint32_t captured_schema;
    if (state->captured)
        return 1;
    config_snapshot_reset(state);
    if (native_size < 4 || native_size > CONFIG_REGION_SIZE ||
        !live(context, state->regions[0], native_size))
    {
        config_snapshot_reset(state);
        return 0;
    }
    memcpy(&captured_schema, state->regions[0], sizeof(captured_schema));
    if (captured_schema != schema)
        goto fail;
    state->present[0] = 1;
    if (profiles_enabled) {
        uint8_t buffer[CONFIG_SLOT_SIZE];
        for (uint8_t i = 0; i < 4; i++) {
            if (!slot(context, i, buffer, sizeof(buffer)))
                goto fail;
            state->present[i + 1] = config_profile_record(buffer, native_size,
                                                            schema, state->regions[i + 1]);
        }
    }
    state->captured = 1;
    return 1;
fail:
    config_snapshot_reset(state);
    return 0;
}

#endif
