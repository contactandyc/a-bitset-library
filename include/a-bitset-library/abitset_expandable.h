#ifndef _abitset_expandable_h
#define _abitset_expandable_h

#include <stdint.h>
#include <stdbool.h>

/*
 * The expandable bitset supports setting, unsetting, querying bits.  It will expand automatically when
 * set or unset is called.
 */

struct abitset_expandable_s;
typedef struct abitset_expandable_s abitset_expandable_t;

/* Initializes a new bitset */
abitset_expandable_t * abitset_expandable_init();

/* Destroys the bitset */
void abitset_expandable_destroy(abitset_expandable_t *h);

/* Returns the number of bits in the bit set */
uint32_t abitset_expandable_size(abitset_expandable_t *h);

/* Returns an array of 64 bit integers representing the bit set (used for serialization).
   This must be deallocated using aml_free.
 */
uint64_t *abitset_expandable_repr(abitset_expandable_t *h);

/* Creates a bitset using the bits from repr */
abitset_expandable_t * abitset_expandable_load(uint64_t *repr, uint32_t size);

/* Checks if the bit at the given ID is enabled. Returns true if set, false otherwise. */
bool abitset_expandable_enabled(abitset_expandable_t *h, uint32_t id);

/* Sets the bit at the given ID to 1. */
void abitset_expandable_set(abitset_expandable_t *h, uint32_t id);

/* Unsets the bit at the given ID (sets it to 0). */
void abitset_expandable_unset(abitset_expandable_t *h, uint32_t id);

/* Counts the number of bits set to 1 in the bitset. */
uint32_t abitset_expandable_count(abitset_expandable_t *h);

#endif
