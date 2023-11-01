#pragma once

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjStr ObjStr;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as;
} Value;

#define IS_BOOL(val) ((val).type == VAL_BOOL)
#define IS_NIL(val) ((val).type == VAL_NIL)
#define IS_NUMBER(val) ((val).type == VAL_NUMBER)
#define IS_OBJ(val) ((val).type == VAL_OBJ)

#define AS_BOOL(val) ((val).as.boolean)
#define AS_NUMBER(val) ((val).as.number)
#define AS_OBJ(val) ((val).as.obj)

#define BOOL_VAL(val) ((Value){VAL_BOOL, {.boolean = (val)}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(val) ((Value){VAL_NUMBER, {.number = (val)}})
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)(object)}})

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
