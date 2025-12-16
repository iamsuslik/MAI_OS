#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>

#include "libs.h"

#define BUFFER_SIZE 1024

static area_func* area_impl = NULL;
static convert_func* convert_impl = NULL;
static void* library_handle = NULL;
static const char* LIB_NAMES[] = {"./lib1.so", "./lib2.so"};
static int current_lib = 0;


static float area_stub(float a, float b) {
    (void)a; (void)b;
    const char msg[] = "warning: area function not available\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    return 0.0f;
}

static char* convert_stub(int x) {
    (void)x;
    const char msg[] = "warning: convert function not available\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    char* result = (char*)malloc(2 * sizeof(char));
    if (result) {
        result[0] = '0';
        result[1] = '\0';
    }
    return result;
}

int load_library(int lib_index) {
    if (library_handle) {
        dlclose(library_handle);
        library_handle = NULL;
    }

    library_handle = dlopen(LIB_NAMES[lib_index], RTLD_LAZY);

    if (library_handle == NULL) {
        const char msg[] = "warning: library failed to load\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);

        area_impl = area_stub;
        convert_impl = convert_stub;
        return 0;
    }

    area_impl = (area_func*)dlsym(library_handle, "area");
    if (area_impl == NULL) {
        const char msg[] = "warning: failed to find area function implementation\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        area_impl = area_stub;
    }

    convert_impl = (convert_func*)dlsym(library_handle, "convert");
    if (convert_impl == NULL) {
        const char msg[] = "warning: failed to find convert function implementation\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        convert_impl = convert_stub;
    }

    current_lib = lib_index;
    return 1;
}

void switch_library() {
    int new_lib = (current_lib == 0) ? 1 : 0;

    char buf[BUFFER_SIZE];
    int length = snprintf(buf, sizeof(buf) - 1, 
                         "Switching from library %d to library %d\n",
                         current_lib + 1, new_lib + 1);
    buf[length] = '\0';
    write(STDOUT_FILENO, buf, length);

    if (load_library(new_lib)) {
        const char* msg = (new_lib == 0) ? 
            "Loaded library 1: rectangle area, binary conversion\n" :
            "Loaded library 2: triangle area, ternary conversion\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    }
}

void process_command_1(char* args, int output_fd) {
    float a, b;
    if (sscanf(args, "%f %f", &a, &b) == 2) {
        float result = area_impl(a, b);
        char buf[BUFFER_SIZE];
        int length = snprintf(buf, sizeof(buf) - 1, 
                             "Area with sides %.2f and %.2f = %.2f\n", 
                             a, b, result);
        buf[length] = '\0';
        write(output_fd, buf, length);
    } else {
        const char msg[] = "error: wrong arguments for command 1\n"
                           "usage: 1 A B\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
}

void process_command_2(char* args, int output_fd) {
    int x;
    if (sscanf(args, "%d", &x) == 1) {
        char* result = convert_impl(x);
        char buf[BUFFER_SIZE];
        const char* system_name = (current_lib == 0) ? "binary" : "ternary";
        int length = snprintf(buf, sizeof(buf) - 1, 
                             "Number %d in %s: %s\n", x, system_name, result);
        buf[length] = '\0';
        write(output_fd, buf, length);
        free(result);
    } else {
        const char msg[] = "error: wrong argument for command 2\n"
                           "usage: 2 X\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
}

int process_input_file(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        char buf[BUFFER_SIZE];
        int len = snprintf(buf, sizeof(buf), "error: cannot open file %s\n", filename);
        write(STDERR_FILENO, buf, len);
        return 0;
    }

    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';

        char* line = buffer;
        char* line_end;
        
        while ((line_end = strchr(line, '\n')) != NULL) {
            *line_end = '\0';

            if (strlen(line) > 0) {
                char* token = strtok(line, " \t");
                if (!token) continue;
                
                if (strcmp(token, "exit") == 0) {
                    close(fd);
                    return 1;
                }

                char* args = token + strlen(token) + 1;
                if (*args == '\0') args = NULL;

                int cmd = atoi(token);
                switch (cmd) {
                    case 0:
                        switch_library();
                        break;
                    case 1:
                        process_command_1(args, STDOUT_FILENO);
                        break;
                    case 2:
                        process_command_2(args, STDOUT_FILENO);
                        break;
                    default:
                        const char err[] = "error: unknown command\n";
                        write(STDERR_FILENO, err, sizeof(err) - 1);
                        break;
                }
            }

            line = line_end + 1;
        }
    }

    close(fd);
    return 1;
}

int main(int argc, char** argv) {
    area_impl = area_stub;
    convert_impl = convert_stub;

    if (!load_library(0)) {
        const char msg[] = "warning: using stub implementations\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (argc > 1) {
        process_input_file(argv[1]);
    } else {
        const char msg[] = "Program 2 (Dynamic Loading)\n"
                           "Commands: 0 | 1 A B | 2 X | exit\n"
                           "Usage: ./prog2 [commands_file.txt]\n"
                           "> ";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);

        int bytes_read = 0;
        char buffer[BUFFER_SIZE];

        while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[bytes_read] = '\0';

            char* newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';

            char* token = strtok(buffer, " \t\n");
            if (!token) {
                write(STDOUT_FILENO, "> ", 2);
                continue;
            }

            if (strcmp(token, "exit") == 0) {
                break;
            }

            char* args = token + strlen(token) + 1;
            if (*args == '\0') args = NULL;

            int cmd = atoi(token);
            switch (cmd) {
                case 0:
                    switch_library();
                    break;
                case 1:
                    process_command_1(args, STDOUT_FILENO);
                    break;
                case 2:
                    process_command_2(args, STDOUT_FILENO);
                    break;
                default:
                    const char err[] = "error: unknown command\n";
                    write(STDERR_FILENO, err, sizeof(err) - 1);
                    break;
            }

            write(STDOUT_FILENO, "> ", 2);
        }
    }

    if (library_handle) {
        dlclose(library_handle);
    }

    return 0;
}
