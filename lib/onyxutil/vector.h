#if !defined(onyxutil_vector_h)
#define onyxutil_vector_h

#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

    /* A vector is a simple array of POD elements of some size.
     * Use vector_t rather than plain arrays or raw malloc memory 
     * to support easy insertion/reallocation/etc.
     */
    struct vector_t {
        void    *items;
        /* The item_count is the number of actual items in the vector */
        size_t  item_count;
        size_t  item_size;
        size_t  phys_size;
    };

    /* Create a new vector, intended to store items of size item_size.
     * @param item_size the size of each item in the vector.
     * @return the allocated vector, or NULL on failure
     */
    vector_t *vector_create(size_t item_size);
    /* Deallocate a vector, destroying the memory in it.
     * @param vec The vector to deallocate
     */
    void vector_destroy(vector_t *vec);
    /* If you allocate the vector_t yourself, but want the library to manage the contents, 
     * use vector_init() and vector_deinit() instead of vector_create() and vector_destroy().
     * @param vec the vector struct to initialize as empty vector.
     * @return 0 on success, -1 on failure (out of memory)
     */
    int vector_init(vector_t *vec, size_t item_size);
    /* Use vector_deinit() to free memory allocated by vector_init() for vectors whose 
     * vector_t struct you declare yourself.
     * @param vec the vector to free
     */
    void vector_deinit(vector_t *vec);
    /* Add an item to the end of the vector, resizing the memory if needed.
     * @param vec the vector to add to
     * @param item the item to add
     * @return the number of items in the vector after the addition, or 0 for error.
     */
    size_t vector_item_append(vector_t *vec, void const *item);
    /* Get a pointer to an item in the vector, given its index.
     * @param vec the vector to get an item from
     * @param index the index of the item to get a pointer to
     * @return the number of items after 
     * Note that the logical item-after-the-last-item is a valid "item" for 
     * purposes of iteration.
     */
    void *vector_item_get(vector_t *vec, size_t index);
    /* Remove one or more items from a vector, given their index.
     * @param vec the vector to remove from
     * @param index the index of the first item to remove (must be <= item_count)
     * @param count the number of items to remove.
     * @return the number of items removed, or 0 for error
     */
    size_t vector_item_remove(vector_t *vec, size_t index, size_t count);
    /* Insert items into the vector at some offset, shuffling other items out of the way
     * in order.
     * @param vec the vector to insert into
     * @param index the index where to place the first new item (must be <= item_count)
     * @param items a pointer to the items to copy into the vector
     * @param count the number of items to copy into the vector
     * @return the numer of items inserted, or 0 for error
     */
    size_t vector_item_insert(vector_t *vec, size_t index, void const *items, size_t count);

    enum {
        /* If your vector contains more than a million items, you don't want a vector.
         * Thus, the library will refuse to make a vector bigger than this.
         */
        VECTOR_MAX_COUNT = 0x100000
    };

#if defined(__cplusplus)
}
#endif

#endif  //  onyxutil_vector_h

