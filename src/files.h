/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === files ===
//
// Methods for managing the files accessed by Bake.
//

#ifndef CITS2002_FILES_H
#define CITS2002_FILES_H

#include <stdio.h>
#include <sys/stat.h>
#include "main.h"
#include "stringbuilder.h"


/**
 * The index of the read end of the pipe from the file descriptor array given by a pipe call.
 */
#define PIPE_READ  0


/**
 * The index of the write end of the pipe from the file descriptor array given by a pipe call.
 */
#define PIPE_WRITE  1


/**
 * Read all the contents from {@param pipeReadFD} and write it into {@param output}.
 */
BakeError readPipeContents(int pipeReadFD, StringBuilder * output);


/**
 * Get the modification time of the file {@param file} and place it into {@param mtimeNS}.
 *
 * If the file does not exist, -1 will be placed into {@param mtimeNS}.
 */
BakeError getFileModificationTime(char * file, time_t * modTime);


/**
 * Find the value of the "Last-Modified" header from the URL response
 * {@param responseHeader}, and place its value into {@param modTime}.
 */
BakeError findLastModifiedHeader(char * url, char * responseHeader, time_t * modTime);


/**
 * Get the modification time of the file found at the URL {@param url} and place it into {@param mtimeNS}.
 */
BakeError getURLModificationTime(char * url, time_t * modTime);


/**
 * Open the file specified by {@param options}.bakefile, or if no option was supplied try
 * to open Bakefile or bakefile, and place the opened file pointer into {@param out}.
 */
BakeError openBakefile(BakeOptions options, FILE ** out);


/**
 * Read a single line from {@param file}, and places it into {@param builder}.
 *
 * Does not append the newline sequence from the line.
 */
BakeError readSingleLine(FILE * file, StringBuilder * builder);


/**
 * Reads a whole line from {@param file}, places it into {@param builder}, and increments
 * {@param lineNumber} by the number of newline separated lines we encounter during reading.
 *
 * {@param tempBuilder} is used as a temporary buffer for use internally in the method.
 * It will be reset before use.
 *
 * Lines are considered as any sequence of characters up to the first newline character
 * which is not preceded by a backwards slash '\', or until the EOF.
 *
 * Whitespace before and afer the line continuation '\' will be removed, leaving a single space.
 */
BakeError readWholeLine(FILE * file, StringBuilder * tempBuilder, StringBuilder * builder, size_t * lineNumber);


#endif //CITS2002_FILES_H
