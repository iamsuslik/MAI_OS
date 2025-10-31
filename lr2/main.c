#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sorting.h"

void write_exit(const char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        write(STDOUT_FILENO, "Использование: ./lab2 <размер_массива> <количество_потоков>\n", 62);
        exit(EXIT_FAILURE);
    }

    int array_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    if (array_size <= 0) write_exit("Ошибка: размер массива должен быть положительным числом\n");
    if (num_threads <= 0) write_exit("Ошибка: количество потоков должно быть положительным числом\n");

    if (num_threads > array_size) {
        char buf[128];
        sprintf(buf, "Предупреждение: уменьшено количество потоков с %d до %d\n", num_threads, array_size);
        write(STDOUT_FILENO, buf, strlen(buf));
        num_threads = array_size;
    }

    int *a_par = malloc(array_size * sizeof(int));
    int *a_seq = malloc(array_size * sizeof(int));
    if (!a_par || !a_seq) write_exit("Ошибка: malloc failed\n");

    srand((unsigned)time(NULL));
    generate_array(a_par, array_size);
    memcpy(a_seq, a_par, array_size * sizeof(int));

    double t0 = get_current_time();
    sequential_batcher_sort(a_seq, array_size);
    double seq_t = get_current_time() - t0;
    char buf[256];
    sprintf(buf, "Последовательная сортировка завершена за: %.4f секунд\n", seq_t);
    write(STDOUT_FILENO, buf, strlen(buf));

    t0 = get_current_time();
    parallel_batcher_sort(a_par, (size_t)array_size, num_threads);
    double par_t = get_current_time() - t0;
    sprintf(buf, "Параллельная сортировка завершена за: %.4f секунд\n", par_t);
    write(STDOUT_FILENO, buf, strlen(buf));

    double speedup = seq_t / par_t;
    double eff = speedup / num_threads * 100.0;
    sprintf(buf, "Ускорение: %.2fx\nЭффективность: %.1f%%\n", speedup, eff);
    write(STDOUT_FILENO, buf, strlen(buf));

    if (!is_sorted(a_par, array_size) || !is_sorted(a_seq, array_size)) {
        write(STDOUT_FILENO, "ОШИБКА: массив(ы) не отсортирован(ы)\n", 36);
    }

    free(a_par);
    free(a_seq);
    return 0;
}
