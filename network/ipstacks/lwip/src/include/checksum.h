#pragma once

// Requires
// - <stddef.h>
// - <stdint.h>

/**
 * Compute the Internet checksum for {@code len} bytes beginning at
 * location {@code addr}.
 *
 * RFC 1071 (https://datatracker.ietf.org/doc/html/rfc1071) describes
 * this operation in details.
 *
 * This code supports a buffer of up to 65,547 bytes (governed by the
 * needs of the IP and UDP protocols).
 *
 * Adjacent octets to be checksummed are paired to form 16-bit
 * integers, and the 1's complement sum of these 16-bit integers is
 * formed.
 *
 * If the number of bytes to be checksummed is odd, an extra
 * zero-valued byte is padded at the end of the buffer to be
 * checksummed.
 *
 * This implementation achieves the latter with a code sequence that
 * only works on little endian machines.
 *
 * @param buf  buffer of bytes
 * @param len  number of bytes
 * @return the Internet checksum of the given array of bytes
 */
uint16_t checksum(const void *buf, size_t len);

/**
 * Calculate the checksum over two buffers. This is useful for calculating the
 * checksum over a pseudo-header and packet without modifying the packet
 * in-place.
 */
uint16_t double_buffered_checksum(const void *restrict b,
                                  size_t bl,
                                  const void *restrict c,
                                  size_t cl);
