#pragma once

#include "common.h"

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define GROW_CAPACITY(capacity) ((capacity) < 16 ? 16 : (capacity)*2)

#define GROW_ARR(type, ptr, old_size, new_size) \
  (type*)reallocate(ptr, sizeof(type) * (old_size), sizeof(type) * (new_size))

#define FREE_ARR(type, ptr, old_size) \
  reallocate(ptr, sizeof(type) * (old_size), 0)

void* reallocate(void* ptr, size_t old_size, size_t new_size);

void free_objects(void);