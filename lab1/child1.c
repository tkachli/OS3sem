#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZ 4096

void handling(char* str, char* result) {
    char* p_str = str;
    char* p_result = result;

    while (*p_str) {
        if (*p_str != 'a' && *p_str != 'e' && *p_str != 'i' && *p_str != 'o' && *p_str != 'u' &&
            *p_str != 'A' && *p_str != 'E' && *p_str != 'I' && *p_str != 'O' && *p_str != 'U') {
            *p_result++ = *p_str;
            }
        ++p_str;
    }

    *p_result = '\0';
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        const char* msg = "Invalid amount parametres\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }

    char buffer[BUFSIZ];
    ssize_t bytes;

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
    if (file == -1) {
        const char* msg = "Error: failed to open file\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }

    char result[BUFSIZ];
    while(bytes = read(STDIN_FILENO, buffer, BUFSIZ)) {
        buffer[bytes - 1] = '\0';
        handling(buffer, result);

        write(STDOUT_FILENO, result, strlen(result));
        write(STDOUT_FILENO, "\n", 1);

        write(file, result, strlen(result));
        write(file, "\n", 1);

    }

    close(file);

    return 0;
}
