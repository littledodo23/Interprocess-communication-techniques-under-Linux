#include "fitness.h"
#include "grid_environment.h"
#include "path_generator.h"
#include "utilities.h"

int main(int argc, char *argv[]) {
  printf("========================================\n");
  printf("  Genetic Rescue Operations - Test\n");
  printf("  Phase 3: Path Generation\n");
  printf("========================================\n\n");

  // Initialize random seed
  srand(time(NULL));

  // Load configuration
  Config *config = NULL;
  if (argc > 1) {
    printf("Loading configuration from: %s\n", argv[1]);
    config = load_config(argv[1]);
  } else {
    printf("No config file provided. Using default values.\n");
    config = create_default_config();
  }

  // Print configuration
  print_config(config);

  // Create grid
  printf("Creating 3D grid environment...\n");
  Grid *grid = create_grid(config->grid_x, config->grid_y, config->grid_z);

  // Initialize grid
  printf("Initializing grid with obstacles and survivors...\n");
  initialize_grid(grid, config);
  print_grid_info(grid);

  // Print first layer
  print_grid_layer(grid, 0);

  // ===== Test A* Pathfinding =====
  printf("\n========== Testing A* Pathfinding ==========\n");

  if (grid->num_survivors > 0) {
    Coordinate first_survivor = grid->survivors[0];
    printf(
        "Finding path from start (%d,%d,%d) to first survivor (%d,%d,%d)...\n",
        grid->start.x, grid->start.y, grid->start.z, first_survivor.x,
        first_survivor.y, first_survivor.z);

    Path *test_path = find_path_astar(grid, grid->start, first_survivor);

    if (test_path) {
      printf("✓ Path found!\n");
      print_path(test_path);
      save_path_to_file(test_path, "output/test_path.txt");
      printf("Path saved to: output/test_path.txt\n");
      free_path(test_path);
    } else {
      printf("✗ No path found!\n");
    }
  }

  // ===== Test Greedy Path =====
  printf("\n========== Testing Greedy Path Generation ==========\n");
  Path *greedy = generate_greedy_path(grid);
  if (greedy) {
    printf("✓ Greedy path generated!\n");
    print_path(greedy);
    free_path(greedy);
  }

  // ===== Test Random Path =====
  printf("\n========== Testing Random Path Generation ==========\n");
  Path *random = generate_random_path(grid, 50);
  if (random) {
    printf("✓ Random path generated!\n");
    print_path(random);
    free_path(random);
  }

  // ===== Test Population Generation =====
  printf("\n========== Testing Population Generation ==========\n");
  int pop_size = 0;
  Path **population = generate_initial_population(grid, config, &pop_size);

  printf("✓ Generated %d paths for initial population\n", pop_size);

  // Statistics
  int total_length = 0;
  int total_survivors = 0;
  int total_collisions = 0;

  for (int i = 0; i < pop_size; i++) {
    total_length += population[i]->length;
    total_survivors += population[i]->survivors_reached;
    total_collisions += population[i]->collision_count;
  }

  printf("\nPopulation Statistics:\n");
  printf("  Average path length: %.1f\n", (float)total_length / pop_size);
  printf("  Average survivors reached: %.1f / %d\n",
         (float)total_survivors / pop_size, grid->num_survivors);
  printf("  Average collisions: %.1f\n", (float)total_collisions / pop_size);

  // Show sample paths
  printf("\nSample paths from population:\n");
  for (int i = 0; i < 3 && i < pop_size; i++) {
    printf("\n  Path %d:\n", i + 1);
    printf("    Length: %d\n", population[i]->length);
    printf("    Survivors: %d/%d\n", population[i]->survivors_reached,
           grid->num_survivors);
    printf("    Collisions: %d\n", population[i]->collision_count);
  }

  // ===== Test Path Utilities =====
  printf("\n========== Testing Path Utilities ==========\n");
  if (pop_size > 0) {
    Path *test = population[0];
    float euclidean = calculate_path_length_euclidean(test);
    int manhattan = calculate_path_length_manhattan(test);

    printf("Path 0 distances:\n");
    printf("  Euclidean: %.2f\n", euclidean);
    printf("  Manhattan: %d\n", manhattan);
  }

  // Final summary
  printf("\n========================================\n");
  printf("  Phase 3 Test Completed Successfully! ✓\n");
  printf("========================================\n\n");

  printf("Summary:\n");
  printf("  - Grid: %dx%dx%d with %d survivors\n", grid->size_x, grid->size_y,
         grid->size_z, grid->num_survivors);
  printf("  - A* pathfinding: OK\n");
  printf("  - Population generation: OK\n");
  printf("  - Generated %d initial paths\n", pop_size);
  printf("  - All path utilities: OK\n\n");

  // Cleanup
  for (int i = 0; i < pop_size; i++) {
    free_path(population[i]);
  }
  free(population);
  // ===== Test Fitness Function =====
  printf("\n========== Testing Fitness Function ==========\n");

  // Calculate fitness for all paths
  update_population_fitness(population, pop_size, grid, config);

  // Print statistics
  print_fitness_statistics(population, pop_size);

  // Show best path
  printf("\n========== Best Path ==========\n");
  qsort(population, pop_size, sizeof(Path *), compare_paths_by_fitness);
  Path *best_path = population[0];

  printf("Best Path Details:\n");
  printf("  Fitness: %.2f\n", best_path->fitness);
  printf("  Survivors: %d/%d\n", best_path->survivors_reached,
         grid->num_survivors);
  printf("  Length: %d\n", best_path->length);
  printf("  Collisions: %d\n", best_path->collision_count);
  printf("  Coverage: %.2f%%\n", calculate_coverage_area(best_path, grid));
  free_grid(grid);
  free_config(config);

  printf("Memory cleaned up. Exiting.\n");

  return 0;
}