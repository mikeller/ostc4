#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "tCommConfigSnapshot.h"
#include "tCommConfigWire.h"

static int fail_send(void *ctx, const uint8_t *data, uint16_t size)
{
    unsigned *calls = ctx;
    assert(data[0] == 0x91 && size == 34);
    (*calls)++;
    return 0; /* HAL may already have transmitted a prefix */
}

int main(void)
{
    uint8_t slot[CONFIG_SLOT_SIZE], output[CONFIG_REGION_SIZE];
    uint8_t stale_ram[CONFIG_REGION_SIZE];
    uint8_t wire[128];
    uint8_t firmware[4] = {1, 7, 7, 1};
    unsigned calls = 0;
    uint8_t size;
    const uint32_t header = 0xffff0031u;
    memset(slot, 0xff, sizeof(slot));
    memset(output, 0, sizeof(output));
    memset(stale_ram, 0x99, sizeof(stale_ram));
    assert(!config_profile_record(slot, 428, header, output));

    slot[0] = 4; slot[1] = 0; /* older, framed layout */
    memcpy(slot + 2, &header, 4);
    slot[6] = 428 & 255; slot[7] = 428 >> 8;
    memcpy(slot + 8, &header, 4);
    memset(slot + 12, 0x45, 424);
    assert(config_profile_record(slot, 428, header, output));
    assert(memcmp(output, stale_ram, 428) != 0);
    assert(!memcmp(output + 4, slot + 12, 424));
    /* A second stored record supersedes the first; do not return stale RAM. */
    slot[436] = 428 & 255; slot[437] = 428 >> 8;
    memcpy(slot + 438, &header, 4);
    memset(slot + 442, 0x62, 424);
    assert(config_profile_record(slot, 428, header, output));
    assert(output[4] == 0x62);
    slot[436] = 0; assert(!config_profile_record(slot, 428, header, output));
    slot[436] = 428 & 255; slot[437] = 428 >> 8;
    slot[438] = 0; assert(!config_profile_record(slot, 428, header, output));
    slot[438] = 0x31;
    assert(config_profile_record(slot, 428, header, output));
    slot[436] = 0xfe; slot[437] = 0xff;
    assert(!config_profile_record(slot, 428, header, output));
    assert(!config_profile_record(slot, 513, header, output));

    assert(config_descriptor_valid(0, 1, 0x20005000u, 428, 428, header, header));
    assert(!config_descriptor_valid(5, 1, 0x20005000u, 428, 428, header, header));
    assert(!config_descriptor_valid(0, 2, 0x20005000u, 428, 428, header, header));
    assert(!config_descriptor_valid(0, 1, 0x20005001u, 428, 428, header, header));
    assert(!config_descriptor_valid(0, 1, 0x20005000u, 0, 428, header, header));
    assert(!config_descriptor_valid(0, 1, 0x20005000u, 513, 513, header, header));
    assert(!config_descriptor_valid(0, 1, 0xffffff00u, 428, 428, header, header));
    assert(!config_descriptor_valid(0, 1, 0x20005000u, 428, 428, 0xffff0032u, header));
    assert(config_ranges_overlap(0x20005000u, 428, 0x20005100u, 428));
    assert(!config_ranges_overlap(0x20005000u, 428, 0x20005200u, 428));

    size = config_wire_discovery(wire, 0x43, firmware);
    assert(size == 13 && !memcmp(wire, "\x90OCFG\x01\x01\x43\x01\x07\x07\x01\x4c", 13));
    config_wire_discovery(wire, 0x44, firmware);
    assert(wire[7] == 0x44 && wire[7] != 0x43);
    size = config_wire_snapshot(wire, 0x43, firmware, 0x12345678u);
    assert(size == 17 && !memcmp(wire + 2, "OSNP", 4));
    size += config_wire_entry(wire + size, 0, 0x20005000u, 428, header, 0x12345678u);
    wire[16]++;
    wire[size++] = 0x4c;
    assert(size == 34 && wire[16] == 1 && wire[17] == 0 && wire[18] == 1);
    assert(!memcmp(wire + 19, "\x20\x00\x50\x00\x01\xac", 6));
    assert(!config_wire_send(&calls, fail_send, wire, size));
    assert(calls == 1);
    return 0;
}
