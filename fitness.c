#include "fitness.h"

// ===== Normalization Functions (NEW) =====

float normalize_survivors(int survivors, int max_survivors) {
    if (max_survivors == 0) return 0.0f;
    return (float)survivors / max_survivors;
}

float normalize_coverage(float coverage) {
    // Coverage is already in 0-100 range
    return coverage / 100.0f;
}

float normalize_length(int length, int max_length) {
    if (max_length == 0) return 0.0f;
    return (float)length / max_length;
}

float normalize_risk(float risk, float max_expected_risk) {
    if (max_expected_risk < 0.001f) return 0.0f;
    float normalized = risk / max_expected_risk;
    // Clamp to [0, 1]
    if (normalized > 1.0f) normalized = 1.0f;
    return normalized;
}

// ===== Main Fitness Function with Normalization =====

float calculate_fitness(Path *path, const Grid *grid, const Config *config) {
    if (!path || !grid || !config) {
        return 0.0f;
    }

    // Calculate raw components
    int survivors = calculate_survivors_reached(path, grid);
    float coverage = calculate_coverage_area(path, grid);
    int length = path->length;
    float risk = calculate_path_risk(path, grid);

    // Normalize each component to [0, 1] range
    float norm_survivors = normalize_survivors(survivors, grid->num_survivors);
    float norm_coverage = normalize_coverage(coverage);
    float norm_length = normalize_length(length, MAX_PATH_LENGTH);
    
    // Estimate maximum expected risk for normalization
    // Max risk = max collisions * 10 + max length * 0.1 + max z-changes * 2
    float max_expected_risk = MAX_PATH_LENGTH * 0.1f + 100.0f; // Reasonable estimate
    float norm_risk = normalize_risk(risk, max_expected_risk);

    // Apply weights to normalized values
    // All components now on same scale [0, 1]
    float fitness = config->w1_survivors * norm_survivors +
                    config->w2_coverage * norm_coverage - 
                    config->w3_length * norm_length -
                    config->w4_risk * norm_risk;

    return fitness;
}

// ===== Component Calculations =====

int calculate_survivors_reached(const Path *path, const Grid *grid) {
    if (!path || !grid || path->length == 0) {
        return 0;
    }

    int count = 0;
    int *found = (int *)calloc(grid->num_survivors, sizeof(int));

    for (int i = 0; i < path->length; i++) {
        Coordinate pos = path->coordinates[i];

        for (int s = 0; s < grid->num_survivors; s++) {
            if (!found[s] && coordinates_equal(pos, grid->survivors[s])) {
                found[s] = 1;
                count++;
            }
        }
    }

    free(found);
    return count;
}

float calculate_coverage_area(const Path *path, const Grid *grid) {
    if (!path || !grid || path->length == 0) {
        return 0.0f;
    }

    int ***visited = (int ***)calloc(grid->size_x, sizeof(int **));
    for (int x = 0; x < grid->size_x; x++) {
        visited[x] = (int **)calloc(grid->size_y, sizeof(int *));
        for (int y = 0; y < grid->size_y; y++) {
            visited[x][y] = (int *)calloc(grid->size_z, sizeof(int));
        }
    }

    int coverage_count = 0;
    int coverage_radius = 2;

    for (int i = 0; i < path->length; i++) {
        Coordinate pos = path->coordinates[i];

        for (int dx = -coverage_radius; dx <= coverage_radius; dx++) {
            for (int dy = -coverage_radius; dy <= coverage_radius; dy++) {
                for (int dz = -coverage_radius; dz <= coverage_radius; dz++) {
                    Coordinate check =
                        create_coordinate(pos.x + dx, pos.y + dy, pos.z + dz);

                    if (is_valid_coordinate(grid, check) &&
                        !visited[check.x][check.y][check.z]) {
                        visited[check.x][check.y][check.z] = 1;
                        coverage_count++;
                    }
                }
            }
        }
    }

    for (int x = 0; x < grid->size_x; x++) {
        for (int y = 0; y < grid->size_y; y++) {
            free(visited[x][y]);
        }
        free(visited[x]);
    }
    free(visited);

    return (float)coverage_count / grid->total_cells * 100.0f;
}

float calculate_path_risk(const Path *path, const Grid *grid) {
    if (!path || !grid) {
        return 0.0f;
    }

    float risk = 0.0f;

    // Collision penalty
    risk += path->collision_count * 10.0f;

    // Length penalty (longer paths are riskier)
    risk += path->length * 0.1f;

    // Z-level change penalty (vertical movement is risky)
    for (int i = 1; i < path->length; i++) {
        int z_diff = abs(path->coordinates[i].z - path->coordinates[i - 1].z);
        if (z_diff > 1) {
            risk += z_diff * 2.0f;
        }
    }

    return risk;
}

// ===== Helper Functions =====

float normalize_fitness_component(float value, float min_val, float max_val) {
    if (max_val - min_val < 0.001f) {
        return 0.0f;
    }
    return (value - min_val) / (max_val - min_val);
}

void update_path_fitness(Path *path, const Grid *grid, const Config *config) {
    if (!path)
        return;

    path->survivors_reached = calculate_survivors_reached(path, grid);
    path->collision_count = check_path_collisions(path, grid);

    path->fitness = calculate_fitness(path, grid, config);
}

void update_population_fitness(Path **population, int pop_size,
                               const Grid *grid, const Config *config) {
    if (!population || pop_size <= 0)
        return;

    for (int i = 0; i < pop_size; i++) {
        update_path_fitness(population[i], grid, config);
    }
}

// ===== Statistics =====

void print_fitness_statistics(Path **population, int pop_size) {
    if (!population || pop_size <= 0) {
        printf("No population to analyze\n");
        return;
    }

    float total_fitness = 0.0f;
    float best_fitness = population[0]->fitness;
    float worst_fitness = population[0]->fitness;
    int total_survivors = 0;
    int total_length = 0;

    for (int i = 0; i < pop_size; i++) {
        float fitness = population[i]->fitness;
        total_fitness += fitness;
        total_survivors += population[i]->survivors_reached;
        total_length += population[i]->length;

        if (fitness > best_fitness)
            best_fitness = fitness;
        if (fitness < worst_fitness)
            worst_fitness = fitness;
    }

    printf("\n========== Population Fitness Statistics ==========\n");
    printf("Population Size: %d\n", pop_size);
    printf("Average Fitness: %.2f\n", total_fitness / pop_size);
    printf("Best Fitness: %.2f\n", best_fitness);
    printf("Worst Fitness: %.2f\n", worst_fitness);
    printf("Average Survivors Reached: %.1f\n",
           (float)total_survivors / pop_size);
    printf("Average Path Length: %.1f\n", (float)total_length / pop_size);
    printf("====================================================\n");
}

float get_average_fitness(Path **population, int pop_size) {
    if (!population || pop_size <= 0)
        return 0.0f;

    float total = 0.0f;
    for (int i = 0; i < pop_size; i++) {
        total += population[i]->fitness;
    }
    return total / pop_size;
}

float get_best_fitness(Path **population, int pop_size) {
    if (!population || pop_size <= 0)
        return 0.0f;

    float best = population[0]->fitness;
    for (int i = 1; i < pop_size; i++) {
        if (population[i]->fitness > best) {
            best = population[i]->fitness;
        }
    }
    return best;
}

float get_worst_fitness(Path **population, int pop_size) {
    if (!population || pop_size <= 0)
        return 0.0f;

    float worst = population[0]->fitness;
    for (int i = 1; i < pop_size; i++) {
        if (population[i]->fitness < worst) {
            worst = population[i]->fitness;
        }
    }
    return worst;
}
