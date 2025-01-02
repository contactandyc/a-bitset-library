#include "a-bitset-library/abitset_expandable.h"
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE (1 << 12)              // 4KB page size (2^12)
#define PAGE_ENTRIES (PAGE_SIZE >> 3)   // Number of 64-bit integers in a page (4KB / 8)
#define INITIAL_PAGES (1 << 11)         // Initial number of page pointers (16KB / 8)

/* Structure representing an expandable bitset. */
struct abitset_expandable_s {
    uint64_t **pages;       // Array of pointers to 4K pages
    uint32_t page_count;    // Number of pages allocated in `pages` array
    uint32_t size;          // Total number of bits in the bitset
    uint32_t max_bit;       // The highest bit
};

/* Initializes a new expandable bitset. */
abitset_expandable_t *abitset_expandable_init() {
    abitset_expandable_t *h = (abitset_expandable_t *)aml_zalloc(sizeof(abitset_expandable_t));
    h->pages = (uint64_t **)aml_zalloc(INITIAL_PAGES * sizeof(uint64_t *));
    h->page_count = INITIAL_PAGES;
    h->size = 0;
    h->max_bit = 0;
    return h;
}

/* Destroys the bitset and frees all allocated memory. */
void abitset_expandable_destroy(abitset_expandable_t *h) {
    if (!h) return;
    for (uint32_t i = 0; i < h->page_count; i++) {
        if (h->pages[i]) aml_free(h->pages[i]);
    }
    aml_free(h->pages);
    aml_free(h);
}

/* Expands the bitset to ensure the given ID can be set. */
static void abitset_expandable_expand(abitset_expandable_t *h, uint32_t id) {
    uint32_t required_page = id >> 18;  // Each page covers 2^18 bits (64 * PAGE_ENTRIES)

    if(id > h->max_bit)
        h->max_bit = id;

    // Expand the page pointer array if needed
    if (required_page >= h->page_count) {
        uint32_t new_page_count = h->page_count;
        while (required_page >= new_page_count) {
            new_page_count <<= 1;  // Double the page count
        }
        uint64_t **new_pages = (uint64_t **)aml_zalloc(new_page_count * sizeof(uint64_t *));
        memcpy(new_pages, h->pages, h->page_count * sizeof(uint64_t *));
        aml_free(h->pages);
        h->pages = new_pages;
        h->page_count = new_page_count;
    }

    // Allocate the required page if it doesn't exist
    if (!h->pages[required_page]) {
        h->pages[required_page] = (uint64_t *)aml_zalloc(PAGE_SIZE);
    }

    // Update the size if the new ID exceeds the current size
    uint32_t new_size = (required_page + 1) << 18;  // (required_page + 1) * 2^18
    if (new_size > h->size) {
        h->size = new_size;
    }
}

/* Sets the bit at the given ID to 1. */
void abitset_expandable_set(abitset_expandable_t *h, uint32_t id) {
    abitset_expandable_expand(h, id);
    uint32_t page = id >> 18;  // Divide by (64 * PAGE_ENTRIES) using shift
    uint32_t offset = (id >> 6) & (PAGE_ENTRIES - 1);  // Offset within the page
    uint32_t bit = id & 63;  // Bit within the 64-bit integer
    h->pages[page][offset] |= (1ULL << bit);
}

/* Unsets the bit at the given ID (sets it to 0). */
void abitset_expandable_unset(abitset_expandable_t *h, uint32_t id) {
    abitset_expandable_expand(h, id);
    uint32_t page = id >> 18;
    uint32_t offset = (id >> 6) & (PAGE_ENTRIES - 1);
    uint32_t bit = id & 63;
    h->pages[page][offset] &= ~(1ULL << bit);
}

/* Checks if the bit at the given ID is enabled. */
bool abitset_expandable_enabled(abitset_expandable_t *h, uint32_t id) {
    if(id >= h->max_bit) return false;
    uint32_t page = id >> 18;
    if (page >= h->page_count || !h->pages[page]) return false;
    uint32_t offset = (id >> 6) & (PAGE_ENTRIES - 1);
    uint32_t bit = id & 63;
    return (h->pages[page][offset] & (1ULL << bit)) != 0;
}

/* Counts the number of bits set to 1. */
uint32_t abitset_expandable_count(abitset_expandable_t *h) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < h->page_count; i++) {
        if (!h->pages[i]) continue;
        for (uint32_t j = 0; j < PAGE_ENTRIES; j++) {
            uint64_t val = h->pages[i][j];
            while (val) {
                val &= (val - 1);
                count++;
            }
        }
    }
    return count;
}

uint32_t abitset_expandable_size(abitset_expandable_t *h) {
    return h->max_bit+1;
}


/* Returns the bitset representation as an array of 64-bit integers. */
uint64_t *abitset_expandable_repr(abitset_expandable_t *h) {
    uint32_t size = h->max_bit + 1;  // Logical size in bits
    uint32_t num_entries = (size + 63) >> 6;  // Total number of 64-bit integers
    uint64_t *repr = (uint64_t *)aml_zalloc(num_entries * sizeof(uint64_t));  // Allocate exact space

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
                h->pages[page] = (uint64_t *)aml_zalloc(PAGE_SIZE);
            }

            // Copy the value directly into the page
            h->pages[page][offset] = value;
        }
    }

    // Update the max_bit field to reflect the highest bit
    h->max_bit = size - 1;

    return h;
}
