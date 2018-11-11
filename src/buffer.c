/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
 */

#include "buffer.h"


BakeError buf_allocate(Buffer * out, size_t capacity) {
    // Allocate space for the buffer contents
    out->data = malloc(capacity);
    out->capacity = capacity;
    out->used = 0;

    // If we weren't able to allocate the memory
    if(out->data == NULL) {
        reportError("Error allocating data: %s\n", strerror(errno));
        return BAKE_ERROR_MEMORY;
    }

    return BAKE_SUCCESS;
}


void buf_free(Buffer * buffer) {
    // Free the buffer's data
    free(buffer->data);

    // Mark it as invalid
    buffer->data = NULL;
    buffer->capacity = 0;
    buffer->used = 0;
}


void buf_reset(Buffer * buffer) {
    // Reset head to the start of buffer
    buffer->used = 0;
}


void * buf_get(Buffer * buffer) {
    // Returns the pointer to the start of the data in buffer
    return buffer->data;
}


void * buf_head(Buffer * buffer) {
    // The head is a pointer to just after the used memory of the buffer
    return &(buffer->data[buffer->used]);
}


size_t buf_available(Buffer * buffer) {
    // The amount of characters left that we could write data into
    return buffer->capacity - buffer->used;
}


BakeError buf_growToCapacity(Buffer * buffer, size_t capacity) {
    // We don't want to deal with shrinking the buffer, and potentially losing data
    if(capacity <= buffer->capacity) {
        reportError("An unexpected program error occured\n");
        return BAKE_ERROR_ARGUMENTS;
    }

    // Reallocate the memory of buffer to the larger capacity
    buffer->data = realloc(buffer->data, capacity);
    if(buffer->data == NULL) {
        reportError("Error while attempting to grow data buffer: %s\n", strerror(errno));
        return BAKE_ERROR_MEMORY;
    }

    // Update the capacity of the buffer so we can write into the new space
    buffer->capacity = capacity;

    return BAKE_SUCCESS;
}


BakeError buf_growToFit(Buffer * buffer, size_t required) {
    // Work out the buffer size we need to hold required new characters
    size_t requiredSize = buffer->used + required;
    size_t newSize = buffer->capacity;

    // If we do not need to grow the data
    if(newSize >= requiredSize)
        return BAKE_SUCCESS;

    // Keep doubling the new size until we can hold all the new characters
    do {
        newSize *= 2;
    } while(newSize < requiredSize);

    // Grow the buffer to the new capacity
    return buf_growToCapacity(buffer, newSize);
}


BakeError buf_append(Buffer * buffer, void * string, size_t length) {
    // Grow the buffer to make sure it can hold all the new data
    BakeError err = buf_growToFit(buffer, length);
    if(err != BAKE_SUCCESS)
        return err;

    // Copy the data into the head of the buffer, and move the head past the appended data
    char * head = buf_head(buffer);
    memcpy(head, string, length);
    buffer->used += length;

    return BAKE_SUCCESS;
}
