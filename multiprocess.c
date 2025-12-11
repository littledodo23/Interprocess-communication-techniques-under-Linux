#include "multiprocess.h"

// ===== Shared Memory Setup =====

int setup_shared_memory(int* shm_id, SharedData** shared_data) {
    key_t key = ftok(".", 'R');
    if (key == -1) {
        perror("ftok failed");
        return -1;
    }
    
    *shm_id = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (*shm_id == -1) {
        perror("shmget failed");
        return -1;
    }
    
    *shared_data = (SharedData*)shmat(*shm_id, NULL, 0);
    if (*shared_data == (void*)-1) {
        perror("shmat failed");
        return -1;
    }
    
    memset(*shared_data, 0, sizeof(SharedData));
    
    return 0;
}

// ===== Semaphore Setup =====

int setup_semaphores(int* sem_id) {
    key_t key = ftok(".", 'S');
    if (key == -1) {
        perror("ftok failed");
        return -1;
    }
    
    *sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (*sem_id == -1) {
        perror("semget failed");
        return -1;
    }
    
    if (semctl(*sem_id, 0, SETVAL, 1) == -1) {
        perror("semctl SETVAL failed");
        return -1;
    }
    
    return 0;
}

// ===== Semaphore Operations =====

void sem_wait(int sem_id, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    
    if (semop(sem_id, &sb, 1) == -1) {
        perror("semop wait failed");
    }
}

void sem_signal(int sem_id, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    
    if (semop(sem_id, &sb, 1) == -1) {
        perror("semop signal failed");
    }
}

// ===== Path Transfer Functions =====

void path_to_shared(const Path* path, SharedPath* shared_path) {
    if (!path || !shared_path) return;
    
    shared_path->length = path->length;
    shared_path->survivors_reached = path->survivors_reached;
    shared_path->fitness = path->fitness;
    shared_path->collision_count = path->collision_count;
    
    int copy_len = path->length < MAX_PATH_LENGTH ? path->length : MAX_PATH_LENGTH;
    memcpy(shared_path->coordinates, path->coordinates, 
           copy_len * sizeof(Coordinate));
}

void shared_to_path(const SharedPath* shared_path, Path* path) {
    if (!shared_path || !path) return;
    
    path->length = shared_path->length;
    path->survivors_reached = shared_path->survivors_reached;
    path->fitness = shared_path->fitness;
    path->collision_count = shared_path->collision_count;
    
    if (path->length > path->capacity) {
        path->capacity = path->length + 100;
        path->coordinates = (Coordinate*)realloc(path->coordinates,
                                                 path->capacity * sizeof(Coordinate));
    }
    
    memcpy(path->coordinates, shared_path->coordinates,
           path->length * sizeof(Coordinate));
}

void copy_population_to_shared(Path** population, int pop_size, SharedData* shared_data) {
    if (!population || !shared_data) return;
    
    shared_data->population_size = pop_size;
    
    for (int i = 0; i < pop_size && i < MAX_POPULATION; i++) {
        path_to_shared(population[i], &shared_data->paths[i]);
    }
}

void copy_population_from_shared(Path** population, int pop_size, SharedData* shared_data) {
    if (!population || !shared_data) return;
    
    for (int i = 0; i < pop_size && i < MAX_POPULATION; i++) {
        shared_to_path(&shared_data->paths[i], population[i]);
    }
}

// ===== Work Distribution =====

void distribute_work(SharedData* shared_data, int pop_size, int num_workers) {
    if (!shared_data || num_workers <= 0) return;
    
    shared_data->num_workers = num_workers;
    shared_data->workers_completed = 0;
    shared_data->work_ready = 1;
    
    int paths_per_worker = pop_size / num_workers;
    int remainder = pop_size % num_workers;
    
    int current_start = 0;
    for (int i = 0; i < num_workers; i++) {
        int range_size = paths_per_worker + (i < remainder ? 1 : 0);
        
        shared_data->worker_ranges[i].worker_id = i;
        shared_data->worker_ranges[i].start_idx = current_start;
        shared_data->worker_ranges[i].end_idx = current_start + range_size;
        
        current_start += range_size;
    }
}

void wait_for_workers(SharedData* shared_data, int sem_id, int num_workers) {
    if (!shared_data) return;
    
    while (1) {
        sem_wait(sem_id, 0);
        int completed = shared_data->workers_completed;
        sem_signal(sem_id, 0);
        
        if (completed >= num_workers) {
            break;
        }
        
        usleep(1000); // 1ms
    }
    
    // Reset for next generation
    sem_wait(sem_id, 0);
    shared_data->work_ready = 0;
    shared_data->workers_completed = 0;
    sem_signal(sem_id, 0);
}

// ===== Worker Process (ACTUALLY DOES WORK NOW!) =====

