#ifndef SORTING_H
#define SORTING_H

#include <stddef.h>
#include <pthread.h>

typedef struct {
    int *arr;
    size_t start;
    size_t len;
    int thread_id;
    int num_threads;
    size_t size;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    int *counter;
} WorkerArgs;


double get_current_time(void);
void generate_array(int *array, int size);
int is_sorted(int *array, int size);

void sequential_batcher_sort(int arr[], int n);

void parallel_batcher_sort(int *array, size_t n, int num_threads);

#endif
