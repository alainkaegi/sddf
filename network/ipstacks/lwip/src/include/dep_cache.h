#pragma once

#include <stddef.h>

#include "rbtree.h"

typedef enum {
    SUCCESS,
    FAILURE,
    EXISTS,
} dep_cache_codes_t;

/**
 * A tree based cache. Users of this API are expected to
 * allocate their memory for instances of this type.
 */
typedef struct {
    struct rbtree tree;
    size_t size;
} dep_cache_t;

/**
 * A generic entry in the cache. Users of this API are responsible for
 * allocating memory for entries. The function {@code cache_del} should be
 * called to remove entries.
 *
 * Fields are aligned on 8 byte boundaries to improve access.
 *
 * {@code key}        Same as {@code struct rbnode->key}
 * {@code node}       The {@code struct rbnode} used by {@code rbtree.h} to
 *                    maintain the tree structure
 * {@code length}     The length in bytes of @{code data}
 * {@code data}       Stores the start of the array holding the content of the
 *                    entry. Starting at that location, there should be enough
 *                    memory to hold {@code length} times 8 bytes. One possible
 *                    way to allocate this memory is to invoke {@code malloc} as
 *                    follows:
 *
 *                    {@code malloc(sizeof(entry_t) + length*8)}
 */
typedef struct {
    void *key;
    struct rbnode node;
    uint64_t length;
    uint64_t data[0];
} dep_entry_t;

/**
 * Initializes a cache.
 *
 * @param cache  A pointer to a cache
 * @param compar  A function that orders cache entries, should behave like
 * memcmp
 */
void dep_cache_init(dep_cache_t *cache, int (*compar)(const void *, const void *));

/**
 * Puts an entry into the cache if it does not already exist. If there is an
 * existing entry with the same key, a {@code cache_codes_t EXISTS} type is
 * returned.
 *
 * @param cache  A pointer to a cache
 * @param entry  A pointer to the cache entry to attempt to add
 * @return status as a {@code cache_codes_t} type
 */
dep_cache_codes_t dep_cache_add(dep_cache_t *cache, dep_entry_t *entry);

/**
 * Returns the entry associated with the given key, if the key exists in the
 * cache.
 *
 * @param cache  A pointer to a cache to search
 * @param key  The key to search for
 * @return a pointer to the associated entry or NULL
 */
dep_entry_t *dep_cache_get(dep_cache_t *cache, void *key);

/**
 * Removes an entry from the cache if it exists.
 *
 * @param cache  A pointer to the cache to search
 * @param key  The key of the entry that should be removed
 * @return SUCCESS if the entry was removed, FAILURE otherwise
 */
dep_cache_codes_t dep_cache_del(dep_cache_t *cache, void *key);
