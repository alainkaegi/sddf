#include <stddef.h>
#include <stdint.h>

#include "crc32.h"

uint32_t crc32(const void *buf, size_t len) {
    uint32_t crc = 0xffffffffU;

    const char *p = (const char *) buf;

    for (size_t i = 0; i < len; ++i) {
        char ch = p[i];
        for (size_t j = 0; j < 8; ++j) {
            uint32_t b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b) crc = crc ^ 0xedb88320U;
            ch >>= 1;
        }
    }

    return ~crc;
}
