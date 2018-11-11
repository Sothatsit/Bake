/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === main ===
//
// Provides the command line interface of bake.
//

#ifndef CITS2002_MAIN_H
#define CITS2002_MAIN_H

#include <stdbool.h>
#include <stdio.h>
#include "errors.h"
#include "targets.h"


/**
 * The arguments supplied to the program.
 */
typedef struct {
    /**
     * A directory to change to before commencing execution of
     * the program, or NULL if no directory change is wanted.
     *
     * Default: NULL
     */
    char * directory;

    /**
     * The file to open and parse as the bakefile, or NULL if
     * the default "Bakefile" or "bakefile" is to be used.
     *
     * Default: NULL
     */
    char * bakefile;

    /**
     * Whether we want to require that action lines
     * succeed before executing the next action line.
     *
     * Default: TRUE
     */
    bool requireSuccess;

    /**
     * Whether we want to only print the commands that
     * we would execute, and not actually execute them.
     *
     * Default: FALSE
     */
    bool onlyPrintCommands;

    /**
     * Whether we want to just expand the variables in the bakefile,
     * print the resultant bakefile with all variables expanded,
     * and then exit without executing.
     *
     * Default: FALSE
     */
    bool expandVariablesAndExit;

    /**
     * Whether we want to not print the commands before we execute them.
     *
     * Default: FALSE
     */
    bool silent;

    /**
     * The target that we want to execute, or NULL if the
     * default first target in the bakefile is to be used.
     *
     * Default: NULL
     */
    char * target;
} BakeOptions;


/**
 * Read the command line options for bake from the array {@param argv}
 * of length {@param argc}, and place them into {@param options}.
 */
BakeError readCommandLineOptions(int argc, char * argv[], BakeOptions * options);


/**
 * Print out the action line {@param actionLine} to {@param file}.
 *
 * This does not print the tab character before the line.
 */
BakeError printActionLine(FILE * file, ActionLine * actionLine);


/**
 * Print out the target {@param target} to {@param file}.
 */
BakeError printTarget(FILE * file, Target * target);


/**
 * Print out the bakefile {@param bakefile} to {@param file}.
 *
 * Will not print any variables that may have
 * been found while parsing {@param bakefile},
 * as these are expanded away before this point.
 */
BakeError printBakefile(FILE * file, Bakefile * bakefile);


#endif //CITS2002_MAIN_H
