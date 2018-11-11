/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */

//
// === stringbuilder ===
//
// An implementation of a utility to allow construction of variable length strings.
// Maintains the null character '\0' at the end of its internal buffer at all times.
//

#ifndef CITS2002_STRINGBUILDER_H
#define CITS2002_STRINGBUILDER_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <stdbool.h>
#include "buffer.h"


/**
 * A dynamically growing buffer for constructing strings.
 */
typedef struct {
    /**
     * A buffer containing the string data as it is built.
     */
    Buffer buffer;
} StringBuilder;


/**
 * Allocate a StringBuilder with the initial capacity {@param capacity} and place it into {@param out}.
 */
BakeError strbuilder_allocate(StringBuilder * out, size_t capacity);


/**
 * Free the memory of {@param builder} and mark it as invalid.
 */
void strbuilder_free(StringBuilder * builder);


/**
 * Reset {@param builder} to build a new string.
 */
void strbuilder_reset(StringBuilder * builder);


/**
 * @return a pointer to the constructed string. The returned string
 *         will change as the contents of {@param builder} change.
 */
char * strbuilder_get(StringBuilder *builder);


/**
 * @return a pointer to the head of the StringBuilder; where new characters should be written.
 */
char * strbuilder_head(StringBuilder * builder);


/**
 * @return the number of characters available to be written into in {@param builder}.
 *         (not including the null character)
 */
size_t strbuilder_available(StringBuilder * builder);


/**
 * If needed, grow {@param builder} to fit another {@param required} characters.
 *
 * Will double the size of the builder until it fits all the extra characters.
 */
BakeError strbuilder_growToFit(StringBuilder * builder, size_t required);


/**
 * Moves the head of {@param builder} by {@param moveBy} characters,
 * placing a null character after the {@param moveBy} characters.
 */
BakeError strbuilder_moveHead(StringBuilder * builder, size_t moveBy);


/**
 * Append the null-terminated string {@param string} into {@param builder}.
 */
BakeError strbuilder_append(StringBuilder * builder, char * string);


/**
 * Append {@param length} characters from {@param string} into {@param builder}.
 */
BakeError strbuilder_appendSubstring(StringBuilder * builder, char * string, size_t length);


/**
 * Append the formatted string specified by {@param format} into {@param builder}.
 */
BakeError strbuilder_appendFormat(StringBuilder * builder, const char * format, ...);


#endif //CITS2002_STRINGBUILDER_H
