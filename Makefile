# ===================================================
# Makefile for Genetic Algorithm Rescue Operations
# ===================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -g -O2
LDFLAGS = -lm -lrt -pthread

# Directories
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = .
CONFIG_DIR = config
OUTPUT_DIR = output

# Target executable
TARGET = $(BIN_DIR)/rescue_ga

# Source files
SOURCES = main.c \
          utilities.c \
          grid_environment.c \
          path_generator.c \
          fitness.c \
          genetic_operators.c \
          multiprocess.c

# Object files
OBJECTS = $(OBJ_DIR)/main.o \
          $(OBJ_DIR)/utilities.o \
          $(OBJ_DIR)/grid_environment.o \
          $(OBJ_DIR)/path_generator.o \
          $(OBJ_DIR)/fitness.o \
          $(OBJ_DIR)/genetic_operators.o \
          $(OBJ_DIR)/multiprocess.o

# Header files
HEADERS = utilities.h \
          grid_environment.h \
          path_generator.h \
          fitness.h \
          genetic_operators.h \
          multiprocess.h

# Default target
all: directories $(TARGET)
	@echo ""
	@echo "========================================="
	@echo "  Build Complete!"
	@echo "========================================="
	@echo "Executable: $(TARGET)"
	@echo "Run with: ./$(TARGET) config/config.txt"
	@echo ""

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OUTPUT_DIR)
	@mkdir -p $(CONFIG_DIR)
	@echo "✓ Directories verified"

# Link object files to create executable
$(TARGET): $(OBJECTS)
	@echo "Linking object files..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Linking complete"

# Compile source files to object files
$(OBJ_DIR)/main.o: main.c $(HEADERS)
	@echo "Compiling main.c..."
	$(CC) $(CFLAGS) -c main.c -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/utilities.o: utilities.c utilities.h
	@echo "Compiling utilities.c..."
	$(CC) $(CFLAGS) -c utilities.c -o $(OBJ_DIR)/utilities.o

$(OBJ_DIR)/grid_environment.o: grid_environment.c grid_environment.h utilities.h
	@echo "Compiling grid_environment.c..."
	$(CC) $(CFLAGS) -c grid_environment.c -o $(OBJ_DIR)/grid_environment.o

$(OBJ_DIR)/path_generator.o: path_generator.c path_generator.h grid_environment.h utilities.h
	@echo "Compiling path_generator.c..."
	$(CC) $(CFLAGS) -c path_generator.c -o $(OBJ_DIR)/path_generator.o

$(OBJ_DIR)/fitness.o: fitness.c fitness.h path_generator.h grid_environment.h utilities.h
	@echo "Compiling fitness.c..."
	$(CC) $(CFLAGS) -c fitness.c -o $(OBJ_DIR)/fitness.o

$(OBJ_DIR)/genetic_operators.o: genetic_operators.c genetic_operators.h path_generator.h grid_environment.h utilities.h
	@echo "Compiling genetic_operators.c..."
	$(CC) $(CFLAGS) -c genetic_operators.c -o $(OBJ_DIR)/genetic_operators.o

$(OBJ_DIR)/multiprocess.o: multiprocess.c multiprocess.h utilities.h path_generator.h grid_environment.h fitness.h
	@echo "Compiling multiprocess.c..."
	$(CC) $(CFLAGS) -c multiprocess.c -o $(OBJ_DIR)/multiprocess.o

# Debug build with symbols
debug: CFLAGS += -g -DDEBUG -O0
debug: clean all
	@echo "✓ Debug build complete (use with gdb)"

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG
release: clean all
	@echo "✓ Release build complete (optimized)"

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(OBJ_DIR)
	@rm -f $(TARGET)
	@echo "✓ Build files cleaned"

# Clean everything including outputs
cleanall: clean
	@echo "Cleaning all generated files..."
	@rm -rf $(OUTPUT_DIR)
	@echo "✓ All files cleaned"

# Run with default config
run: $(TARGET)
	@echo ""
	@echo "========================================="
	@echo "  Running Rescue GA"
	@echo "========================================="
	@echo ""
	@if [ -f "$(CONFIG_DIR)/config.txt" ]; then \
		./$(TARGET) $(CONFIG_DIR)/config.txt; \
	else \
		echo "Warning: No config file found, using defaults"; \
		./$(TARGET); \
	fi

# Run with valgrind for memory leak detection
valgrind: $(TARGET)
	@echo "Running with Valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET) $(CONFIG_DIR)/config.txt

# Run with gdb debugger
gdb: debug
	@echo "Starting GDB debugger..."
	gdb --args ./$(TARGET) $(CONFIG_DIR)/config.txt

# Check for compilation errors without running
check: all
	@echo "✓ Compilation check passed"

# Display help
help:
	@echo "========================================="
	@echo "  Genetic Algorithm Rescue Operations"
	@echo "  Available Make Targets"
	@echo "========================================="
	@echo ""
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debug symbols (-g)"
	@echo "  release   - Build with optimizations (-O3)"
	@echo "  clean     - Remove build files"
	@echo "  cleanall  - Remove all generated files"
	@echo "  run       - Build and run with config"
	@echo "  valgrind  - Run with memory leak detection"
	@echo "  gdb       - Run with GDB debugger"
	@echo "  check     - Compile without running"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Build the project"
	@echo "  make run          # Build and run"
	@echo "  make debug        # Debug build"
	@echo "  make clean all    # Clean rebuild"
	@echo ""

# Declare phony targets
.PHONY: all directories debug release clean cleanall run valgrind gdb check help
