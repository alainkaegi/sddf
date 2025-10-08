#pragma once

#include <stddef.h>

/**
 * Given a pointer to a node, return a pointer to the POD structure
 * embedding that node.
 *
 * The node must be one of the field of the embedding POD structure.
 * The node must declared of type {@code struct list_node} and not as
 * a pointer to that type.
 *
 * @param n  a pointer to a {@code struct list_node}
 * @param type  the name of the type of the POD structure embedding the node
 * @param field  the name of the field in the embedding POD structure; that
 * field must be of type {@code struct list_node}
 */
// TBD: UPPERCASE?
#define container_of(n, type, field) \
    ((type *) ((void *) (n) -offsetof(type, field)))
