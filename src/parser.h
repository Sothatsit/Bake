/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === parser ===
//
// In comes a string we humans can understand, out goes something the computer can.
//

#ifndef CITS2002_PARSER_H
#define CITS2002_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <limits.h>
#include <zconf.h>
#include "stringbuilder.h"
#include "stringmap.h"
#include "main.h"
#include "targets.h"


/**
 * Stores temporary data used to parse a bakefile.
 */
typedef struct {
    /**
     * The line number of the next line to be parsed.
     */
    size_t nextLineNumber;

    /**
     * The target that action lines should be added to as they are parsed.
     */
    Target * activeTarget;

    /**
     * A temporary buffer that can be used during parsing.
     */
    StringBuilder tempBuffer;

    /**
     * Lines read from the file are stored in this buffer.
     */
    StringBuilder readBuffer;

    /**
     * Is used to expand the variables in lines that are read from the file.
     */
    StringBuilder expandBuffer;

    /**
     * Is used to store the values of variables throughout the parsing of the file.
     */
    StringMap variables;
} ParseContext;


/**
 * Allocate a new ParseContext and place it in {@param out}.
 */
BakeError parse_allocate(ParseContext * out);


/**
 * Free the resources of {@param context} and mark it as invalid.
 */
void parse_free(ParseContext * context);


/**
 * Reset the buffers, increment the line number, and read
 * a whole line from {@param file}, storing it into the
 * readBuffer of the parse context {@param context}.
 */
BakeError parse_nextLine(ParseContext * context, FILE * file);


/**
 * @return whether {@param line} is a comment line; i.e. if line starts with #
 */
bool isComment(const char * line);


/**
 * @return whether {@param string} is only whitespace characters
 */
bool isEmptyLine(char * string);


/**
 * @return whether {@param ch} is a valid identifier character;
 *         i.e. if its alphanumeric or an underscore
 */
bool isIdentifierCharacter(char ch);


/**
 * @return a pointer to the start of the non-whitespace characters in {@param line}, or the end of the line
 */
char * trimLeadingWhitespace(char * line);


/**
 * Parse an identifier from {@param line} into a newly allocated
 * string, and place the pointer to the string into {@param out}.
 */
BakeError parseIdentifier(char * line, char ** out);


/**
 * Get the current working directory and place it in {@param builder}.
 */
BakeError appendCWD(StringBuilder * builder);


/**
 * Reserved variables:
 * - PID
 * - PPID
 * - PWD
 * - RAND
 *
 * @return whether {@param identifier} matches the name of a reserved variable.
 */
bool isReservedVariable(char * identifier);


/**
 * Finds the value of the variable identified by {@param identifier} and places it in {@param builder}.
 *
 * Special cases:
 * - PID  = the pid of this process
 * - PPID = the pid of this process's parent
 * - PWD  = the current working directory
 * - RAND = a random integer
 *
 * If {@param identifier} does not match any of these special cases, its value will then
 * be searched for in {@param variables}, followed by from the environment variables.
 * Finally, if no value is found nothing will be appended.
 */
BakeError appendVariable(StringMap * variables, char * identifier, StringBuilder * builder);


/**
 * Finds all variable expansions within {@param line} of the form $(<identifier>), and replaces
 * them with their values.
 *
 * The line with all variables expanded is placed into {@param builder}.
 */
BakeError expandVariables(StringMap * variables, char * line, StringBuilder * builder);


/**
 * Parse and add the value of the variable {@param identifier} to {@param variables}.
 *
 * The value of the variable is {@param restOfLine} with leading whitespace removed.
 */
BakeError parseVariable(ParseContext * context, StringMap * variables, char * identifier, char * restOfLine);


/**
 * Parse and add the target identified by {@param identifier} to {@param bakefile}.
 *
 * {@param restOfLine} should contain a whitespace delimited array of target dependencies
 */
BakeError parseTarget(ParseContext * context, Bakefile * bakefile, char * identifier, char * restOfLine);


/**
 * Parse a line beginning with an identifier, which could signify
 * either a variable definition or the start of a target defintion.
 */
BakeError parseIdentifierLine(ParseContext * context, Bakefile * bakefile, StringMap * variables, char * line);


/**
 * Parse the action line {@param line} and add it to the active target in {@param context}.
 */
BakeError parseActionLine(ParseContext * context, char * line);


/**
 * Parse the bakefile from {@param file}, and put the result in {@param bakefile}.
 */
BakeError parseBakefile(BakeOptions options, FILE * file, Bakefile * bakefile);


#endif //CITS2002_PARSER_H
