#include "multiprocess.h"
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

// Semaphore operations structure
struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_unlock = {0, 1, 0};

// ===== Setup Functions =====
int setup_shared_memory(int* shm_id, SharedData** shared_data) {
    key_t key = ftok(".", 'R');
    if (key == -1) {
        error_exit("ftok failed");
    }
    
    *shm_id = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (*shm_id == -1) {
        error_exit("shmget failed");
    }
    
    *shared_data = (SharedData*)shmat(*shm_id, NULL, 0);
    if (*shared_data == (void*)-1) {
        error_exit("shmat failed");
    }
    
    // Initialize shared data
    (*shared_data)->population_size = 0;
    (*shared_data)->current_generation = 0;
    (*shared_data)->completed_tasks = 0;
    (*shared_data)->total_tasks = 0;
    (*shared_data)->best_fitness = 0.0f;
    (*shared_data)->termination_flag = 0;
    
    return 0;
}

int setup_semaphores(int* sem_id) {
    key_t key = ftok(".", 'S');
    if (key == -1) {
        error_exit("ftok for semaphore failed");
    }
    
    *sem_id = semget(key, 1, IPC_CREAT | 0666);
    if (*sem_id == -1) {
        error_exit("semget failed");
    }
    
    // Initialize semaphore to 1
    if (semctl(*sem_id, 0, SETVAL, 1) == -1) {
        error_exit("semctl SETVAL failed");
    }
    
    return 0;
}

void cleanup_ipc(int shm_id, int sem_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        warning("Failed to remove shared memory");
    }
    
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        warning("Failed to remove semaphore");
    }
}

// ===== Semaphore Operations =====
void sem_wait(int sem_id, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    if (semop(sem_id, &sb, 1) == -1) {
        error_exit("semop wait failed");
    }
}

void sem_signal(int sem_id, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    if (semop(sem_id, &sb, 1) == -1) {
        error_exit("semop signal failed");
    }
}

// ===== Worker Process =====
void worker_process(int worker_id, int shm_id, int sem_id,
                   const Grid* grid, const Config* config) {
    // Attach to shared memory
    SharedData* shared_data = (SharedData*)shmat(shm_id, NULL, 0);
    if (shared_data == (void*)-1) {
        fprintf(stderr, "Worker %d: shmat failed\n", worker_id);
        exit(1);
    }
    
    if (config->verbose) {
        printf("Worker %d: Started\n", worker_id);
    }
    
    // Worker loop - wait for tasks
    while (1) {
        sem_wait(sem_id, 0);
        
        if (shared_data->termination_flag) {
            sem_signal(sem_id, 0);
            break;
        }
        
        // Simulate work (in real implementation, process paths here)
        usleep(1000); // Small delay
        
        shared_data->completed_tasks++;
        
        sem_signal(sem_id, 0);
    }
    
    if (config->verbose) {
        printf("Worker %d: Terminated\n", worker_id);
    }
    
    shmdt(shared_data);
    exit(0);
}

// ===== Create Worker Pool =====
pid_t* create_worker_pool(int num_workers, int shm_id, int sem_id,
                          const Grid* grid, const Config* config) {
    pid_t* worker_pids = (pid_t*)safe_malloc(num_workers * sizeof(pid_t));
    
    for (int i = 0; i < num_workers; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            error_exit("fork failed");
        } else if (pid == 0) {
            // Child process
            worker_process(i, shm_id, sem_id, grid, config);
            exit(0); // Should never reach here
        } else {
            // Parent process
            worker_pids[i] = pid;
        }
    }
    
    return worker_pids;
}

// ===== Terminate Workers =====
void terminate_workers(pid_t* worker_pids, int num_workers) {
    for (int i = 0; i < num_workers; i++) {
        kill(worker_pids[i], SIGTERM);
        waitpid(worker_pids[i], NULL, 0);
    }
    free(worker_pids);
}
