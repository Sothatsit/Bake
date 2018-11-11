/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === targets ===
//
// Contains methods for storing, constructing and
// iterating build targets and their action lines.
//

#ifndef CITS2002_TARGETS_H
#define CITS2002_TARGETS_H

#include "buffer.h"
#include "stringmap.h"


/**
 * An executable action line from a build target.
 */
typedef struct {
    /**
     * Whether we should skip printing this action line to console before we execute it.
     */
    bool skipPrinting;

    /**
     * Whether we should require that this action line succeed before we execute later action lines.
     */
    bool requireSuccess;

    /**
     * The command associated with this action line.
     */
     char * command;
} ActionLine;


/**
 * Used to mark whether a target has not been executed, is being executed, or has been executed.
 */
typedef enum {
    /**
     * The target has not been executed.
     */
    TARGET_NOT_EXECUTED,

    /**
     * The target is currently being executed.
     */
    TARGET_EXECUTING,

    /**
     * The target has already been executed.
     */
    TARGET_EXECUTED,

    /**
     * The target was skipped.
     */
    TARGET_SKIPPED
} TargetState;


/**
 * A build target from a Bakefile.
 */
typedef struct {
    /**
     * The name of this target.
     */
    char * name;

    /**
     * A buffer containing a list of char * dependency strings.
     */
    Buffer dependencies;

    /**
     * A buffer containing a list of ActionLine's associated with this target.
     */
     Buffer actionLines;

    /**
     * The execution state of this target.
     */
    TargetState state;
} Target;


/**
 * Contains all information about a parsed bakefile.
 */
typedef struct {
    /**
     * The first target we encountered in the bakefile.
     *
     * This will be free'd when targets is free'd, and should not be free'd separately.
     */
    Target * firstTarget;

    /**
     * A map containing each Target that is parsed.
     */
    StringMap targets;
} Bakefile;


/**
 * Allocate the new Target with the name {@param name}, and place it in {@param out}.
 *
 * {@param name} will not be free'd when this target is free'd, and should be free'd separately.
 */
BakeError target_allocate(char * name, Target * out);


/**
 * Free all the resources of {@param target} and mark it as invalid.
 */
void target_free(Target * target);


/**
 * @return a pointer to the array of dependency strings of {@param target}
 */
char ** target_getDependencies(Target * target);


/**
 * @return the number of dependencies {@param target} has
 */
size_t target_dependencyCount(Target * target);


/**
 * Add the dependency {@param dependency} to {@param target}.
 */
BakeError target_addDependency(Target * target, char * dependency);


/**
 * @return a pointer to the array of action lines of {@param target}
 */
ActionLine * target_getActionLines(Target * target);


/**
 * @return the number of action lines within {@param target}
 */
size_t target_actionLineCount(Target * target);


/**
 * Add the action line {@param actionLine} to {@param target}.
 */
BakeError target_addActionLine(Target * target, ActionLine actionLine);


/**
 * Allocate a new Bakefile for use in parsing a bakefile, and place it in {@param out}.
 */
BakeError bakefile_allocate(Bakefile * out);


/**
 * Free all resources of {@param bakefile} and mark it as invalid.
 */
void bakefile_free(Bakefile * bakefile);


/**
 * Get the Target identified by {@param identifier} stored in {@param bakefile}.
 */
Target * bakefile_getTarget(Bakefile * bakefile, char * identifier);


/**
 * Add the Target {@param target} identified by {@param identifier} to {@param bakefile}.
 *
 * {@param target} must have a unique name.
 * {@param identifier} must not ever change after this call.
 *
 * {@param identifier} and {@param target} will be free'd when {@param bakefile} is free'd.
 */
BakeError bakefile_addTarget(Bakefile * bakefile, char * identifier, Target * target);


#endif //CITS2002_TARGETS_H
