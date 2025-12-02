#ifndef GENETIC_OPERATORS_H
#define GENETIC_OPERATORS_H

#include "grid_environment.h"
#include "path_generator.h"
#include "utilities.h"

// ===== Selection =====
Path *tournament_selection(Path **population, int pop_size,
                           int tournament_size);
Path **select_parents(Path **population, int pop_size, const Config *config,
                      int num_parents);

// ===== Crossover =====
Path **crossover_paths(const Path *parent1, const Path *parent2,
                       const Grid *grid);
Path *single_point_crossover(const Path *parent1, const Path *parent2,
                              const Grid *grid);
Path *two_point_crossover(const Path *parent1, const Path *parent2,
                           const Grid *grid);
Path *uniform_crossover(const Path *parent1, const Path *parent2,
                        const Grid *grid);

// ===== Mutation =====
void mutate_path(Path *path, const Grid *grid, float mutation_rate);
void mutate_insert_random(Path *path, const Grid *grid);
void mutate_swap_segments(Path *path);
void mutate_remove_loop(Path *path);
void mutate_reverse_segment(Path *path);

// ===== Elitism =====
Path **apply_elitism(Path **population, int pop_size, int elitism_count);

// ===== Population Management =====
Path **create_next_generation(Path **current_pop, int pop_size,
                              const Grid *grid, const Config *config);

#endif // GENETIC_OPERATORS_H
