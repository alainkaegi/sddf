#pragma once

// Requires:
// - <stddef.h>
// - <stdbool.h>
// - <stdint.h>
// - "ip_addr.h"
// - "socket.h"
// - "sk_buf.h"

/**
 * Send a UDP datagram to the given port at the given destination.
 * @param buf  The pointer to the data to be sent
 * @param size  The amount of data in bytes
 * @param src  Source socket
 * @param dst  Destination socket
 * @return The actual number of bytes sent
 */
size_t udp_send(void *buf,
                size_t size,
                const struct socket *src,
                const struct socket *dst);

/**
 * Given a socket buffer seeded with a message add the UDP header.
 *
 * @param skb  A socket buffer with a (possibly empty) message
 */
void udp_wrap(struct sk_buf *skb);

/**
 * Process an incoming UDP datagram.
 *
 * Check the form of the incoming UDP datagram.  If any inconsistency
 * arises, this function returns a bad status code and leaves the
 * socket buffer unchanged.  If the packet is well-formed, it strips
 * the socket buffer to the encapsulated application's message.
 *
 * @param skb  A socket buffer with an incoming UDP datagram
 * @return a status code
 */
enum inet_status_codes udp_unwrap(struct sk_buf *skb);
