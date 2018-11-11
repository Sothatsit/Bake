/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
 */

#include <stdarg.h>
#include <zconf.h>
#include "stringbuilder.h"


BakeError strbuilder_allocate(StringBuilder * out, size_t capacity) {
    // Allocate a new buffer for all of this builders characters, plus the null character
    BakeError err = buf_allocate(&out->buffer, capacity + 1);
    if(err != BAKE_SUCCESS)
        return err;

    // Reset the StringBuilder to get ready for new characters
    strbuilder_reset(out);

    return BAKE_SUCCESS;
}


void strbuilder_free(StringBuilder * builder) {
    // Free the contents of builder
    buf_free(&builder->buffer);
}


void strbuilder_reset(StringBuilder * builder) {
    // Reset the buffer and place a null character at its start
    buf_reset(&builder->buffer);
    strbuilder_head(builder)[0] = '\0';
}


char * strbuilder_get(StringBuilder *builder) {
    // Get the data of the buffer as a char *
    return builder->buffer.data;
}


char * strbuilder_head(StringBuilder * builder) {
    // Get the head of the buffer as a char *
    return buf_head(&builder->buffer);
}


size_t strbuilder_available(StringBuilder * builder) {
    // Get the available space in the buffer, and subtract the space for the null character
    size_t available = buf_available(&builder->buffer);
    return (available > 0 ? available - 1 : 0);
}


BakeError strbuilder_growToFit(StringBuilder * builder, size_t required) {
    // Grow to include required new characters, and the null character
    return buf_growToFit(&builder->buffer, required + 1);
}


BakeError strbuilder_moveHead(StringBuilder * builder, size_t moveBy) {
    // Move the head by moveBy characters
    builder->buffer.used += moveBy;

    // If there's no room to fit the null character
    if(buf_available(&builder->buffer) == 0) {
        reportError("Was left no space to fit the null character\n");
        return BAKE_ERROR_ARGUMENTS;
    }

    // Place a null character at the new head
    strbuilder_head(builder)[0] = '\0';

    return BAKE_SUCCESS;
}


BakeError strbuilder_append(StringBuilder * builder, char * string) {
    // Append the whole of the string into builder
    return strbuilder_appendSubstring(builder, string, strlen(string));
}


BakeError strbuilder_appendSubstring(StringBuilder *builder, char *string, size_t length) {
    // Make sure we have enough space to hold length more characters, plus the null character
    BakeError err = strbuilder_growToFit(builder, length);
    if(err != BAKE_SUCCESS)
        return err;

    // Append the contents of string to the buffer
    err = buf_append(&builder->buffer, string, length);
    if(err != BAKE_SUCCESS)
        return err;

    // buf_append already moved the head of the buffer, but we still
    // want to make sure there is a null character at the head
    err = strbuilder_moveHead(builder, 0);
    if(err != BAKE_SUCCESS)
        return err;

    return BAKE_SUCCESS;
}


BakeError strbuilder_appendFormat(StringBuilder * builder, const char * format, ...) {
    // Get the variable arguments
    va_list arguments;
    va_start(arguments, format);

    // Get the available characters in the string builder
    size_t available = strbuilder_available(builder);

    // Write the formatted string into the buffer.
    // (available + 1) to account for null character that strbuilder_available does not include.
    int length = vsnprintf(strbuilder_head(builder), available + 1, format, arguments);

    // If there was an error formatting the string
    if(length < 0) {
        reportError("An error occurred while appending formatted string: %s\n", strerror(errno));
        va_end(arguments);
        return BAKE_ERROR_UNKNOWN;
    }

    // If we didn't have enough space
    if(length > available) {
        // Grow the StringBuilder to fit the formatted string
        BakeError err = strbuilder_growToFit(builder, (size_t) length);
        if(err != BAKE_SUCCESS) {
            va_end(arguments);
            return err;
        }

        // Try formatting the string again
        available = strbuilder_available(builder);
        length = vsnprintf(strbuilder_head(builder), available + 1, format, arguments);

        // If there was an error formatting the string
        if(length < 0) {
            reportError("An error occurred while appending formatted string: %s\n", strerror(errno));
            va_end(arguments);
            return BAKE_ERROR_UNKNOWN;
        }

        // If we still didn't have enough space, something went wrong
        if(length >= available) {
            reportError("An unknown error occurred while appending formatted string\n");
            va_end(arguments);
            return BAKE_ERROR_UNKNOWN;
        }
    }

    // Move the head of the builder to after the newly appended string
    BakeError err = strbuilder_moveHead(builder, (size_t) length);
    if(err != BAKE_SUCCESS) {
        va_end(arguments);
        return err;
    }

    // Success!
    va_end(arguments);
    return BAKE_SUCCESS;
}
