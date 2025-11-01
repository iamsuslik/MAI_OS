#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "sorting.h"


static void write_exit(const char *msg) {
    size_t len = strlen(msg);
    ssize_t result = write(STDOUT_FILENO, msg, len);
    (void)result;
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        write_exit("Использование: ./lab2 <размер_массива> <количество_потоков>\n");
    }

    int array_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    if (array_size <= 0) {
        write_exit("Ошибка: размер массива должен быть положительным числом\n");
    }
    if (num_threads <= 0) {
        write_exit("Ошибка: количество потоков должно быть положительным числом\n");
    }

    int *original = malloc(array_size * sizeof(int));
    int *seq_result = malloc(array_size * sizeof(int));
    int *par_result = malloc(array_size * sizeof(int));
    
    if (!original || !seq_result || !par_result) {
        write_exit("Ошибка выделения памяти\n");
    }


    srand((unsigned)time(NULL));
    generate_array(original, array_size);
    memcpy(seq_result, original, array_size * sizeof(int));
    memcpy(par_result, original, array_size * sizeof(int));

    double seq_time = get_current_time();
    sequential_batcher_sort(seq_result, array_size);
    seq_time = get_current_time() - seq_time;

    double par_time = get_current_time();
    parallel_batcher_sort(par_result, array_size, num_threads);
    par_time = get_current_time() - par_time;

    double speedup = seq_time / par_time;
    double eff = (speedup / num_threads) * 100.0;

    int seq_correct = is_sorted(seq_result, array_size);
    int par_correct = is_sorted(par_result, array_size);

    char output[512];
    int len = 0;
    
    len += snprintf(output + len, sizeof(output) - len, 
                   "Последовательная сортировка завершена за: %.4f секунд\n", seq_time);
    len += snprintf(output + len, sizeof(output) - len,
                   "Параллельная сортировка завершена за: %.4f секунд\n", par_time);
    len += snprintf(output + len, sizeof(output) - len,
                   "Ускорение: %.2fx\n", speedup);
    len += snprintf(output + len, sizeof(output) - len,
                   "Эффективность: %.1f%%\n", eff);
    
    if (seq_correct && par_correct) {
        len += snprintf(output + len, sizeof(output) - len,
                       "Корректная сортировка: ДА\n");
    } else {
        len += snprintf(output + len, sizeof(output) - len,
                       "Корректная сортировка: НЕТ\n");
    }

    write(STDOUT_FILENO, output, len);

    free(original);
    free(seq_result);
    free(par_result);
    
    return 0;
}
