#include "fitness.h"

// ===== Main Fitness Function =====

float calculate_fitness(Path *path, const Grid *grid, const Config *config) {
  if (!path || !grid || !config) {
    return 0.0f;
  }

  // Calculate components
  float survivors = (float)calculate_survivors_reached(path, grid);
  float coverage = calculate_coverage_area(path, grid);
  float length = (float)path->length;
  float risk = calculate_path_risk(path, grid);

  // Apply weights from config
  float fitness = config->w1_survivors * survivors +
                  config->w2_coverage * coverage - config->w3_length * length -
                  config->w4_risk * risk;

  return fitness;
}

// ===== Component Calculations =====

int calculate_survivors_reached(const Path *path, const Grid *grid) {
  if (!path || !grid || path->length == 0) {
    return 0;
  }

  int count = 0;
  int *found = (int *)calloc(grid->num_survivors, sizeof(int));

  // Check each coordinate in path
  for (int i = 0; i < path->length; i++) {
    Coordinate pos = path->coordinates[i];

    // Check if this position has a survivor
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

  // Create a 3D array to track visited cells
  int ***visited = (int ***)calloc(grid->size_x, sizeof(int **));
  for (int x = 0; x < grid->size_x; x++) {
    visited[x] = (int **)calloc(grid->size_y, sizeof(int *));
    for (int y = 0; y < grid->size_y; y++) {
      visited[x][y] = (int *)calloc(grid->size_z, sizeof(int));
    }
  }

  // Mark cells in path and their neighbors
  int coverage_count = 0;
  int coverage_radius = 2; // Robot can reach 2 cells around its path

  for (int i = 0; i < path->length; i++) {
    Coordinate pos = path->coordinates[i];

    // Check cells within radius
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

  // Free visited array
  for (int x = 0; x < grid->size_x; x++) {
    for (int y = 0; y < grid->size_y; y++) {
      free(visited[x][y]);
    }
    free(visited[x]);
  }
  free(visited);

  // Return coverage as percentage
  return (float)coverage_count / grid->total_cells * 100.0f;
}

float calculate_path_risk(const Path *path, const Grid *grid) {
  if (!path || !grid) {
    return 0.0f;
  }

  float risk = 0.0f;

  // Risk from collisions with obstacles
  risk += path->collision_count * 10.0f;

  // Risk from path length (longer paths = more dangerous)
  risk += path->length * 0.1f;

  // Risk from steep vertical movements
  for (int i = 1; i < path->length; i++) {
    int z_diff = abs(path->coordinates[i].z - path->coordinates[i - 1].z);
    if (z_diff > 1) {
      risk += z_diff * 2.0f; // Climbing/descending is risky
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

  // Update path statistics
  path->survivors_reached = calculate_survivors_reached(path, grid);
  path->collision_count = check_path_collisions(path, grid);

  // Calculate fitness
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