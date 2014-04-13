bluebottle -- a C library for bit-banging TTL async serial communication.

This is useful if you have an inexpensive microcontroller (such as an
[ATtiny85][a85]) that does not have built-in hardware support for
[asynchronous serial IO][UART], or the pins with hardware support
are already being used for another function.

[a85]: http://www.atmel.com/devices/attiny85.aspx
[UART]: http://en.wikipedia.org/wiki/UART

Currently assumes that bytes are sent least-signicant bit first (LSB),
with 8 data bits, no parity bits, and 1 stop bit (8N1), as is relatively
common. (A future version may include those, with an interface change to
the configuration structs.) Note that the bluebottle struct is exported
to allow static allocation, but its existing layout should not be relied
upon.

This library factors out timing entirely - if you're sending/receiving
at 9600 baud, you'll need to call `bluebottle_write_step` and/or
`bluebottle_read_sink` 9,600 times per second, with as little variation
in timing as your hardware can manage, or you'll get corruption. Be sure
to use a crystal whose frequency is a multiple of your baud rate, e.g.
9.6 MHz, 12.96 MHz, or 15.36 MHz for 9600 baud.
