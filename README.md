bluebottle -- a C library for bit-banging TTL async serial communication.

Currently assumes that bytes are sent least-signicant bit first (LSB),
with 8 data bits, no parity bits, and 1 stop bit (8N1).

This library factors out timing entirely - if you're sending/receiving
at 9600 baud, you'll need to call `bluebottle_write_step` and/or
`bluebottle_read_sink` 9,600 times per second, with as little variation
in timing as your hardware can manage, or you'll get corruption. Be sure
to use a crystal whose frequency is a multiple of your baud rate, e.g.
9.6 MHz, 12.96 MHz, or 15.36 MHz for 9600 baud.
