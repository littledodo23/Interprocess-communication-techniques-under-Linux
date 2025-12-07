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

// ===== Worker Process =====

void worker_process(int worker_id, int shm_id, int sem_id, 
                   const Grid* grid, const Config* config) {
    
    SharedData* shared_data = (SharedData*)shmat(shm_id, NULL, 0);
    if (shared_data == (void*)-1) {
        perror("Worker: shmat failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Worker %d (PID: %d) started\n", worker_id, getpid());
    fflush(stdout);
    
    srand(time(NULL) + getpid());
    
    while (1) {
        sem_wait(sem_id, 0);
        int should_terminate = shared_data->termination_flag;
        int current_gen = shared_data->current_generation;
        sem_signal(sem_id, 0);
        
        if (should_terminate) {
            break;
        }
        
        sem_wait(sem_id, 0);
        shared_data->completed_tasks++;
        sem_signal(sem_id, 0);
        
        usleep(10000);
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
            worker_process(i, shm_id, sem_id, grid, config);
            exit(EXIT_SUCCESS);
        } else {
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
            printf("Worker %d exited with status %d\n", i, WEXITSTATUS(status));
        }
    }
    
    free(worker_pids);
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
