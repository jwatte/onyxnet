/* This hashtable has a simple implementation, and keeps the load factor at 1 for a 
 * reasonable memory/performance trade-off. Improvements that could be made but haven't 
 * been include:
 * - Pooling memory allocator (maybe -- has space trade-offs)
 * - Incremental re-hashing on size changes (amortize cost of growing/shrinking)
 */


#if !defined(onyxutil_hashtable_h)
#define onyxutil_hashtable_h

#include <string.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Given some data that is POD and doesn't contain uninitialized padding/fillers, 
 * compute a well-distributed hash function of the data.
 * @param data Pointer to the data
 * @param size Size of the data
 * @return The hash code computed on the contents
 * @note The hash function should return different values for string that are only
 * subtly different. For example, the string of a single 0-byte, or the string of 
 * two 0-bytes, should be distinct. Similarly, the string of AB and the string of 
 * BA should be distinct.
 */
size_t hash_pod(void const *data, size_t size);

typedef struct hash_node_t {
    struct          hash_node_t *next;
    size_t          code;
} hash_node_t;

typedef struct hash_table_t {
    hash_node_t     **top;
    size_t          top_size;
    size_t          item_size;
    size_t          num_items;
    uint32_t        flags;
    size_t          (*hash_func)(void const *item, size_t size);
    int             (*comp_func)(void const *a, void const *b, size_t size);
} hash_table_t;

typedef struct hash_iterator_t {
    hash_table_t    *table;
    hash_node_t     *node;
} hash_iterator_t;

enum {
    /* If HASHTABLE_POINTERS is set, then each item actually stored in the table
     * is a pointer to some data that you manage storage of. If it's not set, then 
     * the hash table copies items of the given size into the table on assignment.
     * @note This affects how hash_func() and comp_func() are called. When the 
     * pointers flag is set, the stored pointers are dereferenced before comparing.
     */
    HASHTABLE_POINTERS = 0x1
};

hash_table_t *hash_table_init(hash_table_t *table, size_t item_size, uint32_t flags);
void hash_table_deinit(hash_table_t *table);

void *hash_table_find(hash_table_t *table, void *key);
void *hash_table_assign(hash_table_t *table, void *key);
int hash_table_remove(hash_table_t *table, void *key);
void *hash_table_begin(hash_table_t *table, hash_iterator_t *iter);
void *hash_table_next(hash_iterator_t *iter);


#if defined(__cplusplus)
}
#endif


#endif  //  onyxutil_hashtable_h
