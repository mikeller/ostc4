# Configuration snapshot service protocol (application only)

Fork-local allocation: service commands `90` and `91`. Enter service mode with
`AA AB CD EF`, expect `4B AB CD EF 4C`. `90` has no request payload and does
not capture. Reply: echo `90`, ASCII `OCFG`, version `01`, capability `01`,
connected model (`43` OSTC4 or `44` OSTC5), four firmware version bytes
(major, minor, patch, beta), `4C`. A legacy prompt-only `4C`, a bootloader,
an unexpected echo/magic/version/capability/model, or a truncated reply is
**not supported**. The existing download-mode `60` response is unchanged.

After **positive** discovery, `91` has no request payload. Its reply begins
`91 00` on success or `91 01 4C` on capture failure. Success continues with
ASCII `OSNP`, version `01`, connected model `43`/`44`, four firmware bytes,
4-byte opaque native ABI identity, one-byte descriptor count (1..5), then
exactly that many 16-byte descriptors and final `4C`. Each descriptor has:

| Bytes | Meaning |
| --- | --- |
| 0 | Type: 0 live current settings, 1..4 stored configuration profile 0..3 |
| 1 | Domain: 1 = CPU byte-addressable RAM, **not external flash** |
| 2..5 | Absolute 32-bit CPU address, unsigned big-endian, low byte zero |
| 6..7 | Byte count, unsigned big-endian, nonzero and at most 512 |
| 8..11 | Opaque native settings header/schema identifier, big-endian |
| 12..15 | Opaque native ABI identity, big-endian |

Example discovery for an OSTC4 running firmware 1.7.7 beta 1:
`90 4F 43 46 47 01 01 43 01 07 07 01 4C`.
Example success prefix: `91 00 4F 53 4E 50 01 43 01 07 07 01`
followed by the build-specific ABI, count, descriptors and `4C`. Addresses,
schema and ABI are build-dependent: do not hardcode example addresses.

Regions are separate static 512-byte aligned buffers, with exactly
`sizeof(SSettings)` bytes advertised. The `20` debug-read address field is
the high 24 bits of the CPU address (`addr >> 8`), big-endian; its length
field is a 24-bit big-endian **byte** count. For example address `0x20005000`
and length 428 (`0x01ac`) use `20`, then `20 00 50 00 01 AC` (six request
bytes); expect echo `20`, 428 bytes, `4C`. This is illustrative only; the
host must use the actual validated descriptor. Never use flash offsets with
`20`. `20` remains unrestricted and unsafe for arbitrary addresses.

The first successful `91` in a service session copies current settings and,
when profile support is compiled in, reads each stored profile from its
own external-flash slot. Only a complete latest native record with matching
header is advertised. Erased, invalid, unsupported and disabled profiles
are omitted, not replaced with potentially stale profile RAM. Flash read
failure returns status 1, not partial descriptors. Subsequent `91` replies
reuse the **same captured bytes** and descriptors until the service session
exits; capture is invalidated and buffers cleared at exit and before a new
session. Addresses must never be reused by a host across reconnect. No
migrations, flash writes, semantic decoding or configuration writes occur.

Native ABI identity encodes the build's compiler major/minor, struct size and
last-field offset; firmware identity and header/schema must also match a
host's explicitly validated allowlist. This is an opaque representation,
**not** a guarantee of portable C layout or semantic settings compatibility.
Reject unknown identifiers, models, domains, overlapping/unaligned/overflowing
ranges, oversized counts and partial replies; never guess a fallback schema.
Host integration awaits separate validation of the transport and capability.

Focused fixture and base transport regression:

```
gcc -std=c11 -Wall -Wextra -Werror -IDiscovery/Inc Discovery/tests/test_config_snapshot.c -o /tmp/opencode/ostc-config-test
/tmp/opencode/ostc-config-test
gcc -std=c11 -Wall -Wextra -Werror -IDiscovery/Inc Discovery/tests/test_debug_read.c -o /tmp/opencode/ostc-debug-read-test
/tmp/opencode/ostc-debug-read-test
make firmware_binary
```
