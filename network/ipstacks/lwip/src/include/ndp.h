#pragma once

// Requires:
// - stdbool.h
// - stdint.h
// - ip_addr.h
// - mac_addr.h
// - dep_queue.h

#define MAX_RTR_SOLICT_DELAY   1     ///< seconds
#define RTR_SOLICT_INTERVAL    4     ///< seconds
#define MAX_RTR_SOLICTS        3     ///< transmissions
#define MAX_MULTICAST_SOLICT   3     ///< transmissions
#define MAX_UNICAST_SOLICT     3     ///< transmissions
#define MAX_ANYCAST_DELAY_TIME 1     ///< seconds
#define MAX_NBR_ADVERTS        3     ///< transmissions
#define REACHABLE_TIME         30000 ///< milliseconds
#define RETRANS_TIMER          1000  ///< milliseconds
#define DELAY_FIRST_PROBE_TIME 5     ///< seconds
#define MIN_RAND_FACTOR        0.5
#define MAX_RAND_FACTOR        1.5

#define ENTRIES_PER_CACHE 100

typedef enum {
    INCOMPLETE,
    REACHABLE,
    STALE,
    DELAY,
    PROBE,
} nbr_cache_state_t;

typedef struct {
    ip_addr addr; ///< The on-link unicast IP address
    uint64_t resv;
    mac_addr link_addr; ///< The node's link address
    struct dep_queue *q;///< Packets to this node that are waiting for address
                        ///  resolution to complete
    nbr_cache_state_t state;
    uint64_t is_router;
} __attribute__((packed)) __attribute__((aligned(8))) neighbor_t;

typedef struct {
    ip_addr dst; ///< The destination's IP address
    uint64_t resv;
    ip_addr nbr; ///< The next hop neighbor's address
} __attribute__((packed)) __attribute__((aligned(8))) dest_map_t;

static const prefix_t ll_all_nds_multicast = {
    .prefix = {0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001},
    .length = 128,
};

static const prefix_t ll_all_rtrs_multicast = {
    .prefix = {0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002},
    .length = 128,
};

static const prefix_t slc_node_multicast = {
    .prefix = {0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0xFF00, 0x0000},
    .length = 104,
};

/**
 * Initialize the internal caches.
 */
void ndp_init_caches();

/**
 * Add a neighbor entry to the neighbors cache. If a matching entry is found,
 * the existing entry is removed and the the given entry is added.
 *
 * @param nbr  Pointer to a neighbor_t
 */
void ndp_nbr_cache_add(neighbor_t *nbr);

/**
 * Get a neighbor entry from the neighbors cache. If a matching entry is found,
 * a pointer to it is returned. If no matching entry is found, NULL is returned.
 *
 * @param nbr Pointer to the IP address to search for
 * @return a pointer to the matching neighbor_t entry or NULL
 */
neighbor_t *ndp_nbr_cache_get(ip_addr *nbr);

/**
 * Remove a neighbor entry from the neighbors cache. If a matching entry is
 * found, it is removed from the cache.
 *
 * @param nbr Pointer to the IP address to search for
 */
void ndp_nbr_cache_del(ip_addr *nbr);

/**
 * Add a destination map entry to the destinations cache. If a matching entry
 * is found, the existing entry is removed and the the given entry is added.
 *
 * @param dst  Pointer to a dest_map_t
 */
void ndp_dst_cache_add(dest_map_t *dst);

/**
 * Get a destination map entry from the destinations cache. If a matching entry
 * is found, a pointer to it is returned. If no matching entry is found, NULL
 * is returned.
 *
 * @param dst Pointer to the IP address to search for
 * @return a pointer to the matching dest_map_t entry or NULL
 */
ip_addr *ndp_dst_cache_get(ip_addr *dst);

/**
 * Remove a destination map entry from the destinations cache. If a matching
 * entry is found, it is removed from the cache.
 *
 * @param dst Pointer to the IP address to search for
 */
void ndp_dst_cache_del(ip_addr *dst);

/**
 * Add a prefix entry to the on-link prefix cache. If a matching entry is found,
 * the existing entry is removed and the the given entry is added.
 *
 * @param prefix  Pointer to a prefix_t
 */
void ndp_pfx_cache_add(prefix_t *prefix);

/**
 * Get a prefix entry from the on-link prefix cache. If a matching entry is
 * found, a pointer to it is returned. If no matching entry is found, NULL is
 * returned.
 *
 * @param prefix  Pointer to a matching prefix
 * @return a pointer to the matching entry or NULL
 */
prefix_t *ndp_pfx_cache_get(prefix_t *prefix);

/**
 * Remove a prefix entry from the on-link prefix cache. If a matching entry is
 * found, it is removed from the cache.
 *
 * @param prefix  Pointer to a matching prefix
 */
void ndp_pfx_cache_del(prefix_t *prefix);