void worker_process(int worker_id, int shm_id, int sem_id, 
                   const Grid* grid, const Config* config) {
    
    SharedData* shared_data = (SharedData*)shmat(shm_id, NULL, 0);
    if (shared_data == (void*)-1) {
        perror("Worker: shmat failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Worker %d (PID: %d) started and ready\n", worker_id, getpid());
    fflush(stdout);
    
    // Seed random with different value for each worker
    srand(time(NULL) + getpid() + worker_id * 1000);
    
    while (1) {
        // Wait for work to be available
        sem_wait(sem_id, 0);
        int should_terminate = shared_data->termination_flag;
        int work_ready = shared_data->work_ready;
        int my_start = shared_data->worker_ranges[worker_id].start_idx;
        int my_end = shared_data->worker_ranges[worker_id].end_idx;
        sem_signal(sem_id, 0);
        
        if (should_terminate) {
            break;
        }
        
        if (!work_ready) {
            usleep(10000); // 10ms - wait for work
            continue;
        }
        
        // DO THE ACTUAL WORK: Evaluate fitness for assigned paths
        for (int i = my_start; i < my_end; i++) {
            SharedPath* sp = &shared_data->paths[i];
            
            // Create temporary path structure
            Path temp_path;
            temp_path.coordinates = sp->coordinates;
            temp_path.length = sp->length;
            temp_path.capacity = MAX_PATH_LENGTH;
            temp_path.survivors_reached = 0;
            temp_path.survivors_visited = NULL;
            temp_path.fitness = 0.0f;
            temp_path.collision_count = 0;
            
            // Calculate fitness components
            temp_path.survivors_reached = calculate_survivors_reached(&temp_path, grid);
            temp_path.collision_count = check_path_collisions(&temp_path, grid);
            temp_path.fitness = calculate_fitness(&temp_path, grid, config);
            
            // Update shared memory
            sem_wait(sem_id, 0);
            sp->survivors_reached = temp_path.survivors_reached;
            sp->collision_count = temp_path.collision_count;
            sp->fitness = temp_path.fitness;
            
            // Track best fitness
            if (temp_path.fitness > shared_data->best_fitness) {
                shared_data->best_fitness = temp_path.fitness;
                shared_data->best_path_index = i;
            }
            sem_signal(sem_id, 0);
        }
        
        // Signal completion
        sem_wait(sem_id, 0);
        shared_data->workers_completed++;
        int completed = shared_data->workers_completed;
        sem_signal(sem_id, 0);
        
        if (config->verbose && completed % shared_data->num_workers == 0) {
            printf("  Worker %d completed evaluation of paths %d-%d\n", 
                   worker_id, my_start, my_end - 1);
            fflush(stdout);
        }
    }
    
    shmdt(shared_data);
    
    printf("Worker %d (PID: %d) terminating\n", worker_id, getpid());
    exit(EXIT_SUCCESS);
}

// ===== Process Pool Management =====

pid_t* create_worker_pool(int num_workers, int shm_id, int sem_id,
                          const Grid* grid, const Config* config) {
    pid_t* worker_pids = (pid_t*)safe_malloc(num_workers * sizeof(pid_t));
    
    for (int i = 0; i < num_workers; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            error_exit("Fork failed");
        } else if (pid == 0) {
            // Child process
            worker_process(i, shm_id, sem_id, grid, config);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            worker_pids[i] = pid;
        }
    }
    
    return worker_pids;
}

void terminate_workers(pid_t* worker_pids, int num_workers) {
    if (!worker_pids) return;
    
    for (int i = 0; i < num_workers; i++) {
        int status;
        waitpid(worker_pids[i], &status, 0);
        
        if (WIFEXITED(status)) {
            if (status != 0) {
                printf("Worker %d exited with status %d\n", i, WEXITSTATUS(status));
            }
        }
    }
    
    free(worker_pids);
}

// ===== Parallel Fitness Evaluation (NEW FUNCTION) =====

void parallel_evaluate_fitness(Path** population, int pop_size,
                              const Grid* grid, const Config* config,
                              SharedData* shared_data, int sem_id) {
    (void)grid;   // Suppress unused warnings
    (void)config; // Grid and config are used by workers
    
    if (!population || !shared_data || pop_size <= 0) return;
    
    // Copy population to shared memory
    copy_population_to_shared(population, pop_size, shared_data);
    
    // Distribute work to workers
    sem_wait(sem_id, 0);
    distribute_work(shared_data, pop_size, shared_data->num_workers);
    sem_signal(sem_id, 0);
    
    // Wait for all workers to complete
    wait_for_workers(shared_data, sem_id, shared_data->num_workers);
    
    // Copy results back from shared memory
    copy_population_from_shared(population, pop_size, shared_data);
}

// ===== Cleanup =====

void cleanup_ipc(int shm_id, int sem_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID failed");
    }
    
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed");
    }
}
