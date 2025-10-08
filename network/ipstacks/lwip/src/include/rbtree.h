#pragma once
#include <stdbool.h>
#include <stddef.h>

/**
 * From the definition of Red-black trees:
 * - Every node is colored red or black.
 * - The root is black. NULL is black.
 * - Every red node has two black children.
 */
enum rb_color {
    RB_RED,
    RB_BLACK,
};

/**
 * A node within the tree.
 */
struct rbnode {
    struct rbnode *left;
    struct rbnode *right;
    const void *key; ///< Users MUST initialize a node's key before
                     ///< adding it to a tree
    enum rb_color color;
};

/**
 * Red-black tree data structure.
 *
 * {@code root}     Root node, may be initialized as NULL.
 * {@code compar}   Call that orders keys. It should behave like memcmp.
 */
struct rbtree {
    struct rbnode *root;
    int (*compar)(const void *, const void *);
};

static inline bool rb_isred(struct rbnode *node) {
    return node != NULL && node->color == RB_RED;
}

static inline bool rb_isblack(struct rbnode *node) {
    return node == NULL || node->color == RB_BLACK;
}

/***************************************************************************
 * Public APIs.
 * Note the caller is responsible for providing:
 *      - a wrapper struct to contain rbnodes,
 *      - allocation for nodes,
 *      - an rbtree with compar callback.
 ***************************************************************************/

/*
 * Adds a node to the tree if it does not exist already. Insertion will
 * silently fail if the key identifies a node already in the tree. In order to
 * perform update, the caller is responsible for calling {@code rb_get} first.
 *
 * Complexity: O(lg(n))
 *
 * @param tree  Pointer to an rbtree
 * @param node  Pointer to the node to add
 */
void rb_add(struct rbtree *tree, struct rbnode *node);

/*
 * Gets a pointer to a node from the tree if it exists.
 *
 * Complexity: O(lg(n))
 *
 * @param tree  Pointer to an rbtree
 * @param key  The key to search
 * @return a pointer to a node
 */
struct rbnode *rb_get(struct rbtree *tree, const void *key);

/*
 * Removes a node from the tree. It is the user's responsibility to ensure the
 * node exists in the tree before deletion.
 *
 * Complexity: O(lg(n))
 *
 * @param tree  Pointer to an rbtree
 * @param key  The key to search
 */
void rb_del(struct rbtree *tree, const void *key);
