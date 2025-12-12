#include "utilities.h"

// ===== Configuration Functions =====

// Load configuration from file
Config* load_config(const char* filename) {
    Config* config = (Config*)safe_malloc(sizeof(Config));
    FILE* file = fopen(filename, "r");
    
    if (!file) {
        warning("Config file not found. Using default values.");
        free(config);
        return create_default_config();
    }
    
    // Initialize with defaults first
    Config* defaults = create_default_config();
    memcpy(config, defaults, sizeof(Config));
    free(defaults);
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // Parse key-value pairs
        char key[64], value[64];
        if (sscanf(line, "%[^=]=%s", key, value) == 2) {
            // Grid settings
            if (strcmp(key, "GRID_X") == 0) config->grid_x = atoi(value);
            else if (strcmp(key, "GRID_Y") == 0) config->grid_y = atoi(value);
            else if (strcmp(key, "GRID_Z") == 0) config->grid_z = atoi(value);
            else if (strcmp(key, "NUM_SURVIVORS") == 0) config->num_survivors = atoi(value);
            else if (strcmp(key, "OBSTACLE_PERCENT") == 0) config->obstacle_percent = atoi(value);
            else if (strcmp(key, "START_X") == 0) config->start_pos.x = atoi(value);
            else if (strcmp(key, "START_Y") == 0) config->start_pos.y = atoi(value);
            else if (strcmp(key, "START_Z") == 0) config->start_pos.z = atoi(value);
            
            // GA parameters
            else if (strcmp(key, "POPULATION_SIZE") == 0) config->population_size = atoi(value);
            else if (strcmp(key, "MAX_GENERATIONS") == 0) config->max_generations = atoi(value);
            else if (strcmp(key, "MUTATION_RATE") == 0) config->mutation_rate = atof(value);
            else if (strcmp(key, "CROSSOVER_RATE") == 0) config->crossover_rate = atof(value);
            else if (strcmp(key, "ELITISM_PERCENT") == 0) config->elitism_percent = atoi(value);
            else if (strcmp(key, "TOURNAMENT_SIZE") == 0) config->tournament_size = atoi(value);
            
            // Fitness weights
            else if (strcmp(key, "W1_SURVIVORS") == 0) config->w1_survivors = atof(value);
            else if (strcmp(key, "W2_COVERAGE") == 0) config->w2_coverage = atof(value);
            else if (strcmp(key, "W3_LENGTH") == 0) config->w3_length = atof(value);
            else if (strcmp(key, "W4_RISK") == 0) config->w4_risk = atof(value);
            
            // Multi-processing
            else if (strcmp(key, "NUM_WORKERS") == 0) config->num_workers = atoi(value);
            
            // Termination
            else if (strcmp(key, "STAGNATION_LIMIT") == 0) config->stagnation_limit = atoi(value);
            else if (strcmp(key, "TIME_LIMIT") == 0) config->time_limit = atoi(value);
            
            // Output
            else if (strcmp(key, "VERBOSE") == 0) config->verbose = atoi(value);
            else if (strcmp(key, "SAVE_STATS") == 0) config->save_stats = atoi(value);
        }
    }
    
    fclose(file);
    return config;
}

// Create default configuration
Config* create_default_config() {
    Config* config = (Config*)safe_malloc(sizeof(Config));
    
    // Grid settings
    config->grid_x = 15;
    config->grid_y = 15;
    config->grid_z = 4;
    config->num_survivors = 8;
    config->obstacle_percent = 25;
    config->start_pos = create_coordinate(0, 0, 0);
    
    // GA parameters
    config->population_size = 50;
    config->max_generations = 100;
    config->mutation_rate = 0.05;
    config->crossover_rate = 0.85;
    config->elitism_percent = 10;
    config->tournament_size = 5;
    
    // Fitness weights
    config->w1_survivors = 15.0;
    config->w2_coverage = 8.0;
    config->w3_length = 1.5;
    config->w4_risk = 3.0;
    
    // Multi-processing
    config->num_workers = 4;
    
    // Termination
    config->stagnation_limit = 20;
    config->time_limit = 300;
    
    // Output
    config->verbose = 1;
    config->save_stats = 1;
    
    return config;
}

