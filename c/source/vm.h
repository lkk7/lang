#pragma once

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frame_count;
  ObjUpvalue* open_upvalues;
  Value stack[STACK_MAX];
  Value* top;
  Table globals;
  Table strings;
  ObjStr* init_str;
  Obj* objects;
  int gray_cnt;
  int gray_capacity;
  Obj** gray_stack;
  size_t bytes_allocated;
  size_t next_gc;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void init_vm(void);
void free_vm(void);
InterpretResult interpret(const char* src);
void push(Value val);
Value pop(void);
