#include "gpiod-dispatch-raw.h"

#include <stdio.h>

const char start_symbols[] = { 0x02, 0x00 };
const char stop_symbols[] = { 0x0d, 0x0a, 0x00 };

static const char* package_pattern = "%sDI;%u.%06u;%02u;%u%s";

char buffer[256];

size_t dispatch_raw_pack(char** data, struct timeval ts, uint8_t chan, uint8_t value)
{
    size_t size = snprintf(buffer, sizeof(buffer), package_pattern, start_symbols, ts.tv_sec, ts.tv_usec, chan, value, stop_symbols);
    *data = buffer;
    return size;
}
