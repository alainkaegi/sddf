#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "mac_addr.h"
#include "crc32.h"
#include "inet.h"
#include "ip_addr.h"
#include "socket.h"
#include "sk_buf.h"
#include "ndp.h"

#include "dep_ethernet.h"

//#include "assert.h"
#define assert(expr)
//#include "config.h"
//#include "icmp.h"
//#include "io.h"

/**
 * The header for an Ethernet frame.
 */
struct ethernet_header {
    mac_addr dst_addr;
    mac_addr src_addr;
    uint16_t ethertype;
};

/**
 * The trailer of an Ethernet frame.
 */
struct ethernet_trailer {
    uint32_t crc;
};

static const uint16_t ETHERTYPE_IPV6 = 0x86dd;

#define MAX(a, b)             \
    ({                        \
        __auto_type _a = (a); \
        __auto_type _b = (b); \
        _a > _b ? _a : _b;    \
    })

enum inet_status_codes ethernet_wrap(struct sk_buf *skb) {
    // Measure the payload size in bytes.
    size_t size = skb->last - skb->first;

    // Measure the size of padding; computation must take place in
    // signed arithmetic!
    // TBD: are packets always at least 64 bytes?
    // clang-format off
    size_t padding = MAX(0,
                         MIN_PACKET_SIZE
                         - (int)size
                         - (int)sizeof(struct ethernet_header)
                         - (int)sizeof(struct ethernet_trailer));

    // Check that we do not exceed the payload size.
    assert(size <= MAX_PACKET_SIZE - sizeof(struct ethernet_header)
                                   - sizeof(struct ethernet_trailer));
    // clang-format on

    // There should be enough headroom to prepend the Ethernet header.
    assert((skb->first - skb->begin) >= sizeof(struct ethernet_header));

    // There should be enough room to append the Ethernet trailer.
    assert((skb->end - skb->last) >= padding + sizeof(struct ethernet_trailer));

    // Compute the address of the Ethernet header.
    struct ethernet_header *hd = skb->first - sizeof(struct ethernet_header);

    neighbor_t *host = ndp_nbr_cache_get(&skb->src->ip_addr);
    // Bail if we cannot find ourself
    if (host == NULL) return ETHERNET_NO_SRC;

    // Fill addresses
    hd->src_addr[0] = host->link_addr[0];
    hd->src_addr[1] = host->link_addr[1];
    hd->src_addr[2] = host->link_addr[2];
    hd->src_addr[3] = host->link_addr[3];
    hd->src_addr[4] = host->link_addr[4];
    hd->src_addr[5] = host->link_addr[5];

    ip_addr *dst_nxt_hop = ndp_dst_cache_get(&skb->dst->ip_addr);
    // Bail if we cannot find destination
    if (dst_nxt_hop == NULL) return ETHERNET_NO_DST;

    neighbor_t *dst_nbr = ndp_nbr_cache_get(dst_nxt_hop);
    // If we can find the next hop IP, assume we can find the nbr entry
    assert(dst_nbr != NULL);
    hd->dst_addr[0] = dst_nbr->link_addr[0];
    hd->dst_addr[1] = dst_nbr->link_addr[1];
    hd->dst_addr[2] = dst_nbr->link_addr[2];
    hd->dst_addr[3] = dst_nbr->link_addr[3];
    hd->dst_addr[4] = dst_nbr->link_addr[4];
    hd->dst_addr[5] = dst_nbr->link_addr[5];

    hd->ethertype = hton16(ETHERTYPE_IPV6);

    // Adjust the buffer to account for the Ethernet header.
    skb->first = skb->first - sizeof(struct ethernet_header);

    // TODO: Necessary to zero padding?

#if ETH_ADD_CRC == true
    // Compute the address of the Ethernet trailer.
    struct ethernet_trailer *tl = skb->last + padding;
    // Fill the trailer.
    tl->crc = crc32(skb->first, skb->last - skb->first + padding);

    // Adjust the padding to account for the Ethernet trailer.
    padding += sizeof(struct ethernet_trailer);
#endif

    // Adjust the buffer to account for padding.
    skb->last = skb->last + padding;

    return ETHERNET_GOOD;
}

enum inet_status_codes ethernet_unwrap(struct sk_buf *skb) {
    struct ethernet_header *hd  = skb->first;
    struct ethernet_trailer *tl = skb->last - sizeof(struct ethernet_trailer);

    // Measure payload length.
    size_t size = (void *) tl - ((void *) hd + sizeof(struct ethernet_header));

    // Check ethertype
    if (ntoh16(hd->ethertype) != ETHERTYPE_IPV6) return ETHERNET_BAD_TYPE;

    // Check payload length fits within a frame's max transfer size.
    if (size > ETHER_MTU) {
        return ETHERNET_BAD_LEN;
    }

    // Compute CRC on received payload.
    uint32_t crc = crc32(skb->first, (void *) tl - (void *) hd);

    // Check CRC's match.
    if (crc != tl->crc) {
        return ETHERNET_BAD_CRC;
    }

    skb->first = skb->first + sizeof(struct ethernet_header);
    skb->last  = skb->last - sizeof(struct ethernet_trailer);

    return ETHERNET_GOOD;
}
