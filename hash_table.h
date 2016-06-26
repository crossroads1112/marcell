#ifndef MSH_HASH_H
#define MSH_HASH_H

#include "msh_execute.h"

#define TABLE_INIT_SIZE 1024 
#define GROWTH_FACTOR 2


typedef struct node {
    void *value; // Pointer to builtin function (or alias)
    char const* key; // Name of function (or alias)
    struct node *next;
} node;

typedef struct hash_table {
    node **nodes; // Array of nodes
    size_t capacity; // Size of **nodes
    size_t size; // Number of key/value pairs in table
} hash_table;



int add_node(void *v, char const *k, hash_table *t);
void *find_node(char const *k, hash_table const *t);
void free_table(hash_table *t);

#endif
