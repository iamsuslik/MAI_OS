#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "libs.h"

extern float area(float a, float b);
extern char* convert(int x);

#define BUFFER_SIZE 1024

static float area_stub(float a, float b) {
    (void)a; (void)b;
    const char msg[] = "error: area function not available\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    return 0.0f;
}

static char* convert_stub(int x) {
    (void)x;
    const char msg[] = "error: convert function not available\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    char* result = (char*)malloc(2 * sizeof(char));
    if (result) {
        result[0] = '0';
        result[1] = '\0';
    }
    return result;
}

void process_command_1(char* args, int output_fd) {
    float a, b;
    if (sscanf(args, "%f %f", &a, &b) == 2) {
        float result = area(a, b);
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
        char* result = convert(x);
        char buf[BUFFER_SIZE];
        int length = snprintf(buf, sizeof(buf) - 1, 
                             "Number %d in binary: %s\n", x, result);
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

                if (strcmp(token, "0") == 0) {
                    const char info[] = "Static linking: using library 1\n"
                                        "  - Area: rectangle (a * b)\n"
                                        "  - Convert: to binary system\n";
                    write(STDOUT_FILENO, info, sizeof(info) - 1);
                } else {
                    char* args = token + strlen(token) + 1;
                    if (*args == '\0') args = NULL;

                    int cmd = atoi(token);
                    switch (cmd) {
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
            }

            line = line_end + 1;
        }
    }

    close(fd);
    return 1;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        process_input_file(argv[1]);
    } else {
        const char msg[] = "Program 1 (Static Linking)\n"
                           "Commands: 0 | 1 A B | 2 X | exit\n"
                           "Usage: ./prog1 [commands_file.txt]\n"
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

            if (strcmp(token, "0") == 0) {
                const char info[] = "Current implementation: Library 1\n"
                                    "  - Area: rectangle (a * b)\n"
                                    "  - Convert: to binary system\n"
                                    "Cannot switch implementations in static linking\n"
                                    "> ";
                write(STDOUT_FILENO, info, sizeof(info) - 1);
                continue;
            }

            char* args = token + strlen(token) + 1;
            if (*args == '\0') args = NULL;

            int cmd = atoi(token);
            switch (cmd) {
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

    return 0;
}
