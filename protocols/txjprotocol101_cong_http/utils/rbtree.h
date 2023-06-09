#ifndef __RBTREE_H__
#define __RBTREE_H__

/* red black tree implementation from linux kernel */

#include "utils.h"

struct rb_node {
    unsigned long parent_color;
    struct rb_node *left, *right;
} __attribute__((aligned(sizeof(long))));

struct rb_root {
    struct rb_node* node;
};

#define DEF_RB_ROOT(r) struct rb_root r = {NULL}

#define rb_init(root) ((root)->node = NULL)
#define rb_empty(root) (!((root)->node))
#define rb_entry(ptr, type, member) container_of(ptr, type, member)

static inline void rb_link_node(struct rb_node* node, struct rb_node* parent,
                                struct rb_node** link)
{
    node->parent_color = (unsigned long)parent;
    node->left = node->right = NULL;
    *link = node;
}

void rb_insert_rebalance(struct rb_node* node, struct rb_root* root);
/* returns the newly inserted node if ok, or the existing one if fails */
struct rb_node* rb_insert(struct rb_node* node, struct rb_root* root,
                          int (*cmp_func)(struct rb_node*, struct rb_node*));
void rb_delete(struct rb_node* node, struct rb_root* root);
struct rb_node* rb_root(struct rb_node* node);
struct rb_node* rb_first(struct rb_root* root);
struct rb_node* rb_last(struct rb_root* root);
struct rb_node* rb_next(struct rb_node* node);
struct rb_node* rb_prev(struct rb_node* node);
void rb_destroy(struct rb_root* root, void (*del_func)(struct rb_node*));

void rb_print(struct rb_root*, const char* (*print)(struct rb_node*));

#endif
