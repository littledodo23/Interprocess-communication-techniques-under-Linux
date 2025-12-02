#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

// ===== Constants =====
#define MAX_PATH_LENGTH 1000
#define MAX_LINE_LENGTH 256
#define MAX_SURVIVORS 50
#define CONFIG_FILE_DEFAULT "config/config.txt"
#define OUTPUT_FILE "output/results.txt"

// ===== Data Structures =====

// 3D Coordinate structure
typedef struct {
    int x;
    int y;
    int z;
} Coordinate;

// Cell types in the grid
typedef enum {
    CELL_EMPTY = 0,      // Empty space
    CELL_OBSTACLE = 1,   // Debris/obstacle
    CELL_SURVIVOR = 2,   // Survivor location
    CELL_START = 3       // Robot starting position
} CellType;

// Configuration parameters
typedef struct {
    // Grid settings
    int grid_x;
    int grid_y;
    int grid_z;
    int num_survivors;
    int obstacle_percent;
    Coordinate start_pos;
    
    // GA parameters
    int population_size;
    int max_generations;
    float mutation_rate;
    float crossover_rate;
    int elitism_percent;
    int tournament_size;
    
    // Fitness weights
    float w1_survivors;
    float w2_coverage;
    float w3_length;
    float w4_risk;
    
    // Multi-processing
    int num_workers;
    
    // Termination criteria
    int stagnation_limit;
    int time_limit;
    
    // Output settings
    int verbose;
    int save_stats;
} Config;

// ===== Function Prototypes =====

// Configuration functions
Config* load_config(const char* filename);
Config* create_default_config();
void print_config(const Config* config);
void free_config(Config* config);

// Utility functions
int random_int(int min, int max);
float random_float(float min, float max);
double get_time_ms();
void print_progress_bar(int current, int total, const char* label);

// Coordinate functions
Coordinate create_coordinate(int x, int y, int z);
int coordinates_equal(Coordinate c1, Coordinate c2);
float distance_3d(Coordinate c1, Coordinate c2);
int manhattan_distance(Coordinate c1, Coordinate c2);

// Memory management
void* safe_malloc(size_t size);
void* safe_calloc(size_t count, size_t size);

// Error handling
void error_exit(const char* message);
void warning(const char* message);

#endif // UTILITIES_H