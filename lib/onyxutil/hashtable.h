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
#include <stdint.h>

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
    size_t          item_count;
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
     *
     * @note This affects how hash_func() and comp_func() are called. When the 
     * pointers flag is set, the stored pointers are dereferenced before comparing.
     * You can store pointers in the table (item_size == sizeof(void*)) without this
     * flag; at that point the data being hashed/compared is the pointer value 
     * itself, rather than the pointed-at data.
     */
    HASHTABLE_POINTERS = 0x1
};

/* Initialize a hash table.
 *
 * You provide the storage for the table struct, but the library will 
 * manage storage of the actual items stored. The table will not allocate 
 * any memory until the first element is added to the table.
 *
 * Elements must be POD (as in, be able to be copied with memcpy())
 *
 * A hash table with HASHTABLE_POINTERS flag, stores pointers-to-elements 
 * instead of actual-elements, which means that you manage memory lifetime 
 * of the actual elements. (The hashtable still needs to allocate memory 
 * to manage the pointers, though. No free lunch!)
 *
 * @param table The table to initialize
 * @param item_size The size of each item within the table. For HASHTABLE_POINTERS 
 * tables, this is the size of the pointed-at element, NOT the size of the pointer.
 * If the size of the pointed-at elements is variable, you must make your hash and 
 * comparison functions capable of figuring this out, and can pass any non-0 value
 * here only for HASHTABLE_POINTERS tables; tables that store the actual elements 
 * must know the size, or at least maximum size, of elements.
 * @param flags 0 or HASHTABLE_POINTERS
 * @param hash_fun A function that, given a pointer-to-data and size-of-data, returns 
 * a suitable hash value. crc32() would be a fine, if somewhat slow, hash function.
 * "Phong Hash" is another one to google. If the entire element contains key material 
 * then you can use the pod_hash() function. If the element contains both key and 
 * value materials, you must hash only the key part.
 * @param comp_fun A function that returns 0 for two elements that have the same key 
 * but returns non-0 if they have different keys. For elements that store both key 
 * material and value material, only compare the key portion. For elements that are
 * all key material, you can pass in memcmp.
 * @return A pointer to the initialized hash table (the table argument).
 */
hash_table_t *hash_table_init(
        hash_table_t *table,
        size_t item_size,
        uint32_t flags,
        size_t (*hash_fun)(void const *data, size_t item_size),
        int (*comp_fun)(void const *a, void const *b, size_t item_size));

/* Deallocate all storage for the hash table items. This will leave the table 
 * empty (with zero elements in it) and free all allocated memory.
 * If you use HASHTABLE_POINTERS, it will not free the pointed-at elements 
 * that you manage.
 * @param table The table to clear.
 */
void hash_table_deinit(hash_table_t *table);

/* Find an element matching the given key, if present in the table.
 * @param table The table to look in.
 * @param key The key to look for.
 * @return The found element, or NULL
 */
void *hash_table_find(hash_table_t *table, void *key);

/* Make sure that the given element (key) is stored in the table. Adds 
 * the key if it's not present; else update the existing element.
 * @param table The table to insert/update into.
 * @param key The element to insert/update.
 * @return NULL on failure. In HASHTABLE_POINTERS mode, return the key that 
 * was passed in if a new element is inserted, else the old value pointer 
 * that was replaced. In regular mode, return a pointer to the element copy
 * that * was inserted.
 * @note in HASHTABLE_POINTERS mode, if the return value is a non-NULL pointer 
 * that is not key, then that means the element in question was removed from 
 * the table to be replaced by the new key.
 */
void *hash_table_assign(hash_table_t *table, void *key);

/* Remove the element that matches the given key from the table, if present.
 * @param table The table to remove from.
 * @param key An element to remove from the table.
 * @return 1 when an element is removed, 0 otherwise (if no match found)
 */
int hash_table_remove(hash_table_t *table, void *key);

/* Start iterating through a hashtable. Return the first element in the table.
 * As a special rule, if you remove the element returned from the table, that 
 * will not invalidate the iterator. Removing other elements may invalidate the 
 * iterator. Adding/removing elements during iteration may cause the same element 
 * to be returned twice, or some elements to be missed entirely.
 * @param table The table to start iterating over
 * @param iter The iterator to keep state of the iteration
 * @return Pointer to the first element in the table, or NULL if the table is empty
 * @note Iteration lets you see "all items" in a table, with reasonable efficiency. 
 * The ordering of the elements is not particularly deterministic. Removing elements 
 * other than the currently-returned element may cause further calls to hash_table_next() 
 * to crash. Adding elements may cause ordering to change, and some elements may not 
 * be returned in the iteration, or may be returned twice.
 */
void *hash_table_begin(hash_table_t *table, hash_iterator_t *iter);

/* Return the next element in a hash table iteration.
 * @param iter The iterator currently being used.
 * @return NULL when at the end, else pointer to next element.
 * @note Regarding adding/removing elements during iteration, see notes at hash_table_begin().
 */
void *hash_table_next(hash_iterator_t *iter);


#if defined(__cplusplus)
}
#endif


#endif  //  onyxutil_hashtable_h
