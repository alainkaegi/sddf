#pragma once

// Requires:
// - stdint.h

/**
 * A media access control (MAC) address is a unique 48-bit identifier.
 *
 * Standard notation represents a MAC address as six groups of two
 * hexadecimal digits separated by hyphens written in network order.
 * This type holds these groups in that transmission order in the
 * array.
 *
 * Consider the MAC address 00-09-8c-00-69-63.  An instance of {@code
 * mac_addr} holding this address would be laid out in memory, from
 * lower to higher addresses, as follows:
 *
 * mac_addr[0]  00
 *         [1]  09
 *         [2]  8c
 *         [3]  00
 *         [4]  69
 *         [5]  63
 *
 * Converting an instance of {@code mac_addr} in network order
 * requires no special treatment (the bytes are already in
 * transmission order).
 */
typedef uint8_t mac_addr[6];
