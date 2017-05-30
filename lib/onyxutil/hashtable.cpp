
#include "hashtable.h"
#include <string.h>


size_t hash_pod(void const *data, size_t size) {
    size_t ret = 1;
    unsigned char *p = (unsigned char *)data;
    unsigned char *q = p + size;
    while (p != q) {
        /* some randomly chosen primes */
        ret = ret * 22511 + 33469 + *p;
        p++;
    }
    return ret;
}

hash_table_t *hash_table_init(hash_table_t *table, size_t item_size, uint32_t flags) {
    memset(table, 0, sizeof(*table));
    table->item_size = item_size;
    table->flags = flags;
    table->hash_func = hash_pod;
    table->comp_func = memcmp;
    return table;
}

void hash_table_deinit(hash_table_t *table) {
    for (size_t i = 0; i != table->top_size; ++i) {
        for (hash_node_t *q, *n = table->top[i]; n; n = q) {
            q = n->next;
            free(n);
        }
    }
    memset(table, 0, sizeof(*table));
}

static void maybe_rehash(hash_table_t *table) {

}


void *hash_table_find(hash_table_t *table, void *key) {
    if (table->top == NULL) {
        return NULL;
    }
    size_t code = table->hash_func(key, table->item_size);
    size_t bucket = code & (table->topsize - 1);
    node *n = table->top[bucket];
    while (n) {
        if (n->code == code) {
            void *nodekey = (void *)(n + 1);
            if (table->flags & HASHTABLE_POINTERS) {
                nodekey = *(void **)nodekey;
            }
            if (table->comp_func(key, nodekey, table->item_size) == 0) {
                return nodekey;
            }
        }
        n = n->next;
    }
    return NULL;
}

void *hash_table_assign(hash_table_t *table, void *key) {
    if (table->top == NULL) {
        table->top = (void **)malloc(sizeof(void *) * 32);
        if (!table->top) {
            return NULL;
        }
        table->top_size = 32;
        memset(table->top, 0, sizeof(void *) * table->top_size);
    }

    //  Find the object if it exists
    size_t code = table->hash_func(key, table->item_size);
    size_t slot = code & (table->top_size - 1);
    for (node *n = table->top[slot]; n; n = n->next) {
        if (n->code == code) {
            void *nodekey = n + 1;
            if (table->flags & HASHTABLE_POINTERS) {
                nodekey = *(void **)nodekey;
            }
            if (table->comp_func(key, nodekey, table->item_size) == 0) {
                if (table->flags & HASHTABLE_POINTERS) {
                    *(void **)(n + 1) = key;
                } else {
                    memcpy(n + 1, key, table->item_size);
                }
                return nodekey;
            }
        }
    }

    //  OK, so I need to assign
    hash_node_t *n = (hash_node_t *)malloc(sizeof(hash_node_t) + 
            ((table->flags & HASHTABLE_POINTERS) ? sizeof(void *) : table->item_size));
    if (!n) {
        return NULL;
    }
    if (table->flags & HASHTABLE_POINTERS) {
        *(void **)(n + 1) = key;
    } else {
        memcpy(n + 1, key, table->item_size);
    }
    n->next = table->top[slot];
    table->top[slot] = n;
    table->item_count++;
    maybe_rehash(table);
    return (table->flags & HASHTABLE_POINTERS) ? key : (n + 1);
}

int hash_table_remove(hash_table_t *table, void *key) {
    if (table->num_items == 0) {
        return 0;
    }
    size_t code = table->hash_func(key, table->item_size);
    size_t slot = code & (table->top_size - 1);
    for (node **npp = &table->top[slot]; *npp; npp = &npp->next) {
        if ((*npp)->code == code) {
            void *nodekey = (*npp) + 1;
            if (table->flags & HASHTABLE_POINTERS) {
                nodekey = *(void **)nodekey;
            }
            if (table->comp_func(key, nodekey, table->item_size) == 0) {
                void *d = *npp;
                *npp = (*npp)->next;
                free(d);
                table->item_count--;
                maybe_rehash(table);
                return 1;
            }
        }
    }
    return 0;
}

void *hash_table_begin(hash_table_t *table, hash_iterator_t *iter) {
    iter->table = table;
    iter->node = NULL;
    for (size_t i = 0; i != table->top_size; ++i) {
        if (table->top[i] != NULL) {
            iter->node = table->top[i];
            void *ret = iter->node + 1;
            if (table->flags & HASHTABLE_POINTERS) {
                ret = *(void **)ret;
            }
            return ret;
        }
    }
    return NULL;
}

void *hash_table_next(hash_iterator_t *iter) {
    if (iter->node == NULL) {
        return NULL;
    }
    size_t slot = iter->node->code & (iter->table->top_size - 1);
    iter->node = iter->node->next;
    if (!iter->node) {
        while (slot != (iter->table->top_size - 1)) {
            ++slot;
            iter->node = table->top[slot];
            if (iter->node != NULL) {
                break;
            }
        }
    }
    if (!iter->node) {
        return NULL;
    }
    void *ret = iter->node + 1;
    if (iter->table->flags & HASHTABLE_POINTERS) {
        ret = *(void **)ret;
    }
    return ret;
}



