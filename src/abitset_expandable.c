// SPDX-FileCopyrightText: 2023–2026 Andy Curtis <contactandyc@gmail.com>
// SPDX-FileCopyrightText: 2024–2025 Knode.ai — technical questions: contact Andy (above)
// SPDX-License-Identifier: Apache-2.0

#include "a-bitset-library/abitset_expandable.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include "a-memory-library/aml_alloc.h"

#define PAGE_SIZE (1 << 12)              // 4KB page size (2^12)
#define PAGE_ENTRIES (PAGE_SIZE >> 3)   // Number of 64-bit integers in a page (4KB / 8)
#define INITIAL_PAGES (1 << 11)         // Initial number of page pointers (16KB / 8)

/* Structure representing an expandable bitset. */
struct abitset_expandable_s {
    _Atomic(_Atomic(uint64_t) *) *pages;   // Array of atomic pointers to 4K pages
    uint64_t **old_pages;                // Regular pointer to retired pages
    _Atomic(uint32_t) page_count;         // Atomic page count
    _Atomic(uint32_t) max_bit;            // The highest bit (atomic)
    atomic_uint_fast32_t bit_count;       // Atomic count of bits set
};

/* Initializes a new expandable bitset. */
abitset_expandable_t *abitset_expandable_init(void) {
    abitset_expandable_t *h = (abitset_expandable_t *)aml_calloc(1, sizeof(abitset_expandable_t));
    h->pages = (_Atomic(_Atomic(uint64_t) *) *)aml_calloc(INITIAL_PAGES, sizeof(_Atomic(_Atomic(uint64_t) *)));
    atomic_store(&h->page_count, INITIAL_PAGES);
    atomic_store(&h->max_bit, 0);
    atomic_init(&h->bit_count, 0);
    return h;
}

/* Destroy the bitset and free all allocated memory. */
void abitset_expandable_destroy(abitset_expandable_t *h) {
    if (!h) return;

    uint32_t page_count = atomic_load(&h->page_count);

    // Free all allocated pages
    for (uint32_t i = 0; i < page_count; i++) {
        _Atomic(uint64_t) *page = atomic_load(&h->pages[i]);
        if (page) {
            aml_free((uint64_t *)page);
        }
    }

    // Free retired pages
    if (h->old_pages) {
        aml_free(h->old_pages);
    }

    // Free the pages array and the bitset itself
    aml_free(h->pages);
    aml_free(h);
}

/* Expands the bitset to include the required ID. */
static void abitset_expandable_expand(abitset_expandable_t *h, uint32_t id) {
    uint32_t required_page = id >> 18;  // Each page covers 2^18 bits

    // Update max_bit atomically
    uint32_t max_bit = atomic_load(&h->max_bit);
    if (id > max_bit) {
        atomic_store(&h->max_bit, id);
    }

    uint32_t page_count = atomic_load(&h->page_count);

    // Check if resizing is needed
    if (required_page >= page_count) {
        uint32_t new_page_count = page_count;
        while (required_page >= new_page_count) {
            new_page_count <<= 1;  // Double the page count
        }

        // Allocate a new pages array
        _Atomic(_Atomic(uint64_t) *) *new_pages = (_Atomic(_Atomic(uint64_t) *) *)aml_calloc(new_page_count, sizeof(_Atomic(_Atomic(uint64_t) *)));
        memcpy(new_pages, h->pages, page_count * sizeof(_Atomic(_Atomic(uint64_t) *)));

        // Retire the old pages array
        uint64_t **old_pages = h->old_pages;
        h->old_pages = (uint64_t **)h->pages;

        // Free the old pages array after the next resize or destruction
        if (old_pages) {
            free(old_pages);
        }

        // Update the page count atomically
        atomic_store(&h->page_count, new_page_count);
    }

    // Allocate the required page if it doesn't exist
    _Atomic(uint64_t) *page = atomic_load(&h->pages[required_page]);
    if (!page) {
        _Atomic(uint64_t) *new_page = (_Atomic(uint64_t) *)aml_calloc(PAGE_ENTRIES, sizeof(_Atomic(uint64_t)));
        if (atomic_compare_exchange_strong(&h->pages[required_page], &page, (_Atomic(uint64_t) *)new_page)) {
            // Successfully stored the new page
        } else {
            free(new_page); // Another thread already initialized it
        }
    }
}

