# The command we want to use to compile our binariess
C99 = cc -std=c99 -Wall -pedantic -Werror

# Where our source files are located
SRC = src

# Where we want to put our built .o files
BUILD = build

# All the .o files built by this program
BINARIES = $(BUILD)/buffer.o $(BUILD)/stringbuilder.o $(BUILD)/stringmap.o  \
           $(BUILD)/files.o $(BUILD)/targets.o $(BUILD)/parser.o            \
           $(BUILD)/execution.o $(BUILD)/main.o

#
# Set up the directory structure and build the bake executable
#
build: setupDirectory bake
	@echo ""
	@echo "==============================="
	@echo "    BAKE BUILT SUCCESSFULLY    "
	@echo "==============================="


#
# Link all sources of bake into the final executable
#
bake: $(BINARIES)
	$(C99)  -o bake  $(BINARIES)


#
# Build each binary file
#
$(BUILD)/%.o: $(SRC)/%.c
	$(C99)  -o $@  -c $^


#
# Remove the built files so the whole program can be rebuilt
#
clean:
	rm -rf $(BUILD)


#
# Set up the directory structure for the built files
#
setupDirectory:
	mkdir -p build
