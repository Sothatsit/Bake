/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === stringmap ===
//
// Provides an implementation that allows creating variably sized
// maps to store mappings from string to a pointer to some data.
//

#ifndef CITS2002_STRINGMAP_H
#define CITS2002_STRINGMAP_H

#include "stringbuilder.h"


/**
 * A dynamically growing map containing string to string mappings.
 */
typedef struct {
    /**
     * A buffer that stores an array of StringMapEntry's.
     */
    Buffer entries;
} StringMap;


/**
 * An entry in the StringMap.
 */
typedef struct {
    /**
     * A pointer to the null-terminated key string of this entry.
     *
     * The contents of this should never change.
     */
    char * key;

    /**
     * A pointer to the value of this entry.
     */
    void * value;
} StringMapEntry;


/**
 * Create a StringMap with initial capacity of {@param capacity} entries, and place it in {@param out}.
 */
BakeError strmap_allocate(StringMap * out, size_t capacity);


/**
 * Free the resources of {@param map} and mark it as invalid.
 */
void strmap_free(StringMap * map);


/**
 * @return the number of entries in {@param map}.
 */
size_t strmap_size(StringMap * map);


/**
 * @return A pointer to an array of all StringMapEntry's in this map
 */
StringMapEntry * strmap_entries(StringMap * map);


/**
 * Put {@param value} at the key {@param key} in {@param map}.
 *
 * Will overwrite any previous value at {@param key}, free'ing the previous entry.
 *
 * Both {@param key} and {@param value} will be free'd when strmap_free is called or when a new entry with
 * the same key is added. {@param key} and {@param value} should NOT be free'd outside to this string map.
 *
 * {@param key} should not ever change.
 */
BakeError strmap_put(StringMap * map, char * key, void * value);


/**
 * Get the value associated with the key {@param key} in {@param map}.
 *
 * @return non-null if the map contains the mapping, else NULL
 */
void * strmap_get(StringMap * map, char * key);


#endif //CITS2002_STRINGMAP_H
