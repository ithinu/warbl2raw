## WARBL2 firmware 4.1 with raw sensor mode

Original (maybe newer) firmware: https://github.com/amowry/WARBL2

This is a development version with the raw mode enabled by default, throttled to max 2 messages per 3ms. Only pressure, tonehole and button events are enabled.

This mode may require a relatively high bandwidth, thus a single update item
<type, value> is packed into NOTE ON/NOTE OFF messages as follows:

* status = 0x80 + (type & 0x1f);
* byte1 = value & 0x7F;
* byte2 = (value >> 7) & 0x7F;

where type is defined by RAW_TYPE_* constants and value falls within
-8192 ... 8191 max, depending on type. For buttons, state is stored in
individual bits whose numbers are given by RAW_BUTTON_* constants.
