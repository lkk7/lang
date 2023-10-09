#include "value.h"

#include <stdio.h>

#include "memory.h"

void init_valarr(ValueArray* arr) {
  arr->size = 0;
  arr->capacity = 0;
  arr->vals = NULL;
}

void write_valarr(ValueArray* arr, Value val) {
  if (arr->capacity < arr->size + 1) {
    int capacity = arr->capacity;
    arr->capacity = GROW_CAPACITY(capacity);
    arr->vals =
        GROW_ARR(Value, arr->vals, (size_t)capacity, (size_t)arr->capacity);
  }
  arr->vals[arr->size] = val;
  ++(arr->size);
}

void free_valarr(ValueArray* arr) {
  FREE_ARR(Value, arr->vals, (size_t)arr->capacity);
  init_valarr(arr);
}

void print_val(Value val) { printf("%g", val); }
