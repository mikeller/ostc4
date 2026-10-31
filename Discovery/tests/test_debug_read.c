#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "tCommDebugRead.h"

struct mock {
    uint8_t request[6];
    uint32_t address[3];
    uint16_t size[3];
    uint8_t reply[2];
    unsigned reads, writes, replies;
    unsigned fail_read, fail_write, fail_reply;
};

static int receive(void *context, uint8_t *data, uint16_t size, uint32_t timeout)
{
    struct mock *m = context;
    assert(size == 6 && timeout == 1000u);
    m->reads++;
    if (m->fail_read) return 0;
    memcpy(data, m->request, size);
    return 1;
}

static int transmit(void *context, uint32_t address, uint16_t size, uint32_t timeout)
{
    struct mock *m = context;
    assert(m->writes < 3 && timeout == 60000u);
    m->address[m->writes] = address;
    m->size[m->writes++] = size;
    return m->fail_write != m->writes;
}

static int reply(void *context, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    struct mock *m = context;
    assert(size == 1 && m->replies < 2);
    assert(timeout == (m->replies ? 10000u : 500u));
    m->reply[m->replies++] = *data;
    return m->fail_reply != m->replies;
}

static struct mock sample(void)
{
    struct mock m = {.request = {0x08, 0x00, 0x10, 0x00, 0x80, 0x01}};
    return m;
}

int main(void)
{
    struct mock m = sample();
    assert(tCommDebugRead(&m, receive, transmit, reply) == 1);
    assert(m.reads == 1 && m.writes == 2 && m.replies == 2);
    assert(m.reply[0] == 0x20 && m.reply[1] == 0x4c);
    assert(m.address[0] == 0x08001000u && m.address[1] == 0x08009000u);
    assert(m.size[0] == 0x8000 && m.size[1] == 1);

    m = sample(); m.request[3] = m.request[4] = m.request[5] = 0;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 0 && m.replies == 1);

    m = sample(); m.request[0] = m.request[1] = m.request[2] = 0xff;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 0 && m.replies == 1);

    m = sample(); m.fail_reply = 1;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.reads == 0 && m.writes == 0 && m.replies == 1);

    m = sample(); m.fail_read = 1;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 0 && m.replies == 1);

    m = sample(); m.fail_write = 1;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 1 && m.replies == 1);

    m = sample(); m.fail_write = 2;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 2 && m.replies == 1);

    m = sample(); m.fail_reply = 2;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 2 && m.replies == 2);

    m = sample(); m.request[0] = m.request[1] = m.request[2] = 0xff;
    m.request[3] = 0xff; m.request[4] = 0xff; m.request[5] = 0xff;
    assert(!tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 0 && m.replies == 1); /* 24-bit maximum, address + count wraps */

    m = sample(); m.request[0] = m.request[1] = m.request[2] = 0;
    m.request[3] = 0; m.request[4] = 0; m.request[5] = 1;
    assert(tCommDebugRead(&m, receive, transmit, reply));
    assert(m.writes == 1 && m.size[0] == 1);
    return 0;
}
