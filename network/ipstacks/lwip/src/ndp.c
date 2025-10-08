#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "dep_queue.h"
#include "inet.h"
#include "mac_addr.h"
#include "ip_addr.h"
//#include "assert.h"
#define assert(expr)
#include "dep_cache.h"
#include "container.h"
#include "ndp.h"

// Reserve a memory region to hold the entries of each cache
uint8_t nbr_entries[ENTRIES_PER_CACHE * (sizeof(dep_entry_t) + sizeof(neighbor_t))];
uint8_t dst_entries[ENTRIES_PER_CACHE * (sizeof(dep_entry_t) + sizeof(dest_map_t))];
uint8_t pfx_entries[ENTRIES_PER_CACHE * (sizeof(dep_entry_t) + sizeof(prefix_t))];

// Reserve a memory region to hold 'free' entries
static uint8_t queue_data[3][sizeof(struct dep_queue) + ENTRIES_PER_CACHE * 8];

// Associate queues with their memory regions
struct dep_queue *nbr_free_entries = (struct dep_queue *) queue_data[0];
struct dep_queue *dst_free_entries = (struct dep_queue *) queue_data[1];
struct dep_queue *pfx_free_entries = (struct dep_queue *) queue_data[2];

// The handles on our caches
dep_cache_t neighbors    = {0};
dep_cache_t destinations = {0};
dep_cache_t prefixes     = {0};

// Our compaison function
int ip_compar(const void *a, const void *b) {
    // We are comparing IPv6 addresses, so split each address into a pair of 64
    // bit ints. First cast to a uint64_t pointer so we can leverage pointer
    // arithmetic.
    uint64_t *a_pointer = (uint64_t *) a;
    uint64_t a_former   = ntoh64(*a_pointer);
    uint64_t a_later    = ntoh64(*(a_pointer + 1));

    uint64_t *b_pointer = (uint64_t *) b;
    uint64_t b_former   = ntoh64(*b_pointer);
    uint64_t b_later    = ntoh64(*(b_pointer + 1));

    // Find the difference of first set of ints
    volatile long less = a_former - b_former;

    // If the first set matches, find the difference of the second
    // set
    if (less == 0) less = a_later - b_later;

    // Return an int
    if (less == 0)
        return 0;
    else if (less > 0)
        return 1;
    else
        return -1;
};

void ndp_init_caches() {
    dep_queue_init(nbr_free_entries, ENTRIES_PER_CACHE);
    dep_queue_init(dst_free_entries, ENTRIES_PER_CACHE);
    dep_queue_init(pfx_free_entries, ENTRIES_PER_CACHE);

    // Enqueue the free entries for the neighbor cache
    size_t entry_size = sizeof(dep_entry_t) + sizeof(neighbor_t);
    for (int i = 0; i < ENTRIES_PER_CACHE; i++) {
        dep_entry_t *entry = (dep_entry_t *) (nbr_entries + (i * entry_size));
        dep_queue_enqueue(nbr_free_entries, (uint64_t) entry);
    }

    // Enqueue the free entries for the destination cache
    entry_size = sizeof(dep_entry_t) + sizeof(dest_map_t);
    for (int i = 0; i < ENTRIES_PER_CACHE; i++) {
        dep_entry_t *entry = (dep_entry_t *) (dst_entries + (i * entry_size));
        dep_queue_enqueue(dst_free_entries, (uint64_t) entry);
    }

    // Enqueue the free entries for the prefix cache
    entry_size = sizeof(dep_entry_t) + sizeof(prefix_t);
    for (int i = 0; i < ENTRIES_PER_CACHE; i++) {
        dep_entry_t *entry = (dep_entry_t *) (pfx_entries + (i * entry_size));
        dep_queue_enqueue(pfx_free_entries, (uint64_t) entry);
    }

    dep_cache_init(&neighbors, ip_compar);
    dep_cache_init(&destinations, ip_compar);
    dep_cache_init(&prefixes, ip_compar);
}

