#pragma once

// Requires:
// - stddef.h
// - stdint.h (self, socket.h, ip_addr.h)
// - ip_addr.h (socket.h)
// - socket.h

/**
 * A socket buffer.
 *
 * An instance of this buffer contains all information necessary to
 * process the sending or receiving of a network message efficiently.
 *
 * The principal goal of the organization of a socket buffer is to
 * avoid unnecessary memory copies.
 *
 * A socket buffer is a contiguous region of memory with addresses
 * greater or equal to {@code begin} but strictly less than {@code
 * end}.  In particular the relation {@code begin} ≤ {@code end}
 * holds.
 *
 * A message processed at a particular layer of the network stack is
 * stored within a subrange of the buffer in a contiguous region of
 * memory with addresses greater or equal to {@code first} but
 * strictly less than {@code last}.  In other words, the following
 * relations hold:
 *
 * - {@code first} ≤ {@code last}
 * - {@code begin} ≤ {@code first}
 * - {@code last} ≤ {@code end}
 *
 * Copies of payloads are avoided if
 *
 * - The buffer is large enough to hold the payload, and all headers
 *   and trailers, and
 * - If the initial choice of {@code first} is such that there is
 *   sufficient headroom for all headers.
 */
struct sk_buf {
    void *begin;        ///< The address of the first byte in the buffer
    void *end;          ///< The address of the first byte beyond the buffer
    void *first;        ///< The address of the first byte in the message
    void *last;         ///< The address of the first byte beyond the message
    struct socket *src; ///< Message source
    struct socket *dst; ///< Message destination
    uint64_t err;       ///< A mask of an ICMP error request
};

/**
 * Provide a free buffer on behalf of the NIC. The stack should call this
 * function whenever an empty buffer is needed to send a packet. Ex. sending an
 * ICMP packet, UDP/TCP packet, etc.
 *
 * @return pointer to free sk_buf
 */
struct sk_buf *sk_buf_dequeue_free();

/**
 * Get a socket buffer.
 *
 * To be completed later.  For now, this function returns NULL.
 */
static inline struct sk_buf *sk_buf_get() { return NULL; }
