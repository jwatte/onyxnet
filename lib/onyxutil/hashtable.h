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

/* Specialize hash if your key is not POD
 *
 *      template<> struct hash<mytype> {
 *          size_t operator()(mytype const &k) {
 *              size_t value = .. compute hash function .. ;
 *              return value;
 *          }
 *      };
 */
template<typename K> struct hash {
    size_t operator()(K const &k) {
        size_t ret = 1;
        unsigned char *p = (unsigned char *)&k;
        unsigned char *q = p + sizeof(k);
        while (p != q) {
            /* some randomly chosen primes */
            ret = ret * 22511 + 33469 + *p;
            p++;
        }
        return ret;
    }
};

/* Hash table from K to V, using a specified hash functor */
template<typename K, typename V, typename Hash=hash<K> > struct hashtable {

    enum {
        /* How many slots are available at a minimum */
        kMinTopSize = 32
    };
    struct iterator;
    struct node;

    /* create an empty hashtable */
    hashtable() {
        numitems = 0;
        topsize = kMinTopSize;
        top = new node*[topsize];
        memset(top, 0, sizeof(node *)*topsize);
    }

    /* @return how many items are currently in the table */
    size_t size() {
        return numitems;
    }

    /* @return an iterator to the element with key k, or the end() iterator */
    iterator find(K const &k) {
        node *n = findnode(k);
        return iterator(this, n);
    }

    /* Make sure the key k contains the value v, inserting or overwriting as necessary.
     * @param k the key to look up or insert
     * @param v the value to store at the key
     * @return an iterator to the created/updated value
     */
    iterator assign(K const &k, V const &v) {
        node *n = findnode(k);
        if (n != NULL) {
            n->v = v;
            return iterator(this, n);
        }
        n = new node(k, v);
        size_t bucket = n->code & (topsize-1);
        n->next = top[bucket];
        top[bucket] = n;
        ++numitems;
        if (numitems > topsize) {
            rehash(topsize*2);
        }
        return iterator(this, n);
    }

    /* Remove the item referenced.
     * @param i an iterator referencing the value to remove.
     */
    void remove(iterator const &i) {
        assert(i.table == this);
        assert(numitems > 0);
        size_t bucket = i.node->code & (topsize-1);
        node **npp = &top[bucket];
        while (**npp != i.node) {
            npp = &(*npp)->next;
        }
        *npp = i.node->next;
        delete i.node;
        --numitems;
        if (topsize > kMinTopSize && numitems <= topsize/2) {
            rehash(topsize/2);
        }
    }

    /* @return an iterator to the first element in the hash table */
    iterator begin() {
        for (size_t i = 0; i != topsize; ++i) {
            if (top[i]) {
                return iterator(this, top[i]);
            }
        }
        return end();
    }

    /* @return an iterator representing one-past-the-last-element of the hash table */
    iterator end() {
        return iterator(this, NULL);
    }

    /* @note internal function used by other functions above */
    node *findnode(K const &k) {
        size_t code = Hash()(k);
        size_t bucket = code & (topsize - 1);
        node *ret = top[bucket];
        while (ret) {
            if (ret->code == code) {
                if (ret->k == k) {
                    break;
                }
            }
            ret = ret->next;
        }
        return ret;
    }

    /* @note internal function used by insertion/removal to balance hash table size */
    void rehash(size_t newsize) {
        assert(!(newsize & (newsize-1)));
        node *newtop = new node*[newsize];
        memset(newtop, 0, sizeof(node *)*newsize);
        for (size_t i = 0; i != topsize; ++i) {
            node *n = top[i];
            while (n != NULL) {
                node *r = n;
                n = n->next;
                r->next = newtop[r->code & (newsize-1)];
                newtop[r->code & (newsize-1)] = r;
            }
        }
        delete[] top;
        top = newtop;
        topsize = newsize;
    }

    struct node {

        node(K const &k, V const &v) : code(Hash()(k)), next(NULL), key(k), value(v) {}

        size_t code;
        node *next;
        K key;
        V value;
    };


    struct iterator {

        iterator() : table(NULL), item(NULL) {}
        iterator(hashtable *t, node *n) : table(t), item(n) {}

        bool operator==(iterator const &o) const {
            return item == o.item;
        }
        iterator &operator++() {
            forward();
            return *this;
        }
        iterator operator++(int) {
            iterator old(*this);
            forward();
            return old;
        }
        V &operator*() {
            return item->v;
        }
        V &value() {
            return item->v;
        }
        K const &key() {
            return item->k;
        }
        void forward() {
            if (item == NULL) {
                return;
            }
            if (item->next) {
                item = item->next;
                return;
            }
            size_t slot = item->code & (table->topsize-1);
            while (slot != table->topsize-1) {
                ++slot;
                item = table->top[slot];
                if (item != NULL) {
                    return;
                }
            }
            item = NULL;
        }
        hashtable *table;
        node *item;
    };

    node **top;
    size_t topsize;
    size_t numitems;
};

extern "C"
#endif
int is_hashtable_awesome();

#endif  //  onyxutil_hashtable_h
