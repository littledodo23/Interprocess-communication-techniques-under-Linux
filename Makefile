# Makefile for Genetic Algorithm Rescue Operations

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm -lrt
DEBUG_FLAGS = -g -DDEBUG

# Source files
SOURCES = main.c utilities.c grid_environment.c path_generator.c \
          fitness.c genetic_operators.c multiprocess.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Executable name
TARGET = rescue_ga

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build successful: $(TARGET)"

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "✓ Debug build complete"

# Create necessary directories
dirs:
	@mkdir -p output config

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "✓ Cleaned build files"

# Clean everything including output
distclean: clean
	rm -rf output/*.txt output/*.csv
	@echo "✓ Cleaned all generated files"

# Run the program
run: $(TARGET) dirs
	./$(TARGET) config/config.txt

# Run with default config
run-default: $(TARGET) dirs
	./$(TARGET)

# Dependencies
main.o: main.c utilities.h grid_environment.h path_generator.h \
        fitness.h genetic_operators.h multiprocess.h

utilities.o: utilities.c utilities.h

grid_environment.o: grid_environment.c grid_environment.h utilities.h

path_generator.o: path_generator.c path_generator.h grid_environment.h utilities.h

fitness.o: fitness.c fitness.h grid_environment.h path_generator.h utilities.h

genetic_operators.o: genetic_operators.c genetic_operators.h \
                     grid_environment.h path_generator.h utilities.h

multiprocess.o: multiprocess.c multiprocess.h utilities.h \
                path_generator.h grid_environment.h fitness.h

.PHONY: all clean distclean run run-default debug dirs
