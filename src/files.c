/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
 */

#include <stdbool.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include "files.h"
#include "execution.h"


/**
 * The buffer size we want to use when we read data from a pipe.
 */
#define PIPE_BUFFER_SIZE  1024


BakeError readPipeContents(int pipeReadFD, StringBuilder * output) {
    // The buffer to use when reading data
    ssize_t charsRead;
    char buf[PIPE_BUFFER_SIZE];

    // Keep reading data from the pipe until an error or the EOF occurs
    while((charsRead = read(pipeReadFD, buf, sizeof(buf))) > 0) {
        strbuilder_appendSubstring(output, buf, (size_t) charsRead);
    }

    // If charsRead is 0, then the pipe was closed
    if(charsRead == 0)
        return BAKE_SUCCESS;

    // Otherwise, an error occurred
    reportError("There was an error reading contents of pipe: %s\n", strerror(errno));
    return BAKE_ERROR_IO;
}


BakeError getFileModificationTime(char * file, time_t * modTime) {
    // Get information about the file
    struct stat result;
    int err = stat(file, &result);
    if(err != 0) {
        // If the file doesn't exist, we want to set the time to -1
        if(errno == ENOENT) {
            errno = 0; // Clear the error
            *modTime = -1;
            return BAKE_SUCCESS;
        }

        reportError("Error getting modification time of file %s: %s\n", file, strerror(errno));
        return BAKE_ERROR_IO;
    }

    // Get the modification time of the file in nanoseconds
    *modTime = result.st_mtimespec.tv_sec;
    return BAKE_SUCCESS;
}


BakeError findLastModifiedHeader(char * url, char * responseHeader, time_t * modTime) {
    const char * lastModifiedPrefix = "Last-Modified: ";
    const size_t lastModifiedLength = strlen(lastModifiedPrefix);

    // Split the responseHeader line by line
    char * line = strtok(responseHeader, "\n");

    // Loop through all lines of the responseHeader
    do {
        // Find the length of the current line we're processing
        size_t lineLength = strlen(line);

        // Skip this line if it is empty
        if(lineLength == 0)
            continue;

        // Remove the carriage return if it is there
        if(line[lineLength - 1] == '\r') {
            line[lineLength - 1] = '\0';
            lineLength -= 1;
        }

        // Check if this line contains the value of the last-modified attribute
        if(strncmp(line, lastModifiedPrefix, lastModifiedLength) == 0) {
            // Extract the time string from the line
            char * date = &line[lastModifiedLength];

            // Extract the actual time from the date string
            struct tm time;
            char * excess = strptime(date, "%a, %d %b %Y %H:%M:%S %Z", &time);

            // If there was an error getting the time, or we did not consume the whole date, then error
            if(excess == NULL || *excess != '\0') {
                reportError("Unable to read time from Last-Modified property \"%s\": %s\n", date, strerror(errno));
                return BAKE_ERROR_FORMAT;
            }

            // Convert time to the number of seconds since the epoch, and store it into modTime
            *modTime = mktime(&time);

            // Check if the conversion was successful
            if(*modTime == -1) {
                reportError("There was an error converting time to seconds since the epoch: %s\n", strerror(errno));
                return BAKE_ERROR_UNKNOWN;
            }

            return BAKE_SUCCESS;
        }

        // Call with NULL uses state from first strtok call
    } while((line = strtok(NULL, "\n")) != NULL);

    reportError("Couldn't find Last-Modified property from header of URL %s\n", url);
    return BAKE_ERROR_FORMAT;
}


BakeError getURLModificationTime(char * url, time_t * modTime) {
    /*
     * The curl command we want to run.
     *
     * Options:
     *   --silent = Don't print progress of URL request
     *   --head   = Only read the header of the URL, not the body
     */
    char * curlCommandPrefix = "curl --silent --head ";

    // Allocate a builder to store the command we want to execute
    StringBuilder command;
    BakeError err = strbuilder_allocate(&command, strlen(curlCommandPrefix) + strlen(url));
    if(err != BAKE_SUCCESS)
        return err;

    // Construct the curl command from curlCommandPrefix and the url passed in
    err = strbuilder_appendFormat(&command, "%s%s", curlCommandPrefix, url);
    if(err != BAKE_SUCCESS) {
        strbuilder_free(&command);
        return err;
    }

    // Allocate a builder to store the output of the command, which should correspond to the header of the URL
    StringBuilder header;
    err = strbuilder_allocate(&header, PIPE_BUFFER_SIZE);
    if(err != BAKE_SUCCESS) {
        strbuilder_free(&command);
        return err;
    }

    // Execute the CURL command
    int exitStatus;
    err = executeCommand(strbuilder_get(&command), &header, &exitStatus);

    // Free the command, as we no longer need it
    strbuilder_free(&command);

    // Check if there was an error executing the CURL command
    if(err != BAKE_SUCCESS) {
        strbuilder_free(&header);
        return err;
    }

    // Check that the CURL command was successful
    if(exitStatus != EXIT_SUCCESS) {
        strbuilder_free(&header);
        reportError("There was an error getting modification time of URL %s: CURL exited with failure status %i\n",
                url, exitStatus);
        return BAKE_ERROR_EXECUTION;
    }

    // Find the modification time from the header
    err = findLastModifiedHeader(url, strbuilder_get(&header), modTime);

    // Free the header, as we no longer need it
    strbuilder_free(&header);

    return err;
}


