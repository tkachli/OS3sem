#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#define SHM_NAME "/shm_example"
#define SEM_CHILD1 "/sem_child1"
#define SEM_CHILD2 "/sem_child2"

#define SHM_SIZE 1024

void HandleError(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    write(STDERR_FILENO, "\n", 1);
    exit(1);
}

void Print(const char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

ssize_t Getline(char **lineptr, size_t *n, int fd) {
    if (*lineptr == NULL) {
        *lineptr = malloc(128);
        *n = 128;
    }

    size_t pos = 0;
    char c;
    while (read(fd, &c, 1) == 1) {
        if (pos >= *n - 1) {
            *n *= 2;
            *lineptr = realloc(*lineptr, *n);
        }
        (*lineptr)[pos++] = c;
        if (c == '\n') {
            break;
        }
    }

    if (pos == 0) {
        return -1;
    }

    (*lineptr)[pos] = '\0';
    return pos;
}

int main() {
    char *file1 = NULL, *file2 = NULL;
    size_t file_len = 0;
    char *input = NULL;
    size_t len = 0;
    ssize_t nread;

    Print("Введите имя файла для дочернего процесса 1: ");
    Getline(&file1, &file_len, STDIN_FILENO);
    file1[strcspn(file1, "\n")] = 0;

    Print("Введите имя файла для дочернего процесса 2: ");
    Getline(&file2, &file_len, STDIN_FILENO);
    file2[strcspn(file2, "\n")] = 0;

    // Создаем shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd == -1) {
        HandleError("Ошибка создания разделяемой памяти");
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        HandleError("Ошибка установки размера разделяемой памяти");
    }

    char *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        HandleError("Ошибка отображения разделяемой памяти");
    }

    // Создаем семафоры
    sem_t *sem_child1 = sem_open(SEM_CHILD1, O_CREAT, 0644, 0);
    sem_t *sem_child2 = sem_open(SEM_CHILD2, O_CREAT, 0644, 0);
    if (sem_child1 == SEM_FAILED || sem_child2 == SEM_FAILED) {
        HandleError("Ошибка создания семафоров");
    }

    pid_t child1, child2;

    if ((child1 = fork()) == 0) {
        execlp("./child1", "./child1", file1, NULL);
        HandleError("Ошибка запуска дочернего процесса 1");
    }

    if ((child2 = fork()) == 0) {
        execlp("./child2", "./child2", file2, NULL);
        HandleError("Ошибка запуска дочернего процесса 2");
    }

    srand(time(NULL));

    while (1) {
        Print("Введите строку (или 'exit' для завершения): ");
        nread = Getline(&input, &len, STDIN_FILENO);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Копируем данные в shared memory
        strncpy(shm_ptr, input, SHM_SIZE - 1);

        int r = rand() % 5 + 1;
        //printf("r = %d\n", r);
        if (r == 3) {
            sem_post(sem_child2);
        } else {
            sem_post(sem_child1);
        }
    }

    Print("Работа завершена.\n");

    // Удаляем ресурсы
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_CHILD1);
    sem_unlink(SEM_CHILD2);


    free(file1);
    free(file2);
    free(input);

    return 0;
}