/**
 * An implementation of the red-black tree data structure.
 * Adapted from Functional Data Structures [Nipkow 2021] sections 20-21.
 * Insert, delete and lookup are all O(lg(n))
 * for n the number of nodes in the tree.
 */
//#include "assert.h"
#define assert(expr)

#include "rbtree.h"

static struct rbnode *rot_right(struct rbnode *node) {
    struct rbnode *nl = node->left;
    node->left        = nl->right;
    nl->right         = node;
    return nl;
}

static struct rbnode *rot_left(struct rbnode *node) {
    struct rbnode *nr = node->right;
    node->right       = nr->left;
    nr->left          = node;
    return nr;
}

static struct rbnode *baliL(struct rbnode *node) {
    struct rbnode *a, *b, *c = node;
    if (rb_isred(node->left) && rb_isred(node->left->left)) {
        /*
         * baliL (R (R t1 a t2) b t3) c t4
         *      = R (B t1 a t2) b (B t3 c t4)
         */
        b        = rot_right(c);
        a        = b->left;
        c        = b->right;
        b->color = RB_RED;
        a->color = RB_BLACK;
        c->color = RB_BLACK;
        return b;
    } else if (rb_isred(node->left) && rb_isred(node->left->right)) {
        /*
         * baliL (R t1 a (R t2 b t3)) c t4
         *      = R (B t1 a t2) b (B t3 c t4)
         */
        a        = c->left;
        c->left  = rot_left(a);
        b        = rot_right(c);
        a->color = RB_BLACK;
        b->color = RB_RED;
        c->color = RB_BLACK;
        return b;
    }
    /*
     * baliL t1 a t2 = B t1 a t2
     */
    if (node != NULL) node->color = RB_BLACK;
    return node;
}

static struct rbnode *baliR(struct rbnode *node) {
    struct rbnode *a = node, *b, *c;
    if (rb_isred(node->right) && rb_isred(node->right->right)) {
        /*
         * baliR t1 a (R t2 b (R t3 c t4))
         *     = R (B t1 a t2) b (B t3 c t4)
         */
        b        = rot_left(a);
        c        = b->right;
        a->color = RB_BLACK;
        b->color = RB_RED;
        c->color = RB_BLACK;
        return b;
    } else if (rb_isred(node->right) && rb_isred(node->right->left)) {
        /*
         * baliR t1 a (R (R t2 b t3) c t4)
         *      = R (B t1 a t2) b (B t3 c t4)
         */
        c        = a->right;
        a->right = rot_right(c);
        b        = rot_left(a);
        a->color = RB_BLACK;
        b->color = RB_RED;
        c->color = RB_BLACK;
        return b;
    }
    /*
     * baliL t1 a t2 = B t1 a t2
     */
    if (node != NULL) node->color = RB_BLACK;
    return node;
}

static struct rbnode *paint(struct rbnode *node, enum rb_color color) {
    if (node != NULL) {
        node->color = color;
    }
    return node;
}

static struct rbnode *baldL(struct rbnode *node) {
    if (rb_isred(node->left)) {
        node->left->color = RB_BLACK;
        node->color       = RB_RED;
        return node;
    } else if (rb_isblack(node->right)) {
        node->right->color = RB_RED;
        return baliR(node);
    } else if (rb_isred(node->right) && rb_isblack(node->right->left)) {
        /*
         * baldL t1 a (R (B t2 b t3) c t4)
         *      = R (B t1 a t2) b (baliR t3 c (paint Red t4))
         */
        struct rbnode *a = node, *b, *c;
        a->right         = rot_right(a->right);
        b                = rot_left(a);
        c                = b->right;
        a->color         = RB_BLACK;
        b->color         = RB_RED;
        c->right         = paint(c->right, RB_RED);
        b->right         = baliR(c);
        return b;
    }
    /*
     * baldL t1 a t2 = R t1 a t2
     */
    assert(node != NULL);
    node->color = RB_RED;
    return node;
}

static struct rbnode *baldR(struct rbnode *node) {
    if (rb_isred(node->right)) {
        node->right->color = RB_BLACK;
        node->color        = RB_RED;
        return node;
    } else if (rb_isblack(node->left)) {
        node->left->color = RB_RED;
        return baliL(node);
    } else if (rb_isred(node->left) && rb_isblack(node->left->right)) {
        /*
         * baldR (R t1 a (B t2 b t3)) c t4
         *      = R (baliL (paint Red t1) a t2) b (B t3 c t4)
         */
        struct rbnode *a, *b, *c = node;
        c->left  = rot_left(c->left);
        b        = rot_right(c);
        a        = b->left;
        a->left  = paint(a->left, RB_RED);
        b->left  = baliL(a);
        b->color = RB_RED;
        c->color = RB_BLACK;
        return b;
    }
    assert(node != NULL);
    node->color = RB_RED;
    return node;
}

