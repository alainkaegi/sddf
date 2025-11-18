#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "checksum.h"
#include "inet.h"
#include "ip_addr.h"
#include "socket.h"
#include "sk_buf.h"
#include "dep_ip.h"

#include "dep_udp.h"

#include <sddf/util/printf.h>

#define UDP_ADD_CHKSUM true

//#include "assert.h"

struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
};

struct udp_pseudo_header {
    ip_addr  src_addr;
    ip_addr  dst_addr;
    uint32_t length;
    uint8_t  zeroes[3];
    uint8_t  protocol : 8;
    struct udp_header udp_header;
};

enum inet_status_codes echo(struct sk_buf *skb);

void udp_dump(struct udp_pseudo_header *hd) {
  sddf_dprintf("VIP|UDP|INFO: %hu > %hu, length %hu\n",
              ntoh16(hd->udp_header.src_port), ntoh16(hd->udp_header.dst_port),
              ntoh16(hd->udp_header.length));
}

enum inet_status_codes udp_wrap(struct sk_buf *skb) {
    // There should be enough headroom.
#if UDP_ADD_CHKSUM == true
    assert((skb->first - skb->begin) >= sizeof(struct udp_pseudo_header));
#else
    assert((skb->first - skb->begin) >= sizeof(struct udp_header));
#endif

    // Measure the payload size in bytes.
    size_t size = skb->last - skb->first;

#if UDP_ADD_CHKSUM == true
    // Compute the address of the UDP pseudo header (which includes
    // the UDP header among other items).
    struct udp_pseudo_header *hd =
        skb->first - sizeof(struct udp_pseudo_header);

    // Fill the pseudo header.
    for (int i = 0; i < 8; ++i) {
        hd->src_addr[i] = hton16(skb->src->ip_addr[i]);
        hd->dst_addr[i] = hton16(skb->dst->ip_addr[i]);
    }
    hd->length    = hton32(size + sizeof(struct udp_header));
    hd->zeroes[0] = hd->zeroes[1] = hd->zeroes[2] = 0;
    hd->protocol                                  = IP_PROTO_UDP;
    hd->udp_header.src_port                       = hton16(skb->src->port);
    hd->udp_header.dst_port                       = hton16(skb->dst->port);
    hd->udp_header.length   = hton16(size + sizeof(struct udp_header));
    hd->udp_header.checksum = 0;
    // Checksum is already in network order since it is computed based
    // on a buffer already in that order.
    hd->udp_header.checksum =
        checksum(hd, size + sizeof(struct udp_pseudo_header));

    // Adjust the buffer to account for the UDP header (but not the
    // pseudo header).
    skb->first = skb->first - sizeof(struct udp_header);
#else
    // Compute the address of the UDP header.
    struct udp_header *hd = skb->first - sizeof(struct udp_header);
    hd->src_port = hton16(skb->src->port);
    hd->dst_port = hton16(skb->dst->port);
    hd->length   = hton16(size + sizeof(struct udp_header));
    hd->checksum = 0;
#endif

    return ip_wrap(skb, IP_PROTO_UDP);
}

enum inet_status_codes udp_unwrap(struct sk_buf *skb) {
#if UDP_CHK_CHKSUM == true
    // There should be enough headroom to check the UDP checksum.
    assert((skb->first - skb->begin) >=
           (sizeof(struct udp_pseudo_header) - sizeof(struct udp_header)));

    // Compute the address of the UDP pseudo header.
    struct udp_pseudo_header *hd =
        skb->first -
        (sizeof(struct udp_pseudo_header) - sizeof(struct udp_header));

    // Fill the pseudo header.
    for (int i = 0; i < 8; ++i) {
        hd->src_addr[i] = hton16(skb->src->ip_addr[i]);
        hd->dst_addr[i] = hton16(skb->dst->ip_addr[i]);
    }
    hd->length    = hton32(skb->last - skb->first);
    hd->zeroes[0] = hd->zeroes[1] = hd->zeroes[2] = 0;
    hd->protocol                                  = IP_PROTO_UDP;

    udp_dump(hd);

    // Measure the data size.
    size_t size = skb->last - skb->first - sizeof(struct udp_header);

    // Check the length.
    if ((skb->last - skb->first) != ntoh16(hd->udp_header.length)) {
        return UDP_BAD_LENGTH;
    }

    // Verify the checksum.
    if (checksum(hd, sizeof(struct udp_pseudo_header) + size) != 0x0000) {
        return UDP_BAD_CHECKSUM;
    }

    // Set ports
    skb->dst->port = ntoh16(hd->udp_header.dst_port);
    skb->src->port = ntoh16(hd->udp_header.src_port);
#else
    struct udp_header *hd = skb->first;

    // Check the length.
    if ((skb->last - skb->first) != ntoh16(hd->length)) {
        return UDP_BAD_LENGTH;
    }

    // Set ports
    skb->dst->port = ntoh16(hd->dst_port);
    skb->src->port = ntoh16(hd->src_port);
#endif

    // Fix the socket buffer.
    skb->first += sizeof(struct udp_header);

    return echo(skb);
}

enum inet_status_codes echo(struct sk_buf *skb) {
    // Flip the sockets
    struct socket *tmp = NULL;
    tmp      = skb->dst;
    skb->dst = skb->src;
    skb->src = tmp;

    // Send the response (an echo)
    return udp_wrap(skb);
}

#if 0
size_t udp_send(void *buf,
                size_t size,
                const struct socket *src,
                const struct socket *dst) {
    // Grab a free sk_buf
    struct sk_buf *skb = acquire_free_buf();

    // Set pointers
    skb->src   = (struct socket *) src;
    skb->dst   = (struct socket *) dst;
    skb->first = skb->begin + 96; // TODO: Fix hardcoding
    skb->last  = skb->first + size;
    skb->err   = 0;

    memcpy(skb->first, buf, size);
    udp_wrap(skb);
    queue_enqueue(_tx, (uint64) skb);

    return size;
}

int udp_echo_recv() {
    struct socket *tmp = NULL;
    int packets_echoed = 0;

    while (!queue_is_empty(_rx)) {
        struct sk_buf *skb = (struct sk_buf *) queue_dequeue(_rx);
        assert(skb != NULL);

        switch (udp_unwrap(skb)) {
            case UDP_GOOD:
                // Flip sockets and return
                tmp      = skb->dst;
                skb->dst = skb->src;
                skb->src = tmp;

                udp_wrap(skb);
                queue_enqueue(_tx, (uint64) skb);
                packets_echoed++;
                break;

            case UDP_BAD_PORT:
                drop(skb, "udp: unwrap bad port\n");
                break;

            case UDP_BAD_CHECKSUM:
                drop(skb, "udp: unwrap bad checksum\n");
                break;

            case UDP_BAD_LENGTH:
                drop(skb, "udp: unwrap bad length\n");
                break;

            default:
                drop(skb, "udp: unrecognized unwrap code\n");
                break;
        }
    }

    return packets_echoed;
}
#endif