void ndp_nbr_cache_add(neighbor_t *nbr) {
    // TODO: Do something if we can't queue_dequeue a free buffer
    dep_entry_t *entry = (dep_entry_t *) dep_queue_dequeue(nbr_free_entries);
    assert(nbr != NULL);
    assert(entry != NULL);
    assert(entry->data != (void *) (entry + sizeof(dep_entry_t)));

    // TODO: Can we avoid this copy somehow?
    memcpy(entry->data, nbr, sizeof(neighbor_t));
    entry->length   = sizeof(neighbor_t);
    entry->key      = ((neighbor_t *) entry->data)->addr;
    entry->node.key = entry->key;

    dep_cache_codes_t sts = dep_cache_add(&neighbors, entry);

    if (sts == EXISTS) {
        dep_cache_del(&neighbors, (char *) &nbr->addr);
        sts = dep_cache_add(&neighbors, entry);
        assert(sts == SUCCESS);
    }
}

neighbor_t *ndp_nbr_cache_get(ip_addr *nbr) {
    dep_entry_t *entry = dep_cache_get(&neighbors, (char *) nbr);
    if (!entry)
        return NULL;
    else
        return (neighbor_t *) entry->data;
}

void ndp_nbr_cache_del(ip_addr *nbr) {
    dep_cache_del(&neighbors, (void *) nbr);
    dep_queue_enqueue(nbr_free_entries,
                  (uint64_t) container_of(nbr, dep_entry_t, data));
}

void ndp_dst_cache_add(dest_map_t *dst) {
    // TODO: Do something if we can't queue_dequeue a free buffer
    dep_entry_t *entry = (dep_entry_t *) dep_queue_dequeue(dst_free_entries);
    assert(dst != NULL);
    assert(entry != NULL);
    assert(entry->data != (void *) (entry + sizeof(dep_entry_t)));

    // TODO: Can we avoid this copy somehow?
    memcpy(entry->data, dst, sizeof(dest_map_t));
    entry->length   = sizeof(dest_map_t);
    entry->key      = ((dest_map_t *) entry->data)->dst;
    entry->node.key = entry->key;

    dep_cache_codes_t sts = dep_cache_add(&destinations, entry);

    if (sts == EXISTS) {
        dep_cache_del(&destinations, (char *) &dst->dst);
        sts = dep_cache_add(&destinations, entry);
        assert(sts == SUCCESS);
    }
}

ip_addr *ndp_dst_cache_get(ip_addr *dst) {
    dep_entry_t *entry = dep_cache_get(&destinations, (char *) dst);
    if (!entry)
        return NULL;
    else
        return (ip_addr *) ((dest_map_t *) entry->data)->nbr;
}

void ndp_dst_cache_del(ip_addr *dst) {
    dep_cache_del(&neighbors, (char *) dst);
    dep_queue_enqueue(dst_free_entries,
                  (uint64_t) container_of(dst, dep_entry_t, data));
}

void ndp_pfx_cache_add(prefix_t *prefix) {
    // TODO: Do something if we can't queue_dequeue a free buffer
    dep_entry_t *entry = (dep_entry_t *) dep_queue_dequeue(pfx_free_entries);
    assert(prefix != NULL);
    assert(entry != NULL);
    assert(entry->data != (void *) (entry + sizeof(dep_entry_t)));

    // TODO: Can we avoid this copy somehow?
    memcpy(entry->data, prefix, sizeof(prefix_t));
    entry->length   = sizeof(prefix_t);
    entry->key      = ((prefix_t *) entry->data)->prefix;
    entry->node.key = entry->key;

    dep_cache_codes_t sts = dep_cache_add(&prefixes, entry);

    if (sts == EXISTS) {
        dep_cache_del(&prefixes, (char *) &prefix->prefix);
        sts = dep_cache_add(&prefixes, entry);
        assert(sts == SUCCESS);
    }
}

prefix_t *ndp_pfx_cache_get(prefix_t *prefix) {
    dep_entry_t *entry = dep_cache_get(&prefixes, (char *) prefix->prefix);
    if (!entry)
        return NULL;
    else
        return (prefix_t *) entry->data;
}

void ndp_pfx_cache_del(prefix_t *prefix) {
    dep_cache_del(&prefixes, (char *) prefix->prefix);
    dep_queue_enqueue(pfx_free_entries,
                  (uint64_t) container_of(prefix, dep_entry_t, data));
}

