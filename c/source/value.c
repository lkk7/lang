#include "value.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"

bool are_equal(Value a, Value b) {
#ifdef NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  return a == b;
#else
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
    case VAL_OBJ: {
      return AS_OBJ(a) == AS_OBJ(b);
    }
    default:
      return false;
  }
#endif
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
#ifdef NAN_BOXING
  if (IS_BOOL(val)) {
    printf(AS_BOOL(val) ? "true" : "false");
  } else if (IS_NIL(val)) {
    printf("nil");
  } else if (IS_NUMBER(val)) {
    printf("%g", AS_NUMBER(val));
  } else if (IS_OBJ(val)) {
    print_obj(val);
  }
#else
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
    case VAL_OBJ:
      print_obj(val);
      break;
  }
#endif
}
