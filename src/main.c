/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
 */

#include <printf.h>
#include <getopt.h>
#include <stdlib.h>
#include <memory.h>
#include "main.h"
#include "parser.h"
#include "execution.h"
#include "files.h"


/**
 * The entry point of the bake command-line program.
 */
int main(int argc, char * argv[]) {
    // Variables for storing success of function calls
    int err;
    BakeError bakeErr;

    // Read the command-line options from the user
    BakeOptions options;
    bakeErr = readCommandLineOptions(argc, argv, &options);
    if(bakeErr != BAKE_SUCCESS)
        return EXIT_FAILURE;

    // Change the directory if one was passed in
    if(options.directory) {
        err = chdir(options.directory);
        if(err != 0) {
            reportError("Couldn't change directory to %s: %s\n", options.directory, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    // Open the bakefile
    FILE * file;
    bakeErr = openBakefile(options, &file);
    if(bakeErr != BAKE_SUCCESS)
        return EXIT_FAILURE;

    // Parse the bakefile
    Bakefile bakefile;
    bakeErr = parseBakefile(options, file, &bakefile);

    // Close the file we parsed
    err = fclose(file);

    // Check for errors when parsing or closing the file, and exit if there were any
    if(bakeErr != BAKE_SUCCESS || err != 0) {
        // If there was no error parsing the bakefile, then we need to free it
        if(bakeErr == BAKE_SUCCESS) {
            bakefile_free(&bakefile);
        }

        // If there was an error closing the file, report it
        if(err != 0) {
            reportError("Unable to close Bakefile: %s\n", strerror(errno));
        }

        return EXIT_FAILURE;
    }

    // If the expandVariablesAndExit option is set, print the parsed bakefile, and skip its execution
    if(options.expandVariablesAndExit) {
        err = printBakefile(stdout, &bakefile);
        bakefile_free(&bakefile);

        // If there was an error printing the bakefile, exit as failure
        if(err != BAKE_SUCCESS)
            return EXIT_FAILURE;

        return EXIT_SUCCESS;
    }

    // Check that the file contained at least one target
    if(bakefile.firstTarget == NULL) {
        bakefile_free(&bakefile);
        reportError("The bakefile did not contain any targets\n");
        return EXIT_FAILURE;
    }

    // Find the target we want to execute
    Target * target = bakefile.firstTarget;

    // If a target was specified through the command-line, use it instead of the first target
    if(options.target != NULL) {
        // Try find the target specified
        target = bakefile_getTarget(&bakefile, options.target);
        if(target == NULL) {
            bakefile_free(&bakefile);
            reportError("Could not find the target %s to execute\n", options.target);
            return EXIT_FAILURE;
        }
    }

    // Execute the target
    bakeErr = executeTarget(options, bakefile, target);
    if(bakeErr != BAKE_SUCCESS) {
        bakefile_free(&bakefile);
        return EXIT_FAILURE;
    }

    // Free the resources we've allocated, then exit
    bakefile_free(&bakefile);

    return EXIT_SUCCESS;
}


BakeError readCommandLineOptions(int argc, char * argv[], BakeOptions * options) {
    // Set the default command-line options
    options->directory = NULL;
    options->bakefile = NULL;
    options->requireSuccess = true;
    options->onlyPrintCommands = false;
    options->expandVariablesAndExit = false;
    options->silent = false;
    options->target = NULL;

    // Read the command-line options
    int opt;
    const char * commandLineOptions = "inpsC:f:";
    while((opt = getopt(argc, argv, commandLineOptions)) != -1) {
        switch(opt) {
            /**
             * Option to change the working directory before execution
             */
            case 'C':
                options->directory = optarg;
                break;

            /**
             * Option to specify a file other than "Bakefile" or "bakefile" to load, parse and execute.
             */
            case 'f':
                options->bakefile = optarg;
                break;

            /**
             * Option to not require that every command be successful before moving on to the next command.
             */
            case 'i':
                options->requireSuccess = false;
                break;

            /**
             * Option to only print the commands that would be executed, and not execute them.
             */
            case 'n':
                options->onlyPrintCommands = true;
                break;

            /**
             * Option to expand all variables in the bakefile, print the resulting file, and skip executing anything.
             */
            case 'p':
                options->expandVariablesAndExit = true;
                break;

            /**
             * Option to not print the commands as they are executed.
             */
            case 's':
                options->silent = true;
                break;

            /**
             * If we found an unknown option, or we are missing a value for an option.
             */
            case '?':
                // If its a valid option and we got here, then musn't have been passed a value
                if(strchr(commandLineOptions, optopt) != NULL) {
                    reportError("Expected value for command-line option %c\n", optopt);
                    return BAKE_ERROR_ARGUMENTS;
                }

                // Otherwise its an unknown option
                reportError("Unknown command-line option %c\n", optopt);
                return BAKE_ERROR_ARGUMENTS;

            /**
             * This should never happen and indicates an error in the commandLineOptions constant.
             */
            default:
                reportError("Unexpectedly reached default case while reading command-line options\n");
                return BAKE_ERROR_UNKNOWN;
        }
    }

    // If there are more arguments after the command-line options, then a targetName may have been supplied
    if(optind < argc) {
        // If there was more than one extra argument, then the command was incorrect
        if(optind + 1 < argc) {
            reportError("Expected only a single target parameter");
            return BAKE_ERROR_ARGUMENTS;
        }

        // Otherwise the last element of argv is our target parameter
        options->target = argv[argc - 1];
    }

    return BAKE_SUCCESS;
}


BakeError printActionLine(FILE * file, ActionLine * actionLine) {
    // Check if we have more than one attribute set
    if(!actionLine->requireSuccess && actionLine->skipPrinting) {
        reportError("No representation for action line which doesn't require success AND is silent");
        return BAKE_ERROR_PRINT;
    }

    // Used to store any errors that may occur when printing
    int err;

    // Print out the '-' symbol to mark this action line as not requiring success
    if(!actionLine->requireSuccess) {
        err = fprintf(file, "-");
        if(err < 0) {
            reportError("Unable to print '-' action line symbol to file: %s\n", strerror(errno));
            return BAKE_ERROR_IO;
        }
    }

    // Print out the '@' symbol to mark this action line as silent
    if(actionLine->skipPrinting) {
        err = fprintf(file, "@");
        if(err < 0) {
            reportError("Unable to print '@' action line symbol to file: %s\n", strerror(errno));
            return BAKE_ERROR_IO;
        }
    }

    // Print the action line's command to file
    err = fprintf(file, "%s", actionLine->command);
    if(err < 0) {
        reportError("Unable to print action line's command to file: %s\n", strerror(errno));
        return BAKE_ERROR_IO;
    }

    return BAKE_SUCCESS;
}


BakeError printTarget(FILE * file, Target * target) {
    // Print the target's name
    int err = fprintf(file, "%s :", target->name);
    if(err < 0) {
        reportError("%i\n",err);
        reportError("Unable to print target's name to file: %s\n", strerror(errno));
        return BAKE_ERROR_IO;
    }

    // Print all the target's dependencies
    size_t dependencyCount = target_dependencyCount(target);
    char ** dependencies = target_getDependencies(target);

    for(size_t index = 0; index < dependencyCount; ++index) {
        // Get the index'th dependency
        char * dependency = dependencies[index];

        // Print out the dependency
        err = fprintf(file, " %s", dependency);
        if(err < 0) {
            reportError("Unable to print target's dependency to file: %s\n", strerror(errno));
            return BAKE_ERROR_IO;
        }
    }

    // Print all the target's action lines
    size_t actionCount = target_actionLineCount(target);
    ActionLine * actions = target_getActionLines(target);

    for(size_t index = 0; index < actionCount; ++index) {
        // Get the index'th action line
        ActionLine * action = &actions[index];

        // Print out a newline and tab before the next action line
        err = fprintf(file, "\n\t");
        if(err < 0) {
            reportError("Unable to print newline and tab before action line: %s\n", strerror(errno));
            return BAKE_ERROR_IO;
        }

        // Print out the action line
        BakeError bakeErr = printActionLine(file, action);
        if(bakeErr != BAKE_SUCCESS)
            return bakeErr;
    }

    // Print a newline after this target
    err = fprintf(file, "\n");
    if(err < 0) {
        reportError("Unable to print newline after target: %s\n", strerror(errno));
        return BAKE_ERROR_IO;
    }

    return BAKE_SUCCESS;
}


BakeError printBakefile(FILE * file, Bakefile * bakefile) {
    // Get the targets in bakefile
    size_t targetCount = strmap_size(&bakefile->targets);
    StringMapEntry * targetEntries = strmap_entries(&bakefile->targets);

    // Loop through all targets in bakefile
    for(size_t index = 0; index < targetCount; ++index) {
        // Get the index'th entry in the targets map
        StringMapEntry entry = targetEntries[index];

        // Print out the target associated with this entry
        BakeError err = printTarget(file, (Target *) entry.value);
        if(err != BAKE_SUCCESS)
            return err;
    }

    return BAKE_SUCCESS;
}
