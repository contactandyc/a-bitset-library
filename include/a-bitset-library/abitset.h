#ifndef _abitset_h
#define _abitset_h

#include "a-memory-library/aml_pool.h"

/*
 * The bitset supports common operations like setting, unsetting, querying bits, and performing
 * bitwise operations such as AND, OR, and NOT.  It is designed to work with a-memory-library's
 * aml_pool for allocation.
 */

/* Defines the `abitset_t` structure and provides a typedef. */
struct abitset_s;
typedef struct abitset_s abitset_t;

/* Initializes a new bitset with the given number of bits */
abitset_t * abitset_init(aml_pool_t *pool, uint32_t size);

/* Creates a copy of the given bitset. */
abitset_t *abitset_copy(aml_pool_t *pool, abitset_t *src);

/* Returns the number of bits in the bit set */
uint32_t abitset_size(abitset_t *h);

/* Returns an array of 64 bit integers representing the bit set (used for serialization) */
uint64_t *abitset_repr(abitset_t *h);

/* Creates a bitset using the bits from repr, this makes a copy if make_copy is true */
abitset_t * abitset_load(aml_pool_t *pool, uint64_t *repr, uint32_t size, bool make_copy);

/* Checks if the bit at the given ID is enabled. Returns true if set, false otherwise. */
bool abitset_enabled(abitset_t *h, uint32_t id);

/* Sets the bit at the given ID to 1. */
void abitset_set(abitset_t *h, uint32_t id);

/* Unsets the bit at the given ID (sets it to 0). */
void abitset_unset(abitset_t *h, uint32_t id);

/* Sets or unsets the bit at the given ID based on the boolean value provided. */
void abitset_boolean(abitset_t *h, uint32_t id, bool v);

/* Counts the number of bits set to 1 in the bitset. */
uint32_t abitset_count(abitset_t *h);

/* Counts the number of bits set to 1 and resets all bits to 0. */
uint32_t abitset_count_and_zero(abitset_t *h);

/* Finds the first enabled bit (set to 1) and returns its index, or -1 if none are set. */
int32_t abitset_first_enabled(abitset_t *bs);

/* Sets all bits in the bitset to 1, considering valid bits in the last block. */
void abitset_true(abitset_t *h);

/* Sets all bits in the bitset to 0. */
void abitset_false(abitset_t *h);

/* Performs a bitwise NOT operation on the bitset (flips the bits). */
void abitset_not(abitset_t *h);

/* Performs a bitwise AND operation on two bitsets, storing the result in the destination. */
void abitset_and(abitset_t *dest, abitset_t *to_and);

/* Performs a bitwise OR operation on two bitsets, storing the result in the destination. */
void abitset_or(abitset_t *dest, abitset_t *to_or);

/* Performs a bitwise AND-NOT operation on two bitsets, storing the result in the destination. */
void abitset_and_not(abitset_t *dest, abitset_t *to_not);

#endif
