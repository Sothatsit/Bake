/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
 */

#include "parser.h"
#include "files.h"


BakeError parse_allocate(ParseContext * out) {
    // First line is line 1
    out->nextLineNumber = 1;

    // Allocate space for storing the values of variables as we parse
    BakeError err = strmap_allocate(&out->variables, 4);
    if(err != BAKE_SUCCESS)
        return err;

    // Allocate a buffer for temporary use
    err = strbuilder_allocate(&out->tempBuffer, 64);
    if(err != BAKE_SUCCESS) {
        strmap_free(&out->variables);
        return err;
    }

    // Allocate a buffer to read each line into
    err = strbuilder_allocate(&out->readBuffer, 64);
    if(err != BAKE_SUCCESS) {
        strmap_free(&out->variables);
        strbuilder_free(&out->tempBuffer);
        return err;
    }

    // Allocate a buffer to write each line with variables expanded into
    err = strbuilder_allocate(&out->expandBuffer, 64);
    if(err != BAKE_SUCCESS) {
        strmap_free(&out->variables);
        strbuilder_free(&out->tempBuffer);
        strbuilder_free(&out->readBuffer);
        return err;
    }

    return BAKE_SUCCESS;
}


void parse_free(ParseContext * context) {
    strmap_free(&context->variables);
    strbuilder_free(&context->tempBuffer);
    strbuilder_free(&context->readBuffer);
    strbuilder_free(&context->expandBuffer);
}


BakeError parse_nextLine(ParseContext * context, FILE * file) {
    // Reset all buffers to be re-used
    strbuilder_reset(&context->readBuffer);
    strbuilder_reset(&context->expandBuffer);

    // Read a whole line from the file into readBuffer
    return readWholeLine(file, &context->tempBuffer, &context->readBuffer, &context->nextLineNumber);
}


bool isComment(const char * line) {
    // A line is considered a comment if it starts with a #
    return line[0] == '#';
}


bool isEmptyLine(char * string) {
    // Loop over string until we find a non-whitespace character
    while(isspace(*string)) {
        string++;
    }

    // If we've found the end of the string, then the string is all whitespace
    return *string == '\0';
}


bool isIdentifierCharacter(const char ch) {
    /*
     * We want to stop parsing the identifier on any character we otherwise use for parsing.
     *
     * Characters:
     *   - space characters = a new word
     *   - equals sign      = a variable definition
     *   - colon            = a target definition
     *   - brackets         = variable expansions
     *   - null character   = end of line
     *
     * The dollar sign '$' is not a blacklisted character as only the pair "$(" is required for parsing,
     * and so not allowing '(' in variable names is sufficient. Also '$' makes more sense in variable names
     * than brackets.
     */
    return !isspace(ch) && ch != '=' && ch != ':' && ch != '(' && ch != ')';
}


char * trimLeadingWhitespace(char * line) {
    // Loop forward until we find a non-whitespace character
    while(isspace(*line)) {
        line++;
    }

    // We now know we've either found a non-whitespace character, or the end of the line
    return line;
}


BakeError parseIdentifier(char * line, char ** out) {
    // Keep increasing length until we find a non-identifier character
    size_t length = 0;
    while(isIdentifierCharacter(line[length])) {
        length += 1;
    }

    // If we found no identifier characters, an error occured
    if(length == 0) {
        reportError("Expected a variable/target name\n");
        return BAKE_ERROR_INVALID_IDENTIFIER;
    }

    // Allocate space for the identifier, and a null character
    *out = malloc(length + 1);
    if(*out == NULL) {
        reportError("Unable to allocate space for variable/target name: %s\n", strerror(errno));
        return BAKE_ERROR_MEMORY;
    }

    // Copy the parsed identifier into identifier, and place the null character
    memcpy(*out, line, length);
    (*out)[length] = '\0';

    return BAKE_SUCCESS;
}


BakeError appendCWD(StringBuilder * builder) {
    // Allocate a buffer we know can hold the maximum path size
    char cwd[PATH_MAX];

    // Attempt to find the current working directory
    char * res = getcwd(cwd, PATH_MAX);

    // Check that it was successful
    if(res == NULL) {
        reportError("An error occurred trying to get the current working directory: %s\n", strerror(errno));
        return BAKE_ERROR_UNKNOWN;
    }

    // Append the current working directory into builder
    return strbuilder_append(builder, res);
}


bool isReservedVariable(char * identifier) {
    if(strcmp("PID",  identifier) == 0) return true;
    if(strcmp("PPID", identifier) == 0) return true;
    if(strcmp("PWD",  identifier) == 0) return true;
    if(strcmp("RAND", identifier) == 0) return true;
    return false;
}


