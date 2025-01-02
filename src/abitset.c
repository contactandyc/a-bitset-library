#include <assert.h>
#include "a-bitset-library/abitset.h"

struct abitset_s {
    uint64_t *items;
    uint64_t *ep;
    uint64_t last_mask;
    uint32_t size;
};

abitset_t *abitset_init(aml_pool_t *pool, uint32_t size) {
    // Calculate the number of blocks and the last mask
    uint32_t full_blocks = size >> 6;      // Number of full 64-bit blocks
    uint32_t remaining_bits = size & 63;  // Bits in the last (partial) block
    uint64_t mask = (remaining_bits == 0) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << remaining_bits) - 1);

    // Allocate memory for the bitset structure
    abitset_t *h = (abitset_t *)aml_pool_zalloc(pool, sizeof(*h));
    h->items = (uint64_t *)aml_pool_zalloc(pool, sizeof(uint64_t) * (full_blocks + (remaining_bits > 0 ? 1 : 0)));
    h->ep = h->items + full_blocks + (remaining_bits > 0 ? 1 : 0);
    h->last_mask = mask;
    h->size = size;

    return h;
}

abitset_t *abitset_copy(aml_pool_t *pool, abitset_t *src) {
    // Allocate memory for the new bitset structure
    abitset_t *h = (abitset_t *)aml_pool_zalloc(pool, sizeof(*h));

    // Copy the bitset data from the source
    uint64_t num_blocks = src->ep - src->items;
    h->items = (uint64_t *)aml_pool_dup(pool, src->items, sizeof(uint64_t) * num_blocks);
    h->ep = h->items + num_blocks;

    // Copy the last_mask directly
    h->last_mask = src->last_mask;
    h->size = src->size;
    return h;
}

uint32_t abitset_size(abitset_t *h) {
    return h->size;
}

uint64_t *abitset_repr(abitset_t *h) {
    return h->items;
}

abitset_t *abitset_load(aml_pool_t *pool, uint64_t *repr, uint32_t size, bool make_copy) {
    // Calculate the number of blocks and the last mask
    uint32_t full_blocks = size >> 6;      // Number of full 64-bit blocks
    uint32_t remaining_bits = size & 63;  // Bits in the last (partial) block
    uint64_t mask = (remaining_bits == 0) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << remaining_bits) - 1);

    // Allocate memory for the bitset structure
    abitset_t *h = (abitset_t *)aml_pool_zalloc(pool, sizeof(*h));
    if(make_copy)
        h->items = (uint64_t *)aml_pool_dup(pool, repr,
                                            sizeof(uint64_t) * (full_blocks + (remaining_bits > 0 ? 1 : 0)));
    else
        h->items = repr;
    h->ep = h->items + full_blocks + (remaining_bits > 0 ? 1 : 0);
    h->last_mask = mask;
    h->size = size;

    return h;
}

bool abitset_enabled(abitset_t *h, uint32_t id) {
    uint32_t block = id >> 6;
    uint32_t mask = id & 63;
    uint64_t *p = h->items + block;
    uint64_t *ep = h->ep;
    if(p >= ep)
        return false;
    return (*p & (1ULL<<mask)) != 0 ? true : false;
}

void abitset_set(abitset_t *h, uint32_t id) {
    assert(h && id < h->size);
    uint32_t block = id >> 6;
    uint32_t mask = id & 63;
    uint64_t *p = h->items + block;
    uint64_t *ep = h->ep;
    if(p >= ep)
        return;
    *p |= (1ULL<<mask);
}

void abitset_unset(abitset_t *h, uint32_t id) {
    assert(h && id < h->size);
    uint32_t block = id >> 6ULL;
    uint32_t mask = id & 63ULL;
    uint64_t *p = h->items + block;
    uint64_t *ep = h->ep;
    if(p >= ep)
        return;
    *p &= ~(1ULL<<mask);
}

void abitset_boolean(abitset_t *h, uint32_t id, bool v) {
    if(v)
        abitset_set(h, id);
    else
        abitset_unset(h, id);
}

uint32_t abitset_count(abitset_t *h) {
    uint64_t *p = h->items;
    uint64_t *ep = h->ep;
    unsigned int count = 0;
    while(p < ep) {
        uint64_t n = *p++;
        // Brian Kernighan’s Algorithm
        while (n) {
            n &= (n - 1);
            count++;
        }
    }
    return count;
}

uint32_t abitset_count_and_zero(abitset_t *h) {
    uint64_t *p = h->items;
    uint64_t *ep = h->ep;
    unsigned int count = 0;
    while(p < ep) {
        uint64_t n = *p;
        if(n) {
            // Brian Kernighan’s Algorithm
            while (n) {
                n &= (n - 1);
                count++;
            }
            *p = 0;
        }
        p++;
    }
    return count;
}

#ifndef __builtin_ctzll
static uint32_t portable_ctzll(uint64_t x) {
    uint32_t index = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        index++;
    }
    return index;
}
#define __builtin_ctzll portable_ctzll
#endif

int32_t abitset_first_enabled(abitset_t *bs) {
    uint64_t *p = bs->items;
    uint64_t *ep = bs->ep;

    // Iterate through the blocks in the bitset
    while (p < ep) {
        uint64_t block = *p++;
        if (block != 0) {
            // If there are any set bits in this block, find the first one
            uint32_t bit_index = portable_ctzll(block); // counts trailing zeros (first set bit)
            return (p - bs->items - 1) * 64 + bit_index;
        }
    }

    return -1;
}

void abitset_true(abitset_t *h) {
    memset(h->items, 0xFF, (h->ep - h->items) * sizeof(uint64_t));
    if (h->items < h->ep) h->ep[-1] &= h->last_mask;
}

void abitset_false(abitset_t *h) {
    memset(h->items, 0, (h->ep - h->items) * sizeof(uint64_t));
}

void abitset_not(abitset_t *h) {
    uint64_t *p = h->items;
    uint64_t *ep = h->ep;
    while(p < ep) {
        *p = ~(*p);
        p++;
    }
    if(h->items < h->ep)
        ep[-1] = ep[-1] & h->last_mask;
}

void abitset_and(abitset_t *dest, abitset_t *to_and) {
    uint64_t *p = dest->items;
    uint64_t *p2 = to_and->items;
    uint64_t *ep = dest->ep;
    while(p < ep) {
        *p = *p & *p2;
        p++;
        p2++;
    }
}

void abitset_or(abitset_t *dest, abitset_t *to_or) {
    uint64_t *p = dest->items;
    uint64_t *p2 = to_or->items;
    uint64_t *ep = dest->ep;
    while(p < ep) {
        *p = *p | *p2;
        p++;
        p2++;
    }
}

void abitset_and_not(abitset_t *dest, abitset_t *to_not) {
    uint64_t *p = dest->items;
    uint64_t *p2 = to_not->items;
    uint64_t *ep = dest->ep;
    while(p < ep) {
        *p = *p & (~*p2);
        p++;
        p2++;
    }
}