BakeError openBakefile(BakeOptions options, FILE ** out) {
    // If a bakefile has been specified through the command-line, use it
    if(options.bakefile) {
        // Attempt to open the file
        *out = fopen(options.bakefile, "r");
        if(*out == NULL) {
            reportError("Couldn't open Bakefile at %s: %s\n", options.bakefile, strerror(errno));
            return BAKE_ERROR_IO;
        }

        return BAKE_SUCCESS;
    }

    // Otherwise try opening "Bakefile"
    *out = fopen("Bakefile", "r");

    // Or if that fails, try "bakefile"
    if(*out == NULL) {
        errno = 0; // Clear the error
        *out = fopen("bakefile", "r");
    }

    // If we couldn't open anything
    if(*out == NULL) {
        reportError("Couldn't open Bakefile: %s\n", strerror(errno));
        return BAKE_ERROR_IO;
    }

    return BAKE_SUCCESS;
}


/**
 * The maximum number of characters to read at once when reading a line from a file.
 */
#define LINE_READ_BUFFER_SIZE  1024


BakeError readSingleLine(FILE * file, StringBuilder * builder) {
    // Counts how many characters we've read in total
    size_t charactersRead = 0;

    // A buffer used to read data into.
    char readBuffer[LINE_READ_BUFFER_SIZE];

    // We may have to make more than one fgets call to read the whole line.
    while(true) {
        // Attempt to read a line from the file into readBuffer
        char * line = fgets(readBuffer, sizeof(readBuffer), file);
        if(line == NULL) {
            // Check if we've hit the end of the file
            if(feof(file)) {
                // If we've hit the EOF, but have read characters, this line is still valid
                if(charactersRead > 0)
                    return BAKE_SUCCESS;

                // Otherwise, we need to signify to the caller that we've reached EOF, and reading should stop
                return BAKE_ERROR_EOF;
            }

            // Check if there was an error with reading from the file
            if(ferror(file)) {
                reportError("An error occured reading a single line from the file: %s\n", strerror(errno));
                return BAKE_ERROR_IO;
            }

            // Otherwise something unexpected happened
            reportError("An unknown error occured reading a single line from the file\n");
            return BAKE_ERROR_IO;
        }

        // Find how many characters we were able to read
        size_t lineCharacters = strlen(line);

        // Update the total number of characters we've read from the file
        charactersRead += lineCharacters;

        // Whether we want to continue reading from the file
        bool continueReading = true;

        // If we've read until a new line in the file
        if(line[lineCharacters - 1] == '\n') {
            // Move before the newline (so it will not be appended to the output)
            lineCharacters -= 1;

            // If there is a carriage return, move before it (so it will not be appended to the output)
            if(lineCharacters > 0 && line[lineCharacters - 1] == '\r') {
                lineCharacters -= 1;
            }

            // Mark that we have read a whole line, and don't need to read any more characters
            continueReading = false;
        }

        // Append the line we've just read into builder
        BakeError err = strbuilder_appendSubstring(builder, line, lineCharacters);
        if(err != BAKE_SUCCESS)
            return err;

        // If we've done reading, return success!
        if(!continueReading)
            return BAKE_SUCCESS;
    }
}


BakeError readWholeLine(FILE * file, StringBuilder * lineBuilder, StringBuilder * builder, size_t * lineNumber) {
    // Counts the number of lines we've read
    size_t linesRead = 0;

    // We may have to read more than one line
    while(true) {
        // Reset the lineBuilder so we can read the next line
        strbuilder_reset(lineBuilder);

        // Read one line from the file into lineBuilder
        BakeError err = readSingleLine(file, lineBuilder);
        if(err != BAKE_SUCCESS) {
            strbuilder_free(lineBuilder);

            // If its the EOF, and we've read at least one line, then this is not an error
            if(err == BAKE_ERROR_EOF && linesRead > 0)
                return BAKE_SUCCESS;

            return err;
        }

        // Get the line we read from the file
        char * line = strbuilder_get(lineBuilder);
        size_t lineLength = strlen(line);

        // If this is a continuation line, we want to remove all leading whitespace
        if(linesRead > 0) {
            while(isspace(*line)) {
                line++;
                lineLength -= 1;
            }
        }

        // Marks whether this is the final line we want to read
        bool isFinalLine = true;

        // If this line ends with a '\', then we want to continue this line over to the next
        if(line[lineLength - 1] == '\\') {
            // Mark that this isn't the final line
            isFinalLine = false;

            // Reduce the length of the line, marking that we don't want the '\' in the outputted line
            lineLength -= 1;
        }

        // Remove all trailing whitespace from the line
        while(lineLength > 0 && isspace(line[lineLength - 1])) {
            lineLength -= 1;
        }

        // If this line isn't empty, append it to builder
        if(lineLength > 0) {
            err = strbuilder_appendSubstring(builder, line, lineLength);
            if(err != BAKE_SUCCESS) {
                strbuilder_free(lineBuilder);
                return err;
            }

            // If this isn't the final line, we want to append a space to separate this line and the next
            if(!isFinalLine) {
                err = strbuilder_append(builder, " ");
                if(err != BAKE_SUCCESS) {
                    strbuilder_free(lineBuilder);
                    return err;
                }
            }
        }

        // Mark that we've read another line
        linesRead += 1;
        *lineNumber += 1;

        // If this is the final line, return success
        if(isFinalLine)
            return BAKE_SUCCESS;
    }
}
