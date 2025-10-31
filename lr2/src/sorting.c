#include "sorting.h"
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <limits.h>

double get_current_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void generate_array(int *array, int size) {
    for (int i = 0; i < size; ++i) array[i] = rand() % 10000;
}

int is_sorted(int *array, int size) {
    for (int i = 0; i + 1 < size; ++i)
        if (array[i] > array[i+1]) return 0;
    return 1;
}

static inline void compare_and_swap(int *arr, int i, int j) {
    if (arr[i] > arr[j]) {
        int t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }
}


void batcher_merge(int *arr, size_t low, size_t cnt, int dir) {
    if (cnt <= 1) return;
    size_t k = cnt / 2;
    for (size_t i = low; i < low + k; ++i) {
        if ((arr[i] > arr[i + k]) == dir)
            compare_and_swap(arr, i, i + k);
    }
    batcher_merge(arr, low, k, dir);
    batcher_merge(arr, low + k, k, dir);
}

void batcher_sort_recursive(int *arr, size_t low, size_t cnt, int dir) {
    if (cnt <= 1) return;
    size_t k = cnt / 2;
    batcher_sort_recursive(arr, low, k, 1);
    batcher_sort_recursive(arr, low + k, k, 0);
    batcher_merge(arr, low, cnt, dir);
}


void sequential_batcher_sort(int arr[], int n) {
    size_t size = 1;
    while (size < (size_t)n) size <<= 1;
    int *tmp = malloc(size * sizeof(int));
    if (!tmp) { perror("malloc"); exit(EXIT_FAILURE); }
    memcpy(tmp, arr, n * sizeof(int));
    for (size_t i = n; i < size; ++i) tmp[i] = INT_MAX;
    batcher_sort_recursive(tmp, 0, size, 1);
    memcpy(arr, tmp, n * sizeof(int));
    free(tmp);
}


void barrier_wait(pthread_mutex_t *mutex, pthread_cond_t *cond, int *counter, int num_threads) {
    pthread_mutex_lock(mutex);
    (*counter)++;
    if (*counter == num_threads) {
        *counter = 0;
        pthread_cond_broadcast(cond);
    } else {
        pthread_cond_wait(cond, mutex);
    }
    pthread_mutex_unlock(mutex);
}


static void *worker_func(void *vargs) {
    WorkerArgs *w = (WorkerArgs*)vargs;
    int *arr = w->arr;
    size_t start = w->start;
    size_t len = w->len;
    size_t size = w->size;
    int id = w->thread_id;
    int P = w->num_threads;

    size_t loc_sz = 1;
    while (loc_sz < len) loc_sz <<= 1;
    int *tmp = malloc(loc_sz * sizeof(int));
    if (!tmp) exit(EXIT_FAILURE);
    memcpy(tmp, arr + start, len * sizeof(int));
    for (size_t i = len; i < loc_sz; ++i) tmp[i] = INT_MAX;
    batcher_sort_recursive(tmp, 0, loc_sz, 1);
    memcpy(arr + start, tmp, len * sizeof(int));
    free(tmp);

    barrier_wait(w->mutex, w->cond, w->counter, P);

    size_t step = len;
    while (step < size) {
        size_t jobs = size / (2 * step);
        for (size_t j = id; j < jobs; j += (size_t)P) {
            size_t start_job = j * 2 * step;
            size_t cnt = 2 * step;
            batcher_merge(arr, start_job, cnt, 1);
        }
        barrier_wait(w->mutex, w->cond, w->counter, P);
        step *= 2;
    }

    return NULL;
}


void parallel_batcher_sort(int *array, size_t n, int num_threads) {
    if (n == 0) return;
    size_t size = 1;
    while (size < n) size <<= 1;

    int *buf = malloc(size * sizeof(int));
    if (!buf) { perror("malloc"); exit(EXIT_FAILURE); }
    memcpy(buf, array, n * sizeof(int));
    for (size_t i = n; i < size; ++i) buf[i] = INT_MAX;

    int P = 1;
    while ((P << 1) <= num_threads && (size_t)(P << 1) <= size) P <<= 1;
    if (P < 1) P = 1;

    size_t seg_len = size / (size_t)P;

    pthread_t *thr = malloc(P * sizeof(pthread_t));
    WorkerArgs *wargs = malloc(P * sizeof(WorkerArgs));
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int counter = 0;

    for (int t = 0; t < P; ++t) {
        wargs[t].arr = buf;
        wargs[t].start = (size_t)t * seg_len;
        wargs[t].len = seg_len;
        wargs[t].thread_id = t;
        wargs[t].num_threads = P;
        wargs[t].size = size;
        wargs[t].mutex = &mutex;
        wargs[t].cond = &cond;
        wargs[t].counter = &counter;

        if (pthread_create(&thr[t], NULL, worker_func, &wargs[t]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int t = 0; t < P; ++t) pthread_join(thr[t], NULL);

    memcpy(array, buf, n * sizeof(int));

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    free(buf);
    free(thr);
    free(wargs);
}
