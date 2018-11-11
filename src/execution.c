/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
*/

#include <zconf.h>
#include "execution.h"
#include "files.h"


BakeError executeCommand(char * command, StringBuilder * output, int * exitStatus) {
    int pipeFDs[2]; // Stores the file descriptors if we open a pipe to retrieve the stdout output
    int err;        // Stores any errors that could occur in this method

    // If we are storing the output, create a pipe so we can get the output of the command
    if(output != NULL) {
        err = pipe(pipeFDs);
        if(err != 0) {
            reportError("Unable to create pipe to retrieve output of command %s: %s\n", command, strerror(errno));
            return BAKE_ERROR_IO;
        }
    }

    // Fork the process
    pid_t childPID = fork();

    // If there was an error forking the process
    if(childPID < 0) {
        reportError("Unable to fork process to execute command %s: %s\n", command, strerror(errno));
        return BAKE_ERROR_EXECUTION;
    }

    // This is the child process
    if(childPID == 0) {
        // If we are storing the output, redirect stdout of this child process to the pipe we created
        if(output != NULL) {
            // We don't need the READ end of the pipe on the child end, so close it.
            close(pipeFDs[PIPE_READ]);

            // Make stdout write to the pipe we created
            err = dup2(pipeFDs[PIPE_WRITE], fileno(stdout));
            if(err < 0) {
                reportError("Unable to point stdout to the created pipe: %s\n", strerror(errno));
                return BAKE_ERROR_IO;
            }

            // stdout now points to the write end of our pipe, so we
            // can close the duplicated write end of the pipe as well
            close(pipeFDs[PIPE_WRITE]);
        }

        // Execute the command!
        execl("/bin/bash", "/bin/bash", "-c", command, (char *) NULL);

        // execl should have taken over control of this process, reaching this point is an error
        fprintf(stderr, "Control flow reached passed execl call: %s\n", strerror(errno));
        return BAKE_ERROR_EXECUTION;
    }

    // If we are storing the output, read it from the pipe and place it into output
    if(output != NULL) {
        // We don't need the WRITE end of the pipe on the parent end, so close it.
        close(pipeFDs[PIPE_WRITE]);

        // Read the contents of the pipe into output
        BakeError bakeErr = readPipeContents(pipeFDs[PIPE_READ], output);

        // Close the READ end of the pipe
        close(pipeFDs[PIPE_READ]);

        // Check if there was an error reading the contents of the pipe
        if(bakeErr != BAKE_SUCCESS) {
            reportError("Error reading command output...\n");
            return bakeErr;
        }
    }

    // Wait for the child process to complete
    int completeStatus;
    pid_t completedPID = wait(&completeStatus);

    // Check that the right process completed
    if(completedPID != childPID) {
        reportError("Unknown child process %i completed while waiting for process %i to complete\n",
                    completedPID, childPID);
        return BAKE_ERROR_EXECUTION;
    }

    // Check that the child process exited normally, and that it completed successfully
    *exitStatus = WEXITSTATUS(completeStatus);

    // We successfully ran the command! Although, the command may not have been successful itself.
    return BAKE_SUCCESS;
}


BakeError executeTargetDependency(BakeOptions options, Bakefile bakefile, Target * target, bool * executed) {
    // Stores any errors that may occur
    BakeError err;

    switch (target->state) {
        /**
         * If the target has not been executed, or if the target
         * is currently executing, then we want to call executeTarget.
         */
        case TARGET_NOT_EXECUTED:
            // Try execute the target
            err = executeTarget(options, bakefile, target);
            if(err != BAKE_SUCCESS)
                return err;

            // If the target was executed, then we should also execute this target
            if(target->state == TARGET_EXECUTED) {
                *executed = true;
            }

            break;


        /**
         * If we're trying to execute a target that is currently
         * being executed, then we've found a circular dependency.
         */
        case TARGET_EXECUTING:
            reportError("Found circular dependency when attempting to execute target %s\n", target->name);
            return BAKE_ERROR_EXECUTION;

        /**
         * If the target dependency has already been updated,
         * then we want to update this target also.
         */
        case TARGET_EXECUTED:
            *executed = true;
            break;

        /**
         * If the target dependency has been skipped,
         * we don't want to update this target.
         */
        case TARGET_SKIPPED:
            break;

        /**
         * If the target is of a state that we don't recognise, its an error.
         */
        default:
            reportError("Unknown target state %i when executing target dependency %s\n",
                        target->state, target->name);
            return BAKE_ERROR_UNKNOWN;

    }

    return BAKE_SUCCESS;
}


