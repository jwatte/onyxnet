#include "vector.h"
#include <string.h>


vector_t *vector_create(size_t item_size) {
    if (item_size == 0) {
        return NULL;
    }
    vector_t *ret = (vector_t *)malloc(sizeof(vector_t));
    if (!ret) {
        return NULL;
    }
    if (vector_init(ret, item_size) < 0) {
        free(ret);
        return NULL;
    }
    return ret;
}

void vector_destroy(vector_t *vec) {
    if (vec) {
        vector_deinit(vec);
        free(vec);
    }
}

int vector_init(vector_t *vec, size_t item_size) {
    memset(vec, 0, sizeof(*vec));
    vec->item_size = item_size;
    if (item_size == 0) {
        return -1;
    }
    return 0;
}

void vector_deinit(vector_t *vec) {
    if (vec) {
        free(vec->items);
    }
}

size_t vector_item_append(vector_t *vec, void const *item) {
    size_t n = vector_item_insert(vec, vec->item_count, item, 1);
    if (n == 0) {
        return 0;
    }
    return vec->item_count;
}

void *vector_item_get(vector_t *vec, size_t index) {
    if (index > vec->item_count) {
        return NULL;
    }
    return (char *)vec->items + index * vec->item_size;
}

size_t vector_item_remove(vector_t *vec, size_t index, size_t count) {
    if (index > vec->item_count) {
        return 0;
    }
    if (vec->item_count - index < count) {
        count = vec->item_count - index;
    }
    size_t to_move = vec->item_count - count - index;
    if (count > 0 && to_move > 0) {
        //  move some items
        memmove((char *)vec->items + index * vec->item_size, (char *)vec->items + (index + count) * vec->item_size, to_move * vec->item_size);
    }
    vec->item_count -= count;
    return count;
}

size_t vector_item_insert(vector_t *vec, size_t index, void const *items, size_t count) {
    if (index > vec->item_count) {
        return 0;
    }
    if (count == 0) {
        return 0;
    }
    if (VECTOR_MAX_COUNT - vec->item_count < count) {
        return 0;
    }
    size_t new_count = vec->item_count + count;
    if (vec->phys_size < new_count) {
        size_t q = new_count;
        q += (q >> 2) + 16;
        char *nu = (char *)malloc(q * vec->item_size);
        if (!nu) {
            return 0;
        }
        //  put together the new contents
        if (index != 0) {
            memmove(nu, vec->items, index * vec->item_size);
        }
        memmove(nu + index * vec->item_size, items, count * vec->item_size);
        if (index < vec->item_count) {
            memmove(nu + (index + count) * vec->item_size, (char *)vec->items + index * vec->item_size, (vec->item_count - index) * vec->item_size);
        }
        free(vec->items);
        vec->items = nu;
        vec->phys_size = q;
    } else {
        //  shuffle around items
        if (index < vec->item_count) {
            memmove((char *)vec->items + (index + count) * vec->item_size, (char *)vec->items + index * vec->item_size, (vec->item_count - index) * vec->item_size);
        }
        memmove((char *)vec->items + index * vec->item_size, items, count * vec->item_size);
    }
    vec->item_count += count;
    return count;
}