// Validate configuration
int validate_config(Config* config) {
    int valid = 1;
    
    printf("Validating configuration...\n");
    
    // Grid validation
    if (config->grid_x <= 0 || config->grid_x > 100) {
        fprintf(stderr, "ERROR: grid_x must be between 1 and 100\n");
        valid = 0;
    }
    if (config->grid_y <= 0 || config->grid_y > 100) {
        fprintf(stderr, "ERROR: grid_y must be between 1 and 100\n");
        valid = 0;
    }
    if (config->grid_z <= 0 || config->grid_z > 20) {
        fprintf(stderr, "ERROR: grid_z must be between 1 and 20\n");
        valid = 0;
    }
    
    // Survivors validation
    if (config->num_survivors <= 0 || config->num_survivors > MAX_SURVIVORS) {
        fprintf(stderr, "ERROR: num_survivors must be between 1 and %d\n", MAX_SURVIVORS);
        valid = 0;
    }
    
    // Check if grid can fit survivors
    int max_possible = config->grid_x * config->grid_y * config->grid_z;
    if (config->num_survivors >= max_possible) {
        fprintf(stderr, "ERROR: Too many survivors for grid size\n");
        valid = 0;
    }
    
    // Obstacle percentage
    if (config->obstacle_percent < 0 || config->obstacle_percent > 80) {
        fprintf(stderr, "WARNING: obstacle_percent should be 0-80, resetting to 25\n");
        config->obstacle_percent = 25;
    }
    
    // Start position
    if (config->start_pos.x < 0 || config->start_pos.x >= config->grid_x ||
        config->start_pos.y < 0 || config->start_pos.y >= config->grid_y ||
        config->start_pos.z < 0 || config->start_pos.z >= config->grid_z) {
        fprintf(stderr, "WARNING: start_pos out of bounds, using (0,0,0)\n");
        config->start_pos = create_coordinate(0, 0, 0);
    }
    
    // GA parameters
    if (config->population_size <= 0 || config->population_size > MAX_POPULATION) {
        fprintf(stderr, "ERROR: population_size must be between 1 and %d\n", MAX_POPULATION);
        valid = 0;
    }
    
    if (config->max_generations <= 0 || config->max_generations > 10000) {
        fprintf(stderr, "ERROR: max_generations must be between 1 and 10000\n");
        valid = 0;
    }
    
    if (config->mutation_rate < 0.0 || config->mutation_rate > 1.0) {
        fprintf(stderr, "WARNING: mutation_rate must be 0.0-1.0, resetting to 0.05\n");
        config->mutation_rate = 0.05;
    }
    
    if (config->crossover_rate < 0.0 || config->crossover_rate > 1.0) {
        fprintf(stderr, "WARNING: crossover_rate must be 0.0-1.0, resetting to 0.85\n");
        config->crossover_rate = 0.85;
    }
    
    if (config->elitism_percent < 0 || config->elitism_percent > 50) {
        fprintf(stderr, "WARNING: elitism_percent should be 0-50, resetting to 10\n");
        config->elitism_percent = 10;
    }
    
    if (config->tournament_size <= 0 || config->tournament_size > config->population_size) {
        fprintf(stderr, "ERROR: tournament_size must be between 1 and population_size\n");
        valid = 0;
    }
    
    // Fitness weights validation
    if (config->w1_survivors < 0.0) {
        fprintf(stderr, "WARNING: w1_survivors should be positive\n");
        config->w1_survivors = 15.0;
    }
    if (config->w2_coverage < 0.0) {
        fprintf(stderr, "WARNING: w2_coverage should be positive\n");
        config->w2_coverage = 8.0;
    }
    if (config->w3_length < 0.0) {
        fprintf(stderr, "WARNING: w3_length should be positive\n");
        config->w3_length = 1.5;
    }
    if (config->w4_risk < 0.0) {
        fprintf(stderr, "WARNING: w4_risk should be positive\n");
        config->w4_risk = 3.0;
    }
    
    // Worker validation
    if (config->num_workers <= 0 || config->num_workers > 16) {
        fprintf(stderr, "WARNING: num_workers must be 1-16, using 4\n");
        config->num_workers = 4;
    }
    
    // Termination criteria
    if (config->stagnation_limit <= 0) {
        fprintf(stderr, "WARNING: stagnation_limit must be positive, using 20\n");
        config->stagnation_limit = 20;
    }
    
    if (config->time_limit < 0) {
        fprintf(stderr, "WARNING: time_limit must be non-negative, using 300\n");
        config->time_limit = 300;
    }
    
    if (valid) {
        printf("✓ Configuration validated successfully\n\n");
    } else {
        fprintf(stderr, "\n✗ Configuration validation FAILED\n\n");
    }
    
    return valid;
}