BakeError executeDependencies(BakeOptions options, Bakefile bakefile, Target * target,
                              long targetModificationTime, bool * dependenciesUpdated) {

    // Stores any errors that occur during this method
    BakeError err;

    // Loop through, and execute or check the modification time of all the target's dependencies
    size_t dependencyCount = target_dependencyCount(target);
    char ** dependencies = target_getDependencies(target);

    for(size_t index = 0; index < dependencyCount; ++index) {
        // Get the index'th dependency of this target
        char * dependencyName = dependencies[index];

        // Check if this dependency is a target
        Target * dependencyTarget = bakefile_getTarget(&bakefile, dependencyName);
        if(dependencyTarget != NULL) {
            // Try execute the dependency
            executeTargetDependency(options, bakefile, dependencyTarget, dependenciesUpdated);

            // Move on to the next dependency
            continue;
        }

        // Variable to store the modification time of this dependency
        time_t dependencyModificationTime;

        // Check if this dependency is a URL
        if(strncmp(dependencyName, "file://", strlen("file://")) == 0
           || strncmp(dependencyName, "http://", strlen("http://")) == 0
           || strncmp(dependencyName, "https://", strlen("https://")) == 0) {

            // If it is, get the last time this URL was modified
            err = getURLModificationTime(dependencyName, &dependencyModificationTime);
            if(err != BAKE_SUCCESS)
                return err;
        } else {
            // If its not a target or a URL, then it must be a file dependency. Get the last
            // modification time of this file dependency to check if we need to re-execute.
            err = getFileModificationTime(dependencyName, &dependencyModificationTime);
            if(err != BAKE_SUCCESS)
                return err;

            // If the dependency file doesn't exist, we should execute this target
            if(dependencyModificationTime == -1) {
                *dependenciesUpdated = true;
                continue;
            }
        }

        // Or if the dependency was modified more recently than this target, we should also execute
        if(dependencyModificationTime > targetModificationTime) {
            *dependenciesUpdated = true;
            continue;
        }
    }

    return BAKE_SUCCESS;
}


BakeError executeActionLines(BakeOptions options, Target * target) {
    BakeError err;

    // Get the action lines from the target
    size_t actionCount = target_actionLineCount(target);
    ActionLine * actions = target_getActionLines(target);

    for(size_t index = 0; index < actionCount; ++index) {
        // Get the index'th ActionLine from actions
        ActionLine action = actions[index];

        // If we haven't been passed the silent option, we want to print the command
        if((!options.silent && !action.skipPrinting) || options.onlyPrintCommands) {
            printf("%s\n", action.command);
        }

        // If we only want to print the commands, don't execute it
        if(options.onlyPrintCommands)
            continue;

        // Execute the command of the action
        int exitStatus;
        err = executeCommand(action.command, NULL, &exitStatus);
        if(err != BAKE_SUCCESS)
            return err;

        // Check if the command was successful, and whether we care
        if(exitStatus != EXIT_SUCCESS && options.requireSuccess && action.requireSuccess) {
            reportError("Command \"%s\" failed, aborting execution...\n", action.command);
            return BAKE_ERROR_EXECUTION;
        }
    }

    return BAKE_SUCCESS;
}


BakeError executeTarget(BakeOptions options, Bakefile bakefile, Target * target) {
    // The target has to have not already been executed
    if(target->state != TARGET_NOT_EXECUTED) {
        reportError("Target cannot be executed if it has already been executed\n");
        return BAKE_ERROR_EXECUTION;
    }

    // Mark that this target is being executed
    target->state = TARGET_EXECUTING;

    // Get the modification time of this target
    time_t targetModificationTime;
    BakeError err = getFileModificationTime(target->name, &targetModificationTime);
    if(err != BAKE_SUCCESS)
        return err;

    // If we couldn't find the target on disk, then we should execute this target to create it
    bool shouldExecute = (targetModificationTime == -1 || target_dependencyCount(target) == 0);

    // Execute the dependencies of this target
    err = executeDependencies(options, bakefile, target, targetModificationTime, &shouldExecute);
    if(err != BAKE_SUCCESS)
        return err;

    // If none of the dependencies have been updated, then we don't need to execute this target
    if(!shouldExecute) {
        // Mark that we have skipped this target
        target->state = TARGET_SKIPPED;
        return BAKE_SUCCESS;
    }

    // Mark the target as having been executed
    target->state = TARGET_EXECUTED;

    // Execute all the actions associated with this target
    err = executeActionLines(options, target);
    if(err != BAKE_SUCCESS)
        return err;

    return BAKE_SUCCESS;
}
