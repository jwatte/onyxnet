#include <onyxutil/vector.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>

struct item {
    int a;
    char b[12];
};

item *mkitem(item *i, int j) {
    i->a = j;
    sprintf(i->b, "%d", j);
    return i;
}

int main() {
    vector_t va = { 0 }, vb = { 0 };

    vector_init(&va, sizeof(item));
    vector_init(&vb, sizeof(item));
    assert(va.item_count == 0);
    assert(vb.item_count == 0);

    for (int j = 11; j < 19; ++j) {
        item i = { 0 };
        int n = vector_item_append(&va, mkitem(&i, j));
        assert(n == j - 10);
    }
    assert(va.item_count == 8);
    assert(vb.item_count == 0);

    for (int j = 0; j < 1000; ++j) {
        item i = { 0 };
        int n = vector_item_append(&vb, mkitem(&i, j));
        assert(n == j + 1);
    }

    void *p = vector_item_get(&va, 0);
    assert(((item *)p)->a == 11);
    assert(!strcmp(((item *)p)->b, "11"));
    vector_item_remove(&va, 0, 4);
    assert(va.item_count == 4);
    p = vector_item_get(&va, 0);
    assert(((item *)p)->a == 15);

    size_t s = vector_item_insert(&vb, 1, p, 4);
    assert(s == 4);
    assert(vb.item_count == 1004);
    p = vector_item_get(&vb, 1);
    assert(((item *)p)->a == 15);

    vector_deinit(&va);
    assert(va.item_count == 0);

    int n = 17;
    size_t sz = vb.item_count;
    while (vb.item_count > 0) {
        n = n % vb.item_count;
        size_t rem = vector_item_remove(&vb, n, 1);
        assert(rem == 1);
        assert(vb.item_count == sz-1);
        --sz;
        n += 13;
    }

    vector_deinit(&vb);
    return 0;
}

