#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rbtree.h"

struct rb_str {
    const char* str;
    struct rb_node node;
};

static int rb_strcmp(struct rb_node* a, struct rb_node* b)
{
    struct rb_str* sa = rb_entry(a, struct rb_str, node);
    struct rb_str* sb = rb_entry(b, struct rb_str, node);

    return strcmp(sa->str, sb->str);
}

static const char* rb_content(struct rb_node* node)
{
    static char buf[1024];
    struct rb_str* rs = rb_entry(node, struct rb_str, node);

    sprintf(buf, "%s", rs->str);

    return buf;
}

int main(void)
{
    int i;
    struct rb_root rbtree;
    const char *str[] = {"m", "e", "h", "p", "t", "x", "b", "d", "f", "g",
        "i", "k", "l", "n", "o", "q", "s", "v", "w", "y", "z", "a", "c",
        "j", "r", "u", NULL};

    rb_init(&rbtree);
    for (i = 0; str[i]; ++i) {
        struct rb_str* rs = (struct rb_str*)malloc(sizeof(struct rb_str));
        rs->str = str[i];
        rb_insert(&rs->node, &rbtree, rb_strcmp);
        printf("------------------------------------------\n");
        rb_print(&rbtree, rb_content);
    }

    return 0;
}
