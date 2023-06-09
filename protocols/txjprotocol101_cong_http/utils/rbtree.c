#include <stdio.h>

#include "rbtree.h"

#define RB_RED    0
#define RB_BLACK  1

#define rb_color(node)              ((node)->parent_color & 1)
#define rb_is_red(node)             (!rb_color(node))
#define rb_is_black(node)           rb_color(node)
#define rb_set_red(node)            ((node)->parent_color &= ~1)
#define rb_set_black(node)          ((node)->parent_color |= 1)

static inline void rb_set_color(struct rb_node* node, int color)
{
    node->parent_color = (node->parent_color & ~1) | color;
}

#define rb_parent(node) ((struct rb_node*)((node)->parent_color & ~3))

static inline void rb_set_parent(struct rb_node* node, struct rb_node* parent)
{
    node->parent_color = (node->parent_color & 3) | (unsigned long)parent;
}

/*
 *     x              y
 *    / \            / \
 *  xl   y    =>    x   yr
 *      / \        / \
 *    yl   yr    xl   yl
 */

static void rb_rotate_left(struct rb_node* x, struct rb_root* root)
{
    struct rb_node *y = x->right, *px = rb_parent(x);

    x->right = y->left;
    rb_set_parent(y, px);
    if (y->left)
        rb_set_parent(y->left, x);

    if (px) {
        if (x == px->left)
            px->left = y;
        else
            px->right = y;
    } else
        root->node = y;

    y->left = x;
    rb_set_parent(x, y);
}

/*
 *       x            y
 *      / \          / \
 *     y   xr  =>  yl   x
 *    / \              / \
 *  yl   yr          yr   xr
 */

static void rb_rotate_right(struct rb_node* x, struct rb_root* root)
{
    struct rb_node *y = x->left, *px = rb_parent(x);

    x->left = y->right;
    rb_set_parent(y, px);
    if (y->right)
        rb_set_parent(y->right, x);

    if (px) {
        if (x == px->left)
            px->left = y;
        else
            px->right = y;
    } else
        root->node = y;

    y->right = x;
    rb_set_parent(x, y);
}

void rb_insert_rebalance(struct rb_node* node, struct rb_root* root)
{
    struct rb_node *parent, *gparent, *uncle;

    while ((parent = rb_parent(node)) && rb_is_red(parent)) {

        gparent = rb_parent(parent);

        if (parent == gparent->left) {
            uncle = gparent->right;
            if (uncle && rb_is_red(uncle)) {
                rb_set_black(uncle);
                rb_set_black(parent);
                rb_set_red(gparent);
                node = gparent;

                continue;
            }

            if (node == parent->right) {
                register struct rb_node* tmp;

                rb_rotate_left(parent, root);
                tmp = node;
                node = parent;
                parent = tmp;
            }

            rb_rotate_right(gparent, root);
            rb_set_black(parent);
            rb_set_red(gparent);

            continue;
        }

        uncle = gparent->left;
        if (uncle && rb_is_red(uncle)) {
            rb_set_black(parent);
            rb_set_black(uncle);
            rb_set_red(gparent);
            node = gparent;

            continue;
        }

        if (node == parent->left) {
            register struct rb_node* tmp;

            rb_rotate_right(parent, root);
            tmp = node;
            node = parent;
            parent = tmp;
        }

        rb_rotate_left(gparent, root);
        rb_set_red(gparent);
        rb_set_black(parent);
    }

    rb_set_black(root->node);
}

struct rb_node* rb_insert(struct rb_node* node, struct rb_root* root,
                          int (*cmp_func)(struct rb_node*, struct rb_node*))
{
    struct rb_node *parent = NULL, **cursor = &root->node;

    while (*cursor) {
        int diff;

        parent = *cursor;
        diff = cmp_func(node, parent);
        if (diff < 0)
            cursor = &parent->left;
        else if (diff > 0)
            cursor = &parent->right;
        else
            return parent;
    }

    rb_link_node(node, parent, cursor);
    rb_insert_rebalance(node, root);

    return node;
}

static inline void rb_delete_rebalance(struct rb_node* node,
                                       struct rb_node* parent,
                                       struct rb_root* root)
{
    struct rb_node* sibling;

