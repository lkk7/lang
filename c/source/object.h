#pragma once

#include "bytecode.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(val) (AS_OBJ(val)->type)

#define IS_BOUND_METHOD(val) is_obj_type(val, OBJ_BOUND_METHOD)
#define IS_CLASS(val) is_obj_type(val, OBJ_CLASS)
#define IS_CLOSURE(val) is_obj_type(val, OBJ_CLOSURE)
#define IS_FUNCTION(val) is_obj_type(val, OBJ_FUNCTION)
#define IS_INSTANCE(val) is_obj_type(val, OBJ_INSTANCE)
#define IS_NATIVE(val) is_obj_type(val, OBJ_NATIVE)
#define IS_STR(val) is_obj_type(val, OBJ_STR)

#define AS_BOUND_METHOD(val) ((ObjBoundMethod*)AS_OBJ(val))
#define AS_CLASS(val) ((ObjClass*)AS_OBJ(val))
#define AS_CLOSURE(val) ((ObjClosure*)AS_OBJ(val))
#define AS_FUNCTION(val) ((ObjFunction*)AS_OBJ(val))
#define AS_INSTANCE(val) ((ObjInstance*)AS_OBJ(val))
#define AS_NATIVE(val) (((ObjNative*)AS_OBJ(val))->function)
#define AS_STR(val) ((ObjStr*)AS_OBJ(val))
#define AS_CSTR(val) (((ObjStr*)AS_OBJ(val))->chars)

typedef enum {
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STR,
  OBJ_UPVALUE,
} ObjType;

struct Obj {
  ObjType type;
  bool is_marked;
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

typedef struct {
  Obj obj;
  ObjStr* name;
  Table methods;
} ObjClass;

typedef struct {
  Obj obj;
  ObjClass* cls;
  Table fields;
} ObjInstance;

typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method);
ObjClass* new_class(ObjStr* name);
ObjClosure* new_closure(ObjFunction* function);
ObjFunction* new_function();
ObjInstance* new_instance(ObjClass* cls);
ObjNative* new_native(NativeFn function);
ObjStr* take_str(char* chars, int length);
ObjStr* copy_str(const char* chars, int length);
ObjUpvalue* new_upvalue(Value* slot);

void print_obj(Value val);

static inline bool is_obj_type(Value val, ObjType type) {
  return IS_OBJ(val) && AS_OBJ(val)->type == type;
}
