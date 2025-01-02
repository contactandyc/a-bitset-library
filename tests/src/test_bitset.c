#include <stdio.h>
#include <stdbool.h>
#include "a-bitset-library/abitset.h"

int main() {
    // Create a memory pool for the bitset
    aml_pool_t *pool = aml_pool_init(1024*16);  // Assuming aml_pool_init initializes a memory pool

    // Initialize a bitset with 100 bits
    abitset_t *bitset = abitset_init(pool, 100);
    printf("Initialized bitset with 100 bits.\n");

    // Set some bits
    abitset_set(bitset, 0);    // Set bit 0
    abitset_set(bitset, 99);   // Set the last bit
    abitset_set(bitset, 50);   // Set a middle bit
    printf("Set bits 0, 50, and 99.\n");

    // Check if specific bits are set
    printf("Bit 0 is %s\n", abitset_enabled(bitset, 0) ? "enabled" : "disabled");
    printf("Bit 50 is %s\n", abitset_enabled(bitset, 50) ? "enabled" : "disabled");
    printf("Bit 99 is %s\n", abitset_enabled(bitset, 99) ? "enabled" : "disabled");
    printf("Bit 25 is %s\n", abitset_enabled(bitset, 25) ? "enabled" : "disabled");

    // Unset a bit
    abitset_unset(bitset, 50);
    printf("Unset bit 50. Bit 50 is now %s\n", abitset_enabled(bitset, 50) ? "enabled" : "disabled");

    // Perform bitwise NOT
    abitset_not(bitset);
    printf("Performed NOT operation. Bit 50 is now %s\n", abitset_enabled(bitset, 50) ? "enabled" : "disabled");

    // Perform a bitwise AND with another bitset
    abitset_t *other_bitset = abitset_init(pool, 100);
    abitset_set(other_bitset, 0);   // Set bit 0 in the second bitset
    abitset_and(bitset, other_bitset);
    printf("Performed AND with another bitset. Bit 0 is %s\n", abitset_enabled(bitset, 0) ? "enabled" : "disabled");

    // Count the number of enabled bits
    uint32_t count = abitset_count(bitset);
    printf("Number of enabled bits: %u\n", count);

    // Find the first enabled bit
    int32_t first = abitset_first_enabled(bitset);
    printf("First enabled bit is at index: %d\n", first);

    // Clean up
    aml_pool_destroy(pool);  // Assuming aml_pool_free cleans up all allocations
    printf("Cleaned up resources.\n");

    return 0;
}