// Print configuration
void print_config(const Config* config) {
    printf("\n========== Configuration ==========\n");
    printf("Grid Dimensions: %dx%dx%d\n", config->grid_x, config->grid_y, config->grid_z);
    printf("Number of Survivors: %d\n", config->num_survivors);
    printf("Obstacle Percentage: %d%%\n", config->obstacle_percent);
    printf("Start Position: (%d, %d, %d)\n", 
           config->start_pos.x, config->start_pos.y, config->start_pos.z);
    printf("\nGA Parameters:\n");
    printf("  Population Size: %d\n", config->population_size);
    printf("  Max Generations: %d\n", config->max_generations);
    printf("  Mutation Rate: %.3f\n", config->mutation_rate);
    printf("  Crossover Rate: %.3f\n", config->crossover_rate);
    printf("  Elitism: %d%%\n", config->elitism_percent);
    printf("  Tournament Size: %d\n", config->tournament_size);
    printf("\nFitness Weights:\n");
    printf("  W1 (Survivors): %.2f\n", config->w1_survivors);
    printf("  W2 (Coverage): %.2f\n", config->w2_coverage);
    printf("  W3 (Length): %.2f\n", config->w3_length);
    printf("  W4 (Risk): %.2f\n", config->w4_risk);
    printf("\nWorker Processes: %d\n", config->num_workers);
    printf("===================================\n\n");
}

// Free configuration
void free_config(Config* config) {
    if (config) {
        free(config);
    }
}

// ===== Utility Functions =====

// Generate random integer in range [min, max]
int random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Generate random float in range [min, max]
float random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

// Get current time in milliseconds
double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// Print progress bar
void print_progress_bar(int current, int total, const char* label) {
    int bar_width = 50;
    float progress = (float)current / total;
    int filled = (int)(bar_width * progress);
    
    printf("\r%s [", label);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("=");
        else if (i == filled) printf(">");
        else printf(" ");
    }
    printf("] %d/%d (%.1f%%)", current, total, progress * 100);
    fflush(stdout);
    
    if (current == total) printf("\n");
}

// ===== Coordinate Functions =====

// Create a coordinate
Coordinate create_coordinate(int x, int y, int z) {
    Coordinate c;
    c.x = x;
    c.y = y;
    c.z = z;
    return c;
}

// Check if two coordinates are equal
int coordinates_equal(Coordinate c1, Coordinate c2) {
    return (c1.x == c2.x && c1.y == c2.y && c1.z == c2.z);
}

// Calculate Euclidean distance between two coordinates
float distance_3d(Coordinate c1, Coordinate c2) {
    int dx = c1.x - c2.x;
    int dy = c1.y - c2.y;
    int dz = c1.z - c2.z;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

// Calculate Manhattan distance
int manhattan_distance(Coordinate c1, Coordinate c2) {
    return abs(c1.x - c2.x) + abs(c1.y - c2.y) + abs(c1.z - c2.z);
}

// ===== Memory Management =====

// Safe malloc with error checking
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        error_exit("Memory allocation failed");
    }
    return ptr;
}

// Safe calloc with error checking
void* safe_calloc(size_t count, size_t size) {
    void* ptr = calloc(count, size);
    if (!ptr) {
        error_exit("Memory allocation failed");
    }
    return ptr;
}

// ===== Error Handling =====

// Print error and exit
void error_exit(const char* message) {
    fprintf(stderr, "ERROR: %s\n", message);
    if (errno != 0) {
        perror("System error");
    }
    exit(EXIT_FAILURE);
}

// Print warning
void warning(const char* message) {
    fprintf(stderr, "WARNING: %s\n", message);
}
