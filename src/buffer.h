/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === buffer ===
//
// A resizable buffer implementation, useful for storing variable length data.
//

#ifndef CITS2002_BUFFER_H
#define CITS2002_BUFFER_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <stdbool.h>
#include "errors.h"


/**
 * A dynamically growing buffer.
 */
typedef struct {
    /**
     * A pointer to the start of the buffer's data.
     */
    char * data;

    /**
     * The number of characters allocated for use within data.
     */
    size_t capacity;

    /**
     * The number of characters used in data.
     */
    size_t used;
} Buffer;


/**
 * Allocate a Buffer with the initial capacity {@param capacity}, and place it into {@param out}.
 */
BakeError buf_allocate(Buffer * out, size_t capacity);


/**
 * Free the memory of {@param buffer} and mark it as invalid.
 */
void buf_free(Buffer * buffer);


/**
 * Reset {@param buffer} to empty.
 */
void buf_reset(Buffer * buffer);


/**
 * @return the data contained within {@param buffer}
 */
void * buf_get(Buffer * buffer);


/**
 * Get a pointer to the head of the Buffer; where new characters should be written.
 */
void * buf_head(Buffer * buffer);


/**
 * @return the number of characters left available for writing in {@param buffer}.
 */
size_t buf_available(Buffer * buffer);


/**
 * Grow {@param buffer} to have a capacity of {@param capacity} characters.
 *
 * Requires that {@param capacity} be greater than buffer->capacity.
 */
BakeError buf_growToCapacity(Buffer * buffer, size_t capacity);


/**
 * If needed, grow {@param buffer} to fit another {@param required} characters.
 *
 * Will double the size of buffer until it fits all the extra characters.
 */
BakeError buf_growToFit(Buffer * buffer, size_t required);


/**
 * Append {@param length} characters from {@param data} into {@param buffer}.
 */
BakeError buf_append(Buffer * buffer, void * data, size_t length);


#endif //CITS2002_BUFFER_H
