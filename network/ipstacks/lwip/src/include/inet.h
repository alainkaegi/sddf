#pragma once

// Requires:
// - stdint.h

// TBD
// Brought from ethernet.h, ip.h
enum inet_status_codes {
    // The Ethernet processing status codes.
    ETHERNET_GOOD,     ///< All good
    ETHERNET_BAD_CRC,  ///< Bad CRC
    ETHERNET_BAD_LEN,  ///< Bad payload length
    ETHERNET_BAD_TYPE, ///< Not an IPv6 packet
    ETHERNET_NO_SRC,   ///< No neighbor entry for source
    ETHERNET_NO_DST,   ///< No neighbor entry for dest
    // The IP layer processing status codes.
    IP_GOOD_UDP,       ///< The IP packet is valid, next header is UDP
    IP_GOOD_ICMP,      ///< The IP packet is valid, next header is ICMP
    IP_BAD_LENGTH,     ///< The IP packet length is invalid
    IP_BAD_DST_ADDR,   ///< Unexpected destination IP address
    IP_BAD_VERSION,    ///< Unexpected IP version
    IP_BAD_NXT_HDR,    ///< Unexpected next header protocol
};

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
static inline uint64_t hton64(uint64_t data) {
    return ((data & 0xff) << 56) | (((data >> 8) & 0xff) << 48) |
           (((data >> 16) & 0xff) << 40) | (((data >> 24) & 0xff) << 32) |
           (((data >> 32) & 0xff) << 24) | (((data >> 40) & 0xff) << 16) |
           (((data >> 48) & 0xff) << 8) | (((data >> 56) & 0xff));
}
static inline uint32_t hton32(uint32_t data) {
    return ((data & 0xff) << 24) | (((data >> 8) & 0xff) << 16) |
           (((data >> 16) & 0xff) << 8) | (((data >> 24) & 0xff));
}
static inline uint16_t hton16(uint16_t data) {
    return ((data & 0xff) << 8) | (((data >> 8) & 0xff));
}
static inline uint64_t ntoh64(uint64_t data) {
    return ((data & 0xff) << 56) | (((data >> 8) & 0xff) << 48) |
           (((data >> 16) & 0xff) << 40) | (((data >> 24) & 0xff) << 32) |
           (((data >> 32) & 0xff) << 24) | (((data >> 40) & 0xff) << 16) |
           (((data >> 48) & 0xff) << 8) | (((data >> 56) & 0xff));
}
static inline uint32_t ntoh32(uint32_t data) {
    return ((data & 0xff) << 24) | (((data >> 8) & 0xff) << 16) |
           (((data >> 16) & 0xff) << 8) | (((data >> 24) & 0xff));
}
static inline uint16_t ntoh16(uint16_t data) {
    return ((data & 0xff) << 8) | (((data >> 8) & 0xff));
}
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static inline uint64_t hton64(uint64_t data) { return data; }
static inline uint32_t hton32(uint32_t data) { return data; }
static inline uint16_t hton16(uint16_t data) { return data; }
static inline uint64_t ntoh64(uint64_t data) { return data; }
static inline uint32_t ntoh32(uint32_t data) { return data; }
static inline uint16_t ntoh16(uint16_t data) { return data; }
#endif
