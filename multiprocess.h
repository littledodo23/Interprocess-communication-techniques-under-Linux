#ifndef MULTIPROCESS_H
#define MULTIPROCESS_H

#include "utilities.h"
#include "path_generator.h"
#include "grid_environment.h"
#include "fitness.h"

// ===== Shared Memory Structure =====
typedef struct {
    int population_size;
    int current_generation;
    int completed_tasks;
    int total_tasks;
    float best_fitness;
    int termination_flag;
} SharedData;

// ===== IPC Setup =====
int setup_shared_memory(int* shm_id, SharedData** shared_data);
int setup_semaphores(int* sem_id);
void cleanup_ipc(int shm_id, int sem_id);

// ===== Semaphore Operations =====
void sem_wait(int sem_id, int sem_num);
void sem_signal(int sem_id, int sem_num);

// ===== Worker Process =====
void worker_process(int worker_id, int shm_id, int sem_id, 
                   const Grid* grid, const Config* config);

// ===== Master Process =====
void master_process(Path** population, int pop_size, const Grid* grid, 
                   const Config* config, int shm_id, int sem_id);

// ===== Process Pool Management =====
pid_t* create_worker_pool(int num_workers, int shm_id, int sem_id,
                          const Grid* grid, const Config* config);
void terminate_workers(pid_t* worker_pids, int num_workers);

#endif // MULTIPROCESS_H
