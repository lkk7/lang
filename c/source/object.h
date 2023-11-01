#pragma once

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STR(value) is_obj_type(value, OBJ_STR)
#define AS_STR(value) ((ObjStr*)AS_OBJ(value))
#define AS_CSTR(value) (((ObjStr*)AS_OBJ(value))->chars)

typedef enum { OBJ_STR } ObjType;

struct Obj {
  ObjType type;
  Obj* next;
};

struct ObjStr {
  Obj obj;
  int length;
  char* chars;
};

ObjStr* take_str(char* chars, int length);
ObjStr* copy_str(const char* chars, int length);

void print_obj(Value value);

static inline bool is_obj_type(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
