#ifndef TCOMM_DEBUG_READ_H
#define TCOMM_DEBUG_READ_H

#include <stdint.h>

typedef int (*tCommDebugReceive)(void *context, uint8_t *data, uint16_t size, uint32_t timeout);
typedef int (*tCommDebugTransmit)(void *context, uint32_t address, uint16_t size, uint32_t timeout);
typedef int (*tCommDebugReply)(void *context, const uint8_t *data, uint16_t size, uint32_t timeout);

/* Service mode 0x20: three big-endian address bytes represent bits 31:8,
 * followed by a three-byte big-endian BYTE count. This is an unrestricted
 * debug read, not a guarantee that the selected address can be dereferenced.
 * Returns zero without a completion prompt on any framing or UART failure.
 */
static inline int tCommDebugRead(void *context, tCommDebugReceive receive,
                                 tCommDebugTransmit transmit, tCommDebugReply reply)
{
    uint8_t request[6];
    uint8_t marker = 0x20;
    uint32_t address, length;

    if (!reply(context, &marker, 1, 500u) ||
        !receive(context, request, sizeof(request), 1000u))
        return 0;

    address = ((uint32_t)request[0] << 24) | ((uint32_t)request[1] << 16) |
              ((uint32_t)request[2] << 8);
    length = ((uint32_t)request[3] << 16) | ((uint32_t)request[4] << 8) |
             (uint32_t)request[5];
    if (length == 0 || length > UINT32_MAX - address)
        return 0;

    while (length != 0) {
        uint16_t chunk = length > 0x8000u ? 0x8000u : (uint16_t)length;
        if (!transmit(context, address, chunk, 60000u))
            return 0;
        address += chunk;
        length -= chunk;
    }

    marker = 0x4C;
    return reply(context, &marker, 1, 10000u);
}

/* Service-mode filter and debug dispatch. Other service commands continue
 * through select_mode(); zero ends the session, 0x4c keeps it open. */
#define TCOMM_DEBUG_CONTINUE 0xffu
static inline uint8_t tCommDebugRoute(uint8_t type, void *context,
                                      tCommDebugReceive receive,
                                      tCommDebugTransmit transmit, tCommDebugReply reply)
{
    if ((type < 0x60u && type != 0x20u) || type == 0xffu)
        return 0;
    if (type == 0x20u)
        return tCommDebugRead(context, receive, transmit, reply) ? 0x4cu : 0;
    return TCOMM_DEBUG_CONTINUE;
}

#endif