BakeError appendVariable(StringMap * variables, char * identifier, StringBuilder * builder) {
    if(strcmp("PID", identifier) == 0) {
        // Append the process's PID formatted as a string into builder
        return strbuilder_appendFormat(builder, "%i", getpid());
    }

    if(strcmp("PPID", identifier) == 0) {
        // Append the process's PPID formatted as a string into builder
        return strbuilder_appendFormat(builder, "%i", getppid());
    }

    if(strcmp("PWD", identifier) == 0) {
        // Append the current working directory into builder
        return appendCWD(builder);
    }

    if(strcmp("RAND", identifier) == 0) {
        // Append a random integer formatted as a string into builder
        return strbuilder_appendFormat(builder, "%i", rand());
    }

    // Try find the value from the variables map
    char * value = strmap_get(variables, identifier);
    if(value != NULL) {
        // Append the variable's value into builder
        return strbuilder_append(builder, value);
    }

    // Try find the value from the environment
    value = getenv(identifier);
    if(value != NULL) {
        // Append the environment value into builder
        return strbuilder_append(builder, value);
    }

    // Or if all else fails, append nothing
    return BAKE_SUCCESS;
}


BakeError expandVariables(StringMap * variables, char * line, StringBuilder * builder) {
    // Keep track of the last place we found characters we want to copy directly
    size_t copyFrom = 0;

    // Keep track of the current position we are looking at in line
    size_t position = 0;

    // Loop through the line looking for variable expansions
    char ch, next;
    while((ch = line[position++]) != '\0') {
        next = line[position];

        // The start of a variable expansion
        if(ch == '$' && next == '(') {
            // Copy everything before this variable expansion into builder
            strbuilder_appendSubstring(builder, &line[copyFrom], position - copyFrom - 1);

            // Move up past the opening bracket
            position++;

            // Parse the identifier of this variable
            char * identifier;
            BakeError err = parseIdentifier(&line[position], &identifier);
            if(err != BAKE_SUCCESS) {
                reportError(" .. while parsing a variable expansion\n");
                return err;
            }

            // Move up past the variable, and make sure there's the closing bracket
            position += strlen(identifier);
            if(line[position++] != ')') {
                free(identifier); // Still need to free this guy
                reportError("Expected closing bracket ')' while parsing variable expansion '%s'\n", identifier);
                return BAKE_ERROR_PARSING;
            }

            // Set the new position to copy characters directly from
            copyFrom = position;

            // Append the value of the variable into builder
            appendVariable(variables, identifier, builder);

            // Free the identifier of the variable
            free(identifier);
        }
    }

    // Append the trailing characters after the last variable expansion
    strbuilder_appendSubstring(builder, &line[copyFrom], position - copyFrom);

    return BAKE_SUCCESS;
}


BakeError parseVariable(ParseContext * context, StringMap * variables, char * identifier, char * restOfLine) {
    // If the variable identifier matches a reserved variable name, error
    if(isReservedVariable(identifier)) {
        reportError("Cannot assign value to the reserved variable '%s'\n", identifier);
        return BAKE_ERROR_PARSING;
    }

    // Skip all leading whitespace characters
    restOfLine = trimLeadingWhitespace(restOfLine);

    // Copy the value of the variable so we can put it in the map
    char * value = strdup(restOfLine);
    if(value == NULL) {
        reportError("Unable to copy value of variable: %s\n", strerror(errno));
        return BAKE_ERROR_MEMORY;
    }

    // Variable definitions shouldn't appear in the middle of a target's action lines
    context->activeTarget = NULL;

    // Put the variable in the variables map
    return strmap_put(variables, identifier, value);
}


BakeError parseTarget(ParseContext * context, Bakefile * bakefile, char * identifier, char * restOfLine) {
    // Allocate space for storing the output target when we add it to bakefile
    Target * target = malloc(sizeof(Target));
    if(target == NULL) {
        reportError("Unable to allocate space for target '%s': %s\n", identifier, strerror(errno));
        return BAKE_ERROR_MEMORY;
    }

    // Allocate the targets resources for storing its dependency and action lines
    BakeError err = target_allocate(identifier, target);
    if(err != BAKE_SUCCESS) {
        free(target);
        return err;
    }

    // Loop through the rest of the line to find all of this targets dependencies
    while(true) {
        restOfLine = trimLeadingWhitespace(restOfLine);

        size_t length = 0;
        char ch;
        while((ch = restOfLine[length]) != '\0' && !isspace(ch)) {
            length += 1;
        }

        if(length == 0)
            break;

        char * dependency = malloc(length + 1);
        if(dependency == NULL) {
            reportError("Unable to allocate memory for dependency: %s\n", strerror(errno));
            target_free(target);
            free(target);
            return BAKE_ERROR_MEMORY;
        }

        memcpy(dependency, restOfLine, length);
        dependency[length] = '\0';

        restOfLine = &restOfLine[length];

        err = target_addDependency(target, dependency);
        if(err) {
            target_free(target);
            free(target);
            return err;
        }
    }

    // Set this target as the new active target of the parser
    context->activeTarget = target;

    // Add the target to the bakefile
    return bakefile_addTarget(bakefile, identifier, target);
}


