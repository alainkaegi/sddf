#pragma once

// Requires:
// - stdint.h (inet.h, sk_buf.h)
// - inet.h
// - ip_addr.h (ineth.h)
// - socket.h (sk_buf.h)
// - sk_buf.h

/*
 * Link details
 *
 * Ethernet parameters specified in IEEE Std 802.3-2022, IEEE Standard
 * for Ethernet.
 */
/// Maximum MAC Client Data field size for basic frames (Section 3.2.7, p. 242)
#define ETHER_MTU 1500
/// minFrameSize (Section 4.4.2, Table 4-2, p. 282)
#define MIN_PACKET_SIZE 64
/// maxBasicFrameSize (Section 4.4.2, Table 4-2, p. 282)
#define MAX_PACKET_SIZE 1518

/*
 * The following public definitions are only depended on when running the Linux
 * unit tests. Therefore, only export these publicly when building tests.
 */

/**
 * Given a socket buffer seeded with an IP packet add the Ethernet
 * header and trailer.
 *
 * @param skb  A socket buffer with an IP packet
 * @return a status code
 */
enum inet_status_codes ethernet_wrap(struct sk_buf *skb);

/**
 * Process an incoming Ethernet frame.
 *
 * Check the form of the incoming Ethernet frame.  If any inconsistency
 * arises, this function returns a bad status code and leaves the
 * socket buffer unchanged.  If the frame is well-formed, it strips
 * the socket buffer to the encapsulated IP packet.
 *
 * @param skb  A socket buffer with an incoming Ethernet frame
 * @return a status code
 */
enum inet_status_codes ethernet_unwrap(struct sk_buf *skb);
