/* CITS2002 Project 2018
   Name(s):              Padraig Lamont
   Student number(s):    22494652
 */


//
// === execution ===
//
// Defines the methods of executing targets, their dependencies
// and the commands associated with their action lines.
//

#ifndef CITS2002_EXECUTION_H
#define CITS2002_EXECUTION_H

#include "errors.h"
#include "stringbuilder.h"
#include "main.h"
#include "targets.h"


/**
 * Execute the command {@param command}, storing its stdout output into
 * {@param output} and its exit status into {@param exitStatus};
 */
BakeError executeCommand(char * command, StringBuilder * output, int * exitStatus);


/**
 * Run the target {@param target} if it has not already been executed.
 *
 * If {@param target} has been executed (in this call or previously), true will be placed into {@param executed}.
 */
BakeError executeTargetDependency(BakeOptions options, Bakefile bakefile, Target * target, bool * executed);


/**
 * Runs through all the dependencies of {@param target}, and performs the following:
 *
 *    Targets = If a target dependency has not already been executed, and it
 *              should be, execute it and set dependenciesUpdated to true.
 *
 *    Files & URLs = Find the modification date of the File/URL and compare it to {@param targetModificationTime}.
 *                   If it was modified more recently, set dependenciesUpdated to true.
 */
BakeError executeDependencies(BakeOptions options, Bakefile bakefile, Target * target,
                              long targetModificationTime, bool * dependenciesUpdated);


/**
 * Execute the commands of all the action lines in {@param target}.
 */
BakeError executeActionLines(BakeOptions options, Target * target);


/**
 * Execute the target {@param target}, and any targets that it has as dependencies.
 */
BakeError executeTarget(BakeOptions options, Bakefile bakefile, Target * target);


#endif //CITS2002_EXECUTION_H
