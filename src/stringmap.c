/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
 */

#include "stringmap.h"


BakeError strmap_allocate(StringMap * out, size_t capacity) {
    // Attempt to allocate a buffer to hold all the entries
    BakeError err = buf_allocate(&out->entries, capacity * sizeof(StringMapEntry));
    if(err != BAKE_SUCCESS)
        return err;

    return BAKE_SUCCESS;
}


void strmap_free(StringMap * map) {
    size_t size = strmap_size(map);
    StringMapEntry * entries = strmap_entries(map);

    // Free all entries
    for(size_t index = 0; index < size; ++index) {
        StringMapEntry * entry = &entries[index];

        // Free this entry's key and value
        free(entry->key);
        free(entry->value);

        // Mark it as invalid
        entry->key = NULL;
        entry->value = NULL;
    }

    // Free the entries buffer
    buf_free(&map->entries);
}


size_t strmap_size(StringMap * map) {
    // The amount of StringMapEntry's that could fit in the used memory
    return map->entries.used / sizeof(StringMapEntry);
}


StringMapEntry * strmap_entries(StringMap * map) {
    // Return a pointer to the map's data
    return buf_get(&map->entries);
}


BakeError strmap_put(StringMap * map, char * key, void * value) {
    size_t size = strmap_size(map);
    StringMapEntry * entries = strmap_entries(map);

    // See if we can find an existing mapping to update
    for(size_t index = 0; index < size; ++index) {
        StringMapEntry * entry = &entries[index];

        // If this entry's key doesn't match, move on to the next one
        if(strcmp(key, entry->key) != 0)
            continue;

        // Free the previous key and value
        free(entry->key);
        free(entry->value);

        // Update the entry's key and value to the new ones!
        entry->key = key;
        entry->value = value;

        return BAKE_SUCCESS;
    }

    // Construct a new entry
    StringMapEntry entry;
    entry.key = key;
    entry.value = value;

    // Append the entry to the entries buffer
    BakeError err = buf_append(&map->entries, &entry, sizeof(StringMapEntry));
    if(err != BAKE_SUCCESS)
        return err;

    return BAKE_SUCCESS;
}


void * strmap_get(StringMap * map, char * key) {
    size_t size = strmap_size(map);
    StringMapEntry * entries = strmap_entries(map);

    // Loop through all entries looking for an entry with a matching key
    for(size_t index = 0; index < size; ++index) {
        StringMapEntry entry = entries[index];

        // If this entry's key doesn't match, move on to the next one
        if(strcmp(key, entry.key) != 0)
            continue;

        // We found the matching entry!
        return entry.value;
    }

    // We found no matching entries
    return NULL;
}
