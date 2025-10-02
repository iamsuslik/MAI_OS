#include <windows.h>
#include <stdio.h>
#include <string.h>

static char SERVER_PROGRAM_NAME[] = "server.exe";

int main(int argc, char* argv[]) {
    if (argc < 2) {
        const char msg[] = "Usage: client.exe <filename>\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg)-1, NULL, NULL);
        return 1;
    }

    char filename[256];
    strcpy(filename, argv[1]);

    // pipe: child stdout → parent
    HANDLE pipe[2];
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&pipe[0], &pipe[1], &sa, 0)) {
        const char msg[] = "ERROR: cannot create pipe\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg)-1, NULL, NULL);
        return 1;
    }

    // путь к server.exe
    char serverPath[MAX_PATH];
    GetModuleFileNameA(NULL, serverPath, sizeof(serverPath));
    char *lastSlash = strrchr(serverPath, '\\');
    if (lastSlash) *lastSlash = '\0';
    strcat(serverPath, "\\");
    strcat(serverPath, SERVER_PROGRAM_NAME);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    // pipe для stdin сервера
    HANDLE pipe_stdin[2];
    if (!CreatePipe(&pipe_stdin[0], &pipe_stdin[1], &sa, 0)) {
        const char msg[] = "ERROR: cannot create stdin pipe\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg)-1, NULL, NULL);
        return 1;
    }

    si.hStdInput  = pipe_stdin[0];
    si.hStdOutput = pipe[1];
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
    si.dwFlags = STARTF_USESTDHANDLES;

    if (!CreateProcessA(
        serverPath,
        NULL,
        NULL,
        NULL,
        TRUE, // наследовать дескрипторы
        0,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        const char msg[] = "ERROR: cannot start server.exe\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg)-1, NULL, NULL);
        return 1;
    }

    CloseHandle(pipe[1]);       // закрываем запись stdout сервера в родителе
    CloseHandle(pipe_stdin[0]); // закрываем чтение stdin сервера в родителе

    // читаем файл построчно и пишем в stdin сервера
    FILE *file = fopen(filename, "r");
    if (!file) {
        const char msg[] = "ERROR: cannot open file\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, sizeof(msg)-1, NULL, NULL);
        return 1;
    }

    char line[128];
    DWORD written;
    while (fgets(line, sizeof(line), file)) {
        WriteFile(pipe_stdin[1], line, (DWORD)strlen(line), &written, NULL);
    }
    fclose(file);
    CloseHandle(pipe_stdin[1]);

    // читаем вывод сервера и пишем в консоль
    char buffer[256];
    DWORD bytesRead;
    while (ReadFile(pipe[0], buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, bytesRead, NULL, NULL);
    }

    CloseHandle(pipe[0]);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return 0;
}


