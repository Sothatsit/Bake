# Bake
Bake is a command-line utility that implements a subset of the features of GNU Make. This project was written for the CITS2002 UWA unit in 2018.

Information on **writing bakefiles**, **building bake** and **running bake** can be found below, along with an example Bakefile.

## Example Bakefile

    # The command we want to use to compile our binaries
    C99 = cc -std=c99 -Wall -pedantic -Werror

    # Where our source and build files are located
    SRC = src
    BUILD = build

    # The target and source files of the program
    OBJECTS = $(BUILD)/buffer.o $(BUILD)/main.o
    SOURCES = $(SRC)/buffer.h $(SRC)/buffer.c \
              $(SRC)/main.h $(SRC)/main.c

    #
    # Compile all sources into the $(BUILD) folder
    #
    bake: $(OBJECTS) $(SOURCES)
    	$(C99)  -o $(BUILD)/buffer.o  -c $(SRC)/buffer.c
    	$(C99)  -o $(BUILD)/main.o    -c $(SRC)/main.c

    	$(C99)  -o bake  $(OBJECTS)
    	@echo " === PROGRAM BUILT === "

## Variables
Variables can be used to **define a value once**, which you can then re-use later in your Bakefile.

**Defining a variable:**

    <IDENTIFIER> = <VALUE>

**Using a variable:**

    $(<IDENTIFIER>)

## Targets
Targets are used to represent **files built** by your Bakefile.

The modification time of the target is found as the modification time of \<FILE\>. If the file does not exist on disk, then the target will always be ran.

**Defining a target:**

    <FILE> : <DEPENDENCY-1 [DEPENDENCY-2 ...]>


## Dependencies
Dependencies define **when** targets should be rebuilt.

If a target has no dependencies, it will always be rebuilt.

**File Dependencies:**
Files that when modified, signal that this target needs to be rebuilt.

    <FILE-NAME>

**URL Dependencies:**
URLs that when modified, signal that this target needs to be rebuilt. The modification date of URLs is found from their **Last-Modified** header.

    http://<URL> or https://<URL> or file://<FILE>

**Target Dependencies:**
Targets that when rebuilt, signal that this target should also be rebuilt.

    <TARGET-FILE>

## Action Lines
The **commands** that are ran to rebuild the target. Modifiers can be placed before the commands to adjust how they are executed.

**Available Modifiers:**
- **'@'** = Do not print the commands before executing them.
- **' - '** = Ignore whether the command was successful, and carry on as usual.

Each action line should follow a target definition, and be prefixed by a tab character.

    my_target : file1 file2
    	-rm -f out.o
    	$(C99) -o out.o  -c in.c
    	@echo " === Success === "

## Comments
Empty lines, or lines starting with **'#'** will be ignored during parsing.

    # This is a comment.

    # ^ and that empty line's valid too.

# Building Bake
The GNU **make** command can be used to build Bake:

    cd Bake
    make

Or, once you have a version of **bake** compiled:

    cd Bake
    ./bake

# Bake's Command-Line Options
There are several options that can be specified when executing bake.

- **-C \<dir\>** = Change the directory to **\<dir\>** before commencing execution.

- **-f \<file\>** = Execute **\<file\>** instead of the default **Bakefile** or **bakefile**.

- **-i** = Run all commands ignoring whether they were successful.

- **-n** = Print all commands that would have been executed, without actually executing them.

- **-p** = Print out the parsed bakefile with all variables expanded.

- **-s** = Do not print the commands before they are executed.

An optional **target parameter** can also be specified after all options, which dictates the target to be ran. If no target is specified, the first target in the file will be ran.

    ./bake [OPTIONS] [TARGET]
