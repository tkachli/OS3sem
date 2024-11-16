#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define BUFSIZ 4096

void filter_data(int pipe1[], int pipe2[], char* buffer) {
    if ((rand() % 100) < 80)
    {
        write(pipe1[1], buffer, strlen(buffer) + 1);
    }
    else
    {
        write(pipe2[1], buffer, strlen(buffer) + 1);
    }
}




int main(int argc, char* argv[]) {
    if (argc > 1) {
        const char* msg = "Required console input\n";
        write(STDERR_FILENO, msg, strlen(msg) + 1);
        exit(EXIT_FAILURE);
    }
    char buffer[BUFSIZ];
    int pipe1[2], pipe2[2];
    pid_t child1_pid, child2_pid;
    ssize_t bytes;
    int cnt_of_lines = 1;

    char file1[BUFSIZ];
    char file2[BUFSIZ];

    const char* msg1 = "Enter filename for child1\n";
    write(STDOUT_FILENO, msg1, strlen(msg1));
    read(STDIN_FILENO, buffer, BUFSIZ);
    buffer[strcspn(buffer, "\n")] = '\0';
    strcpy(file1, buffer);


    const char* msg2 = "Enter filename for child2\n";
    write(STDOUT_FILENO, msg2, strlen(msg1));
    read(STDIN_FILENO, buffer, BUFSIZ);
    buffer[strcspn(buffer, "\n")] = '\0';
    strcpy(file2, buffer);


    if (pipe(pipe1) == -1) {
        const char* msg = "Error: failed to create pipe1\n";
        write(STDERR_FILENO, msg, strlen(msg) + 1);
        exit(EXIT_FAILURE);
    }

    if (pipe(pipe2) == -1) {
        const char* msg = "Error: failed to create pipe2\n";
        write(STDERR_FILENO, msg, strlen(msg) + 1);
        exit(EXIT_FAILURE);
    }

    child1_pid = fork();
    if (child1_pid == -1) {
        const char* msg = "Error: failed to spawn new proccess\n";
        write(STDERR_FILENO, msg, strlen(msg) + 1);
        exit(EXIT_FAILURE);
    }
    else if (child1_pid == 0) {
        pid_t child1_pid = getpid();
        dup2(pipe1[0], STDIN_FILENO);

        char* args[] = {"./child", file1, NULL};
        int status = execv("./child", args);

        if (status == -1) {
            const char* msg = "Failed to exec child1\n";
            write(STDERR_FILENO, msg, strlen(msg) + 1);
            exit(EXIT_FAILURE);
        }
    }

    child2_pid = fork();
    if (child2_pid == -1) {
        const char* msg = "Error: failed to spawn new proccess\n";
        write(STDERR_FILENO, msg, strlen(msg) + 1);
        exit(EXIT_FAILURE);
    } else if (child2_pid == 0) {
        dup2(pipe2[0], STDIN_FILENO);
        char* args[] = {"./child", file2, NULL};
        int status = execv("./child", args);

        if (status == -1) {
            const char* msg = "Failed to exec child2\n";
            write(STDERR_FILENO, msg, strlen(msg) + 1);
            exit(EXIT_FAILURE);
        }
    }

    close(pipe1[0]);
    close(pipe2[0]);



    while(bytes = read(STDIN_FILENO, buffer, BUFSIZ)) {
        if (bytes < 0) {
            const char* msg = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        if (buffer[0] == '\n') {
            break;
        }

        buffer[bytes - 1] = '\0';
        filter_data(pipe1, pipe2, buffer);
    }



    close(pipe1[1]);
    close(pipe2[1]);

    int child1_status;
    int child2_status;

    wait(&child1_status);
    wait(&child2_status);

    return 0;
}