#if 0
bool ndp_validate_nbr_solicit(struct sk_buf *skb, icmp_hdr_t *hd) {
    // MUST silently discard if any of the following fail:
    // Code must be 0
    if (icmp_hdr_nbr_solicit_ptr_get_code(hd) != 0) return false;

    // Hop limit must be 255
    if (*(uint8 *) skb->first - 29 != 255) return false;

    // ICMP length must be 24 or more bytes
    if (skb->last - skb->first < 24) return false;

    // Target address is not multicast
    ip_addr *tgt_addr = skb->first + 64; // TODO: fix hardcoding
    if (ip_has_prefix(tgt_addr, &multicast)) return false;

    // If the IP source is the unspecified address, the IP destination
    // address must be a solicited-node multicast address
    //
    // TODO:
    //   There also must NOT be a source link-layer address option
    //   present
    if (ip_has_prefix(&skb->src->ip_addr, &unspecified))
        if (!ip_has_prefix(&skb->src->ip_addr, &slc_node_multicast))
            return false;

    return true;
}

bool ndp_validate_nbr_advert(struct sk_buf *skb, icmp_hdr_t *hd) {
    // MUST silently discard if any of the following fail:
    // Code must be 0
    if (icmp_hdr_nbr_advert_ptr_get_code(hd) != 0) return false;

    // Hop limit must be 255
    if (*(uint8 *) skb->first - 29 != 255) return false;

    // ICMP length must be 24 or more bytes
    if (skb->last - skb->first < 24) return false;

    // Target address is not multicast
    // Target address is not multicast
    ip_addr *tgt_addr = skb->first + 64; // TODO: fix hardcoding
    if (ip_has_prefix(tgt_addr, &multicast)) return false;

    // If the IP destination is multicast, the solicated flag must be
    // zero
    if (ip_has_prefix(&skb->dst->ip_addr, &multicast))
        if (icmp_hdr_nbr_advert_ptr_get_s(hd) != 0) return false;

    return true;
}

bool ndp_validate_rtr_advert(struct sk_buf *skb, icmp_hdr_t *hd) {
    // MUST silently discard if any of the following fail:
    // Code must be 0
    if (icmp_hdr_rtr_advert_ptr_get_code(hd) != 0) return false;

    // Hop limit must be 255
    if (*(uint8 *) skb->first - 29 != 255) return false;

    // ICMP length must be 15 or fewer bytes
    if (skb->last - skb->first > 15) return false;

    // IP src address must be link-local unicast
    if (!ip_has_prefix(&skb->src->ip_addr, &ll_unicast)) return false;

    return true;
}

bool ndp_validate_rtr_redirect(struct sk_buf *skb, icmp_hdr_t *hd) {
    // MUST silently discard if any of the following fail:
    // Code must be 0
    if (icmp_hdr_redirect_ptr_get_code(hd) != 0) return false;

    // Hop limit must be 255
    if (*(uint8 *) skb->first - 29 != 255) return false;

    // ICMP length must be 40 or more bytes
    if (skb->last - skb->first < 40) return false;

    // IP src address must be link-local unicast
    if (!ip_has_prefix(&skb->src->ip_addr, &ll_unicast)) return false;

    ip_addr *tgt_addr = skb->first + sizeof(icmp_hdr_t);
    ip_addr *dst_addr = tgt_addr + sizeof(ip_addr);

    // ICMP destination address MUST NOT be multicast
    if (ip_has_prefix(dst_addr, &multicast)) return false;

    // IP src must be the same as the current first hop rtr for the destination
    // address
    ip_addr *cur_fst_hop;
    dep_entry_t *entry = dep_cache_get(&destinations, (char *) skb->src->ip_addr);
    if (entry) {
        cur_fst_hop = &((dest_map_t *) entry->data)->dst;

        if (memcmp(skb->src->ip_addr, cur_fst_hop, sizeof(ip_addr)))
            return false;
    }

    // ICMP tgt address must be link-local
    if (!ip_has_prefix(dst_addr, &ll_unicast))
        // If it's not, then it must be the same as the tgt address
        if (!memcmp(dst_addr, tgt_addr, sizeof(ip_addr))) return false;

    return true;
}
#endif
