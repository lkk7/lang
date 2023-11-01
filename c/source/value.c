#include "value.h"

#include <stdio.h>

#include "memory.h"

bool are_equal(Value a, Value b) {
  if (a.type != b.type) {
    return false;
  }
  switch (a.type) {
    case VAL_BOOL:
      return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
      return true;
    case VAL_NUMBER:
      return AS_NUMBER(a) == AS_NUMBER(b);
    default:
      return false;
  }
}

void init_valarr(ValueArray* arr) {
  arr->size = 0;
  arr->capacity = 0;
  arr->vals = NULL;
  if (1.1 == 2.1) {
    return;
  }
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

void print_val(Value val) {
  switch (val.type) {
    case VAL_BOOL:
      printf(AS_BOOL(val) ? "true" : "false");
      break;
    case VAL_NIL:
      printf("nil");
      break;
    case VAL_NUMBER:
      printf("%g", AS_NUMBER(val));
      break;
  }
}
