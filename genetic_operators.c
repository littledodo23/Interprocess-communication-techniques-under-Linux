#include "genetic_operators.h"
#include <float.h>

// ===== Tournament Selection =====
Path *tournament_selection(Path **population, int pop_size,
                           int tournament_size) {
    if (!population || pop_size <= 0)
        return NULL;

    if (tournament_size > pop_size) {
        tournament_size = pop_size;
    }

    Path *best = NULL;
    float best_fitness = -FLT_MAX;

    for (int i = 0; i < tournament_size; i++) {
        int idx = random_int(0, pop_size - 1);

        if (population[idx]->fitness > best_fitness) {
            best_fitness = population[idx]->fitness;
            best = population[idx];
        }
    }

    return best;
}

Path **select_parents(Path **population, int pop_size, const Config *config,
                      int num_parents) {
    Path **parents = (Path **)safe_malloc(num_parents * sizeof(Path *));

    for (int i = 0; i < num_parents; i++) {
        parents[i] =
            tournament_selection(population, pop_size, config->tournament_size);
    }

    return parents;
}

// ===== FIXED: Single Point Crossover with Connectivity =====
Path *single_point_crossover(const Path *parent1, const Path *parent2,
                              const Grid *grid) {
    if (!parent1 || !parent2) {
        return parent1 ? clone_path(parent1) : NULL;
    }

    if (parent1->length < 2 || parent2->length < 2) {
        return clone_path(parent1);
    }

    int point1 = random_int(1, parent1->length - 1);
    int point2 = random_int(1, parent2->length - 1);

    Path *child = create_path(parent1->length + parent2->length);

    // Take first part from parent1
    for (int i = 0; i < point1 && i < parent1->length; i++) {
        add_coordinate_to_path(child, parent1->coordinates[i]);
    }

    // FIX: Check if we need to connect the segments
    if (child->length > 0 && point2 < parent2->length) {
        Coordinate last_coord = child->coordinates[child->length - 1];
        Coordinate next_coord = parent2->coordinates[point2];

        // If not adjacent, use A* to connect them
        if (!are_coordinates_adjacent(last_coord, next_coord)) {
            Path *connector = connect_coordinates(grid, last_coord, next_coord);
            if (connector && connector->length > 1) {
                // Add connecting path (skip first as it's already in child)
                for (int i = 1; i < connector->length - 1; i++) {
                    add_coordinate_to_path(child, connector->coordinates[i]);
                }
                free_path(connector);
            }
        }
    }

    // Take second part from parent2
    for (int i = point2; i < parent2->length; i++) {
        add_coordinate_to_path(child, parent2->coordinates[i]);
    }

    if (child->length < 2) {
        free_path(child);
        return clone_path(parent1);
    }

    return child;
}

// ===== FIXED: Two Point Crossover with Connectivity =====
Path *two_point_crossover(const Path *parent1, const Path *parent2,
                           const Grid *grid) {
    if (!parent1 || !parent2 || parent1->length < 3 || parent2->length < 3) {
        return single_point_crossover(parent1, parent2, grid);
    }

    int p1_start = random_int(0, parent1->length / 2);
    int p1_end = random_int(p1_start + 1, parent1->length - 1);

    int p2_start = random_int(0, parent2->length / 2);
    int p2_end = random_int(p2_start + 1, parent2->length - 1);

    Path *child = create_path(MAX_PATH_LENGTH);

    // First segment from parent1
    for (int i = 0; i < p1_start && i < parent1->length; i++) {
        add_coordinate_to_path(child, parent1->coordinates[i]);
    }

    // FIX: Connect to parent2 segment
    if (child->length > 0 && p2_start < parent2->length) {
        Coordinate last = child->coordinates[child->length - 1];
        Coordinate next = parent2->coordinates[p2_start];
        
        if (!are_coordinates_adjacent(last, next)) {
            Path *connector = connect_coordinates(grid, last, next);
            if (connector && connector->length > 1) {
                for (int i = 1; i < connector->length - 1; i++) {
                    add_coordinate_to_path(child, connector->coordinates[i]);
                }
                free_path(connector);
            }
        }
    }

    // Middle segment from parent2
    for (int i = p2_start; i < p2_end && i < parent2->length; i++) {
        add_coordinate_to_path(child, parent2->coordinates[i]);
    }

    // FIX: Connect back to parent1
    if (child->length > 0 && p1_end < parent1->length) {
        Coordinate last = child->coordinates[child->length - 1];
        Coordinate next = parent1->coordinates[p1_end];
        
        if (!are_coordinates_adjacent(last, next)) {
            Path *connector = connect_coordinates(grid, last, next);
            if (connector && connector->length > 1) {
                for (int i = 1; i < connector->length - 1; i++) {
                    add_coordinate_to_path(child, connector->coordinates[i]);
                }
                free_path(connector);
            }
        }
    }

    // Final segment from parent1
    for (int i = p1_end; i < parent1->length; i++) {
        add_coordinate_to_path(child, parent1->coordinates[i]);
    }

    if (child->length < 2) {
        free_path(child);
        return clone_path(parent1);
    }

    return child;
}

