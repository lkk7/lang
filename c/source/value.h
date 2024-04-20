#pragma once

#include <string.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjStr ObjStr;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1    // 01.
#define TAG_FALSE 2  // 10.
#define TAG_TRUE 3   // 11.

typedef uint64_t Value;

#define IS_BOOL(val) (((val) | 1) == TRUE_VAL)
#define IS_NIL(val) ((val) == NIL_VAL)
#define IS_NUMBER(val) (((val) & QNAN) != QNAN)
#define IS_OBJ(val) (((val) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(val) ((val) == TRUE_VAL)
#define AS_NUMBER(val) val_to_num(val)
#define AS_OBJ(val) ((Obj*)(uintptr_t)((val) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(val) num_to_val(val)
#define OBJ_VAL(obj) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline double val_to_num(Value value) {
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

static inline Value num_to_val(double num) {
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

#else

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

#endif

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
