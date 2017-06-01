#include <onyxutil/hashtable.h>
#include <stdio.h>
#include <assert.h>


struct item {
    char key[12];
    char value[16];
};

size_t hash_12(void const *data, size_t) {
    return hash_pod(data, 12);
}

int comp_12(void const *a, void const *b, size_t) {
    return memcmp(a, b, 12);
}

bool same_number(item *i) {
    int a = -1;
    int b = -1;
    if (1 != sscanf(i->key, "key %d", &a)) {
        return false;
    }
    if (1 != sscanf(i->value, "value %d", &b)) {
        return false;
    }
    if (a != b) {
        return false;
    }
    return true;
}

void simple_test() {
    hash_table_t ht;
    memset(&ht, 0xff, sizeof(ht));
    hash_table_init(&ht, sizeof(item), 0, hash_12, comp_12);
    assert(ht.item_count == 0);
    hash_iterator_t iter;
    void *iptr = hash_table_begin(&ht, &iter);
    assert(iptr == NULL);
    iptr = hash_table_next(&iter);
    assert(iptr == NULL);
    hash_table_deinit(&ht);
}

void big_test() {
    hash_table_t ht;
    hash_table_init(&ht, sizeof(item), 0, hash_12, comp_12);
    item itm = { 0 };
    for (int i = 0; i != 1000; ++i) {
        memset(&itm, 0, sizeof(itm));
        sprintf(itm.key, "key %d", i);
        sprintf(itm.value, "value %d", i);
        hash_table_assign(&ht, &itm);
        assert(ht.item_count == (size_t)(i+1));
    }
    memset(&itm, 0, sizeof(itm));
    sprintf(itm.key, "key 666");
    void *p = hash_table_find(&ht, &itm);
    assert(p != NULL);
    assert(!strcmp(((item *)p)->value, "value 666"));
    assert(!strcmp(((item *)p)->key, "key 666"));
    assert(p != &itm);
    assert(same_number((item *)p));
    int r = hash_table_remove(&ht, &itm);
    assert(r != 0);
    r = hash_table_remove(&ht, &itm);
    assert(r == 0);
    p = hash_table_find(&ht, &itm);
    assert(p == NULL);
    assert(ht.item_count == 999);
    hash_iterator_t iter;
    int n = 0;
    for (void *p = hash_table_begin(&ht, &iter); p; p = hash_table_next(&iter)) {
        assert(same_number((item *)p));
        n++;
    }
    assert(n == 999);
}

int main() {
    simple_test();
    big_test();
    return 0;
}

