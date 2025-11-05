#include "sorting.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>


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
        if (array[i] > array[i + 1]) return 0;
    }
    return 1;
}


void sequential_batcher_sort(int arr[], int n) {
    if (n <= 1) return;
    int sorted;
    do {
        sorted = 1;
        for (int i = 0; i < n - 1; i += 2) {
            if (arr[i] > arr[i + 1]) {
                int tmp = arr[i]; arr[i] = arr[i + 1]; arr[i + 1] = tmp;
                sorted = 0;
            }
        }
        for (int i = 1; i < n - 1; i += 2) {
            if (arr[i] > arr[i + 1]) {
                int tmp = arr[i]; arr[i] = arr[i + 1]; arr[i + 1] = tmp;
                sorted = 0;
            }
        }
    } while (!sorted);
}


typedef struct {
    int *array;
    int start;
    int len;
} BlockArgs;

static void *sort_block_thread(void *arg) {
    BlockArgs *args = (BlockArgs*)arg;
    sequential_batcher_sort(args->array + args->start, args->len);
    return NULL;
}

void parallel_batcher_sort(int *array, int n, int num_threads) {
    if (n <= 1 || num_threads <= 1) {
        sequential_batcher_sort(array, n);
        return;
    }

    int P = num_threads;
    if (P > n/2) P = n/2;
    

    pthread_t threads[P];
    BlockArgs args[P];
    
    int block_size = n / P;
    
    for (int i = 0; i < P; i++) {
        args[i].array = array;
        args[i].start = i * block_size;
        args[i].len = (i == P - 1) ? (n - i * block_size) : block_size;
        pthread_create(&threads[i], NULL, sort_block_thread, &args[i]);
    }
    

    for (int i = 0; i < P; i++) {
        pthread_join(threads[i], NULL);
    }
    

    sequential_batcher_sort(array, n);
}
