#pragma once

// Requires:
// - stddef.h
// - stdint.h

/**
 * Compute ISO 3309's CRC-32 of {@code len} bytes beginning at
 * location {@code addr}.
 *
 * This implementation is adapted from code found elsewhere
 * (https://lxp32.github.io/docs/a-simple-example-crc32-calculation/).
 *
 * @param buf  buffer of bytes
 * @param len  number of bytes
 * @return ISO 3309's CRC-32 of the given array of bytes
 */
uint32_t crc32(const void *buf, size_t len);
