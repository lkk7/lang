#pragma once

#include "bytecode.h"
#include "value.h"

#define OBJ_TYPE(val) (AS_OBJ(val)->type)
#define IS_CLOSURE(val) isObjType(val, OBJ_CLOSURE)
#define IS_FUNCTION(val) is_obj_type(val, OBJ_FUNCTION)
#define IS_NATIVE(val) isObjType(val, OBJ_NATIVE)
#define IS_STR(val) is_obj_type(val, OBJ_STR)

#define AS_CLOSURE(val) ((ObjClosure*)AS_OBJ(val))
#define AS_FUNCTION(val) ((ObjFunction*)AS_OBJ(val))
#define AS_NATIVE(val) (((ObjNative*)AS_OBJ(val))->function)
#define AS_STR(val) ((ObjStr*)AS_OBJ(val))
#define AS_CSTR(val) (((ObjStr*)AS_OBJ(val))->chars)

typedef enum {
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STR,
  OBJ_UPVALUE,
} ObjType;

struct Obj {
  ObjType type;
  Obj* next;
};

typedef struct {
  Obj obj;
  int arity;
  int upvalue_cnt;
  ByteSequence bseq;
  ObjStr* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

struct ObjStr {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};

typedef struct ObjUpvalue {
  Obj obj;
  Value* location;
  Value closed;
  struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalue_cnt;
} ObjClosure;

ObjClosure* new_closure(ObjFunction* function);
ObjFunction* new_function();
ObjNative* new_native(NativeFn function);
ObjStr* take_str(char* chars, int length);
ObjStr* copy_str(const char* chars, int length);
ObjUpvalue* new_upvalue(Value* slot);

void print_obj(Value val);

static inline bool is_obj_type(Value val, ObjType type) {
  return IS_OBJ(val) && AS_OBJ(val)->type == type;
}
