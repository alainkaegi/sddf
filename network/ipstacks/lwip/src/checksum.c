#include <stddef.h>
#include <stdint.h>

#include "checksum.h"

// Implementation adapted from RFC 1071
// (https://datatracker.ietf.org/doc/html/rfc1071).
uint16_t checksum(const void *buf, size_t len) {
    uint32_t sum = 0;

    while (len > 1) {
        sum += *(uint16_t *) buf;
        buf += 2;
        len -= 2;
    }

    // Add left-over byte, if any.  Careful, this code sequence only
    // works on little endian machines.
    if (len > 0) sum += *(uint8_t *) buf;

    // Fold 32-bit sum to 16 bits.
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

uint16_t double_buffered_checksum(const void *restrict b,
                                  size_t bl,
                                  const void *restrict c,
                                  size_t cl) {
    uint32_t sum = 0;

    while (bl > 1) {
        sum += *(uint16_t *) b;
        b += 2;
        bl -= 2;
    }

    if (bl > 0) sum += *(uint8_t *) b;

    while (cl > 1) {
        sum += *(uint16_t *) c;
        c += 2;
        cl -= 2;
    }

    if (cl > 0) sum += *(uint8_t *) c;

    // Fold 32-bit sum to 16 bits.
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}
