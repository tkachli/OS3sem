#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_NAME "/shm_example"
#define SEM_CHILD1 "/sem_child1"
#define SHM_SIZE 1024

void HandleError(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, "\n", 1);
    exit(1);
}

void RemoveVowels(char *str) {
    char *p = str, *q = str;
    while (*p) {
        if (!strchr("AEIOUaeiou", *p)) {
            *q++ = *p;
        }
        p++;
    }
    *q = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        HandleError("Usage: <program> <output_file>");
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        HandleError("Cannot open file");
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0644);
    if (shm_fd == -1) {
        HandleError("Ошибка доступа к разделяемой памяти");
    }

    char *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        HandleError("Ошибка отображения разделяемой памяти");
    }

    sem_t *sem_child1 = sem_open(SEM_CHILD1, 0);
    if (sem_child1 == SEM_FAILED) {
        HandleError("Ошибка доступа к семафору");
    }

    while (1) {
        sem_wait(sem_child1);

        char buffer[SHM_SIZE];
        strncpy(buffer, shm_ptr, SHM_SIZE - 1);
        buffer[SHM_SIZE - 1] = '\0';

        RemoveVowels(buffer);
        write(fd, buffer, strlen(buffer));
        write(fd, "\n", 1);
    }

    munmap(shm_ptr, SHM_SIZE);
    close(fd);

    return 0;
}