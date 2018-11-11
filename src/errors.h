/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === errors ===
//
// Defines methods of reporting errors that occur during the running of Bake.
//

#ifndef CITS2002_ERRORS_H
#define CITS2002_ERRORS_H


/**
 * Report an error that occurred during the operating of the program to the user.
 *
 * Wrapped in ifndef to allow reportError to be overridden if different error reporting is desired.
 */
#ifndef reportError
#include <stdio.h>
#define reportError(...) fprintf(stderr, __VA_ARGS__)
#endif // reportError


/**
 * Used to indicate any errors that occurred during a method.
 */
typedef enum {
    /**
     * If the method was successful!
     */
    BAKE_SUCCESS,

    /**
     * If we've hit the end of the file.
     */
    BAKE_ERROR_EOF,

    /**
     * An error that occurred to do with loading/saving files or loading URLs.
     */
    BAKE_ERROR_IO,

    /**
     * Was passed data of a format that was not expected.
     */
    BAKE_ERROR_FORMAT,

    /**
     * If we had a problem allocating memory.
     */
    BAKE_ERROR_MEMORY,

    /**
     * If we were passed something incorrect.
     */
    BAKE_ERROR_ARGUMENTS,

    /**
     * If we were unable to parse an identifier.
     */
     BAKE_ERROR_INVALID_IDENTIFIER,

    /**
     * Found an invalidly formatted line.
     */
    BAKE_ERROR_PARSING,

    /**
     * An error occurred during execution of the bakefile.
     */
    BAKE_ERROR_EXECUTION,

    /**
     * An error occurred while attempting to print a bakefile.
     */
    BAKE_ERROR_PRINT,

    /**
     * If an unknown error occurred.
     */
    BAKE_ERROR_UNKNOWN
} BakeError;


#endif //CITS2002_ERRORS_H
