#ifndef FITNESS_H
#define FITNESS_H

#include "grid_environment.h"
#include "path_generator.h"
#include "utilities.h"

// ===== Fitness Calculation =====

// Main fitness function with normalization
float calculate_fitness(Path *path, const Grid *grid, const Config *config);

// Component calculations
int calculate_survivors_reached(const Path *path, const Grid *grid);
float calculate_coverage_area(const Path *path, const Grid *grid);
float calculate_path_risk(const Path *path, const Grid *grid);

// Normalization functions (NEW)
float normalize_survivors(int survivors, int max_survivors);
float normalize_coverage(float coverage);
float normalize_length(int length, int max_length);
float normalize_risk(float risk, float max_expected_risk);

// Helper functions
float normalize_fitness_component(float value, float min_val, float max_val);
void update_path_fitness(Path *path, const Grid *grid, const Config *config);
void update_population_fitness(Path **population, int pop_size,
                               const Grid *grid, const Config *config);

// Statistics
void print_fitness_statistics(Path **population, int pop_size);
float get_average_fitness(Path **population, int pop_size);
float get_best_fitness(Path **population, int pop_size);
float get_worst_fitness(Path **population, int pop_size);

#endif // FITNESS_H
