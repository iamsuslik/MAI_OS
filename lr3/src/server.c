#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>

#define SHM_SIZE 4096

int is_prime(int n) {
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++)
        if (n % i == 0) return 0;
    return 1;
}

typedef struct {
    int shm_fd;
    char *shm_buf;
    sem_t *sem_req;
    sem_t *sem_res;
    char shm_name[32];
    char sem_req_name[32];
    char sem_res_name[32];
} shared_resources_t;

void cleanup_resources(shared_resources_t *res) {
    if (res->sem_req != SEM_FAILED) {
        sem_close(res->sem_req);
        sem_unlink(res->sem_req_name);
    }
    if (res->sem_res != SEM_FAILED) {
        sem_close(res->sem_res);
        sem_unlink(res->sem_res_name);
    }
    if (res->shm_buf != MAP_FAILED) {
        munmap(res->shm_buf, SHM_SIZE);
    }
    if (res->shm_fd != -1) {
        close(res->shm_fd);
        shm_unlink(res->shm_name);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        const char msg[] = "Usage: server <filename>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    shared_resources_t res = {0};
    pid_t client = -1;
    
    pid_t server_pid = getpid();
    snprintf(res.shm_name, sizeof(res.shm_name), "/prime-shm-%d", server_pid);
    snprintf(res.sem_req_name, sizeof(res.sem_req_name), "/prime-req-%d", server_pid);
    snprintf(res.sem_res_name, sizeof(res.sem_res_name), "/prime-res-%d", server_pid);

    res.shm_fd = shm_open(res.shm_name, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (res.shm_fd == -1) {
        perror("ERROR: shm_open failed");
        cleanup_resources(&res);
        _exit(EXIT_FAILURE);
    }

    if (ftruncate(res.shm_fd, SHM_SIZE) == -1) {
        perror("ERROR: ftruncate failed");
        cleanup_resources(&res);
        _exit(EXIT_FAILURE);
    }

    res.shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, res.shm_fd, 0);
    if (res.shm_buf == MAP_FAILED) {
        perror("ERROR: mmap failed");
        cleanup_resources(&res);
        _exit(EXIT_FAILURE);
    }

    res.sem_req = sem_open(res.sem_req_name, O_CREAT | O_TRUNC, 0600, 1);
    res.sem_res = sem_open(res.sem_res_name, O_CREAT | O_TRUNC, 0600, 0);
    if (res.sem_req == SEM_FAILED || res.sem_res == SEM_FAILED) {
        perror("ERROR: sem_open failed");
        cleanup_resources(&res);
        _exit(EXIT_FAILURE);
    }

    client = fork();
    if (client == 0) {
        char server_pid_str[32];
        snprintf(server_pid_str, sizeof(server_pid_str), "%d", server_pid);
        char *args[] = {"./client", filename, server_pid_str, NULL};
        execv("./client", args);
        perror("ERROR: execv failed");
        _exit(EXIT_FAILURE);
    } else if (client == -1) {
        perror("ERROR: fork failed");
        cleanup_resources(&res);
        _exit(EXIT_FAILURE);
    }

    uint32_t *length = (uint32_t *)res.shm_buf;
    char *data = res.shm_buf + sizeof(uint32_t);
    bool running = true;
    
    while (running) {
        if (sem_wait(res.sem_res) == -1) {
            perror("ERROR: sem_wait failed");
            break;
        }
        
        if (*length == UINT32_MAX) {
            running = false;
        } else if (*length > 0) {
            int num;
            if (sscanf(data, "%d", &num) == 1) {
                if (num < 0 || is_prime(num)) {
                    *length = UINT32_MAX;
                    running = false;
                } else {
                    char response[32];
                    int response_len = snprintf(response, sizeof(response), "%d\n", num);
                    if (response_len > (SHM_SIZE - sizeof(uint32_t))) {
                        response_len = SHM_SIZE - sizeof(uint32_t);
                    }
                    *length = response_len;
                    memcpy(data, response, response_len);
                }
            } else {
                *length = 0;
            }
        }
        
        sem_post(res.sem_req);
        
        if (!running) {
            break;
        }
    }

    waitpid(client, NULL, 0);
    cleanup_resources(&res);
    return 0;
}
