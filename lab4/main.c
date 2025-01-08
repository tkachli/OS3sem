#include <dlfcn.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

typedef struct Allocator {
    void *(*allocator_create)(void *addr, size_t size);
    void *(*allocator_alloc)(void *allocator, size_t size);
    void (*allocator_free)(void *allocator, void *ptr);
    void (*allocator_destroy)(void *allocator);
} Allocator;

void *standard_allocator_create(void *memory, size_t size) {
    (void)size;
    (void)memory;
    return memory;
}

void *standard_allocator_alloc(void *allocator, size_t size) {
    (void)allocator;
    uint32_t *memory =
        mmap(NULL, size + sizeof(uint32_t), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    *memory = (uint32_t)(size + sizeof(uint32_t));
    return memory + 1;
}

void standard_allocator_free(void *allocator, void *memory) {
    (void)allocator;
    if (memory == NULL) return;
    uint32_t *mem = (uint32_t *)memory - 1;
    munmap(mem, *mem);
}

void standard_allocator_destroy(void *allocator) { (void)allocator; }

void load_allocator(const char *library_path, Allocator *allocator) {
    void *library = dlopen(library_path, RTLD_LOCAL | RTLD_NOW);
    if (library_path == NULL || library_path[0] == '\0' || !library) {
        char message[] = "WARNING: failed to load shared library\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        allocator->allocator_create = standard_allocator_create;
        allocator->allocator_alloc = standard_allocator_alloc;
        allocator->allocator_free = standard_allocator_free;
        allocator->allocator_destroy = standard_allocator_destroy;
        return;
    }

    allocator->allocator_create = dlsym(library, "allocator_create");
    allocator->allocator_alloc = dlsym(library, "allocator_alloc");
    allocator->allocator_free = dlsym(library, "allocator_free");
    allocator->allocator_destroy = dlsym(library, "allocator_destroy");

    if (!allocator->allocator_create || !allocator->allocator_alloc ||
        !allocator->allocator_free || !allocator->allocator_destroy) {
        const char msg[] = "Error: failed to load all allocator functions\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        dlclose(library);
        return;
    }
}

void write_message(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void write_address(const char *prefix, int index, void *address) {
    char buffer[64];
    int len = 0;
    while (prefix[len] != '\0') {
        buffer[len] = prefix[len];
        len++;
    }
    buffer[len++] = ' ';
    if (index < 10) {
        buffer[len++] = '0' + index;
    } else {
        buffer[len++] = '0' + (index / 10);
        buffer[len++] = '0' + (index % 10);
    }
    buffer[len++] = ' ';
    buffer[len++] = 'a';
    buffer[len++] = 'd';
    buffer[len++] = 'd';
    buffer[len++] = 'r';
    buffer[len++] = 'e';
    buffer[len++] = 's';
    buffer[len++] = 's';
    buffer[len++] = ':';
    buffer[len++] = ' ';
    uintptr_t addr = (uintptr_t)address;
    for (int i = (sizeof(uintptr_t) * 2) - 1; i >= 0; --i) {
        int nibble = (addr >> (i * 4)) & 0xF;
        buffer[len++] = (nibble < 10) ? ('0' + nibble) : ('a' + (nibble - 10));
    }
    buffer[len++] = '\n';
    write(STDOUT_FILENO, buffer, len);
}

long get_time_in_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

int main(int argc, char **argv) {
    const char *library_path = (argc > 1) ? argv[1] : NULL;
    Allocator allocator_api;
    load_allocator(library_path, &allocator_api);

    size_t size = 4096;
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        char message[] = "mmap failed\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        return EXIT_FAILURE;
    }

    void *allocator = allocator_api.allocator_create(addr, size);
    if (!allocator) {
        char message[] = "Failed to initialize allocator\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        munmap(addr, size);
        return EXIT_FAILURE;
    }

    long start_time = get_time_in_us();
    void *blocks[12];
    size_t block_sizes[12] = {32,  128, 8, 24, 256, 56, 128, 8, 32, 120, 8, 64};

    int alloc_failed = 0;
    for (int i = 0; i < 12; ++i) {
        blocks[i] = allocator_api.allocator_alloc(allocator, block_sizes[i]);
        if (blocks[i] == NULL) {
            alloc_failed = 1;
            char alloc_fail_message[] = "Memory allocation failed\n";
            write(STDERR_FILENO, alloc_fail_message,
                  sizeof(alloc_fail_message) - 1);
            break;
        }
    }
    long alloc_time = get_time_in_us() - start_time;

    if (!alloc_failed) {
        char alloc_success_message[] = "Memory allocated successfully\n";
        write(STDOUT_FILENO, alloc_success_message,
              sizeof(alloc_success_message) - 1);

        for (int i = 0; i < 12; ++i) {
            write_address("Block", i + 1, blocks[i]);
        }
    }

    // Benchmark the deallocation
    start_time = get_time_in_us();
    for (int i = 0; i < 12; ++i) {
        if (blocks[i] != NULL)
            allocator_api.allocator_free(allocator, blocks[i]);
    }
    long free_time = get_time_in_us() - start_time;

    size_t total_allocated = 0;
    size_t total_used = 0;

    for (int i = 0; i < 12; ++i) {
        blocks[i] = allocator_api.allocator_alloc(allocator, block_sizes[i]);
        if (blocks[i] != NULL) {
            total_allocated += block_sizes[i] + sizeof(uint32_t);  // Включаем размер заголовка
            total_used += block_sizes[i];
        }
    }

    // Фактор использования
    double usage_factor = (double)total_used / total_allocated;

    char free_message[] = "Memory freed\n";
    write(STDOUT_FILENO, free_message, sizeof(free_message) - 1);

    allocator_api.allocator_destroy(allocator);

    char exit_message[] = "Program exited successfully\n";
    write(STDOUT_FILENO, exit_message, sizeof(exit_message) - 1);

    // Print the benchmark results
    char result_message[128];
    snprintf(result_message, sizeof(result_message), "Allocation time: %ld us\nDeallocation time: %ld us\n", alloc_time, free_time);
    write(STDOUT_FILENO, result_message, strlen(result_message));
    printf("Usage Factor: %.2f\n", usage_factor);

    return EXIT_SUCCESS;
}
