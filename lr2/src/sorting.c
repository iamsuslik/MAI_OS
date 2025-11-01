#include "sorting.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

double get_current_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void generate_array(int *array, int size) {
    for (int i = 0; i < size; ++i) array[i] = rand() % 10000;
}

int is_sorted(int *array, int size) {
    for (int i = 0; i + 1 < size; ++i) {
        if (array[i] > array[i+1]) {
            return 0;
        }
    }
    return 1;
}


void sequential_batcher_sort(int arr[], int n) {
    if (n <= 1) return;
    
    int sorted;
    do {
        sorted = 1;
        
        // Четная фаза
        for (int i = 0; i < n - 1; i += 2) {
            if (arr[i] > arr[i + 1]) {
                int temp = arr[i];
                arr[i] = arr[i + 1];
                arr[i + 1] = temp;
                sorted = 0;
            }
        }
        
        // Нечетная фаза
        for (int i = 1; i < n - 1; i += 2) {
            if (arr[i] > arr[i + 1]) {
                int temp = arr[i];
                arr[i] = arr[i + 1];
                arr[i + 1] = temp;
                sorted = 0;
            }
        }
    } while (!sorted);
}


typedef struct {
    int *arr;
    int start;
    int len;
    pthread_mutex_t *block_mutex;
} BlockArgs;


void sort_block(int *arr, int start, int len) {
    sequential_batcher_sort(arr + start, len);
}


static void *block_sort_thread(void *vargs) {
    BlockArgs *args = (BlockArgs*)vargs;

    pthread_mutex_lock(args->block_mutex);
    sort_block(args->arr, args->start, args->len);
    pthread_mutex_unlock(args->block_mutex);
    
    free(args);
    return NULL;
}


void parallel_batcher_sort(int *array, int n, int num_threads) {
    if (n <= 1) return;
    
    if (n < 2000) {
        sequential_batcher_sort(array, n);
        return;
    }
    
    int P = num_threads;
    if (P > n / 200) P = n / 200;
    if (P < 1) P = 1;
    if (P > 8) P = 8;
    
    int *buf = malloc(n * sizeof(int));
    if (!buf) { 
        char msg[] = "Ошибка: malloc failed\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    memcpy(buf, array, n * sizeof(int));

    pthread_mutex_t *mutexes = malloc(P * sizeof(pthread_mutex_t));
    for (int i = 0; i < P; i++) {
        if (pthread_mutex_init(&mutexes[i], NULL) != 0) {
            char msg[] = "Ошибка: pthread_mutex_init failed\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }
    
    pthread_t *threads = malloc(P * sizeof(pthread_t));
    int block_size = n / P;
    
    for (int i = 0; i < P; i++) {
        int start = i * block_size;
        int len = (i == P - 1) ? (n - start) : block_size;
        
        BlockArgs *block_args = malloc(sizeof(BlockArgs));
        if (!block_args) {
            char msg[] = "Ошибка: malloc failed\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        
        block_args->arr = buf;
        block_args->start = start;
        block_args->len = len;
        block_args->block_mutex = &mutexes[i];
        
        if (pthread_create(&threads[i], NULL, block_sort_thread, block_args) != 0) {
            char msg[] = "Ошибка: pthread_create failed\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < P; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            char msg[] = "Ошибка: pthread_join failed\n";
            write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }


    for (int i = 0; i < P; i++) {
        pthread_mutex_destroy(&mutexes[i]);
    }
    free(mutexes);
    
    sequential_batcher_sort(buf, n);
    memcpy(array, buf, n * sizeof(int));
    
    free(buf);
    free(threads);
}
