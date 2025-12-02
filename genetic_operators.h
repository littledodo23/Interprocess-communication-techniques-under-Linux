#ifndef GENETIC_OPERATORS_H
#define GENETIC_OPERATORS_H

#include "utilities.h"
#include "grid_environment.h"
#include "path_generator.h"

// ===== Selection =====

// Tournament selection - select one parent
Path* tournament_selection(Path** population, int pop_size, int tournament_size);

// Select parents for reproduction
void select_parents(Path** population, int pop_size, const Config* config,
                   Path** parent1, Path** parent2);

// ===== Crossover =====

// Single-point crossover
Path* crossover_single_point(const Path* parent1, const Path* parent2, const Grid* grid);

// Two-point crossover
Path* crossover_two_point(const Path* parent1, const Path* parent2, const Grid* grid);

// Uniform crossover
Path* crossover_uniform(const Path* parent1, const Path* parent2, const Grid* grid);

// Main crossover function (uses config to decide which type)
Path* crossover(const Path* parent1, const Path* parent2, 
                const Grid* grid, const Config* config);

// ===== Mutation =====

// Point mutation - change single coordinate
void mutate_point(Path* path, const Grid* grid);

// Swap mutation - swap two coordinates
void mutate_swap(Path* path);

// Insert mutation - move a segment to different position
void mutate_insert(Path* path);

// Scramble mutation - shuffle a subsection
void mutate_scramble(Path* path);

// Main mutation function
void mutate(Path* path, const Grid* grid, const Config* config);

// ===== Elitism =====

// Get elite paths (top N%)
Path** get_elite_paths(Path** population, int pop_size, 
                       int elitism_count, int* elite_size);

// Copy elite to new population
void preserve_elite(Path** old_population, Path** new_population,
                   int pop_size, int elitism_count);

// ===== Generation Management =====

// Create next generation
Path** create_next_generation(Path** current_population, int pop_size,
                             const Grid* grid, const Config* config);

// Replace population with new generation
void replace_population(Path*** population, Path** new_generation, int pop_size);

// ===== Validation =====

// Repair invalid path
void repair_path(Path* path, const Grid* grid);

// Check if path is feasible
int is_feasible_path(const Path* path, const Grid* grid);

#endif // GENETIC_OPERATORS_H