// ===== Uniform Crossover =====
Path *uniform_crossover(const Path *parent1, const Path *parent2,
                        const Grid *grid) {
    (void)grid;
    
    if (!parent1 || !parent2) {
        return parent1 ? clone_path(parent1) : NULL;
    }

    Path *child = create_path(MAX_PATH_LENGTH);

    int min_length = parent1->length < parent2->length ? parent1->length
                                                        : parent2->length;

    for (int i = 0; i < min_length; i++) {
        if (random_float(0.0, 1.0) < 0.5) {
            add_coordinate_to_path(child, parent1->coordinates[i]);
        } else {
            add_coordinate_to_path(child, parent2->coordinates[i]);
        }
    }

    if (parent1->length > min_length) {
        for (int i = min_length; i < parent1->length; i++) {
            add_coordinate_to_path(child, parent1->coordinates[i]);
        }
    } else if (parent2->length > min_length) {
        for (int i = min_length; i < parent2->length; i++) {
            add_coordinate_to_path(child, parent2->coordinates[i]);
        }
    }

    if (child->length < 2) {
        free_path(child);
        return clone_path(parent1);
    }

    return child;
}

// ===== Crossover with two children =====
Path **crossover_paths(const Path *parent1, const Path *parent2,
                       const Grid *grid) {
    Path **children = (Path **)safe_malloc(2 * sizeof(Path *));

    children[0] = single_point_crossover(parent1, parent2, grid);
    children[1] = single_point_crossover(parent2, parent1, grid);

    return children;
}

// ===== Mutation: Insert Random Point =====
void mutate_insert_random(Path *path, const Grid *grid) {
    if (!path || path->length < 2 || path->length >= MAX_PATH_LENGTH - 10) {
        return;
    }

    int insert_pos = random_int(1, path->length - 1);
    Coordinate current = path->coordinates[insert_pos];

    Coordinate neighbors[6];
    int neighbor_count = get_neighbors(grid, current, neighbors);

    if (neighbor_count == 0)
        return;

    Coordinate new_coord = neighbors[random_int(0, neighbor_count - 1)];

    if (path->length >= path->capacity) {
        return;
    }

    for (int i = path->length; i > insert_pos; i--) {
        path->coordinates[i] = path->coordinates[i - 1];
    }

    path->coordinates[insert_pos] = new_coord;
    path->length++;
}

// ===== Mutation: Swap Two Segments =====
void mutate_swap_segments(Path *path) {
    if (!path || path->length < 4)
        return;

    int pos1 = random_int(1, path->length - 2);
    int pos2 = random_int(pos1 + 1, path->length - 1);

    Coordinate temp = path->coordinates[pos1];
    path->coordinates[pos1] = path->coordinates[pos2];
    path->coordinates[pos2] = temp;
}

// ===== Mutation: Remove Loop =====
void mutate_remove_loop(Path *path) {
    if (!path || path->length < 3)
        return;

    for (int i = 0; i < path->length - 2; i++) {
        for (int j = i + 2; j < path->length; j++) {
            if (coordinates_equal(path->coordinates[i], path->coordinates[j])) {
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

// ===== Mutation: Reverse Segment =====
void mutate_reverse_segment(Path *path) {
    if (!path || path->length < 3)
        return;

    int start = random_int(0, path->length - 2);
    int end = random_int(start + 1, path->length - 1);

    while (start < end) {
        Coordinate temp = path->coordinates[start];
        path->coordinates[start] = path->coordinates[end];
        path->coordinates[end] = temp;
        start++;
        end--;
    }
}

// ===== Main Mutation Function =====
void mutate_path(Path *path, const Grid *grid, float mutation_rate) {
    if (!path || !grid)
        return;

    if (random_float(0.0, 1.0) > mutation_rate) {
        return;
    }

    int mutation_type = random_int(0, 3);

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
    case 3:
        mutate_reverse_segment(path);
        break;
    }
}

// ===== Elitism =====
Path **apply_elitism(Path **population, int pop_size, int elitism_count) {
    if (!population || elitism_count <= 0 || elitism_count > pop_size) {
        return NULL;
    }

    Path **elite = (Path **)safe_malloc(elitism_count * sizeof(Path *));

    for (int i = 0; i < elitism_count; i++) {
        elite[i] = clone_path(population[i]);
    }

    return elite;
}

// ===== Create Next Generation =====
Path **create_next_generation(Path **current_pop, int pop_size,
                              const Grid *grid, const Config *config) {
    if (!current_pop || !grid || !config || pop_size <= 0) {
        error_exit("Invalid parameters for create_next_generation");
    }

    Path **next_gen = (Path **)safe_malloc(pop_size * sizeof(Path *));
    int next_count = 0;

    int elitism_count = (pop_size * config->elitism_percent) / 100;
    if (elitism_count < 1)
        elitism_count = 1;
    if (elitism_count > pop_size / 2)
        elitism_count = pop_size / 2;

    Path **elite = apply_elitism(current_pop, pop_size, elitism_count);

    for (int i = 0; i < elitism_count; i++) {
        next_gen[next_count++] = elite[i];
    }
    free(elite);

    while (next_count < pop_size) {
        Path *parent1 =
            tournament_selection(current_pop, pop_size, config->tournament_size);
        Path *parent2 =
            tournament_selection(current_pop, pop_size, config->tournament_size);

        Path *child;

        if (random_float(0.0, 1.0) < config->crossover_rate) {
            int crossover_type = random_int(0, 2);

            switch (crossover_type) {
            case 0:
                child = single_point_crossover(parent1, parent2, grid);
                break;
            case 1:
                child = two_point_crossover(parent1, parent2, grid);
                break;
            case 2:
                child = uniform_crossover(parent1, parent2, grid);
                break;
            default:
                child = single_point_crossover(parent1, parent2, grid);
            }
        } else {
            child = clone_path(parent1);
        }

        mutate_path(child, grid, config->mutation_rate);

        next_gen[next_count++] = child;
    }

    return next_gen;
}
