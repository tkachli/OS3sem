#include <stddef.h>

typedef struct Allocator {
    void* memory_start;
    size_t memory_size;
    void* free_list;
} Allocator;

typedef struct FreeBlock {
    size_t size;
    struct FreeBlock* next;
} FreeBlock;

Allocator* allocator_create(void* const memory, const size_t size) {
    if (!memory || size < sizeof(FreeBlock)) {
        return NULL;
    }

    Allocator* allocator = (Allocator*)memory;
    allocator->memory_start = (char*)memory + sizeof(Allocator);
    allocator->memory_size = size - sizeof(Allocator);
    allocator->free_list = allocator->memory_start;

    FreeBlock* initial_block = (FreeBlock*)allocator->memory_start;
    initial_block->size = allocator->memory_size;
    initial_block->next = NULL;

    return allocator;
}

void allocator_destroy(Allocator* const allocator) {
    allocator->memory_start = NULL;
    allocator->memory_size = 0;
    allocator->free_list = NULL;
}

void* allocator_alloc(Allocator* const allocator, const size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    FreeBlock* prev = NULL;
    FreeBlock* curr = (FreeBlock*)allocator->free_list;

    while (curr) {
        if (curr->size >= size + sizeof(FreeBlock)) {
            if (curr->size > size + sizeof(FreeBlock)) {
                FreeBlock* new_block = (FreeBlock*)((char*)curr + sizeof(FreeBlock) + size);
                new_block->size = curr->size - size - sizeof(FreeBlock);
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            if (prev) {
                prev->next = curr->next;
            } else {
                allocator->free_list = curr->next;
            }

            return (char*)curr + sizeof(FreeBlock);
        }

        prev = curr;
        curr = curr->next;
    }

    return NULL;
}

void allocator_free(Allocator* const allocator, void* const memory) {
    if (!allocator || !memory) {
        return;
    }

    FreeBlock* block = (FreeBlock*)((char*)memory - sizeof(FreeBlock));
    block->next = (FreeBlock*)allocator->free_list;
    allocator->free_list = block;
}