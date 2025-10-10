#pragma once

// Requires:
// - <stdint.h>
// - "inet.h" (needs <stdint.h>)
// - "ip_addr.h"
// - "socket.h"
// - "sk_buf.h"

/**
 * IP protocol numbers.
 *
 * Source: https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
 */
#define IPV6           6
#define IP_PROTO_UDP   0x11
#define IP_PROTO_ICMP6 0x3a

#define IP_HEADER_SIZE 40

/**
 * Given a socket buffer seeded with a message add the IP header.
 *
 * The socket buffer is guaranteed to have enough headroom the IP
 * header; no check is needed.
 *
 * @param skb  A socket buffer with a UDP datagram
 * @param proto The IPv6 upper layer protocol number.
 */
void ip_wrap(struct sk_buf *skb, uint8_t proto);

/**
 * Process an incoming packet.
 *
 * Check the form of the incoming IP packet. If any inconsistency arises,
 * this function returns a bad status code and leaves the socket buffer
 * unchanged. If the packet is well-formed, adjust the socket buffer to
 * correspond to UDP's datagram.
 *
 * @param skb  A socket buffer with an incoming packet
 * @return a status code
 */
enum inet_status_codes ip_unwrap(struct sk_buf *skb);
