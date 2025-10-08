#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "container.h"
#include "inet.h"
#include "ip_addr.h"

#include "dep_cache.h"

void dep_cache_init(dep_cache_t *cache, int (*compar)(const void *, const void *)) {
    cache->tree.root   = NULL;
    cache->tree.compar = compar;
    cache->size        = 0;
}

dep_cache_codes_t dep_cache_add(dep_cache_t *cache, dep_entry_t *entry) {
    struct rbnode *node = rb_get(&cache->tree, entry->key);
    if (node != NULL) return EXISTS;
    rb_add(&cache->tree, &entry->node);
    cache->size++;
    return SUCCESS;
}

dep_entry_t *dep_cache_get(dep_cache_t *cache, void *key) {
    struct rbnode *node = rb_get(&cache->tree, key);
    if (node == NULL) return NULL;
    return container_of(node, dep_entry_t, node);
}

dep_cache_codes_t dep_cache_del(dep_cache_t *cache, void *key) {
    struct rbnode *node = rb_get(&cache->tree, key);
    if (node == NULL) return FAILURE;
    rb_del(&cache->tree, key);
    cache->size--;
    return SUCCESS;
}
