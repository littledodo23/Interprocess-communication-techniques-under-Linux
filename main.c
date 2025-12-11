#include "fitness.h"
#include "genetic_operators.h"
#include "grid_environment.h"
#include "multiprocess.h"
#include "path_generator.h"
#include "utilities.h"

// Function prototypes
void print_generation_stats(Path **population, int pop_size, int generation,
                            double elapsed_time);
void save_best_paths(Path **population, int pop_size, const Grid *grid,
                     const char *filename);

int main(int argc, char *argv[]) {
  printf("========================================\n");
  printf("  Genetic Algorithm Rescue Operations\n");
  printf("  Multi-Processing with IPC\n");
  printf("========================================\n\n");

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

  print_config(config);

  // Create output directory
  int ret = system("mkdir -p output");
  (void)ret;

  // Create grid environment
  printf("Creating 3D grid environment...\n");
  Grid *grid = create_grid(config->grid_x, config->grid_y, config->grid_z);

  printf("Initializing grid with obstacles and survivors...\n");
  initialize_grid(grid, config);
  print_grid_info(grid);

  save_grid_to_file(grid, "output/grid_layout.txt");
  printf("Grid layout saved to: output/grid_layout.txt\n");

  if (config->verbose) {
    print_grid_layer(grid, 0);
  }

  // Generate initial population
  printf("\n========== Generating Initial Population ==========\n");
  int pop_size = 0;
  Path **population = generate_initial_population(grid, config, &pop_size);
  printf("✓ Generated %d paths for initial population\n", pop_size);

  // Setup IPC for multiprocessing
  printf("\n========== Setting Up Multi-Processing ==========\n");
  int shm_id, sem_id;
  SharedData *shared_data;

  if (setup_shared_memory(&shm_id, &shared_data) != 0) {
    error_exit("Failed to setup shared memory");
  }
  printf("✓ Shared memory initialized\n");

  if (setup_semaphores(&sem_id) != 0) {
    cleanup_ipc(shm_id, sem_id);
    error_exit("Failed to setup semaphores");
  }
  printf("✓ Semaphores initialized\n");

  // Initialize shared data
  shared_data->population_size = pop_size;
  shared_data->current_generation = 0;
  shared_data->workers_completed = 0;
  shared_data->best_fitness = -1000.0f;
  shared_data->termination_flag = 0;
  shared_data->work_ready = 0;
  shared_data->num_workers = config->num_workers;

  // Create worker pool
  printf("Creating worker pool (%d workers)...\n", config->num_workers);
  pid_t *workers =
      create_worker_pool(config->num_workers, shm_id, sem_id, grid, config);
  printf("✓ Worker pool created successfully\n");

  sleep(1); // Give workers time to start

  // Evaluate initial fitness using parallel workers
  printf("\nEvaluating initial fitness in parallel...\n");
  parallel_evaluate_fitness(population, pop_size, grid, config, 
                           shared_data, sem_id);
  
  qsort(population, pop_size, sizeof(Path *), compare_paths_by_fitness);

  printf("\nInitial Population Statistics:\n");
  print_fitness_statistics(population, pop_size);

  // Open statistics file
  FILE *stats_file = NULL;
  if (config->save_stats) {
    stats_file = fopen("output/generation_stats.csv", "w");
    if (stats_file) {
      fprintf(stats_file, "Generation,Best_Fitness,Average_Fitness,Worst_"
                          "Fitness,Avg_Survivors,Avg_Length\n");
    }
  }

  // Main GA loop
  printf("\n========== Starting Genetic Algorithm Evolution ==========\n");
  printf("Maximum generations: %d\n", config->max_generations);
  printf("Stagnation limit: %d generations\n", config->stagnation_limit);
  if (config->time_limit > 0) {
    printf("Time limit: %d seconds\n\n", config->time_limit);
  }

  int generation = 0;
  int stagnation_counter = 0;
  float prev_best_fitness = 0.0f;
  double start_time = get_time_ms();

  while (generation < config->max_generations) {
    double gen_start_time = get_time_ms();

    // Update shared data
    sem_wait(sem_id, 0);
    shared_data->current_generation = generation;
    shared_data->best_fitness = population[0]->fitness;
    sem_signal(sem_id, 0);

    if (config->verbose) {
      printf("\n========== Generation %d ==========\n", generation + 1);
    } else {
      print_progress_bar(generation + 1, config->max_generations, "Evolution");
    }

    float best_fitness = population[0]->fitness;
    float avg_fitness = get_average_fitness(population, pop_size);

    if (config->verbose) {
      double elapsed = (get_time_ms() - start_time) / 1000.0;
      print_generation_stats(population, pop_size, generation + 1, elapsed);
    }

    // Save statistics
    if (stats_file) {
      float worst_fitness = get_worst_fitness(population, pop_size);
      int total_survivors = 0;
      int total_length = 0;
      for (int i = 0; i < pop_size; i++) {
        total_survivors += population[i]->survivors_reached;
        total_length += population[i]->length;
      }
      fprintf(stats_file, "%d,%.2f,%.2f,%.2f,%.2f,%.2f\n", generation + 1,
              best_fitness, avg_fitness, worst_fitness,
              (float)total_survivors / pop_size, (float)total_length / pop_size);
      fflush(stats_file);
    }

    // Check for stagnation
    if (fabs(best_fitness - prev_best_fitness) < 0.01) {
      stagnation_counter++;
      if (config->verbose) {
        printf("Stagnation counter: %d/%d\n", stagnation_counter,
               config->stagnation_limit);
      }
    } else {
      stagnation_counter = 0;
    }

    // Check termination conditions
    if (stagnation_counter >= config->stagnation_limit) {
      printf("\n✓ Stopping: No improvement for %d generations\n",
             config->stagnation_limit);
      break;
    }

    double elapsed = (get_time_ms() - start_time) / 1000.0;
    if (config->time_limit > 0 && elapsed > config->time_limit) {
      printf("\n✓ Stopping: Time limit of %d seconds reached (%.1fs)\n",
             config->time_limit, elapsed);
      break;
    }

    if (population[0]->survivors_reached == grid->num_survivors &&
        generation > 10) {
      printf("\n✓ Stopping: All survivors reached in optimal path!\n");
      break;
    }

    prev_best_fitness = best_fitness;

    // Create next generation
    Path **next_generation =
        create_next_generation(population, pop_size, grid, config);

    // Evaluate fitness in parallel using workers
    if (config->verbose) {
      printf("Evaluating new generation fitness in parallel...\n");
    }
    parallel_evaluate_fitness(next_generation, pop_size, grid, config,
                             shared_data, sem_id);

    // Sort by fitness
    qsort(next_generation, pop_size, sizeof(Path *),
          compare_paths_by_fitness);

    // Free old population
    for (int i = 0; i < pop_size; i++) {
      free_path(population[i]);
    }
    free(population);

    population = next_generation;
    generation++;

    if (config->verbose) {
      double gen_time = (get_time_ms() - gen_start_time) / 1000.0;
      printf("Generation time: %.3f seconds\n", gen_time);
    }
  }

  if (stats_file) {
    fclose(stats_file);
    printf("\n✓ Statistics saved to: output/generation_stats.csv\n");
  }

  // Results
  printf("\n========================================\n");
  printf("       Evolution Complete!              \n");
  printf("========================================\n\n");

  double total_time = (get_time_ms() - start_time) / 1000.0;

  printf("Summary:\n");
  printf("  Total Generations: %d\n", generation);
  printf("  Total Time: %.2f seconds\n", total_time);
  printf("  Avg Time per Generation: %.3f seconds\n",
         total_time / (generation > 0 ? generation : 1));
  printf("\n");

  printf("========== Final Population Statistics ==========\n");
  print_fitness_statistics(population, pop_size);

  printf("\n========== Best Solution Found ==========\n");
  Path *best_path = population[0];

  printf("Fitness: %.2f\n", best_path->fitness);
  printf("Survivors Reached: %d/%d (%.1f%%)\n", best_path->survivors_reached,
         grid->num_survivors,
         (float)best_path->survivors_reached / grid->num_survivors * 100.0f);
  printf("Path Length: %d steps\n", best_path->length);
  printf("Collision Count: %d\n", best_path->collision_count);
  printf("Coverage Area: %.2f%%\n", calculate_coverage_area(best_path, grid));
  printf("Euclidean Distance: %.2f\n",
         calculate_path_length_euclidean(best_path));
  printf("Manhattan Distance: %d\n",
         calculate_path_length_manhattan(best_path));

  save_path_to_file(best_path, "output/best_path.txt");
  printf("\n✓ Best path saved to: output/best_path.txt\n");

  save_best_paths(population, pop_size < 5 ? pop_size : 5, grid,
                  "output/top_paths.txt");
  printf("✓ Top paths saved to: output/top_paths.txt\n");

  // Save final results
  FILE *results = fopen("output/results.txt", "w");
  if (results) {
    fprintf(results, "========================================\n");
    fprintf(results, "  Genetic Algorithm Rescue Operations\n");
    fprintf(results, "  Final Results\n");
    fprintf(results, "========================================\n\n");

    fprintf(results, "Grid Configuration:\n");
    fprintf(results, "  Dimensions: %dx%dx%d\n", grid->size_x, grid->size_y,
            grid->size_z);
    fprintf(results, "  Total Survivors: %d\n", grid->num_survivors);
    fprintf(results, "  Obstacles: %d (%.1f%%)\n\n", grid->obstacle_count,
            (float)grid->obstacle_count / grid->total_cells * 100);

    fprintf(results, "Algorithm Parameters:\n");
    fprintf(results, "  Population Size: %d\n", config->population_size);
    fprintf(results, "  Generations: %d\n", generation);
    fprintf(results, "  Mutation Rate: %.3f\n", config->mutation_rate);
    fprintf(results, "  Crossover Rate: %.3f\n", config->crossover_rate);
    fprintf(results, "  Elitism: %d%%\n", config->elitism_percent);
    fprintf(results, "  Workers: %d\n\n", config->num_workers);

    fprintf(results, "Best Solution:\n");
    fprintf(results, "  Fitness Score: %.2f\n", best_path->fitness);
    fprintf(results, "  Survivors Reached: %d/%d\n",
            best_path->survivors_reached, grid->num_survivors);
    fprintf(results, "  Path Length: %d\n", best_path->length);
    fprintf(results, "  Collisions: %d\n", best_path->collision_count);
    fprintf(results, "  Coverage: %.2f%%\n\n",
            calculate_coverage_area(best_path, grid));

    fprintf(results, "Execution Time: %.2f seconds\n", total_time);

    fclose(results);
    printf("✓ Results summary saved to: output/results.txt\n");
  }

  if (config->verbose && best_path->length > 0) {
    printf("\nBest Path Coordinates (first 20 steps):\n");
    int show_count = best_path->length < 20 ? best_path->length : 20;
    for (int i = 0; i < show_count; i++) {
      printf("  Step %2d: (%2d, %2d, %2d)", i + 1,
             best_path->coordinates[i].x, best_path->coordinates[i].y,
             best_path->coordinates[i].z);

      int survivor_idx = get_survivor_at(grid, best_path->coordinates[i]);
      if (survivor_idx >= 0) {
        printf(" <- SURVIVOR #%d", survivor_idx + 1);
      }
      printf("\n");
    }
    if (best_path->length > 20) {
      printf("  ... (%d more steps)\n", best_path->length - 20);
    }
  }

  // Cleanup
  printf("\n========== Cleanup ==========\n");
  printf("Terminating worker processes...\n");

  sem_wait(sem_id, 0);
  shared_data->termination_flag = 1;
  sem_signal(sem_id, 0);

  terminate_workers(workers, config->num_workers);
  printf("✓ Workers terminated\n");

  cleanup_ipc(shm_id, sem_id);
  printf("✓ IPC resources cleaned up\n");

  for (int i = 0; i < pop_size; i++) {
    free_path(population[i]);
  }
  free(population);
  printf("✓ Population memory freed\n");

  free_grid(grid);
  free_config(config);
  printf("✓ Grid and configuration freed\n");

  printf("\n========================================\n");
  printf("  Program completed successfully!\n");
  printf("========================================\n\n");

  return 0;
}

