#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "inet.h"
#include "ip_addr.h"
#include "socket.h"
#include "sk_buf.h"

#include "dep_ip.h"

#include <sddf/util/printf.h>

#include "dep_udp.h"

//#include "assert.h"
//#define assert(expr) (assert is defined in util.h imported from printf.h)
//#include "icmp.h"
//#include "inet.h"
//#include "ip_addr.h"
//#include "queue.h"
//#include "sk_buf.h"
//#include "stack.h"
//#include "string.h"

/**
 * The header for an IPv6 packet.
 *
 * Note: GCC's default endianness for bit field matches the
 * underlying's byte ordering (see GCC's scalar storage order).  So it
 * appears difficult to use bitfields with unaligned fields such as
 * {@code traffic_class}.
 */
// TBD: used to be be_uint*
struct ip_header {
    uint32_t vtf; ///< Version, traffic class, and flow label
    uint16_t payload_length;
    uint8_t next_header;
    uint8_t hop_limit;
    ip_addr src_addr;
    ip_addr dst_addr;
};

static inline unsigned int get_version(const struct ip_header *hd) {
    return (ntoh32(hd->vtf) >> 28) & 0xf;
}

static inline void set_vtf(struct ip_header *hd,
                           unsigned int version,
                           unsigned int traffic_class,
                           unsigned int flow_label) {
    hd->vtf = hton32((version & 0xf) << 28 | (traffic_class & 0xff) << 20 |
                     (flow_label & 0xfffff));
}

void ip_dump(struct ip_header *hd) {
    sddf_printf("IP|ERROR: %04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx\n",
                ntoh16(hd->src_addr[0]), ntoh16(hd->src_addr[1]),
                ntoh16(hd->src_addr[2]), ntoh16(hd->src_addr[3]),
                ntoh16(hd->src_addr[4]), ntoh16(hd->src_addr[5]),
                ntoh16(hd->src_addr[6]), ntoh16(hd->src_addr[7]));
    sddf_printf("IP|ERROR: > %04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx, length %hu, next %hx\n",
                ntoh16(hd->dst_addr[0]), ntoh16(hd->dst_addr[1]),
                ntoh16(hd->dst_addr[2]), ntoh16(hd->dst_addr[3]),
                ntoh16(hd->dst_addr[4]), ntoh16(hd->dst_addr[5]),
                ntoh16(hd->dst_addr[6]), ntoh16(hd->dst_addr[7]),
                ntoh16(hd->payload_length),
                hd->next_header);
}

void ip_wrap(struct sk_buf *skb, uint8_t proto) {
    // There should be enough headroom to prepend the IP header.
    assert((skb->first - skb->begin) >= sizeof(struct ip_header));

    // Measure the payload size in bytes.
    size_t size = skb->last - skb->first;

    // Compute the address of the IP header.
    struct ip_header *hd = skb->first - sizeof(struct ip_header);
    // Fill the header.
    set_vtf(hd, IPV6, 0, 0);
    hd->payload_length = hton16(size);
    hd->next_header    = proto;
    hd->hop_limit      = 64;
    for (int i = 0; i < 8; ++i) {
        hd->src_addr[i] = hton16(skb->src->ip_addr[i]);
        hd->dst_addr[i] = hton16(skb->dst->ip_addr[i]);
    }

    // Adjust the buffer to account for the IP header.
    skb->first = skb->first - sizeof(struct ip_header);

    enum inet_status_codes status = ethernet_wrap(skb);
    assert(status == ETHERNET_GOOD);
}

enum inet_status_codes ip_unwrap(struct sk_buf *skb) {
    // Compute the address of the IP header.
    struct ip_header *hd = skb->first;

    ip_dump(hd);

    // Measure the data size.
    size_t size = skb->last - skb->first - sizeof(struct ip_header);

    // Is this an IPv6 packet?
    if (IPV6 != get_version(hd)) {
        return IP_BAD_VERSION;
    }

    // Is the length valid?
    if (size != ntoh16(hd->payload_length)) {
        return IP_BAD_LENGTH;
    }

    /* Set the socket buffer's address fields */
    for (int i = 0; i < 8; ++i) {
        skb->src->ip_addr[i] = ntoh16(hd->src_addr[i]);
        skb->dst->ip_addr[i] = ntoh16(hd->dst_addr[i]);
    }

    // TODO: Are there any checks for the source address?

    // Adjust the buffer to account for the IP header.
    skb->first = skb->first + sizeof(struct ip_header);

    // Determine what the next header is
    switch (hd->next_header) {
        case IP_PROTO_UDP:
        {
            enum inet_status_codes status = udp_unwrap(skb);
            if (status != UDP_GOOD)
               return status;
            return IP_GOOD_UDP;
        }
        case IP_PROTO_ICMP6:
            return IP_GOOD_ICMP;
        default:
            return IP_BAD_NXT_HDR;
    }
}
