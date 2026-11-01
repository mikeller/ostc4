# Service-mode debug read (0x20)

In the Discovery application, enter service mode with
`AA AB CD EF`, expect `4B AB CD EF 4C`, then send `20`. The response is
the command echo `20`; send exactly six bytes, big-endian: address bits
31:8 followed by a 24-bit **byte count**. Address bytes `08 00 10`
mean `0x08001000`, not `0x00080010`. Count bytes `00 80 01` request
32,769 bytes. The response is exactly that many bytes in up-to-32,768-byte
UART transfers followed by the service-mode completion prompt `4C`.

Zero length and a count whose end would overflow 32 bits terminate the
session without data or completion prompt. Missing/truncated payloads time
out; UART echo, receive, data transmit and completion prompt failures stop
processing. A transmit error may have sent a partial chunk; no completion
prompt is deliberately sent after a data failure. The command does **not**
validate the memory map or protect against unreadable, device-specific or
sensitive memory: arbitrary addresses can fault or expose data. It is not
the ordinary OSTC3 external-flash read or a safe descriptor snapshot.

Host simulation (runs the same header used by firmware):

```
gcc -std=c11 -Wall -Wextra -Werror -IDiscovery/Inc Discovery/tests/test_debug_read.c -o /tmp/opencode/ostc-debug-read-test
/tmp/opencode/ostc-debug-read-test
```

The standalone bootloader builds `BootLoader/Src/tComm_mini.c`, not this
Discovery handler, and continues to reject 0x20. No physical OSTC4 or OSTC5
has been tested with this command. Do not probe an old application firmware
or bootloader with 0x20; it rejects the command and can end the service session.
