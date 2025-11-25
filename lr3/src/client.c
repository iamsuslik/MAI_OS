#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>

#define SHM_SIZE 4096

int main(int argc, char *argv[])
{
    if (argc < 3) {
        const char msg[] = "Usage: client <filename> <server_pid>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    int server_pid = atoi(argv[2]);
    
    char shm_name[32], sem_req_name[32], sem_res_name[32];
    snprintf(shm_name, sizeof(shm_name), "/prime-shm-%d", server_pid);
    snprintf(sem_req_name, sizeof(sem_req_name), "/prime-req-%d", server_pid);
    snprintf(sem_res_name, sizeof(sem_res_name), "/prime-res-%d", server_pid);

    int shm = shm_open(shm_name, O_RDWR, 0);
    if (shm == -1) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    sem_t *sem_req = sem_open(sem_req_name, O_RDWR);
    sem_t *sem_res = sem_open(sem_res_name, O_RDWR);
    if (sem_req == SEM_FAILED || sem_res == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphores\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        const char msg[] = "error: cannot open file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    uint32_t *length = (uint32_t *)shm_buf;
    char *data = shm_buf + sizeof(uint32_t);
    bool running = true;
    char line[128];

    while (running && fgets(line, sizeof(line), file)) {
        sem_wait(sem_req);
        
        *length = strlen(line);
        memcpy(data, line, *length);
        
        sem_post(sem_res);
        
        sem_wait(sem_req);
        
        if (*length == UINT32_MAX) {
            running = false;
        } else if (*length > 0) {
            write(STDOUT_FILENO, data, *length);
            sem_post(sem_req);
        }
        
        if (!running) {
            break;
        }
    }

    if (running) {
        sem_wait(sem_req);
        *length = UINT32_MAX;
        sem_post(sem_res);
    }

    fclose(file);
    sem_close(sem_req);
    sem_close(sem_res);
    munmap(shm_buf, SHM_SIZE);
    close(shm);
    return 0;
}