// Helper Functions

void print_generation_stats(Path **population, int pop_size, int generation,
                            double elapsed_time) {
  float best = population[0]->fitness;
  float avg = get_average_fitness(population, pop_size);
  float worst = get_worst_fitness(population, pop_size);

  printf("Generation: %d\n", generation);
  printf("Best Fitness: %.2f\n", best);
  printf("Average Fitness: %.2f\n", avg);
  printf("Worst Fitness: %.2f\n", worst);
  printf("Best Path: %d survivors, %d length, %d collisions\n",
         population[0]->survivors_reached, population[0]->length,
         population[0]->collision_count);
  printf("Elapsed Time: %.2f seconds\n", elapsed_time);
}

void save_best_paths(Path **population, int count, const Grid *grid,
                     const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    warning("Could not save top paths");
    return;
  }

  fprintf(file, "========================================\n");
  fprintf(file, "  Top %d Rescue Paths\n", count);
  fprintf(file, "========================================\n\n");

  for (int i = 0; i < count; i++) {
    Path *path = population[i];

    fprintf(file, "=== Path #%d ===\n", i + 1);
    fprintf(file, "Fitness: %.2f\n", path->fitness);
    fprintf(file, "Survivors: %d/%d\n", path->survivors_reached,
            grid->num_survivors);
    fprintf(file, "Length: %d steps\n", path->length);
    fprintf(file, "Collisions: %d\n", path->collision_count);
    fprintf(file, "Coverage: %.2f%%\n\n", calculate_coverage_area(path, grid));

    fprintf(file, "Coordinates:\n");
    for (int j = 0; j < path->length; j++) {
      fprintf(file, "  %3d: (%2d, %2d, %2d)", j, path->coordinates[j].x,
              path->coordinates[j].y, path->coordinates[j].z);

      int survivor_idx = get_survivor_at(grid, path->coordinates[j]);
      if (survivor_idx >= 0) {
        fprintf(file, " <- Survivor #%d", survivor_idx + 1);
      }
      fprintf(file, "\n");
    }
    fprintf(file, "\n");
  }

  fclose(file);
}