BakeError parseIdentifierLine(ParseContext * context, Bakefile * bakefile, StringMap * variables, char * line) {
    // Parse an identifier from the start of the line
    char * identifier;
    BakeError err = parseIdentifier(line, &identifier);
    if(err != BAKE_SUCCESS)
        return err;

    // Move past the identifier characters
    line = &line[strlen(identifier)];

    // Skip all whitespace characters
    line = trimLeadingWhitespace(line);

    // The first non-whitespace character after the identifier will
    // tell us whether this is a variable definition or a target definition
    char ch = *(line++);

    switch (ch) {
        /**
         * An '=' after an identifier signifies a variable definition.
         */
        case '=': {
            // Parse the variable, and add it to bakefile
            err = parseVariable(context, variables, identifier, line);

            // If we didn't succeed, we want to free the identifier
            if(err != BAKE_SUCCESS) {
                free(identifier);
            }

            return err;
        }

        /**
         * A ':' after an identifier signifies the start of a target definition.
         */
        case ':': {
            // Parse the target, and add it to bakefile
            err = parseTarget(context, bakefile, identifier, line);

            // If we didn't succeed, we want to free the identifier
            if(err != BAKE_SUCCESS) {
                free(identifier);
            }

            return err;
        }

        /**
         * Otherwise something went wrong.
         */
        default: {
            reportError("Expected '=' or ':' after variable/target name\n");
            return BAKE_ERROR_PARSING;
        }
    }
}


BakeError parseActionLine(ParseContext * context, char * line) {
    // Action lines must start with a tab character
    if(line[0] != '\t') {
        reportError("Expected tab character for start of action line\n");
        return BAKE_ERROR_PARSING;
    }

    // We must still have a parsed target definition to add this action line to
    if(context->activeTarget == NULL) {
        reportError("Expected to have found a target definition before this action line\n");
        return BAKE_ERROR_PARSING;
    }

    // Move past the tab character
    line++;

    // Start constructing the action line!
    ActionLine actionLine;
    actionLine.skipPrinting = false;
    actionLine.requireSuccess = true;

    // Look for special cases for this action line
    switch (*line) {
        /**
         * An @ symbol at the start of an action line signifies that
         * we shouldn't print the command before executing it
         */
        case '@':
            actionLine.skipPrinting = true;

            // Move past the @ character
            line++;
            break;

        /**
         * A '-' character at the start of an action line signifies
         * that we should ignore if this action line errored.
         */
        case '-':
            actionLine.requireSuccess = false;

            // Move past the '-' character
            line++;
            break;

        /**
         * Or just use the default options.
         */
        default:
            break;
    }

    // Duplicate the ActionLine's command, so we can store it for later.
    actionLine.command = strdup(line);

    // Add the action line to the active target
    return target_addActionLine(context->activeTarget, actionLine);
}


BakeError parseBakefile(BakeOptions options, FILE * file, Bakefile * bakefile) {
    // Stores whether parsing was a success
    BakeError err;

    // Stores the line number of the current line being parsed
    size_t currentLineNumber;

    // Allocate the output bakefile
    err = bakefile_allocate(bakefile);
    if(err != BAKE_SUCCESS)
        return err;

    // Allocate space for storing the values of variables as we parse
    ParseContext context;
    err = parse_allocate(&context);
    if(err != BAKE_SUCCESS) {
        bakefile_free(bakefile);
        return err;
    }

    // Loop through all the lines in file
    while(true) {
        // Get the line number of the line we're about to parse
        currentLineNumber = context.nextLineNumber;

        // Read the next line in the file
        err = parse_nextLine(&context, file);
        if(err != BAKE_SUCCESS) {
            // End of file isn't an error, but does mean we should stop parsing
            if(err == BAKE_ERROR_EOF) {
                err = BAKE_SUCCESS;
            }
            break;
        }

        // Get the line that was read into the readBuffer
        char * line = strbuilder_get(&context.readBuffer);

        // If its a comment or empty line, skip it
        if(isComment(line) || isEmptyLine(line))
            continue;

        // Expand the variables in line into the ParseContext's expandBuffer
        err = expandVariables(&context.variables, line, &context.expandBuffer);
        if(err != BAKE_SUCCESS)
            break;

        // Update line to new variable expanded version
        line = strbuilder_get(&context.expandBuffer);

        // If the line starts with an identifier character, parse it as a variable or target definition
        if(isIdentifierCharacter(line[0])) {
            err = parseIdentifierLine(&context, bakefile, &context.variables, line);
            if(err != BAKE_SUCCESS)
                break;

            // Move on to the next line
            continue;
        }

        // If its not a comment, empty, variable or target line, then it must be an action line
        err = parseActionLine(&context, line);
        if(err != BAKE_SUCCESS)
            break;
    }

    // If we hit an error, print the line number we encountered it on, and free the parsed bakefile
    if(err != BAKE_SUCCESS) {
        reportError(" .. while parsing line %li of the bakefile\n", currentLineNumber);
        bakefile_free(bakefile);
    }

    // Free all our temporary allocated resources
    parse_free(&context);

    // Finished!
    return err;
}