    while ((!node || rb_is_black(node)) && (node != root->node)) {
        if (node == parent->left) {
            sibling = parent->right;

            if (rb_is_red(sibling)) {
                rb_rotate_left(parent, root);
                rb_set_red(parent);
                rb_set_black(sibling);
                sibling = parent->right;
            }

            if ((!sibling->left || rb_is_black(sibling->left)) &&
                (!sibling->right || rb_is_black(sibling->right))) {
                rb_set_red(sibling);
                node = parent;
                /* if (node) */ parent = rb_parent(node);
            } else {
                if (!sibling->right || rb_is_black(sibling->right)) {
                    rb_set_black(sibling->left);
                    rb_set_red(sibling);
                    rb_rotate_right(sibling, root);
                    sibling = parent->right;
                }

                rb_set_color(sibling, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(sibling->right);
                rb_rotate_left(parent, root);
                node = root->node;

                break;
            }
        } else {
            sibling = parent->left;

            if (rb_is_red(sibling)) {
                rb_rotate_right(parent, root);
                rb_set_red(parent);
                rb_set_black(sibling);
                sibling = parent->left;
            }

            if ((!sibling->left || rb_is_black(sibling->left)) &&
                (!sibling->right || rb_is_black(sibling->right))) {
                rb_set_red(sibling);
                node = parent;
                /* if (node) */ parent = rb_parent(node);
            } else {
                if (!sibling->left || rb_is_black(sibling->left)) {
                    rb_set_black(sibling->right);
                    rb_set_red(sibling);
                    rb_rotate_left(sibling, root);
                    sibling = parent->left;
                }

                rb_set_color(sibling, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(sibling->left);
                rb_rotate_right(parent, root);
                node = root->node;

                break;
            }
        }
    }

    if (node)
        rb_set_black(node);
}

void rb_delete(struct rb_node* node, struct rb_root* root)
{
    int color;
    struct rb_node *parent, *child = NULL;

    if (node->left && node->right) {
        struct rb_node* old = node;
        struct rb_node* old_parent = rb_parent(node);

        node = node->right;
        while (node->left)
            node = node->left;

        if (old_parent) {
            if (old == old_parent->left)
                old_parent->left = node;
            else
                old_parent->right = node;
        } else
            root->node = node;

        child = node->right;
        parent = rb_parent(node);
        color = rb_color(node);

        if (parent == old)
            parent = node;
        else {
            if (child)
                rb_set_parent(child, parent);
            parent->left = child;

            node->right = old->right;
            rb_set_parent(old->right, node);
        }

        node->parent_color = old->parent_color;
        node->left = old->left;
        rb_set_parent(old->left, node);

        goto rebalance;
    }

    if (!node->left)
        child = node->right;
    else
        child = node->left;

    parent = rb_parent(node);
    color = rb_color(node);

    if (child)
        rb_set_parent(child, parent);

    if (parent) {
        if (node == parent->left)
            parent->left = child;
        else
            parent->right = child;
    } else
        root->node = child;

rebalance:
    if (color == RB_BLACK)
        rb_delete_rebalance(child, parent, root);
}

struct rb_node* rb_root(struct rb_node* node)
{
    struct rb_node* parent;

    while ((parent = rb_parent(node)))
        node = parent;

    return node;
}

struct rb_node* rb_first(struct rb_root* root)
{
    struct rb_node* n;

    n = root->node;
    if (!n)
        return NULL;

    while (n->left)
        n = n->left;

    return n;
}

struct rb_node* rb_last(struct rb_root* root)
{
    struct rb_node* n;

    n = root->node;
    if (!n)
        return NULL;

    while (n->right)
        n = n->right;

    return n;
}

struct rb_node* rb_next(struct rb_node* node)
{
    struct rb_node* parent;

    if (rb_parent(node) == node)
        return NULL;

    if (node->right) {
        node = node->right;
        while (node->left)
            node = node->left;
        return node;
    }

    while ((parent = rb_parent(node)) && (node == parent->right))
        node = parent;

    return parent;
}

struct rb_node* rb_prev(struct rb_node* node)
{
    struct rb_node* parent;

    if (rb_parent(node) == node)
        return NULL;

    if (node->left) {
        node = node->left;
        while (node->right)
            node = node->right;
        return node;
    }

    while ((parent = rb_parent(node)) && (node == parent->left))
        node = parent;

    return parent;
}

static inline void rb_null_del(struct rb_node* node) {}

void rb_destroy(struct rb_root* root, void (*del_func)(struct rb_node*))
{
    struct rb_node* node = root->node;

    if (!del_func)
        del_func = rb_null_del;

    while (node) {
        if (node->left)
            node = node->left;
        else if (node->right)
            node = node->right;
        else {
            struct rb_node* tmp = node;

            node = rb_parent(node);
            if (node) {
                if (tmp == node->left)
                    node->left = NULL;
                else
                    node->right = NULL;
            }

            del_func(tmp);
        }
    }

    root->node = NULL;
}

/*---------------------------------------------------------------------------*/

static void __rb_print(struct rb_node *root, int space, int pos,
                       const char* (*content)(struct rb_node*))
{
    if (root) {
        int i;
        for (i = 0; i < space; ++i)
            printf(" ");
        printf("%s\t%d\t%s\n", content(root), pos,
               rb_is_black(root) ? "BLACK" : "RED");
        space += 3;
        __rb_print(root->left, space, 1, content);
        __rb_print(root->right, space, -1, content);
    }
}

void rb_print(struct rb_root* root, const char* (*content)(struct rb_node*))
{
    __rb_print(root->node, 0, 0, content);
}
