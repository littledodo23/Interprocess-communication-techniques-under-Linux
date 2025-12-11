#ifndef MULTIPROCESS_H
#define MULTIPROCESS_H

#include "utilities.h"
#include "path_generator.h"
#include "grid_environment.h"
#include "fitness.h"

// ===== Shared Path Data for IPC =====
typedef struct {
    Coordinate coordinates[MAX_PATH_LENGTH];
    int length;
    int survivors_reached;
    float fitness;
    int collision_count;
} SharedPath;

// ===== Worker Range Assignment =====
typedef struct {
    int start_idx;
    int end_idx;
    int worker_id;
} WorkerRange;

// ===== Shared Memory Structure (EXPANDED) =====
typedef struct {
    // Population data
    SharedPath paths[MAX_POPULATION];
    int population_size;
    
    // Worker assignments
    WorkerRange worker_ranges[16];  // Support up to 16 workers
    int num_workers;
    
    // Synchronization
    int work_ready;                 // Flag: 1 = work available
    int workers_completed;          // Count of workers finished
    int current_generation;
    
    // Best solution tracking
    float best_fitness;
    int best_path_index;
    
    // Termination
    int termination_flag;
} SharedData;

// ===== IPC Setup =====
int setup_shared_memory(int* shm_id, SharedData** shared_data);
int setup_semaphores(int* sem_id);
void cleanup_ipc(int shm_id, int sem_id);

// ===== Semaphore Operations =====
void sem_wait(int sem_id, int sem_num);
void sem_signal(int sem_id, int sem_num);

// ===== Path Transfer Functions =====
void path_to_shared(const Path* path, SharedPath* shared_path);
void shared_to_path(const SharedPath* shared_path, Path* path);
void copy_population_to_shared(Path** population, int pop_size, SharedData* shared_data);
void copy_population_from_shared(Path** population, int pop_size, SharedData* shared_data);

// ===== Worker Process =====
void worker_process(int worker_id, int shm_id, int sem_id, 
                   const Grid* grid, const Config* config);

// ===== Work Distribution =====
void distribute_work(SharedData* shared_data, int pop_size, int num_workers);
void wait_for_workers(SharedData* shared_data, int sem_id, int num_workers);

// ===== Process Pool Management =====
pid_t* create_worker_pool(int num_workers, int shm_id, int sem_id,
                          const Grid* grid, const Config* config);
void terminate_workers(pid_t* worker_pids, int num_workers);

// ===== Parallel Fitness Evaluation =====
void parallel_evaluate_fitness(Path** population, int pop_size,
                              const Grid* grid, const Config* config,
                              SharedData* shared_data, int sem_id);

#endif // MULTIPROCESS_H
