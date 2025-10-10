#pragma once

// Requires:
// - stdbool.h
// - stdint.h
// - inet.h

/**
 * An IPv6 address is a 128-bit numerical label.
 *
 * Typically, text represents an IPv6 address as eight groups of 16
 * bits separated by colons.  The groups are stored from most
 * significant to least significant in the array.
 *
 * Consider the IP address 2001:0db8:85a3:0000:0000:8a2e:0370:7334.
 * An instance of {@code ip_addr} holding this address would be laid
 * out in memory, from lower to higher addresses, as follows:
 *
 * ip_addr[0]  2001
 *        [1]  0db8
 *        [2]  85a3
 *        [3]  0000
 *        [4]  0000
 *        [5]  8a2e
 *        [6]  0370
 *        [7]  7334
 *
 * To convert an instance of {@code ip_addr} in network order, one
 * keeps the array order but convert each group to network order.
 */
typedef uint16_t ip_addr[8];

/**
 * A big endian version of {@code ip_addr}.
 */
typedef uint16_t be_ip_addr[8];   // TBD: prefer "be_uint16 be_ip_addr[8];" but then inet.h say "uint16_t hton16(uint16_t)"

typedef struct {
    ip_addr prefix; ///< The IP prefix
    uint64_t length;  ///< The length of the prefix in bits
} __attribute__((packed)) __attribute__((aligned(8))) prefix_t;

/**
 * IPv6 Address types
 */
static const prefix_t unspecified = {
    .prefix = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    .length = 128,
};

static const prefix_t loopback = {
    .prefix = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001},
    .length = 128,
};

static const prefix_t multicast = {
    .prefix = {0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    .length = 8,
};

static const prefix_t ll_unicast = {
    .prefix = {0xFE80, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    .length = 10,
};

static const prefix_t sl_unicast = {
    .prefix = {0xFEC0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    .length = 10,
};

/**
 * Check if a given IP address has the given prefix. A prefix can vary in length
 * from 0 to 128 bits.
 *
 * @param ip  Pointer to an IP address
 * @param prefix  Pointer to the prefix against which the IP address should be
 *                compared.
 * @return true if the IP matches the given prefix
 */
static bool ip_has_prefix(const ip_addr *ip, const prefix_t *prefix) {
    be_ip_addr ip_be  = {0};
    be_ip_addr pfx_be = {0};

    for (int i = 0; i < 8; i++) {
        ip_be[i]  = hton16((*ip)[i]);
        pfx_be[i] = hton16(prefix->prefix[i]);
    }

    uint64_t *ip_pointer = (uint64_t *) ip_be;
    uint64_t ip_former   = ntoh64(*ip_pointer);
    uint64_t ip_latter   = ntoh64(*(ip_pointer + 1));

    uint64_t *pfx_pointer = (uint64_t *) pfx_be;
    uint64_t pfx_former   = ntoh64(*pfx_pointer);
    uint64_t pfx_latter   = ntoh64(*(pfx_pointer + 1));

    uint64_t sft = 64 - prefix->length % 64;
    if (prefix->length < 64) {
        uint64_t ip_f_cleared = (ip_former >> sft) << sft;
        return ip_f_cleared == pfx_former;
    } else {
        uint64_t ip_l_cleared = (ip_latter >> sft) << sft;
        return (ip_former == pfx_former) && (ip_l_cleared == pfx_latter);
    }
}
