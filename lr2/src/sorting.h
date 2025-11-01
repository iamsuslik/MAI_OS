#ifndef SORTING_H
#define SORTING_H

#include <stddef.h>

double get_current_time(void);
void generate_array(int *array, int size);
int is_sorted(int *array, int size);
void sequential_batcher_sort(int arr[], int n);

void parallel_batcher_sort(int *array, int n, int num_threads);

#endif
