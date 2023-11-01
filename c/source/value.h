#pragma once

#include "common.h"

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as;
} Value;

#define IS_BOOL(val) ((val).type == VAL_BOOL)
#define IS_NIL(val) ((val).type == VAL_NIL)
#define IS_NUMBER(val) ((val).type == VAL_NUMBER)

#define AS_BOOL(val) ((val).as.boolean)
#define AS_NUMBER(val) ((val).as.number)

#define BOOL_VAL(val) ((Value){VAL_BOOL, {.boolean = (val)}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(val) ((Value){VAL_NUMBER, {.number = (val)}})

typedef struct {
  int size;
  int capacity;
  Value* vals;
} ValueArray;

bool are_equal(Value a, Value b);
void init_valarr(ValueArray* arr);
void write_valarr(ValueArray* arr, Value val);
void free_valarr(ValueArray* arr);

void print_val(Value val);
