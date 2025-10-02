#include <windows.h>
#include <stdio.h>

int is_prime(int n) {
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++)
        if (n % i == 0) return 0;
    return 1;
}

int main() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    char line[128];

    while (fgets(line, sizeof(line), stdin)) {
        int num;
        if (sscanf(line, "%d", &num) == 1) {
            if (num < 0 || is_prime(num)) {
                char msg[128];
                int len = sprintf(msg, "Stop number found: %d\nProgram finished.\n", num);
                DWORD written;
                WriteFile(hOut, msg, len, &written, NULL);
                break;
            }
            char output[32];
            int len = sprintf(output, "%d\n", num);
            DWORD written;
            WriteFile(hOut, output, len, &written, NULL);
        }
    }
    return 0;
}

