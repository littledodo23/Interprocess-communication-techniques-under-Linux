#include "genetic_operators.h"

// ===== Tournament Selection =====
Path* tournament_selection(Path** population, int pop_size, int tournament_size) {
    if (tournament_size > pop_size) {
        tournament_size = pop_size;
    }
    
    Path* best = NULL;
    float best_fitness = -1e9;
    
    for (int i = 0; i < tournament_size; i++) {
        int idx = random_int(0, pop_size - 1);
        if (population[idx]->fitness > best_fitness) {
            best_fitness = population[idx]->fitness;
            best = population[idx];
        }
    }
    
    return best;
}

Path** select_parents(Path** population, int pop_size, const Config* config, int num_parents) {
    Path** parents = (Path**)safe_malloc(num_parents * sizeof(Path*));
    
    for (int i = 0; i < num_parents; i++) {
        parents[i] = tournament_selection(population, pop_size, config->tournament_size);
    }
    
    return parents;
}

// ===== Single Point Crossover =====
Path* single_point_crossover(const Path* parent1, const Path* parent2, const Grid* grid) {
    if (!parent1 || !parent2 || parent1->length < 2 || parent2->length < 2) {
        return clone_path(parent1);
    }
    
    // Choose crossover point
    int point1 = random_int(1, parent1->length - 1);
    int point2 = random_int(1, parent2->length - 1);
    
    // Create child
    Path* child = create_path(parent1->length + parent2->length);
    
    // Copy first part from parent1
    for (int i = 0; i < point1; i++) {
        add_coordinate_to_path(child, parent1->coordinates[i]);
    }
    
    // Copy second part from parent2
    for (int i = point2; i < parent2->length; i++) {
        add_coordinate_to_path(child, parent2->coordinates[i]);
    }
    
    // Validate and fix if needed
    if (child->length == 0) {
        free_path(child);
        return clone_path(parent1);
    }
    
    return child;
}

// ===== Two Point Crossover =====
Path* two_point_crossover(const Path* parent1, const Path* parent2, const Grid* grid) {
    if (!parent1 || !parent2 || parent1->length < 3 || parent2->length < 3) {
        return single_point_crossover(parent1, parent2, grid);
    }
    
    int p1_start = random_int(0, parent1->length / 2);
    int p1_end = random_int(p1_start + 1, parent1->length - 1);
    
    int p2_start = random_int(0, parent2->length / 2);
    int p2_end = random_int(p2_start + 1, parent2->length - 1);
    
    Path* child = create_path(MAX_PATH_LENGTH);
    
    // Copy from parent1
    for (int i = 0; i < p1_start; i++) {
        add_coordinate_to_path(child, parent1->coordinates[i]);
    }
    
    // Copy middle from parent2
    for (int i = p2_start; i < p2_end && i < parent2->length; i++) {
        add_coordinate_to_path(child, parent2->coordinates[i]);
    }
    
    // Copy end from parent1
    for (int i = p1_end; i < parent1->length; i++) {
        add_coordinate_to_path(child, parent1->coordinates[i]);
    }
    
    if (child->length == 0) {
        free_path(child);
        return clone_path(parent1);
    }
    
    return child;
}

Path** crossover_paths(const Path* parent1, const Path* parent2, const Grid* grid) {
    Path** children = (Path**)safe_malloc(2 * sizeof(Path*));
    
    children[0] = single_point_crossover(parent1, parent2, grid);
    children[1] = single_point_crossover(parent2, parent1, grid);
    
    return children;
}

// ===== Mutation Operations =====
void mutate_insert_random(Path* path, const Grid* grid) {
    if (path->length >= MAX_PATH_LENGTH - 10) return;
    
    int insert_pos = random_int(1, path->length - 1);
    Coordinate current = path->coordinates[insert_pos];
    
    // Get a random neighbor
    Coordinate neighbors[6];
    int count = get_neighbors(grid, current, neighbors);
    
    if (count > 0) {
        Coordinate new_coord = neighbors[random_int(0, count - 1)];
        
        // Shift coordinates
        for (int i = path->length; i > insert_pos; i--) {
            if (i < path->capacity) {
                path->coordinates[i] = path->coordinates[i - 1];
            }
        }
        
        path->coordinates[insert_pos] = new_coord;
        if (path->length < path->capacity) {
            path->length++;
        }
    }
}

void mutate_swap_segments(Path* path) {
    if (path->length < 4) return;
    
    int pos1 = random_int(1, path->length - 2);
    int pos2 = random_int(pos1 + 1, path->length - 1);
    
    Coordinate temp = path->coordinates[pos1];
    path->coordinates[pos1] = path->coordinates[pos2];
    path->coordinates[pos2] = temp;
}

void mutate_remove_loop(Path* path) {
    if (path->length < 3) return;
    
    // Find and remove simple loops
    for (int i = 0; i < path->length - 2; i++) {
        for (int j = i + 2; j < path->length; j++) {
            if (coordinates_equal(path->coordinates[i], path->coordinates[j])) {
                // Remove loop
                int loop_size = j - i;
                for (int k = i + 1; k < path->length - loop_size; k++) {
                    path->coordinates[k] = path->coordinates[k + loop_size];
                }
                path->length -= loop_size;
                return;
            }
        }
    }
}

void mutate_path(Path* path, const Grid* grid, float mutation_rate) {
    if (random_float(0.0, 1.0) > mutation_rate) {
        return; // No mutation
    }
    
    int mutation_type = random_int(0, 2);
    
    switch (mutation_type) {
        case 0:
            mutate_insert_random(path, grid);
            break;
        case 1:
            mutate_swap_segments(path);
            break;
        case 2:
            mutate_remove_loop(path);
            break;
    }
}

// ===== Elitism =====
Path** apply_elitism(Path** population, int pop_size, int elitism_count) {
    if (elitism_count <= 0 || elitism_count > pop_size) {
        return NULL;
    }
    
    // Sort population by fitness
    qsort(population, pop_size, sizeof(Path*), compare_paths_by_fitness);
    
    // Clone top individuals
    Path** elite = (Path**)safe_malloc(elitism_count * sizeof(Path*));
    for (int i = 0; i < elitism_count; i++) {
        elite[i] = clone_path(population[i]);
    }
    
    return elite;
}

// ===== Create Next Generation =====
Path** create_next_generation(Path** current_pop, int pop_size, 
                              const Grid* grid, const Config* config) {
    Path** next_gen = (Path**)safe_malloc(pop_size * sizeof(Path*));
    int next_count = 0;
    
    // Apply elitism
    int elitism_count = (pop_size * config->elitism_percent) / 100;
    Path** elite = apply_elitism(current_pop, pop_size, elitism_count);
    
    // Add elite to next generation
    for (int i = 0; i < elitism_count; i++) {
        next_gen[next_count++] = elite[i];
    }
    free(elite);
    
    // Generate rest through crossover and mutation
    while (next_count < pop_size) {
        // Select parents
        Path* parent1 = tournament_selection(current_pop, pop_size, config->tournament_size);
        Path* parent2 = tournament_selection(current_pop, pop_size, config->tournament_size);
        
        Path* child;
        
        // Crossover
        if (random_float(0.0, 1.0) < config->crossover_rate) {
            child = single_point_crossover(parent1, parent2, grid);
        } else {
            child = clone_path(parent1);
        }
        
        // Mutation
        mutate_path(child, grid, config->mutation_rate);
        
        next_gen[next_count++] = child;
    }
    
    return next_gen;
}