static struct rbnode *join(struct rbnode *l, struct rbnode *r) {
    struct rbnode *a, *b, *c, *t1, *t2, *t3;
    /*
     * join Leaf t = t | join t Leaf = t
     */
    if (l == NULL) {
        return r;
    } else if (r == NULL) {
        return l;
    } else if (rb_isred(l) && rb_isred(r)) {
        /*
         * join (R t1 a t2) (R t3 c t4) =
         *   (case join t2 t3 of
         *     R u2 b u3 => (R (R t1 a u2) b (R u3 c t4)) |
         *     t23 => R t1 a (R t23 c t4))
         */
        a  = l;
        c  = r;
        t1 = a->left;
        t2 = a->right;
        t3 = c->left;
        b  = join(t2, t3);
        if (rb_isred(b)) {
            struct rbnode *u2 = b->left, *u3 = b->right;
            a->right = u2;
            c->left  = u3;
            b->left  = a;
            b->right = c;
            return b;
        }
        c->left  = b;
        a->right = c;
        return a;
    } else if (rb_isblack(l) && rb_isblack(r)) {
        /*
         * join (B t1 a t2) (B t3 c t4) =
         *  (case join t2 t3 of
         *    R u2 b u3 => R (B t1 a u2) b (B u3 c t4) |
         *    t23 => baldL t1 a (B t23 c t4))
         */
        a  = l;
        c  = r;
        t1 = a->left;
        t2 = a->right;
        t3 = c->left;
        b  = join(t2, t3);
        if (rb_isred(b)) {
            struct rbnode *u2 = b->left, *u3 = b->right;
            a->right = u2;
            c->left  = u3;
            b->left  = a;
            b->right = c;
            return b;
        }
        c->left  = b;
        a->right = c;
        return baldL(a);
    } else if (rb_isred(r)) {
        t1      = l;
        a       = r;
        t2      = a->left;
        a->left = join(t1, t2);
        return a;
    }
    assert(rb_isred(l));
    /*
     * join (R t1 a t2) t3 = R t1 a (join t2 t3)
     */
    a        = l;
    t3       = r;
    t2       = a->right;
    a->right = join(t2, t3);
    return a;
}

static struct rbnode *ins(struct rbtree *t,
                          struct rbnode *node,
                          const void *key,
                          struct rbnode *buf) {
    if (node == NULL) {
        buf->right = buf->left = NULL;
        buf->key               = key;
        buf->color             = RB_RED;
        return buf;
    }
    int cmp = t->compar(key, node->key);
    if (rb_isblack(node)) {
        if (cmp < 0) {
            node->left = ins(t, node->left, key, buf);
            return baliL(node);
        } else if (cmp > 0) {
            node->right = ins(t, node->right, key, buf);
            return baliR(node);
        }
        /* key already in tree */
        return node;
    }
    assert(rb_isred(node));
    if (cmp < 0) {
        node->left = ins(t, node->left, key, buf);
    } else if (cmp > 0) {
        node->right = ins(t, node->right, key, buf);
    } else {
        /* key already in tree */
    }
    node->color = RB_RED;
    return node;
}

static struct rbnode *
del(struct rbtree *t, struct rbnode *node, const void *key) {
    struct rbnode *a, *l, *r;
    if (node == NULL) {
        return NULL;
    }
    int cmp = t->compar(key, node->key);
    if (cmp < 0) {
        /*
         * if l != Leaf && color l == Black
         * then baldL (del x l) a r else R (del x l) a r
         */
        a = node;
        l = node->left;
        r = node->right;
        if (l != NULL && rb_isblack(l)) {
            a->left = del(t, l, key);
            return baldL(a);
        }
        a->left  = del(t, l, key);
        a->color = RB_RED;
        return a;
    } else if (cmp > 0) {
        /*
         * if r != Leaf && color r = Black
         * then baldR l a (del x r) else R l a (del x r)
         */
        a = node;
        l = node->left;
        r = node->right;
        if (r != NULL && rb_isblack(r)) {
            a->right = del(t, r, key);
            return baldR(a);
        }
        a->right = del(t, r, key);
        a->color = RB_RED;
        return a;
    }
    struct rbnode *new = join(node->left, node->right);
    return new;
}

// ----------------
// PUBLIC FUNCTIONS
// ----------------
void rb_add(struct rbtree *tree, struct rbnode *node) {
    struct rbnode *res = ins(tree, tree->root, node->key, node);
    tree->root         = paint(res, RB_BLACK);
}

struct rbnode *rb_get(struct rbtree *t, const void *key) {
    int cmp;
    struct rbnode *node = t->root;
    while (node != NULL) {
        cmp = t->compar(key, node->key);
        if (cmp < 0)
            node = node->left;
        else if (cmp > 0)
            node = node->right;
        else
            break;
    }
    return node;
}

void rb_del(struct rbtree *tree, const void *key) {
    assert(tree && tree->compar && key);
    struct rbnode *res = del(tree, tree->root, key);
    tree->root         = paint(res, RB_BLACK);
}
