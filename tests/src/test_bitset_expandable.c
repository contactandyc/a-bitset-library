#include <stdio.h>
#include <stdbool.h>
#include "a-bitset-library/abitset_expandable.h"

int main() {
    // Initialize an expandable bitset
    abitset_expandable_t *bitset = abitset_expandable_init();
    printf("Initialized expandable bitset.\n");

    // Set some bits
    abitset_expandable_set(bitset, 0);      // Set bit 0
    abitset_expandable_set(bitset, 100);    // Set bit 100
    abitset_expandable_set(bitset, 4095);   // Set bit 4095 (last bit in the first page)
    abitset_expandable_set(bitset, 8192);   // Set bit 8192 (first bit in the third page)
    printf("Set bits 0, 100, 4095, and 8192.\n");

    // Check if specific bits are set
    printf("Bit 0 is %s\n", abitset_expandable_enabled(bitset, 0) ? "enabled" : "disabled");
    printf("Bit 100 is %s\n", abitset_expandable_enabled(bitset, 100) ? "enabled" : "disabled");
    printf("Bit 4095 is %s\n", abitset_expandable_enabled(bitset, 4095) ? "enabled" : "disabled");
    printf("Bit 8192 is %s\n", abitset_expandable_enabled(bitset, 8192) ? "enabled" : "disabled");
    printf("Bit 2000 is %s\n", abitset_expandable_enabled(bitset, 2000) ? "enabled" : "disabled");

    // Unset a bit
    abitset_expandable_unset(bitset, 100);
    printf("Unset bit 100. Bit 100 is now %s\n", abitset_expandable_enabled(bitset, 100) ? "enabled" : "disabled");

    // Count the number of enabled bits
    uint32_t count = abitset_expandable_count(bitset);
    printf("Number of enabled bits: %u\n", count);

    // Serialize the bitset
    uint64_t *repr = abitset_expandable_repr(bitset);
    uint32_t size = abitset_expandable_size(bitset);
    printf("Serialized bitset to an array of size %u.\n", size);

    // Create a new bitset from the serialized data
    abitset_expandable_t *loaded_bitset = abitset_expandable_load(repr, size);
    printf("Loaded bitset from serialized data.\n");

    // Verify the loaded bitset
    printf("Bit 0 in loaded bitset is %s\n", abitset_expandable_enabled(loaded_bitset, 0) ? "enabled" : "disabled");
    printf("Bit 8192 in loaded bitset is %s\n", abitset_expandable_enabled(loaded_bitset, 8192) ? "enabled" : "disabled");
    printf("Bit 100 in loaded bitset is %s\n", abitset_expandable_enabled(loaded_bitset, 100) ? "enabled" : "disabled");

    // Cleanup
    abitset_expandable_destroy(bitset);
    abitset_expandable_destroy(loaded_bitset);
    aml_free(repr);
    printf("Cleaned up resources.\n");

    return 0;
}