void abitset_expandable_set(abitset_expandable_t *h, uint32_t id) {
    abitset_expandable_expand(h, id);
    uint32_t page = id >> 18;
    uint32_t offset = (id >> 6) & (PAGE_ENTRIES - 1);
    uint32_t bit = id & 63;

    _Atomic(uint64_t) *entry = atomic_load(&h->pages[page]) + offset;
    if (!(atomic_fetch_or(entry, (1ULL << bit)) & (1ULL << bit))) {
        atomic_fetch_add(&h->bit_count, 1);
    }
}

void abitset_expandable_unset(abitset_expandable_t *h, uint32_t id) {
    abitset_expandable_expand(h, id);
    uint32_t page = id >> 18;
    uint32_t offset = (id >> 6) & (PAGE_ENTRIES - 1);
    uint32_t bit = id & 63;

    _Atomic(uint64_t) *entry = atomic_load(&h->pages[page]) + offset;
    if (atomic_fetch_and(entry, ~(1ULL << bit)) & (1ULL << bit)) {
        atomic_fetch_sub(&h->bit_count, 1);
    }
}

bool abitset_expandable_enabled(abitset_expandable_t *h, uint32_t id) {
    uint32_t page_count = atomic_load(&h->page_count);
    uint32_t required_page = id >> 18;

    if (required_page >= page_count) {
        return false;  // Prevent out-of-bounds access
    }
    _Atomic(uint64_t) *page = atomic_load(&h->pages[required_page]);
    if (!page) {
        return false;
    }

    uint32_t offset = (id >> 6) & (PAGE_ENTRIES - 1);
    uint32_t bit = id & 63;
    return (atomic_load(page + offset) & (1ULL << bit)) != 0;
}

uint32_t abitset_expandable_count(abitset_expandable_t *h) {
    return atomic_load(&h->bit_count);
}

uint32_t abitset_expandable_size(abitset_expandable_t *h) {
    return atomic_load(&h->max_bit) + 1;
}

/* Returns the bitset representation as an array of 64-bit integers. */
uint64_t *abitset_expandable_repr(abitset_expandable_t *h) {
    uint32_t size = h->max_bit + 1;  // Logical size in bits
    uint32_t num_entries = (size + 63) >> 6;  // Total number of 64-bit integers
    uint64_t *repr = (uint64_t *)aml_calloc(1,num_entries * sizeof(uint64_t));  // Allocate exact space

    for (uint32_t i = 0; i < h->page_count; i++) {
        if (!h->pages[i]) continue;

        // Calculate the start index in the repr array for this page
        uint32_t start_idx = i << 9;  // (i * PAGE_ENTRIES)

        // Calculate the number of bytes to copy
        uint32_t bits_remaining = size - (start_idx << 6);  // Remaining bits in repr starting from this page
        uint32_t bytes_to_copy = (bits_remaining >= (PAGE_ENTRIES << 6))
                                    ? PAGE_SIZE
                                    : ((bits_remaining + 63) >> 6) << 3;  // Convert bits to bytes

        memcpy(&repr[start_idx], h->pages[i], bytes_to_copy);
    }

    return repr;
}

/* Creates a bitset using the bits from repr. */
abitset_expandable_t *abitset_expandable_load(uint64_t *repr, uint32_t size) {
    // Initialize a new expandable bitset
    abitset_expandable_t *h = abitset_expandable_init();

    // Calculate the number of 64-bit entries in the representation
    uint32_t num_entries = (size + 63) >> 6;  // Total number of 64-bit integers

    // Expand the bitset to accommodate the highest bit (size - 1)
    abitset_expandable_expand(h, size - 1);

    // Copy the data from repr into the bitset pages
    for (uint32_t i = 0; i < num_entries; i++) {
        uint64_t value = repr[i];
        if (value) {  // Only process non-zero entries
            uint32_t page = i >> 9;                     // i / PAGE_ENTRIES
            uint32_t offset = i & (PAGE_ENTRIES - 1);   // i % PAGE_ENTRIES

            // Allocate the page if it doesn't already exist
            if (!h->pages[page]) {
                h->pages[page] = (_Atomic(uint64_t) *)aml_calloc(PAGE_ENTRIES, sizeof(_Atomic(uint64_t)));
            }

            // Copy the value directly into the page
            h->pages[page][offset] = value;
        }
    }

    // Update the max_bit field to reflect the highest bit
    h->max_bit = size - 1;

    return h;
}
